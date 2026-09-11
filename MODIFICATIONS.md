# Description of modifications

This file records changes made to inherited Reality Factory and bundled source
while producing the private native Linux compatibility runtime. It supplements
the Git history; it does not claim ownership of upstream work.

## RF-11o — bounded inherited gameplay loop (relay round 4)

- adapters/native_main.cpp calls the actual CRFMenu::GameLoop for hidden stage
  5. It does not reproduce a subset of managers. Stage-only observers bound
  execution to 120 completed ticks/frames, compare first/last RGB captures,
  check GL errors and EndFrame, and require consumed quit. Transitions/death/
  pause fail this start-level measurement instead of entering another level.
- upstream/CMenu.cpp adds inert-outside-stage-5 tick/capture observers around
  the existing GameLevel sequence. Input, DispatchTick, effects, components,
  RenderWorld, overlays, HUD, EndFrame and post-frame media polling retain their
  original order. GameLoop's inventory snapshot and preview writes remain.
- upstream/CCommonData.cpp returns 1000/60 milliseconds through the existing
  DispatchTick time query only during this controller. Absolute/free-running
  clocks still use the inherited timer; this is not a deterministic-clock or
  timer-race repair. Native pacing sleeps on steady deadlines, without spinning
  or unbounded catch-up. No production interactive timing change.
- adapters/host_window.*, input_compat.cpp and joystick.*: stage-5 keyboard/
  mouse events are ignored, pointer queries return the hidden window center,
  and joystick polling is neutral during GameLoop. Close/quit still pump.
  Only the final stage-5 frame queues quit; stages 1–4 retain their contracts.
- New boot_watchdog.h bounds initial GameLoop setup at ten seconds and each tick at five
  seconds using a sleeping watchdog thread. Timeout writes a diagnostic and
  exits 124, without pretending that forced exit performs clean teardown. The
  final tick deadline also covers GameLoop's return path; common-manager
  teardown remains bounded by the outer measurement process-group timeout.
- CMake enables C++11 only for native_main.cpp and the watchdog fixture; the
  inherited 157-unit graph remains C++98. Watchdog cancellation/expiry,
  virtual-joystick neutralization, pointer/key suppression and the 120-frame
  quit boundary have fixtures. Neutral stages 4/5 remain parked without avatars.

## RF-11m — public pixel-composer rebuild (relay round 2)

- No source changes this pass. Rebuilt both configurations against public
  3c10f45; six unsigned pixel composers clear D19. Unchanged stage-4 controller
  now enters RenderWorld, then stops at D20's face-capacity assertion.
- Documentation records the new boundary; no frame, content, stage-5 or
  sanitizer-policy changes were made to advance past it.

## RF-11l — normal level frame controller boundary

- adapters/native_main.cpp: after normal InitializeLevel succeeds, stage 4
  attempts the existing CGenesisEngine BeginFrame/RenderWorld/EndFrame path.
  Bounded framebuffer comparison and queued-quit validation reject an unchanged
  frame or missing quit; no camera override, visible window or input capture.
- First execution stops in public pixel composition inside BeginFrame before
  RenderWorld/readback; full controller execution remains unproven. Stage 5
  explicitly remains pending. No inherited source changed.
- Rebuilt against public 317c2ac: actor rejection cleanup preserves content
  failures and clears the previous exercised leak. No asset changes.

## RF-11k — pre-0.68 inventory compatibility and actor filename ownership

- `upstream/CInventory.cpp`: missing/empty arrow image names are optional;
  explicit images or alpha files that fail to load still reject configuration.
  Existing alpha-name fallback is unchanged. Log one diagnostic when all four
  arrow image names are absent. This supports the pre-RF-0.68 format; the reviewed
  history places arrow-key introduction at RF 0.68, 2002-11-24.
- Guard the four arrow bitmap registrations and four paginated draw calls on
  non-null images. Contrary to the initial blueprint, Blit does draw arrows
  when an item/weapon inventory exceeds nine entries. Null-safe drawing is
  necessary for optional resources, not a new inventory feature.
- `upstream/CActorManager.cpp`: array-delete cached szFilename in the destructor
  to match LoadActor's new char[]. Other object deletions remain unchanged.
