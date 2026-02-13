#include "ram.hpp"

#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Internal/Structures.hpp>
#include <Libraries/Processes/PackageManager.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>

#include <map>
#include <regex>
#include <string.h>

#include <unistd.h>

#include <boost/tokenizer.hpp>

namespace Hardware
{

struct RAMCard::RAMCardPrivate {
    Libraries::Internal::RAM_CardInfo m_cardInfo;
};

RAMCard::RAMCard(const Libraries::Internal::RAM_CardInfo &cardInfo) : d {new RAMCardPrivate}
{
    d->m_cardInfo = cardInfo;
}

void RAMCard::dump()
{
    d->m_cardInfo.dump();
}

int64_t RAMCard::free() const
{
    // TODO: Get understand how the hell "free" gets used mem
    //    if(sysinfo(&d->m_cardInfo.memInfo) == 0)
    //        return d->m_cardInfo.memInfo.freeram;
    //    else
    //        return 0;

    std::string output;

    if (!Libraries::ProcessInvoker::invoke("free", Libraries::StringList("--mega"), output)) {
        COMPLOG_WARNING("Error getting free RAM");
        return 0;
    }

    boost::tokenizer ramTokenizer(output);
    int curPos = 0;
    for (auto& tkn : ramTokenizer) {
        if (curPos < 3) {
            curPos++;
            continue;
        }
        auto result = Libraries::safeSton<int64_t>(tkn).tryGetValue();
        return result;
    }
    return {};
}

int64_t RAMCard::total() const
{
    if (sysinfo(&d->m_cardInfo.memInfo) == 0)
        return d->m_cardInfo.memInfo.totalram / 1e6; // Not in bytes
    else
        return 0;
}

int64_t RAMCard::usage() const
{
    if (sysinfo(&d->m_cardInfo.memInfo) == 0)
        return (d->m_cardInfo.memInfo.totalram -
                d->m_cardInfo.memInfo.freeram) /
               1048576;
    else
        return 0;
}

void RAMCard::init()
{
    // Get dynamic-changing values
    d->m_cardInfo.memorySpace.free  = this->free();
    d->m_cardInfo.memorySpace.usage = this->usage();

    if (d->m_cardInfo.type == "Unknown")
        d->m_cardInfo.product += " (empty)";

    // Setup UUID
    d->m_cardInfo.guid = Libraries::generateGuid(
        std::string(d->m_cardInfo.product + d->m_cardInfo.serial));
}

void RAMCard::updateDynamic()
{
    d->m_cardInfo.memorySpace.free  = this->free();
    d->m_cardInfo.memorySpace.usage = this->usage();
}

std::string& RAMCard::uuid()
{
    return d->m_cardInfo.guid.value();
}



} // namespace Hardware
