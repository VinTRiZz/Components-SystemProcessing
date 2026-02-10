#include "constantmaster.hpp"

#include "../Constants/vendormap.hpp"
#include "../Datawork/numberic.hpp"
#include "../Etc/loggers.hpp"
#include "../Filework/fileworkutil.hpp"

#include <regex>
#include <algorithm>

namespace Libraries
{

struct ConstantMaster::ConstantMasterPrivate {
    SysinfoMaster dmiManager;
    std::map<int8_t, int64_t> gpuPciIdEqus;
};

ConstantMaster &ConstantMaster::getInstance()
{
    static ConstantMaster cm;
    return cm;
}

void ConstantMaster::init()
{
    updateCardPciEqus();
    d->dmiManager.updateInfo();
    LOG_INFO("Constant manager inited");
}

JOptional<std::string> ConstantMaster::getSubvendor(const std::string& hexCode) const
{
    auto vendorIt = VENDOR_MAP.find(hexCode);

    if (vendorIt == VENDOR_MAP.end()) return {};

    return vendorIt->second.data();
}

JOptional<int64_t> ConstantMaster::getGpuId(const std::string &pciId)
{
    try {
        auto pciIdNumberic = std::stoll(pciId, nullptr, 16);
        for (auto& pciIdEquPair : d->gpuPciIdEqus) {
            if (pciIdEquPair.second == pciIdNumberic) {
                return pciIdEquPair.first;
            }
        }
    } catch (std::invalid_argument& ex) {
        return {};
    }
    return {};
}

JOptional<int64_t> ConstantMaster::getPciId(int16_t gpuId) const
{
    auto pciIdIt = d->gpuPciIdEqus.find(gpuId);
    if (pciIdIt == d->gpuPciIdEqus.end())
        return {};

    return pciIdIt->second;
}

JOptional<std::string> ConstantMaster::getPciIdHex(int16_t gpuId) const
{
    auto pciIdIt = d->gpuPciIdEqus.find(gpuId);
    if (pciIdIt == d->gpuPciIdEqus.end())
        return {};

    std::stringstream ss;
    ss << std::hex << pciIdIt->second;
    return ss.str();
}

SysinfoMaster ConstantMaster::getDmiManager() const
{
    return d->dmiManager;
}

void ConstantMaster::updateCardPciEqus()
{
    const std::string drmDirPath   = "/sys/class/drm";      // Directory to search in

    // Get count of directory subdirs (cards actually)
    int16_t cardCount = FileworkUtil::dirsCount(drmDirPath);
    if (cardCount == 0) return;

    const std::regex targetMatch ("consumer:pci:0000:");

    for (int i = 0; i < cardCount; i++) {

        for (auto& contentName : FileworkUtil::getContentNames(drmDirPath + "/card" + std::to_string(i) + "/device")) {
            if (!std::regex_search(contentName, targetMatch)) {
                continue;
            }

            auto bufId = std::regex_replace(contentName, targetMatch, "");
            if (bufId.size() > 2) {
                try {
                    d->gpuPciIdEqus[i] = std::stoi(std::string(bufId.begin(), bufId.begin() + 2), nullptr, 16);
                } catch (std::invalid_argument& ex) {
                    LOG_ERROR("Error casting:", std::string(bufId.begin(), bufId.begin() + 2));
                    continue;
                }
            }
        }
    }

    LOG_EMPTY("------------------------------");
    LOG_INFO("Found card equs:");
    for (auto& pciEqu : d->gpuPciIdEqus)
    {
        LOG_EMPTY("Card:", (int)pciEqu.first, "PCI (decimal):", pciEqu.second);
    }
    LOG_EMPTY("------------------------------");
}

ConstantMaster::ConstantMaster() : d{new ConstantMasterPrivate} {}

ConstantMaster::~ConstantMaster() {}

} // namespace Libraries
