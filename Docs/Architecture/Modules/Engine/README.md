# Engine Module Architecture

Status: Engine module index

This index mirrors the durable modules under `Engine`. Open the owning module first; use hyperlinks to follow producer/consumer relationships without relocating that knowledge into a mixed folder.

| Module | Owns | Module documentation |
| --- | --- | --- |
| Application | runtime/editor host composition, configuration, loop, startup, and shutdown | [Application](Application/README.md) |
| Assets | engine-authored shaders, defaults, environments, and fixtures | [Assets](Assets/README.md) |
| Core | common infrastructure, diagnostics, files, processes, serialization, math, input, and time | [Core](Core/README.md) |
| Editor | workspace, viewport sessions, editing UX, settings, and editor tool entry points | [Editor](Editor/README.md) |
| GameFramework | levels, worlds, ECS, editing, publication, and render extraction | [GameFramework](GameFramework/README.md) |
| Platform | Windows application, window, DPI, message, input, cursor, and capture integration | [Platform](Platform/README.md) |
| Renderer | scene/view/frame policy, frame graph, render passes, providers, and shader registrations | [Renderer](Renderer/README.md) |
| RHI | backend-neutral GPU contracts and D3D12/Vulkan implementations | [RHI](RHI/README.md) |
| Tasks | task graphs, lanes, parallel ranges, cancellation, events, failure, and shutdown | [Tasks](Tasks/README.md) |

Rules that apply while changing these modules live in [Engineering Modules](../../../Engineering/Modules/README.md). Cross-owner system designs live in [CrossModule](../../CrossModule/README.md).
