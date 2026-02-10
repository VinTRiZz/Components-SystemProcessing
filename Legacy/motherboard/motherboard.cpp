#include "motherboard.hpp"

#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>
#include <Libraries/Internal/Structures.hpp>
#include <Libraries/Constants/ConstantMaster.hpp>
#include <Libraries/Datawork/HWNodesWork.hpp>

#include <nlohmann/json.hpp>

#include <regex>

#include "pciobjectmanager.hpp"
#include "usbobjectmanager.hpp"

namespace Hardware
{
struct Motherboard::MotherboardPrivate {
    Libraries::Internal::MotherboardParameters params;
    PCIObjectManager pciManager;
    USBObjectManager usbManager;

    void addMotherboard(hwNode* pNode)
    {
        setupFromNode(pNode, &params);

        params.bootType = pNode->getConfig("boot");
        params.family   = pNode->getConfig("family");
        if (params.busInfo->size() > 5) {
            params.busInfo = params.busInfo->substr(9, 11);
        }

        std::regex dataRgx("\\(Default string\\)");
        std::smatch matches;
        if (std::regex_search(params.product.tryGetValue(), matches, dataRgx)) {
            params.product->erase(matches.position());
        }
    }
};

void Motherboard::init()
{
    d = std::make_shared<MotherboardPrivate>();
    parseNodeTree();
    setupPCISlots();
    setupUSBSlots();

    d->params.guid         = Libraries::generateGuid(
        std::string(d->params.serial + d->params.vendor +
                            d->params.product));
    setCanWork();
    setInited();
}

void Motherboard::start() {}

nlohmann::json Motherboard::processInfoRequestPrivate(const std::string& uuid)
{
    nlohmann::json
        allJson,
            allInfo,
            portCount,
            pciInfo
    ;

    auto pciBusCount = d->pciManager.getPciBusCount();

        allInfo["manufacturer"] = d->params.vendor;
        allInfo["periphery"] = d->params.product;
        allInfo["serialNumber"] = d->params.serial;
            portCount["sata"] = d->params.sataCount;
            portCount["pciX16"] = std::get<3>(pciBusCount);
            portCount["pciX4"] = std::get<1>(pciBusCount);
            portCount["ram"] = d->params.ramCount;
        allInfo["portsCount"] = portCount;
    allJson["information"] = allInfo;

    allJson["pcies"] = nlohmann::json::array();
    auto pciDevs = d->pciManager.objects();
    for (auto& dev : pciDevs)
    {
        pciInfo["id"] = 0; // TODO: Ask Daniil for this thing
        pciInfo["bus"] = dev.getPciNumber();
        pciInfo["isInstalled"] = !dev.isFree();

        allJson["pcies"].push_back(pciInfo);
    }

    allJson["id"] = d->params.guid;
    return allJson;
}

nlohmann::json
Motherboard::processDynamicRequestPrivate(const std::string& uuid)
{
    return {};
}

bool Motherboard::processOverclockRequestPrivate(const nlohmann::json& payload,
                                                 const std::string& uuid)
{
    return false;
}

void Motherboard::dump()
{
    d->params.dump();
}

void Motherboard::parseNodeTree()
{
    auto selfDevice = Libraries::ConstantMaster::getInstance().getDmiManager().getPropertyTree();
    std::vector<hwNode*> motherboardNodes;
    Libraries::searchForDevices(hw::hwClass::system, selfDevice, motherboardNodes);

    for (auto pNode : motherboardNodes) {
        d->addMotherboard(pNode);
    }

    std::vector<hwNode*> biosNodes;
    Libraries::searchForDevices(hw::hwClass::memory, selfDevice, motherboardNodes);

    for (auto pNode : motherboardNodes) {
        if (pNode->getDescription() != "BIOS") {
            continue;
        }
        d->params.biosVersion = pNode->getVersion();
        break;
    }
}

void Motherboard::setupPCISlots()
{
    d->pciManager.updateObjectList();
}

void Motherboard::setupUSBSlots()
{
    d->usbManager.updateObjects();
}

} // namespace Hardware
