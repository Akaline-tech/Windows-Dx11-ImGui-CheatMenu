# Windows-DX11-ImGui-CheatMenu

A modern customizable cheat menu framework based on **Dear ImGui + DirectX11**.

This project provides a lightweight and clean UI system for Windows applications, game tools, debug menus, and custom overlays. It includes a custom ImGui extension layer (`imguiSDK`) that simplifies creating modern interfaces with animations, custom widgets, and theme support.

<img width="1110" height="709" alt="d8c8448cd39fdd338fd83da99e356d2c" src="https://github.com/user-attachments/assets/3025d9bd-d143-4672-ad8d-d9c86256b968" />

## Features

- DirectX11 rendering backend
- Dear ImGui based interface
- Custom UI framework (`imguiSDK`)
- Modern dark theme design
- Sidebar navigation system
- Custom buttons, sliders, switches, and tabs
- Smooth animations
- Global theme color support
- Easy integration into existing projects
- Lightweight and dependency friendly

## Requirements

- Windows 10 / Windows 11
- Visual Studio 2022
- DirectX 11 SDK
- Dear ImGui

Recommended:


Visual Studio 2022
C++17 or newer
Windows SDK

# Installation

## 1. Add Dear ImGui

Add the original Dear ImGui source files:


imgui.cpp
imgui_draw.cpp
imgui_tables.cpp
imgui_widgets.cpp


Add backend files:


imgui_impl_dx11.cpp
imgui_impl_win32.cpp


Include directories:


#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"


---

# Custom UI SDK

This project contains a custom UI extension:


imguiSDK.cpp
imguiSDK.h


You only need to add these two files into your project.

Example:


YourProject
│
├── imguiSDK.cpp
└── imguiSDK.h


Then include:

```cpp
#include "imguiSDK.h"

No additional library is required.
