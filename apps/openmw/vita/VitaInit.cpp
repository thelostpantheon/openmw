#ifdef __vita__

#include "VitaInit.h"

#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/power.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/sysmodule.h>

#include <LinearMath/btAlignedAllocator.h>

#include <components/debug/debuglog.hpp>
#include <components/sceneutil/lightingmethod.hpp>
#include <components/settings/values.hpp>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <malloc.h>
#include <new>
#include <unistd.h>

// These symbols MUST be at file scope with C linkage — the Vita loader and
// newlib crt0 resolve them as unmangled C symbols at process startup.
// Inside a C++ namespace they get name-mangled and the loader can't find them,
// silently falling back to defaults (wrong heap size, 256KB stack -> overflow).
// These symbols MUST be at file scope with C linkage — the Vita loader and
// newlib crt0 resolve them as unmangled C symbols at process startup.
extern "C" {
// Extra memory mode (ATTRIBUTE2=12) grants ~357MB total user RAM.
unsigned int _newlib_heap_size_user = 224 * 1024 * 1024;
unsigned int sceUserMainThreadStackSize = 2 * 1024 * 1024;

// Write an unsigned int as decimal to fd (no heap allocation)
static void writeUint(SceUID fd, unsigned int val)
{
    char buf[16];
    int pos = 0;
    if (val == 0) { buf[pos++] = '0'; }
    else {
        char tmp[16]; int t = 0;
        while (val > 0) { tmp[t++] = '0' + (val % 10); val /= 10; }
        while (t > 0) buf[pos++] = tmp[--t];
    }
    sceIoWrite(fd, buf, pos);
}

// Early heap diagnostic — runs before C++ static constructors.
// Uses only SCE kernel calls (no malloc/snprintf).
__attribute__((constructor(101)))
static void vita_early_heap_check()
{
    sceIoMkdir("ux0:data/openmw", 0777);
    SceUID fd = sceIoOpen("ux0:data/openmw/early_diag.log",
        SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) return;

    sceIoWrite(fd, "=== Early heap diagnostic ===\n", 30);

    // Check free memory before malloc test
    SceKernelFreeMemorySizeInfo freeInfo;
    freeInfo.size = sizeof(freeInfo);
    int ret = sceKernelGetFreeMemorySize(&freeInfo);
    sceIoWrite(fd, "getFreeMemorySize ret=", 22);
    writeUint(fd, (unsigned int)ret);
    sceIoWrite(fd, "\n", 1);
    if (ret >= 0) {
        sceIoWrite(fd, "free_user=", 10);
        writeUint(fd, (unsigned int)freeInfo.size_user);
        sceIoWrite(fd, " free_cdram=", 12);
        writeUint(fd, (unsigned int)freeInfo.size_cdram);
        sceIoWrite(fd, " free_phycont=", 14);
        writeUint(fd, (unsigned int)freeInfo.size_phycont);
        sceIoWrite(fd, "\n", 1);
    }

    // Test malloc
    void* p = malloc(64);
    if (p) {
        sceIoWrite(fd, "malloc(64): OK\n", 15);
        free(p);
    } else {
        sceIoWrite(fd, "malloc(64): FAILED\n", 19);
    }

    // Report heap size requested
    sceIoWrite(fd, "heap_size_user=", 15);
    writeUint(fd, _newlib_heap_size_user);
    sceIoWrite(fd, "\n", 1);

    sceIoClose(fd);
}

void vitaBreadcrumb(const char* msg)
{
    SceUID fd = sceIoOpen("ux0:data/openmw/boot.log",
        SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 0777);
    if (fd >= 0)
    {
        int len = 0;
        while (msg[len]) ++len;
        sceIoWrite(fd, msg, len);
        sceIoWrite(fd, "\n", 1);
        sceIoClose(fd);
    }
}

void vitaTimedBreadcrumb(const char* msg)
{
    SceUID fd = sceIoOpen("ux0:data/openmw/boot.log",
        SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 0777);
    if (fd >= 0)
    {
        SceUInt64 us = sceKernelGetProcessTimeWide();
        unsigned long ms = (unsigned long)(us / 1000ULL);
        char buf[256];
        int pos = 0;
        buf[pos++] = '[';
        char numBuf[16];
        int numLen = 0;
        unsigned long tmp = ms;
        do { numBuf[numLen++] = '0' + (tmp % 10); tmp /= 10; } while (tmp > 0);
        for (int i = numLen - 1; i >= 0; --i) buf[pos++] = numBuf[i];
        buf[pos++] = ']';
        buf[pos++] = ' ';
        int msgLen = 0;
        while (msg[msgLen] && pos < 254) buf[pos++] = msg[msgLen++];
        buf[pos++] = '\n';
        sceIoWrite(fd, buf, pos);
        sceIoClose(fd);
    }
}

void vitaMemBreadcrumb(const char* msg)
{
    SceUID fd = sceIoOpen("ux0:data/openmw/boot.log",
        SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 0777);
    if (fd >= 0)
    {
        struct mallinfo mi = mallinfo();
        unsigned int usedKB = (unsigned int)(mi.uordblks / 1024);
        unsigned int freeKB = (unsigned int)(mi.fordblks / 1024);
        char buf[300];
        int len = snprintf(buf, sizeof(buf), "[VitaMem] %s | used: %uKB | free: %uKB\n",
            msg, usedKB, freeKB);
        if (len > 0)
            sceIoWrite(fd, buf, (size_t)len);
        sceIoClose(fd);
    }
}

// Vita newlib stubs for POSIX functions referenced at link time
int fchown(int /*fd*/, uid_t /*owner*/, gid_t /*group*/)
{
    errno = ENOSYS;
    return -1;
}

ssize_t readlink(const char* /*path*/, char* /*buf*/, size_t /*bufsiz*/)
{
    errno = ENOSYS;
    return -1;
}

int symlink(const char* /*target*/, const char* /*linkpath*/)
{
    errno = ENOSYS;
    return -1;
}

int link(const char* /*oldpath*/, const char* /*newpath*/)
{
    errno = ENOSYS;
    return -1;
}

int lchown(const char* /*path*/, uid_t /*owner*/, gid_t /*group*/)
{
    errno = ENOSYS;
    return -1;
}

int utimensat(int /*dirfd*/, const char* /*pathname*/,
              const void* /*times*/, int /*flags*/)
{
    return 0;
}

// SQLite VFS stubs — built with SQLITE_OS_OTHER=1 since Vita lacks
// flock/fcntl/mmap needed by the Unix VFS.
int sqlite3_os_init(void) { return 0; /* SQLITE_OK */ }
int sqlite3_os_end(void) { return 0; }

} // extern "C"

