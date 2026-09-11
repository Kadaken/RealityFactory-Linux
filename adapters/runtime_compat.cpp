#define RF_PLATFORM_NATIVE_STDIO
#include "rf_platform_compat.h"

#include <Genesis.h>

#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>
#include <time.h>
#include <dirent.h>
#include <set>
#include <string>

extern "C" int RF_HostFatalShutdown(void) __attribute__((weak));
extern "C" int RF_HostBootStage(void) __attribute__((weak));
extern "C" int __lsan_do_recoverable_leak_check(void) __attribute__((weak));

extern "C" char *rf_fgets(char *buffer, int size, FILE *stream)
{
    char *result = ::fgets(buffer, size, stream);
    if (!result) return NULL;
    const size_t length = strlen(buffer);
    if (length >= 2 && buffer[length-2] == '\r' && buffer[length-1] == '\n') {
        buffer[length-2] = '\n'; buffer[length-1] = '\0';
    } else if (length && buffer[length-1] == '\r') {
        // A full buffer may split CRLF. Consume only its matching LF, not
        // the next record's first byte; preserve normal fgets chunk limits.
        if ((int)length == size-1) {
            int next = fgetc(stream);
            if (next != '\n' && next != EOF) ungetc(next, stream);
        }
        buffer[length-1] = '\n';
    }
    return result;
}

extern "C" void rf_fatal_exit(int code)
{
    static int entered;
    // Legacy fatal sites use -1; expose that as the CLI failure status 1.
    // Preserve explicit nonnegative process statuses.
    const int status = code < 0 ? 1 : code;
    if (__sync_lock_test_and_set(&entered, 1)) {
        static const char msg[] = "RF fatal: re-entry during teardown; terminating\n";
        write(STDERR_FILENO, msg, sizeof(msg)-1);
        _exit(1);
    }
    fprintf(stderr, "RF fatal: stage=%d code=%d exit=%d\n",
            RF_HostBootStage ? RF_HostBootStage() : 0, code, status);
    if (RF_HostFatalShutdown && !RF_HostFatalShutdown()) _exit(2);
    fflush(NULL);
    if (__lsan_do_recoverable_leak_check) {
        const int leaks = __lsan_do_recoverable_leak_check();
        fprintf(stderr, "RF fatal: explicit LSan check=%d\n", leaks);
        if (leaks) _exit(2);
    }
    _exit(status);
}

extern "C" int rf_case_path(const char *path, char *resolved, size_t capacity, int route)
{
    if (!path || !*path || !resolved || capacity == 0) return -1;
    std::string input(path), current = input[0] == '/' ? "/" : ".";
    for (size_t i=0; i<input.size(); ++i) if (input[i]=='\\') input[i]='/';
    size_t start = input[0]=='/' ? 1 : 0;
    while (start < input.size()) {
        size_t end = input.find('/', start);
        if (end == std::string::npos) end = input.size();
        const std::string component = input.substr(start, end-start);
        start = end+1;
        if (component.empty() || component==".") continue;
        if (component=="..") { current += "/.."; continue; }
        DIR *directory = opendir(current.c_str());
        if (!directory) return 0;
        unsigned matches = 0;
        std::string found;
        errno = 0;
        while (dirent *entry = readdir(directory)) {
            if (!strcasecmp(entry->d_name, component.c_str())) { ++matches; found=entry->d_name; }
        }
        const int read_error = errno;
        closedir(directory);
        if (read_error) return -1;
        if (matches > 1) {
            fprintf(stderr, "case-fallback: route-%d ambiguous component; resolve duplicate casing\n", route);
            return -1;
        }
        if (!matches) return 0;
        if (current[current.size()-1] != '/') current += "/";
        current += found;
    }
    if (current.size() >= capacity) return -1;
    memcpy(resolved, current.c_str(), current.size()+1);
    static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
    static std::set<std::pair<std::string, int> > logged;
    // Only diagnostics are deduplicated; lookup results are never cached.
    struct Lock {
        pthread_mutex_t *mutex;
        Lock(pthread_mutex_t *m) : mutex(m) { pthread_mutex_lock(mutex); }
        ~Lock() { pthread_mutex_unlock(mutex); }
    } lock(&log_mutex);
    if (logged.insert(std::make_pair(input, route)).second)
        fprintf(stderr, "case-fallback: route-%d\n", route);
    return 1;
}

