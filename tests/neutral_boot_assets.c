/* Generated neutral assets only. Run in CTest's isolated build fixture directory. */
#include "Genesis.h"
#include "neutral_bsp_fixture.h"
#include "neutral_archive.h"
#include <stdio.h>
static geVFile *archive;
static unsigned char entity_data[512];
static int entity_used;
static void entity_int(int32 n) { memcpy(entity_data+entity_used,&n,4); entity_used+=4; }
static void entity_string(const char *s) { int n=(int)strlen(s)+1; entity_int(n); memcpy(entity_data+entity_used,s,n); entity_used+=n; }
static void entity_pair(const char *k,const char *v) { entity_string(k); entity_string(v); }

static int audio_world(void)
{
    geVFile *file=geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,"named-audio.bsp",NULL,GE_VFILE_OPEN_CREATE);
    int ok;
    if (!file) return 0;
    entity_int(2);
    entity_int(4); entity_pair("classname","%typedef%"); entity_pair("%typename%","AudioSource3D");
    entity_pair("origin","point"); entity_pair("%defaultvalue%","0 0 0");
    entity_int(3); entity_pair("classname","AudioSource3D"); entity_pair("%Name%","NeutralAudio"); entity_pair("origin","0 0 0");
    ok=neutral_bsp_write_with_entities(file,entity_data,entity_used);
    return geVFile_Close(file) && ok;
}

/* GIF87a: 1x1, two-entry global palette, one image, LZW clear/0/end. */
static int gif_file(void)
{
    static const unsigned char gif[] = {
        'G','I','F','8','7','a', 1,0, 1,0, 0x80,0,0,
        0,0,0, 255,255,255,
        0x2c, 0,0, 0,0, 1,0, 1,0, 0,
        2, 2, 0x44,0x01, 0, 0x3b
    };
    FILE *file = fopen("video/menu/neutral.gif", "wb");
    int ok;
    if (!file) return 0;
    ok = fwrite(gif, 1, sizeof(gif), file) == sizeof(gif);
    return fclose(file) == 0 && ok;
}

/* BITMAPFILEHEADER + BITMAPINFOHEADER, little-endian 24-bit solid tile. */
static int bitmap_file(const char *path, unsigned char color)
{
    unsigned char header[54] = { 'B', 'M', 54, 12, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0,
        40, 0, 0, 0, 32, 0, 0, 0, 32, 0, 0, 0, 1, 0, 24, 0 };
    unsigned char pixels[32*32*3];
    FILE *file = fopen(path, "wb");
    int ok;
    if (!file) return 0;
    memset(pixels, color, sizeof(pixels));
    ok = fwrite(header, 1, sizeof(header), file) == sizeof(header) &&
         fwrite(pixels, 1, sizeof(pixels), file) == sizeof(pixels);
    return fclose(file) == 0 && ok;
}

static int text_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");
    int ok;
    if (!file) return 0;
    ok = fputs(text, file) >= 0;
    if (!strncmp(path, "install/", 8)) {
        char vfs_path[256];
        size_t i;
        if (strlen(path) >= sizeof(vfs_path)) ok = 0;
        else {
            strcpy(vfs_path, path);
            for (i=0; vfs_path[i]; ++i) if (vfs_path[i]=='/') vfs_path[i]='\\';
            ok = neutral_archive_text(archive, vfs_path, text) && ok;
        }
    }
    return fclose(file) == 0 && ok;
}
int main(void)
{
    geVFile *file = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_DOS, "neutral.bsp",
                                         NULL, GE_VFILE_OPEN_CREATE);
    int ok;
    if (!file) return 1;
    ok = neutral_bsp_write(file);
    ok = geVFile_Close(file) && ok;
    ok = audio_world() && ok;
    archive = neutral_archive_create("neutral.vfs");
    if (!archive) return 1;
    ok = text_file("RealityFactory.ini", "GameName=Neutral Runtime Fixture\r\nLevelDirectory=.\r\nWidth=320\r\nHeight=240\r\nFullScreen=false\r\nUseDirectInput=false\r\n") && ok;
    ok = text_file("install/camera.ini", "[General]\nfieldofview=2.0\n") && ok;
    /* CMenu::LoadMenuIni: token lists, not INI sections. All artwork is generated. */
    ok = text_file("install/menu.ini", "designsize=320 240\nfadetime=0\n"
        "titles=0 neutral.bmp alpha.bmp\n"
        "cursor=neutral.bmp alpha.bmp -1\n"
        "menutitle=0 0 16 16 31 31 0 0 -1\nmain=-1 0\n") && ok;
    ok = bitmap_file("bitmaps/menu/neutral.bmp", 128) && ok;
    ok = bitmap_file("bitmaps/menu/alpha.bmp", 255) && ok;
    ok = gif_file() && ok;
    /* CPlayer::LoadAttributes reads named sections with initial/low/high.
       CWeapon::LoadDefaults accepts an empty section list (no weapons).
       These do not replace the map's mandatory PlayerSetup or player actor. */
    ok = text_file("install/player.ini", "[health]\ninitial=100\nlow=0\nhigh=100\n") && ok;
    ok = text_file("install/weapon.ini", "; Neutral fixture: no weapon definitions\n") && ok;
    ok = geVFile_Close(archive) && ok;
    geVFile_CloseAPI();
    return ok ? 0 : 1;
}
