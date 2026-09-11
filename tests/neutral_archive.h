#ifndef RF_NEUTRAL_ARCHIVE_H
#define RF_NEUTRAL_ARCHIVE_H
#include "Genesis.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static geVFile *neutral_archive_create(const char *path)
{
    geVFile *archive = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_VIRTUAL,
        path, NULL, GE_VFILE_OPEN_CREATE | GE_VFILE_OPEN_DIRECTORY);
    geVFile *directory;
    if (!archive) return NULL;
    directory = geVFile_Open(archive, "install", GE_VFILE_OPEN_CREATE | GE_VFILE_OPEN_DIRECTORY);
    if (!directory) { geVFile_Close(archive); return NULL; }
    if (!geVFile_Close(directory)) { geVFile_Close(archive); return NULL; }
    return archive;
}

static int neutral_archive_text(geVFile *archive, const char *path, const char *text)
{
    geVFile *file;
    size_t size = strlen(text);
    char *writable;
    int ok;
    if (!size || size > INT_MAX) return 0;
    /* Inherited FSVFS_Write casts away const and XORs its input, even with
       mask zero. Exercise plain archives with owned writable storage; the
       public const-contract defect is recorded in FINDINGS_OUTSIDE_SCOPE. */
    writable = (char *)malloc(size);
    if (!writable) return 0;
    memcpy(writable, text, size);
    file = geVFile_Open(archive, path, GE_VFILE_OPEN_CREATE);
    if (!file) { free(writable); return 0; }
    ok = geVFile_Write(file, writable, (int)size) && !memcmp(writable, text, size);
    free(writable);
    return geVFile_Close(file) && ok;
}
#endif
