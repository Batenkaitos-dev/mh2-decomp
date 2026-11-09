#!/usr/bin/env bash
# Installs the pinned, project-local toolchain on Linux x64.
#
# Everything is installed under tools/deps/ (gitignored), with no administrator
# rights and no system packages. Every download is verified against the SHA-256 in
# tools/versions.txt and kept in tools/deps/downloads/, so a re-run
# downloads nothing it already has. Safe to re-run.
#
# What it needs from the system: a downloader (curl or wget), tar and gzip.
# It checks for them and says what is missing; it installs nothing with apt.
#
# Usage: tools/setup.sh [--if-needed]
#   --if-needed  do nothing, and print nothing, when the last run finished
#                with this script, versions.txt and the Python lock unchanged
set -euo pipefail

usage() { sed -n '2,/^set /p' "$0" | sed '$d' | grep -v '^# @' | sed 's/^# \{0,1\}//'; }

IF_NEEDED=0
for arg in "$@"; do
    case "$arg" in
        --if-needed) IF_NEEDED=1 ;;
        -h | --help) usage; exit 0 ;;
        *) echo "unknown option: $arg" >&2; usage >&2; exit 2 ;;
    esac
done

if [ "$(uname -s)" != Linux ] || [ "$(uname -m)" != x86_64 ]; then
    echo "setup.sh runs on 64-bit Linux only. On Windows, the README says how to set up." >&2
    exit 1
fi

TOOLS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=env.sh
source "$TOOLS/env.sh"    # sets MH2_ROOT, MH2_DEPS, PATH, UV_* variables
DOWNLOADS="$TOOLS/deps/downloads"
GH=https://github.com

# The stamp of a finished run: a hash of what the installed tools depend on, and
# the scope of the run: with --if-needed, a run whose stamp is current does nothing.
STAMP="$MH2_DEPS/.setup-stamp"
SCOPE=full
stamp_hash() { cat "${BASH_SOURCE[0]}" "$TOOLS/versions.txt" "$MH2_ROOT/pyproject.toml" "$MH2_ROOT/uv.lock" | sha256sum | cut -d' ' -f1; }
if [ "$IF_NEEDED" = 1 ] && [ -f "$STAMP" ]; then
    read -r s_hash s_scope <"$STAMP" || true
    if [ "$s_hash" = "$(stamp_hash)" ] && { [ "$s_scope" = full ] || [ "$SCOPE" = basic ]; }; then
        exit 0
    fi
fi

# tools/versions.txt -> V_<KEY> shell variables
while IFS='=' read -r key value; do
    value="${value%$'\r'}"
    if [[ "$key" =~ ^[A-Z0-9_]+$ ]]; then printf -v "V_$key" '%s' "$value"; fi
done <"$TOOLS/versions.txt"

# --- What the system must provide ---------------------------------------------
# Nothing is installed from here: a missing program is named, with the Debian
# or Ubuntu package that has it.
missing=()          # what is missing, and the Debian or Ubuntu package that has each
packages=()
have() { command -v "$1" >/dev/null 2>&1; }
need() { missing+=("$1"); packages+=("$2"); }
if have curl; then DOWNLOADER=curl; elif have wget; then DOWNLOADER=wget; else
    DOWNLOADER=
    need "a downloader: curl or wget" curl
fi
have tar || need tar tar
have gzip || need gzip gzip
have sha256sum || need sha256sum coreutils
if [ "${#missing[@]}" -gt 0 ]; then
    echo "tools/setup.sh needs these programs, and can't find them:" >&2
    printf '  - %s
' "${missing[@]}" >&2
    if have apt-get; then
        echo "On Debian or Ubuntu:  sudo apt-get install ca-certificates ${packages[*]}" >&2
    else
        echo "Install them with your distribution's package manager, then run tools/setup.sh again." >&2
    fi
    exit 1
fi


# --- Helpers ------------------------------------------------------------------

# download URL FILE: curl, or wget when there is no curl
download() {
    if [ "$DOWNLOADER" = curl ]; then
        curl -fsSL --retry 3 -o "$2" "$1"
    else
        wget -q --tries=3 -O "$2" "$1"
    fi
}

# get_pinned URL FILENAME SHA256 [MIRROR_URL]: download into tools/deps/downloads (cached),
# verify, and print the local path. If URL fails or the file does not match the SHA-256,
# MIRROR_URL (when given) is tried, with the same check.
get_pinned() {
    local url=$1 name=$2 sha=$3 mirror=${4:-} u
    local dest="$DOWNLOADS/$name"
    mkdir -p "$DOWNLOADS"
    if [ -f "$dest" ] && echo "$sha  $dest" | sha256sum -c --status; then
        echo "$dest"
        return
    fi
    for u in "$url" "$mirror"; do
        [ -n "$u" ] || continue
        echo "       downloading $u" >&2
        if download "$u" "$dest.part" && echo "$sha  $dest.part" | sha256sum -c --status; then
            mv "$dest.part" "$dest"
            echo "$dest"
            return
        fi
        echo "       download failed, or its SHA-256 does not match (expected $sha): $u" >&2
        rm -f "$dest.part"
    done
    echo "could not get $name with the expected SHA-256" >&2
    return 1
}

