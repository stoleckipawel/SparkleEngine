# PTLAS/TLAS Design Review

Date: 2026-06-17  
Scope: SparkleEngine PTLAS/TLAS implementation review against current official Vulkan, NVIDIA, and AMD guidance  
Status: Baseline design review before implementation refactor

## Executive Summary

Sparkle already has a strong PTLAS foundation:

- The renderer has a clean top-level strategy split between classic TLAS and partitioned TLAS.
- There is a real partition planner, a logical update stream, backend capability reporting, and smoke-test evidence export.
- The codebase is already instrumented well enough to prove behavior once the core update path is corrected.

The main problem is that the current PTLAS execution path does not yet realize PTLAS's primary value proposition.

- The engine computes changed-instance metadata, but the build path still repacks and rewrites all valid instances every frame.
- The staged GPU logical-update/native-pack pipeline exists mostly as scaffolding and is not the path the engine actually uses.
- Several generic interfaces are carrying PTLAS-specific concepts that are unused in classic TLAS mode.
- Some capability/status messaging still reports "not wired" even though PTLAS selection is already active in the renderer.

Bottom line:

- Architecturally promising.
- Well instrumented.
- Not yet production-ready as a PTLAS optimization path.
- Very fixable without a rewrite, if the next pass prioritizes behavioral correctness and simplification over more feature surface.

## Official Reference Set

The review was grounded in official, primary sources:

- Khronos Vulkan PTLAS extension: `VK_NV_partitioned_acceleration_structure`
  - https://docs.vulkan.org/refpages/latest/refpages/source/VK_NV_partitioned_acceleration_structure.html
- Khronos PTLAS operation semantics: `VkPartitionedAccelerationStructureOpTypeNV`
  - https://docs.vulkan.org/refpages/latest/refpages/source/VkPartitionedAccelerationStructureOpTypeNV.html
- NVIDIA PTLAS sample repository
  - https://github.com/nvpro-samples/vk_partitioned_tlas
- NVIDIA RTX Mega Geometry / PTLAS sample announcement
  - https://developer.nvidia.com/blog/nvidia-rtx-mega-geometry-now-available-with-new-vulkan-samples/
- NVIDIA NVAPI PTLAS docs
  - https://docs.nvidia.com/nvapi/struct__NVAPI__D3D12__BUILD__RAYTRACING__PARTITIONED__TLAS__INDIRECT__INPUTS.html
  - https://docs.nvidia.com/nvapi/group__dx.html
- AMD ray tracing optimization guidance
  - https://gpuopen.com/learn/improving-rt-perf-with-rra/
  - https://gpuopen.com/download/RDNA3_Beyond-the-current-gen-v4.pdf

Important note on the AMD side:

- NVIDIA currently publishes a direct PTLAS sample and API surface.
- I did not find an AMD public repository that provides an equivalent PTLAS implementation, because PTLAS is currently exposed through NVIDIA-specific interfaces.
- For AMD, the relevant reference set is their official ray tracing optimization guidance and analysis tooling rather than a drop-in PTLAS sample.

## External Acceptance Criteria

These are the practical criteria implied by the vendor documentation and samples.

### What PTLAS is supposed to buy us

Khronos describes PTLAS as the answer to the classic TLAS requirement to rebuild the whole structure even when only a few instances change. PTLAS is meant to reuse previously built parts and reduce build cost for large scenes with localized motion.

Implication for Sparkle:

- If we still rewrite the full scene every frame, we are not meeting PTLAS's core acceptance criterion even if the API call itself is valid.

### What a correct PTLAS update model looks like

Khronos operation semantics make the intent explicit:

- `WRITE_INSTANCE` assigns an instance to a partition and rebuilds affected partitions.
- `UPDATE_INSTANCE` updates the BLAS reference for an existing instance.
- `WRITE_PARTITION_TRANSLATION` updates partition translation state.

Implication for Sparkle:

- The engine should emit only the minimal set of native operations needed for the changed instances and changed partitions.
- "All instances every frame" is functionally valid, but architecturally defeats the intended use.

### What NVIDIA's sample demonstrates

NVIDIA's sample is important because it shows the expected behavioral model, not just the API syntax:

