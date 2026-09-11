# Reality Factory, native Linux

A native 64-bit Linux port of the **Reality Factory** runtime, upstream 0.76.1.
Reality Factory is the game creation kit built on the Genesis3D engine, last
released for Windows in 2009.

**This is not finished.** It builds, it links, it opens a window, it renders.
It does not yet run a game end to end. Everything below is measured, and the
gaps are listed as plainly as the wins.

## What works

- Native x86-64 build on current Fedora and Ubuntu. No Windows headers, no
  `_WIN32` conditionals left in gameplay files.
- **SDL2 host**: window, input and joystick, replacing the Win32 message loop
  and the legacy DirectInput joystick boundary.
- **POSIX compatibility boundary** for the platform calls the original assumed.
- The virtual file system, menu system, GIF decoding, entity schemas and actor
  loading all pass their fixtures.
- Reaches **stage 4** and completes one frame, with a queued quit, clean
  teardown and exit 0, sanitizer clean under ASan and UBSan.

## What does not work yet

Read `FINDINGS_OUTSIDE_SCOPE.md` for the full record. The headline gaps:

- **Stage 5 stops on tick 1** at `CWeapon.cpp:1343`, `WeaponD[CurrentWeapon]`
  with `CurrentWeapon = -1`, before first-weapon assignment. Source review
  points to a missing previous-weapon guard. Not yet fixed.
- **No interactive launch.** The human launch path has no built-in-panel
  selector and no relative-mouse capture, so ESC-to-menu and capture release are
  untested.
- **Thread safety unproven.** `CCommonData::TimerFunction` reads `HasFocus` and
  `TimeCounter` from the periodic timer thread without synchronisation against
  main-thread access. ASan and UBSan do not establish race freedom; this needs
  ThreadSanitizer before anyone claims interactive timing is safe.
- **No visual equivalence testing** against the original Windows renderer.
- Test suites sit at **39 of 41** Release, 40 of 42 sanitized, with stages 4 and
  5 parked.

## Build

Fedora:

```bash
sudo dnf install gcc gcc-c++ cmake make pkgconf-pkg-config \
  mesa-libGL-devel libX11-devel SDL2-devel freeimage-devel
```

Ubuntu and Debian:

```bash
sudo apt install build-essential cmake pkg-config libgl1-mesa-dev \
  libx11-dev libsdl2-dev libfreeimage-dev
```

Then:

```bash
cmake -S . -B build
cmake --build build --parallel
```

Compiling needs no game assets. Running one does, and you must supply content
you are entitled to use. None is distributed here.

## Layout

- `upstream/` — the Reality Factory source with port changes. See
  `upstream/PROVENANCE.md` and `MODIFICATIONS.md`.
- `adapters/` — the Linux platform layer. Kept out of gameplay files on purpose.
- `tests/` — fixtures, including neutral content fixtures.
- `tools/` — build-time generators and the frame-clock audit.

## Licence, and one clause that matters

Reality Factory is distributed under an MIT-form grant by **Ralph Deane, 2002**,
with one added condition, quoted in full:

> It is prohibited to use Reality Factory for games that intentionally
> propagate genocide against a certain group, race, nation or religion
> existing in today's real world.

That clause is part of the licence and it travels with this port. Read
`upstream/license.txt` before using, modifying or redistributing. Porting the
runtime is unaffected by it; what you build with the runtime is not.

## Help is wanted

The gaps above are the work. Stage 5's weapon guard is the obvious next fix,
interactive input is the obvious next feature, and the thread race needs
somebody with ThreadSanitizer and patience. Open an issue, open a pull request,
or just tell us what broke on hardware we do not have.

Reality Factory was kept alive by its community for two decades after its last
official release. This is an attempt to move it somewhere it has never run.

Linux port by **Kadaken**, 2026. Upstream work is not ours and is not claimed.
