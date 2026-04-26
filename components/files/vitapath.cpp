#ifdef __vita__

#include "vitapath.hpp"

#include <cstdlib>

namespace Files
{
    VitaPath::VitaPath(const std::string& applicationName)
        : mName(applicationName)
    {
    }

    std::filesystem::path VitaPath::getUserConfigPath() const
    {
        return std::filesystem::path("ux0:data/openmw/config");
    }

    std::filesystem::path VitaPath::getUserDataPath() const
    {
        return std::filesystem::path("ux0:data/openmw");
    }

    std::filesystem::path VitaPath::getGlobalConfigPath() const
    {
        // Same as user config — Vita has no system-wide config location
        return std::filesystem::path("ux0:data/openmw/config");
    }

    std::filesystem::path VitaPath::getLocalPath() const
    {
        // Read-only application directory inside the VPK
        return std::filesystem::path("app0:");
    }

    std::filesystem::path VitaPath::getGlobalDataPath() const
    {
        return std::filesystem::path("ux0:data/openmw");
    }

    std::filesystem::path VitaPath::getCachePath() const
    {
        return std::filesystem::path("ux0:data/openmw/cache");
    }

    std::vector<std::filesystem::path> VitaPath::getInstallPaths() const
    {
        return {};
    }

} /* namespace Files */

#endif /* __vita__ */