- Added neutral constructor/destructor and paginated-draw fixtures for absent,
  partial and complete arrow configurations, plus rejection of each explicitly
  missing image/alpha. Only the test translation unit disables access checking
  to seed paging state; production class layout/access controls are unchanged.
  Restoring the required-arrow check fails construction; removing a draw guard
  fails the test-only null-bitmap check. Both temporary mutations were restored.
  No game data or loader behavior was modified for missing Motions directories.

## RF-11j — persistent-attribute ownership (D14)

- `upstream/CPersistentAttributes.cpp`: changed all six scalar deletions of
  array-owned Name/UserData storage to delete[]. Both Delete branches now also
  release UserData; deleting the head preserves its successor before freeing
  the removed node. Node objects retain scalar delete. No other runtime file
  was changed for this repair.
- Added a neutral test-only boot-stage wrapper using the same 157 production
  runtime objects. It walks a five-node list after head/middle/tail deletion,
  checks surviving values/data, exercises buffer replacement, explicit deletion,
  repeated clear, sole-node removal and final destructor cleanup.
- The fixture reproduces the old allocation/deallocation mismatch. Restoring
  the null-head bug fails its list walk; omitting removal-time UserData cleanup
  leaks 121 bytes in four allocations under LSan. All temporary mutations were
  restored. No sanitizer checks or existing tests were disabled.

## RF-11i — field-aware schemas and borrowed entity names

- Declared all 65 consumed entity classes before world construction through the
  explicit public schema constructor. The generated 1,173-field table is checked
  for staleness by CTest; C++ compile-time checks verify every field offset and
  size. The two file-local light types remain mechanically derived, not hand
  maintained. Retained sizeof reservation for trailing native struct padding.
- Gravity scalar-to-Y-vector follows the older scalar Y application and the
  0.75B vector representation (2006-09-20); absent WindSpeed from pre-0.76.0
  data remains zero (introduced 2008-05-25). Integer InitialAlpha parses as a
  floating-point value. These are schema conversions, not gameplay validation.
- Replaced 42 syntactic block-local name assignments in 37 inherited files with
  borrowed engine-owned name strings. Byte-preserving mechanical replacement
  retains the legacy source encodings. No other local buffers were removed.
- Added a generated named-audio world and test-only constructor/registry harness.
  It exposes a separate persistent-attribute array/scalar teardown defect; the
  sanitized test remains red rather than suppressing or bypassing that defect.
- No production media implementation was added; existing unavailable media paths
  remain unchanged.

D12 inherited file: `upstream/CGenesisEngine.cpp`.

D13 inherited files: `upstream/C3DAudioSource.cpp`, `upstream/CActMaterial.cpp`, `upstream/CAttribute.cpp`, `upstream/CAutoDoors.cpp`, `upstream/CChangeLevel.cpp`, `upstream/CCorona.cpp`, `upstream/CCountDown.cpp`, `upstream/CDSpotLight.cpp`, `upstream/CDamage.cpp`, `upstream/CDynalite.cpp`, `upstream/CElectric.cpp`, `upstream/CFirePoint.cpp`, `upstream/CFlame.cpp`, `upstream/CFlipBook.cpp`, `upstream/CFlipTree.cpp`, `upstream/CFloating.cpp`, `upstream/CFoliage.cpp`, `upstream/CLevelController.cpp`, `upstream/CLiftBelt.cpp`, `upstream/CLiquid.cpp`, `upstream/CLogic.cpp`, `upstream/CMessage.cpp`, `upstream/CMorph.cpp`, `upstream/CMorphingFields.cpp`, `upstream/CMovingPlatforms.cpp`, `upstream/COverlay.cpp`, `upstream/CParticleSystem.cpp`, `upstream/CPawn.cpp`, `upstream/CRain.cpp`, `upstream/CScriptPoint.cpp`, `upstream/CSpout.cpp`, `upstream/CStaticEntity.cpp`, `upstream/CStaticMesh.cpp`, `upstream/CTeleporter.cpp`, `upstream/CTriggers.cpp`, `upstream/CWallDecal.cpp`, `upstream/CutScene.cpp`.

## RF-11h — entity capacity boundary (not complete schema migration)

- CGenesisEngine reserves native userdata capacity after successful world
  creation and before geEngine_AddWorld or any level manager sees entities.
  Reservation failure frees the unregistered world and stops loading.
