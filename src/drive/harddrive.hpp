/**
 * @file hardwares.hpp
 * @author Root Falls ()
 * @brief File contains HardDrive hardware
 * @version 0.1
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef DriveH
#define DriveH

#include <nlohmann/json.hpp>
#include <stdint.h>
#include <string>
#include <vector>

#include <Libraries/Internal/AbstractHardware.hpp>

namespace Hardware
{
class DriveManager final : public Libraries::AbstractHardware
{
  public:
    DECLARE_HARDWARE(DriveManager, Libraries::HardwareType::Harddrives)

    void dump() override;

  private:
    void updateDynamic();

    struct DriveManagerPrivate;
    std::shared_ptr<DriveManagerPrivate> d;

    void parseNodeTree();
};
} // namespace Hardware

#endif // DriveH
