#!/usr/bin/env bash
# Builds the 13 targets of the matching build from your copy of the game, with
# one command, and checks each against the original.
#
# Usage: tools/build.sh [-j N] [NINJA_TARGET...]
#   -j N     build with N jobs (default: ninja's)
#   TARGET   a ninja target in place of the default, for example: progress,
#            provenance, plaincheck
#
# What it does, from the repository's root or anywhere:
#   1. tools/setup.sh, when anything is missing or out of date (the pinned
#      toolchain into tools/deps/; the first run downloads about 400 MB);
#   2. python tools/configure.py (extracts the targets from your copy of the
#      game, splits them, writes build.ninja);
#   3. ninja, which ends with one line per target: OK for all 13 means each
#      built file's SHA-1 equals the game's.
#
# Your copy of the game: MH2_ORIG_DIR, or the repository's parent directory.
# Nothing else needs to be set: no source tools/env.sh, no PATH.
set -euo pipefail

usage() { sed -n '2,/^set /p' "$0" | sed '$d' | sed 's/^# \{0,1\}//'; }

jobs=()
targets=()
while [ $# -gt 0 ]; do
    case "$1" in
        -h | --help) usage; exit 0 ;;
        -j) [ $# -ge 2 ] || { echo "-j needs a number" >&2; exit 2; }; jobs=(-j "$2"); shift ;;
        -j[0-9]*) jobs=(-j "${1#-j}") ;;
        -*) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
        *) targets+=("$1") ;;
    esac
    shift
done

TOOLS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$TOOLS")"
cd "$ROOT"

setup_args=(--if-needed)
"$TOOLS/setup.sh" "${setup_args[@]}"
# shellcheck source=env.sh
source "$TOOLS/env.sh"

echo "[..]   configure (extract, split, write build.ninja)"
python "$TOOLS/configure.py"
echo "[..]   ninja"
ninja "${jobs[@]}" "${targets[@]}"
if [ "${#targets[@]}" -eq 0 ]; then
    # the verdict again, last
    echo
    mapfile -t built < <(awk '!/^#/ && NF { print "build/out/" $3 }' config/targets.sha1)
    python "$TOOLS/check_targets.py" "${built[@]}"
fi
