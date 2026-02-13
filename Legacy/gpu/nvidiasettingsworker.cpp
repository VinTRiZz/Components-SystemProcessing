#include "nvidiasettingsworker.hpp"

#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>
#include <Libraries/Filework/FileworkUtils.hpp>

#include <NVML/nvml.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>

#if (NVIDIA_MUST_BUILD == 1)
#include <cuda_runtime.h>
#endif


namespace Hardware {
namespace GPU
{

struct NvidiaSettingsWorker::NvidiaSettingsWorkerPrivate
{
    std::shared_ptr<PDisplay> pDisplay;

    nvmlDevice_t device;
    unsigned fanUnitCount {0};
};

NvidiaSettingsWorker::NvidiaSettingsWorker() :
    CardSettingsWorker(),
    d {new NvidiaSettingsWorkerPrivate}
{

}

NvidiaSettingsWorker::~NvidiaSettingsWorker() {}

void NvidiaSettingsWorker::init(int64_t gpuId)
{
    this->gpuId = gpuId;

    auto result = nvmlDeviceGetHandleByIndex(gpuId, &d->device);
    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error initing Nvidia GPU handle for id", gpuId, "Error:", nvmlErrorString(result));
        return;
    }

    result = nvmlDeviceGetNumFans(d->device, &d->fanUnitCount);
    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error initing Nvidia GPU fan unit count for id", gpuId, "Error:", nvmlErrorString(result));
        return;
    }
}

bool NvidiaSettingsWorker::setPowerLimit(int64_t lim)
{
    auto result = nvmlDeviceSetPowerManagementLimit(
        d->device, lim * 1000);
    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error setting Nvidia GPU power limit for id", gpuId, "Error:", nvmlErrorString(result));
        return false;
    }
    return true;
}

Libraries::JOptional<int64_t> NvidiaSettingsWorker::getPowerLimitDefault()
{
    unsigned int defaultPowerLimit {};
    auto result = nvmlDeviceGetPowerManagementDefaultLimit(d->device,
                                                      &defaultPowerLimit);
    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error getting Nvidia GPU default power limit for id", gpuId, "Error:", nvmlErrorString(result));
        return {};
    }
    return (defaultPowerLimit / 1e3);
}

Libraries::JOptional<int64_t> NvidiaSettingsWorker::getPowerLimitCurrent()
{
    unsigned int enforcedPowerLimit {0};
    auto result = nvmlDeviceGetEnforcedPowerLimit(d->device, &enforcedPowerLimit);
    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error get power limit for Nvidia GPU with id", gpuId, "Error:", nvmlErrorString(result));
        return {};
    }
    return (enforcedPowerLimit / 1e3);
}

Libraries::JOptional<int64_t> NvidiaSettingsWorker::getPowerCurrent()
{
    unsigned int powerUsage {0};
    auto result = nvmlDeviceGetPowerUsage(d->device, &powerUsage);
    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error get current power for Nvidia GPU settings with id", gpuId, "Error:", nvmlErrorString(result));
        return {};
    }
    return (powerUsage / 1e3);
}

Libraries::JOptional<int64_t> NvidiaSettingsWorker::getPowerMax()
{
    unsigned int powerLimitMin {}, powerLimitMax {};
    auto result = nvmlDeviceGetPowerManagementLimitConstraints(
            d->device, &powerLimitMin, &powerLimitMax);

    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error get power max for Nvidia GPU settings with id", gpuId, "Error:", nvmlErrorString(result));
        return {};
    }
    return (powerLimitMax / 1e3);
}

Libraries::JOptional<int64_t> NvidiaSettingsWorker::getPowerMin()
{
    unsigned int powerLimitMin {}, powerLimitMax {};
    auto result = nvmlDeviceGetPowerManagementLimitConstraints(
            d->device, &powerLimitMin, &powerLimitMax);
    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error getting power min for Nvidia GPU with id", gpuId, "Error:", nvmlErrorString(result));
        return {};
    }
    return (powerLimitMin / 1e3);
}

