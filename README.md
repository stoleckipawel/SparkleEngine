<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?style=for-the-badge&logo=cplusplus" alt="C++20"/>
  <img src="https://img.shields.io/badge/DirectX-12-green?style=for-the-badge&logo=microsoft" alt="DirectX 12"/>
  <img src="https://img.shields.io/badge/HLSL-SM%206.0%2B-purple?style=for-the-badge" alt="HLSL SM 6.0+"/>
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" alt="MIT License"/>
</p>

<h1 align="center">Sparkle Engine</h1>

<p align="center">
  <strong>A modern DirectX 12 rendering engine built with clean architecture principles</strong>
</p>

---

## Key Features

| | Category | Highlights |
|:-:|----------|------------|
| :art: | **Graphics** | DirectX 12 with explicit resource management, descriptor heaps, triple-buffered swap chain |
| :movie_camera: | **Camera** | 3-tier architecture (GameCamera  RenderCamera  Frustum) with FPS controls and culling |
| :video_game: | **Input** | Layer-based event routing, RAII subscriptions, mouse capture, pollable state |
| :triangular_ruler: | **Geometry** | 15 procedural primitives including Platonic solids and subdivision surfaces |
| :straight_ruler: | **Depth** | Configurable Reversed-Z for superior precision in large scenes |
| :file_folder: | **Assets** | Marker-based discovery with compile-time hashed asset IDs |
| :desktop_computer: | **UI** | Integrated ImGui with debug panels and stats overlay |

---

## Architecture

Sparkle Engine uses a **modular DLL architecture** where each subsystem is a separate library that can be built as either a shared (DLL) or static library. This enables faster iteration, cleaner dependencies, and better code organization.

<table>
<tr>
<td align="center"><b>Module</b></td>
<td><b>Library</b></td>
<td><b>Description</b></td>
</tr>
<tr>
<td align="center">🧱<br><b>Core</b></td>
<td><code>SparkleCore</code></td>
<td>Foundation: Math, Events, Timer, Diagnostics, Logging</td>
</tr>
<tr>
<td align="center">🖥️<br><b>Platform</b></td>
<td><code>SparklePlatform</code></td>
<td>OS abstraction: Window, Input System</td>
</tr>
<tr>
<td align="center">⚡<br><b>RHI</b></td>
<td><code>SparkleRHI</code></td>
<td>DirectX 12 backend: Device, Heaps, PSO, Shaders, Resources</td>
</tr>
<tr>
<td align="center">🎨<br><b>Renderer</b></td>
<td><code>SparkleRenderer</code></td>
<td>High-level rendering: Camera, Depth, Textures, Materials</td>
</tr>
<tr>
<td align="center">🎮<br><b>GameFramework</b></td>
<td><code>SparkleGameFramework</code></td>
<td>Game systems: Scene, Mesh, Assets, Application framework</td>
</tr>
<tr>
<td align="center">🖼️<br><b>UI</b></td>
<td><code>SparkleUI</code></td>
<td>ImGui integration: Panels, Overlays, Debug tools</td>
</tr>
</table>

### Module Dependencies

```
SparkleCore (base - no dependencies)
    └── SparklePlatform
            └── SparkleRHI
                    └── SparkleRenderer
                            ├── SparkleUI
                            └── SparkleGameFramework
```

### Design Principles

| Principle | Implementation |
|-----------|----------------|
| **Per-Module PCH** | Each module has its own precompiled header for fast builds |
| **Global Logging** | `Log.h` included in all PCH files — `LOG_INFO`, `CHECK` available everywhere |
| **Public/Private Split** | Public headers in `Public/`, implementation in `Private/` |
| **DLL/Static Toggle** | `SPARKLE_BUILD_SHARED` CMake option switches between DLL and static libs |

---

## Quick Start

| Step | Command | Description |
|:----:|---------|-------------|
| 1 | `Scripts\Setup.bat` | First-time setup (tools + deps + solution) |
| 2 | `Scripts\BuildProjects.bat` | Interactive build (select project + config) |
| 3 | `Scripts\CreateNewProject.bat MyGame` | Create new project from template |

**Prerequisites:** Visual Studio 2022 (17.0+)  Windows SDK (10.0.19041+)  CMake (3.20+)

---

## Technical Highlights

<table>
<tr>
<td width="50%" valign="top">

### DirectX 12

| Feature | |
|---------|:-:|
| Managed descriptor heap allocation | :white_check_mark: |
| Root signature with CBV/SRV/UAV | :white_check_mark: |
| Fence-based CPU/GPU sync | :white_check_mark: |
| Frame resources + linear allocator | :white_check_mark: |
| GPU validation & debug layer | :white_check_mark: |