namespace Vita
{
    static char s_emergencyBuf[256];
    static size_t s_minHeapFree = SIZE_MAX;
    static SceUID s_debugLogFd = -1;

    // Emergency reserve: freed on first OOM to give breathing room for
    // the failing allocation to succeed. The per-frame watchdog then
    // detects the high memory state and flushes caches next frame.
    static constexpr size_t EMERGENCY_RESERVE_SIZE = 4 * 1024 * 1024; // 4MB
    static void* s_emergencyReserve = nullptr;

    size_t getFreeUserMemory()
    {
        struct mallinfo mi = mallinfo();
        return static_cast<size_t>(mi.fordblks);
    }

    void logMemoryStatus(const char* label)
    {
        struct mallinfo mi = mallinfo();
        size_t heapFree = static_cast<size_t>(mi.fordblks);

        if (heapFree < s_minHeapFree)
            s_minHeapFree = heapFree;

        char buf[192];
        snprintf(buf, sizeof(buf), "[VitaMem] %s | used: %uKB | free: %uKB | min: %uKB",
            label,
            static_cast<unsigned>(mi.uordblks / 1024),
            static_cast<unsigned>(heapFree / 1024),
            static_cast<unsigned>(s_minHeapFree / 1024));
        breadcrumb(buf);
        Log(Debug::Info) << buf;
    }

    static void releaseEmergencyReserve()
    {
        void* reserve = __atomic_exchange_n(&s_emergencyReserve, nullptr, __ATOMIC_ACQ_REL);
        if (reserve)
            free(reserve);
    }

