#include "hwnodeswork.hpp"

#include <Libraries/Etc/Logging.hpp>

namespace Libraries
{


void setupFromNode(hwNode *pNode, HardwareLoggableStruct *pTargetStruct) {
    pTargetStruct->nodeIdentificator = reinterpret_cast<uint64_t>(pNode);

    if (!pTargetStruct->serial.has_value()) pTargetStruct->serial       = pNode->getSerial();
    else if (pTargetStruct->serial->empty()) pTargetStruct->serial      = pNode->getSerial();

    if (!pTargetStruct->product.has_value()) pTargetStruct->product     = pNode->getProduct();
    else if (pTargetStruct->product->empty()) pTargetStruct->product    = pNode->getProduct();

    if (!pTargetStruct->vendor.has_value()) pTargetStruct->vendor   = pNode->getVendor();
    else if (pTargetStruct->vendor->empty()) pTargetStruct->vendor  = pNode->getVendor();

    if (!pTargetStruct->physId.has_value()) pTargetStruct->physId   = pNode->getPhysId();
    else if (pTargetStruct->physId->empty()) pTargetStruct->physId  = pNode->getPhysId();

    if (!pTargetStruct->slot.has_value()) pTargetStruct->slot       = pNode->getSlot();
    else if (pTargetStruct->slot->empty()) pTargetStruct->slot      = pNode->getSlot();

    if (!pTargetStruct->logicalName.has_value()) pTargetStruct->logicalName     = pNode->getLogicalName();
    else if (pTargetStruct->logicalName->empty()) pTargetStruct->logicalName    = pNode->getLogicalName();

    if (!pTargetStruct->version.has_value()) pTargetStruct->version     = pNode->getVersion();
    else if (pTargetStruct->version->empty()) pTargetStruct->version    = pNode->getVersion();

    if (!pTargetStruct->busInfo.has_value()) pTargetStruct->busInfo     = pNode->getBusInfo();
    else if (pTargetStruct->busInfo->empty()) pTargetStruct->busInfo    = pNode->getBusInfo();
}

void printHwTree(hwNode *pNode, int spaces)
{
    std::string prefix;
    for (int i = 0; i < spaces; i++)
        prefix += "-";
    prefix += "+";

    LOG_EMPTY(prefix, pNode->getClassName());
    for (int i = 0; i < pNode->countChildren(); i++)
    {
        printHwTree(pNode->getChild(i), spaces + 4);
    }
}

void printNodeInfo(hwNode *pNode) {
    LOG_WARNING(">>>>>>>>> NODE NAME:   ", pNode->getClassName());
    LOG_EMPTY("Phys id:", pNode->getPhysId());
    LOG_EMPTY("Bus info:", pNode->getBusInfo());
    LOG_EMPTY("Logical name:", pNode->getLogicalName());
    LOG_EMPTY("Vendor:", pNode->getVendor());
    LOG_EMPTY("Product:", pNode->getProduct());
    LOG_EMPTY("Serial:", pNode->getSerial());
    LOG_EMPTY("Version:", pNode->getVersion());
    LOG_EMPTY("Description:", pNode->getDescription());
    LOG_EMPTY("Subvendor:", pNode->getSubVendor());
    LOG_EMPTY("Subproduct:", pNode->getSubProduct());
    LOG_EMPTY("Dev:", pNode->getDev());
    LOG_EMPTY("Date:", pNode->getDate());
    LOG_EMPTY("Clock:", pNode->getClock());
    LOG_EMPTY("Size:", pNode->getSize());
    LOG_EMPTY("Start:", pNode->getStart());
    LOG_EMPTY("Capacity:", pNode->getCapacity());
    LOG_EMPTY("Width:", pNode->getWidth());
    LOG_EMPTY("Slot:", pNode->getSlot());
    LOG_EMPTY("Modalias:", pNode->getModalias());
    for (auto cfgKey : pNode->getConfigKeys())
    {
        LOG_EMPTY("Key:", cfgKey, " Value:", pNode->getConfig(cfgKey));
    }
    LOG_EMPTY("Capabilities:", pNode->getCapabilities());

    int i = 0;
    for (auto res : pNode->getResources(" "))
    {
        LOG_EMPTY("Resource", i, res);
    }

    i = 0;
    for (auto hint : pNode->getHints()) {
        LOG_EMPTY("HINT", i, hint);
    }
}

void printChildren(hwNode *pNode)
{
    printNodeInfo(pNode);

    if (!pNode->countChildren()) {
        return;
    }

    LOG_WARNING("------------------------ CHILDREN FOR", pNode->getClassName(), "--------------------------");
    for (int i = 0; i < pNode->countChildren(); i++)
    {
        printChildren(pNode->getChild(i));
    }
    LOG_EMPTY("--------------------------");
}

hwNode *findChild(hw::hwClass hwClassid, hwNode *searchNode)
{
    hwNode* pRes = NULL;

    if (searchNode == NULL)
        return NULL;

    if (searchNode->getClass() == hwClassid)
        return searchNode;

    for (int i = 0; i < searchNode->countChildren(); i++)
    {
        pRes = findChild(hwClassid, searchNode->getChild(i));
        if (pRes != NULL)
            return pRes;
    }
    return NULL;
}

void searchForDevices(hw::hwClass hwClassId, hwNode *searchNode, std::vector<hwNode *> &oVect)
{
    if (searchNode == NULL)
        return;

    if (searchNode->getClass() == hwClassId) {
        oVect.push_back(searchNode);
    }

    for (int i = 0; i < searchNode->countChildren(); i++) {
        searchForDevices(hwClassId, searchNode->getChild(i), oVect);
    }
}

}
