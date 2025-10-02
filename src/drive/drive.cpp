#include "drive.hpp"

#include <future>
#include <iomanip>
#include <map>

#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Internal/Structures.hpp>
#include <Libraries/Processes/PackageManager.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>

namespace Hardware
{

Drive::Drive(const Libraries::Internal::DriveParameters& info) :
    m_info { info }
{

}

void Drive::dump()
{
    m_info.dump();
}

int64_t Drive::free(std::string path) const
{
//    return stdfs::space(path).free;
    return m_info.space.free.tryGetValue();
}

int64_t Drive::available(std::string path) const
{
//    return stdfs::space(path).available;
    return m_info.space.free.tryGetValue();
}

int64_t Drive::capacity(std::string path) const
{
    return m_info.space.total.tryGetValue();
//    return stdfs::space(path).capacity;
}

int64_t Drive::temperature() const
{
    if (!Libraries::PackageManager::isInstalled("smartctl"))
    {
        LOG_WARNING("Not installed: smartctl");
        return 0;
    }

    int64_t result;

    std::string harddrivePath = "/dev/" + m_info.logicalName.tryGetValue();

    // Returned value boost::optional<std::string>
    std::string output;
    if (!Libraries::ProcessInvoker::invoke(
            "smartctl",
            std::string(" -a /dev/") + m_info.logicalName.tryGetValue() +
                " | awk '/Temperature:/{gsub(\".*:\",\"\",$0); print $0}' | "
                "awk "
                "'{$1=$1};1' | xargs",
            output))
    {
        LOG_WARNING("Error getting temperature of hard drive: %s",
                    m_info.logicalName.tryGetValue());
    }

    if (output.size() > 3)
        output = output.substr(0, 3);
    result = Libraries::safeSton<int64_t>(output);

    return result;
}

void Drive::init()
{
    updateDynamic();
    m_info.guid = Libraries::generateGuid(m_info.product + m_info.serial);
}

void Drive::updateDynamic()
{
    m_info.temperature.current = this->temperature();
    m_info.space.free          = this->free() / 1024 / 1024;
    m_info.space.used          = (this->capacity() / 1024 / 1024 - m_info.space.free.tryGetValue());
}

std::string Drive::uuid() const
{
    return m_info.guid.tryGetValue();
}

nlohmann::json Drive::getAllParameters()
{
    nlohmann::json allJson, allInfo;

    allJson["id"]           = m_info.guid;
    allInfo["manifacturer"] = m_info.product;
    allInfo["periphery"]    = "SATA";             // TODO: Write correct
    allInfo["serialNumber"] = m_info.serial;
    allInfo["capacity"]     = m_info.space.total;
    allJson["information"]  = allInfo;
    return allJson;
}

nlohmann::json Drive::getDynamicParameters()
{
    updateDynamic();

    nlohmann::json allInfo;
    allInfo["id"]          = m_info.guid;
    allInfo["power"] = 0; // TODO: Write correct
    allInfo["temperature"] = temperature();
    allInfo["available"] = this->free();
    allInfo["used"] = this->capacity() - this->free();
    return allInfo;
}

}

