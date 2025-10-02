#ifndef GPUMANAGER_H
#define GPUMANAGER_H

#include <Libraries/Internal/AbstractHardware.hpp>
#include <memory>

namespace Hardware
{

class GPUManager final : public Libraries::AbstractHardware
{
  public:
    void deinit() override;
    std::pair<std::string, std::string> getDriverVersions() const;

    void dump() override;

    DECLARE_HARDWARE(GPUManager, Libraries::HardwareType::GPU)

  private:
    bool setOverdrive(const std::string& uuid,
                      const std::string& overdriveParams);

    struct GPUManagerPrivate;
    std::shared_ptr<GPUManagerPrivate> d;

    void parseNodeTree();
};

} // namespace Hardware

#endif // GPUMANAGER_H
