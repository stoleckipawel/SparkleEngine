# Renderer Engineering

**Status:** binding Renderer integration standard

**Applies to:** `Engine/Renderer`, renderer-owned shaders, persistent GPU-scene policy, frame metadata, frame graphs, render products, path/neural rendering, captures, and graphics evidence

This standard owns Renderer change guardrails. The canonical [Renderer and RHI Architecture Boundary](../../Architecture/Decisions/RendererRhiBoundary.md) owns the dependency, responsibility, lifetime, backend-parity, and enforcement design. [RHI Engineering](RHI.md) owns neutral GPU contracts, backend lowering, native validation, and driver-facing rules. The [World Coordinate, Units, and Transform Contract](../../Architecture/Decisions/WorldCoordinateAndUnits.md) owns world, matrix, camera/light, animation, and skinning semantics.

## Persistent GPU Data

Prefer persistent indexed state plus dirty ranges over full scene rebuild/upload.

- Static assets cross through immutable handles and residency.
- Dynamic transforms, lights, skinning, morph, visibility, and temporal values update required ranges only.
- Resolve stable IDs to renderer slots before hot traversal.
- Coalesce dirty ranges deliberately.
- Capacity growth publishes replacement storage at a frame boundary and retires the old allocation by GPU token.
- Measure upload bytes, descriptors, memory, resource churn, RT build/update time, and queue behavior.

## Frame Metadata

- `FrameId` is the shared correlation identity.
- Temporal discontinuity, camera cut, teleport, history reset, resolution, exposure, provider tags, motion/depth conventions cross only when consumers need them.
- Jitter is Renderer-derived from `FrameId` and settings; it is not Application-owned mutable state.
- Content is regenerated to the newest representation rather than protected by compatibility versions.
- Stable-handle generations and GPU tokens remain because they prove lifetime, not content version.

## Required Render Products

- Every required graph product has exactly one scheduled semantic producer before any consumer can execute. Frame/feature orchestration schedules that producer and does not duplicate shader, acceleration-structure, descriptor, pipeline, or backend capability checks owned below it.
- Never add a clear, copy, no-op, dummy resource, default value, or stale-history pass merely to mark a required product as produced. Invalid handles, null checks, graph production state, native validation, and assertions must expose the missing producer rather than conceal it.
- A supported alternate renderer path must produce the same declared product contract, be selected explicitly by the owning Renderer policy, and carry its own correctness evidence. A deferred implementation is not a fallback.
- Shadow visibility consumed by direct lighting is mandatory. Schedule the selected real producer without a high-level capability guard; shader/runtime/RHI owners expose an unavailable implementation through their normal materialization or execution failure contract.

## Frame-Graph Composition

- `FramePipeline::BuildRenderFrameGraph` is the existing composition root. It reads renderer policy at the branch that consumes it and directly schedules exactly one concrete middle-frame implementation; it does not manufacture a recipe object, registry, factory dependency bag, callback, or selector hierarchy.
- The selected feature owns its passes, resources, state, and local policy. The composition root owns only shared resource creation, shared scene publication, the one mutually exclusive Lit-versus-alternate branch, shared downstream processing, and the common product contract.
- `RenderViewMode` is the one host-independent per-view rendering choice. It names mutually exclusive rendered views such as Lit, Wireframe, a focused buffer view, or Reference Path Tracer; `ViewportRenderRequest` carries it and `RenderView` freezes it. Editor owns labels, icons, menu grouping, shortcuts, and widget state, but does not mirror or translate this rendering semantic through an Editor enum.
- Renderer owns the meaning and execution of each `RenderViewMode`: canonical scene/view data, graph composition, passes, resources, shader parameters, feature lifecycle, and generic products/progress. A mode is not a settings bag and must not accumulate unrelated toggles.
- A mode has one rendering meaning and one declared primary consumer. Reference Path Tracer selects the alternate middle, Wireframe selects raster fill, and buffer/lobe/instance modes select the debug resolve. Do not re-encode any of these as a CVar, show flag, target enum, Editor enum, graph setting, feature setting, or RHI field. Passes and shaders receive the mode only where they select among the products that pass owns.
- Add lower-level per-view show flags only when independently selectable contributions have real production consumers. They are resolved rendering state below the selected mode, never a second mode selector, and must not be introduced merely to imitate another engine's type hierarchy.
- Read CVar-controlled implementation policy at the narrow composition/pass owner that consumes it. Retain a resolved value only when it is required for graph topology identity, extent, thread handoff, or another named lifetime boundary; never copy it through request/settings/context layers merely to reach the decision.
- Frontend code never uses process-global CVars as the normal transport for a per-view mode. A genuinely global Renderer CVar action still crosses the sequenced control boundary and is applied on the Renderer owner thread. This is not permission to introduce a generic settings bag or duplicate per-view/global selector.
- A frame-graph helper or factory must not accumulate concrete feature objects. When the existing `FramePipeline` already owns a feature's lifetime, its graph-composition member invokes that feature directly instead of threading it through lower-level constructors or function parameters.