extern char **environ;

namespace
{
geVFile *PlainVfs = NULL;

struct PeriodicTimer
{
	pthread_t thread;
	pthread_mutex_t mutex;
	bool active;
	bool stopping;
	bool stop;
	MMRESULT id;
	UINT period_ms;
	LPTIMECALLBACK callback;
	DWORD_PTR user;
};

const size_t MaxPeriodicTimers = 64;
PeriodicTimer PeriodicTimers[MaxPeriodicTimers];
pthread_mutex_t PeriodicTimersMutex = PTHREAD_MUTEX_INITIALIZER;
MMRESULT NextTimerId = 1;

void AddMilliseconds(timespec *deadline, UINT milliseconds)
{
	deadline->tv_sec += milliseconds / 1000u;
	deadline->tv_nsec += (long)(milliseconds % 1000u) * 1000000L;
	if(deadline->tv_nsec >= 1000000000L)
	{
		++deadline->tv_sec;
		deadline->tv_nsec -= 1000000000L;
	}
}

bool DeadlinePassed(const timespec &deadline, const timespec &now)
{
	return deadline.tv_sec < now.tv_sec ||
		(deadline.tv_sec == now.tv_sec && deadline.tv_nsec <= now.tv_nsec);
}

void *PeriodicTimerMain(void *context)
{
	PeriodicTimer *timer = static_cast<PeriodicTimer *>(context);
	timespec deadline;
	clock_gettime(CLOCK_MONOTONIC, &deadline);
	AddMilliseconds(&deadline, timer->period_ms);

	for(;;)
	{
		int sleep_result;
		do
		{
			sleep_result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
				&deadline, NULL);
		} while(sleep_result == EINTR);

		pthread_mutex_lock(&timer->mutex);
		const bool stop = timer->stop;
		pthread_mutex_unlock(&timer->mutex);
		if(stop)
			break;

		timer->callback(timer->id, 0, timer->user, 0, 0);
		AddMilliseconds(&deadline, timer->period_ms);
		timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		if(DeadlinePassed(deadline, now))
		{
			deadline = now;
			AddMilliseconds(&deadline, timer->period_ms);
		}
	}
	return NULL;
}
}

extern "C" void OutputDebugString(const char *text)
{
	if(text != NULL)
		fputs(text, stderr);
}

extern "C" char *_strupr(char *text)
{
	if(text == NULL)
		return NULL;

	for(char *cursor = text; *cursor != '\0'; ++cursor)
		*cursor = (char)toupper((unsigned char)*cursor);
	return text;
}

extern "C" char *itoa(int value, char *buffer, int radix)
{
	static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	unsigned int magnitude;
	char reversed[sizeof(unsigned int) * CHAR_BIT + 1];
	size_t length = 0;
	bool negative = false;

	if(buffer == NULL || radix < 2 || radix > 36)
		return NULL;

	if(radix == 10 && value < 0)
	{
		negative = true;
		magnitude = 0u - (unsigned int)value;
	}
	else
	{
		magnitude = (unsigned int)value;
	}

	do
	{
		reversed[length++] = digits[magnitude % (unsigned int)radix];
		magnitude /= (unsigned int)radix;
	} while(magnitude != 0u);

	char *output = buffer;
	if(negative)
		*output++ = '-';
	while(length != 0)
		*output++ = reversed[--length];
	*output = '\0';
	return buffer;
}

extern "C" intptr_t _spawnv(int mode, const char *path,
	const char *const *arguments)
{
	pid_t child;
	pid_t waited;
	int status;

	if(mode != _P_WAIT || path == NULL || arguments == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	const int spawn_result = posix_spawn(&child, path, NULL, NULL,
		const_cast<char *const *>(arguments), environ);
	if(spawn_result != 0)
	{
		errno = spawn_result;
		return -1;
	}

	do
	{
		waited = waitpid(child, &status, 0);
	} while(waited < 0 && errno == EINTR);
	if(waited < 0)
		return -1;

	if(WIFEXITED(status))
		return (intptr_t)WEXITSTATUS(status);
	if(WIFSIGNALED(status))
		return (intptr_t)(128 + WTERMSIG(status));
	return -1;
}

geVFile *PassWord(char *virtual_file, bool encrypt)
{
	if(encrypt)
	{
		fprintf(stderr,
			"Reality Factory Linux: encrypted CF00 VFS archives are unsupported.\n");
		return NULL;
	}

	if(virtual_file == NULL || *virtual_file == '\0')
		return NULL;

	if(PlainVfs != NULL)
	{
		geVFile_Close(PlainVfs);
		PlainVfs = NULL;
	}

	PlainVfs = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_VIRTUAL,
		virtual_file, NULL, GE_VFILE_OPEN_READONLY | GE_VFILE_OPEN_DIRECTORY);
	if (!PlainVfs)
		fprintf(stderr, "Reality Factory Linux: archive directory open failed; check format and read access.\n");
	return PlainVfs;
}