- Added a generated sizeof table from userdata casts in the 157-unit graph:
  276 source cast sites, 65 distinct types, including two mechanically derived
  file-local lighting declarations. A CTest checks the committed output.
- Calls the public field-size/reserve APIs, using zero for newly allocated
  tail bytes and logging declared/runtime size differences. This compatibility
  policy does not parse authoring defaults, relocate fields or convert types.
- The run passes the old allocation overflow but exposes incompatible prefix
  fields in EnvironmentSetup and a separate stack lifetime error. Tail reserve
  is therefore not a complete D12 correction; no gameplay/release claim.

Affected inherited file: `CGenesisEngine.cpp`. See RF11_DIVERGENCE.md for the
measured offsets, per-class rows and required design revision.

## RF-11g — GIF field alignment and array ownership

- D10: added rf_read_le16 in the compatibility header and replaced all seven
  CAnimGif WORD-pointer reads with byte-wise little-endian loads. Existing
  pointer advances remain unchanged; no alignment suppression is used.
- D11: changed all ten scalar deletions of pcBitmap/pcGifTable to delete[].
  The table pointer is converted back to BYTE* at deletion to match the
  actual new BYTE[] allocation; allocations and exit-path flow are unchanged.
- A friend observer declaration in CAnimGif.h allows the test-only constructor
  control to check Active and the decoded 1x1-at-0,0 descriptor before and
  after NextFrame. It adds no fields or runtime implementation. The fixture
  also checks distinct little-endian bytes decode as 0x1234.

Affected inherited files: `CAnimGif.cpp`, `CAnimGif.h`.
See `Reports/RF11_DIVERGENCE.md` for results and remaining gate failures.

## RF-11f — exact GIF payload extent

- CAnimGif's constructor now subtracts the actual 13-byte header consumed by
  its first read, plus the global color table, from file size. Previously it
  subtracted 12 and requested one byte past EOF.
- Added a generated single-frame GIF87a and separate neutral animated-title
  menu fixtures. A test-only constructor/read wrapper checks exact read
  extents and Active via non-null NextFrame; no production behavior is mocked.
- Release tests pass. Sanitized tests expose a pre-existing misaligned WORD
  read inside TakeIt, now reachable after successful loading; that decoder
  defect is recorded as D10, not suppressed or repaired by this pass.

Affected inherited file: `CAnimGif.cpp`. See `Reports/RF11_DIVERGENCE.md`.

## RF-11e — menu stream ownership

- CRFMenu destruction now calls Delete, deletes the owned StreamingAudio
  object and clears the pointer, including when stream Create failed.
- DoMenu clears the pointer after its existing stream deletion so subsequent
  destruction cannot use it again. Other deletion and Create-failure paths
  were left unchanged.
- Added neutral music-config stage-2/3 lifetime tests and a separate test-only
  linker success control to reach the otherwise unavailable playback branch.
  The production runner contains no success control. Removing the new DoMenu
  null assignment makes that fixture fail under ASan; restoring it passes.

Affected inherited file: `CMenu.cpp`. See `Reports/RF11_DIVERGENCE.md`.

## RF-11c — archive opening and separator coverage (gate blocked)

- Plain-VFS adapter opens an archive directory, not a file-only handle.
- CCommonData::InitializeCommon now checks mounting failure before logging
  detection, and uses the existing fatal error boundary on rejection.
- Documented the unchanged backslash contract in OpenRFFile(V).
- Added generated archives, separator/case/rejection tests, archive-only
  common initialization, and a bounded line-progress regression. The latter
  exposes a public-engine reader defect; no private parser workaround was added.

Affected inherited file: `CCommonData.cpp`. See RF11_DIVERGENCE.md for D04,
D05 and the blocking D06. Public engine source was not changed.

## RF-11b — private text, case and fatal-exit boundary

- Added CRLF-normalizing fgets and guarded fatal-exit wrappers through the
  platform header without editing inherited call sites. Fatal cleanup tracks
  host engine/audio ownership, shuts down GL before SDL, and explicitly checks
  LSan before `_exit`; it does not claim to unwind the entire RF object graph.