- Only moving objects are rewritten.
- The sample uses a regular partition grid plus an optional global partition for dynamic objects.
- The sample keeps PTLAS builds indirect and device-driven, but the important lesson is selective updates, not "must be GPU-driven at all costs."
- The sample warns that duplicate `instanceIndex` writes are undefined behavior.

Implication for Sparkle:

- We do not need to match the sample's GPU pipeline immediately.
- We do need to match its update discipline: changed-instance writes only, unique stable instance identity, and explicit handling of partition migration.

### What NVIDIA's API docs imply for layout policy

NVAPI PTLAS sizing requires:

- `instanceCount`
- `maxInstancePerPartitionCount`
- `partitionCount`
- `maxInstanceInGlobalPartitionCount`

The same docs expose `FAST_TRACE`, `FAST_BUILD`, and partition-translation flags.

Implication for Sparkle:

- Worst-case sizing is safe, but it should be intentionally conservative, not accidentally conservative.
- Hardwiring a single build policy leaves performance on the table and makes future behavior less explainable.

### What AMD's guidance adds

AMD's official guidance reinforces several non-vendor-specific truths:

- BVH build cost and traversal cost are a trade-off.
- Instance overlap and empty space matter.
- Large instance deformation can be worse than updating a BLAS.
- TLAS should be rebuilt every frame when needed, and async execution is desirable when feasible.
- Everything should be measured, not assumed.

Implication for Sparkle:

- PTLAS should be judged on measured update cost, traversal cost, and memory footprint together.
- Partitioning must not be treated as an abstract feature win; it has to improve the build-vs-trace trade-off in real captures.

## Current Sparkle Architecture

### Current frame flow

The high-level flow today is:

1. `RayTracingTopLevelScenePlanner::PlanFrame(...)` builds a partition plan and logical update stream.
2. `RayTracingPartitionedTlasStrategy::Prepare(...)` selects PTLAS or classic TLAS and ensures PTLAS resources exist.
3. `RayTracingPartitionedTlasStrategy::Build(...)` uploads logical update records, then builds backend-native PTLAS operation data, then submits the PTLAS build.
4. Backend services in D3D12/Vulkan translate the abstract operation pack into native buffer records.
5. Diagnostics and smoke capture export the provider, partition, update, and timing evidence.

Core files:

- Planner: `Engine/Renderer/Private/RayTracing/RayTracingTopLevelScenePlanner.cpp`
- Partition planner: `Engine/Renderer/Private/RayTracing/RayTracingPtlasPartitionPlanner.cpp`
- Logical update stream: `Engine/Renderer/Private/RayTracing/RayTracingPtlasLogicalUpdateStream.cpp`
- PTLAS strategy: `Engine/Renderer/Private/RayTracing/RayTracingPartitionedTlasStrategy.cpp`
- D3D12 PTLAS backend: `Engine/RHI/Private/D3D12/RayTracing/D3D12PartitionedTlasServices.cpp`
- Vulkan PTLAS backend: `Engine/RHI/Private/Vulkan/RayTracing/VulkanPartitionedTlasServices.cpp`
- Smoke evidence: `Engine/Application/Private/Validation/RhiSmokeCaptureArtifacts.cpp`

### What is already well designed

- Strategy split between classic TLAS and PTLAS is a healthy top-level boundary.
- Stable planner state across frames is already present, which is the right prerequisite for selective updates.
- Diagnostics are unusually strong for work at this stage.
- The engine already tracks planner counters such as dirty transforms, moved partitions, global partition use, and duplicate stable indices.
- Smoke artifacts already serialize PTLAS capability state, selected writer path, native operation counts, and timing data.

This is a good foundation. The issue is not "there is no architecture." The issue is "the architecture has not yet been carried through to the critical update behavior."

## Detailed Findings

### Critical: the PTLAS build path still rewrites the full scene every frame

Evidence:

- `RayTracingPtlasLogicalUpdateStream` emits only dirty or moved entries.
- `RayTracingPartitionedTlasStrategy::BuildPartitionedTlas(...)` then iterates every valid scene instance, rebuilds a full `instanceWrites` array, and creates a single `WriteInstance` operation whose argument count equals total built instance count.
- `MaxOperations` is hardcoded to `1`, and the operation pack contains one full-scene write batch.