</td>
<td width="50%" valign="top">

### Engine Systems

| Feature | |
|---------|:-:|
| 3-tier camera with dirty flags | :white_check_mark: |
| Event-driven layered input | :white_check_mark: |
| DXC shader compilation (SM 6.0+) | :white_check_mark: |
| WIC texture loading | :white_check_mark: |
| PBR-ready shader library | :white_check_mark: |

</td>
</tr>
</table>

---

## Design Patterns & Decisions

<table>
<tr>
<td width="50%" valign="top">

### Patterns Used

| Pattern | Application |
|---------|-------------|
| **RAII** | D3D12 resource wrappers |
| **NVI** | App lifecycle hooks |
| **Observer** | Input & depth events |
| **Factory** | Mesh generation |
| **Strategy** | Input backends |

</td>
<td width="50%" valign="top">

### Key Decisions

| Decision | Rationale |
|----------|-----------|
| **3-Tier Camera** | Separates game, render, cull |
| **Reversed-Z** | Better depth precision |
| **Frame Resources** | No CPU/GPU stalls |
| **Compile-Time Hash** | Zero-cost asset IDs |

</td>
</tr>
</table>

---

## Project Structure

<table>
<tr>
<td width="50%" valign="top">

### Engine Modules

| Directory | Contents |
|-----------|----------|
| `Engine/Core/` | Math, Events, Timer, Diagnostics, Logging |
| `Engine/Platform/` | Window, Input System |
| `Engine/RHI/` | D3D12 Device, Heaps, PSO, Shaders |
| `Engine/Renderer/` | Camera, Depth, Textures, Materials |
| `Engine/GameFramework/` | Scene, Mesh, Assets, App Framework |
| `Engine/UI/` | ImGui Panels, Overlays, Debug Tools |
| `Engine/third_party/` | d3dx12 |

Each module follows the structure:
```
Module/
├── Public/      # Headers for other modules
├── Private/     # Implementation (.cpp, internal .h)
│   └── PCH.h    # Per-module precompiled header
└── CMakeLists.txt
```

</td>
<td width="50%" valign="top">

### Game Projects

| Project | Description |
|---------|-------------|
| `Projects/ABeautifulGame/` | A Beautiful Game scene viewer |
| `Projects/CesiumMan/` | CesiumMan scene viewer |
| `Projects/DamagedHelmet/` | DamagedHelmet scene viewer |
| `Projects/DiffuseTransmissionPlant/` | DiffuseTransmissionPlant scene viewer |
| `Projects/Sponza/` | Sponza scene viewer |
| `Projects/TemplateProject/` | Template for new projects |

### Build Output

| Directory | Contents |
|-----------|----------|
| `build/` | CMake cache, VS solution |
| `build/lib/` | Module libraries (.lib/.dll) |
| `bin/Debug/` | Debug builds + PDBs |
| `bin/Release/` | Optimized builds |

### Scripts

All build and development scripts are located in the `Scripts/` folder.

| Script | Purpose |
|--------|---------|
| `Setup.bat` | First-time setup (validate tools, fetch deps, generate solution) |
| `GenerateProjectFiles.bat` | Incremental VS solution generation via CMake |
| `BuildProjects.bat` | Interactive project + config build menu |
| `CreateNewProject.bat <name>` | Scaffold new game from template |
| `CheckDependencies.bat` | Verify build prerequisites |
| `CheckThirdParty.bat` | Validate/sync third-party dependencies |
| `Clean.bat` | Clean build artifacts, deps, or everything |
| `RunClangFormat.bat` | Format all source files |

**Usage from repository root:**
```powershell
Scripts\Setup.bat                    # First-time setup (clone → build)
Scripts\GenerateProjectFiles.bat     # Regenerate VS solution
Scripts\BuildProjects.bat            # Interactive build menu
Scripts\Clean.bat                    # Clean with options menu
Scripts\CreateNewProject.bat MyGame  # Create new project
```

**Internal scripts** (`Scripts/Internal/`) handle logging, shared config, and build implementation.  
**Build logs** are written to the `logs/` folder (gitignored).

</td>
</tr>
</table>

---

<p align="center">
  <strong>MIT License</strong>  See <a href="LICENSE.txt">LICENSE.txt</a>
</p>

<p align="center">
  <sub>Built with modern C++20 and DirectX 12</sub>
</p>
