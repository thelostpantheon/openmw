# Wrapper toolchain for PS Vita cross-compilation
# Includes the VitaSDK toolchain and adds OpenMW-specific settings

if(NOT DEFINED ENV{VITASDK})
    set(ENV{VITASDK} "/usr/local/vitasdk")
    message(STATUS "VITASDK not set, defaulting to /usr/local/vitasdk")
endif()

include("$ENV{VITASDK}/share/vita.toolchain.cmake")

# -mcpu=cortex-a9 enables Cortex-A9-specific instruction scheduling (dual-issue, branch hints)
# -mfpu=neon enables NEON SIMD + VFPv3 floating point (NOT vfpv4 — Cortex-A9 lacks VFPv4
# instructions like vfma.f64, which cause "undefined instruction" crashes at runtime)
# Applied to all build types unconditionally (the Vita CPU is always Cortex-A9)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a9 -mfpu=neon" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a9 -mfpu=neon" CACHE STRING "" FORCE)

# Use -Os for release configurations (size matters on Vita) with safe optimizations.
# NOTE: Do NOT use -flto — causes undefined reference errors for OSG vtable/typeinfo
# symbols at link time (GCC 10 LTO slim objects in OSG .a libs are incompatible).
# NOTE: Do NOT use -ffast-math — generates VFPv4 fused multiply-add (vfma) instructions
# that don't exist on the Vita's Cortex-A9 (VFPv3 only). Crashes in osg::asciiToDouble.
set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG -ftree-vectorize -funroll-loops -fomit-frame-pointer" CACHE STRING "c++ Release flags" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG -ftree-vectorize -funroll-loops -fomit-frame-pointer" CACHE STRING "c Release flags" FORCE)

# All-static Vita builds have circular deps between libs (vitaGL ↔ SceGxm,
# freetype ↔ bz2, avformat ↔ avcodec, etc.). Wrap link with --start/end-group
# so the linker iterates until all symbols resolve, regardless of order.
set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> -Wl,--start-group <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group -o <TARGET>"
    CACHE STRING "" FORCE)
