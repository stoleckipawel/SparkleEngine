# RHI And Renderer Documentation Coverage Precedent

Status: research; primary-source precedent and local coverage audit, not Sparkle architecture, implementation, or acceptance evidence

Research snapshot: 2026-09-07

Scope: what production-oriented engines and rendering frameworks document about their render hardware interfaces and renderers; how that precedent sharpens, but does not replace, Sparkle's existing capability-dossier pattern

## Question And Method

This study asks a narrower question than the [external repository comparison](RendererRepositories.md): **what must an engineer be able to learn from RHI and Renderer documentation before changing, integrating, debugging, or approving a rendering feature?**

The sources are first-party engine manuals, API guides, specifications, and vendor-maintained repositories. Their terminology and abstraction levels differ, so the result is a coverage model rather than a copied table of contents. A precedent becomes a Sparkle requirement only when the local Architecture owner adopts it and source or executable configuration establishes the current local state.

## Primary Sources Reviewed

| Source | Documentation emphasis relevant to this study |
| --- | --- |
| [Unreal graphics programming overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/graphics-programming-overview-for-unreal-engine) | separates scene/view/high-level Renderer concepts from the low-level cross-platform RHI and names feature-level/platform variation |
| [Unreal Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine) | documents pass declarations, shader parameters, setup versus execute, dependency validation, transient lifetime/aliasing, barriers, async fences, parallel recording, culling, and graph visualization |
| [Unreal parallel rendering overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/parallel-rendering-overview-for-unreal-engine) | documents game/render/RHI thread roles, command-list/context ownership, frame lag, and synchronization points |
| [Godot internal rendering architecture](https://docs.godotengine.org/en/stable/engine_details/architecture/internal_rendering_architecture.html) | states design philosophy, rendering-method and driver/API combinations, tradeoffs, central classes, and shader organization |
| [Godot RenderingDevice](https://docs.godotengine.org/en/stable/classes/class_renderingdevice.html) | distinguishes the low-level device abstraction from the higher-level rendering server and exposes resource/pipeline/command primitives |
| [O3DE Atom RHI](https://docs.o3de.org/docs/atom-guide/dev-guide/rhi/rhi/) | documents backend abstraction, platform optimization, frame scheduling, multithreaded command generation, transient memory, async queues, resource tracking, and validation |
| [O3DE frame rendering](https://docs.o3de.org/docs/atom-guide/dev-guide/frame-rendering/) | traces render component to feature processor, draw item/packet, view/draw list, pass, and RHI, including culling, filtering, and sorting |
| [O3DE pass system](https://docs.o3de.org/docs/atom-guide/dev-guide/passes/pass-system/) | documents pass inputs/outputs, validation, build/frame lifecycle, and the extension route for new passes |
| [bgfx internals](https://bkaradzic.github.io/bgfx/internals.html) | documents API-thread/render-thread handoff, double-buffered frames, resource/view/encoder APIs, draw sorting, concurrency rules, and fixed limits |
| [NVRHI programming guide](https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/ProgrammingGuide.md) | documents device/command-list separation, handle lifetime, state tracking, barriers, synchronization, binding layouts/sets, bindless limits, immutable pipelines, and uploads |
| [NRI](https://github.com/NVIDIA-RTX/NRI) | makes backend/build/prerequisite and extension coverage explicit while preserving a deliberately low-level, validation-capable interface |
| [Direct3D 12 programming model changes](https://learn.microsoft.com/en-us/windows/win32/direct3d12/important-changes-from-directx-11-to-directx-12) | explains explicit synchronization, residency, pipeline-state objects, command lists, and descriptor heaps |
| [Direct3D 12 residency](https://learn.microsoft.com/en-us/windows/win32/direct3d12/residency) and [resource barriers](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12) | establish memory-budget, pressure, transition, UAV, and aliasing responsibilities that an RHI contract must locate |
| [Vulkan specification](https://registry.khronos.org/vulkan/specs/latest/pdf/vkspec.pdf) and [swapchain recreation tutorial](https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/04_Swap_chain_recreation.html) | make application synchronization and out-of-date/suboptimal presentation-resource recreation explicit |

## Recurrent Documentation Model

The useful commonality is not a universal class hierarchy. Mature documentation repeatedly answers the following questions.

### RHI Coverage

| Area | What the documentation must make explicit | Typical hidden failure when omitted |
| --- | --- | --- |
| purpose and non-goals | abstraction level, policy deliberately hidden or exposed, and where Renderer automation begins | two owners independently track barriers, lifetime, or fallback |
| build/backend matrix | compiled targets, prerequisites, default/requested selection, minimum API/device features, and deliberate backend asymmetry | source presence is advertised as runtime support |
| device lifecycle | owner thread, ordered initialization/publication, partial-create failure, steady state, idle/settlement, destruction, device removal, and recovery or terminal behavior | use-before-publication, teardown races, or a diagnostic path mistaken for recovery |
| capabilities and limits | queried device truth, queue topology, formats/usages, sample counts, alignment, descriptor and ray limits | backend name or enum vocabulary substitutes for supported behavior |
| resources and memory | resource/view descriptions, allocation kinds, upload/readback, state, aliasing, budgets/pressure, lifetime and completion-safe reclamation | C++ lifetime is confused with GPU lifetime or memory pressure has no owner |
| binding and pipelines | layout identity, arrays/bindless bounds, shader ABI/reflection, immutable pipeline identity, caching, and invalid combinations | stale or incomplete descriptors materialize as native state |
| commands and synchronization | recording state machine, queue legality, barriers, waits, submissions, completion tokens, cancellation/stall semantics | false completion, circular waits, or backend-dependent ordering |
| presentation | acquire, back-buffer state, frames in flight, resize/minimize/out-of-date handling, pacing, VSync/HDR support | a swapchain rebuild is confused with device recovery |
| diagnostics and interop | object identity, validation/crash evidence, timestamps, capture, native escape hatches, observer cost and availability | a native pointer leaks into general policy or failure is unattributable |
| extension procedure | how to add a resource use, pipeline/command, backend, optional extension, test, and documentation row | one backend or inventory silently lags a public contract |

### Renderer Coverage

| Area | What the documentation must make explicit | Typical hidden failure when omitted |
| --- | --- | --- |
| frame ownership | request/admission, scene/view/frame identities, thread handoff, graph generation, publication, completion, and shutdown | stale view/scene/provider state crosses frames |
| scene and view preparation | persistent scene updates, camera/projection, visibility, LOD/occlusion policy, draw classification, sorting, batching, and multi-view/stereo boundary | a visible object disappears, transparent order changes, or one view borrows another's state |
| pass graph | pass inputs/outputs, order, conditions, queue choice, transient/history resources, failure, diagnostics, and extension seam | feature order or lifetime exists only in code folklore |
| geometry/material contract | supported geometry/deformation/alpha/topology and exact material/GBuffer semantics | "PBR" or "transparent" overstates a partial path |
| lighting and ray traversal | lights, estimators/BRDFs, visibility, traversal variants, histories, limits, fallbacks, and semantic parity | one traversal or lighting mode lends claims to another |
| resolution and sampling | output versus render extent, viewport/scissor, temporal jitter, AA method, upscale/reconstruction combination rules, resize/history invalidation, and sample-count resolve path | provider quality vocabulary is mistaken for a generic AA feature |
| post and presentation | exposure, reconstruction, tone/color pipeline, debug-domain behavior, encoding, target publication, and UI order | double transform, wrong debug values, or ambiguous capture provenance |
| selectors and active state | public/settings/CVar request, default, clamp, persistence, capability resolution, fallback, active result, and restart needs | a requested mode is reported as active after fallback |
| observability and proof | products, markers, captures, failure reason, quality/performance/cost matrix, acceptance criteria and invalidating evidence | a plausible screenshot becomes the only oracle |
| extension procedure | how to add a feature/pass/shader/selector/product and update both backends/evidence routes | orphaned pass, shader, selector, or documentation |

## Vertical And Horizontal Review Rule

A complete feature description needs both dimensions:

```text
request / default
  -> capability and active-state resolution
  -> scene/view preparation
  -> pass and resource declaration
  -> shader/pipeline materialization
  -> RHI recording, barriers, submit and completion
  -> presentation/product/diagnostic publication
  -> failure, fallback, recovery or retirement
```

Horizontally, the same description must distinguish every relevant backend, queue, viewport/view kind, geometry/material class, selector state, extent/sample mode, provider/traversal route, lifecycle transition, and failure injection. A pass in one cell cannot silently approve unlike cells.

This directly supports Sparkle's existing dossier contract: identity and motivation, requested versus active state, owner/lifetime, capacity, algorithm, backend matrix, failure/recovery, observability, `AC-*`, `FM-*`, `CHK-*`, evidence destination, and explicit non-claims.

## Sparkle Coverage Reconciliation

The 2026-09-07 source/doc audit found strong existing ownership for backend selection, resources, descriptors, pipelines/shaders, commands, ray tracing, presentation, diagnostics/capture/interop, frame graph, scene/view state, material/GBuffer, lighting, temporal history, post processing, selectors, products, and evidence. Three areas needed a sharper owner or deeper contract:

| Gap | Source-backed current state | Documentation action |
| --- | --- | --- |
| device lifecycle and failure recovery | `RenderDeviceServices` composes one owner-thread backend service, backends settle queues before destruction, swapchain resize is recoverable, D3D12 DRED/Vulkan results expose device loss, but no in-process device recreation coordinator was found | add an RHI lifecycle/recovery dossier and capability/evidence rows; distinguish swapchain recreation, orderly shutdown, fatal device loss, and future recovery |
| visibility and draw preparation | view preparation performs parallel frustum/AABB classification; batch preparation validates identities, preserves compatible authored groups, sorts/batches opaque work, and keeps transparent items as distance-sorted singles | add a dedicated Renderer dossier so culling, classification, sort/batch semantics, limits, failures, and absent occlusion/LOD/GPU-driven routes have stable criteria |
| resolution and anti-aliasing | output extent comes from the viewport/window; providers resolve render extent; active temporal sampling is Halton; Linear or capability-gated DLSS/RR produces output extent; RHI sample-count vocabulary is not an active Renderer MSAA route | add a resolution/sampling dossier and negative capability rows for MSAA/standalone post AA/dynamic resolution so `NativeAA` and dormant jitter/sample vocabulary cannot overstate support |

The resource/memory dossier also required deeper object, description, transfer, allocation, state, aliasing, and retirement matrices. Existing owners remain authoritative; this research does not create another architecture layer.

## Maintenance Rule

When RHI or Renderer gains a public operation, selector, source owner, shader/pass, backend lowering, output product, or externally recognizable feature:

1. update the exact capability row and owning dossier;
2. trace the complete producer/consumer/lifetime path;
3. enumerate unlike horizontal cells and explicit non-claims;
4. add `AC-*`, controlled `FM-*`, claim-falsifying `CHK-*`, and an evidence destination;
5. update source-owner/public-surface routing and the candidate completion report when the result is release-relevant;
6. leave the state unproved until executable evidence exists.

The implementation procedure remains owned by the [Engineering task map](../../Engineering/README.md#choose-by-task), not by this research note.