- Both CCommonData::OpenRFFile overloads now retry failed filesystem opens
  using unique case-insensitive component matches; ambiguous matches fail.
- CGenesisEngine registers/clears its engine and audio lifetime with the host.
- Added neutral line-ending, both-overload case, missing-camera, re-entry,
  exit-status and intentional-leak regression tests. Generated root INI is CRLF.

Affected inherited files: `CCommonData.cpp`, `CGenesisEngine.cpp`.
See `Reports/RF11_DIVERGENCE.md`. Real stage 2 remains blocked by D04; the
plain-VFS directory-open flag was diagnosed but not changed in this pass.

## RF-10 — staged bootstrap (partial; stages 4–5 blocked)

- Added explicit staged hidden boot profiles. Stage 1 alone returns before
  common managers; stages 2+ retain their upstream construction order.
- Added a menu-frame completion hook that queues SDL_QUIT, checks hidden
  visibility, and reports event-injection failure. Normal launches do not
  activate the hook. Camera-file failure prints the resolved config directory.
- Media managers explicitly report unavailable; CD playback requests no
  longer return false success. Native AVI lifecycle is not a decoder.
- Fixed constructor-time saved-volume loading to use the menu instance rather
  than the not-yet-assigned global menu pointer.
- Moved the existing player actor-pointer initialization ahead of defaults-file
  early returns. Missing PlayerSetup now returns failure to InitializeLevel
  instead of deleting the active object and global graph inside LoadAvatar.
- Extended the generated neutral fixture with upstream-format menu/attribute/
  empty-weapon configuration and generated BMP tiles. No gameplay assets were
  imported. Real level loading still rejects its missing player prerequisites.

Affected inherited files: `CCommonData.cpp`, `RabidFrameworkMain.cpp`,
`CCameraManager.cpp`, `CAudioManager.cpp`, `CCDAudio.cpp`, `CMIDIAudio.cpp`,
`CMenu.cpp`, and `CPlayer.cpp`. See `Reports/RF10_STAGED_BOOTSTRAP.md`.

## RF-8 — native host and first-frame startup

- Replaced Win32 class/window construction with an SDL-owned hidden OpenGL
  window. Driver directory discovery uses the actual loaded engine DSO rather
  than a compiled-in path. Engine/context teardown precedes window destruction
  and SDL shutdown; joystick handles are released before the latter.
- Centralized SDL event polling, virtual-key state, ASCII conversion, focus,
  quit notifications and the reachable main-thread multiplayer timer in the
  host/input adapters. Desktop cursor warping is disabled; showing the window
  requires explicit `--human-launch`. File dialogs fail with a diagnostic.
- Replaced the inert link probe with `realityfactory_linux`, entering inherited
  `WinMain`. Added explicit `--neutral-frame` render-only startup: initialize
  common state, engine and camera, render a generated BSP, then shut down.
  This profile intentionally stops before menus and game entity initialization.
- Replaced the deferred native resize hook with SDL resizing, preserving
  window visibility and placement.
- Fixed pool-head duplicate linkage, a Clang-rejected string vararg, a leaked
  FreeImage packing pragma that corrupted cross-unit Simkin layouts, and a
  console initialization write beyond its array. Converted the bundled
  FreeImage header's source encoding to UTF-8 while preserving its notices.
- Removed forced `NDEBUG` so consumer allocator declarations match the chosen
  engine build configuration. Added header-layout, input/host and neutral
  startup regression tests.

Affected inherited files: `CGenesisEngine.cpp`, `CCommonData.cpp`,
`RabidFrameworkMain.cpp`, `CMenu.cpp`, `AutoSelect.c`, `CEffects.cpp`,
`SPool.cpp`, `TPool.cpp`, and `include/FreeImage.h`.

## RF-2 — POSIX compatibility boundary

- Added one forced-in platform header for fixed-width primitive types, opaque
  legacy handles, POSIX strings/filesystem calls, pthread mutexes, and runtime
  declarations.
- Routed the central runtime header through that boundary.
- Normalized uniquely resolvable case-sensitive and backslash-separated include
  paths required by Linux filesystems.

