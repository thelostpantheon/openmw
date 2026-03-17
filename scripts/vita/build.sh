#!/bin/bash
# Build OpenMW for PS Vita
#
# Prerequisites:
#   - VitaSDK installed (VITASDK env or /usr/local/vitasdk)
#   - vitaGL built (VITAGL_DIR env or ~/vitaGL)
#   - Dependencies installed: scripts/vita-deps/build-all.sh
#
# Usage: scripts/vita/build.sh [clean]
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${SRC_DIR}/build-vita"
VITASDK="${VITASDK:-/usr/local/vitasdk}"
VITAGL_DIR="${VITAGL_DIR:-${HOME}/vitaGL}"

export PATH="${VITASDK}/bin:${PATH}"

echo "=== OpenMW Vita Build ==="
echo "Source:  ${SRC_DIR}"
echo "Build:   ${BUILD_DIR}"
echo "VitaSDK: ${VITASDK}"
echo "vitaGL:  ${VITAGL_DIR}"

# Validate
[ -d "${VITASDK}" ] || { echo "ERROR: VitaSDK not found"; exit 1; }
[ -f "${VITAGL_DIR}/libvitaGL.a" ] || { echo "ERROR: vitaGL not built (run make in ${VITAGL_DIR})"; exit 1; }

# Clean build if requested
if [ "${1}" = "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# ICU host build (needed for data filtering during cross-compile)
ICU_HOST="${BUILD_DIR}/extern/icu-host-build"
if [ ! -f "${ICU_HOST}/bin/icupkg" ]; then
    echo "=== Building ICU host tools ==="
    # ICU source is fetched by cmake on first configure. If not present yet,
    # run cmake once to fetch it, then build host tools.
    if [ ! -f "${BUILD_DIR}/extern/fetched/icu/icu4c/source/configure" ]; then
        echo "ICU source not yet fetched. Running cmake to fetch dependencies..."
        cmake -DCMAKE_TOOLCHAIN_FILE="${SRC_DIR}/cmake/VitaToolchain.cmake" \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
              -DVITA=ON \
              -DVITAGL_DIR="${VITAGL_DIR}" \
              -DICU_HOST_BUILD_DIR="${ICU_HOST}" \
              "${SRC_DIR}" 2>&1 | tail -5 || true
    fi
    if [ -f "${BUILD_DIR}/extern/fetched/icu/icu4c/source/configure" ]; then
        mkdir -p "${ICU_HOST}"
        cd "${ICU_HOST}"
        "${BUILD_DIR}/extern/fetched/icu/icu4c/source/configure" \
            --enable-static --disable-shared \
            --disable-extras --disable-tests --disable-samples --disable-icuio
        make -j$(nproc)
        cd "${BUILD_DIR}"
    else
        echo "WARNING: ICU source not available. ICU data filtering will be skipped."
    fi
fi

# Configure
echo "=== Configuring ==="
cmake -DCMAKE_TOOLCHAIN_FILE="${SRC_DIR}/cmake/VitaToolchain.cmake" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
      -DVITA=ON \
      -DVITAGL_DIR="${VITAGL_DIR}" \
      -DICU_HOST_BUILD_DIR="${ICU_HOST}" \
      "${SRC_DIR}"

# Build full OSG (osgParticle/osgShadow needed but not in USED_OSG_COMPONENTS)
echo "=== Building OSG ==="
make -C _deps/osg-build -j$(nproc)

# Build eboot.bin
echo "=== Building OpenMW ==="
make -j$(nproc) eboot.bin-self

# Build VPK (needed for ATTRIBUTE2 extra memory mode)
echo "=== Building VPK ==="
make openmw.vpk-vpk

echo ""
echo "=== Build Complete ==="
echo "eboot.bin: ${BUILD_DIR}/apps/openmw/eboot.bin"
echo "VPK:       ${BUILD_DIR}/apps/openmw/openmw.vpk"
echo ""
echo "Deploy eboot.bin for quick iteration."
echo "Deploy VPK for first install (includes param.sfo with extra memory mode)."