Why this matters:

- This defeats PTLAS's main purpose.
- Partitioning still exists, but the update stream does not drive the native build stream.
- Any performance win from only touching changed partitions is weakened or lost by the all-instance rewrite policy.

Assessment:

- This is the most important issue in the implementation.
- Until this is fixed, the engine has PTLAS API integration more than PTLAS behavioral integration.

### High: the logical-update pipeline exists, but it does not control native operations

Evidence:

- `RayTracingPtlasLogicalUpdateStream` produces changed-instance records.
- `UploadLogicalUpdateRecords(...)` uploads them every frame.
- `PackPartitionedTlasNativeOperations(...)` in the PTLAS strategy is empty.
- Backend GPU pack functions return `false` in both D3D12 and Vulkan services.

Why this matters:

- The code presents three layers:
  - logical dirty records
  - native operation packing
  - PTLAS build submission
- In practice, only the final submission plus a CPU full-pack path is alive.

Assessment:

- This is not inherently bad if treated as an incremental roadmap.
- It becomes bad when the scaffolding starts dictating interfaces and file structure before it has earned its keep.

### High: there is a half-wired PTLAS framegraph path

Evidence:

- `RayTracingSceneFrameData` carries `PtlasFrameGraphResources`.
- `FramePipeline::BindRayTracingFrameGraphResources(...)` binds them if present.
- `AddRayTracingSceneBuildPasses(...)` has separate PTLAS logical-update and native-pack passes.
- I did not find any code path assigning real `PtlasFrameGraphResources` into `RayTracingSceneFrameData`.
- `RayTracingPartitionedTlasStrategy::BuildPartitionedTlasFrameData(...)` returns TLAS-only frame data and leaves PTLAS framegraph bindings empty.

Why this matters:

- The renderer pays interface complexity for a staged PTLAS framegraph path that is not actually what drives the live build path.
- This also polluted the generic top-level strategy interface with PTLAS-specific no-op methods in the classic path.

Assessment:

- This is the strongest simplification candidate in the current architecture.

### High: partition sizing is more conservative than necessary

Evidence:

- `ResolveMaxInstancesPerPartition(...)` returns the full instance capacity even when a partition plan exists.
- `BuildPartitionedTlasLayout(...)` uses that value directly for `MaxInstancesPerPartition`.

Why this matters:

- PTLAS sizing APIs need realistic occupancy ceilings.
- Full-scene worst-case per-partition capacity is safe but can overstate memory and scratch requirements substantially.
- Oversized PTLAS layouts make the feature look heavier than it should be and reduce the credibility of future performance measurements.

Assessment:

- This is a memory-efficiency flaw, not a correctness flaw.
- It should be fixed early because it affects every performance and memory review.

### Medium: provider/capability reporting is stale in places

Evidence:

- Renderer strategy selection does use PTLAS when `r.RayTracing.PreferPartitionedTlas` is enabled and capability says supported.
- Some RHI-level reason strings still say variants of `*-supported-but-renderer-selection-not-wired`.

Why this matters:

- Engineers trust diagnostics only if they are truthful.
- Mixed messaging undermines smoke validation and portfolio presentation even if the build technically works.

Assessment:

- Easy cleanup, high trust payoff.

### Medium: build policy is hardwired to fast-trace semantics

Evidence:

- Vulkan PTLAS inputs are created with `VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR`.
- D3D12 NVAPI inputs choose `FAST_TRACE`, optionally with partition translation.
- There is no engine-level PTLAS build policy abstraction.

Why this matters:

- Vendor APIs expose a build-vs-trace trade-off.
- AMD explicitly recommends measuring that trade-off.
- Hardwiring a single choice limits experimentation and makes benchmarking less meaningful.

Assessment:

- This should become an explicit policy, but only after the core selective-update behavior is fixed.

### Medium: the planner/global partition policy is intentionally simple, but currently under-leveraged

Evidence:

- The planner uses a uniform grid and treats non-static meshes as global-partition eligible.
- Dirty transforms immediately allow global-partition use for eligible instances.

Why this matters:

