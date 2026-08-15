#include "MemoryManager.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <windows.h>

namespace VRMod {
namespace MemoryManager {

    static uintptr_t g_baseAddress = 0;
    
    // Authentic-looking default Camera Pointer Chain (fallback)
    static uintptr_t g_cameraBaseOffset = 0x03D5C000; 
    static std::vector<unsigned int> g_cameraOffsets = { 0x20, 0x58, 0x8, 0x10 };

    // Utility function to resolve multi-level pointers (DMA - Dynamic Memory Allocation)
    uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets) {
        uintptr_t addr = ptr;
        for (unsigned int i = 0; i < offsets.size(); ++i) {
            if (IsBadReadPtr((void*)addr, sizeof(uintptr_t))) return 0;
            addr = *(uintptr_t*)addr;
            if (addr == 0) return 0;
            addr += offsets[i];
        }
        return addr;
    }

    static bool g_motionEnabled = false;
    static uintptr_t g_playerBaseOffset = 0x041D3A00;
    static std::vector<unsigned int> g_playerOffsets = { 0x10, 0x28, 0x68 };
    static unsigned int g_boneArrayOffset = 0x8A0;
    static int g_rightHandBoneIndex = 42;
    static int g_leftHandBoneIndex = 43;

    std::string GetConfigPath(HMODULE hModule) {
        char path[MAX_PATH];
        GetModuleFileNameA(hModule, path, MAX_PATH);
        std::string fullPath(path);
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            std::string dir = fullPath.substr(0, lastSlash);
            std::string sameDir = dir + "\\vr_config.ini";
            if (GetFileAttributesA(sameDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
                return sameDir; // Found in same folder (Distribution Mod)
            }
            return dir + "\\..\\..\\vr_config.ini"; // Development fallback
        }
        return ".\\vr_config.ini";
    }

    bool Initialize(HMODULE hModule) {
        // GetModuleHandle(NULL) gets the dynamic base address of the target executable
        g_baseAddress = (uintptr_t)GetModuleHandle(NULL);
        
        if (g_baseAddress == 0) {
            std::cout << "[MemoryManager] Failed to get base address!\n";
            return false;
        }

        std::string configPath = GetConfigPath(hModule);

        // Parse Camera
        char baseOffsetBuf[256];
        if (GetPrivateProfileStringA("Memory", "CameraBaseOffset", "", baseOffsetBuf, 256, configPath.c_str())) {
            g_cameraBaseOffset = std::stoul(baseOffsetBuf, nullptr, 16);
        }

        char chainBuf[256];
        if (GetPrivateProfileStringA("Memory", "CameraOffsets", "", chainBuf, 256, configPath.c_str())) {
            g_cameraOffsets.clear();
            std::string chain(chainBuf);
            size_t pos = 0;
            while ((pos = chain.find(',')) != std::string::npos) {
                g_cameraOffsets.push_back(std::stoul(chain.substr(0, pos), nullptr, 16));
                chain.erase(0, pos + 1);
            }
            if (!chain.empty()) g_cameraOffsets.push_back(std::stoul(chain, nullptr, 16));
        }

        // Parse Motion Controllers
        char enabledBuf[16];
        GetPrivateProfileStringA("MotionControllers", "Enabled", "false", enabledBuf, 16, configPath.c_str());
        g_motionEnabled = (_stricmp(enabledBuf, "true") == 0);

        if (g_motionEnabled) {
            char pBaseBuf[256];
            if (GetPrivateProfileStringA("MotionControllers", "PlayerBaseOffset", "", pBaseBuf, 256, configPath.c_str())) {
                g_playerBaseOffset = std::stoul(pBaseBuf, nullptr, 16);
            }
            
            char pChainBuf[256];
            if (GetPrivateProfileStringA("MotionControllers", "PlayerOffsets", "", pChainBuf, 256, configPath.c_str())) {
                g_playerOffsets.clear();
                std::string chain(pChainBuf);
                size_t pos = 0;
                while ((pos = chain.find(',')) != std::string::npos) {
                    g_playerOffsets.push_back(std::stoul(chain.substr(0, pos), nullptr, 16));
                    chain.erase(0, pos + 1);
                }
                if (!chain.empty()) g_playerOffsets.push_back(std::stoul(chain, nullptr, 16));
            }

            g_boneArrayOffset = GetPrivateProfileIntA("MotionControllers", "BoneArrayOffset", 0x8A0, configPath.c_str());
            g_rightHandBoneIndex = GetPrivateProfileIntA("MotionControllers", "RightHandBoneIndex", 42, configPath.c_str());
            g_leftHandBoneIndex = GetPrivateProfileIntA("MotionControllers", "LeftHandBoneIndex", 43, configPath.c_str());
        }

        std::cout << "[MemoryManager] Initialized. Base Address: " << std::hex << g_baseAddress << "\n";
        return true;
    }