Affected inherited files: `AutoSelect.c`, `AutoSelect.h`, `CCommonData.cpp`,
`CCommonData.h`, `CEffects.cpp`, `CExplosion.cpp`, `CFlipBook.cpp`,
`CFoliage.cpp`, `CLevelController.cpp`, `CLevelController.h`, `CMenu.cpp`,
`CMessage.cpp`, `CNetBuffer.cpp`, `CNetwork.h`, `CPawn.cpp`, `CPawn.h`,
`CPawnCon.cpp`, `CPawnHigh.cpp`, `CPawnLow.cpp`, `CTriggers.h`,
`CWallDecal.cpp`, `Clogic.h`, `Drawbbox.c`, `EffManger.cpp`,
`EffParticle.cpp`, `GameEntityDataTypes.h`, `Mixer.cpp`, `ProcParticles.cpp`,
`ProcUtil.h`, `RabidFramework.h`, `RabidFrameworkMain.cpp`, `SPool.cpp`,
`TPool.cpp`; `Qx/QxUser.cpp`, `Qx/QxUser.h`, `Qx/qxBinTriTree.cpp`,
`Qx/qxCloudMachine.cpp`, `Qx/qxEffectBase.cpp`,
`Qx/qxEffectParticleChamber.cpp`, `Qx/qxEffectTextureFlow.cpp`,
`Qx/qxEffectTextureFlow.h`, `Qx/qxMoon.cpp`, `Qx/qxMoon.h`,
`Qx/qxParticle.cpp`, `Qx/qxParticle.h`, `Qx/qxSkyDome.cpp`,
`Qx/qxSkyDome.h`, `Qx/qxStars.cpp`, `Qx/qxSun.cpp`,
`Qx/qxTerrainDefinitionFile.cpp`, `Qx/qxTerrainMap.cpp`,
`Qx/qxTerrainMap.h`, `Qx/qxTerrainMapBase.cpp`, `Qx/qxTerrainMapBase.h`,
`Qx/qxTerrainMgr.cpp`, `Qx/qxTerrainMgr.h`, `Qx/qxTerrainPoly.cpp`,
`Qx/qxTerrainTile.cpp`, `Qx/qxTerrainUtils.cpp`, `Qx/qxTerrainVert.cpp`,
`Qx/qxTerrainVert.h`; `Simkin/skAsciiString.h`, `Simkin/skElementObject.h`,
`Simkin/skInputSource.h`, `Simkin/skMSXMLElementObject.h`,
`Simkin/skMSXMLExecutable.cpp`, `Simkin/skOutputDestination.h`,
`Simkin/skString.h`, `Simkin/skTreeNode.cpp`,
`Simkin/skXMLElementObject.h`, `Simkin/skXMLExecutable.cpp`, and
`Simkin/skXMLExecutable.h`.

## RF-3 — SDL joystick boundary

- Replaced the unavailable joystick SDK include with an SDL2 adapter exposing
  only the five operations and four state fields consumed by the runtime.
- Added deterministic zero-device behavior and a headless contract test.
- Added bounds and null checks to script-accessible joystick operations and
  made shutdown close every owned slot after hot unplug.
- Added a repeatable include-case/path normalization tool.

Affected inherited files: `CCommonData.cpp`, `RabidFramework.h`,
`Qx/qxColor.cpp`, `hashtable/hash_fun.h`, and `hashtable/hash_table.h`.

## RF-4 — shared portability roots

- Added exact-width Linux declarations for inherited bitmap and timing types.
- Replaced raw asynchronous key polling declarations with an SDL event-state
  adapter while retaining configuration-file virtual-key values.
- Disabled the bundled IPX implementation on non-Windows systems, matching its
  stated platform scope.
- Guarded Microsoft debug-heap instrumentation with the Microsoft compiler.
- Corrected in-class constructor qualification rejected by standard C++.
- Replaced the WinMM system-mixer implementation on Linux with a bounded
  compatibility control connected to Genesis3D's OpenAL master-volume API.

Affected inherited files: `CCommonData.cpp`, `CGenesisEngine.cpp`,
`CNetwork.cpp`, `HawkNL/nl.h`, `Mixer.cpp`, `Mixer.h`, and `Qx/qxColor.h`.

## RF-5 — compile closure

- Exposed the public engine degree/radian constants to consumers without
  duplicating them privately.
- Compiled the CD and MIDI classes as inert, save-layout-preserving Linux
  implementations after the isolated runtime audit found no MIDI files, CD
  media files, or configuration/entity references to either path.