- This is acceptable as a first heuristic.
- However, while the build path rewrites the full scene, smarter partition movement policy cannot express its full value.

Assessment:

- Do not over-design this until the selective native update path is real.
- The current heuristic is good enough for a first production-grade pass.

### Medium: capability reporting mixes API support, runtime presence, and engine implementation status

Evidence:

- `RhiPartitionedTlasCapabilities` contains many booleans spanning:
  - API/provider support
  - runtime availability
  - backend implementation details
  - potential engine features

Why this matters:

- The level of detail is useful for diagnostics.
- The current grouping makes it easy to overstate what the engine can actually do today.

Assessment:

- Keep the detail.
- Tighten the vocabulary and grouping.

### Medium: generic interfaces are carrying PTLAS-specific behavior

Evidence:

- `RayTracingTopLevelAccelerationStructureStrategy` contains PTLAS-only methods:
  - `BuildPartitionedTlasLogicalUpdateResources(...)`
  - `PackPartitionedTlasNativeOperations(...)`
- `RayTracingClassicTlasStrategy` implements them as no-ops.
- `RayTracingSceneFrameData` contains PTLAS-specific framegraph bindings even when running classic TLAS.

Why this matters:

- This is a clean example of complexity that has not earned its right to exist.
- The generic strategy layer should not expose stage-specific hooks for an optional implementation path unless that path is truly first-class and in active use.

Assessment:

- Strong candidate for removal in the first simplification pass.

### Low but real: file and function concentration is too high in a few places

Measured hotspots:

- `Engine/Renderer/Private/RayTracing/RayTracingPartitionedTlasStrategy.cpp` — 667 lines
- `Engine/RHI/Private/D3D12/RayTracing/D3D12PartitionedTlasServices.cpp` — 416 lines
- `Engine/RHI/Private/Vulkan/RayTracing/VulkanPartitionedTlasServices.cpp` — 415 lines
- `Engine/Renderer/Private/RayTracing/RayTracingPtlasPartitionPlanner.cpp` — 254 lines

Why this matters:

- The strategy file currently owns policy, resource lifetime, update upload, native packing orchestration, build submission, fallback control, and metrics handoff.
- Backend service files duplicate layout/packing orchestration patterns that are similar in shape but not identical in native details.

Assessment:

- This is a maintainability issue, not the root performance issue.
- It should be addressed immediately after behavioral correctness work starts, not before.

## Production-Readiness Assessment

Current rating: not yet production-ready as a PTLAS optimization path.

Reasons:

- The engine does not yet use PTLAS incrementally in the way the official extension and sample are designed to be used.
- There is dead or half-wired architecture in the framegraph and GPU packing stages.
- Memory layout sizing is overly pessimistic.
- Diagnostics are better than the execution path they describe.

What is already production-quality:

- Capability discovery groundwork
- Fallback strategy structure
- Smoke evidence and telemetry breadth
- Partition/debug observability

What must change before this is presentable as a production-grade PTLAS integration:

- Native update stream must become selective.
- Dead scaffolding must either be removed or fully wired.
- Diagnostics must describe reality exactly.
- Memory sizing and policy knobs must be intentional.

## Recommended Simplification Direction

This is the most important architecture choice.

I recommend a CPU-first, behavior-correct PTLAS architecture before reintroducing GPU-native staging.

That means:

1. Keep the planner.
2. Keep the logical update record model.
3. Delete or temporarily collapse unused PTLAS framegraph stages and generic no-op hooks.
4. Make the live `Build(...)` path generate selective native operations directly from the logical update stream on CPU.
5. Reintroduce GPU-native operation packing only when it is actually implemented end to end.

Why this direction is the right trade:

- It matches the behavioral contract of the NVIDIA sample.
- It keeps the code honest and compact.
- It produces measurable value immediately.
- It avoids turning "future GPU-driven PTLAS" into today's complexity tax.

## Recommended Refactor Shape

### Keep

- `RayTracingTopLevelScenePlanner`
- `RayTracingPtlasPartitionPlanner`
- `RayTracingPtlasLogicalUpdateStream`
- `RayTracingClassicTlasStrategy`
- Backend-native packers in D3D12/Vulkan
- Smoke evidence and diagnostics export

### Remove or collapse in the next pass

