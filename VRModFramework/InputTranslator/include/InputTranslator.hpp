#ifndef VRMOD_INPUTTRANSLATOR_HPP
#define VRMOD_INPUTTRANSLATOR_HPP

#include <windows.h>

namespace VRMod {
namespace InputTranslator {

    // Initializes the input mapping system (e.g., loading keybinds)
    bool Initialize();

    // Simulates pressing a physical keyboard key
    void SendKeyPress(WORD virtualKey);

    // Simulates releasing a physical keyboard key
    void SendKeyRelease(WORD virtualKey);

    // Handles the "Attack" action (e.g., Right Trigger pulled)
    void OnActionAttack(bool isPressed);

    // Handles the "Deflect" action (e.g., Left Trigger pulled)
    void OnActionDeflect(bool isPressed);

    // Translates Headset Rotation Deltas into synthetic Mouse Movement
    void UpdateMouseLook(float deltaPitch, float deltaYaw, float sensX, float sensY, bool invertY);

    // Shuts down the input translator
    void Shutdown();

} // namespace InputTranslator
} // namespace VRMod

#endif // VRMOD_INPUTTRANSLATOR_HPP
