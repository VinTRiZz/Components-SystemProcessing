#include "nvidiafrequencymanager.h"

#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Constants/ConstantMaster.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>
#include <Libraries/Datawork/Numberic.hpp>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

//#define NVIDIA_MUST_BUILD 1

#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>
#include <NVML/nvml.h>

#if (NVIDIA_MUST_BUILD == 1)
#include <cuda_runtime.h>
#endif // NVIDIA_MUST_BUILD

namespace Hardware
{
namespace GPU
{


int64_t getVoltage(const std::string& nvidiaSmiOutput) {

    std::vector<std::string> lines;
    boost::iter_split(lines, nvidiaSmiOutput, boost::first_finder("\n"));

    bool nextLineIsVoltage {false};
    for (auto& line : lines) {
        boost::tokenizer valueSplitter(line);
        int curpos {};
        for (auto& tkn : valueSplitter) {

            if (nextLineIsVoltage) {
                if (curpos < 1) {
                    curpos++;
                    continue;
                }

                try {
                    return std::stoull(tkn);
                } catch (std::invalid_argument& ex) {
                    // TODO: Shit happens
                    continue;
                }
            }

            if (tkn.find("Voltage") != std::string::npos) {
                nextLineIsVoltage = true;
                break;
            }
        }
    }

    return 0;
}












struct NvidiaFrequencyManager::Impl
{
    nvmlDevice_t device;

    std::shared_ptr<PDisplay> pDisplay;

    int64_t defaultCoreFreqBuffer {0};
    int64_t defaultCoreVoltageBuffer {0};
    int64_t defaultMemFreqBuffer {0};
    int64_t defaultMemVoltageBuffer {0};
};

NvidiaFrequencyManager::NvidiaFrequencyManager(const std::string& gpuId) :
    AbstractFrequencyManager(gpuId),
    d {new Impl}
{
    auto result = nvmlDeviceGetHandleByIndex(Libraries::safeSton<unsigned>(gpuId), &d->device);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error initing Nvidia GPU with id", gpuId, "Error:", nvmlErrorString(result));
        return;
    }
    updateFreqs();
}

NvidiaFrequencyManager::NvidiaFrequencyManager(int64_t gpuId) :
    AbstractFrequencyManager(gpuId),
    d {new Impl}
{
    auto result = nvmlDeviceGetHandleByIndex(gpuId, &d->device);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error initing Nvidia GPU with id", gpuId, "Error:", nvmlErrorString(result));
        return;
    }
    updateFreqs();
}

NvidiaFrequencyManager::~NvidiaFrequencyManager()
{

}

bool NvidiaFrequencyManager::updateFreqs()
{
    unsigned int resultValue {};

    // Get the default memory and core clock frequency
    unsigned int clockMHz {0};
    auto result = nvmlDeviceGetDefaultApplicationsClock(d->device, NVML_CLOCK_MEM, &clockMHz);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error getting default memory freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
    } else {
        d->defaultMemFreqBuffer = clockMHz;
    }

    result = nvmlDeviceGetDefaultApplicationsClock(d->device, NVML_CLOCK_GRAPHICS, &clockMHz);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error getting default core freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
    } else {
        d->defaultCoreFreqBuffer = clockMHz;
    }

    // Get min max freqs
    unsigned minClockMHz {0}, maxClockMHz {0};
    result = nvmlDeviceGetMaxClockInfo(d->device, NVML_CLOCK_MEM, &maxClockMHz );
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error getting min max memory freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
    } else {
        m_limits.m_maxMemFreq = maxClockMHz;
        m_limits.m_minMemFreq = minClockMHz;
    }
    // Нет такой, минимальной, в нвидиа
//    result = nvmlDeviceGetMinClockInfo(d->device, NVML_CLOCK_MEM, &minClockMHz );
//    if (result != NVML_SUCCESS) {
//        LOG_ERROR("Error getting min max core freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
//        return false;
//    }

    minClockMHz = maxClockMHz = 0;
    result = nvmlDeviceGetMaxClockInfo(d->device, NVML_CLOCK_GRAPHICS, &maxClockMHz );
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error getting min max core freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
    } else {
        m_limits.m_maxCoreFreq = maxClockMHz;
        m_limits.m_minCoreFreq = minClockMHz;
    }
    // Нет такой, минимальной, в нвидиа
