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

# The directories of the tools, the first one listed ending last on PATH.
_mh2_dirs=()
_mh2_dirs+=(objdiff wibo)
_mh2_dirs+=(binutils uv venv/bin)
for _mh2_p in "${_mh2_dirs[@]}"; do
    case ":$PATH:" in
        *":$MH2_DEPS/$_mh2_p:"*) ;;
        *) PATH="$MH2_DEPS/$_mh2_p:$PATH" ;;
    esac
done
export PATH
unset _mh2_p _mh2_dirs
