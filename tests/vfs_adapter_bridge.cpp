#include "Genesis.h"
extern geVFile *PassWord(char *, bool);
extern void CloseFile();
extern "C" geVFile *rf_test_archive_open(char *path, int encrypted)
{
    return PassWord(path, encrypted != 0);
}
extern "C" void rf_test_archive_close(void) { CloseFile(); }