- Added explicit non-operational Linux compile boundaries for AVI, compressed
  streaming audio, and MP3/Ogg playback. These are temporary RF-5 seams, not
  claimed playback support; RF-7 replaces them with FFmpeg and OpenAL.
- Added an SDL-backed, non-capturing keyboard/message compatibility boundary
  sufficient to compile the inherited message loop; SDL window ownership is
  still deferred to RF-8.
- Replaced three MSVC inline-assembly image-copy/blur paths with equivalent
  bounds-preserving C loops or `memcpy` operations.
- Replaced LP64-unsafe leaf, timer callback, and thread-local error casts with
  fixed-width or pointer-width types.
- Made temporary Simkin contexts explicit lvalues at legacy method-call sites,
  and corrected standard-C++ const/rvalue and stream-open incompatibilities.

Affected inherited files: `CAVIPlayer.cpp`, `CAnimGif.cpp`, `CCDAudio.cpp`,
`CCommonData.cpp`, `CCommonData.h`, `CDecal.h`, `CGenesisEngine.cpp`,
`CLevelController.cpp`, `CMIDIAudio.cpp`, `CMp3.cpp`, `CParticleSystem.cpp`,
`CPawnCon.cpp`, `CPawnHigh.cpp`, `CPawnLow.cpp`, `CProcedural.cpp`,
`CVideoTexture.cpp`, `CWindGenerator.cpp`, `Drawbbox.c`, `EffManger.cpp`,
`HawkNL/err.c`, `OggAudio.cpp`, `ProcSmoke.cpp`, `ProcUtil.cpp`,
`Qx/QxUser.h`, `Qx/qxColor.cpp`, `Qx/qxColor.h`, `Qx/qxTerrainMgr.cpp`,
`RabidFramework.h`, `Simkin/skInputSource.cpp`,
`Simkin/skScriptedExecutable.cpp`, `Simkin/skScriptedExecutable.h`,
`StreamingAudio.cpp`, `StreamingAudio.h`, `Utilities.cpp`, and `oggaudio.h`.

## RF-6 — first link measurement

- Added an inert executable link probe that consumes every compile-closure
  object, the native Genesis3D shared library, and both SDL adapters.
- The probe exists only to measure unresolved symbols and does not initialize
  or run any inherited subsystem.

## RF-7 foundation — resolved deterministic link dependencies

- Linked Fedora's native FreeImage 3.19 library after confirming that all 17
  image APIs used by the inherited runtime retain compatible signatures.
- Added POSIX implementations for debug logging, uppercase/integer conversion,
  and synchronous child-process execution.
- Replaced the unavailable binary-only VFS encryption dependency with a plain
  `VF00` virtual-filesystem opener. Encrypted `CF00` archives are explicitly
  unsupported and fail immediately with a diagnostic; all 101 local private
  VFS archives were verified as `VF00` before this path was enabled.
- Filled the upstream-absent ground-texture trace helper through the corrected
  public Genesis3D texture-name API and added a deterministic contract fixture.
- Routed trace material lookup through the exact collision result after an
  independent review demonstrated that infinite-plane distance alone could
  select an adjacent surface; retained a bounded-polygon point-query fallback.
- Implemented `timeGetTime` with `CLOCK_MONOTONIC` and periodic multimedia
  callbacks with an absolute-deadline pthread scheduler that skips, rather
  than accumulates, missed periods.
- Added explicit no-state AVI library init/exit boundaries. These two symbols
  are lifecycle placeholders only and do not claim video decoding support.

## RF-8 — SDL display selection boundary

- Replaced the Linux build of the inherited Win32 driver-selection dialog with
  deterministic selection from Genesis3D's mode list, preferring resolutions
  advertised by SDL displays and the primary display's current dimensions.
- Preserved the original Win32 dialog behind `_WIN32` and preserved the sorted
  automatic fallback when SDL video has not yet been initialized.
- Added a dummy-video contract fixture covering pre-SDL fallback, rejection of
  failed modes, and selection of SDL's current desktop mode.
- Left the Linux window-resize hook as an explicit diagnostic boundary pending
  resolution of the legacy `geEngine` driver/window ownership conflict.