- PTLAS-specific methods from the generic top-level strategy interface unless GPU staging is finished immediately
- PTLAS framegraph resource bindings in `RayTracingSceneFrameData` if the build path remains direct
- Unused PTLAS logical/native pack passes from the framegraph contract if they are not going to drive real work

### Split

`RayTracingPartitionedTlasStrategy.cpp` should be split into smaller collaborators:

- PTLAS policy and provider selection
- PTLAS resource allocator / layout manager
- PTLAS CPU native operation builder
- PTLAS metrics/result mapper

### Merge or tighten

- Keep per-backend native record packing local.
- If duplication remains annoying, share only the buffer layout math and validation helpers, not the full backend packing code.

## Recommended Implementation Phases

### Phase 0: truthfulness and cleanup

- Fix stale provider-selection reason strings.
- Remove dead "not wired" wording from capability reporting.
- Mark unimplemented GPU-native writer paths as unavailable in a way that matches runtime behavior exactly.
- Decide whether the framegraph PTLAS stages live or die.

Expected result:

- Diagnostics become trustworthy immediately.

### Phase 1: real selective PTLAS CPU path

- Translate logical updates into native write/update/partition operations.
- Submit only changed-instance operations plus partition translations when needed.
- Preserve unique stable instance identity.
- Keep CPU packing as the only supported writer path for now.

Expected result:

- Sparkle finally benefits from PTLAS behaviorally, not just architecturally.

### Phase 2: memory/layout correctness

- Compute `MaxInstancesPerPartition` from observed or planned occupancy high-water marks.
- Compute global-partition high-water marks intentionally.
- Separate "steady-state capacity" from "current frame count."
- Add a PTLAS build policy enum for fast-trace vs fast-build if benchmarking shows value.

Expected result:

- Better memory efficiency and more honest performance measurements.

### Phase 3: optional GPU-native pipeline

- Only after Phase 1 and 2 are stable.
- Implement GPU logical-dirty writing if it clearly reduces CPU cost.
- Implement GPU native-operation packing end to end.
- Reintroduce staged framegraph passes only if they now perform real work.

Expected result:

- Extra sophistication that has finally earned its complexity budget.

## Concrete Action Items

### Must do

- Replace full-scene `WriteInstance` packing with selective update packing.
- Fix stale PTLAS capability/provider reason strings.
- Remove or complete the dead PTLAS framegraph path.
- Recompute partition occupancy sizing instead of using full-scene worst case for every partition.

### Should do

- Remove PTLAS-specific methods from the generic TLAS strategy interface unless the staged PTLAS path becomes truly active.
- Introduce a PTLAS build policy enum only after the selective path is working.
- Keep smoke validation focused on real supported paths instead of aspirational ones.

### Nice to do after the core fixes

- Add async scheduling experiments for TLAS/PTLAS builds where backend and frame structure make it worthwhile.
- Improve global-partition migration heuristics based on distance or screen-space relevance once update packing is selective.
- Add occupancy histograms and partition pressure metrics to smoke evidence.

## Acceptance Checklist For The Next Implementation Pass

- PTLAS updates only changed instances or changed partitions.
- Full-scene PTLAS writes happen only on first build, resize, or explicit reset.
- Diagnostics clearly distinguish:
  - provider supported by API
  - provider implemented by engine
  - provider selected at runtime
- No unused PTLAS-only hooks remain in generic interfaces.
- No dead PTLAS framegraph resource path remains.
- Memory sizing uses meaningful occupancy limits.
- Smoke artifacts prove:
  - dirty instance count
  - moved partition count
  - native operation count
  - selected writer path
  - update timings
  - explicit fallback reasons when PTLAS is unavailable

## Final Verdict

Sparkle is closer to a good PTLAS architecture than it may look at first glance.

The engine already has the hard parts that many codebases skip:

- stable per-instance identity
- per-frame change detection
- partition planning
- provider abstraction
- deep diagnostics
- automated evidence capture

What it does not yet have is the one thing PTLAS exists to provide:

- a truly selective top-level update path

That is the next pass that matters most.

If we fix that first and delete the currently unearned scaffolding around it, the implementation can become both simpler and much more impressive to other engineers.
