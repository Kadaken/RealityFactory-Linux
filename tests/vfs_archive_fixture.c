#include "neutral_archive.h"
#include <stdio.h>

extern geVFile *rf_test_archive_open(char *, int);
extern void rf_test_archive_close(void);

static int read_text(geVFile *archive, const char *name, const char *expected)
{
    char buffer[128] = {0};
    geVFile *file = geVFile_Open(archive, name, GE_VFILE_OPEN_READONLY);
    int ok;
    if (!file) return 0;
    ok = geVFile_Read(file, buffer, (int)strlen(expected)) && !strcmp(buffer, expected);
    return geVFile_Close(file) && ok;
}
static int line_progress(geVFile *archive)
{
    geVFile *file = geVFile_Open(archive, "install\\camera.ini", GE_VFILE_OPEN_READONLY);
    char line[132];
    long before = 0, after = 0, size = 0;
    int i, ok = 0;
    if (!file || !geVFile_Size(file, &size)) return 0;
    for (i=0; i<16; ++i) {
        geBoolean got;
        if (!geVFile_Tell(file, &before)) break;
        got = geVFile_GetS(file, line, sizeof(line));
        if (!geVFile_Tell(file, &after)) break;
        if (!got) { ok = after == size; break; }
        if (after <= before) {
            fprintf(stderr, "RF archive line progress: stalled position=%ld length=%ld read=%d\n", after, size, got);
            break;
        }
    }
    return geVFile_Close(file) && ok;
}
int main(int argc, char **argv)
{
    char path[] = "fixture.vfs", rejected[] = "rejected.vfs";
    const char *camera = "[General]\r\nfieldofview=2.0\r\n";
    geVFile *archive = neutral_archive_create(path), *file, *nested;
    FILE *raw;
    int ok;
    if (!archive) return 1;
    ok = neutral_archive_text(archive, "install\\camera.ini", camera);
    nested = geVFile_Open(archive, "install\\nested", GE_VFILE_OPEN_CREATE | GE_VFILE_OPEN_DIRECTORY);
    if (!nested) { geVFile_Close(archive); return 2; }
    ok = geVFile_Close(nested) && ok;
    ok = neutral_archive_text(archive, "install\\nested\\neutral.txt", "neutral") && ok;
    ok = geVFile_Close(archive) && ok;
    file = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_VIRTUAL, path, NULL, GE_VFILE_OPEN_READONLY);
    if (file) { geVFile_Close(file); ok = 0; }
    archive = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_VIRTUAL, path, NULL,
        GE_VFILE_OPEN_READONLY | GE_VFILE_OPEN_DIRECTORY);
    if (!archive) return 3;
    if (argc == 2 && !strcmp(argv[1], "--line-progress")) {
        ok = line_progress(archive) && ok;
        ok = geVFile_Close(archive) && ok;
        geVFile_CloseAPI();
        return ok ? 0 : 7;
    }
    ok = read_text(archive, "install\\camera.ini", camera) && ok;
    ok = read_text(archive, "Install\\Camera.ini", camera) && ok;
    ok = read_text(archive, "INSTALL\\Nested\\Neutral.TXT", "neutral") && ok;
    file = geVFile_Open(archive, "install/camera.ini", GE_VFILE_OPEN_READONLY);
    if (file) { geVFile_Close(file); ok = 0; }
    ok = geVFile_Close(archive) && ok;
    archive = rf_test_archive_open(path, 0);
    ok = archive && read_text(archive, "install\\camera.ini", camera) && ok;
    rf_test_archive_close();
    raw = fopen(rejected, "wb");
    if (!raw) return 4;
    ok = fwrite("CF00", 1, 4, raw) == 4 && ok;
    ok = fclose(raw) == 0 && ok;
    /* Same prefix-driven selection used by InitializeCommon. */
    raw = fopen(rejected, "rb");
    if (!raw) return 5;
    { char prefix[4] = {0};
      ok = fread(prefix, 1, 4, raw) == 4 && ok;
      ok = fclose(raw) == 0 && ok;
      file = rf_test_archive_open(rejected, !memcmp(prefix, "CF00", 4));
    }
    if (file) ok = 0;
    rf_test_archive_close();
    geVFile_CloseAPI();
    fprintf(stderr, "RF archive fixture: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 6;
}
