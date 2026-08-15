# Universal VR Mod Framework

A generic, open-source framework designed to inject DirectX 11 (D3D11) PC games with Stereoscopic 3D rendering and Universal OpenXR VR Tracking. 

This framework acts as a foundation for VR modders. Out of the box, it provides a "Universal" tier of VR support (3D graphics + Mouse Emulation Headtracking). With advanced configuration, it supports "Native" VR support (6DOF tracking + Physical Motion Controls) via memory overriding.

## Features

- **Stereoscopic 3D Graphics**: Hooks the DirectX 11 Present pipeline and injects a custom HLSL shader to render left and right eye views, passing the stereoscopic texture natively to OpenXR headsets.
- **Universal Head Tracking**: Intercepts the physical rotation of your VR headset and translates it into standard Windows Mouse Movement (`SendInput`), allowing you to look around in almost any first-person or third-person game.
- **Native 6DOF Tracking (Memory Overriding)**: Advanced mode that directly overwrites the game engine's camera transformation matrix using pointer chains, providing true 1:1 positional and rotational head tracking.
- **Native VR Motion Controls**: Creates OpenXR Hand Spaces and overwrites the target game's Havok/Animation skeletal bone matrices, mapping your physical arm movements to the in-game character's arms.

## Usage

1. Download the latest release folder.
2. Open `vr_config.ini` in a text editor.
3. Change `ProcessName` to the exact executable name of your game (e.g. `sekiro`, `gta5`).
4. Start your game.
5. Run `Launch_VR.bat` as Administrator. 

## Advanced Configuration (For Modders)

The framework relies on memory offsets for true VR presence (6DOF and Motion Controls). Since memory addresses are dynamic and specific to the game engine, you must use tools like Cheat Engine to find the pointer chains for your target game.

### 1. 6DOF Head Tracking
Find the memory address that dictates the game's camera coordinates (Translation and Rotation). Enter the base offset and the pointer chain in `vr_config.ini`:
```ini
[Memory]
CameraBaseOffset=0x03D5C000
CameraOffsets=0x20,0x58,0x8,0x10
```
Set `Mode=Memory` under `[HeadTracking]` to activate.

### 2. Physical Motion Controls
Find the pointer chain to the Local Player Instance, the offset to the Skeletal Bone Array, and the specific integer indices of the Left Arm and Right Arm bones.
```ini
[MotionControllers]
Enabled=true
PlayerBaseOffset=041D3A00
PlayerOffsets=10,28,68
BoneArrayOffset=0x8A0
RightHandBoneIndex=42
LeftHandBoneIndex=43
```

## Contributing
If you find the memory offsets for a popular game, please submit a Pull Request to add the `.ini` profile to our repository so others can play that game in true VR!

## Building from Source
Requirements:
- Visual Studio 2022 (MSVC)
- CMake
- OpenXR SDK
- MinHook

```powershell
mkdir build
cd build
cmake .. -A x64
cmake --build . --config Release
```
