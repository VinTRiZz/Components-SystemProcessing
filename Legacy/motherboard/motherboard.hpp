/**
 * @file hardwares.hpp
 * @author Root Falls ()
 * @brief File contains Motherboard hardware
 * @version 0.1
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef MOTHERBOARD_H
#define MOTHERBOARD_H

#include <Libraries/Internal/AbstractHardware.hpp>
#include <string>

namespace Hardware
{

class Motherboard final : public Libraries::AbstractHardware
{
  public:
    DECLARE_HARDWARE(Motherboard, Libraries::HardwareType::Motherboard)

    void dump() override;

  private:
    struct MotherboardPrivate;
    std::shared_ptr<MotherboardPrivate> d;

    void parseNodeTree();
    void setupPCISlots();
    void setupUSBSlots();
};
} // namespace Hardware

#endif // MOTHERBOARD_H
