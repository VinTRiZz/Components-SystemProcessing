#ifndef OPENCLADAPTER_HPP
#define OPENCLADAPTER_HPP

#include <memory>
#include "../Internal/structures.hpp"

namespace Libraries
{

class OpenCLAdapter
{
public:
    OpenCLAdapter();
    ~OpenCLAdapter();

    bool updateInfo();

    Libraries::JOptional<std::string> cardName(int64_t gpuIndex, bool isAmdCard);
    Libraries::JOptional<std::string> cardInfoProvider(int64_t gpuIndex, bool isAmdCard);
    Libraries::JOptional<std::string> cardInfoProviderVersion(int64_t gpuIndex, bool isAmdCard);
    Libraries::JOptional<std::string> cardDriverVersion(int64_t gpuIndex, bool isAmdCard);

    std::string lastError() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

}

#endif // OPENCLADAPTER_HPP
