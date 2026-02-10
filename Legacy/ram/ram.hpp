#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include <Libraries/Internal/AbstractHardware.hpp>

namespace Hardware
{
class RAMCard
{
  public:
    RAMCard(const Libraries::Internal::RAM_CardInfo& cardInfo);
    ~RAMCard() = default;

    void dump();

    void init();
    void updateDynamic();

    std::string& uuid();

    int64_t free() const;
    int64_t total() const;
    int64_t usage() const;

  private:
    struct RAMCardPrivate;
    std::shared_ptr<RAMCardPrivate> d;
};

} // namespace Hardware