void CloseFile()
{
	if(PlainVfs != NULL)
	{
		geVFile_Close(PlainVfs);
		PlainVfs = NULL;
	}
}

extern "C" DWORD timeGetTime(void)
{
	timespec now;
	if(clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return 0;
	const uint64_t milliseconds = (uint64_t)now.tv_sec * 1000u +
		(uint64_t)now.tv_nsec / 1000000u;
	return (DWORD)milliseconds;
}

extern "C" MMRESULT timeSetEvent(UINT delay, UINT, LPTIMECALLBACK callback,
	DWORD_PTR user, UINT event_type)
{
	PeriodicTimer *timer = NULL;
	if(delay == 0 || callback == NULL ||
		(event_type & (TIME_PERIODIC | TIME_CALLBACK_FUNCTION)) !=
			(TIME_PERIODIC | TIME_CALLBACK_FUNCTION))
		return 0;

	pthread_mutex_lock(&PeriodicTimersMutex);
	for(size_t i = 0; i < MaxPeriodicTimers; ++i)
	{
		if(!PeriodicTimers[i].active)
		{
			timer = &PeriodicTimers[i];
			timer->active = true;
			/* Reserve the slot until pthread_create publishes a valid thread. */
			timer->stopping = true;
			timer->stop = false;
			timer->id = NextTimerId++;
			if(NextTimerId == 0)
				NextTimerId = 1;
			timer->period_ms = delay;
			timer->callback = callback;
			timer->user = user;
			pthread_mutex_init(&timer->mutex, NULL);
			break;
		}
	}
	if(timer == NULL)
	{
		pthread_mutex_unlock(&PeriodicTimersMutex);
		return 0;
	}

	if(pthread_create(&timer->thread, NULL, PeriodicTimerMain, timer) != 0)
	{
		pthread_mutex_destroy(&timer->mutex);
		timer->active = false;
		pthread_mutex_unlock(&PeriodicTimersMutex);
		return 0;
	}
	timer->stopping = false;
	pthread_mutex_unlock(&PeriodicTimersMutex);
	return timer->id;
}

extern "C" MMRESULT timeKillEvent(UINT timer_id)
{
	PeriodicTimer *timer = NULL;
	pthread_mutex_lock(&PeriodicTimersMutex);
	for(size_t i = 0; i < MaxPeriodicTimers; ++i)
	{
		if(PeriodicTimers[i].active && !PeriodicTimers[i].stopping &&
			PeriodicTimers[i].id == timer_id)
		{
			timer = &PeriodicTimers[i];
			timer->stopping = true;
			break;
		}
	}
	pthread_mutex_unlock(&PeriodicTimersMutex);
	if(timer == NULL)
		return 1;

	pthread_mutex_lock(&timer->mutex);
	timer->stop = true;
	pthread_mutex_unlock(&timer->mutex);
	pthread_join(timer->thread, NULL);

	pthread_mutex_lock(&PeriodicTimersMutex);
	pthread_mutex_destroy(&timer->mutex);
	timer->active = false;
	timer->stopping = false;
	pthread_mutex_unlock(&PeriodicTimersMutex);
	return 0;
}

extern "C" void AVIFileInit(void)
{
	/* Lifecycle boundary only. FFmpeg initialization is not required. */
	fprintf(stderr, "RF [unavailable] AVIFile: lifecycle only; native video decoding is not implemented\n");
}

extern "C" void AVIFileExit(void)
{
	/* Lifecycle boundary only. No decoder-global state is owned here. */
}