Affected inherited file: `AutoSelect.c`.

## 2026-09-09 — D21 weapon sentinel guard (applied by Claude)

- `CWeapon::SetWeapon` ran its old-animation scan unconditionally, so the first
  weapon selection of a level (current weapon still the `-1` sentinel from the
  constructor) indexed `WeaponD[-1]`. The scan now sits inside the same
  `-1 / MAX_WEAPONS` sentinel guard that already protects the hide/HUD block,
  and additionally skips when the player's motion name is NULL.
- Behaviour for a valid current weapon is unchanged; the scan still runs on an
  ordinary switch.
- Added `tests/weapon_sentinel_control.cpp`, a stage-2 boot wrapper that links
  the unchanged runtime objects, installs a level-free player and actor manager,
  wraps the entity-set lookup and motion query, and exercises sentinel-first
  selection, an ordinary switch, and the `MAX_WEAPONS` sentinel. It fails at
  `CWeapon.cpp:1343` on the inherited code and passes with the guard.
  The valid-current-weapon branch also intercepts SetMotion and verifies the
  matching old-animation index selects the new weapon's animation; sentinel
  paths must not call it.

Affected inherited file: `CWeapon.cpp`.

## 2026-09-09 — RF-12 human host placement and relative input

- Human window creation selects the SDL display containing the pointer and
  requests one-time centred placement there; unresolvable positions fall back
  to display 0 with a diagnostic. Automated modes retain undefined placement.
- `ProcessMenu` and `GameLevel` have scoped host-state notifications. This is
  intentionally independent of `GetInGame`, which remains true in pause menus.
  Scope exit restores the prior state on every return path.
- Relative input is enabled only for an explicitly human-launched, focused
  gameplay window, and released on Esc, menu entry, focus loss, quit and
  teardown. SDL failures report the failed action. Legacy desktop cursor warps
  remain refused. The pointer adapter supplies client centre plus consumed,
  bounded SDL motion deltas to the inherited look arithmetic.
- Four hidden host fixtures cover synthetic display selection, hidden-mode
  exclusion, menu transitions, and the real native argument parser with
  `--human-launch`. Show and capture requests are intercepted in test binaries;
  physical input and display placement remain the owner's playtest.
- A separate excluded-from-default-build diagnostic runner wraps clock/counter
  reads during stage-5 ticks, forwarding their original values. Its wrappers
  are absent from the production executable. `tools/audit_frame_clocks.py`
  audits the same 157 inherited compilation units.

Affected inherited file: `CMenu.cpp` (two scoped host notifications).

## 2026-09-10 — D25 window-local menu pointer and cursor ownership

- Replaced the menu's desktop-global pointer minus a once-cached window origin
  with coordinates local to the owned SDL window, scaled into the engine's
  logical render dimensions. Window movement, compositor placement and resize
  can no longer stale the hit-test transform.
- Preserved a complete mouse press/release received between menu frames as one
  inherited press frame followed by one release frame, so the legacy action
  contract does not lose a short click.
- Made RF's software cursor the sole cursor while the menu window has mouse
  focus. The SDL cursor is restored on pointer leave, focus loss, quit and
  teardown; gameplay relative capture remains a separate scope and desktop
  cursor warping remains disabled.
- Added the selected SDL video-driver diagnostic and an opt-in D25 pointer
  trace. Hidden fixtures cover local/logical scaling, nonzero placement, move,
  resize, short click and cursor restoration. A real legacy-driver pixel test
  verifies that later cursor submission covers an overlapping button pixel.

Affected inherited file: `CMenu.cpp` (menu pointer acquisition only).

## 2026-09-10 — D31 recovered 0.68A NPC/navigation subsystem

- Imported `CNPC`, `CNPCPathPoint`, and `CTrack` from canonical upstream commit
  `250b75047d19bf0633fead05091c8a46462c45fe` into a separately versioned
  `upstream-0.68a/` boundary. The six original files were committed verbatim
  before the adaptations below.
- Added an isolated entity-declaration header and utility boundary containing
  only the recovered stack, weighted-distance, random-range, and Y-rotation
  contracts required by those sources.
- Compiler-required adaptations: corrected `genesis.h` filename case; declared
  historical implicit-int `StartTrack` as `int32`; supplied the current
  network/save macro's leading `type` argument; and matched the const-qualified
  texture/sound pool declarations used by 0.76.
