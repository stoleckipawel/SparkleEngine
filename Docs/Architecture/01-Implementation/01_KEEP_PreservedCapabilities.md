# 01. KEEP - Preserved Capabilities And Direction

Status: implementation guardrail
Date: 2026-07-04
Source set: `Docs/Architecture/00-Review`
Use this as: the "do not break this" prompt before every refactor batch

## Purpose

This document gathers the capabilities, architecture instincts, and personal engineering direction that must be preserved while SparkleEngine is made smaller and sharper.

The goal is not to freeze the engine. The goal is to prevent cleanup from deleting the parts that prove advanced graphics engineering depth:

- explicit D3D12/Vulkan ownership and parity
- clear Core, RHI, Renderer, GameFramework, and Tools separation
- thin high-level engine concepts that hide low-level implementation details only where that improves reasoning
- renderer feature depth
- shader/cook/runtime ABI discipline
- professional graphics debugging and capture support
- classic TLAS and PTLAS as real ray tracing capabilities
- multiple levels and projects without depot pollution
- neural rendering readiness without runtime ML bloat
- a compact renderer-first product identity

If a change makes the repo smaller but removes one of these preserved capabilities, it is the wrong change. Harden, narrow, or move ownership instead.

## Product Identity To Preserve

Sparkle should continue moving toward this identity:

> A compact renderer-first engine focused on realtime/path-traced rendering, explicit D3D12/Vulkan RHI ownership, shader pipeline quality, cooked content workflows, and advanced graphics feature depth.

The engine should be easy to reason about at two levels:

- high-level user concepts: project, level, scene, camera, material, mesh, light, render path, backend, capture, cook, package
- low-level implementation concepts: command lists, descriptors, resources, barriers, queues, shader packages, acceleration structures, backend-native handles

The high-level layer should be thin. It should protect users from unnecessary implementation details, but it must not add so many wrappers that rendering behavior becomes hard to trace.

Keep this identity clear:

- Sparkle is not a generic app framework.
- Sparkle is not a validation lab.
- Sparkle is not a pile of samples.
- Sparkle is not a broad ML runtime.
- Sparkle is not a launcher product with a renderer attached.
- Sparkle is a renderer-first engine with enough runtime, editor, content, and tools to prove real graphics features.

Implementation prompt:

1. Before editing a subsystem, state which part of this identity it serves.
2. If the subsystem does not serve the identity, move to `04_REMOVE_DeletionsAndCleanup.md`.
3. If it serves the identity but is bloated, move to `02_MODIFY_RefactorExistingSystems.md`.
4. If a missing capability is required, move to `03_ADD_MinimalMissingCapabilities.md`.

## Module Ownership Contract To Preserve

Keep the engine split around ownership, not around convenience wrappers. A batch should move code toward the owning area instead of adding another forwarding layer.

| Area | Owns | Does Not Own |
| --- | --- | --- |
| Core | foundation utilities, math, files, config, fatal checks, small platform-neutral helpers | renderer policy, backend resources, workflow UI, content schemas beyond common utilities |
| RHI | explicit D3D12/Vulkan resources, descriptors, pipelines, queues, command lists, barriers, uploads/readbacks, ray tracing, native interop, capture, presentation | scene extraction, material policy, frame scheduling, editor panels, generic wrapper layers |
| Renderer | frame graph, passes, render paths, scene extraction, shader registrations, provider boundaries, TLAS/PTLAS policy, render products | backend-private API details, level ownership, launcher/cooker workflows |
| GameFramework | project runtime concepts, levels, scenes, components, cameras, materials, meshes, lights, cooked scene loading | renderer-private implementation details, RHI resources, shader package compilation |
| Tools | shader compile, import, cook, launch, clean, package-if-owned, asset workflow entrypoints | runtime render ownership, diagnostic cockpit behavior, unowned package products |
| Projects | sample projects, selectable levels, scene/content data, optional heavy content packs | engine contracts, backend-specific behavior, required heavy content in the default repo footprint |

## Persona Direction To Preserve

The engine should develop and demonstrate the advanced graphics engineer persona described in `H_AdvancedGraphicsEngineerPersona.md`.

Preserve work that strengthens:

- explicit graphics API ownership
- renderer feature depth
- shader and kernel craft
- GPU architecture thinking
- neural rendering readiness
- debugging and tool fluency
- product engineering discipline

Do not preserve work that only makes the repo look busy:

- more logs
- more validation systems
- more diagnostic panels
- more report formats
- more wrappers
- more future scaffolding
- more docs instead of code cleanup

The persona is proven through code shape:

- less public surface
- more direct ownership
- fewer default artifacts
- preserved real capabilities
- cleaner implementation paths

## Non-Negotiable Keep List

Keep these unless the user explicitly changes product direction:

| Capability | Why it stays | Refactor rule |
| --- | --- | --- |
| D3D12 backend | Explicit graphics API depth and real platform coverage. | Keep backend-native ownership private; preserve or improve parity with Vulkan. |
| Vulkan backend | Explicit graphics API depth and cross-backend design discipline. | Preserve or improve parity for RHI, ray tracing, PTLAS where supported, and presentation. |
| RHI service model | Strong low-level engine boundary. | Do not replace with a new abstraction; narrow public services if needed. |
| Renderer frame graph | Core render scheduling abstraction. | Keep as the one graph; simplify pass/resource ownership inside it. |
| Core/RHI/Renderer/GameFramework/Tools/Projects separation | Keeps the engine understandable. | Move behavior to the owning layer instead of adding cross-layer wrappers. |
| Shader compiler/cook/runtime ABI | One of the strongest product systems. | Remove debug artifacts, but preserve source-to-package-to-runtime linkage. |
| HLSL/Slang readiness | Enables modern shader work and neural rendering readiness. | Keep flexible without adding runtime ML frameworks. |
| Classic TLAS | Core ray tracing path. | Preserve build/update/trace behavior. |
| PTLAS | Product ray tracing capability equal in importance to classic TLAS. | Minimize to reference-style functional flow; do not delete capability. |
| Screenshot/BMP capture | Important editor/tool capability. | Harden and narrow ownership; delete only smoke/ad hoc coupling. |
| PIX/RenderDoc/Nsight support | Professional graphics debugging evidence. | Preserve markers, object names, timestamps, and debug-layer support. |
| Multi-level project support | Engine capability and user workflow. | Use catalogs/manifests to avoid clutter; do not reduce to one level. |
| Cookers/importers | Source-to-cooked content pipeline. | Emit product assets and fatal errors; remove default reports. |
| Minimal launcher workflows | Build/cook/run/clean/package if owned. | Keep workflow shell; delete diagnostic cockpit behavior. |
| Provider boundaries | Upscaling/ray reconstruction integration discipline. | Keep narrow; avoid provider diagnostics and fallback scaffolding. |

## Architecture Surfaces To Keep

### RHI

Keep:

- `RenderHardwareInterface` as the low-level service locator.
- resource, descriptor, pipeline, upload, ray tracing, interop, capture, diagnostics, and presentation services.
- D3D12MA and VMA memory allocator integrations.
- backend-private native API usage.
- native handles only for explicit provider bridges.
- capability queries for backend and feature selection.

Modify only to:

- reduce broad public diagnostics
- narrow capture ownership
- preserve D3D12/Vulkan parity
- remove wrappers that do not delete code

Do not:

- replace the RHI with a new abstraction
- hide explicit API concepts under generic wrappers
- remove the capture capability
- remove PTLAS services
- make frame graph depend on backend-private details

### Renderer

Keep:

- frame graph, transient resources, barriers, typed pass parameters
- deferred path, GBuffer, lighting, reservoir direct lighting, sky, composite
- reference path tracing, but with a clear role
- ray tracing scene ownership
- classic TLAS and PTLAS strategy selection
- upscaling and ray reconstruction provider boundaries
- shader registrations and package metadata

Modify only to:

- make passes own clearer inputs/outputs
- delete debug views and unused diagnostics
- clarify reference mode role
- reduce public snapshots
- minimize PTLAS scaffolding

Do not:

- add a replacement render graph
- add broad feature wrappers
- remove product RT paths
- add panels to make unfinished work look real

### Shader Pipeline

Keep:

- HLSL shader libraries under BRDF, Common, Geometry, Lighting, Material, RayTracing, Resources, Display, and Debug when used
- pass shaders under `Engine/Assets/Shaders/Passes`
- C++ shader registrations
- DXC and Slang backends
- reflection and layout safety
- cooked shader packages
- runtime shader package cache
- package inspection commands if used

Modify only to:

- remove default debug bundles
- remove stats CSV from normal workflows
- delete demo shaders that do not serve renderer features
- reduce duplicate declarations only when the replacement deletes materially more code than it adds

Do not:

- add generated code unless it removes more binding code than it introduces
- add shader demos for appearance
- weaken runtime ABI checks

### Tools And Workflow

Keep:

- shader compiler
- cookers needed to build runtime assets
- importers needed by cookers
- launcher workflows for build, cook, run, clean, package if package is product-owned
- source dependency sync only if actively used

Modify only to:

- remove diagnostic pages
- remove default reports
- remove shader debug artifact toggles from normal UI
- shrink launcher to workflows

Do not:

- make the launcher the place for every local workflow
- keep package assembly that is not product-owned
- keep cook reports by default

## Current Strengths To Protect

From the review set, Sparkle already scores well in several areas:

| Area | Current strength | Preserve by |
| --- | --- | --- |
| Module layering | Renderer consumes GameFramework privately; backend details stay private. | Keep include direction and boundary checks. |
| RHI explicitness | D3D12/Vulkan concepts remain recognizable. | Avoid hiding API reality behind wrappers. |
| Renderer frame architecture | Real frame graph and pass system. | Simplify, do not replace. |
| Shader pipeline | Compiler, reflection, contracts, cache, cook, inspection. | Keep ABI, trim artifacts. |
| Provider integration | Upscaling and ray reconstruction are separated. | Keep bridge narrow. |
| Deletion readiness | Cleanup targets are identifiable. | Prefer net-negative changes. |

## Implementation Guardrail Checklist

Use this checklist before applying a patch:

- [ ] Does the change preserve or improve D3D12/Vulkan parity unless explicitly backend-specific?
- [ ] Does it preserve offline cooked shader packages with reflection data, or update all source/cook/runtime pieces together?
- [ ] Does it preserve classic TLAS and PTLAS where relevant?
- [ ] Does it preserve screenshot/BMP capture where relevant?
- [ ] Does it preserve PIX/RenderDoc/Nsight markers and object names?
- [ ] Does it preserve multi-level support?
- [ ] Does it reduce or avoid public API growth?
- [ ] Does it avoid new docs/logs/validation/report systems/wrappers/scaffolding?
- [ ] Does it delete or simplify nearby code?
- [ ] Is any build/cook/run risk recorded for the final Stage 14 stabilization pass?

## Done State

The keep contract is satisfied when Sparkle can become smaller without becoming less capable:

- D3D12/Vulkan remain first-class and parity is preserved or extended.
- RHI remains explicit.
- Frame graph remains the scheduler.
- Shader pipeline remains a centerpiece.
- Classic TLAS and PTLAS both remain usable.
- Screenshot/BMP capture remains hardened.
- Multiple levels remain supported.
- Debugger/profiler hooks remain strong.
- New work makes the engine more real, not broader.