bool NvidiaSettingsWorker::setTemp(int64_t temp)
{
    int tempInt = temp; // TODO: Handle this thing (overflow caution!)

    auto result = nvmlDeviceSetTemperatureThreshold(
        d->device, NVML_TEMPERATURE_THRESHOLD_GPU_MAX, &tempInt);
    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error getting max temp for Nvidia GPU with id", gpuId, "Error:", nvmlErrorString(result));
        return {};
    }
    return temp;
}

Libraries::JOptional<int64_t> NvidiaSettingsWorker::getTempCurrent()
{
    unsigned int temp {};
    auto result = nvmlDeviceGetTemperature(
        d->device, NVML_TEMPERATURE_GPU, &temp);
    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error getting current temp for Nvidia GPU with id", gpuId, "Error:", nvmlErrorString(result));
        return {};
    }
    return temp;
}

Libraries::JOptional<int64_t> NvidiaSettingsWorker::getTempMax()
{
    unsigned int temp {};
    auto result = nvmlDeviceGetTemperatureThreshold(
        d->device, NVML_TEMPERATURE_THRESHOLD_GPU_MAX, &temp);
    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error getting max temp for Nvidia GPU with id", gpuId, "Error:", nvmlErrorString(result));
        return {};
    }
    return temp;
}

bool NvidiaSettingsWorker::setFan(int64_t fanSpeed)
{
    // Get average fan speed
    for (int i = 0; i < d->fanUnitCount; i++) {
        auto result = nvmlDeviceSetFanSpeed_v2(d->device, i, fanSpeed);
        if (result != NVML_SUCCESS) {
            COMPLOG_ERROR("Error setting fan mode for Nvidia GPU with id", gpuId, "Error:", nvmlErrorString(result));
            return {};
        }
    }
    return true;
}

bool NvidiaSettingsWorker::setFanMode(GPUFanOperatingMode fanMode)
{
    nvmlFanControlPolicy_t policy;
    switch (fanMode)
    {
    case GPUFanOperatingMode::autoState:
        policy = NVML_FAN_POLICY_TEMPERATURE_CONTINOUS_SW;
        break;

    case GPUFanOperatingMode::manualState:
        policy = NVML_FAN_POLICY_MANUAL;
        break;

    default:
        COMPLOG_ERROR("Nvidia: got unknown fan state");
        return false;
    }

    for (int i = 0; i < d->fanUnitCount; i++) {
        auto result = nvmlDeviceSetFanControlPolicy(d->device, i, policy);
        if (result != NVML_SUCCESS) {
            COMPLOG_ERROR("Error setting fan mode for Nvidia GPU with id", gpuId, "Error:", nvmlErrorString(result));
            continue;
        }
    }

    return true;
}

Libraries::JOptional<int64_t> NvidiaSettingsWorker::getFanCurrent()
{
    // Get average fan speed
    unsigned speed {0};
    auto res = nvmlDeviceGetFanSpeed(d->device, &speed);
    if (res != NVML_SUCCESS) {
        COMPLOG_ERROR("Error getting fan devices for Nvidia GPU with id", gpuId, "Error:", nvmlErrorString(res));
        return {};
    }
    return speed;
}

Libraries::JOptional<std::string> NvidiaSettingsWorker::getCudaVersion() const {
#if (NVIDIA_MUST_BUILD == 1)
    int tmpVal {0};
    cudaRuntimeGetVersion(&tmpVal);

    return std::to_string(tmpVal / 1000) + "." + std::to_string(tmpVal % 100 / 10);
#else
    COMPLOG_ERROR("Nvidia Built without CUDA: unknown version");
    return {};
#endif
}

std::pair<int64_t, int64_t> NvidiaSettingsWorker::getFanLimits() const
{
    // Get the current core clock frequency
    unsigned int minFanValue {0};
    unsigned int maxFanValue {0};
    auto result = nvmlDeviceGetMinMaxFanSpeed(d->device, &minFanValue, &maxFanValue);
    if (result != NVML_SUCCESS) {
        COMPLOG_ERROR("Error getting fan limits for Nvidia GPU Error:", nvmlErrorString(result));
        return {};
    }
    return std::make_pair(minFanValue, maxFanValue);
}

void NvidiaSettingsWorker::setDisplay(std::shared_ptr<PDisplay> pDisplay)
{
    d->pDisplay = pDisplay;
}

}
}