- Safety-required adaptation: replaced the block-local fallback entity-name
  buffer in `CNPC` with the engine-owned immutable entity-name string, matching
  the previously established D13 ownership contract.
- Added the two recovered entity layouts to the generated schema and size
  tables. `NonPlayerCharacter` reconciles the level's 41 fields with three
  runtime tail fields; neither measured map instantiates `NPCPathPoint`.
- Restored construction, save/restore, tick, render-debug, and destruction at
  the same lifecycle positions used by 0.68A.
- Runtime measurement reaches authored, constructed, actor-load, submission,
  and tick boundaries. Matched sanitizer execution stops at the preserved
  zero-path navigation assertion in `Track.cpp:32`; selecting fallback behavior
  requires an explicit compatibility decision and is not made here.

Affected inherited files: `CCommonData.cpp`, `CCommonData.h`, and
`RabidFramework.h`. Recovered-source adaptations are confined to
`upstream-0.68a/`.

## 2026-09-10 — D32 empty NPC track iteration

- `CTrack::Track_GetNextTrack(NULL)` now returns NULL when `TrackCount` is zero
  instead of returning the non-member `TrackList[0]` sentinel. The inherited
  caller's existing no-tracks loop exit is consequently reachable.
- A zero/two-track iterator fixture fails against the verbatim 0.68A source and
  verifies zero iterations or exactly two iterations followed by NULL.
- Public engine commit `fadce84` retains authored `%defaultvalue%` metadata and
  applies it to omitted fields in application-declared schemas. The generic
  public fixture distinguishes authored zero from an omitted 255 default.
- Real-data measurement shows `ActorAlpha` itself is absent from the level's
  `NonPlayerCharacter` declaration (`action=zero`), so no default is available
  for that field. No private alpha heuristic was added.

Affected recovered file: `upstream-0.68a/Track.cpp`.

## 2026-09-10 — D33b declaration defaults and D34 electric arithmetic

- Extended the generated 67-class schema table to carry application defaults
  directly from `GE_DefaultValue` pragmas. The 65 current classes are derived
  from the inherited 0.76 header; the two recovered NPC declarations retain
  their exact 0.68A pragmas beside the recovered structs.
- Public engine commit `7e93709` establishes instance, level, declaration,
  then zero precedence. No content-specific value or alpha heuristic exists.
- Replaced the electric effect's six-pair `int` accumulator with a 64-bit
  accumulator while preserving its random-call count, divisor, and float
  result. The deterministic fixture checks both extrema and legacy-safe parity.
- Added stage-5-only NPC position receipts: the bounded hidden controller
  snapshots actor positions after tick one and reports retained/moved counts at
  tick 120. This measurement does not change gameplay or claim visual success.

Affected inherited file: `upstream/CElectric.cpp`. Generated/recovered boundary:
`upstream-0.68a/LegacyNPCEntityDataTypes.h`.

## 2026-09-10 — RF-12 round 15 input transport and weapon wheel

- The human desktop launcher now fixes SDL to its X11 backend before startup.
  This avoids the Fedora Plasma Wayland long-press/compose layer that consumed
  the physical A, S and D holds before Reality Factory's compatibility input
  stack received them. The application remains windowed and chooses the
  pointer's display through the existing host policy.
- SDL wheel detents are retained in a bounded accumulator, drained one per
  input frame, and translated into two new internal next/previous-weapon
  actions. `CWeapon::CycleWeapon` traverses the existing slot table and calls
  the inherited selection path; it never creates inventory or changes counts.
- A host fixture covers detent preservation and flipped-wheel direction. A
  real-data measurement target confirms W/S/A/D reach actions 1/2/5/6 and
  moving states 1/2/3/4, and confirms one wheel step selects slot 1 then the
  reverse step returns to slot 0.

Affected inherited files: `upstream/CInput.cpp`, `upstream/CCommonData.cpp`,
`upstream/CWeapon.cpp`, `upstream/Cweapon.h`, and `upstream/RGFStatus.h`.
Platform boundary: `adapters/input_compat.cpp`, `adapters/input_compat.h`, and
`tools/launch_human.sh`.