# install_step NAME DIR ID FUNCTION [OTHER_ID]: run FUNCTION DIR unless DIR already holds
# install id ID (or OTHER_ID). DIR is emptied first. FUNCTION may set STEP_ID to the id
# to record instead of ID.
install_step() {
    local name=$1 dir=$2 id=$3 fn=$4 other=${5:-}
    case "$dir" in
        "$MH2_DEPS"/*) ;;
        *) echo "refusing to install outside tools/deps: $dir" >&2; exit 1 ;;
    esac
    if [ -f "$dir/.mh2-installed" ] && { [ "$(cat "$dir/.mh2-installed")" = "$id" ] || [ "$(cat "$dir/.mh2-installed")" = "${other:-$id}" ]; }; then
        echo "[ok]   $name $(cat "$dir/.mh2-installed")"
        return
    fi
    echo "[..]   $name $id"
    rm -rf "$dir"
    mkdir -p "$dir"
    STEP_ID=""
    "$fn" "$dir"
    id=${STEP_ID:-$id}
    echo "$id" >"$dir/.mh2-installed"
    echo "[done] $name $id"
}

mkdir -p "$MH2_DEPS"

# --- uv, CPython, Python packages -------------------------------------------
install_uv() {
    local f
    f=$(get_pinned "$GH/astral-sh/uv/releases/download/$V_UV_VERSION/uv-x86_64-unknown-linux-gnu.tar.gz" \
        "uv-$V_UV_VERSION-x86_64-unknown-linux-gnu.tar.gz" "$V_UV_SHA256_LINUX")
    tar -xzf "$f" -C "$1" --strip-components 1
}
install_step uv "$MH2_DEPS/uv" "$V_UV_VERSION" install_uv

echo "[..]   CPython $V_PYTHON_VERSION and Python packages (uv.lock)"
uv python install --quiet "$V_PYTHON_VERSION"
uv sync --locked --quiet --project "$MH2_ROOT"
echo "[done] CPython $V_PYTHON_VERSION and Python packages"

# --- PS2 binutils (R5900) ----------------------------------------------------
install_binutils() {
    local f
    f=$(get_pinned "$GH/decompals/binutils-mips-ps2-decompals/releases/download/$V_BINUTILS_PS2_VERSION/binutils-mips-ps2-decompals-linux-x86-64.tar.gz" \
        "binutils-mips-ps2-decompals-$V_BINUTILS_PS2_VERSION-linux-x86-64.tar.gz" "$V_BINUTILS_PS2_SHA256_LINUX")
    tar -xzf "$f" -C "$1"
}
install_step binutils-mips-ps2-decompals "$MH2_DEPS/binutils" "$V_BINUTILS_PS2_VERSION" install_binutils

# --- Matching compiler: MWCC for PS2 via wibo --------------------------------
install_wibo() {
    local f
    f=$(get_pinned "$GH/decompals/wibo/releases/download/$V_WIBO_VERSION/wibo-x86_64" \
        "wibo-$V_WIBO_VERSION-x86_64" "$V_WIBO_SHA256")
    install -m 755 "$f" "$1/wibo"
}
install_step wibo "$MH2_DEPS/wibo" "$V_WIBO_VERSION" install_wibo

install_mwcc() {
    local f
    f=$(get_pinned "$GH/decompme/compilers/releases/download/compilers/mwcps2-$V_MWCC_PS2_VERSION.tar.gz" \
        "mwcps2-$V_MWCC_PS2_VERSION.tar.gz" "$V_MWCC_PS2_SHA256")
    tar -xzf "$f" -C "$1"
}
install_step "MWCC PS2" "$MH2_DEPS/mwcc" "$V_MWCC_PS2_VERSION" install_mwcc

# --- objdiff ------------------------------------------------------------------
install_objdiff() {
    local base="$GH/encounter/objdiff/releases/download/$V_OBJDIFF_VERSION" cli gui
    cli=$(get_pinned "$base/objdiff-cli-linux-x86_64" "objdiff-cli-$V_OBJDIFF_VERSION-linux-x86_64" "$V_OBJDIFF_CLI_SHA256_LINUX")
    gui=$(get_pinned "$base/objdiff-linux-x86_64" "objdiff-$V_OBJDIFF_VERSION-linux-x86_64" "$V_OBJDIFF_GUI_SHA256_LINUX")
    install -m 755 "$cli" "$1/objdiff-cli"
    install -m 755 "$gui" "$1/objdiff"
}
install_step objdiff "$MH2_DEPS/objdiff" "$V_OBJDIFF_VERSION" install_objdiff


# --- clang for tools/content_gen.py: the one in Zig's release ------------------
# content_gen reads the C files' types from a clang parse (CONTRIBUTING,
# "Content"). Zig's PyPI wheel (a zip) holds one program with a clang inside:
# only that program is unpacked, to tools/deps/linux-x64/zig/zig, and
# `zig clang` is clang's command line.
install_zig() {
    local f
    f=$(get_pinned "$V_ZIG_URL_LINUX" "${V_ZIG_URL_LINUX##*/}" "$V_ZIG_SHA256_LINUX")
    python -I -c 'import sys, zipfile
with zipfile.ZipFile(sys.argv[1]) as z, open(sys.argv[2], "wb") as out:
    out.write(z.read("ziglang/zig"))' "$f" "$1/zig"
    chmod 755 "$1/zig"
    "$1/zig" clang --version >/dev/null
}
install_step "clang (Zig)" "$MH2_DEPS/zig" "$V_ZIG_VERSION" install_zig


# --- Verify -------------------------------------------------------------------
echo
check_args=()
python "$TOOLS/check_env.py" "${check_args[@]}"
echo "$(stamp_hash) $SCOPE" >"$STAMP"
echo
if [ -f "$TOOLS/build.sh" ]; then
    echo "Setup complete. tools/build.sh builds with no further step; for the tools in your own shell: source tools/env.sh"
else
    echo "Setup complete. In each new shell, run:  source tools/env.sh"
fi
