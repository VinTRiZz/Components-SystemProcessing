#include "harddrive.hpp"

#include <future>
#include <iomanip>
#include <map>
#include <regex>

#include <boost/algorithm/string.hpp>

#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Internal/Structures.hpp>
#include <Libraries/Processes/PackageManager.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>
#include <Libraries/Datawork/HWNodesWork.hpp>

#include "drive.hpp"

#if (__cplusplus > 201402L)
#include <filesystem>
namespace stdfs = std::filesystem;
#else
#include <experimental/filesystem>
namespace stdfs = std::experimental::filesystem;
#endif

namespace Hardware
{

struct DriveManager::DriveManagerPrivate {
    std::vector<Drive> m_drives;
    std::vector<Libraries::Internal::DriveParameters> driveParameters;

    std::vector<std::string> getAllDrives()
    {
        std::vector<std::string> harddrives;

        std::string result;
        if (!Libraries::ProcessInvoker::invoke(
                "lsblk",
                "-d -n -o type,name | grep -v \"loop\" | sort | uniq -c | awk "
                "\'{print $3}\'",
                result))
        {
            COMPLOG_WARNING("Error query harddrive names with text: ",
                        result.c_str());
        }

        std::stringstream ss(result);
        std::string line;

        while (std::getline(ss, line, '\n'))
            harddrives.push_back(line);

        return harddrives;
    }

    void addDisk(hwNode* pNode)
    {
        if (pNode->getPhysId().empty() && pNode->getLogicalName().empty()) {
            COMPLOG_WARNING("Skipped drive (maybe is a controller)");
            return;
        }

        auto tmpSerial = pNode->getSerial();
        auto foundDrive = std::find_if(driveParameters.begin(), driveParameters.end(), [tmpSerial](auto& rDiskA){
            if (!rDiskA.serial.has_value()) {
                return true;
            }
            if (rDiskA.serial->empty()) {
                return true;
            }
            return (rDiskA.serial == tmpSerial);
        });
        if (foundDrive == driveParameters.end()) {
            driveParameters.push_back({});
            foundDrive = driveParameters.end() - 1;
        }
        Libraries::Internal::DriveParameters& rDisk = *foundDrive;

        std::string tmpLogicalName;
        if (std::regex_search(rDisk.logicalName.tryGetValue(), std::regex("nvme|sd[a-z]"))) {
            tmpLogicalName = std::regex_replace(rDisk.logicalName.value(), std::regex("n[0-9].*"), "");
            if (tmpLogicalName == rDisk.logicalName) {
                tmpLogicalName = std::regex_replace(tmpLogicalName, std::regex("[0-9]$"), "");
            }
        }
        setupFromNode(pNode, &rDisk);
        if (!tmpLogicalName.empty()) { // Return logical name if it's drive actually
            rDisk.logicalName = tmpLogicalName;
        } else {
            rDisk.logicalName = pNode->getLogicalName();
        }

        rDisk.vendor = std::regex_replace(rDisk.vendor.tryGetValue(), std::regex("Advanced Micro Devices, Inc."), "AMD");
//        rDisk.vendor = std::regex_replace(rDisk.vendor.tryGetValue(), std::regex("Advanced Micro Devices, Inc."), "AMD"); // TODO: Replace for intel

        if (rDisk.busInfo->size() > 10) {
            rDisk.busInfo->erase(0, 9);
            rDisk.busInfo->erase(2, rDisk.busInfo->size());
        }

        std::regex dataRgx("\\[[^\\]]+\\]");
        std::smatch matches;
        if (std::regex_search(rDisk.vendor.tryGetValue(), matches, dataRgx)) {
            rDisk.vendor->erase(matches.position());
            boost::algorithm::trim_left(rDisk.vendor.value());
            boost::algorithm::trim_right(rDisk.vendor.value());
        }

        // Check if it's bus
        if ((rDisk.vendor == "AMD") || (rDisk.vendor == "Intel")) {
            return;
        }

        if (std::regex_search(rDisk.product.tryGetValue(), matches, dataRgx)) {
            rDisk.product->erase(matches.position());
            boost::algorithm::trim_left(rDisk.product.value());
            boost::algorithm::trim_right(rDisk.product.value());
        }

        for (int i = 0; i < pNode->countChildren(); i++)
        {
            auto pChild = pNode->getChild(i);
            rDisk.sectorSize = Libraries::safeSton<int64_t>(pChild->getConfig("sectorsize"));
            if (pChild->getClass() == hw::hwClass::disk)
            {
                for (int j = 0; j < pChild->countChildren(); j++)
                {
                    auto pDisk = pChild->getChild(j);
                    rDisk.space.total += pDisk->getSize() / 1024 / 1024;
                }
            }
        }

        rDisk.valuesCheckup();
        if (driveParameters.empty()) {
            driveParameters.push_back(rDisk);
            return;
        }
        if (rDisk != driveParameters.back()) {
            driveParameters.push_back(rDisk);
        }
    }
};

void DriveManager::init()
{
    d = std::make_shared<DriveManagerPrivate>();
    parseNodeTree();

    if (d->driveParameters.empty()) {
        setErrorText("Drives init error (not found any)");
        return;
    }

    for (auto& drive : d->driveParameters)
    {
        d->m_drives.push_back(Drive(drive));
        d->m_drives.back().init();
    }
    setInited();
    setCanWork();
}

void DriveManager::start() {}

nlohmann::json DriveManager::processInfoRequestPrivate(const std::string& uuid)
{
    nlohmann::json result, resultPart;
    for (auto& drive : d->m_drives)
    {
        result.push_back(drive.getAllParameters());
    }
    return result;
}

nlohmann::json
DriveManager::processDynamicRequestPrivate(const std::string& uuid)
{
    nlohmann::json result, resultPart;
    for (auto& drive : d->m_drives) {
        result.push_back(drive.getDynamicParameters());
    }
    return result;
}

bool DriveManager::processOverclockRequestPrivate(const nlohmann::json& payload,
                                                  const std::string& uuid)
{
    for (auto& drive : d->m_drives)
    {
        if (drive.uuid() == uuid)
        {
            //            drive.setOverclock(payload);
            COMPLOG_ERROR("Overclock for drive not set");
            return true;
        }
    }
    return false;
}

void DriveManager::dump()
{
    for (auto& drive : d->m_drives) {
        drive.dump();
    }
}

void DriveManager::parseNodeTree()
{
    auto selfDevice = Libraries::ConstantMaster::getInstance().getDmiManager().getPropertyTree();
    std::vector<hwNode*> diskNodes;
    Libraries::searchForDevices(hw::hwClass::disk, selfDevice, diskNodes);

    std::vector<hwNode*> storageNodes;
    Libraries::searchForDevices(hw::hwClass::storage, selfDevice, storageNodes);

    for (auto pNode : diskNodes) {
        d->addDisk(pNode);
    }

    for (auto pNode : storageNodes) {
        d->addDisk(pNode);
    }
}

} // namespace Hardware