//    result = nvmlDeviceGetMinClockInfo(d->device, NVML_CLOCK_MEM, &minClockMHz );
//    if (result != NVML_SUCCESS) {
//        LOG_ERROR("Error getting min max core freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
//        return false;
//    }

    return true;
}

void NvidiaFrequencyManager::resetToDefault()
{
    auto result = nvmlDeviceResetGpuLockedClocks(d->device);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error resetting freqs to default for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
        return;
    }

    result = nvmlDeviceResetMemoryLockedClocks(d->device);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error resetting memory freqs to default for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
        return;
    }
}

FrequencyValue_t NvidiaFrequencyManager::getCurrentMemoryFreq() const
{
    // Get the current memory clock frequency
    unsigned int clockMHz {0};
    auto result = nvmlDeviceGetClockInfo(d->device, NVML_CLOCK_GRAPHICS, &clockMHz);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error getting core freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
        return {};
    }
    return clockMHz;
}

FrequencyValue_t NvidiaFrequencyManager::getCurrentCoreFreq() const
{
    // Get the current memory clock frequency
    unsigned int clockMHz {0};
    auto result = nvmlDeviceGetClockInfo(d->device, NVML_CLOCK_MEM, &clockMHz);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error getting memory freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
        return {};
    }
    return clockMHz;
}

FrequencyValue_t NvidiaFrequencyManager::getCurrentMemoryLock() const
{
//    nvmlDeviceSetGpuLockedClocks(nvmlDevice_t device, unsigned int minGpuClockMHz, unsigned int maxGpuClockMHz);
    LOG_DEBUG("Not written function:", __PRETTY_FUNCTION__);
    return {};
}

FrequencyValue_t NvidiaFrequencyManager::getCurrentCoreLock() const
{
//    nvmlDeviceSetGpuLockedClocks(nvmlDevice_t device, unsigned int minGpuClockMHz, unsigned int maxGpuClockMHz);
    LOG_DEBUG("Not written function:", __PRETTY_FUNCTION__);
    return {};
}

FrequencyValue_t NvidiaFrequencyManager::getCurrentCoreVoltage() const
{
//    if ((d->pDisplay.use_count() < 1) || (!d->pDisplay)) {
//        LOG_ERROR("Display init error!", __PRETTY_FUNCTION__);
//        return {};
//    }

//    int coreVoltage {0};

//    auto res = XNVCTRLQueryAttribute (
//        static_cast<Display*>(d->pDisplay.get()),
//        0,
//        0,
//        NV_CTRL_GPU_CURRENT_CORE_VOLTAGE,
//        &coreVoltage
//    );
//    if (res != True) {
//        LOG_ERROR("Error getting current core voltage (no such parameter)");
//        return {};
//    }

//    return coreVoltage;




    // Временно?
    std::string inputStr;

    // nvidia-smi -q --id=0 -d VOLTAGE
    if (!Libraries::ProcessInvoker::invoke("nvidia-smi", Libraries::StringList("-q", "--id=" + m_gpuId, "-d", "VOLTAGE"), inputStr)) {
        LOG_ERROR("Error getting Nvidia core voltage for", m_gpuId);
        return 0;
    }

    return getVoltage(inputStr);
}

FrequencyValue_t NvidiaFrequencyManager::getCurrentMemVoltage() const
{
    LOG_DEBUG("Not written function:", __PRETTY_FUNCTION__);
    return {};
}

