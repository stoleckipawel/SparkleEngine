# Shader System Migration Baseline

Status: archive; dated migration snapshot, not current architecture, implementation status, or delivery authority

Responsibility: preserve the frozen Phase 0 shader-system findings, counts, and deletion ledger as migration provenance

Architecture authority: [Shader System Architecture](../Architecture/CrossModule/ShaderSystem.md)

Delivery authority: [Shader System Delivery Plan](../Plans/CrossModule/ShaderSystem.md)

## Purpose And Boundary

This record preserves the frozen Phase 0 findings, exact counts, and deletion ledger used to design the clean-break migration. Revalidate current behavior in code, build configuration, and the capability inventories; do not use these historical counts as current-state claims.

## Phase 0 Baseline Findings

The 2026-08-23 Phase 0 source review at `5b0bd1469339897eef1fde3e5c9ab07137860d0f` traced the pre-migration production path below. This section is retained as frozen inventory and provenance; it is not a statement of the post-Phase 1 worktree.

```text
Authoring                     Host tooling                              Runtime and GPU
=========                     ============                              ===============

HLSL / HLSLI ----------+      ShaderCompiler CLI                        FrameGraphBuilder::Draw/Dispatch
                       |              |                                             |
C++ shader type -------+----> static GlobalShaderRegistry                           |
and FParameters        |              |                                             |
                       |              v                                             |
C++ pass Parameters ---+----> contract catalog ----> cook plan                      |
       |                              |                 |                            |
       |                              |                 v                            |
       +--> RDG resource use          |          bounded SparkleTasks jobs          |
                                      |                 |                            |
                         physical source/include closure                             |
                                                        |                            |
                                              +---------+---------+                  |
                                              |                   |                  |
                                             DXC                Slang                |
                                              |                   |                  |
                                              +--> bytecode + reflection             |
                                                        |                            |
                                              compile every selected stage           |
                                                        |                            |
                                              transactional .sparkshader             |
                                              + registry + recook signal             |
                                                        |                            |
                                                        +--------------------------->+
                                                                                     |
                                                                     package load + ABI validation
                                                                                     |
                                                                     binding-layout creation
                                                                                     |
                                                                     lazy pipeline creation
                                                                                     |
                                                                     pass Execute: bind + draw/dispatch
```

The separation at the right is important: graph construction may materialize a pass runtime, while graph execution does not load a package or create a pipeline. The following findings classify what that path actually provides today:

