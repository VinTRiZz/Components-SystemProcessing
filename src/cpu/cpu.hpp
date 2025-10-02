#ifndef CPU_HPP
#define CPU_HPP

#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Internal/Structures.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>
#include <Libraries/Filework/FileworkUtils.hpp>

#include <fstream>
#include <future>
#include <map>
#include <nlohmann/json.hpp>
#include <regex>


namespace Hardware
{

class CPU
{
  public:
    CPU(const Libraries::Internal::CPU_Parameters& info);
    ~CPU() = default;

    void dump();

    std::string uuid() const;

    void init();
    void updateDynamic();

    nlohmann::json getAllInfo();
    nlohmann::json getDynamic();
    bool setOverclock(const nlohmann::json& overclockJson);

  private:
    int64_t currentTemperature() const noexcept;
    double powerUsage() const noexcept;
    double currentClock() const;
    int64_t getCurrentClock() const;
    void setHugepagesCount(int64_t newCount);

    Libraries::Internal::CPU_Parameters info;
};

}

#endif // CPU_HPP
