# Puts the project-local toolchain (installed by tools/setup.sh) on PATH for
# the current shell. Source it:
#
#     source tools/env.sh
#
# Nothing here touches system or user-wide settings.

MH2_TOOLS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MH2_ROOT="$(dirname "$MH2_TOOLS")"
MH2_DEPS="$MH2_TOOLS/deps/linux-x64"
export MH2_ROOT MH2_DEPS

# uv: pinned interpreter, venv and cache all live under tools/deps.
export UV_PYTHON_INSTALL_DIR="$MH2_DEPS/python"
export UV_PROJECT_ENVIRONMENT="$MH2_DEPS/venv"
export UV_CACHE_DIR="$MH2_DEPS/uv-cache"
export UV_PYTHON_PREFERENCE=only-managed
export UV_LINK_MODE=copy

for _mh2_p in objdiff wibo binutils uv venv/bin; do
    case ":$PATH:" in
        *":$MH2_DEPS/$_mh2_p:"*) ;;
        *) PATH="$MH2_DEPS/$_mh2_p:$PATH" ;;
    esac
done
export PATH
unset _mh2_p