FrequencyValue_t NvidiaFrequencyManager::getCurrentCoreOffset() const
{
//    auto gpuId = Libraries::safeSton<int>(m_gpuId);
//    NVCTRLAttributeValidValuesRec coreClockOffsets;

//    auto res = XNVCTRLQueryValidTargetAttributeValues(
//            d->pDisplay, NV_CTRL_TARGET_TYPE_GPU, gpuId,
//            0, NV_CTRL_GPU_NVCLOCK_OFFSET, &coreClockOffsets);

//    if (res != True) {
//        return {};
//    }
//    return coreClockOffsets.u.bits.ints;

    int offsetInfo {0};
    auto res = nvmlDeviceGetGpcClkVfOffset ( d->device, &offsetInfo );
    if (res != NVML_SUCCESS) {
        LOG_ERROR("Error getting gpc clock offset for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(res));
        return {};
    }
    return offsetInfo;
}

FrequencyValue_t NvidiaFrequencyManager::getCurrentMemoryOffset() const
{
//    auto gpuId = Libraries::safeSton<int>(m_gpuId);
//    NVCTRLAttributeValidValuesRec coreClockOffsets;

//    auto res = XNVCTRLQueryValidTargetAttributeValues(
//            d->pDisplay, NV_CTRL_TARGET_TYPE_GPU, gpuId,
//            0, NV_CTRL_STRING_GPU_CURRENT_CLOCK_FREQS, &coreClockOffsets);

//    if (res != True) {
//        return {};
//    }
//    return coreClockOffsets.u.bits.ints;

    int offsetInfo {0};
    auto res = nvmlDeviceGetMemClkVfOffset ( d->device, &offsetInfo );
    if (res != NVML_SUCCESS) {
        LOG_ERROR("Error getting mem clock offset for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(res));
        return {};
    }
    return offsetInfo;
}

bool NvidiaFrequencyManager::setCoreOffset(int64_t clockOffset)
{
    LOG_DEBUG("Not written function:", __PRETTY_FUNCTION__);
    return false;
}

bool NvidiaFrequencyManager::setMemoryOffset(int64_t clockOffset)
{
    LOG_DEBUG("Not written function:", __PRETTY_FUNCTION__);
    return false;
}

bool NvidiaFrequencyManager::setCoreLock(int64_t clockLock)
{
    auto result = nvmlDeviceSetGpuLockedClocks(d->device, clockLock, clockLock);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error setting lock core freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
        return false;
    }
    return true;
}

bool NvidiaFrequencyManager::setMemoryLock(int64_t clockLock)
{
    auto result = nvmlDeviceSetMemoryLockedClocks(d->device, clockLock, clockLock);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error setting lock mem freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
        return false;
    }
    return true;
}

bool NvidiaFrequencyManager::setCoreVoltage(int64_t clockVoltage)
{
    auto result = nvmlDeviceSetGpcClkVfOffset(d->device, clockVoltage);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error setting lock core freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
        return false;
    }
    return true;
}

bool NvidiaFrequencyManager::setMemoryVoltage(int64_t clockVoltage)
{
    auto result = nvmlDeviceSetMemClkVfOffset(d->device, clockVoltage);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error setting mem voltage for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
        return false;
    }
    return false;
}

FrequencyValue_t NvidiaFrequencyManager::getDefaultCoreClock() const
{
    unsigned defaultClock {0};
    auto result = nvmlDeviceGetDefaultApplicationsClock(d->device, NVML_CLOCK_MEM, &defaultClock);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error getting default core clock for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
        return false;
    }
    return {};
}

FrequencyValue_t NvidiaFrequencyManager::getDefaultCoreVoltage() const
{
    LOG_DEBUG("Not written function:", __PRETTY_FUNCTION__);
    return {};
}

FrequencyValue_t NvidiaFrequencyManager::getDefaultMemoryClock() const
{
    int offsetVal {0};
    auto result = nvmlDeviceGetMemClkVfOffset(d->device, &offsetVal);
    if (result != NVML_SUCCESS) {
        LOG_ERROR("Error getting offset mem freq for Nvidia GPU", m_gpuId, "Error:", nvmlErrorString(result));
        return {};
    }
    return offsetVal;
}

FrequencyValue_t NvidiaFrequencyManager::getDefaultMemoryVoltage() const
{
    LOG_DEBUG("Not written function:", __PRETTY_FUNCTION__);
    return {};
}

void NvidiaFrequencyManager::setDisplay(std::shared_ptr<PDisplay> pDisplay)
{
    d->pDisplay = pDisplay;
}

}
}
