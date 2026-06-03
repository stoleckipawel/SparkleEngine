# Phase 6 Final Build, Package, And Fresh-Machine Validation Report

Date: 2026-06-03

Status: Passed with follow-up risks.

Validated Goal:

- fresh MSVC configure from an isolated build tree
- launcher, tools, editor, runtime, cook, package, manifest, checksum, and launch-first package smoke
- no release package was built directly from `build/`

Environment:

| Tool | Result |
| --- | --- |
| CMake | `4.3.3` at `C:\Program Files\CMake\bin\cmake.exe` |
| Visual Studio | Visual Studio 2026 Developer Command Prompt `18.6.2` |
| MSVC | `19.51.36246` for x64 |
| Windows SDK | `10.0.26100.0` selected by CMake |
| Qt kit | `C:\Qt\6.11.1\msvc2022_64` |
| Git | `C:\Program Files\Git\cmd\git.exe` |
| Source commit | `9cac861d4f228e28c73ad6994d1223d71657cdda` |

Validated Paths:

| Area | Path |
| --- | --- |
| Build tree | `build-phase6-msvc` |
| Launcher artifact | `artifacts/dev/launcher/DevelopmentEditor/SparkleLauncher.exe` |
| Tool artifacts | `artifacts/dev/tools/<Tool>/DevelopmentEditor/` |
| Showcase editor artifact | `artifacts/dev/projects/Showcase/editor/DevelopmentEditor/ShowcaseEditor.exe` |
| Showcase runtime artifact | `artifacts/dev/projects/Showcase/runtime/DevelopmentEditor/ShowcaseRuntime.exe` |
| Showcase cooked root | `artifacts/dev/projects/Showcase/cooked/` |
| Runtime package | `dist/releases/0.0.0-phase6/sparkle-runtime-0.0.0-phase6-dev-windows-x64/` |
| Symbols package | `dist/releases/0.0.0-phase6/sparkle-symbols-0.0.0-phase6-dev-windows-x64/` |
| Symbols archive | `dist/releases/0.0.0-phase6/sparkle-symbols-0.0.0-phase6-dev-windows-x64.zip` |

Commands Run:

```bat
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64
set "PATH=C:\Program Files\Git\cmd;%PATH%"
"C:\Program Files\CMake\bin\cmake.exe" --fresh -S . -B build-phase6-msvc -G "Visual Studio 18 2026" -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\msvc2022_64 -DSPARKLE_PACKAGE_VERSION=0.0.0-phase6 -DSPARKLE_RELEASE_CHANNEL=dev -DSPARKLE_PACKAGE_PLATFORM=windows-x64
"C:\Program Files\CMake\bin\cmake.exe" --build build-phase6-msvc --config DevelopmentEditor --target SparkleLauncher AssetCooker TextureCooker ShaderCompiler ShowcaseEditor ShowcaseRuntime -- /m
```

```powershell
..\..\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe list-shaders --validate
..\..\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe cook --warnings-as-errors off --strip-reflection off --strip-debug off
artifacts\dev\tools\AssetCooker\DevelopmentEditor\AssetCooker.exe cook-textures Showcase DevelopmentGame --root .
artifacts\dev\tools\AssetCooker\DevelopmentEditor\AssetCooker.exe cook-assets Showcase DevelopmentGame --root .
```

```bat
"C:\Program Files\CMake\bin\cmake.exe" --build build-phase6-msvc --config DevelopmentEditor --target sparkle_release_assembly -- /m
```

Results:

| Check | Result |
| --- | --- |
| Clean MSVC configure | Passed |
| Launcher build | Passed |
| Required tool builds | Passed |
| Showcase editor/runtime builds | Passed |
| Shader registration validation | Passed, 17 typed shader registrations |
| Shader cook | Passed, 10 packages, 27 stage jobs, DXIL and SPIR-V |
| Texture cook | Passed, 163/163 textures |
| Scene/mesh/material cook | Passed, 9 scenes |
| Package assembly | Passed |
| Package manifest JSON parse | Passed |
| Package checksum verification | Passed, 558 manifest entries, 0 bad hashes using long-path-safe verification |
| Package-root launcher smoke | Passed, survived 5 seconds |
| Package ShowcaseEditor smoke | Passed, survived 5 seconds |
| Package ShowcaseRuntime smoke | Passed, survived 5 seconds |
| Symbols archive | Present |

Fixes Made During Validation:

- fixed `LauncherMainWindow.cpp` MSVC compile failure by capturing `targets` in the project clean-target lambda
- fixed multi-config artifact routing by setting per-config target output directories in `SparkleArtifactContract.cmake`
- fixed artifact-root `AssetCooker` runtime by copying `assimp-vc145-mt.dll` beside `AssetCooker.exe`
- fixed packaged editor/runtime root detection by adding package-root mode in `FileSystemUtils.cpp`

Dependency Report:

| User type | Needed |
| --- | --- |
| Runtime package user | packaged `SparkleLauncher.exe`, packaged editor/runtime apps, cooked assets, Qt runtime files staged beside launcher, normal GPU/runtime system support |
| Development/source user | Visual Studio MSVC toolchain, Windows SDK, CMake, Git on PATH, Qt MSVC kit, Vulkan SDK for shader compiler workflows, optional source dependency groups for content/shader workflows |

Risks And Follow-Ups:

- `git` and `cmake` were installed but not on this PowerShell PATH; configure needed absolute CMake path and explicit Git PATH injection.
- CMake 4.3 reports upstream/dependency developer warnings from Assimp custom commands and deprecated `FetchContent_Populate` use for D3D12MA/VMA.
- `clang-cl` smoke was not run because `clang-cl` is not installed/visible in the Visual Studio developer environment.
- Checksum verification on Windows needs long-path-safe handling because cooked texture paths exceed 260 characters in the package layout.
- The Phase 5 assembly target still prints a Phase 5-oriented "final validation was not run" status line even when invoked during Phase 6; the generated package data is valid, but the message should be renamed in a later polish pass.

Release Readiness Decision:

- Runtime package: ready for local review and launch-first exploration from `dist/`.
- Publishing: still should wait for a named release version, clean CI reproduction, clang-cl documentation or installation, and CMake warning cleanup.