- 28 pass classes declare `static constexpr const char* PassName`.
- `RendererShaderPackages.h` declares 28 package strings.
- Renderer shader-registration sources make 29 calls to `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE` and no calls to the package-inferred `IMPLEMENT_GLOBAL_SHADER` path.
- The three names normally repeat: pass name, package constant, and generated binding/pipeline debug names.
- Most current programs contain one compute stage. `GBuffer` demonstrates the legitimate multi-file, multi-stage case with a vertex shader and a pixel shader sharing one cooked program.
- `BuildShaderPackageIdFromSourcePath` derives only the filename stem, so same-basename sources in different directories collide and a source rename changes the fallback package ID.
- `GlobalShaderRegistry::Register` silently ignores a repeated shader name instead of reporting a conflicting declaration.
- The contract validator catches empty IDs, duplicate shader names, package layout/kind mismatch, and duplicate stages, but the registry's early duplicate suppression can prevent the validator from seeing the conflict.
- Direct Lighting and comparable passes declare resources twice: `DirectLightingPassParameters` drives frame-graph/pass binding, while `DirectLightingCS::FParameters` independently drives shader reflection verification. Both lists can compile while drifting semantically.
- `PassBinder` accepts either the same parameter-layout instance or merely the same parameter count. Equal counts do not prove equal binding order, kinds, names, visibility, or sizes.
- Source resolution is based on physical paths and searches project shaders before engine shaders. Includes search the includer's directory, a primary root, then additional roots, and absolute include paths are accepted. This is functional locally but lacks an explicit virtual namespace, permits machine-specific source identity, and can silently shadow an engine shader with a project file of the same relative path.
- The preprocessor expands nested includes, detects recursion, handles `#pragma once`, and emits line directives. Job construction captures that exact preprocessed compiler input, verifies the virtual dependency closure is stable around the capture, and never asks a backend or failure replay to reread mutable source files. `IncludeClosureHasher` returns the sorted virtual dependency closure, and `ShaderCompileRequestHasher` contributes compiler-affecting environment/ABI policy without hashing a physical checkout path.
- `ShaderCompileBatch` performs bounded parallel jobs through the existing `TaskExecutor`, compiles every selected input, deduplicates only exact identical inputs inside that operation, then fans out and validates every logical shader-type result. There is no persisted compiler result, cache key/status, cache-directory option, or second worker system.
- Cancellation is cooperative across Application and the cooker through one private per-operation signal; the parent does not hard-terminate the cooker. Cancellation observed before emission settles without publication, while cancellation that arrives after the transactional commit begins loses to the completed coherent file set instead of interrupting it between renames. Task failure prevents the emitter from running. The inner batch checks cancellation before each compile task, preserves deterministic indexed result integration, and bounds simultaneous compiler sessions to the existing `1..8` setting.
- `ShaderCompileInputHash` covers virtual source identity, the exact captured compiler-source bytes, the verified content closure, entry/stage/target, compile-unit kind, required features, compiler policy, ABI binding remaps, target profile, backend identity/version, and unambiguous hash-domain separation. Package/program/pass labels, basenames, presentation text, physical roots, and successful-analysis output policy are excluded.
- Changed-source monitoring publishes exact virtual paths. `ShaderDependencyManifest` persists verified forward and reverse virtual dependencies; valid metadata selects the affected shader types and the temporary atomic physical publication group, while missing/corrupt/incomplete metadata fails loudly and directs an explicit global rebuild instead of silently falling back.
- The cooker publishes selected `.sparkshader` files, a registry, and a recook signal as one transactional file set. The registry is not currently the runtime lookup authority; runtime computes a path directly from the manually repeated package ID.
- Bytecode remains embedded per current package until Phase 4, but identical compiler-affecting jobs now compile once per active operation and fan out to every logical consumer. There is deliberately no cross-operation result store; Phase 4 owns cooked code-library deduplication.
- DXC can emit disassembly, preprocessed source, compiler arguments, diagnostics, and separate debug data for a successful opt-in analysis cook. Slang's current analysis output is narrower and does not emit equivalent disassembly.
- The DXC backend applies requested debug information, optimization, warnings-as-errors, and debug stripping. The Slang backend currently does not apply equivalent cook-policy switches, and its stage mapping covers vertex, pixel/fragment, and compute rather than the full stage enum. Slang is therefore an architectural backend seam, not a proved release-equivalent compiler path.
- Successful analysis artifacts remain opt-in. The first compile or parameter-ABI failure overwrites one bounded `LastShaderCompileFailure.json` replay record containing typed identity, target/backend provenance, hashes, virtual dependencies, defines, diagnostic, and bounded preprocessed source; no per-job log stream or accumulating failure directory is added.
- `CookedShaderStats.csv` reports package/stage/backend/target/entry, bytecode bytes, reflection counts, and layout-record counts. It does not report queue/wall/CPU time, peak memory, source/code hash correlation, native pipeline creation, or runtime hitch percentiles.
- `inspect-shader` inspects registration/catalog metadata; `inspect-package` inspects the cooked container, reflection, layouts, and ray-tracing records. There is no single command that follows a typed shader from declaration through compile-input hash, code hash, active map/library record, runtime shader reference, pipeline, and captured GPU event.
- `RenderPassRuntimeCache` already builds and validates a complete replacement generation, atomically activates it, and retires the old generation only after recorded RHI submission tokens complete. This is a strong lifetime pattern and must be preserved.
- Runtime package validation is materially stronger than the authoring layer: it verifies schema/version, source and layout identities, bounds, complete logical binding records, required stages, runtime backend format, bytecode hashes, reflection, feature flags, and ray-tracing metadata before use.
- The editor already launches the shader cooker out of process, coalesces one follow-up request, rejects stale publications, and leaves the active generation unchanged after cook or runtime-validation failure.
- The current `Shader Tools` window presents Refresh, Reload Cooked, Recook All, and Recook Selected as equal toolbar actions above a ten-column table. Package ID, binding layout, parameter count, backend/target text, generation, and artifact availability are visible before the user has asked an expert question. This is an implementation-oriented inventory, not the selected intent-first frontend.
- The current selection area opens raw Source, Reflection, Disassembly, Param Match, and Compile Request artifacts and discovers their directory from shader/package identity. These are valuable expert details, but the panel lacks one operation/status model, source-located failure summary, shader consumer trace, and a guided next action. Raw artifacts must move behind contextual Diagnostics/Advanced disclosure rather than be removed.
- Current source tracking automatically schedules a changed recook, while the toolbar and console also expose manual global/package/shader actions. The target must converge these into one coalesced `Apply Changed` workflow with one visible state; full rebuild, manual reload, and identity-targeted recovery remain expert actions rather than parallel normal paths.
- A pass runtime is materialized lazily from `FrameGraphBuilder::Draw`, `Dispatch`, or `DispatchAsync`. Its package, binding layout, and pipeline are created during graph construction, not command recording. This preserves the frame-graph Execute boundary but can place first-use pipeline creation on a frame-critical construction path.
- Renderer frame orchestration now uses `BuildRenderFrameGraph`, explicit pass parameter records, and the infrastructure-only `PassCommandContext`. Shader work must preserve that boundary: graph setup may resolve a prepared immutable shader/pipeline runtime, while pass recording may only bind declared resources and record commands.
- `RenderFrameIdentity` captures the active shader generation from `RenderPassRuntimeCache`; `RenderViewState` invalidates history from the generation transition. The shader migration must keep one renderer-owned generation source and must not publish package/cache/compiler mechanics through GameFramework, `RenderFrameSubmission`, `RenderView`, or `PreparedRenderScene`.
- `FrameGraph::HasBeenProduced` is now the authority for whether a declared graph resource has prior contents. Shader and pipeline work must not reintroduce one-field history-validity records, mirrored production flags, or broad frame/pass contexts.
- `FrameGraphBuilder` and `FrameGraph` currently expose `Read(texture)` / `Read(buffer)` and `CreateSRV(texture)` / `CreateSRV(buffer)` as duplicate aliases returning the same typed shader fields. Production pass call sites already use `CreateSRV` for texture/buffer shader reads; the stale proposal example was the only `builder.Read(texture)` spelling found. Acceleration-structure call sites use a separate `Read(sceneTlas)` overload. Phase 1 replaces the AS overload with `CreateAccelerationStructureBinding`; Phase 2 deletes the unused texture/buffer `Read` aliases and freezes explicit SRV/UAV versus attachment vocabulary.
- Renderer C++ passes now live under semantic `Passes/GBuffer`, `Passes/Lighting/<Direct|Reference|Restir|Shadows|Sky>`, `Passes/PostProcessing`, `Passes/Presentation`, `Passes/RayTracing`, and `Passes/Debug` owners. Shader sources still collect 18 files under `Engine/Assets/Shaders/Passes/Deferred`; the source-namespace phase must move those files to matching semantic owners and delete the broad directory rather than treating `Deferred` as renderer-wide architecture.
- D3D12 creates every graphics/compute pipeline with an empty `CachedPSO`; Vulkan calls `vkCreateGraphicsPipelines` and `vkCreateComputePipelines` with `VK_NULL_HANDLE` for the pipeline cache. Vulkan computes a local cache-key-shaped struct and then discards it. There is no renderer pipeline cache, native persistent cache, asynchronous precache coordinator, or hit/miss/too-late telemetry.
- Pass labels, D3D12 PIX events, Vulkan object names, binding-layout names, and pipeline names are readable. They do not carry stable shader-type/code identity that can join a capture event to the cooker artifacts and external shader symbols.
- Current committed source extends the generic registration, cook, map/library, neutral RHI, paired backend, runtime-generation, and typed graph infrastructure to ray-tracing stages and removes the former valid-library rejection. Phase 7 adds the reachable GBuffer ray-generation/miss/closest-hit route; Phase 8 adds its alpha any-hit export plus the shadow ray-generation/miss/closest-hit/any-hit route through the same path. Intersection/callable and nonzero local-record conformance remain Phase 12 obligations. Source reachability is not executable proof.
- `DirectShadowSignalNoRayQuery` and `DirectShadowSignalDeviceAddress` are registered and have pass implementations, but the production frame path always dispatches `DirectShadowSignal`. `CanUseInlineRayQueryShadows` has no selection consumer, and top-level provider selection falls back to classic descriptor access rather than choosing the device-address shader. These alternatives are not verified fallbacks. The device-address root duplicates the live shader solely because native AS binding representation leaked into shader identity; the no-query root duplicates lighting work only to skip traversal. Phase 1 deletes both instead of moving or retaining them.
- Geometry, hull, and domain stages appear in shader enums, package validation, and Vulkan shader-module mapping, but current graphics pipeline descriptions/backends wire only vertex plus optional pixel stages. Mesh/task stages are absent and both RHI capability reports mark them unsupported. Schema awareness is not executable stage support.
- Runtime capability checks directly reject missing acceleration-structure and inline-ray-query support, but do not explicitly check every declared package feature such as acceleration-structure device-address access and descriptor indexing in the same capability gate.
- CLI validation structurally validates the catalog but cooks and inspects only the representative `ComputeClear` artifact. `ShaderCompilerCliValidation` is a custom build target rather than a registered CTest, and no shader-specific unit/integration test suite was found.
- The existing build already discovers and links the shader-registration object library into compiler/runtime consumers, so authoring automation does not require scanning all shader source files.
- 56 generated `.sparkshader` files were present in the reviewed development artifacts: 28 files totaling 2,004,452 bytes under each of the `Shared` and `Showcase` project roots, for 4,008,904 bytes in aggregate. The two roots contain the same 28 artifact names. This is not evidence that 28 unique physical programs are a performance problem.

Relevant implementation entry points:

- [`GlobalShader.h`](../../Engine/RHI/Public/Shaders/Authoring/GlobalShader.h)
- [`ShaderAuthoring.cpp`](../../Engine/RHI/Private/Shaders/ShaderAuthoring.cpp)
- [`GlobalShaderMap.h`](../../Engine/RHI/Public/Shaders/GlobalShaderMap.h)
- [`ShaderMap.cpp`](../../Engine/RHI/Private/Shaders/ShaderMap.cpp)
- [`DirectLighting.h`](../../Engine/Renderer/Private/Passes/Lighting/Direct/DirectLighting.h)
- [`DirectLighting.cpp`](../../Engine/Renderer/Private/Passes/Lighting/Direct/DirectLighting.cpp)
- [`DirectLightingShaders.cpp`](../../Engine/Renderer/ShaderRegistrations/DirectLightingShaders.cpp)
- [`FrameGraphBuilder.h`](../../Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h)
- [`ShaderContractCatalogBuilder.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractCatalogBuilder.cpp)
- [`ShaderContractValidator.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractValidator.cpp)
- [`ShaderCompileBatch.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderCompileBatch.cpp)
- [`ShaderCompileJobBuilder.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderCompileJobBuilder.cpp)
- [`ShaderDependencyManifest.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/Dependencies/ShaderDependencyManifest.cpp)
- [`ShaderDebugArtifactWriter.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderDebugArtifactWriter.cpp)
- [`IncludeClosureHasher.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/Identity/IncludeClosureHasher.cpp)
- [`ShaderCompileRequestHasher.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/Identity/ShaderCompileRequestHasher.cpp)
- [`ShaderArtifactPublication.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderArtifactPublication.cpp)
- [`PassBinder.cpp`](../../Engine/Renderer/Private/Pipeline/PassBinder.cpp)
- [`RenderPassShaderRuntime.h`](../../Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h)
- [`PipelineRuntimeLibrary.cpp`](../../Engine/Renderer/Private/PipelineRuntime/PipelineRuntimeLibrary.cpp)
- [`RenderPassRuntimeCache.cpp`](../../Engine/Renderer/Private/Pipeline/RenderPassRuntimeCache.cpp)
- [`D3D12Pipeline.cpp`](../../Engine/RHI/Private/D3D12/Pipeline/D3D12Pipeline.cpp)
- [`VulkanPipeline.cpp`](../../Engine/RHI/Private/Vulkan/Pipeline/VulkanPipeline.cpp)
- [`ShaderSourceChangeTracker.cpp`](../../Engine/Application/Private/ShaderRecook/ShaderSourceChangeTracker.cpp)
- [`ShaderRecookCoordinator.cpp`](../../Engine/Application/Private/ShaderRecook/ShaderRecookCoordinator.cpp)

Historical Phase 0 entry points deleted by Phase 4 were `RendererShaderPackages.h`, `CookedShaderPackage.h`, `CookedPackageWriter.cpp`, `CookedShaderPackageCache.cpp`, and `CookedShaderPackageValidation.cpp`; they remain plain historical names rather than broken current links.
- [`ShaderCompilerProcess.cpp`](../../Engine/Application/Private/ShaderRecook/ShaderCompilerProcess.cpp)
- [`ShaderCompiler` build definition](../../Tools/Shaders/ShaderCompiler/CMakeLists.txt)
- [`ValidateShaderCompilerCli.cmake`](../../Tools/Shaders/ShaderCompiler/ValidateShaderCompilerCli.cmake)

## Phase 0 Frozen Inventory and Deletion Ledger

This section is the completed Phase 0 contract. Counts are exact for the unstaged `master` worktree at `5b0bd1469339897eef1fde3e5c9ab07137860d0f`; generated-artifact counts are a filesystem observation made on 2026-08-23 and are not attributed to that revision unless stated otherwise. Before this Phase 0 inventory edit, the staging area was empty and this document already contained unstaged, in-scope delivery-contract refinements; no unrelated dirty path existed, so the Phase 0 exclusion list is **empty**. The source/tool diff from the previous audit revision `44c2f192a82947d9dcdd0e4bbd7ba0cb1a7145e4` to the audited Phase 0 revision is empty, but all counts and owner traces below were nevertheless re-run against the Phase 0 worktree. Later phases must re-run `git status --short --branch` and establish their own exclusions rather than assuming that remains true.

The inventory used `rg`, `rg --files`, file counts, and bounded reads over Renderer shader registrations and passes, RHI shader contracts/runtime, ShaderCompiler source/cook/publication/inspection, Application recook routing, Editor Shader Tools, CMake membership, ignored development artifacts, diagnostics, and then-current documentation. No configure, build, compiler invocation, cook, launch, test, capture, or runtime check contributed to this inventory.

### Exact count baseline

| Surface | Exact Phase 0 count | Frozen interpretation and deletion owner |
| --- | ---: | --- |
| renderer shader-registration `.cpp` files / `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE` calls | 29 / 29 | One registration per then-current stage. Phase 1 deletes the two shadow representation duplicates; Phase 2 replaces the macro on the remaining 27 declarations. |
| concrete shader classes / nested `FParameters` schemas | 29 / 29 | Phase 1 deletes the two catalog-only shadow variants; the remaining 27 are retained as authoring authorities and renamed to target `GlobalShader`/`Parameters` vocabulary in Phase 2. |
| logical program/package IDs | 28 | `GBufferVS` and `GBufferPS` share `GBuffer`. Phase 1 deletes the two rejected shadow identities; Phase 4 deletes the remaining 26 handwritten package identities. |
| `RendererShaderPackages` constants | 28 | Phase 1 deletes the two rejected shadow constants; Phase 4 deletes the remaining constants and then the header. |
| authored pass classes / `PassName` constants | 28 / 28 | Phase 1 deletes the two rejected shadow wrappers and labels; Phase 2 deletes the remaining 25 compute wrappers and narrows `GBufferPass` to real graphics behavior. |
| pass wrapper files | 56 | Twenty-eight matching `*Pass.h`/`*Pass.cpp` pairs. Phase 1 deletes the two rejected shadow pairs; Phase 2 deletes the remaining 25 forwarding pairs and rewrites the retained GBuffer pair in place. |
| pass API declarations and definitions | 28 `GetDefinition`, 28 `GetParameterMetadata`, 28 `Execute` pairs | Phase 1 deletes the two rejected-shadow forwarding surfaces. Phase 2 deletes the remaining generic accessors/Execute forwarding surfaces; the retained graphics collaborator receives a behavior-specific draw surface. |
| repeated binding-layout / pipeline string literals | 28 / 28 | Phase 1 deletes the four strings owned by the two rejected shadow wrappers. Phase 2 derives remaining diagnostics from shader type or accepts an instance label and deletes the other 52 strings. |
| nested shader parameter field declarations | 392 | Phase 1 deletes 32 fields belonging to the two rejected shadow variants. Phase 2 retains/renames the remaining 337 compute fields and 23 GBuffer-stage fields as the one declaration authority. |
| directly authored shader-visible fields in pass-side records | 352 | Duplicate declarations: 324 in compute pass records, 10 in `GBufferPassParameters`, and 18 in `GBufferDrawParameters`. Phase 2 deletes them and uses nested shader metadata. |
| effective concrete compute pass-mirror fields after common-base expansion | 369 | Exactly mirrors the 369 nested compute fields. Phase 1 deletes the 32 rejected-shadow mirrors; Phase 2 deletes the remaining 337 mirrors, including inherited common fields. |
| effective GBuffer shader-visible pass/draw field uses | 28 | Mirrors 23 stage fields across `GBufferPassParameters` and `GBufferDrawParameters`; Phase 2 removes the copies while preserving the real draw collaborator. |
| graph-only GBuffer attachment fields | 7 | `BaseColor`, `Normal`, `Material`, `Emissive`, `Subsurface`, `MotionVector`, and `DeviceZ` remain only in a narrow graphics graph envelope in Phase 2. |
| semantic graph resource-view calls | 178 `CreateSRV`, 49 `CreateUAV`, 7 AS `Read`, 6 `CreateRenderTarget`, 1 `CreateDepthTarget` | The seven `Read` calls are all acceleration-structure bindings and become `CreateAccelerationStructureBinding` in Phase 1. Phase 2 removes the duplicate texture/buffer `Read` aliases while retaining canonical SRV/UAV and raster-attachment routes. |
| files under `Engine/Assets/Shaders/Passes/Deferred` | 18 | Thirteen `.hlsl` and five `.hlsli` files. All move to semantic owners and the old directory is deleted in Phase 1. |
| generated `.sparkshader` files | 56 files / 4,008,904 bytes | Two identical-name sets: 28 files / 2,004,452 bytes under `Shared` and the same under `Showcase`. All old files are disposable Phase 4 outputs. |
| generated shader package registries | 2 | One `ShaderPackageRegistry.sreg` under each `Shared` and `Showcase`; both are disposable Phase 4 outputs. |

The parameter counts deliberately distinguish declarations from effective use. `352` is the number of authored duplicate shader-visible field declarations. `397` is the larger effective mirror-use count (`369` concrete compute fields plus `28` GBuffer pass/draw uses) after inheritance and reuse. Phase 1 removes the 32 rejected-shadow fields from both authorities; Phase 2 removes the remaining mirrors. Neither count includes the seven graph-only GBuffer attachments.

The 2026-08-23 Phase 2 checkpoint-worktree PSO audit was run after the staged/unstaged Phase 2 checkpoint and is therefore separate from the revision-pinned pre-migration counts above. Exact runtime/build occurrence counts are: `GraphicsShaderPipelineState` 9, `RasterPassPipelineRuntime` 24, `GraphicsPipelineDesc` 15, `RhiVertexLayoutKind` 10, `RhiDepthTestDesc` 7, `RhiStencilTestDesc` 7, `WireframePipeline` 9, `TwoSidedPipeline` 8, `RenderTargetFormats` 8, `RenderTargetCount` 12, `DepthStencilFormat` 16, and `SetPrimitiveTopology` 11. There is one typed graphics graph call, `Draw<GBufferVS, GBufferPS>`, in `RasterizedGBuffer.cpp`. Phase 5 owns the complete field/consumer re-inventory at implementation time and the clean-break dispositions below; these occurrence counts are evidence of the reviewed checkpoint, not target quotas.

| Phase 2 checkpoint owner/consumer | Checkpoint authority problem | Phase 5 disposition |
| --- | --- | --- |
| `RasterizedGBuffer.cpp` graph setup | graph attachments and the caller aggregate both state target compatibility; the aggregate also chooses mesh and pass facts | keep attachments and granular pass intent; delete the complete caller aggregate |
| `FrameGraphBuilder::Draw` | accepts and forwards the aggregate | accept typed shaders, narrow render state, and prepared draws only |
| `RenderPassRuntimeCache` | one shader-pair key stores one state and rejects another legitimate state | assemble/hash the complete key from all authorities and retain it in the existing generation owner |
| `RenderPassShaderRuntime` / `RasterPassPipelineRuntime` | eagerly creates base, two-sided, and wireframe variants | materialize exact requested keys lazily before recording and delete the bundle |
| `ShaderPassOperations` | selects wireframe from view semantics while binding | remove semantic policy from generic binding; the mesh/material/pass owner requests fill/cull |
| `GBufferMeshPass` and `GpuMesh` | targets/clears and triangle topology have duplicate routes | graph attachment execution owns target actions; prepared mesh draw owns topology once |
| public RHI and D3D12/Vulkan graphics creation | neutral state is incomplete while backends invent opaque blend, sample one, triangle topology, and raster defaults | complete the neutral descriptor and require both backends to consume or reject every supported field |

### Complete shader-class and nested-parameter inventory

Every Phase 0 live semantic row retains the concrete shader and its nested schema as the Phase 2 authority. The then-current `FParameters` spelling becomes `Parameters`; the corresponding pass schema, pass metadata accessor, package selector, and forwarding Execute body do not survive. The two catalog-only shadow variants are explicit Phase 1 deletions and never reach the target catalog.

| Shader class | Stage | Nested fields | Logical program | Phase 0 graph status |
| --- | --- | ---: | --- | --- |
| `ComputeClearCS` | compute | 1 | `ComputeClear` | live, reused with instance labels |
| `DirectLightingCS` | compute | 19 | `DirectLighting` | live |
| `DirectLightReservoirSpatialCS` | compute | 19 | `DirectLightReservoirSpatial` | live |
| `DirectLightReservoirTemporalCS` | compute | 20 | `DirectLightReservoirTemporal` | live |
| `DirectShadowSignalDeviceAddressCS` | compute | 20 | `DirectShadowSignalDeviceAddress` | Phase 1 deletion; backend binding detail, no graph consumer |
| `DirectShadowSignalNoRayQueryCS` | compute | 12 | `DirectShadowSignalNoRayQuery` | Phase 1 deletion; graph fallback policy, no graph consumer |
| `DirectShadowSignalCS` | compute | 21 | `DirectShadowSignal` | live |
| `ExposureDownsampleSceneCS` | compute | 2 | `ExposureDownsampleScene` | live |
| `ExposureDownsampleTextureCS` | compute | 2 | `ExposureDownsampleTexture` | live |
| `ExposureReduceSceneCS` | compute | 2 | `ExposureReduceScene` | live |
| `ExposureReduceTextureCS` | compute | 2 | `ExposureReduceTexture` | live |
| `ExposureCS` | compute | 5 | `Exposure` | live asynchronous dispatch |
| `GBufferVS` | vertex | 11 | `GBuffer` | live raster branch |
| `GBufferPS` | pixel | 12 | `GBuffer` | live raster branch |
| `LightingCompositeCS` | compute | 8 | `LightingComposite` | live |
| `LinearUpscaleCS` | compute | 3 | `LinearUpscale` | live |
| `OutputEncodingCS` | compute | 3 | `OutputEncoding` | live presentation branch |
| `PathTracedDirectLightingCS` | compute with inline ray query | 26 | `PathTracedDirectLighting` | live reference-lighting branch |
| `PathTracedIndirectLightingCS` | compute with inline ray query | 32 | `PathTracedIndirectLighting` | live reference-lighting branch |
| `RaytracedGBufferCS` | compute with inline ray query | 25 | `RaytracedGBuffer` | live ray-query GBuffer branch |
| `ReferenceLightingAccumulationCS` | compute | 7 | `ReferenceLightingAccumulation` | live reference-lighting branch |
| `RestirIndirectResolveCS` | compute with inline ray query | 37 | `RestirIndirectResolve` | live ReSTIR branch |
| `RestirIndirectSpatialCS` | compute with inline ray query | 35 | `RestirIndirectSpatial` | live ReSTIR branch |
| `RestirIndirectTemporalCS` | compute with inline ray query | 36 | `RestirIndirectTemporal` | live ReSTIR branch |
| `SceneDepthCS` | compute | 3 | `SceneDepth` | live |
| `SkyMotionVectorCS` | compute | 5 | `SkyMotionVector` | live |
| `SkyCS` | compute | 8 | `Sky` | live |
| `ToneMappingCS` | compute | 4 | `ToneMapping` | live presentation branch |
| `VisualizeBuffersCS` | compute | 12 | `VisualizeBuffers` | live debug branch |

The only Phase 0 one-field pass schema was `ComputeClearPassParameters::Output`. It was not a justified strong handle, ABI wrapper, policy type, or extensible result: it duplicated `ComputeClearCS::FParameters::Output` and was deleted in Phase 2. Type-only shader parameter wrappers such as `StructuredBuffer<T>` are template vocabulary rather than one-property runtime records. No new one-field carrier is authorized by this plan.

### Pass, consumer, and collaborator dispositions

| Classification | Complete Phase 0 set | Frozen disposition |
| --- | --- | --- |
| direct one-shader compute wrappers | `ComputeClearPass`; `DirectLightingPass`; `DirectLightReservoirSpatialPass`; `DirectLightReservoirTemporalPass`; `DirectShadowSignalPass`; `DirectShadowSignalDeviceAddressPass`; `DirectShadowSignalNoRayQueryPass`; `ExposureDownsampleScenePass`; `ExposureDownsampleTexturePass`; `ExposureReduceScenePass`; `ExposureReduceTexturePass`; `ExposurePass`; `LightingCompositePass`; `LinearUpscalePass`; `OutputEncodingPass`; `PathTracedDirectLightingPass`; `PathTracedIndirectLightingPass`; `RaytracedGBufferPass`; `ReferenceLightingAccumulationPass`; `RestirIndirectResolvePass`; `RestirIndirectSpatialPass`; `RestirIndirectTemporalPass`; `SceneDepthPass`; `SkyMotionVectorPass`; `ToneMappingPass`; `VisualizeBuffersPass` | Phase 1 deletes the two catalog-only shadow variant wrappers with their shader roots. Phase 2 deletes the remaining 25 forwarding classes/files. Twenty-six Phase 0 Execute bodies only called sized compute forwarding; `ExposurePass` forwarded a fixed `1x1x1` dispatch. `AddComputeClearPass` may remain only as a narrow repeated graph-intent helper after it dispatches `ComputeClearCS` directly. |
| multi-stage graphics | `GBufferPass` using `GBufferVS` and `GBufferPS` | Retain and narrow in Phase 2. It owns target clears/binds, viewport/scissor/topology, mesh-batch traversal, material table binding, skeletal validity, two-sided/wireframe pipeline choice, and draw calls. Remove its package/runtime-definition/metadata boilerplate and shader-visible copies. |
| real focused collaborator | `GBufferMeshBatchDrawer` | Retain with explicit GBuffer draw inputs. It owns mesh/cache iteration and per-draw behavior; it must not become a shader/package/runtime service locator. |
| shaderless graph work | texture copy, buffer copy, `LightingTargetClear`, `RaytracedGBufferTargetClear`, `RayTracingSceneBuild`, external upscaler evaluation, external ray-reconstruction evaluation | Retain as graph resource operations or focused provider/RT collaborators. They declare resources but do not need a fake shader class, shader parameters, or forwarding pass wrapper. |
| RT scene capability | `RenderRayTracingScene::Build` and its prepared scene/view plan | Retain under the RT scene owner. It owns acceleration-structure preparation/lifetime, not shader-package or frame-graph infrastructure. |
| external provider capability | upscaler and ray-reconstruction graph-generation objects | Retain at the graph generation that owns provider evaluation/native interop; pass recording receives only declared resources. |

The live typed compute consumers are the semantic graph builders in `Passes/Debug/VisualizeBuffers.cpp`, `Passes/GBuffer/{RaytracedGBuffer,SceneDepth,SkyMotionVectors}.cpp`, `Passes/Lighting/Direct/{DirectLighting,DirectLightReservoir}.cpp`, `Passes/Lighting/LightingComposite.cpp`, `Passes/Lighting/Reference/{PathTracedDirectLighting,PathTracedIndirectLighting,ReferenceLightingAccumulation}.cpp`, `Passes/Lighting/Restir/{RestirIndirectResolve,RestirIndirectSpatial,RestirIndirectTemporal}.cpp`, `Passes/Lighting/Shadows/DirectShadowSignal.cpp`, `Passes/Lighting/Sky/Sky.cpp`, `Passes/PostProcessing/{Exposure,ExposureMomentChain}.cpp`, `Passes/Presentation/{LinearUpscaling,Presentation}.cpp`, and the compute-clear helper. `RasterizedGBuffer.cpp` is the GBuffer draw consumer. Phase 2 updates every one directly; it does not add old/new dispatch overloads.

Parameter-metadata consumers have one destination each:

- graph setup and resource declaration consume `Shader::Parameters` directly in Phase 2;
- `PassBinder` and typed parameter instances consume the same structural metadata in Phase 2, and count-only compatibility acceptance is deleted;
- `GlobalShaderRegistry`, `ShaderContractCatalogBuilder`, `ShaderContractValidator`, and `ShaderParameterStructCookVerifier` consume the registered shader schema until Phase 3 renames their compile-job/catalog surfaces and Phase 4 publishes the map/library result;
- runtime layout/pipeline materialization remains in `RenderPassRuntimeCache`; Phase 2 removes pass-class lookup from its frontend, while Phase 4 changes its backing package storage atomically;
- the GBuffer material table is consumed only by `GBufferPass`/`GBufferMeshBatchDrawer`; it is a real draw dependency, not justification for a generic pass context or duplicated shader schema.

### Field authority ledger

| Phase 0 field family | Phase 0 producer and consumers | Target owner | Exact phase |
| --- | --- | --- | --- |
| every live semantic shader `FParameters` field | concrete shader class; registry/catalog/verifier; pass-side mirror | same concrete shader class as nested `Parameters`; graph and binding use the same metadata | Phase 2 retain/rename |
| 103 authored `_NAMED` parameter declarations plus three `_NAMED` macro definitions; field/layout/shader name triples in parameter metadata | shader headers; Renderer metadata builder; RHI layout/package/reflection; ShaderCompiler verification/cooking | one member/binding `Name` from shader class through HLSL reflection and runtime binding | Phase 2 delete aliases and rename HLSL bindings atomically |
| 32 fields on the two catalog-only shadow variant schemas and their pass mirrors | duplicate registrations/wrappers with no graph producer | deletion; live `DirectShadowSignalCS::Parameters` owns the semantic AS binding and no-query behavior is rejected before graph construction | Phase 1 delete |
| remaining effective compute `*PassParameters` fields, including `DirectLightReservoirCommonParameters`, `DirectShadowSignalCommonPassParameters`, and `DirectShadowSignalRayQueryPassParameters` expansion | semantic graph builder copies values into pass record; Execute/binder reads it | corresponding concrete compute shader `Parameters` field with the same semantic name | Phase 2 delete mirrors |
| ten shader-visible `GBufferPassParameters` fields and all eighteen `GBufferDrawParameters` field uses | raster graph builder/pass/drawer | `GBufferVS::Parameters` and `GBufferPS::Parameters`, composed at the real draw owner without another schema | Phase 2 delete mirrors |
| seven GBuffer attachment fields | raster graph builder and GBuffer pass | narrow graph-only GBuffer envelope | Phase 2 retain/narrow |
| `RenderPassDefinition::PassName` and each pass `PassName` | pass class; graph diagnostics | shader-type-derived default or graph-instance `RenderPassLabel` override | Phase 2 delete/derive |
| `RenderPassDefinition::PipelineKind`, `AllowInputAssemblerInputLayout`, and `Graphics` | pass definition/runtime factory | typed compute dispatch or complete graphics draw description at graph setup | Phase 2 delete bag |
| `BindingLayoutDebugName` and `PipelineDebugName` in definition/runtime request types | pass class/runtime pipeline creation | derived bounded diagnostics at pipeline owner | Phase 2 delete authored fields |
| `RenderPassDefinition::ShaderPackage` | pass class forwards `ShaderPackageDefinition` | no pass field; typed shader lookup through active map | Phase 2 deletes the containing bag; Phase 4 deletes `ShaderPackageDefinition` itself |
| `RenderPassRuntimeCache::ShaderRuntimeGeneration::{Generation,ShaderPackages,RuntimeStorageByPassType}` | renderer runtime cache; frame identity and typed materialization | one retained generation containing active `GlobalShaderMap`, `CookedShaderLibrary`, and derived runtime objects | Phase 4 atomic refactor |
| `RenderFrameIdentity::ShaderPackageGeneration` and `GetShaderPackageGeneration` | renderer runtime cache/frame pipeline; view-history invalidation | renderer-owned shader generation with no package/cache wording | Phase 4 rename; behavior retained |
| package IDs, keys, expected stages/features, package paths, schema versions, package load reports/cache generations | registrations/cooker/RHI runtime/editor diagnostics | typed catalog/map entry, code hash/library record, and one renderer shader generation | Phase 1 deletes the two rejected shadow package identities; Phase 4 deletes/replaces the remaining package surface |
| package-targeted recook request/model fields, package registry/publication fields, package arguments, package columns, and package artifact-path discovery | Editor/Application/CLI | typed shader/map/library vocabulary without a package compatibility spelling | Phase 4 delete/replace |
| parallel recook/reload actions, manual normal-path reload, implementation-first table layout, artifact-directory scans, and duplicated status formatting | Editor/Application | semantic shader/source `Apply Changed`, expert typed shader target, immutable operation/provenance views | Phase 10 delete/replace after Phase 4 has removed package identity |

### Resource declaration and attachment ledger

At the Phase 0 checkpoint, `FrameGraphBuilder` and `FrameGraph` each exposed texture/buffer `Read` aliases beside texture/buffer `CreateSRV`, texture/buffer `CreateUAV`, acceleration-structure `Read`, `CreateRenderTarget`, and `CreateDepthTarget`. Production semantic graph setup contained 178 `CreateSRV`, 49 `CreateUAV` including the compute-clear graph helper, six `CreateRenderTarget`, one `CreateDepthTarget`, zero texture/buffer calls to `builder.Read`, and seven `builder.Read` calls that all bound `SceneTlas`. These author-facing counts excluded the then-current `*Pass.cpp` recording/binding bodies; those were inventoried separately as forwarding surfaces. This confirmed two separate clean breaks rather than a blanket rename:

- Phase 1 adds the one semantic `CreateAccelerationStructureBinding` route and updates all seven AS consumers. Backend descriptor type, address, and mutable-descriptor mechanics stay private to RHI lowering.
- Phase 2 deletes the duplicate texture/buffer `Read` declarations from both graph surfaces and retains only `CreateSRV`/`CreateUAV` for shader views plus `CreateRenderTarget`/`CreateDepthTarget` for raster attachments.
- `PassResourceBuilder::Read` and internal `resources.Read(...CopySource...)` remain declared-resource resolution/copy infrastructure. They are not shader-view authoring aliases and must not be renamed merely to satisfy a spelling search.

### Package, cook, runtime, publication, and frontend deletion ledger

Every old package definition, spelling, and consumer is owned below. A path named in one row is not deferred to a generic cleanup phase.

The frozen broad package floor, excluding documentation, third-party code, and generated artifacts, contains 942 matching lines across 187 source or build files for the exact case-insensitive expression `ShaderPackage|shader package|shader-package|\.sparkshader|ShaderPackageRegistry|ShaderRecookRequestType::PackageId|--package|inspect-package|PackageId|PackageKey|PackagePath`. The number is evidence for this revision, not a future API quota; the owner rows below are the stable deletion contract.

| Owner surface | Exact Phase 0 definitions/paths | Disposition |
| --- | --- | --- |
| Renderer authored identity | `RendererShaderPackages.h`, 28 constants, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, `ShaderPackageDefinition` references in registration/pass/runtime code | Phase 4 deletes package identity and changes remaining typed registrations/runtime lookup to map identity. Phase 2 may remove a containing pass bag but does not create a package replacement. |
| Renderer pass facade | `RenderPassDefinition`, `RenderPassGraphicsPipelineDefinition`, `RenderPassDefinitionPipelineKind`, `RenderPassDefinitionRuntime`, `RenderPassShaderRuntimeDesc`, generic `GetDefinition`/`GetParameterMetadata`, `ComputePassOperations`, and package/debug forwarding fields | Phase 2 deletes these pass-facing bags and forwarding surfaces. This is the only deletion phase for these definitions. |
| Renderer package-backed runtime | `PipelineRuntimePackageRequest`; package load/capability methods in `PipelineRuntimeLibrary`; `RenderPassShaderRuntimeStorage::ShaderPackage`; `CookedShaderPackageCache` and package-keyed pass-type holders in `RenderPassRuntimeCache`; reload/get-generation names | Phase 4 replaces storage and lookup atomically with map/library-backed shader references while preserving one cache owner, lazy graph-time materialization, replacement validation, and submission retirement. |
| RHI public package schema | `CookedShaderPackage.h`, `CookedShaderPackageContract.h`, `CookedShaderPackageIdentity.h`, `CookedShaderPackageCache.h`, `LoadedShaderPackage.h`, and `ShaderPackageLayoutBuilder.h` | Phase 4 deletes package containers/identity/cache. Only genuinely neutral code-record, reflection, parameter-layout, and RT metadata validation primitives survive under map/library-neutral names. |
| RHI private package implementation | `CookedShaderPackage.cpp`, `CookedShaderPackageReader.cpp`, `CookedShaderPackageValidation.cpp`, `CookedShaderPackageIdentity.cpp`, `CookedShaderPackageCache.cpp`, `LoadedShaderPackage.cpp`, `ShaderPackageLayoutBuilder.cpp`, plus package-shaped binding/RT validation entry points | Phase 4 deletes readers/cache/path/schema dispatch and renames or narrows only neutral validation used by map/library records. No old reader remains. |
| Core paths and extensions | `GetCookedShaderPackageRootPath`, `GetCookedShaderRegistryPath`, package path helpers, `ShaderPackageRegistry.sreg`, package directories, and `.sparkshader` extension/path construction | Phase 4 replaces publication paths with the generated map/library paths and deletes every old helper/extension consumer. |
| ShaderCompiler physical package output | `CookedPackageWriter`, `CookedRegistryWriter`, `CookedShaderPackageEmitter`, `CookedShaderPackageOutput`, `ShaderPackageCooker`, package output/registry/signal payload fields, package inspection, and package-shaped stats/debug fields | Phase 4 replaces them with deterministic map/library writers and provenance joins. Phase 3 separately renames compile work (`CookNode`, cook-plan/job records) without deleting physical package authority early. |
| CLI/build validation | `--package`, `inspect-package`, package help/diagnostics, `ValidateShaderCompilerCli.cmake` `.sparkshader` expectation, and recursive source-group membership of deleted/moved files | Package selection and `.sparkshader` validation are Phase 4 deletion; semantic shader targeting remains. Each code phase reconciles recursive glob/source-group results and explicit validation paths in the same CL. |
| Application package identity/publication | `ShaderRecookRequestType::PackageId`, package argument construction, package registry/signal payload fields and readers, `ReloadCookedShaders` package wording, and package-generation status text | Phase 4 replaces every package spelling and publication field with typed shader/map/library vocabulary. This occurs with the format switch, not in the later UX phase. |
| Editor package identity | package ID/layout columns, package-targeted handler/model fields, package artifact-path derivation, package help/autocomplete, and package-generation wording | Phase 4 deletes or replaces every package identity consumer. The UI may remain implementation-oriented until Phase 10, but it may not retain package vocabulary or readers. |
| Editor/Application workflow | parallel changed/global/selected/manual-reload actions, implementation-first table presentation, general artifact-directory scans, and duplicated lifecycle/status formatting | Phase 10 converges these already package-free surfaces on `Apply Changed`, immutable shader/source views, and contextual expert provenance. |
| ignored generated output | 56 `.sparkshader` files and two `ShaderPackageRegistry.sreg` files observed under `artifacts/dev/projects/{Shared,Showcase}/cooked/Shaders` | Phase 4 removes/regenerates these disposable products after the clean-break format switch. They are not source and must not be retained as compatibility fixtures. |

`Engine/Renderer` and `Engine/RHI` use recursive `CONFIGURE_DEPENDS` source membership; Renderer has a separate recursive shader-registration object-library glob. ShaderCompiler recursively includes `Private` and `Backends`, while `ValidateShaderCompilerCli.cmake` explicitly names a representative `.sparkshader`. Consequently a move/delete phase must verify both glob inclusion/exclusion and explicit validation paths: relying on recursive discovery alone is insufficient.

### Source/include and diagnostic dispositions

The 18 files under `Passes/Deferred` are `DirectLighting.hlsl`, `DirectLightReservoirSpatial.hlsl`, `DirectLightReservoirTemporal.hlsl`, `DirectShadowSignal.hlsl`, `DirectShadowSignalCommon.hlsli`, `DirectShadowSignalDeviceAddress.hlsl`, `DirectShadowSignalNoRayQuery.hlsl`, `GBufferPacking.hlsli`, `GBufferPS.hlsl`, `GBufferUtils.hlsli`, `GBufferVS.hlsl`, `LightingComposite.hlsl`, `MotionVector.hlsli`, `SceneDepth.hlsl`, `SceneDepthUtils.hlsli`, `Sky.hlsl`, `SkyMotionVector.hlsl`, and `VisualizeBuffers.hlsl`. Phase 1 deletes the two redundant shadow roots and moves the remaining sixteen files, registration source paths, root/relative includes, source resolver inputs, dependency/hash diagnostics, CMake/source-group presentation, and documentation spellings to semantic virtual-source ownership, then deletes `Passes/Deferred`.

At Phase 0, author-written identity/diagnostic repetition comprised 28 pass labels, 28 package constants, 28 binding-layout labels, and 28 pipeline labels. Phase 1 deletes the two rejected shadow identities completely; Phase 2 deletes or derives the remaining pass/layout/pipeline presentation; Phase 4 deletes the remaining package constants. Compiler/cook/runtime/frontend diagnostics that then said package ID/key/path/generation change to shader type, virtual source, target, code hash, map/library record, renderer generation, and when applicable RT composition/pipeline/table/effect identity in Phases 3 through 9. Diagnostic labels remain bounded presentation and never become lookup keys.

### Legacy-eradication search floor

The owning phase is incomplete while its exact floor returns a runtime/tool/build or active-document definition or consumer:

- Phase 1: `Passes/Deferred`, physical authored registration roots, project-first source shadowing, basename fallback, absolute authored shader includes, `DirectShadowSignalDeviceAddress*`, `DirectShadowSignalNoRayQuery*`, `SPARKLE_RAY_TRACING_SCENE_TLAS_DEVICE_ADDRESS`, `SPARKLE_RAY_TRACED_SHADOWS_DISABLED`, `DeviceAddressRayQuery`, `UsesAccelerationStructureDeviceAddress`, `RayTracingSceneTlasShaderAccessMode`, `SupportsShaderDeviceAddress`, `SupportsShaderDeviceAddressAccess`, `SupportsMutableDescriptorType`, `EnabledMutableDescriptorType`, `VK_EXT_mutable_descriptor_type`, shader/effect `SceneTlasGpuAddress*` fields, shader-visible raw-address conversion, and `Read(FrameGraphAccelerationStructureHandle)` as a shader-binding spelling.
- Phase 2: `TGlobalShader`, `TShaderRef`, nested `FParameters`, remaining `*PassParameters` mirrors, `RenderPassDefinition`, `RenderPassDefinitionRuntime`, `GetDefinition`, `GetParameterMetadata`, `ComputePassOperations`, the remaining 25 forwarding pass class names/files, authored `_BindingLayout`/`_Pipeline` strings, count-only parameter-layout acceptance, every `SHADER_PARAMETER_*_NAMED` / `SPARKLE_REGISTER_NAMED_GRAPH_SHADER_PARAMETER` spelling, parameter-field `LayoutName`/`ShaderName` aliases and reflection fallback, `Read(FrameGraphTextureHandle)`, `Read(FrameGraphBufferHandle)`, generic shader texture/buffer macros that conceal SRV/UAV kind, and neutral `RTV`/`DSV` authoring spellings.
- Phase 3: `CookNode`, cook-plan records used as compile identity, checkout-path-bearing include/option hashes, full-catalog changed fallback, and any persistent compiler-result/cache spelling.
- Phase 4: `RendererShaderPackages`, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, `ShaderPackageDefinition`, `BuildShaderPackageIdFromSourcePath`, `CookedShaderPackage`, `LoadedShaderPackage`, `ShaderPackageLayoutBuilder`, `PipelineRuntimePackageRequest`, `ShaderPackageGeneration`, `GetShaderPackageGeneration`, package readers/writers/cache/identity/path/schema dispatch, `ShaderPackageRegistry`, `.sparkshader`, `--package`, `inspect-package`, `ShaderRecookRequestType::PackageId`, and all package-named Application/Editor request/publication/model/help/diagnostic fields.
- Phase 5: `GraphicsShaderPipelineState`, `RasterPassPipelineRuntime`, one-value `RhiVertexLayoutKind`, eager base/wireframe/two-sided runtime bundles, caller-authored target formats/count/depth/sample facts, shader-pair-only graphics keys, generic binding-time view/material policy, duplicate target bind/clear or topology routes, ignored neutral descriptor fields, and compatibility draw overloads.
- Phase 6: compiler-only RT records/rejection, ambiguous `SupportsRayTracing`, duplicate RT registry, disabled RT public facade, backend/graph bypass, native identifiers outside backend-private RHI, compute-disguised trace, second runtime cache/generation, and stale pipeline/table generation acceptance.
- Phase 7: ambiguous `GBufferMode::Raytraced` execution meaning, duplicated GBuffer semantic/output/scene owners, two scheduled frontends, hidden strict fallback, and compatibility execution enum values.
- Phase 8: constant-zero Renderer TLAS contributions where production mapping is required, duplicate scene/SBT mapping, effect-local table authority, large/transient local records, unconditional table/TLAS rebuild, and divergent alpha policy.
- Phase 9: deep per-pass API selection, partial strict-frame scheduling, hidden automatic fallback, duplicate execution settings/plan/history, ambiguous algorithm-versus-API labels, and unclassified ray-query effects.
- Phase 10: parallel changed/global/selected/manual-reload controls, implementation-first table presentation, general artifact-directory scans, duplicate shader operation status/log formatting, normal-path manual reload, and native backend controls in the frontend. The Phase 4 package floor must remain clean.
- Phase 11: any residual or semantic equivalent of the Phase 1-10 floor, plus compatibility aliases/adapters/readers/writers, dual authorities, migration diagnostics, and renamed legacy owners.

Historical rationale may quote a rejected name only when explicitly marked historical; executable paths, current-state descriptions, target examples, and implementation prompts must use the vocabulary valid for their phase boundary.

### Baseline provenance and blocked final claims

The only revision-pinned Phase 0 evidence is this static source/build/document inventory at `5b0bd1469339897eef1fde3e5c9ab07137860d0f`. The ignored `Shared` and `Showcase` cooked outputs were observed on disk on 2026-08-23, but they do not embed evidence sufficient to bind them to that revision, build configuration, compiler versions, backend, adapter/driver, workload, command line, or capture. They are therefore artifact inventory only.

No pre-edit, revision-pinned D3D12 runtime capture, Vulkan runtime capture, paired-backend shader-cook result, shader-reload/retirement run, first-use pipeline timing, frame-time distribution, memory/size comparison, or external PIX/RenderDoc/Nsight identity trace was found with the complete provenance required by [Validation, Performance, and Evidence](../Engineering/Verification/ValidationAndEvidence.md). The candidate logs under `artifacts/validation/renderer-scene-view-frame-phase7` were inspected and rejected as a shader baseline: they do not bind revision/compiler/cook/capture/performance provenance, the final D3D12 stderr contains only unrelated level warnings, and the final Vulkan stderr records provider fallbacks followed by `VK_ERROR_DEVICE_LOST`. The corresponding Phase 12 runtime, capture, retirement, backend-parity, and performance-regression claims are **blocked unless Phase 12 acquires fresh evidence for the complete candidate**. Phase 0 makes no build, cook, runtime, visual, capture, or performance claim and does not treat an old log or the presence of generated files as a baseline.

### Ray-tracing Phase 0 extension

The same revision-pinned source audit froze the RT starting point. The table also records explicit post-Phase 6 through Phase 8 annotations where stated; those annotations describe source reachability, not executable proof:

| Surface | Frozen starting point or explicit later source annotation | Unified disposition |
| --- | --- | --- |
| inline traversal | one shared `RayTracingTraceQuery.hlsli` implements `RayQuery`, `TraceRayInline`, and `Proceed`; GBuffer, shadow, path/reference, and ReSTIR HLSL reach it through `RayTracingSceneTlas`, `PathTrace`, `PathLighting`, and shadow helpers | preserve as the first-class inline frontend and parity oracle; semantic kernels become shared effect owners in Phases 7-9 |
| renderer RT shader registrations | the generic `GlobalShader` path accepts all six RT stages; GBuffer and shadow product routes register ray-generation, miss, closest-hit, and alpha any-hit stages | Phase 12 owns focused intersection/callable and nonzero-local-record conformance without fake product stages or submitted test scaffolding |
| final RT metadata | the final global shader map and cooked code library now carry stage, export, payload, attribute, recursion, and local-record fields; the old package representation remains deleted | Retain the one final representation, but do not call it delivered while its concrete consumer and portable local-record ABI are absent |
| runtime behavior | the former valid-library rejection is removed; the existing runtime-generation owner materializes exact GBuffer/shadow compositions and pipelines, while each graph owns the immutable scene-plan table it captures | Phase 12 must prove both backends, reload, retirement, failures, and parity before runtime support is accepted |
| native execution | backend-private D3D12 state-object/identifier/table/dispatch and Vulkan pipeline/group-handle/table/dispatch source paths exist | Native correctness is unexecuted Phase 12 evidence; nonzero local records are not yet shader-visible on either backend |
| capability authority | acceleration-structure, inline-query, and native RT-pipeline mechanism fields are independent; the ambiguous whole-word `SupportsRayTracing` field is deleted | Overall Renderer RT-pipeline availability still needs one authority that also requires the compiler, map/library, typed graph, and concrete-effect route; hardware mechanism readiness alone cannot advertise product support |
| frame graph and runtime | one typed ray-tracing pass kind, shader-table resource declaration, neutral trace command, exact-composition runtime materialization, reload, and all-queue generation retirement source path exists | Close it with the concrete typed consumer and portable local-record contract; no second cache or graph bypass is allowed |
| scene/SBT mapping | one `RenderRayTracingScene`-owned `RayTracingShaderTablePlan` orders instances, geometry segments, Surface/ShadowVisibility ray types, and logical records; classic and partitioned builders query the same checked contribution | Phase 12 executes multi-instance/multi-geometry corruption, native mapping, and lifetime oracles; no effect or backend owns a parallel logical order |
| effect selection | `GBufferAlgorithm` selects rasterization versus ray tracing; `RayTracingExecutionMode` independently supplies Automatic, Inline, or Pipeline intent; `RayTracingGBufferExecutionPlan` stores only the derived active frontend and actionable reason | Phase 9 may lift compatible per-effect decisions into one whole-frame plan; it must not copy requested intent, readiness booleans, shader generation, or deep execution-policy branches into another holder |

`RayTracingGBuffer` is the first product parity route. Both frontends use the same prepared scene, TLAS, geometry/material buffers, view, output attachments, hit reconstruction, motion/depth conventions, and explicit rasterized-GBuffer alternative. Its pipeline now uses ray generation, miss, opaque closest hit, and alpha-tested closest-hit/any-hit groups with recursion depth one, global resources, and no local SBT data. `DirectShadowSignal` has matching inline and pipeline frontends over one request/visibility semantic kernel and uses the second scene ray contribution. Procedural intersection and callable support are proved through existing validation surfaces or a temporary local conformance harness removed before handoff rather than by fake empty stages in product effects or submitted test scaffolding.

At Phase 0, no revision-pinned inline D3D12/Vulkan parity capture, valid-library rejection transcript, compiler target matrix, native-feature absence report, or RT performance baseline with complete provenance was found. Those Phase 12 claims remain blocked pending fresh final-candidate evidence.

### Phase 0 evidence and Code Review gate

The following read-only command families were run from the repository root at the frozen revision. Each was paired with bounded reads of the defining owner and representative producer/consumer in the ledgers above:

```powershell
git status --short --branch
git diff --cached --name-only
git diff --name-status 44c2f192a82947d9dcdd0e4bbd7ba0cb1a7145e4..5b0bd1469339897eef1fde3e5c9ab07137860d0f -- Engine Tools
rg -n "IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE|IMPLEMENT_GLOBAL_SHADER|IMPLEMENT_RAY_TRACING_SHADER|IMPLEMENT_RAY_TRACING_HIT_GROUP" Engine/Renderer
rg -n "TGlobalShader|BEGIN_SHADER_PARAMETER_STRUCT|SHADER_PARAMETER_" Engine/Renderer
rg -n "PassName|GetDefinition|GetParameterMetadata|::Execute\(" Engine/Renderer/Private/Passes
rg -n "builder\.(Read|CreateSRV|CreateUAV|CreateRenderTarget|CreateDepthTarget)\(" Engine/Renderer/Private/Passes Engine/Renderer/Private/Frame
rg -n -i --glob "!**/ThirdParty/**" --glob "!artifacts/**" --glob "!Docs/**" "ShaderPackage|shader package|shader-package|\.sparkshader|ShaderPackageRegistry|ShaderRecookRequestType::PackageId|--package|inspect-package|PackageId|PackageKey|PackagePath" Engine Tools CMakeLists.txt cmake
rg --files Engine/Assets/Shaders/Passes/Deferred
rg --files artifacts/dev/projects -g "*.sparkshader" -g "ShaderPackageRegistry.sreg"
rg -n "ID3D12StateObject|SetPipelineState1|DispatchRays|vkCreateRayTracingPipelinesKHR|vkGetRayTracingShaderGroupHandlesKHR|vkCmdTraceRaysKHR" Engine/RHI Engine/Renderer --glob "!**/ThirdParty/**"
rg -n "DirectShadowSignal(DeviceAddress|NoRayQuery)|CanUseInlineRayQueryShadows|SceneTlasGpuAddress|RayTracingSceneTlasShaderAccessMode" Engine Tools
rg -n "Passes/Deferred|ShaderPackage|sparkshader|RendererSceneViewFrameArchitecture" Docs
```

| Phase 0 AC | Cheapest claim-falsifying evidence | Result |
| --- | --- | --- |
| every shader/pass/package field and material consumer has one target owner or deletion | exact declaration/use searches above; complete shader table, pass classification, field-authority ledger, and package owner ledger; representative reverse reads from graph consumers to registration/runtime owners | **PASS** - 29 shader classes/schemas, 28 pass surfaces, 392 nested fields, 352 authored pass-side shader-field copies, GBuffer material drawing, and every package surface have an explicit retained owner or one deletion phase |
| every forwarding pass and duplicate parameter schema has one Phase 1 or Phase 2 disposition | exact `PassName`/definition/metadata/Execute searches plus all 28 wrapper pairs; bounded Execute-body inspection | **PASS** - two unconsumed shadow variants are Phase 1 deletions; 25 remaining compute wrappers are Phase 2 deletions; `GBufferPass` alone is retained for demonstrated graphics/draw behavior |
| every package reader/writer/cache/identity/generation spelling has one Phase 1 or Phase 4 disposition | the exact 942-line/187-file package floor and the Renderer/RHI/Core/ShaderCompiler/CLI/Application/Editor/generated-output owner rows | **PASS** - the two rejected shadow identities are deleted in Phase 1 and every remaining package producer/consumer/format/generation spelling is deleted or replaced atomically in Phase 4; Phase 10 owns only package-free workflow simplification |
| every resource view/attachment, graphics-state, and RT item has one disposition | exact graph vocabulary, graphics state/key/variant/attachment/topology, RT macro/native-call, capability, TLAS contribution, direct-shadow, runtime-rejection, graph/runtime, and effect-selection searches plus bounded backend/consumer reads | **PASS** - resource routes are split between Phases 1 and 2; package-shaped RT scaffolding is deleted in Phase 4; graphics-state ownership is Phase 5; complete RT runtime/effect/table/selection work is owned by Phases 6-10; the target-state RT document owns no implementation task |
| missing baseline evidence is explicit | ignored-artifact inventory and bounded inspection of candidate `artifacts/validation/renderer-scene-view-frame-phase7` logs against the provenance standard | **PASS** - generated files are inventory only; invalid/unbound candidate logs are rejected; all final runtime/backend/capture/retirement/performance claims remain blocked for Phase 12 |
| frozen eradication floor covers exact and semantic leftovers | phase-by-phase exact floor plus alias/adapter/fallback/parallel authority/generated-format/build/frontend/document semantic searches required by the common delivery contract | **PASS** - every Phase 0 match is assigned to one later phase; no generic cleanup owner, compatibility checkpoint, permutation, precache, or persistent compiler-result store is authorized |
| scoped documentation and review gate | documentation diff inspection, local target/anchor resolution for touched documents, staging/scope checks, and `git diff --check`; review route: [SparkleEngine Code Review](../Engineering/Workflow/CodeReview.md) | **PASS** - documentation-only scope, no P0-P2 finding, no runtime/tool/build/generated edit, and no executable evidence claimed |

Phase 0 is documentation-only and has no runtime performance class. The target is not implemented by this evidence; it is now sufficiently owned, ordered, and falsifiable for Phase 1 to begin without an intermediate architecture or unassigned legacy cleanup.
