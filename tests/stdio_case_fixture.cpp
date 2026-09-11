#include "rf_platform_compat.h"
#include <string>
#include <sys/stat.h>

static bool line(const std::string &input, const std::string &expected, int capacity = 132)
{
    FILE *file = tmpfile();
    if (!file) return false;
    const bool written = fwrite(input.data(), 1, input.size(), file) == input.size();
    rewind(file);
    char buffer[256];
    std::string result;
    while (fgets(buffer, capacity, file)) result += buffer;
    const bool ok = written && !ferror(file) && result == expected;
    fclose(file);
    return ok;
}

static bool touch(const std::string &path)
{
    FILE *f = fopen(path.c_str(), "wb");
    return f && fclose(f) == 0;
}

int main()
{
    if (!line("alpha\r\nbeta\r\n", "alpha\nbeta\n") ||
        !line("alpha\nbeta\n", "alpha\nbeta\n") ||
        !line("alpha\r", "alpha\n") || !line("alpha", "alpha") ||
        !line(std::string(131, 'a') + "\r\nnext\r\n", std::string(131, 'a') + "\nnext\n") ||
        !line(std::string(130, 'a') + "\r\nnext\n", std::string(130, 'a') + "\nnext\n") ||
        !line("abc\rx", "abc\nx", 5)) return 1;
    char root[] = "/tmp/rf-case-XXXXXX";
    if (!mkdtemp(root)) return 2;
    const std::string parent = std::string(root) + "/Mixed";
    const std::string first = parent + "/Entry.INI", second = parent + "/entry.ini";
    if (mkdir(parent.c_str(), 0700) || !touch(first)) return 3;
    char resolved[PATH_MAX];
    bool ok = rf_case_path((std::string(root) + "/mixed/ENTRY.ini").c_str(), resolved, sizeof(resolved), 1) == 1 && resolved == first;
    ok = ok && rf_case_path((std::string(root) + "\\mixed\\ENTRY.ini").c_str(), resolved, sizeof(resolved), 2) == 1 && resolved == first;
    ok = ok && rf_case_path("", resolved, sizeof(resolved), 1) == -1;
    ok = ok && rf_case_path((parent + "/absent").c_str(), resolved, sizeof(resolved), 1) == 0;
    ok = ok && rf_case_path(first.c_str(), resolved, 2, 1) == -1;
    ok = ok && touch(second) && rf_case_path((parent + "/ENTRY.ini").c_str(), resolved, sizeof(resolved), 2) == -1;
    unlink(second.c_str()); unlink(first.c_str()); rmdir(parent.c_str()); rmdir(root);
    return ok ? 0 : 4;
}
