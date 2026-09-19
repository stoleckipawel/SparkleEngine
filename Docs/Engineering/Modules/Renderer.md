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

- `FramePipeline::BuildRenderFrameGraph` is the high-level frame composition root. It calls ordinary `Add...Passes` composition functions in frame order: shared resources, ray-tracing-scene publication, lighting, and downstream post-processing. `AddLightingPasses` owns the one mutually exclusive Lit-versus-Reference decision and directly invokes the selected feature's `Add...Passes` function; no class-shaped stage, recipe object, registry, factory dependency bag, callback, or selector hierarchy mediates graph construction.
- Feature graph construction follows the Unreal-style nested-pass pattern: a focused `Add...Passes` function declares everything unique to that feature, while its higher-level composition function places genuinely shared passes exactly once after or before the branch. Stateful feature lifetime may remain in a focused feature owner, but graph construction itself is expressed by functions rather than forwarding stage classes.
- High-level composition bodies read as rendering intent in execution order: add the selected renderer, add exposure, add upscaling, and add presentation. Their conditionals may select a real implementation or output policy, but they do not inspect handle validity, choose providers, allocate targets/history, assemble parameter records, map products, or repeat a child feature's enablement policy. Those mechanics belong to the narrow `Add...Pass`/`Add...Passes`, resource, or publication owner named by the orchestration call.
- Pass source files have one role. A singular pass file owns one GPU pass definition and its parameter binding; a cohesive `*PassDefinitions` file may own a tightly related implementation family; and a `*Passes` file only orders already-defined passes, selects a route, creates composition-level products, or publishes the result. Resource/target construction lives in an explicitly named `*Resources` or `*RenderTargets` file. Never combine shader dispatch implementation, route selection, unrelated post-processing, and topology policy in one generic feature file.
- File and function names state the rendered operation and composition level: `AddToneMappingPass`, `AddRealTimePathTracerPasses`, `LightingPasses.cpp`, and `GBufferRenderTargets.cpp` are representative. Avoid vague `Process`, `Execute`, `Utility`, or feature-only filenames when the unit actually owns a narrower pass, target, or orchestration responsibility.

### Frame-authoring vocabulary

Use the same verb and file shape at every composition depth:

| Shape | Meaning |
| --- | --- |
| `Build...Graph` | Assemble and return one complete graph-level product at the owning root. |
| `Add...Passes` in `*Passes.cpp` | Order semantic operations or select mutually exclusive implementations; do not encode a GPU dispatch. |
| `Add...Pass` in an operation-named file | Define one semantic graph pass and bind the parameters it owns. |
| `Create...Resources` / `Create...RenderTargets` | Declare graph resources owned by that feature or composition scope. |
| `Publish...Products` | Map completed producer outputs to producer-neutral frame or viewport products. |
| `Resolve...` | Make a side-effect-free policy choice; do not mutate the graph or feature lifetime. |

The source filename uses the primary operation or owner stem: `LinearizeDeviceZ.cpp` defines `AddLinearizeDeviceZPass`, `ExposureAdaptation.cpp` defines `AddExposureAdaptationPass`, and `DirectShadowSignalResources.cpp` creates `DirectShadowSignalResources`. Pluralization is semantic: use `Passes` only when the function can order more than one pass or choose among pass implementations. Sibling composition functions use the same parameter order—builder first, immutable settings/context next, mutable feature owners next, and the output resource aggregate last. Do not rename established rendering data (`SceneDepth`, shader types, resource members) merely to mirror the operation that produces it.
- `RenderViewMode` is the one host-independent per-view rendering choice. It names mutually exclusive rendered views such as Lit, Wireframe, a focused buffer view, or Reference Path Tracer; `ViewportRenderRequest` carries it and `RenderView` freezes it. Editor owns labels, icons, menu grouping, shortcuts, and widget state, but does not mirror or translate this rendering semantic through an Editor enum.
- Renderer owns the meaning and execution of each `RenderViewMode`: canonical scene/view data, graph composition, passes, resources, shader parameters, feature lifecycle, and generic products/progress. A mode is not a settings bag and must not accumulate unrelated toggles.
- A mode has one rendering meaning and one declared primary consumer. Reference Path Tracer selects the alternate middle, Wireframe selects raster fill, and buffer/lobe/instance modes select the debug resolve. Do not re-encode any of these as a CVar, show flag, target enum, Editor enum, graph setting, feature setting, or RHI field. Passes and shaders receive the mode only where they select among the products that pass owns.
- Add lower-level per-view show flags only when independently selectable contributions have real production consumers. They are resolved rendering state below the selected mode, never a second mode selector, and must not be introduced merely to imitate another engine's type hierarchy.
- Read CVar-controlled implementation policy at the narrow composition/pass owner that consumes it. Retain a resolved value only when it is required for graph topology identity, extent, thread handoff, or another named lifetime boundary; never copy it through request/settings/context layers merely to reach the decision.
- Frontend code never uses process-global CVars as the normal transport for a per-view mode. A genuinely global Renderer CVar action still crosses the sequenced control boundary and is applied on the Renderer owner thread. This is not permission to introduce a generic settings bag or duplicate per-view/global selector.
- A generic frame-graph helper or factory must not accumulate concrete feature objects. Put a concrete feature under the narrow semantic `Add...Passes` composition function it replaces—lighting, shadows, post-processing, presentation—and invoke it directly. Do not hide features behind forwarding classes or a global feature manager merely to remove names from `FramePipeline`.
- Outside graph construction, a stateful feature may expose the narrow lifecycle operations its persistent GPU resources require. High-level frame orchestration must not unpack those operations into feature-specific progress, sample-prefix identity, allocation, or binding calls. Pass the canonical prepared frame plus only the focused one-shot control values the feature actually consumes, never the complete mutable viewport request.

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

