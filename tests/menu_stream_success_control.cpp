/* Linked only into rf_menu_stream_control, never the production runner.
   Force successful construction state to cover the DoMenu delete branch;
   this is a lifetime fixture, not media decoding or playback. */
#include <cstdio>
extern "C" int __wrap__ZN14StreamingAudio6CreateEPc(void *, char *)
{
    std::fprintf(stderr, "RF fixture-only: stream Create success control; no playback implementation\n");
    return 1;
}
