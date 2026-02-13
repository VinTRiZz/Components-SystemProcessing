#include "statusmanager.hpp"

#include <Components/Logger/Logger.h>
#include <Components/Filework/Common.h>

#include <algorithm>
#include <numeric>
#include <regex>

#include <sys/sysinfo.h>

namespace SystemProcessing {

double StatusManager::getCPUCurrentTemperature() const noexcept
{
    const std::string tempDataFile = "/sys/class/hwmon/hwmon2/temp1_input";

    std::string temperatureString;
    if (!Filework::Common::readFileData(tempDataFile, temperatureString))
    {
        COMPLOG_WARNING("Error getting CPU temperature");
        return 0;
    }

    // Data in a file represented like 46 123 what mean 46,123 C degrees
    try {
        return std::stoi(temperatureString) / 1000.0;
    } catch (std::invalid_argument& ex) {
        return 0;
    }


    // V2?
    /*
    auto tempers = exec("sensors | grep Tctl");
    auto currentPos = std::find(tempers.begin(), tempers.end(), '+') + 1;
    auto endPos = std::find(currentPos, tempers.end(), 'C') - 2;
    return std::stof(std::string(currentPos, endPos));
    */
}

double StatusManager::getCPULoad() const noexcept
{
    size_t previous_idle_time=0, previous_total_time=0;
    size_t idle_time{}, total_time{};

    double utilization = 0;
    for (int i = 0; i < 2; i++)
    {
        getCPUtimes(idle_time, total_time);
        const double idle_time_delta = idle_time - previous_idle_time;
        const double total_time_delta = total_time - previous_total_time;
        utilization = 100.0 * (1.0 - idle_time_delta / total_time_delta);
        previous_idle_time = idle_time;
        previous_total_time = total_time;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return utilization;
}

unsigned long long StatusManager::getUptimeSec() const
{
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        return 0;
    }
    return info.uptime;
}

std::vector<size_t> StatusManager::getCPUtimes() const {
    std::ifstream proc_stat("/proc/stat");
    proc_stat.ignore(5, ' '); // Skip the 'cpu' prefix.
    std::vector<size_t> times;
    for (size_t time; proc_stat >> time; times.push_back(time));
    return times;
}

bool StatusManager::getCPUtimes(size_t &idleTime, size_t &totalTime) const {
    const std::vector<size_t> cpu_times = getCPUtimes();
    if (cpu_times.size() < 4)
        return false;
    idleTime = cpu_times[3];
    totalTime = std::accumulate(cpu_times.begin(), cpu_times.end(), 0);
    return true;
}


} // namespace SystemProcessing
