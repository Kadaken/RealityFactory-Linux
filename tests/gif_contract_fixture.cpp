/* Constructor-focused integration control; linked only into rf_gif_control.
   No loader behavior is replaced. Check exact reads before the decoder runs.
   Non-null NextFrame also proves Active: its first branch rejects !Active. */
#include "RabidFramework.h"
#include <cstdio>
#include <cstdlib>

/* Constructor runs only on the fixture's main thread. */
static bool constructing;
static unsigned reads;
static bool reached_eof;

struct RFGifFixtureAccess
{
    static bool descriptor_matches(const CAnimGif &gif)
    {
        return gif.Active && gif.iLeft == 0 && gif.iTop == 0 &&
               gif.iWidth == 1 && gif.iHeight == 1;
    }
};

static void require(bool condition, const char *message)
{
    if (!condition) {
        std::fprintf(stderr, "RF GIF fixture failure: %s\n", message);
        std::abort();
    }
}

extern "C" geBoolean __real_geVFile_Read(geVFile *, void *, int);
extern "C" geBoolean __wrap_geVFile_Read(geVFile *file, void *buffer, int count)
{
    long before = 0, size = 0, after = 0;
    if (constructing) {
        require(geVFile_Tell(file, &before) && geVFile_Size(file, &size), "read metadata");
        require(count > 0 && before <= size && count <= size-before, "over-request past GIF EOF");
    }
    const geBoolean result = __real_geVFile_Read(file, buffer, count);
    if (constructing) {
        require(result && geVFile_Tell(file, &after) && after-before == count, "incomplete read");
        ++reads;
        reached_eof = after == size;
        if (reached_eof)
            std::fprintf(stderr, "RF GIF fixture: exact EOF read, bytes=%ld reads=%u\n", after, reads);
    }
    return result;
}

extern "C" void __real__ZN8CAnimGifC1EPKci(CAnimGif *, const char *, int);
extern "C" void __wrap__ZN8CAnimGifC1EPKci(CAnimGif *gif, const char *path, int format)
{
    require(!constructing, "nested construction");
    /* Distinct high/low bytes check endian order beyond the 1x1 descriptor. */
    const uint8_t packed[] = {0xa5, 0x34, 0x12};
    require(rf_read_le16(packed+1) == 0x1234, "little-endian byte order");
    constructing = true;
    reads = 0;
    reached_eof = false;
    __real__ZN8CAnimGifC1EPKci(gif, path, format);
    constructing = false;
    require(reads == 3 && reached_eof, "13-byte header, GCT and exact remainder required");
    require(RFGifFixtureAccess::descriptor_matches(*gif), "decoded descriptor must be 1x1 at 0,0");
    require(gif->NextFrame(true) != NULL, "Active/NextFrame");
    require(RFGifFixtureAccess::descriptor_matches(*gif), "replayed descriptor must be 1x1 at 0,0");
    std::fprintf(stderr, "RF GIF fixture: descriptor 1x1 at 0,0; LE16 0x1234 confirmed\n");
    std::fprintf(stderr, "RF GIF fixture: Active and non-null NextFrame confirmed\n");
}
