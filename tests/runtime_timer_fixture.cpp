#include "rf_platform_compat.h"

#include <pthread.h>
#include <unistd.h>

namespace
{
pthread_mutex_t CounterMutex = PTHREAD_MUTEX_INITIALIZER;
unsigned int CallbackCount = 0;

void CALLBACK CountTimer(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR)
{
	pthread_mutex_lock(&CounterMutex);
	++CallbackCount;
	pthread_mutex_unlock(&CounterMutex);
}

unsigned int ReadCount()
{
	pthread_mutex_lock(&CounterMutex);
	const unsigned int count = CallbackCount;
	pthread_mutex_unlock(&CounterMutex);
	return count;
}
}

int main()
{
	const DWORD before = timeGetTime();
	usleep(12000);
	const DWORD after = timeGetTime();
	if((DWORD)(after - before) < 8u)
		return 1;

	const MMRESULT timer = timeSetEvent(5, 0, &CountTimer, 0,
		TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
	if(timer == 0)
		return 2;
	usleep(40000);
	if(timeKillEvent(timer) != 0)
		return 3;
	const unsigned int stopped_count = ReadCount();
	if(stopped_count < 3u || stopped_count > 12u)
		return 4;
	usleep(15000);
	if(ReadCount() != stopped_count)
		return 5;

	AVIFileInit();
	AVIFileExit();
	return 0;
}
