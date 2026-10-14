#!/usr/bin/env bash
# Installs the pinned, project-local toolchain of the matching build on
# Linux x64.
#
# Everything is installed under tools/deps/ (gitignored), except a few
# distribution packages (curl, git, tar, clang), which are installed with apt on
# Debian/Ubuntu (skip with --no-apt). Every download is verified against
# the SHA-256 in tools/versions.txt and kept in tools/deps/downloads/, so a
# re-run downloads nothing it already has. Safe to re-run.
#
# Usage: tools/setup.sh [--no-apt]
#   --no-apt     don't install distribution packages (install them yourself)
set -euo pipefail

usage() { sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'; }

NO_APT=0
for arg in "$@"; do
    case "$arg" in
        --no-apt) NO_APT=1 ;;
        -h | --help) usage; exit 0 ;;
        *) echo "unknown option: $arg" >&2; usage >&2; exit 2 ;;
    esac
done

if [ "$(uname -s)" != Linux ] || [ "$(uname -m)" != x86_64 ]; then
    echo "setup.sh supports Linux x86_64 only (on Windows, run it inside WSL)" >&2
    exit 1
fi

TOOLS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=env.sh
source "$TOOLS/env.sh"    # sets MH2_ROOT, MH2_DEPS, PATH, UV_* variables
DOWNLOADS="$TOOLS/deps/downloads"
GH=https://github.com

# tools/versions.txt -> V_<KEY> shell variables
while IFS='=' read -r key value; do
    value="${value%$'\r'}"
    if [[ "$key" =~ ^[A-Z0-9_]+$ ]]; then printf -v "V_$key" '%s' "$value"; fi
done <"$TOOLS/versions.txt"

# --- Distribution packages ---------------------------------------------------
# clang: tools/content_gen.py reads the C files' types from its parse
# (CONTRIBUTING, "Content")
pkgs=(ca-certificates curl git tar clang)

if [ "$NO_APT" = 0 ]; then
    if ! command -v apt-get >/dev/null; then
        echo "apt-get not found. Install the equivalents of these packages for your" >&2
        echo "distribution, then re-run with --no-apt:" >&2
        echo "  ${pkgs[*]}" >&2
        exit 1
    fi
    SUDO=
    if [ "$(id -u)" -ne 0 ]; then SUDO=sudo; fi
    echo "[..]   apt packages"
    $SUDO apt-get update -qq
    $SUDO env DEBIAN_FRONTEND=noninteractive apt-get install -y -qq --no-install-recommends "${pkgs[@]}" >/dev/null
    echo "[done] apt packages"
else
    echo "[skip] apt packages (--no-apt). Required: ${pkgs[*]}"
fi

# --- Helpers ------------------------------------------------------------------

# get_pinned URL FILENAME SHA256: download into tools/deps/downloads (cached),
# verify, and print the local path.
get_pinned() {
    local url=$1 name=$2 sha=$3
    local dest="$DOWNLOADS/$name"
    mkdir -p "$DOWNLOADS"
    if [ -f "$dest" ] && echo "$sha  $dest" | sha256sum -c --status; then
        echo "$dest"
        return
    fi
    echo "       downloading $url" >&2
    curl -fsSL --retry 3 -o "$dest.part" "$url"
    if ! echo "$sha  $dest.part" | sha256sum -c --status; then
        echo "SHA-256 mismatch for $name" >&2
        echo "  expected $sha" >&2
        echo "  got      $(sha256sum "$dest.part" | cut -d' ' -f1)" >&2
        rm -f "$dest.part"
        return 1
    fi
    mv "$dest.part" "$dest"
    echo "$dest"
}

# install_step NAME DIR ID FUNCTION: run FUNCTION DIR unless DIR already holds
# install id ID. DIR is emptied first.
install_step() {
    local name=$1 dir=$2 id=$3 fn=$4
    case "$dir" in
        "$MH2_DEPS"/*) ;;
        *) echo "refusing to install outside tools/deps: $dir" >&2; exit 1 ;;
    esac
    if [ -f "$dir/.mh2-installed" ] && [ "$(cat "$dir/.mh2-installed")" = "$id" ]; then
        echo "[ok]   $name $id"
        return
    fi
    echo "[..]   $name $id"
    rm -rf "$dir"
    mkdir -p "$dir"
    "$fn" "$dir"
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

# --- Verify -------------------------------------------------------------------
echo
python "$TOOLS/check_env.py"
echo
echo "Setup complete. In each new shell, run:  source tools/env.sh"