    static void logOomAndExit(const char* prefix, size_t size)
    {
        struct mallinfo mi = mallinfo();
        char buf[192];
        if (size > 0)
            snprintf(buf, sizeof(buf),
                "[%s] malloc(%u) failed | used: %uKB | free: %uKB",
                prefix, static_cast<unsigned>(size),
                static_cast<unsigned>(mi.uordblks / 1024),
                static_cast<unsigned>(mi.fordblks / 1024));
        else
            snprintf(buf, sizeof(buf),
                "[%s] Heap: %uKB used / %uKB free",
                prefix,
                static_cast<unsigned>(mi.uordblks / 1024),
                static_cast<unsigned>(mi.fordblks / 1024));

        breadcrumb(buf);

        SceUID fd = sceIoOpen("ux0:data/openmw/critical_oom.log",
            SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 0777);
        if (fd >= 0)
        {
            int len = snprintf(s_emergencyBuf, sizeof(s_emergencyBuf), "%s\n", buf);
            if (len > 0)
                sceIoWrite(fd, s_emergencyBuf, static_cast<size_t>(len));
            sceIoClose(fd);
        }
        sceKernelExitProcess(1);
    }

    static void* vitaBtAlloc(size_t size)
    {
        void* ptr = malloc(size);
        if (!ptr)
        {
            // Try releasing emergency reserve and retry
            releaseEmergencyReserve();
            ptr = malloc(size);
            if (!ptr)
                logOomAndExit("BT_OOM", size);
        }
        return ptr;
    }

    static void vitaBtFree(void* ptr)
    {
        free(ptr);
    }

    static void vitaNewHandler()
    {
        // Release emergency reserve to give breathing room for the
        // retried allocation. The per-frame watchdog will detect
        // high memory and flush caches on the next frame.
        if (s_emergencyReserve)
        {
            releaseEmergencyReserve();
            breadcrumb("[OOM_RECOVERY] Released 4MB emergency reserve");
            // Return to let operator new retry the allocation
            return;
        }
        // Reserve already spent — unrecoverable
        logOomAndExit("OOM", 0);
    }

    void debugLog(const char* msg)
    {
        if (s_debugLogFd < 0)
        {
            s_debugLogFd = sceIoOpen("ux0:data/openmw/debug.log",
                SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 0777);
        }
        if (s_debugLogFd >= 0)
        {
            char localBuf[256];
            int len = snprintf(localBuf, sizeof(localBuf), "%s\n", msg);
            if (len > 0)
                sceIoWrite(s_debugLogFd, localBuf,
                    std::min(static_cast<size_t>(len), sizeof(localBuf) - 1));
        }
    }

    // Check if a filename has a given extension (case-insensitive)
    static bool hasExtension(const char* name, const char* ext)
    {
        int nameLen = 0;
        while (name[nameLen]) ++nameLen;
        int extLen = 0;
        while (ext[extLen]) ++extLen;
        if (nameLen < extLen) return false;
        for (int i = 0; i < extLen; ++i)
        {
            char a = name[nameLen - extLen + i];
            char b = ext[i];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (a != b) return false;
        }
        return true;
    }

    // Read entire file into a malloc'd buffer (caller must free). Returns size, or -1.
    static int readFileToBuffer(const char* path, char** outBuf)
    {
        SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
        if (fd < 0) { *outBuf = nullptr; return -1; }
        SceOff size = sceIoLseek(fd, 0, SCE_SEEK_END);
        sceIoLseek(fd, 0, SCE_SEEK_SET);
        if (size <= 0 || size > 64 * 1024) { sceIoClose(fd); *outBuf = nullptr; return -1; }
        *outBuf = static_cast<char*>(malloc(static_cast<size_t>(size) + 1));
        if (!*outBuf) { sceIoClose(fd); return -1; }
        sceIoRead(fd, *outBuf, static_cast<size_t>(size));
        (*outBuf)[size] = '\0';
        sceIoClose(fd);
        return static_cast<int>(size);
    }

