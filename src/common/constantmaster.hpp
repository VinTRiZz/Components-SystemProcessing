#ifndef CONSTANTMASTER_H
#define CONSTANTMASTER_H

#include <memory>

#include <Libraries/Internal/JOptional.hpp>
#include <Libraries/Datawork/SysInfoMaster.hpp>

namespace Libraries
{

class ConstantMaster final
{
  public:
    static ConstantMaster& getInstance();
    ConstantMaster(ConstantMaster&& cm)         = default;
    ConstantMaster(const ConstantMaster& cm)    = default;
    ~ConstantMaster();

    void init();

    // If says "Unknown vendor", try std::toupper()
    JOptional<std::string> getSubvendor(const std::string& hexCode) const;

    JOptional<int64_t> getGpuId(const std::string& pciId);
    JOptional<int64_t> getPciId(int16_t gpuId) const;
    JOptional<std::string> getPciIdHex(int16_t gpuId) const;

    SysinfoMaster getDmiManager() const;

  private:
    struct ConstantMasterPrivate;
    std::shared_ptr<ConstantMasterPrivate> d;

    void updateCardPciEqus();

    ConstantMaster();
};

} // namespace Libraries

#endif // CONSTANTMASTER_H
