# Linux Platform Support

**Status:** negative/target capability dossier; no native Linux product path was found

**Scope:** define the absent native Linux build, platform, Vulkan presentation, host-tool, packaging, and product-evidence boundary

**Owner:** Engine/Platform for window/input/process integration, with Build/Packaging, Application, RHI Vulkan, Tools, and product targets as required contributors

**Snapshot:** 2026-09-07; Platform/Application/CMake host surfaces expose Win32 types/backends and Windows libraries, while no Linux/X11/Wayland owner or product route was found; source evidence `S` only

**Strategy sources:** [`PGE-14`](../../../../Strategy/Requirements.md), [Gap Assessment](../../../../Strategy/Assessments/GapAssessment.md), and [First Release](../../../../Acceptance/FirstRelease.md)

**Current readiness:** **0/100** — target only; no native Linux configure/build/run/package product path was found. See [Current Feature Readiness](../../../../Acceptance/CurrentReadiness.md#explicit-missing-or-not-yet-admitted-capabilities).

## At A Glance

| Required Linux surface | Current state | Why Vulkan-on-Windows is insufficient |
| --- | --- | --- |
| configure/build/toolchain | Not found | compiler flag branches do not provide Linux source ownership or product targets |
| window/input/application host | Not found | current public Platform contracts expose Win32/`HWND` behavior |
| Vulkan surface/presentation | Not found as a Linux product path | backend GPU commands do not create a Linux window/surface lifecycle |
| host tools/content pipeline | Not found as supported Linux products | tool process, filesystem, SDK, and dependency behavior remain Windows-oriented |
| stage/package/clean-machine run | Not found | there is no general release package on Windows or Linux |

Linux support is an end-to-end product matrix, not a preprocessor symbol. It becomes a feature only when a Linux user can obtain, configure, build or install, launch, render, interact, diagnose, and exit through documented owners.

## Capability Identity

| ID | Capability | Current state |
| --- | --- | --- |
| `PLAT-LINUX-01` | Native Linux build/toolchain and product membership | Not found |
| `PLAT-LINUX-02` | Linux window, event, input, focus, DPI, and process integration | Not found |
| `PLAT-LINUX-03` | Linux Vulkan surface, presentation, lifecycle, diagnostics, and capture | Not found |
| `PLAT-LINUX-04` | Linux package/install and standard-user operation | Not found |

## Current Boundary

Sparkle's present platform layer exposes Win32 types and behavior, links Windows libraries, and builds D3D12/Vulkan as graphics backends on a Windows host. Vulkan support does not imply Linux support. No Linux window/input backend, host toolchain/profile, product target, package/install route, runtime dependency staging, or native execution evidence was found.

Linux is therefore **unsupported/absent**, not Experimental. The first-release authority may keep it Excluded; this dossier exists so the roadmap target and exclusion are explicit rather than undocumented.

## Target Capability

- Platform: opaque native-window boundary, event/message loop, DPI/scale, keyboard/mouse/focus/capture/cursor, timing, filesystem/process/environment integration, and shutdown.
- Build and tools: supported compiler/SDK/dependency discovery, host tools, generated-content route, runtime library resolution, symbols/diagnostics, and isolated artifacts.
- Graphics: Vulkan surface/swapchain/presentation, capability truth, resize/out-of-date/device-loss handling, validation, capture, and feature exclusions.
- Product delivery: standard-user paths/permissions, package/install/uninstall, case sensitivity, Unicode/long paths, clean-machine prerequisites, support identity, and redistribution.

## Acceptance Handoff

- `AC-LINUX-01`: supported distribution/version, CPU/GPU/driver/window-system/compiler matrix and excluded configurations are frozen.
- `AC-LINUX-02`: clean configure/build/cook and runtime/editor/tool membership match the declared product without Windows headers, libraries, paths, or binaries.
- `AC-LINUX-03`: window/input/focus/DPI/resize/minimize/fullscreen/quit and Vulkan present/device-loss paths have native evidence.
- `AC-LINUX-04`: packaged standard-user first run, repeat run, missing dependency/content, diagnostics/capture, and uninstall are reproducible.
- `FM-LINUX-01`: unsupported host/toolchain/window system/driver -> configure or startup fails with an exact support reason.
- `FM-LINUX-02`: runtime library, permission, case-sensitive path, display, surface, or device is unavailable -> bounded failure without partial false success.
- `CHK-LINUX-01`: run clean build/product/package gates on the frozen native Linux matrix and inspect binaries/dependencies for Windows leakage.
- `CHK-LINUX-02`: exercise platform and Vulkan lifecycle cases with native validation and retained diagnostics.

No Linux build or runtime check was run and no criterion is passed by this dossier. [`PLAT-E03`](../../../../Plans/CapabilityEvidence.md#foundation-and-host-evidence) owns the negative-capability audit until the Roadmap admits implementation.
