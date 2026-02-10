#ifndef GPUABSTRACT_HPP
#define GPUABSTRACT_HPP

#include "freqmanager.hpp"
#include "settingsworker.hpp"

#include <Libraries/Internal/Structures.hpp>
#include <Libraries/Constants/ConstantMaster.hpp>

typedef void PDisplay;

namespace Hardware
{
namespace GPU
{
enum class GPU_CARD_VENDOR {
    GPU_CARD_VENDOR_UNKNOWN,
    GPU_CARD_VENDOR_AMD,
    GPU_CARD_VENDOR_NVIDIA
};

class GPUCard
{
  public:
    static std::shared_ptr<GPUCard> createGPU(GPU_CARD_VENDOR gcv, int64_t gpuId, std::shared_ptr<PDisplay> pDisplay);

    GPUCard(const int64_t gpuId,
        GPU_CARD_VENDOR vendor = GPU_CARD_VENDOR::GPU_CARD_VENDOR_UNKNOWN);
    ~GPUCard();

    void dump();

    GPU_CARD_VENDOR getVendor() const;
    std::string getDriverVersion() const;

    void setGpuCardParameters(const Libraries::Internal::GPU_Parameters& pParameters);

    std::string uuid() const;

    nlohmann::json getFullInformation() const;

    void setupCard(const std::shared_ptr<CardSettingsWorker>& pCardSettings, const std::shared_ptr<AbstractFrequencyManager>& pFreqManager);

    int64_t getTemperature() const;

    void init();
    void updateDynamic();
    void setOverclock(const Libraries::Internal::OverclockParameters& paramStruct);
    bool isConnected() const;

    nlohmann::json getDynamic();

  private:
    GPU_CARD_VENDOR m_vendor {GPU_CARD_VENDOR::GPU_CARD_VENDOR_UNKNOWN};
    struct GPUCardPrivate;
    std::shared_ptr<GPUCardPrivate> d;
};
} // namespace GPU
} // namespace Hardware

#endif