    // Check if cfg already contains "key=value" (e.g. "content=myfoo.esp")
    // Also matches quoted form: key="value"
    static bool cfgContainsEntry(const char* cfgBuf, const char* key, const char* value)
    {
        if (!cfgBuf) return false;
        int keyLen = 0;
        while (key[keyLen]) ++keyLen;
        int valLen = 0;
        while (value[valLen]) ++valLen;

        const char* p = cfgBuf;
        while (*p)
        {
            // Match "key=" at start of line
            bool match = true;
            for (int i = 0; i < keyLen && match; ++i)
                if (p[i] != key[i]) match = false;
            if (match && p[keyLen] == '=')
            {
                const char* v = p + keyLen + 1;
                // Skip optional opening quote
                if (*v == '"') ++v;
                bool valMatch = true;
                for (int i = 0; i < valLen && valMatch; ++i)
                    if (v[i] != value[i]) valMatch = false;
                if (valMatch && (v[valLen] == '\n' || v[valLen] == '\r'
                    || v[valLen] == '\0' || v[valLen] == '"'))
                    return true;
            }
            // Skip to next line
            while (*p && *p != '\n') ++p;
            if (*p == '\n') ++p;
        }
        return false;
    }

    // Auto-detect content files in Data Files/ and append missing ones to user config.
    // Scans top-level only. Additive — never removes existing config lines.
    static void autoDetectContent()
    {
        static const char* dataDir = "ux0:data/openmw/Data Files";
        static const char* cfgPath = "ux0:data/openmw/config/openmw.cfg";

        // Read existing config
        char* cfgBuf = nullptr;
        readFileToBuffer(cfgPath, &cfgBuf);

        // Scan Data Files directory
        SceUID dfd = sceIoDopen(dataDir);
        if (dfd < 0)
        {
            free(cfgBuf);
            return;
        }

        // Collect filenames to append (ESMs first, then ESPs, then omwaddons, then BSAs)
        struct FileEntry { char name[256]; int type; }; // type: 0=esm, 1=esp, 2=omwaddon, 3=bsa
        static const int MAX_FILES = 128;
        FileEntry* found = static_cast<FileEntry*>(malloc(sizeof(FileEntry) * MAX_FILES));
        int foundCount = 0;

        if (found)
        {
            SceIoDirent dirent;
            memset(&dirent, 0, sizeof(dirent));
            while (sceIoDread(dfd, &dirent) > 0 && foundCount < MAX_FILES)
            {
                int type = -1;
                if (hasExtension(dirent.d_name, ".esm")) type = 0;
                else if (hasExtension(dirent.d_name, ".esp")) type = 1;
                else if (hasExtension(dirent.d_name, ".omwaddon")) type = 2;
                else if (hasExtension(dirent.d_name, ".bsa")) type = 3;

                if (type >= 0)
                {
                    const char* key = (type == 3) ? "fallback-archive" : "content";
                    if (!cfgContainsEntry(cfgBuf, key, dirent.d_name))
                    {
                        memcpy(found[foundCount].name, dirent.d_name, 256);
                        found[foundCount].type = type;
                        foundCount++;
                    }
                }
                memset(&dirent, 0, sizeof(dirent));
            }
        }
        sceIoDclose(dfd);
        free(cfgBuf);

        if (foundCount == 0)
        {
            free(found);
            return;
        }

        // Sort: ESMs first, then ESPs, then omwaddons, then BSAs.
        // Within each type, alphabetical order.
        for (int i = 0; i < foundCount - 1; ++i)
            for (int j = i + 1; j < foundCount; ++j)
            {
                bool swap = false;
                if (found[i].type > found[j].type)
                    swap = true;
                else if (found[i].type == found[j].type && strcmp(found[i].name, found[j].name) > 0)
                    swap = true;
                if (swap)
                {
                    FileEntry tmp = found[i];
                    found[i] = found[j];
                    found[j] = tmp;
                }
            }

        // Append to config
        SceUID fd = sceIoOpen(cfgPath, SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 0666);
        if (fd >= 0)
        {
            sceIoWrite(fd, "\n# Auto-detected content\n", 25);
            for (int i = 0; i < foundCount; ++i)
            {
                const char* key = (found[i].type == 3) ? "fallback-archive=" : "content=";
                char line[300];
                int lineLen = snprintf(line, sizeof(line), "%s%s\n", key, found[i].name);
                if (lineLen > 0)
                    sceIoWrite(fd, line, lineLen);

                char buf[300];
                snprintf(buf, sizeof(buf), "Auto-detected: %s%s", key, found[i].name);
                breadcrumb(buf);
            }
            sceIoClose(fd);
        }
        free(found);
    }

