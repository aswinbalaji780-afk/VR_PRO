#include "InputTranslator.hpp"
#include <iostream>

namespace VRMod {
namespace InputTranslator {

    // Store state so we don't spam keys if they are held down
    static bool g_attackState = false;
    static bool g_deflectState = false;

    bool Initialize() {
        std::cout << "[InputTranslator] VR Input Translation Layer Initialized. Keyboard/Mouse injection ready.\n";
        return true;
    }

    void SendMouseClick(DWORD flags) {
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = flags;
        SendInput(1, &input, sizeof(INPUT));
    }

    void OnActionAttack(bool isPressed) {
        if (isPressed && !g_attackState) {
            // Trigger Pulled -> Simulate Left Mouse Down
            SendMouseClick(MOUSEEVENTF_LEFTDOWN);
            g_attackState = true;
        } else if (!isPressed && g_attackState) {
            // Trigger Released -> Simulate Left Mouse Up
            SendMouseClick(MOUSEEVENTF_LEFTUP);
            g_attackState = false;
        }
    }

    void OnActionDeflect(bool isPressed) {
        if (isPressed && !g_deflectState) {
            // Trigger Pulled -> Simulate Right Mouse Down
            SendMouseClick(MOUSEEVENTF_RIGHTDOWN);
            g_deflectState = true;
        } else if (!isPressed && g_deflectState) {
            // Trigger Released -> Simulate Right Mouse Up
            SendMouseClick(MOUSEEVENTF_RIGHTUP);
            g_deflectState = false;
        }
    }

    void UpdateMouseLook(float deltaPitch, float deltaYaw, float sensX, float sensY, bool invertY) {
        if (deltaPitch == 0.0f && deltaYaw == 0.0f) return;

        int mouseDX = (int)(deltaYaw * sensX);
        int mouseDY = (int)(deltaPitch * sensY);
        if (invertY) {
            mouseDY = -mouseDY;
        }

        if (mouseDX != 0 || mouseDY != 0) {
            INPUT input = {0};
            input.type = INPUT_MOUSE;
            input.mi.dx = mouseDX;
            input.mi.dy = mouseDY;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            SendInput(1, &input, sizeof(INPUT));
        }
    }

    void Shutdown() {
        std::cout << "[InputTranslator] Shutting down Input Translation Layer.\n";
        // Ensure keys aren't left stuck down if closed unexpectedly
        if (g_attackState) SendMouseClick(MOUSEEVENTF_LEFTUP);
        if (g_deflectState) SendMouseClick(MOUSEEVENTF_RIGHTUP);
    }

} // namespace InputTranslator
} // namespace VRMod
