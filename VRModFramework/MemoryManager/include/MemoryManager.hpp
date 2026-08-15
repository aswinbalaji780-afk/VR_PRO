#ifndef VRMOD_MEMORYMANAGER_HPP
#define VRMOD_MEMORYMANAGER_HPP

#include <windows.h>
#include <cstdint>

namespace VRMod {
namespace MemoryManager {

    // Call once to attach to target game memory
    bool Initialize(HMODULE hModule);

    // Directly overwrites the game's camera struct using fast pointer arithmetic
    void UpdateCamera(float x, float y, float z, float pitch, float yaw, float roll);

    // Overwrites the Havok Bone Transform Matrices for the left and right hands
    void UpdateHands(float rx, float ry, float rz, float rp, float ryaw, float rroll,
                     float lx, float ly, float lz, float lp, float lyaw, float lroll);

} // namespace MemoryManager
} // namespace VRMod

#endif // VRMOD_MEMORYMANAGER_HPP
