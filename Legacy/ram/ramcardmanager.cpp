#include "ramcardmanager.hpp"

#include "ram.hpp"

#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Internal/Structures.hpp>
#include <Libraries/Processes/PackageManager.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>
#include <Libraries/Datawork/HWNodesWork.hpp>

#include <map>
#include <regex>
#include <string.h>

#include <unistd.h>

#include <boost/tokenizer.hpp>

namespace Hardware
{

struct RAMCardManager::Impl
{
    std::vector<RAMCard> ramCards;
    std::vector<Libraries::Internal::RAM_CardInfo> ramParameters;

    void addRam(hwNode* pNode)
    {
        Libraries::Internal::RAM_CardInfo ramCardParameters;

        Libraries::setupFromNode(pNode, &ramCardParameters);

        ramCardParameters.memorySpace.total = pNode->getSize() / 1024 / 1024;   // It gather in bytes
        ramCardParameters.configuredSpeed = pNode->getClock() / 1e6;            // It gather in Hz
        ramCardParameters.speed           = ramCardParameters.configuredSpeed;
        ramCardParameters.width           = pNode->getWidth();
        ramCardParameters.type            = pNode->getDescription();
        if (ramCardParameters.busInfo->size() > 10) {
            ramCardParameters.busInfo->erase(0, 9);
            ramCardParameters.busInfo->erase(2, ramCardParameters.busInfo->size());
        }

        ramCardParameters.valuesCheckup();
        if (ramParameters.empty()) {
            ramParameters.push_back(ramCardParameters);
            return;
        }
        if (ramCardParameters != ramParameters.back()) {
            ramParameters.push_back(ramCardParameters);
        }
    }
};

std::vector<std::string> createRamHandlerList()
{
    if (!Libraries::ProcessInvoker::isSuperuser())
    {
        LOG_WARNING("Not a root user");
        return {};
    }

    std::vector<std::string> ramHandlers;

    std::string dmidecodeOutput;
    if (!Libraries::ProcessInvoker::invoke("dmidecode", " -t memory",
                                           dmidecodeOutput))
    {
        LOG_WARNING("Error calling dmidecode with text: ",
                    dmidecodeOutput.c_str());
        return ramHandlers;
    }

    const auto handleStringSize = strlen("Handle 0x00FF");
    std::string strToFind       = "Handle 0x00";
    std::string::iterator dataEndPos;

    for (std::string::iterator currentPos = dmidecodeOutput.begin();
         currentPos != dmidecodeOutput.end(); currentPos++)
    {
        // Searh for card ID
        strToFind  = "Handle 0x00";
        currentPos = std::search(currentPos, dmidecodeOutput.end(),
                                 strToFind.begin(), strToFind.end());
        if (currentPos == dmidecodeOutput.end()) break;

        // Search for the end of data to avoid gathering incorrect info
        dataEndPos =
            std::search(currentPos + strToFind.size(), dmidecodeOutput.end(),
                        strToFind.begin(), strToFind.end());

        // Find if it's memory device
        strToFind = "Memory Device";
        if (std::search(currentPos, dataEndPos, strToFind.begin(),
                        strToFind.end()) == dataEndPos)
            continue;

        if (currentPos == dmidecodeOutput.end()) continue;

        ramHandlers.push_back(
            std::string(currentPos, currentPos + handleStringSize));
    }

    return ramHandlers;
}

void RAMCardManager::parseNodeTree()
{
    auto selfDevice = Libraries::ConstantMaster::getInstance().getDmiManager().getPropertyTree();
    std::vector<hwNode*> processorNodes;
    Libraries::searchForDevices(hw::hwClass::memory, selfDevice, processorNodes);

    for (auto pNode : processorNodes) {

        if (pNode->getWidth() == 0) {
            continue;
        }

        if (pNode->getDescription() == "[empty]") {
            continue;
        }

        d->addRam(pNode);
    }
}

void RAMCardManager::dump()
{
    for (auto& ram : d->ramCards) {
        ram.dump();
    }
}

void RAMCardManager::updateDynamic()
{
    for (auto& ram : d->ramCards)
        ram.updateDynamic();
}

void RAMCardManager::init()
{
    d = decltype(d)(new Impl);

    parseNodeTree();
    d->ramCards.reserve(d->ramParameters.size());
    for (auto& ram : d->ramParameters) {
        RAMCard card(ram);
        card.init();
        d->ramCards.push_back(card);
    }

    if (d->ramCards.empty()) {
        setErrorText("RAM init error (not found any)");
        return;
    }
    setCanWork();
    setInited();
}

void RAMCardManager::start() {}

nlohmann::json
RAMCardManager::processInfoRequestPrivate(const std::string& uuid)
{
    nlohmann::json result;
    for (auto ramCard : d->ramCards)
    {
        nlohmann::json allJson;

        allJson["uuid"]  = ramCard.uuid();
        allJson["total"] = ramCard.total();

        result.push_back(allJson);
    }
    return result;
}

nlohmann::json
RAMCardManager::processDynamicRequestPrivate(const std::string& uuid)
{
    return {};
}

bool RAMCardManager::processOverclockRequestPrivate(
    const nlohmann::json& payload, const std::string& uuid)
{
    return false;
}

}