## Shader Parameter Identity

- A shader parameter has one authoritative name. The C++ parameter member, generated graph/layout metadata, reflected HLSL binding, cooked binding record, and runtime lookup all use that exact name.
- Parameter macros take a resource or value type plus that one member/binding name. Constant-buffer declarations use `SHADER_PARAMETER_CBUFFER(Type, Name)`; the HLSL cbuffer or object is also `Name`.
- Do not add `_NAMED` parameter macros, layout-name/shader-name pairs, metadata aliases, reflection fallback searches, or compatibility spellings. Resolve a mismatch by renaming the authored C++ member and HLSL binding together.
- Shader type/program identity remains separate from parameter identity. A shader registration name may identify code; it must not become a second name for one parameter field.

## Graphics Pipeline Policy

- Feature and graph setup code states only semantic raster intent that cannot be derived: blend policy, depth/stencil policy, and a genuinely dynamic override. It must not author a complete backend pipeline descriptor.
- Declared frame-graph color/depth attachments are the authority for render-target count, formats, depth/stencil format, sample compatibility, and load/store/clear behavior. Do not copy those facts into a pass-owned pipeline-state record.
- Mesh/geometry ownership supplies an actual stable vertex-input declaration and primitive topology. Material/pass policy supplies fill, cull, blend, and depth/stencil choices. Shader lookup supplies concrete stages and binding layout. Viewport, scissor, blend constants, and stencil reference remain dynamic command state where supported.
- The Renderer pipeline owner combines those inputs into one complete immutable pipeline key and asks RHI to materialize the complete neutral descriptor. Backend-private code lowers that descriptor; it does not invent feature defaults or ignore an exposed state.
- Required pipeline variants are materialized lazily from exact requested state before recording and retained by the owning shader/runtime generation. Do not eagerly create speculative variants.
- A shader pair is not a complete pipeline identity. Cache/reuse identity includes shader code, binding layout, vertex input, topology, fixed-function state, attachment compatibility, and the owning generation.
- Do not add a frontend `GraphicsPipelineState`, `PSODesc`, generic state bag, backend descriptor, or duplicated attachment signature. Narrow render-state values and internal materialization keys may exist only at their actual owner/lifetime boundaries.

## CPU And GPU Concurrency

Always distinguish CPU task concurrency, render-thread pipelining, command recording concurrency, GPU graphics/compute/copy concurrency, frames in flight, provider execution, and input-to-present latency.

Parallel CPU recording does not prove GPU overlap. Add GPU queue concurrency only when correlated timelines show useful overlap after synchronization and bandwidth cost.

## Path Tracing And Neural Kernels

- State coordinate spaces, units, radiometric meaning, PDFs/weights, precision, accumulation/history, and numerical limits at the owning math/shader contract.
- Connect important equations to executable known-value reference tests; citing a paper is not a correctness test.
- Neural preprocessing, operators/kernels, and postprocessing are explicit passes or cohesive shader operations with declared resources.
- Expose bounds, tensor/image layout, channels/tiles, precision, and fallback capability; do not hide them behind opaque macros or a generic operator dispatcher.
- Select fusion, tiling, wave/cooperative operations, shared memory, precision, and dispatch size from captures plus quality tests.
- Inspect DXIL/SPIR-V, reflection, layouts, disassembly, and counters when they answer the performance question.
- Report quality, latency, memory, pacing, and temporal behavior together.
- Preserve deterministic classical/reference paths for comparison and fallback.

## Renderer Review Questions

- Does the change preserve the Renderer side of every applicable dependency, ownership, frame-graph, lifetime, recording, parity, and enforcement rule?
- Does every required render product have one real producer, with missing capability rejected before scheduling rather than hidden by fabricated output?
- Does the frame composition contain one readable branch on the immutable per-view `RenderViewMode`, with feature mechanism enclosed and no recipe hierarchy, dependency bag, parallel target/flag taxonomy, global CVar mirror, or RHI leakage?
- Can Editor be removed while the same Renderer feature remains selectable by its own control, and can Renderer be removed without leaving UI taxonomy in RHI or Application contracts?
- Does every shader parameter have one exact C++/metadata/HLSL binding name with no alias or reflection fallback?
- Does graphics setup state only non-derivable raster intent while attachments, geometry, shaders, and the pipeline owner supply their own facts exactly once?
- Is every materialized graphics pipeline keyed by the complete state and created only when an actual draw requests it?
- Are persistent data and dirty ranges used where justified?
- Do graphics claims name the exact workload, backend, hardware, driver, build, and evidence?
