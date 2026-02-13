#include "amdsettingsworker.hpp"

#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>
#include <Libraries/Filework/FileworkUtils.hpp>

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <vector>

// Alexey's troubles
#if (__cplusplus > 201402L)
#include <filesystem>
namespace stdfs = std::filesystem;
#else
#include <experimental/filesystem>
namespace stdfs = std::experimental::filesystem;
#endif

namespace Hardware {
namespace GPU
{

AMDSettingsWorker::AMDSettingsWorker() :
    CardSettingsWorker(),
    Libraries::SettingsFileWorker()
{

}

AMDSettingsWorker::~AMDSettingsWorker() {}

void AMDSettingsWorker::init(int64_t gpuId)
{
    this->gpuId = gpuId;
    setSettingsDir("");
    const std::string cardPath = "/sys/class/drm/card" + std::to_string(gpuId) + "/device/hwmon";

    DIR* cardDirectory;
    struct dirent* dirEntry;

    // Check if directory exist
    if (!Libraries::FileworkUtil::objectExist(cardPath))
    {
        COMPLOG_CRITICAL("AMD init error: no such directory", cardPath);
        return;
    }

    auto dirContent = Libraries::FileworkUtil::getContentPaths(cardPath);
    for (auto& dirC : dirContent)
    {
        if (dirC.find("hwmon/hwmon") != std::string::npos) {
            setSettingsDir(dirC);
            break;
        }
    }
}

bool AMDSettingsWorker::setPowerLimit(int64_t lim)
{
    setSetting("power1_cap", lim);
    return true;
}

Libraries::JOptional<int64_t> AMDSettingsWorker::getPowerLimitDefault()
{
    auto val = Libraries::safeSton<int64_t>( getSetting("power1_cap_default") );
    if (val.has_value()) {
        return val.value() / 1e6;
    }
    return {};
}

Libraries::JOptional<int64_t> AMDSettingsWorker::getPowerLimitCurrent()
{
    auto val = Libraries::safeSton<int64_t>( getSetting("power1_cap") );

    if (val.has_value()) {
        return val.value() / 1e6;
    }
    return {};
}

Libraries::JOptional<int64_t> AMDSettingsWorker::getPowerCurrent()
{
    // На случай если нет первого, смотреть второй. Не знаю с чем связаны такие приколы
    auto fileValue = getSetting("power1_average");

    if (fileValue.tryGetValue() == "") {
        fileValue = getSetting("power1_input");
    }

    auto val = Libraries::safeSton<int64_t>( fileValue );
    if (val.has_value()) {
        return val.value() / 1e6;
    }
    return {};
}

Libraries::JOptional<int64_t> AMDSettingsWorker::getPowerMax()
{
    auto val = Libraries::safeSton<int64_t>( getSetting("power1_cap_max") );

    if (val.has_value()) {
        return val.value() / 1e6;
    }
    return {};
}

Libraries::JOptional<int64_t> AMDSettingsWorker::getPowerMin()
{
    auto val = Libraries::safeSton<int64_t>( getSetting("power1_cap_min") );

    if (val.has_value()) {
        return val.value() / 1e6;
    }
    return {};
}

bool AMDSettingsWorker::setTemp(int64_t temp)
{
    // TODO: Set critical temperature
    COMPLOG_WARNING("AMD Temperature set failed");
    return false;
}

Libraries::JOptional<int64_t> AMDSettingsWorker::getTempCurrent()
{
    auto val = Libraries::safeSton<int64_t>(  getSetting("temp1_input") );

    if (val.has_value()) {
        return val.value() / 1e3;
    }
    return {};
}

Libraries::JOptional<int64_t> AMDSettingsWorker::getTempMax()
{
    auto val = Libraries::safeSton<int64_t>( getSetting("temp1_crit") );

    if (val.has_value()) {
        return val.value() / 1e3;
    }
    return {};
}

bool AMDSettingsWorker::setFan(int64_t fanSpeed)
{
    if ((fanSpeed < 0) || (fanSpeed > 255))
    {
        COMPLOG_ERROR("Invalid fan value to set: %i", fanSpeed);
        return false;
    }
    setSetting("pwm1", fanSpeed); // 0..255
    return true;
}

bool AMDSettingsWorker::setFanMode(GPUFanOperatingMode fanMode)
{
    // 1,2,3 (auto, man, unknown)
    switch (fanMode)
    {
    case GPUFanOperatingMode::autoState: setSetting("pwm1_enable", 1); break;
    case GPUFanOperatingMode::manualState: setSetting("pwm1_enable", 2); break;
    case GPUFanOperatingMode::amdUnknownState: setSetting("pwm1_enable", 3); break;
    default:
        COMPLOG_ERROR("AMD: Unknown fan state got");
        return false;
    }
    return true;
}

Libraries::JOptional<int64_t> AMDSettingsWorker::getFanCurrent()
{
    return Libraries::safeSton<int64_t>( getSetting("pwm1") );
}

}
}