    void ensureDataDirectories()
    {
        sceIoMkdir("ux0:data/openmw", 0777);
        sceIoMkdir("ux0:data/openmw/config", 0777);
        sceIoMkdir("ux0:data/openmw/data", 0777);
        sceIoMkdir("ux0:data/openmw/saves", 0777);
        sceIoMkdir("ux0:data/openmw/cache", 0777);
        sceIoMkdir("ux0:data/openmw/screenshots", 0777);

        // Create default user openmw.cfg if it doesn't exist.
        // The bundled app0:/openmw.cfg has paths but no content= lines.
        // All Vita installs use the same data path, so we can safely generate this.
        static const char* userCfgPath = "ux0:data/openmw/config/openmw.cfg";
        SceUID fd = sceIoOpen(userCfgPath, SCE_O_RDONLY, 0);
        if (fd >= 0) {
            sceIoClose(fd);  // Already exists, don't overwrite
        } else {
            fd = sceIoOpen(userCfgPath, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
            if (fd >= 0) {
                static const char defaultCfg[] =
                    "# Auto-generated OpenMW Vita user config\n"
                    "# Place Morrowind data files in ux0:data/openmw/Data Files/\n"
                    "\n"
                    "content=Morrowind.esm\n"
                    "content=Tribunal.esm\n"
                    "content=Bloodmoon.esm\n"
                    "\n"
                    "fallback-archive=Morrowind.bsa\n"
                    "fallback-archive=Tribunal.bsa\n"
                    "fallback-archive=Bloodmoon.bsa\n";
                sceIoWrite(fd, defaultCfg, sizeof(defaultCfg) - 1);
                sceIoClose(fd);
                breadcrumb("Created default user openmw.cfg");
            }
        }

        // Scan Data Files/ and append any new .esm/.esp/.omwaddon/.bsa to config
        autoDetectContent();
    }

    void initClocks()
    {
        scePowerSetArmClockFrequency(444);
        scePowerSetGpuClockFrequency(222);
        scePowerSetBusClockFrequency(222);

        char buf[128];
        snprintf(buf, sizeof(buf), "Vita clocks: CPU=%dMHz GPU=%dMHz Bus=%dMHz",
            scePowerGetArmClockFrequency(),
            scePowerGetGpuClockFrequency(),
            scePowerGetBusClockFrequency());
        debugLog(buf);
    }

    void initialize()
    {
        // Force the linker to keep sceUserMainThreadStackSize — it lives in its
        // own section (-fdata-sections) and --gc-sections strips it because the
        // Vita loader's reference is invisible to the linker.  The asm volatile
        // with "m" constraint creates an unoptimizable memory reference that
        // makes the linker treat the section as reachable.
        asm volatile("" :: "m"(sceUserMainThreadStackSize));

        // Environment setup (must be after main, not in constructors)
        setenv("TMPDIR", "ux0:data/openmw/cache", 1);
        setenv("HOME", "ux0:data/openmw", 1);
        setenv("OSG_NOTIFY_LEVEL", "ALWAYS", 0);

        ensureDataDirectories();

        // Rotate logs
        sceIoRemove("ux0:data/openmw/boot.log.prev");
        sceIoRename("ux0:data/openmw/boot.log", "ux0:data/openmw/boot.log.prev");
        sceIoRemove("ux0:data/openmw/debug.log.prev");
        sceIoRename("ux0:data/openmw/debug.log", "ux0:data/openmw/debug.log.prev");

        breadcrumb("BOOT: Vita::initialize() start");

        // Log what std::terminate is called with
        std::set_terminate([]() {
            breadcrumb("[TERMINATE] std::terminate called");
            std::exception_ptr ep = std::current_exception();
            if (ep) {
                try { std::rethrow_exception(ep); }
                catch (const std::exception& e) {
                    char buf[256];
                    snprintf(buf, sizeof(buf), "[TERMINATE] exception: %s", e.what());
                    breadcrumb(buf);
                }
                catch (...) { breadcrumb("[TERMINATE] non-std exception"); }
            } else {
                breadcrumb("[TERMINATE] no active exception (noexcept violation?)");
            }
            abort();
        });

        std::set_new_handler(vitaNewHandler);
        btAlignedAllocSetCustom(vitaBtAlloc, vitaBtFree);

        // Allocate emergency reserve — freed on first OOM to buy time
        s_emergencyReserve = malloc(EMERGENCY_RESERVE_SIZE);
        if (s_emergencyReserve)
            breadcrumb("BOOT: 4MB emergency reserve allocated");
        else
            breadcrumb("BOOT: WARNING - emergency reserve allocation failed");

        initClocks();

        // IME sysmodule is loaded later by Vita::initIme()

        char initBuf[128];
        snprintf(initBuf, sizeof(initBuf),
            "Vita Platform Initialized: heap=%uMB stack=%uKB",
            _newlib_heap_size_user / (1024 * 1024),
            sceUserMainThreadStackSize / 1024);
        debugLog(initBuf);
        breadcrumb(initBuf);
        breadcrumb("BOOT: Vita::initialize() done");
    }

    void breadcrumb(const char* msg)
    {
        vitaBreadcrumb(msg);
    }

    int getHeapUsedMB()
    {
        struct mallinfo mi = mallinfo();
        return mi.uordblks / (1024 * 1024);
    }

    bool isMemoryPressure(int thresholdMB)
    {
        return getHeapUsedMB() > thresholdMB;
    }

    void replenishEmergencyReserve()
    {
        if (!s_emergencyReserve)
        {
            s_emergencyReserve = malloc(EMERGENCY_RESERVE_SIZE);
            if (s_emergencyReserve)
                breadcrumb("[MemWatchdog] Emergency reserve replenished");
        }
    }

    void applySettingsOverrides()
    {
        debugLog("Applying Vita platform defaults...");

        // --- Video: render at 640x368, hardware scaler upscales to 960x544 ---
        Settings::video().mResolutionX.set(640);
        Settings::video().mResolutionY.set(368);
        Settings::video().mAntialiasing.set(0);
        Settings::video().mFramerateLimit.set(30.0f);

        // --- Shaders: FFP only (vitaGL cannot compile OpenMW GLSL shaders) ---
        Settings::shaders().mLightingMethod.set(SceneUtil::LightingMethod::FFP);
        Settings::shaders().mForceShaders.set(false);
        Settings::shaders().mForcePerPixelLighting.set(false);
        Settings::shaders().mAutoUseObjectNormalMaps.set(false);
        Settings::shaders().mAutoUseObjectSpecularMaps.set(false);
        Settings::shaders().mAutoUseTerrainNormalMaps.set(false);
        Settings::shaders().mAutoUseTerrainSpecularMaps.set(false);
        Settings::shaders().mSoftParticles.set(false);
        Settings::shaders().mWeatherParticleOcclusion.set(false);
        Settings::shaders().mAntialiasAlphaTest.set(false);
        Settings::shaders().mAdjustCoverageForAlphaTest.set(false);
        Settings::shaders().mMaxLights.set(4);
        Settings::shaders().mMaximumLightDistance.set(256.0f);
        Settings::shaders().mLightFadeStart.set(0.8f);
        Settings::shaders().mLightBoundsMultiplier.set(0.5f);

        // --- Shadows: fully disabled ---
        Settings::shadows().mEnableShadows.set(false);

        // --- Post-processing: disabled ---
        Settings::postProcessing().mEnabled.set(false);

        // --- Terrain: maximum LOD reduction, no distant terrain/object paging ---
        // Object paging requires QuadTreeWorld (distant terrain), which adds significant
        // memory overhead. Cross-object STAT merge + small feature culling provide similar
        // draw call reduction without the memory cost.
        Settings::terrain().mDistantTerrain.set(false);
        Settings::terrain().mLodFactor.set(12.0f);                // more aggressive LOD for better performance
        Settings::terrain().mObjectPaging.set(false);
        Settings::terrain().mObjectPagingActiveGrid.set(false);
        Settings::terrain().mCompositeMapResolution.set(64);      // lower resolution terrain textures

        // --- Camera: draw distance balanced for playability ---
        Settings::camera().mViewingDistance.set(2048.0f);
        Settings::camera().mReverseZ.set(false);
        Settings::camera().mSmallFeatureCulling.set(true);
        Settings::camera().mSmallFeatureCullingPixelSize.set(4.0f);
        Settings::camera().mFieldOfView.set(50.0f);
        Settings::camera().mVitaDynamicFog.set(true);

        // --- Water: minimal quality ---
        Settings::water().mShader.set(false);
        Settings::water().mRefraction.set(false);
        Settings::water().mReflectionDetail.set(0);
        Settings::water().mRainRippleDetail.set(0);

        // --- Fog: keep it simple ---
        Settings::fog().mUseDistantFog.set(false);
        Settings::fog().mRadialFog.set(false);
        Settings::fog().mExponentialFog.set(false);
        Settings::fog().mSkyBlending.set(false);

        // --- Game: actor processing range controls NPC visibility ---
        // Actors beyond this get setNodeMask(0) = invisible.
        // Must be >= viewing distance for NPCs to stay visible at draw distance.
        Settings::game().mActorsProcessingRange.set(3584);        // minimum allowed value

        // --- Physics: disabled async (data abort in btGjkEpaSolver2::Penetration
        // when main thread modifies world state while physics worker is mid-computation) ---
        Settings::physics().mAsyncNumThreads.set(1);

        // --- Cells: preload exterior grid only, conservative cache ---
        // Async preload runs on its own thread; ICO compile ops run on the
        // main GL thread so this is safe even though vitaGL can't handle
        // multi-thread draws. Goal: hide cell-transition stutter.
        Settings::cells().mPreloadEnabled.set(true);
        Settings::cells().mPreloadNumThreads.set(1);
        Settings::cells().mPreloadExteriorGrid.set(true);
        Settings::cells().mPreloadFastTravel.set(false);
        Settings::cells().mPreloadDoors.set(false);
        Settings::cells().mPreloadInstances.set(false);
        Settings::cells().mPreloadCellCacheMin.set(1);
        Settings::cells().mPreloadCellCacheMax.set(1);
        Settings::cells().mCacheExpiryDelay.set(0.25f);
        Settings::cells().mTargetFramerate.set(30.0f);
        Settings::cells().mPointersCacheSize.set(40);

        // --- Navigator: completely disabled ---
        Settings::navigator().mEnable.set(false);

        // --- Lua: no worker thread, reduced memory ---
        Settings::lua().mLuaNumThreads.set(0);
        Settings::lua().mMemoryLimit.set(static_cast<std::uint64_t>(8 * 1024 * 1024));
        Settings::lua().mLuaProfiler.set(false);

        // --- General: texture quality ---
        Settings::general().mAnisotropy.set(0);

        // --- Map: reduced resolution to save RAM ---
        Settings::map().mLocalMapResolution.set(32);
        Settings::map().mLocalMapWidgetSize.set(128);
        Settings::map().mGlobalMapCellSize.set(4);

        // --- GUI: scale for 960x544 screen, enable controller menus ---
        Settings::gui().mScalingFactor.set(0.8f);
        Settings::gui().mFontSize.set(16);
        Settings::gui().mControllerMenus.set(true);

        // --- Groundcover: disabled ---
        Settings::groundcover().mEnabled.set(false);

        // --- Sound: minimal buffer cache ---
        Settings::sound().mBufferCacheMin.set(1);
        Settings::sound().mBufferCacheMax.set(8);

        // --- Stereo: disabled ---
        Settings::stereo().mStereoEnabled.set(false);

        debugLog("Vita platform defaults applied.");
        logMemoryStatus("Post-defaults");
    }

    bool isSelectHeld()
    {
        sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
        SceCtrlData pad{};
        sceCtrlPeekBufferPositive(0, &pad, 1);
        bool held = (pad.buttons & SCE_CTRL_SELECT) != 0;
        if (held)
            breadcrumb("SELECT held — forcing mod rescan");
        return held;
    }

} // namespace Vita

#endif // __vita__
