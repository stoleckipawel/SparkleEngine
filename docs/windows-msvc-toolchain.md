# Windows Toolchain Policy

SparkleEngine's official Windows development path is:

- Visual Studio with the Desktop C++ workload
- MSVC x64 build tools
- Windows SDK
- CMake
- Git
- Qt 6 Widgets using an MSVC-compatible x64 kit

This repository does not treat Qt `mingw_64` as a supported primary workflow.

The default compiler path is MSVC. `clang-cl` is also supported when used as a Visual Studio CMake toolset on top of the same Windows SDK and Qt MSVC kit.

## Supported Qt Kit

Install a Qt MSVC-compatible x64 kit such as:

- `C:\Qt\6.11.1\msvc2022_64`

If Sparkle Launcher cannot find the Qt kit automatically, set:

```powershell
$env:SPARKLE_QT_ROOT="C:\Qt\6.11.1\msvc2022_64"
```

Point `SPARKLE_QT_ROOT` at the Qt kit root that contains:

- `bin\qmake.exe`
- `lib\cmake\Qt6\Qt6Config.cmake`

## Visual Studio

Install Visual Studio with:

- Desktop development with C++
- MSVC x64/x86 build tools
- Windows SDK
- C++ CMake tools for Windows

Sparkle Launcher prefers the Visual Studio generator and validates the local MSVC toolchain before build workflows run.

## Clang Support

Clang support on Windows means `clang-cl`, not MinGW clang.

Use the same Visual Studio and Qt MSVC kit setup, then select the Clang toolset with either:

```powershell
$env:SPARKLE_USE_CLANGCL="1"
```

or:

```powershell
$env:SPARKLE_CMAKE_TOOLSET="ClangCL"
```

This keeps the repository on one Windows ABI and one Qt kit family while still allowing Clang-based compilation.

## Contributor Guidance

For Windows contributors:

- use Visual Studio as the default and documented Windows path
- use an MSVC-compatible Qt kit, not `mingw_64`
- use MSVC by default, or `clang-cl` intentionally through the supported Visual Studio toolset path
- treat Rider as optional IDE integration, not as a separate compiler/toolchain choice

If future MinGW support is added intentionally, it should be documented and validated as a separate supported workflow rather than being left as accidental compatibility.