    void UpdateCamera(float x, float y, float z, float pitch, float yaw, float roll) {
        if (g_baseAddress == 0) return;

        uintptr_t cameraStruct = FindDMAAddy(g_baseAddress + g_cameraBaseOffset, g_cameraOffsets);
        if (cameraStruct == 0) return;

        float* camX = (float*)(cameraStruct + 0x80);
        float* camY = (float*)(cameraStruct + 0x84);
        float* camZ = (float*)(cameraStruct + 0x88);

        if (!IsBadWritePtr(camX, sizeof(float))) *camX += x; 
        if (!IsBadWritePtr(camY, sizeof(float))) *camY += y;
        if (!IsBadWritePtr(camZ, sizeof(float))) *camZ += z;

        float* camPitch = (float*)(cameraStruct + 0xA0); 
        float* camYaw   = (float*)(cameraStruct + 0xA4);
        float* camRoll  = (float*)(cameraStruct + 0xA8);

        if (!IsBadWritePtr(camPitch, sizeof(float))) *camPitch += pitch;
        if (!IsBadWritePtr(camYaw, sizeof(float))) *camYaw += yaw;
        if (!IsBadWritePtr(camRoll, sizeof(float))) *camRoll += roll;
    }

    void UpdateHands(float rx, float ry, float rz, float rp, float ryaw, float rroll,
                     float lx, float ly, float lz, float lp, float lyaw, float lroll) {
        if (!g_motionEnabled || g_baseAddress == 0) return;

        // 1. Resolve pointer to Local Player Character
        uintptr_t playerStruct = FindDMAAddy(g_baseAddress + g_playerBaseOffset, g_playerOffsets);
        if (playerStruct == 0) return;

        // 2. Resolve pointer to Havok Bone Array
        uintptr_t boneArray = *(uintptr_t*)(playerStruct + g_boneArrayOffset);
        if (boneArray == 0 || IsBadReadPtr((void*)boneArray, 8)) return;

        // 3. Each bone matrix is 64 bytes (4x4 floats, usually padded or struct aligned)
        // Note: Havok standard transform is often [Vector4 Translation][Quaternion Rotation][Vector4 Scale]
        // Assuming flat 4x4 matrix representation for simplicity in this example hook:
        size_t matrixSize = 64; 

        // Update Right Hand Bone
        float* rightMatrix = (float*)(boneArray + (g_rightHandBoneIndex * matrixSize));
        if (!IsBadWritePtr(rightMatrix, matrixSize)) {
            rightMatrix[12] = rx; // Translation X
            rightMatrix[13] = ry; // Translation Y
            rightMatrix[14] = rz; // Translation Z
            // Note: Full quaternion/euler rotation matrix population is highly engine specific.
            // A true implementation needs to rebuild the 3x3 rotation matrix from rp, ryaw, rroll.
        }

        // Update Left Hand Bone
        float* leftMatrix = (float*)(boneArray + (g_leftHandBoneIndex * matrixSize));
        if (!IsBadWritePtr(leftMatrix, matrixSize)) {
            leftMatrix[12] = lx; // Translation X
            leftMatrix[13] = ly; // Translation Y
            leftMatrix[14] = lz; // Translation Z
        }
    }

} // namespace MemoryManager
} // namespace VRMod
