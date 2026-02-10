#ifndef HWNODESWORK_HPP
#define HWNODESWORK_HPP

#include <Libraries/Internal/Structures.hpp>
#include <lshw-dmi/common.h>

namespace Libraries
{

void setupFromNode(hwNode* pNode, Libraries::Internal::HardwareLoggableStruct* pTargetStruct);

void printHwTree(hwNode* pNode, int spaces = 0);

void printNodeInfo(hwNode* pNode);

void printChildren(hwNode* pNode);

// Recursive search
hwNode* findChild(hw::hwClass hwClassid, hwNode* searchNode);
void searchForDevices(hw::hwClass hwClassId, hwNode* searchNode, std::vector<hwNode*>& oVect);

}

#endif // HWNODESWORK_HPP
