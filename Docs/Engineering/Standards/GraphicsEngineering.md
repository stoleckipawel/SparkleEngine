# Graphics Engineering

Status: binding graphics integration standard

Applies to: persistent GPU data, frame metadata, shaders/kernels, path/neural rendering, captures, performance, and hardware/driver-facing work

This standard owns graphics-change guardrails and review questions. The canonical [Renderer and RHI Architecture Boundary](../../Architecture/RendererRhiBoundary.md) owns the dependency, responsibility, lifetime, backend-parity, and enforcement design. The [World Coordinate, Units, and Transform Contract](../../Architecture/WorldCoordinateAndUnits.md) owns world, matrix, camera/light, animation, and skinning semantics. Graphics work must preserve both rather than reproduce a competing boundary here.

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

## Shader Parameter Identity

- A shader parameter has one authoritative name. The C++ parameter member, generated graph/layout metadata, reflected HLSL binding, cooked binding record, and runtime lookup all use that exact name.
- Parameter macros take a resource or value type plus that one member/binding name. Constant-buffer declarations use `SHADER_PARAMETER_CBUFFER(Type, Name)`; the HLSL cbuffer or object is also `Name`.
- Do not add `_NAMED` parameter macros, layout-name/shader-name pairs, metadata aliases, reflection fallback searches, or compatibility spellings. Resolve a mismatch by renaming the authored C++ member and HLSL binding together.
- Shader type/program identity remains separate from parameter identity. A shader registration name may identify code; it must not become a second name for one parameter field.

## Graphics Pipeline State Ownership

- Feature and graph setup code states only semantic raster intent that cannot be derived: blend policy, depth/stencil policy, and a genuinely dynamic override. It must not author a complete backend pipeline descriptor.
- Declared frame-graph color/depth attachments are the authority for render-target count, formats, depth/stencil format, sample compatibility, and load/store/clear behavior. Do not copy those facts into a pass-owned pipeline-state record.
- Mesh/geometry ownership supplies an actual stable vertex-input declaration and primitive topology; do not replace that declaration with a one-value layout-kind switch. Material/pass policy supplies fill, cull, blend, and depth/stencil choices. Shader lookup supplies concrete stages and binding layout. Viewport, scissor, blend constants, and stencil reference remain dynamic command state where supported.
- The Renderer pipeline owner combines those inputs into one complete immutable pipeline key and asks RHI to materialize the complete neutral descriptor. Backend-private code lowers that descriptor to D3D12/Vulkan; it does not invent feature defaults or ignore an exposed state.
- Neutral pixel-format traits, capture-format/layout mapping, and graphics/compute descriptor validity are defined once by the public RHI contract. D3D12 and Vulkan invoke those authorities before native work; backend-local code retains only native enum/structure translation, resource/copy construction, device/API capability checks, and native failure handling. Do not duplicate the same neutral case list, byte layout, or descriptor predicate in each backend.
- Required pipeline variants are materialized lazily from exact requested state before recording and retained by the owning shader/runtime generation. Do not eagerly create wireframe, two-sided, or other variants merely because they may be useful, and do not disguise eager variant creation as precaching.
- A shader pair is not a complete pipeline identity. Cache/reuse identity includes shader code, binding layout, vertex input, topology, fixed-function state, attachment compatibility, and the owning generation.
- Do not add a frontend `GraphicsPipelineState`, `PSODesc`, generic state bag, backend descriptor, or duplicated attachment signature. Narrow render-state values and internal materialization keys may exist only at their actual owner/lifetime boundaries.

## CPU and GPU Concurrency

Always distinguish CPU task concurrency, render-thread pipelining, command recording concurrency, GPU graphics/compute/copy concurrency, frames in flight, provider execution, and input-to-present latency.

Parallel CPU recording does not prove GPU overlap. Add GPU queue concurrency only when correlated timelines show useful overlap after synchronization and bandwidth cost.

## Path Tracing and Neural Kernels

- State coordinate spaces, units, radiometric meaning, PDFs/weights, precision, accumulation/history, and numerical limits at the owning math/shader contract.
- Connect important equations to executable known-value reference tests; citing a paper is not a correctness test.
- Neural preprocessing, operators/kernels, and postprocessing are explicit passes or cohesive shader operations with declared resources.
- Expose bounds, tensor/image layout, channels/tiles, precision, and fallback capability; do not hide them behind opaque macros or a generic operator dispatcher.
- Select fusion, tiling, wave/cooperative operations, shared memory, precision, and dispatch size from captures plus quality tests.
- Inspect DXIL/SPIR-V, reflection, layouts, disassembly, and counters when they answer the performance question.
- Report quality, latency, memory, pacing, and temporal behavior together.
- Preserve deterministic classical/reference paths for comparison and fallback.

## Hardware and Driver-Facing Work

- Record vendor, adapter/architecture, device ID when available, driver, OS, API, compiler, and feature capability for driver-sensitive conclusions.
- Prove API validity and ownership with engine assertions and native validation first.
- Reduce a suspected driver issue to the smallest resource/pipeline/command/synchronization reproducer before external attribution.
- Backend workarounds live in the owning backend-private file, have an exact predicate, cite evidence, and state removal/retest conditions.
- Renderer-wide policy branches on neutral capability, not vendor or driver identity.
- A vendor fast path retains a correct neutral fallback and does not redefine the public feature contract.
- Future hardware remains a measured hypothesis behind existing capability seams until evidence exists.
- Linux support requires native configure/build/run, Vulkan validation, capture/debug, package, and shutdown evidence.

## Graphics Review Questions

- Does the change preserve every applicable dependency, ownership, frame-graph, lifetime, recording, parity, and enforcement rule in the canonical architecture boundary?
- Does every required render product have one real producer, with missing capability rejected before scheduling rather than hidden by fabricated output?
- Does every shader parameter have one exact C++/metadata/HLSL binding name with no alias or reflection fallback?
- Does graphics setup state only non-derivable raster intent while attachments, geometry, shaders, and the pipeline owner supply their own facts exactly once?
- Do neutral format/capture-layout/descriptor/state invariants have one RHI owner, with each backend limited to native translation and real capability differences rather than copied validation?
- Is every materialized graphics pipeline keyed by the complete state and created only when an actual draw requests it, with no eager speculative variants?
- Are persistent data and dirty ranges used where justified?
- Are D3D12 and Vulkan behavior and shader targets validated where supported?
- Do graphics claims name exact workload, hardware, driver, build, and evidence?
