#!/bin/bash
# Master build script for all OpenMW Vita dependencies
#
# Dependencies that come from VitaSDK (no build needed):
#   zlib, SDL2, OpenAL, LZ4, FreeType
#
# Dependencies built by OpenMW's FetchContent (no pre-build needed):
#   Bullet, OpenSceneGraph, MyGUI, RecastNavigation, SQLite3, yaml-cpp
#
# Dependencies this script installs:
#   PVR_PSP2               (pre-built GPU driver stubs)
#   Boost                  (vdpm or source)
#   LuaJIT                 (vdpm or source)
#   FFmpeg                 (vdpm or source)
#   ICU                    (vdpm or source)
#
# Usage: ./build-all.sh [work_dir]
#   work_dir defaults to ./vita-deps-build/
#
# Prerequisites:
#   - VitaSDK installed and VITASDK env var set (or at /usr/local/vitasdk)
#   - wget, unzip, git, make, cmake
#   - For manual builds: gcc (host compiler for ICU stage 1)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKDIR="${1:-$(pwd)/vita-deps-build}"
VITASDK="${VITASDK:-/usr/local/vitasdk}"

echo "========================================"
echo " OpenMW Vita Dependencies Builder"
echo "========================================"
echo "VitaSDK: ${VITASDK}"
echo "Work dir: ${WORKDIR}"
echo "Script dir: ${SCRIPT_DIR}"
echo ""

if [ ! -d "${VITASDK}" ]; then
    echo "ERROR: VitaSDK not found at ${VITASDK}"
    echo "Install from https://vitasdk.org/ or set VITASDK env var"
    exit 1
fi

export VITASDK

mkdir -p "${WORKDIR}"

# Track status
FAILED=""

run_script() {
    local name="$1"
    local script="$2"
    echo ""
    echo "========================================"
    echo " Building: ${name}"
    echo "========================================"
    if bash "${script}" "${WORKDIR}/${name}"; then
        echo "[OK] ${name} complete"
    else
        echo "[FAIL] ${name} failed!"
        FAILED="${FAILED} ${name}"
    fi
}

# Build in dependency order
run_script "pvr-psp2" "${SCRIPT_DIR}/build-pvr-psp2.sh"
run_script "boost"  "${SCRIPT_DIR}/build-boost.sh"
run_script "luajit" "${SCRIPT_DIR}/build-luajit.sh"
run_script "ffmpeg" "${SCRIPT_DIR}/build-ffmpeg.sh"
run_script "icu"    "${SCRIPT_DIR}/build-icu.sh"

echo ""
echo "========================================"
echo " Summary"
echo "========================================"

if [ -z "${FAILED}" ]; then
    echo "All dependencies built successfully!"
    echo ""
    echo "Next steps:"
    echo "  mkdir build-vita && cd build-vita"
    echo "  cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/VitaToolchain.cmake \\"
    echo "        -DCMAKE_BUILD_TYPE=RelWithDebInfo \\"
    echo "        .."
    echo "  make -j\$(nproc)"
else
    echo "FAILED:${FAILED}"
    echo "Fix the errors above and re-run this script."
    exit 1
fi
