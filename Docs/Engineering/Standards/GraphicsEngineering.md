# Graphics Engineering

Status: binding graphics integration standard

Applies to: persistent GPU data, frame metadata, shaders/kernels, path/neural rendering, captures, performance, and hardware/driver-facing work

This standard owns graphics-change guardrails and review questions. The canonical [Renderer and RHI Architecture Boundary](../../Architecture/RendererRhiBoundary.md) owns the dependency, responsibility, lifetime, backend-parity, and enforcement design; graphics work must preserve it rather than reproduce a competing boundary here.

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
- Are persistent data and dirty ranges used where justified?
- Are D3D12 and Vulkan behavior and shader targets validated where supported?
- Do graphics claims name exact workload, hardware, driver, build, and evidence?
