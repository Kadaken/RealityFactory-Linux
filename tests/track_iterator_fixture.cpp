#include "RabidFramework.h"
#include <cstdio>

#define CHECK(condition) do { if(!(condition)) { \
    std::fprintf(stderr, "track iterator failed at %d\n", __LINE__); return 1; \
} } while(0)

bool RF_BootGameActive() { return false; }
bool RF_BootGameBeginTick() { return false; }
bool RF_BootGameCapture() { return false; }
bool RF_BootGameEndFrame(bool, int) { return false; }
bool RF_BootGameEndTick() { return false; }
float RF_BootGameStepMilliseconds() { return 0.0f; }
extern "C" int RF_RunBootStage(void) { return -1; }

int main()
{
    CTrack tracks;
    Track *cursor = NULL;
    int visited = 0;

    CHECK(tracks.TrackCount == 0);
    CHECK(tracks.Track_GetNextTrack(NULL) == NULL);
    while((cursor = tracks.Track_GetNextTrack(cursor)) != NULL)
        ++visited;
    CHECK(visited == 0);

    tracks.TrackCount = 2;
    cursor = NULL;
    CHECK(tracks.Track_GetNextTrack(cursor) == &tracks.TrackList[0]);
    while((cursor = tracks.Track_GetNextTrack(cursor)) != NULL)
        ++visited;
    CHECK(visited == 2);
    CHECK(cursor == NULL);

    std::fprintf(stderr, "track iterator: PASS\n");
    return 0;
}
