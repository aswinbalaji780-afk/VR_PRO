#include "InputTranslator.hpp"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace VRMod {
namespace InputTranslator {

    // Store state so we don't spam keys if they are held down
    static bool g_attackState = false;
    static bool g_deflectState = false;

    bool Initialize() {
        std::cout << "[InputTranslator] VR Input Translation Layer Initialized. Keyboard/Mouse injection ready.\n";
        return true;
    }

    void SendMouseClick(bool left, bool down) {
#ifdef _WIN32
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        if (left) {
            input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        } else {
            input.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
        }
        SendInput(1, &input, sizeof(INPUT));
#elif defined(__APPLE__)
        CGEventType eventType;
        CGMouseButton button;
        if (left) {
            eventType = down ? kCGEventLeftMouseDown : kCGEventLeftMouseUp;
            button = kCGMouseButtonLeft;
        } else {
            eventType = down ? kCGEventRightMouseDown : kCGEventRightMouseUp;
            button = kCGMouseButtonRight;
        }
        
        CGEventRef currentEvent = CGEventCreate(NULL);
        CGPoint currentPos = CGEventGetLocation(currentEvent);
        CFRelease(currentEvent);

        CGEventRef event = CGEventCreateMouseEvent(NULL, eventType, currentPos, button);
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
#endif
    }

    void OnActionAttack(bool isPressed) {
        if (isPressed && !g_attackState) {
            SendMouseClick(true, true);
            g_attackState = true;
        } else if (!isPressed && g_attackState) {
            SendMouseClick(true, false);
            g_attackState = false;
        }
    }

    void OnActionDeflect(bool isPressed) {
        if (isPressed && !g_deflectState) {
            SendMouseClick(false, true);
            g_deflectState = true;
        } else if (!isPressed && g_deflectState) {
            SendMouseClick(false, false);
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
#ifdef _WIN32
            INPUT input = {0};
            input.type = INPUT_MOUSE;
            input.mi.dx = mouseDX;
            input.mi.dy = mouseDY;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            SendInput(1, &input, sizeof(INPUT));
#elif defined(__APPLE__)
            CGEventRef currentEvent = CGEventCreate(NULL);
            CGPoint currentPos = CGEventGetLocation(currentEvent);
            CFRelease(currentEvent);
            
            CGPoint newPos = CGPointMake(currentPos.x + mouseDX, currentPos.y + mouseDY);
            CGEventRef moveEvent = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, newPos, kCGMouseButtonLeft);
            CGEventPost(kCGHIDEventTap, moveEvent);
            CFRelease(moveEvent);
#endif
        }
    }

    void Shutdown() {
        std::cout << "[InputTranslator] Shutting down Input Translation Layer.\n";
        if (g_attackState) SendMouseClick(true, false);
        if (g_deflectState) SendMouseClick(false, false);
    }

} // namespace InputTranslator
} // namespace VRMod
