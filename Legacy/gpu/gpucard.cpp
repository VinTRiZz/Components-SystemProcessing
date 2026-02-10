#include "gpucard.hpp"

#include <Libraries/Datawork/Charstrings.hpp>
#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Filework/FileworkUtils.hpp>
#include <Libraries/Etc/Logging.hpp>

#include "amdsettingsworker.hpp"
#include "nvidiasettingsworker.hpp"
#include "amdfrequencymanager.h"
#include "nvidiafrequencymanager.h"

#include <NVML/nvml.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>
#include <X11/Xlib.h>

#include <thread>

#if (NVIDIA_MUST_BUILD == 1)
#include <cuda_runtime.h>
#endif // NVIDIA_MUST_BUILD


namespace Hardware {
namespace GPU
{

struct GPUCard::GPUCardPrivate
{
    Libraries::Internal::GPU_Parameters parameters;

    std::shared_ptr<CardSettingsWorker>     settingsWorker;
    std::shared_ptr<AbstractFrequencyManager>       freqManager;
};

nlohmann::json GPUCard::getFullInformation() const
{
    nlohmann::json result = {};
    nlohmann::json power, fan, temper, pci, voltage, vcore, vmem, clock, ccore,
        cmem, info, infodriv, infotech, infomem;

    result["id"]  = d->parameters.guid;

        pci["id"]     = Libraries::safeSton<int64_t, 16>(d->parameters.busInfo.tryGetValue());
        pci["bus"]    = d->parameters.pciInfoString;
    result["pci"] = pci;

        fan["rangeValue"] = d->parameters.fan.info(true);
        fan["count"]    = 1;
    result["fan"]   = fan;

    result["power"] = d->parameters.powerLimit(true);

        temper["core"] =
            d->parameters.temperature(true);
        temper["memory"] =
            d->parameters.temperature(true);
    result["temperature"] = temper;

            ccore["lock"] = d->parameters.coreClock.info(true);
            ccore["offset"] =
                d->parameters.coreClock.offset((m_vendor != GPU_CARD_VENDOR::GPU_CARD_VENDOR_AMD));
        clock["core"] = ccore;

            cmem["lock"]  = d->parameters.memoryClock.info(true);
            cmem["offset"] =
                d->parameters.memoryClock.offset((m_vendor != GPU_CARD_VENDOR::GPU_CARD_VENDOR_AMD));
        clock["memory"] = cmem;
    result["clock"] = clock;

        vcore["lock"] = d->parameters.coreVoltage.info(true);
        vcore["offset"] =
            d->parameters.coreVoltage.offset((m_vendor != GPU_CARD_VENDOR::GPU_CARD_VENDOR_AMD));
    voltage["core"] = vcore;

        vmem["lock"]    = d->parameters.memVoltage.info(true);
        vmem["offset"] =
            d->parameters.memVoltage.offset((m_vendor != GPU_CARD_VENDOR::GPU_CARD_VENDOR_AMD));
    voltage["memory"]     = vmem;
    result["voltage"]     = voltage;

    info["manufacturer"]  = d->parameters.vendor;
    info["periphery"]     = d->parameters.product;
    info["serialNumber"]  = d->parameters.serial;
    info["vendor"]        = d->parameters.subvendor;
    info["vbios"]         = {};

        infodriv["version"]   = d->parameters.driverVersion;
        infodriv["provider"]  = d->parameters.vendor;
    info["driver"]        = infodriv;

        infotech["version"]   = d->parameters.infoProviderVersion;
        infotech["provider"]  = d->parameters.infoProvider;
    info["technology"]    = infotech;

        infomem["total"]      = d->parameters.vram;
        infomem["type"]       = {};
        infomem["vendor"]     = {};
    info["memory"]        = infomem;
    result["information"] = info;

    return result;
}

void GPUCard::setupCard(const std::shared_ptr<CardSettingsWorker> &pCardSettings, const std::shared_ptr<AbstractFrequencyManager> &pFreqManager)
{
    d->settingsWorker  = pCardSettings;
    d->freqManager     = pFreqManager;
}

int64_t GPUCard::getTemperature() const
{
    return d->settingsWorker->getTempCurrent();
}

std::string GPUCard::uuid() const
{
    return d->parameters.guid;
}

std::shared_ptr<GPUCard> GPUCard::createGPU(GPU_CARD_VENDOR gcv, int64_t gpuId, std::shared_ptr<PDisplay> pDisplay)
{
    if (gcv == GPU_CARD_VENDOR::GPU_CARD_VENDOR_UNKNOWN) {
        LOG_CRITICAL("Invalid GPU vendor (vendor HM-internal code:", (int)gcv, ")");
        return {};
    }

    std::shared_ptr<GPUCard> result = std::make_shared<GPUCard>(gpuId, gcv);

    std::shared_ptr<CardSettingsWorker> settingsWorker;
    std::shared_ptr<AbstractFrequencyManager> cardInfoManager;

    auto constantMaster = Libraries::ConstantMaster::getInstance();
    switch (gcv)
    {
    case GPU_CARD_VENDOR::GPU_CARD_VENDOR_AMD:
        settingsWorker  = std::dynamic_pointer_cast<CardSettingsWorker>(std::make_shared<AMDSettingsWorker>());
        cardInfoManager = std::dynamic_pointer_cast<AbstractFrequencyManager>(std::make_shared<AMDFrequencyManager>(gpuId));
        break;

    case GPU_CARD_VENDOR::GPU_CARD_VENDOR_NVIDIA:
        settingsWorker  = std::dynamic_pointer_cast<CardSettingsWorker>(std::make_shared<NvidiaSettingsWorker>());
        std::dynamic_pointer_cast<NvidiaSettingsWorker>(settingsWorker)->setDisplay(pDisplay);

        cardInfoManager = std::dynamic_pointer_cast<AbstractFrequencyManager>(std::make_shared<NvidiaFrequencyManager>(gpuId));
        std::dynamic_pointer_cast<NvidiaFrequencyManager>(cardInfoManager)->setDisplay(pDisplay);
        break;

    case GPU_CARD_VENDOR::GPU_CARD_VENDOR_UNKNOWN:
        break;
    }

    if (!settingsWorker.use_count() || !cardInfoManager.use_count()) {
        return {};
    }

    settingsWorker->init(gpuId);
    result->setupCard(settingsWorker, cardInfoManager);
    return result;
}

GPUCard::GPUCard(const int64_t gpuId, GPU_CARD_VENDOR vendor) :
    m_vendor{vendor},
    d {new GPUCardPrivate()}
{

}

GPUCard::~GPUCard() {}

void GPUCard::dump()
{
    d->parameters.dump();
}

GPU_CARD_VENDOR GPUCard::getVendor() const
{
    return m_vendor;
}

std::string GPUCard::getDriverVersion() const
{
    return d->parameters.driverVersion;
}

void GPUCard::setGpuCardParameters(const Libraries::Internal::GPU_Parameters &rParameters)
{
    d->parameters = rParameters;

    if (m_vendor == GPU_CARD_VENDOR::GPU_CARD_VENDOR_NVIDIA) {
        d->parameters.infoProvider = "CUDA";
        d->parameters.infoProviderVersion = std::dynamic_pointer_cast<NvidiaSettingsWorker>(d->settingsWorker)->getCudaVersion();

#if (NVIDIA_MUST_BUILD == 1)
        int deviceCount;
        cudaGetDeviceCount(&deviceCount);

        if (deviceCount == 0) {
            LOG_MESSAGE("No CUDA devices found to init Nvidia GPU card");
            return;
        }

        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, d->parameters.actualId.tryGetValue());
        d->parameters.vram = prop.totalGlobalMem / 1024 / 1024;
#endif // NVIDIA_MUST_BUILD
    }
}

void GPUCard::updateDynamic()
{
    d->parameters.coreClock.info.current   = d->freqManager->getCurrentCoreFreq();
    d->parameters.coreVoltage.info.current   = d->freqManager->getCurrentCoreVoltage();

    d->parameters.memoryClock.info.current = d->freqManager->getCurrentMemoryFreq();
    d->parameters.memVoltage.info.current   = d->freqManager->getCurrentMemVoltage();

    d->parameters.temperature.maxVal       = d->settingsWorker->getTempMax();
    d->parameters.temperature.current      = d->settingsWorker->getTempCurrent();
    d->parameters.fan.info.current         = d->settingsWorker->getFanCurrent();

    d->parameters.powerLimit.current = d->settingsWorker->getPowerCurrent().tryGetValue();
}


void GPUCard::setOverclock(const Libraries::Internal::OverclockParameters &paramStruct)
{
    d->settingsWorker->setFanMode(GPUFanOperatingMode::manualState);
    d->settingsWorker->setFan(paramStruct.fanSpeed.tryGetValue());
    d->parameters.fan.info.defaultVal =
        d->settingsWorker->getFanCurrent(); // Update data after set

    d->freqManager->setCoreLock(paramStruct.coreClockLock.tryGetValue());
    d->freqManager->setCoreVoltage(paramStruct.coreVoltage.tryGetValue());

    d->freqManager->setMemoryLock(paramStruct.memoryClockLock.tryGetValue());
    d->freqManager->setMemoryVoltage(paramStruct.memVoltage.tryGetValue());

    if (m_vendor == GPU_CARD_VENDOR::GPU_CARD_VENDOR_NVIDIA) {

        auto pNvidiaFreqManager = std::dynamic_pointer_cast<NvidiaFrequencyManager>(d->freqManager);

        pNvidiaFreqManager->setCoreOffset(paramStruct.coreClockOffset.tryGetValue());
        pNvidiaFreqManager->setMemoryOffset(paramStruct.memoryClockOffset.tryGetValue());
    }

    d->settingsWorker->setPowerLimit(paramStruct.powerLimit.tryGetValue());
}

bool GPUCard::isConnected() const
{
    bool isConnected = false;

    const std::string modaliasFilePath = std::string("/sys/class/drm/card") +
                                         d->parameters.physId.tryGetValue() +
                                         "/device/modalias";
    isConnected = stdfs::exists(modaliasFilePath);
    return isConnected;
}

nlohmann::json GPUCard::getDynamic()
{
    nlohmann::json result;

    result["id"]          = uuid();
    result["power"]       = d->settingsWorker->getPowerCurrent();
    result["temperature"] = d->settingsWorker->getTempCurrent();
    result["fan"]         = d->settingsWorker->getFanCurrent();

    result["clock"]       = {};
        result["clock"]["core"]          = d->freqManager->getCurrentCoreFreq();
        result["clock"]["memory"]        = d->freqManager->getCurrentMemoryFreq();

    result["voltage"]     = {};
        result["voltage"]["core"]          = d->freqManager->getCurrentCoreVoltage();
        result["voltage"]["memory"]        = d->freqManager->getCurrentMemVoltage();

    return result;
}

void GPUCard::init()
{
    d->parameters.powerLimit.minVal = d->settingsWorker->getPowerMin().tryGetValue();
    d->parameters.powerLimit.maxVal = d->settingsWorker->getPowerMax().tryGetValue();
    d->parameters.powerLimit.defaultVal =
        d->settingsWorker->getPowerLimitDefault().tryGetValue();

    CardOverdriveLimits limits = d->freqManager->getLimits();

    d->parameters.memoryClock.info.minVal  = limits.m_minMemFreq;
    d->parameters.memoryClock.info.maxVal  = limits.m_maxMemFreq;
    d->parameters.memoryClock.info.defaultVal = d->parameters.memoryClock.info.minVal;

    d->parameters.coreClock.info.minVal    = limits.m_minCoreFreq;
    d->parameters.coreClock.info.maxVal    = limits.m_maxCoreFreq;
    d->parameters.coreClock.info.defaultVal = d->parameters.coreClock.info.minVal;

    d->parameters.coreVoltage.info.minVal  = limits.m_minCoreVoltage;
    d->parameters.coreVoltage.info.maxVal  = limits.m_maxCoreVoltage;
    d->parameters.coreVoltage.info.defaultVal = d->parameters.coreVoltage.info.minVal;

    // Bad values, wha to do?
    d->parameters.memVoltage.info.minVal   = limits.m_minMemVoltage;
    d->parameters.memVoltage.info.maxVal   = limits.m_maxMemVoltage;
    d->parameters.memVoltage.info.defaultVal = d->parameters.memVoltage.info.minVal;

    d->parameters.fan.info.minVal          = 0;
    d->parameters.fan.info.maxVal          = 100;

    d->parameters.temperature.minVal = 0;
    d->parameters.temperature.maxVal = d->settingsWorker->getTempMax().tryGetValue();
    d->parameters.temperature.defaultVal = d->parameters.temperature.maxVal;

    d->parameters.coreClock.offset.defaultVal      = d->freqManager->getDefaultCoreClock();
    d->parameters.coreClock.offset.minAvailable    = 0;
    d->parameters.coreClock.offset.maxAvailable    = 0;

    d->parameters.memoryClock.offset.defaultVal    = d->freqManager->getDefaultMemoryClock();
    d->parameters.memoryClock.offset.minAvailable  = 0;
    d->parameters.memoryClock.offset.maxAvailable  = 0;

    d->parameters.coreVoltage.offset.defaultVal    = d->freqManager->getDefaultCoreVoltage();
    d->parameters.coreVoltage.offset.minAvailable  = 0;
    d->parameters.coreVoltage.offset.maxAvailable  = 0;

    d->parameters.memVoltage.offset.defaultVal     = d->freqManager->getDefaultMemoryVoltage();
    d->parameters.memVoltage.offset.minAvailable   = 0;
    d->parameters.memVoltage.offset.maxAvailable   = 0;

    if (m_vendor == GPU_CARD_VENDOR::GPU_CARD_VENDOR_NVIDIA) {
        auto fanLimits = std::dynamic_pointer_cast<NvidiaSettingsWorker>(d->settingsWorker)->getFanLimits();
        d->parameters.fan.info.minVal = fanLimits.first;
        d->parameters.fan.info.maxVal = fanLimits.second;
    }
    d->parameters.fan.info.defaultVal = d->parameters.fan.info.minVal;

    updateDynamic();

    auto staticInfoJson = getFullInformation();
    d->parameters.guid   = Libraries::generateGuid(staticInfoJson.dump());
}

}
}
