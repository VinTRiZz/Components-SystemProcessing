#include "cpu.hpp"


namespace Hardware
{

CPU::CPU(const Libraries::Internal::CPU_Parameters& info) : info {info} {}

void CPU::dump()
{
    info.dump();
}

std::string Hardware::CPU::uuid() const
{
    return info.guid.value();
}

int64_t CPU::currentTemperature() const noexcept
{
    const std::string tempDataFile = "/sys/class/hwmon/hwmon2/temp1_input";

    std::string temperatureString;
    if (!Libraries::FileworkUtil::readFileData(tempDataFile, temperatureString))
    {
        COMPLOG_WARNING("Error getting CPU temperature");
        return 0;
    }

    // Data in a file represented like 46 123 what mean 46,123 C degrees
    return Libraries::safeSton<int64_t>(temperatureString).tryGetValue() / 1000;
}

double CPU::powerUsage() const noexcept
{
    double result = 0.0;

    // - "timeout 1.1 turbostat --Summary --quiet --show PkgWatt --interval 1 |
    // gawk 'NR>1 and $1>0 { print substr($1, 1, 4); fflush(); }'" std::string
    // output; if (ProcessWorking::ProcessInvoker::invoke("turbostat",
    // "--Summary
    // --quiet --show PkgWatt --interval 1 | grep -o -E
    // '\\b[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?\\b'", output, 1500))
    // {
    //     COMPLOG_WARNING("Error getting CPU power usage with text: %s",
    //     output.c_str());
    // }

    // try {
    //     result = std::stod(output);
    // } catch (const std::invalid_argument &exc) {
    //     // TODO: Handle error
    // }

    return result;
}

double Hardware::CPU::currentClock() const
{
    double result = 0.0;

    std::string output;

    Libraries::StringList paramList;
    paramList << "-F:"
              << "'/cpu MHz/ {sum+=$2; count++} END {print sum/count}'"
              << "/proc/cpuinfo";

    if (!Libraries::ProcessInvoker::invoke("awk", paramList, output, 1000))
    {
        COMPLOG_WARNING("Error getting CPU clock current with text:",
                    output.c_str());
        return result;
    }

    try
    {
        result = Libraries::safeSton<double>(output);
    } catch (const std::invalid_argument& exc)
    {
        // Do nothing, result will be zero on error
    }

    return result;
}

int64_t Hardware::CPU::getCurrentClock() const
{
    std::string catProcInfoRes;

    Libraries::ProcessInvoker::invoke(
                "cat",
                "/proc/cpuinfo | grep '^cpu MHz.*$' | grep -o "
                "'[0-9]\\{0,4\\}\\.[0-9]\\{0,4\\}'",
                catProcInfoRes);
    if (!catProcInfoRes.size()) return 0;

    double averageFreq = 0; // TODO: Do rightly
    int cpuCoreCount   = 0;

    std::string currentFreqLine;
    std::string::iterator posBegin = catProcInfoRes.begin(),
            posEnd   = std::find(catProcInfoRes.begin(),
                                 catProcInfoRes.end(), '\n');
    while (posBegin < catProcInfoRes.end())
    {
        averageFreq +=
                Libraries::safeSton<double>(std::string(posBegin, posEnd)).tryGetValue();
        cpuCoreCount++;

        posBegin = posEnd + 1,
                posEnd   = std::find(posBegin, catProcInfoRes.end(), '\n');
    }

    if (cpuCoreCount == 0) return 0;

    return averageFreq / double(cpuCoreCount);
}

void Hardware::CPU::setHugepagesCount(int64_t newCount)
{
    Libraries::ProcessInvoker::invoke(
                "echo", "always > /sys/kernel/mm/transparentHugepage/enabled");

    Libraries::ProcessInvoker::invoke(
                "echo", std::to_string(newCount) + " > /proc/sys/vm/nrHugepages");
}

void Hardware::CPU::init()
{
    info.clock.defaultVal = info.clock.minVal;

    info.fan.minAvailable = 0;
    info.fan.maxAvailable = 100;
    info.fan.defaultVal = 0;

    info.temperature.minVal = 0;

    info.physId = "0";
    info.pciIdHex = "pci@0000:00:00.0";

    info.power.maxVal = 0;
    info.power.minVal = 0;
    info.power.defaultVal = 0;

    updateDynamic();
    info.guid = Libraries::generateGuid(
                info.product + info.vendor + info.serial +
                info.architecture.tryGetValue()
                );
}

void Hardware::CPU::updateDynamic()
{
    info.temperature.current = currentTemperature();
    info.power.current       = powerUsage();
    info.clock.current       = currentClock();
}

nlohmann::json Hardware::CPU::getAllInfo()
{
    nlohmann::json allJson, pciInfo, fanInfo, information, cache;

    allJson["id"] = info.guid;

    // No way to get it!
    pciInfo["id"]         = Libraries::safeSton<int64_t>(info.physId);
    pciInfo["bus"]        = info.pciIdHex;
    allJson["pci"]        = pciInfo;

    allJson["power"]      = info.power(true);

    fanInfo["rangeValue"] = info.fan(true);
    fanInfo["rangeValue"]["rangeValue"] = 0;
    fanInfo["count"]                    = 0;
    allJson["fan"]                      = fanInfo;

    allJson["temperature"] = info.temperature(true);

    allJson["clock"] = info.clock(true);

    information["manufacturer"] = info.vendor;
    information["periphery"]    = info.product;
    information["cores"]        = info.coreCount;
    information["threads"]      = info.threadPerCore;
    information["architecture"] = info.opmode;
    cache["l1"]                 = info.cacheSize.l1;
    cache["l2"]                 = info.cacheSize.l2;
    cache["l3"]                 = info.cacheSize.l3;
    information["cache"]        = cache;
    allJson["information"]      = information;

    return allJson;
}

nlohmann::json Hardware::CPU::getDynamic()
{
    nlohmann::json result;
    result["id"]          = info.guid;
    result["power"]       = powerUsage();
    result["clock"]       = currentClock();
    result["temperature"] = currentTemperature();
    result["fan"]         = 0;
    return result;
}

bool Hardware::CPU::setOverclock(const nlohmann::json &overclockJson)
{
    try
    {
        int64_t newHugePagesCount = overclockJson["hugePages"];
        setHugepagesCount(newHugePagesCount);
        return true;
    } catch (nlohmann::json::exception& ex)
    {
        COMPLOG_ERROR("CPU overclock set failed: not found parameters");
        return false;
    }
}



}
