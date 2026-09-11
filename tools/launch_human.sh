#!/usr/bin/env bash
set -euo pipefail

project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
runner="$project_dir/build/realityfactory_linux"
game_dir="$project_dir/local/game"

if [[ ! -x "$runner" ]]; then
    printf 'Genesis3D test: missing runner; build %s first.\n' "$runner" >&2
    exit 1
fi
if [[ ! -f "$game_dir/RealityFactory.ini" ]]; then
    printf 'Genesis3D test: missing private game data at %s.\n' "$game_dir" >&2
    exit 1
fi

export SDL_VIDEODRIVER=x11
cd -- "$game_dir"
if [[ ${1-} == --dry-run ]]; then
    printf 'runner=%s\nconfig=%s\nvideo_driver=%s\n' "$runner" "$game_dir" "$SDL_VIDEODRIVER"
    exit 0
fi
exec "$runner" --human-launch "$@"
