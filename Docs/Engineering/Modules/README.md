# Module Engineering

**Status:** module engineering index

These standards add rules for a particular implementation domain. Read the document for every affected domain; a cross-module change may require more than one.

## Cross-Module Selection

| Change crosses... | Minimum module rule set |
| --- | --- |
| world/ECS -> immutable render publication | GameFramework + Renderer |
| Renderer feature -> native GPU command/resource | Renderer + RHI |
| task graph -> owning subsystem publication | Tasks + the producing module |
| editor intent -> world/settings/render result | Editor + affected GameFramework/Renderer owner |
| source asset -> cooked/runtime product | Tools + consuming GameFramework/Renderer/RHI owner |

Module rules compose; selecting the primary owner does not waive the rules of another boundary the data or lifetime actually crosses.

## Domain Rules

| Domain | Source boundaries commonly covered | Read it when... |
| --- | --- | --- |
| [Tasks](Tasks.md) | `Engine/Tasks` and concurrent producers/consumers | changing task graphs, lanes, synchronization, cancellation, failure propagation, or shutdown |
| [GameFramework](GameFramework.md) | `Engine/GameFramework` and world/ECS consumers | changing worlds, levels, components, systems, editing, or render publication |
| [Renderer](Renderer.md) | `Engine/Renderer` and renderer-owned shaders | changing scene/view/frame policy, frame graphs, render products, shader interfaces, or graphics pipelines |
| [RHI](RHI.md) | `Engine/RHI`, D3D12, and Vulkan | changing neutral GPU contracts, backend lowering, native resources, synchronization, validation, or driver handling |
| [Editor](Editor.md) | `Engine/Editor` | changing editor state, panels, UI/render publication, capture UX, or editor background operations |
| [Tools](Tools.md) | `Tools` | changing import, cooking, shader compilation, Launcher, CLI workflows, or artifact publication |

Current module ownership and capability maps live under [Architecture Modules](../../Architecture/Modules/README.md). These files own engineering rules, not current capability claims.
