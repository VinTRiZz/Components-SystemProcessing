#ifndef RAMCARDMANAGER_HPP
#define RAMCARDMANAGER_HPP

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include <Libraries/Internal/AbstractHardware.hpp>

namespace Hardware
{

class RAMCardManager final : public Libraries::AbstractHardware
{
  public:
    DECLARE_HARDWARE(RAMCardManager, Libraries::HardwareType::RAM)

    void dump() override;

  private:
    void updateDynamic();

    struct Impl;
    std::shared_ptr<Impl> d;

    void parseNodeTree();
};

}

#endif // RAMCARDMANAGER_HPP
