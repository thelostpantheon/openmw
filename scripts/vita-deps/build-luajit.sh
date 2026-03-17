#!/bin/bash
# Build/install LuaJIT for OpenMW Vita port
# Uses vitasdk/packages if available, otherwise builds from source
# Note: JIT is disabled on Vita (console W^X restriction), but the fast
# interpreter is still significantly faster than standard Lua 5.1.
set -e

VITASDK="${VITASDK:-/usr/local/vitasdk}"
SYSROOT="${VITASDK}/arm-vita-eabi"

echo "=== Installing LuaJIT for Vita ==="

WORKDIR="${1:-$(pwd)/vita-deps-build/luajit}"

echo "Building LuaJIT from source..."
echo "Work dir: ${WORKDIR}"

mkdir -p "${WORKDIR}"
cd "${WORKDIR}"

if [ ! -f luajit_done ]; then
    if [ ! -d "LuaJIT-Vita" ]; then
        git clone https://github.com/SonicMastr/LuaJIT-Vita.git
    fi

    cd LuaJIT-Vita/src

    make clean || true
    make \
        HOST_CC="gcc -m32" \
        CROSS=arm-vita-eabi- \
        TARGET_SYS=PSP2 \
        TARGET_FLAGS="-marm -fno-optimize-sibling-calls" \
        BUILDMODE=static \
        XCFLAGS="-DLUAJIT_DISABLE_JIT -DLUAJIT_DISABLE_FFI" \
        -j$(nproc)

    # Install headers and library
    cp luajit.h lua.h lualib.h lauxlib.h lua.hpp luaconf.h lj_arch.h "${SYSROOT}/include/"
    cp libluajit.a "${SYSROOT}/lib/"

    cd ../..
    touch luajit_done
fi

echo "=== LuaJIT installation complete ==="