## Capture And Publication

- Name shared render products by their data and semantic domain, never by the feature that first produced them. `Radiance` is a producer-neutral scene-linear HDR product; `RadianceSecondMoment` is an optional producer-declared moment/accumulator product whose exact representation must be documented by its producer. Lit, reference, and optimized path tracers use the same contract, while estimator authority, convergence, checkpoint, and evidence policy remain with their feature owner.
- Renderer selects one semantic render product, snapshots its immutable frame/generation/product metadata, and asks RHI for destination-free bytes. Progressive products use an exact render-identity/sample-prefix contract; ordinary presentation products leave that metadata empty. Renderer does not carry output paths, codecs, staging state, manifests, or filesystem results through viewport or RHI contracts.
- The requester owns the returned capture ticket and polls that exact ticket until it takes the terminal readback. Renderer admits a bounded number of outstanding tickets and retains every admitted completion for its owner; high-level hosts never drain an anonymous completion queue, route results by feature, or silently lose an admitted result to make room for another.
- The Application/tool workflow that accepted the user's save intent retains the destination, chooses the product encoding, schedules background work, and publishes through existing Core file primitives. Reusable image-buffer decoding/encoding and verified atomic bundle publication have one private workflow owner; a feature supplies only its semantic channels, filenames, manifest/checkpoint schema, and intent policy.
- A new capture consumer extends the existing product/readback boundary. It does not add a feature-specific RHI service, duplicate copy/polling state, or create a second renderer/export path.
- Generalization follows actual semantics: extract shared readback, image encoding, task execution, hashing, and publication mechanics once they have concrete consumers, but keep estimator, prefix-settlement, checkpoint, and manifest meaning in the feature that owns them. A generic name over feature policy is still duplication, not reuse.

## Host And UI Separation

- Renderer owns rendering-state values, GPU work, immutable UI draw packets, UI texture bindings, and the exact post-graph presentation operation. It does not own Editor interaction models, labels, panels, restart messages, filesystem persistence, codecs, manifests, output destinations, or background workflow policy.
- Public viewport render products contain only rendering semantics and immutable rendering metadata. UI presentation bindings cross the focused Renderer UI seam and never become fields on `RenderProduct`.
- Renderer source has no `Public/Editor` or `Private/Editor` subsystem. A capability required only by Editor belongs to Editor or Application; a genuinely reusable drawing capability uses rendering/UI terminology and remains independently usable by runtime hosts.
- Rendering settings split by responsibility: Renderer owns the value contract and capture/apply behavior, Editor owns its private mutable settings interaction model, and Application owns persistence plus startup/save ordering.

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
- Do capture requests/results stop at typed bytes and immutable product metadata, with destination and publication policy retained by the Application/tool owner?
- Does each capture consumer retain and poll its exact ticket without teaching an Application orchestrator how to dispatch other consumers' results?
- Does every shader parameter have one exact C++/metadata/HLSL binding name with no alias or reflection fallback?
- Does graphics setup state only non-derivable raster intent while attachments, geometry, shaders, and the pipeline owner supply their own facts exactly once?
- Is every materialized graphics pipeline keyed by the complete state and created only when an actual draw requests it?
- Are persistent data and dirty ranges used where justified?
- Do graphics claims name the exact workload, backend, hardware, driver, build, and evidence?
