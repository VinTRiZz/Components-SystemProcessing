#ifndef SYSINFOMASTER_HPP
#define SYSINFOMASTER_HPP

#include <memory>
#include <vector>

class hwNode;

namespace Libraries
{

class SysinfoMaster
{
  public:
    SysinfoMaster();
    ~SysinfoMaster();

    void updateInfo();

    hwNode* getPropertyTree();

  private:
    struct DmiManagerPrivate;
    std::shared_ptr<DmiManagerPrivate> d;

    void scanDevices();
};

} // namespace Libraries

#endif // SYSINFOMASTER_HPP
