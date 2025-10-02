#include "gpumanager.hpp"

#include <Libraries/Datawork/Charstrings.hpp>
#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>
#include <Libraries/Filework/ModaliasParser.hpp>
#include <Libraries/Constants/ConstantMaster.hpp>
#include <Libraries/Datawork/HWNodesWork.hpp>
#include <Libraries/Datawork/OpenCLAdapter.hpp>

#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>
#include <X11/Xlib.h>
#include <NVML/nvml.h>

#include <algorithm>
#include <regex>
#include <fstream>

#include <boost/algorithm/string.hpp>

#include "gpucard.hpp"

namespace Hardware
{

typedef std::shared_ptr<GPU::GPUCard> rGpuCard;

struct GPUManager::GPUManagerPrivate {
    std::vector<rGpuCard> m_gpus;
    std::vector<Libraries::Internal::GPU_Parameters> gpuParameters;
    std::shared_ptr<PDisplay> pDisplay;
    bool nvidiaCanWork = false;

    static int x11ErrorHandler(Display *display, XErrorEvent *error) {
        char errorText[256];
        XGetErrorText(display, error->error_code, errorText, sizeof(errorText));
        LOG_CRITICAL("Error in X11 library functions! Text:", errorText);
        return 0;
    }

    Libraries::OpenCLAdapter openclAdapter;
    uint16_t amdCurrentId {0};
    uint16_t nvidiaCurrentId {0};
    void addGpu(hwNode* pNode)
    {
        if (pNode->getDescription() != "VGA compatible controller") {
            LOG_WARNING("May be skipped card (not a VGA compatible controller)");
            return;
        }

        Libraries::Internal::GPU_Parameters rGpu;

        setupFromNode(pNode, &rGpu);
        rGpu.vram = pNode->getWidth();
        if (!rGpu.subvendor.has_value()) rGpu.subvendor = pNode->getSubVendor();

        // Trim vendor
        std::regex dataRgx("\\[[^\\]]+\\]");
        std::smatch matches;
        if (std::regex_search(rGpu.vendor.tryGetValue(), matches, dataRgx)) {
            rGpu.vendor->erase(matches.position());
            boost::algorithm::trim_left(rGpu.vendor.value());
            boost::algorithm::trim_right(rGpu.vendor.value());
        }

        if (std::regex_search(rGpu.vendor.tryGetValue(),
                              std::regex("Nvidia", std::regex::flag_type::_S_ECMAScript | std::regex::flag_type::_S_icase))) {
            rGpu.vendor = "Nvidia";
        } else {
            rGpu.vendor = "AMD";
        }

        if (rGpu.busInfo.has_value()) {
            rGpu.pciInfoString = rGpu.busInfo.value();
            rGpu.busInfo = std::regex_replace(rGpu.busInfo.value(), std::regex("pci@0000:"), "");
            rGpu.busInfo = std::regex_replace(rGpu.busInfo.value(), std::regex(":00.0"), "");

            auto physIdOpt = Libraries::ConstantMaster::getInstance().getGpuId(rGpu.busInfo.value());
            if (physIdOpt.has_value()) {
                rGpu.physId = std::to_string(physIdOpt.value());
            }
        }

        auto isAmdCard = (rGpu.vendor.value() == "AMD");

        if (isAmdCard) {
            rGpu.actualId = amdCurrentId++;
        } else {
            rGpu.actualId = nvidiaCurrentId++;
        }

        rGpu.driverVersion = openclAdapter.cardDriverVersion(rGpu.actualId, isAmdCard);
        rGpu.infoProvider = openclAdapter.cardInfoProvider(rGpu.actualId, isAmdCard);
        rGpu.infoProviderVersion = openclAdapter.cardInfoProviderVersion(rGpu.actualId, isAmdCard);
        rGpu.product = openclAdapter.cardName(rGpu.actualId, isAmdCard);

        // Get subvendor and trim it
        if (std::regex_search(rGpu.subvendor.tryGetValue(), matches, dataRgx)) {
            rGpu.subvendor->erase(matches.position());
            boost::algorithm::trim_left(rGpu.subvendor.value());
            boost::algorithm::trim_right(rGpu.subvendor.value());
        }

        rGpu.valuesCheckup();
        if (gpuParameters.empty()) {
            gpuParameters.push_back(rGpu);
            return;
        }
        if (rGpu != gpuParameters.back()) {
            gpuParameters.push_back(rGpu);
        }
    }
};

enum GPUManagerStatus
{
    GPU_MANAGER_STATUS_ERROR,
    GPU_MANAGER_STATUS_INITED,
    GPU_MANAGER_STATUS_WORKING
};

void GPUManager::deinit()
{
    if (d->nvidiaCanWork) {
        nvmlShutdown();
    }
    setInited(false);
}

std::pair<string, string> GPUManager::getDriverVersions() const
{
    std::string amdVersion;
    std::string nvidiaVersion;
    for (auto gpu : d->m_gpus) {
        if (!amdVersion.empty() && !nvidiaVersion.empty()) {
            break;
        }

        if (gpu->getVendor() == GPU::GPU_CARD_VENDOR::GPU_CARD_VENDOR_AMD) {
            amdVersion = gpu->getDriverVersion();
        }

        if (gpu->getVendor() == GPU::GPU_CARD_VENDOR::GPU_CARD_VENDOR_NVIDIA) {
            nvidiaVersion = gpu->getDriverVersion();
        }
    }
    return std::make_pair(amdVersion, nvidiaVersion);
}

void GPUManager::dump()
{
    for (auto pGpu : d->m_gpus) {
        pGpu->dump();
    }
}

void GPUManager::parseNodeTree()
{
    auto selfDevice = Libraries::ConstantMaster::getInstance().getDmiManager().getPropertyTree();
    std::vector<hwNode*> gpuNodes;
    Libraries::searchForDevices(hw::hwClass::display, selfDevice, gpuNodes);

    for (auto pNode : gpuNodes) {
        d->addGpu(pNode);
    }
}

void GPUManager::init()
{
    d = std::make_shared<GPUManagerPrivate>();

    parseNodeTree();

    XSetErrorHandler(GPUManager::GPUManagerPrivate::x11ErrorHandler);

    // Приколы из С
    d->pDisplay = decltype(d->pDisplay)(
        static_cast<void*>(XOpenDisplay(nullptr)),
        [](PDisplay* disp) {
        if (disp == nullptr) {
            return;
        }
        XCloseDisplay(static_cast<Display*>(disp));
    });

    auto result = nvmlInit();
    d->nvidiaCanWork = (result == NVML_SUCCESS);
    if (!d->nvidiaCanWork) {
        LOG_ERROR("NVML init error text:", nvmlErrorString(result));
    }

    for (auto gpuInfo : d->gpuParameters)
    {
        if (!gpuInfo.actualId.has_value()) {
            LOG_WARNING("Error setting up GPU:");
            LOG_EMPTY(gpuInfo.busInfo, gpuInfo.vendor, gpuInfo.product);
            continue;
        }

        auto gpuVendorType = GPU::GPU_CARD_VENDOR::GPU_CARD_VENDOR_UNKNOWN;
        if (gpuInfo.vendor == "AMD") {
            gpuVendorType = GPU::GPU_CARD_VENDOR::GPU_CARD_VENDOR_AMD;
        } else if (gpuInfo.vendor == "Nvidia") {

            if (!d->nvidiaCanWork) {
                LOG_WARNING("Not inited Nvidia card:");
                LOG_EMPTY(gpuInfo.busInfo, gpuInfo.vendor, gpuInfo.product);
                continue;
            }
            gpuVendorType = GPU::GPU_CARD_VENDOR::GPU_CARD_VENDOR_NVIDIA;
        }

        auto pCard = GPU::GPUCard::createGPU(
                    gpuVendorType,
                    gpuInfo.actualId,
                    d->pDisplay
        );

        if (pCard.use_count()) {
            pCard->setGpuCardParameters(gpuInfo);
            pCard->init();
            d->m_gpus.push_back(pCard);
        }
    }
    setCanWork();
    setInited();
}

void GPUManager::start() {}

nlohmann::json GPUManager::processInfoRequestPrivate(const std::string& uuid)
{
    nlohmann::json result;

    for (auto gpu : d->m_gpus) {
        if (!gpu.use_count()) {
            LOG_ERROR("Invalid use count of rGpu!");
            continue;
        }
        result.push_back(gpu->getFullInformation());
    }

    return result;
}

nlohmann::json GPUManager::processDynamicRequestPrivate(const std::string& uuid)
{
    for (auto gpu : d->m_gpus)
        gpu->updateDynamic();

    nlohmann::json result;
    for (auto gpu : d->m_gpus)
    {
        gpu->updateDynamic();
        result.push_back(gpu->getDynamic());
    }
    return result;
}

bool GPUManager::processOverclockRequestPrivate(const nlohmann::json& payload,
                                                const std::string& uuid)
{
    nlohmann::json subJson;

    for (auto gpu : d->m_gpus)
    {
        if (gpu->uuid() != uuid) continue;

        Libraries::Internal::OverclockParameters newOverdriveParams;

        newOverdriveParams.powerLimit        = payload["power"];
        newOverdriveParams.fanSpeed          = payload["fan"];

        subJson                              = payload["clock"];
        newOverdriveParams.coreClockOffset   = subJson["core"];
        newOverdriveParams.memoryClockOffset = subJson["memory"];

        subJson                              = payload["voltage"];
        newOverdriveParams.coreVoltage       = subJson["core"];
        newOverdriveParams.memVoltage        = subJson["memory"];

        gpu->setOverclock(newOverdriveParams);

        return true;
    }
    return false;
}

} // namespace Hardware
