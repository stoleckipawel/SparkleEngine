# Shader System Design Precedent

Status: research; external technical precedent and deferred pipeline-preparation options, not local architecture or implemented behavior

Responsibility: preserve the source-backed Unreal, NVIDIA, and AMD comparisons and the deferred PSO-preparation options that informed Sparkle's shader-system decisions

Authority boundary: [Shader System Architecture](../Architecture/CrossModule/ShaderSystem/README.md) owns the selected local design; the [delivery plan](../Plans/CrossModule/ShaderSystem.md) owns sequencing; code and executable build configuration own implemented behavior

Source review recorded: 2026-08-15; version-sensitive claims must be revalidated before they drive a new decision

Interpretation: local-sounding words such as “adopt,” “should,” “target,” and phase dispositions below record the conclusions and options of that review. They are historical rationale, not independent Sparkle requirements; follow the linked Architecture dossier for the accepted current design.

## External Precedent and What Sparkle Adopts

The sources below are precedents, not local implementation authority. Repository links are pinned to the reviewed revisions where possible.

### Unreal Engine as the Core Model

Epic's global-shader documentation registers a C++ shader type against a source file, entry point, and shader stage. Its example deliberately registers vertex and pixel shader types from the same file with different entry points, then retrieves instances by type from the Global Shader Map. `FShaderType` metadata also carries the source filename, function, frequency, permutation count, parameter metadata, and hooks for compile eligibility, environment modification, output validation, and precaching. A source file is therefore an input to a shader type, never the universal runtime identity.

Epic's `FShaderCompilerInput` gathers the read-only inputs for one compile, including the virtual source path, entry point, target, platform/format, environment, parameter metadata, debug information, and deterministic input identity. `FShaderCompileJob` pairs input, a logical job key, preprocess output, compiler output, and diagnostics; the logical `FShaderCompileJobKey` is not the full content hash. `FShaderCompilingManager` coordinates priorities, pending jobs, result application, cancellation, and worker processes. Epic explains that Shader Compile Workers provide process-level parallelism around compiler implementations that may otherwise serialize internally.

Unreal cooking collects unique shader code into shader libraries. At runtime `FShaderCodeLibrary` can open project/plugin libraries and test for code by hash. `FShaderMapResource` separately owns the render-resource lifetime. Sparkle adopts the logical shader map, cooked code library, and live RHI-object separation while deliberately omitting persistent storage of compiler results.

Unreal's Render Dependency Graph intentionally lets a shader parameter structure also describe the common one-to-one pass, while `FRDGBuilder::AddPass` still receives a separate event name for debugging and profiling. Epic also documents pass parameter structures without shader semantics, such as copy passes. Sparkle should therefore reuse or compose one authoritative shader-visible schema, not force every graph pass into one shader struct. Epic's parameter metadata exposes both a layout hash and a strong persistable layout signature; that is stronger than Sparkle's current independent Direct Lighting declarations plus count-only runtime fallback.

Unreal's shader frontend is the more important precedent for this migration: a concrete global-shader class declares its `FParameters`; the implementation macro binds source, entry, and stage; RDG allocates the same parameter type; and `FComputeShaderUtils::AddPass` consumes a typed shader reference, those parameters, and group count. Unreal also has optional shader-pipeline and PSO-precaching systems, but Sparkle does not adopt them in this plan because the current workload has not justified their authoring or runtime cost.

Adopt now:

- typed shader registration
- explicit virtual source path, entry point, and stage
- shader-class-local compile eligibility/environment/result-validation hooks only where currently needed
- complete immutable compile inputs, deterministic input hashes and logical job IDs, in-flight deduplication, priorities, and cancellation
- typed shader-map lookup and independently owned RHI shader lifetime
- compile-input hashes derived from the full compiler-affecting closure and compiler provenance
- cook-time collection and deduplication of global shader code
- a nested shader parameter contract reused directly by one-to-one graph dispatch, with a pass envelope only for real multi-shader or graph-only composition
- render-graph event labels kept separate from shader identity
- direct typed compute dispatch utilities and typed shader-map lookup

Do not copy yet:

- Unreal's full material, vertex-factory, and permutation universe
- authored shader-pipeline types, PSO precaching/prewarming, driver-cache integration, and preload/residency controls before measurements justify them
- engine-scale macro, plugin, chunking, and patching complexity where Sparkle has no corresponding workload

Sources:

- [Epic: Adding Global Shaders](https://dev.epicgames.com/documentation/en-us/unreal-engine/adding-global-shaders-to-unreal-engine)
- [Epic: `GetShaderSourceFilePath`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/GetShaderSourceFilePath)
- [Epic: `FShaderType`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderType)
- [Epic: `TShaderPermutationDomain`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/TShaderPermutationDomain)
- [Epic: `FShaderCompilerInput`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompilerInput)
- [Epic: `FShaderCompileJob`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompileJob)
- [Epic: `FShaderCompileJobKey`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompileJobKey)
- [Epic: `FShaderCommonCompileJob` and full `InputHash`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCommonCompileJob)
- [Epic: `FShaderCompilingManager`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FShaderCompilingManager)
- [Epic: Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [Epic: `FShaderCodeLibrary`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCodeLibrary)
- [Epic: `FShaderLibraryCooker`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderLibraryCooker)
- [Epic: `FShaderMapResource`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderMapResource)
- [Epic: `FShaderPipelineType`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderPipelineType)
- [Epic: `FShaderParametersMetadata`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderParametersMetadata)
- [Epic: Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Epic: `FRDGBuilder::AddPass`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRDGBuilder/AddPass)
- [Epic: PSO Precaching](https://dev.epicgames.com/documentation/en-us/unreal-engine/pso-precaching-for-unreal-engine)
- [Epic: Debugging the Shader Compile Process](https://dev.epicgames.com/documentation/en-us/unreal-engine/debugging-the-shader-compile-process-in-unreal-engine)

#### Graphics Pipeline and Mesh-Draw State Study

The review followed Epic's documented entry point at `FDeferredShadingSceneRenderer::Render`, then followed the source-derived API responsibilities into mesh-pass processing, RDG attachment extraction, pipeline-state assembly, and RHI binding. No licensed Unreal source checkout is present in the reviewed workspace, so this plan does not claim a line-by-line local reading of private implementation bodies. It relies on Epic's official architecture documentation and generated API pages, whose source locations identify `DeferredShadingRenderer.cpp`, `MeshPassProcessor.h/.cpp`, `RenderGraphResources.h/.inl`, `PipelineStateCache.h/.cpp`, and RHI headers. NVIDIA NVRHI provides the independent primary-source cross-check.

| Read path | Production responsibility observed | Sparkle conclusion |
| --- | --- | --- |
| `FDeferredShadingSceneRenderer::Render` and documented base-pass flow | sequences scene-rendering phases and calls focused draw systems | frame/feature orchestration does not author a backend-complete PSO |
| `FMeshBatch` -> `FMeshPassProcessor` -> `BuildMeshDrawCommands` | filters mesh work, selects shaders and pass state, and builds stateless draw commands | keep a focused GBuffer mesh collaborator; do not make graph construction enumerate native pipeline fields |
| `FMeshPassProcessorRenderState` | carries only pass-wide blend/depth-stencil/access/stencil and uniform-buffer overrides, then applies them to PSO state | adopt the narrow-state idea as an even smaller `RasterPassRenderState`; attachment access remains graph-owned and the complete aggregate is deleted |
| `FGraphicsMinimalPipelineStateInitializer` | holds mesh-draw shader/fixed-function state without render-target state | key shader, raster, depth/stencil, blend, and primitive facts separately from attachments |
| RDG `RENDER_TARGET_BINDING_SLOTS`, `FGraphicsPipelineRenderTargetsInfo`, and `ExtractRenderTargetsInfo` | make attachments authoritative for formats, count, depth/stencil access, load/store, and samples | derive only formats/count/depth/sample compatibility from resource descriptions; keep load/store/clear/access in graph execution and validate them against pass depth/stencil intent |
| `FGraphicsPipelineStateInitializer`, `SetGraphicsPipelineState`, and pipeline-state cache | assemble/cache/bind the complete state required by the RHI | retain a complete internal descriptor and key; never require feature authors to fill it |
| `FMeshDrawCommand` submission | binds shader parameters, vertex streams, cached/minimal pipeline identity, topology, stencil reference, and draw arguments | prepared draw work contributes mesh facts; dynamic recording data is not stored in a frontend PSO bag |
| NVRHI `GraphicsPipelineDesc`, `FramebufferInfo`, and command `GraphicsState` | separate reusable pipeline state, attachment compatibility, and per-command binding state | Sparkle's split is not Unreal-specific and maps cleanly onto both backend APIs |

The important Unreal lesson is where completeness occurs. The RHI initializer is intentionally large because a native graphics pipeline is a compound object. The authored mesh-pass surface is intentionally smaller because no single feature caller owns all compound facts. Sparkle should reproduce that ownership split, not copy Unreal prefixes, its full material/vertex-factory/permutation system, cached mesh-draw-command framework, or PSO-precache machinery.

The current Sparkle Phase 2 checkpoint exposes the following incomplete ownership:

| Current surface | Problem | Owning disposition |
| --- | --- | --- |
| `GraphicsShaderPipelineState` at `RasterizedGBuffer` | repeats six color formats, count, depth format, vertex layout, and fixed state next to graph declarations | Phase 5 deletes it; the graph, mesh/material, and pass state become the separate authorities |
| `FrameGraphBuilder::Draw(..., pipelineState, ...)` | forces the graph caller to construct a backend-shaped aggregate | Phase 5 accepts only typed shaders, narrow render state, and real draw work |
| `RenderPassRuntimeCache` keyed only by shader pair | rejects a second legitimate state for the same vertex/pixel shaders | Phase 5 introduces a complete immutable key assembled from all contributing authorities |
| `RasterPassPipelineRuntime` base/wireframe/two-sided bundle | eagerly creates speculative variants for every graphics runtime | Phase 5 lazily materializes only exact requested variants before recording; it does not add precaching |
| generic `ShaderPassOperations` view-mode selection | leaks semantic wireframe policy into generic shader binding | Phase 5 moves fill/cull choice to the mesh/material/pass owner and leaves binding semantic-free |
| `GBufferMeshPass::PrepareTargets` plus graph attachments | manually binds and clears targets already declared by the graph | Phase 5 makes attachment actions authoritative and removes the duplicate target preparation path |
| `GBufferMeshPass::ConfigurePipeline` plus `GpuMesh::Bind` | sets triangle topology in two owners | Phase 5 leaves topology with prepared mesh draw work exactly once |
| public RHI fields plus backend defaults | omits authored blend/sample authority while D3D12/Vulkan hard-code opaque, sample-count-one, triangle, and raster defaults | Phase 5 completes the neutral descriptor and makes paired backends consume every supported field or reject it loudly |

The target state contribution is therefore:

```text
typed shader references ----------- shader code + binding-layout identity --+
RasterPassRenderState ------------- blend + depth/stencil -------------------+
prepared mesh/material draw ------- vertex input + topology + fill/cull -----+--> GraphicsPipelineKey
frame-graph attachments ------------ target/depth formats + samples ----------+        |
                                                                                      v
                                                                     complete GraphicsPipelineDesc
                                                                                      |
                                                                        D3D12 / Vulkan object

dynamic command state: viewport, scissor, blend constants, stencil reference,
vertex/index streams, parameter bindings, and draw arguments
```

Pipeline materialization remains lazy but must finish before parallel command recording begins. The graph/runtime owner collects the finite exact keys requested by prepared draw work, resolves them once per active shader-map generation, then recording performs lookup and binding only. This is not precaching, prewarming, or a readiness system: no unrequested wireframe/two-sided combination is created, no driver cache is introduced, and no speculative future variant becomes an authored obligation.

Primary graphics sources:

- [Epic: Graphics Programming Overview and `FDeferredShadingSceneRenderer::Render`](https://dev.epicgames.com/documentation/en-us/unreal-engine/graphics-programming-overview-for-unreal-engine)
- [Epic: Mesh Drawing Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/mesh-drawing-pipeline-in-unreal-engine)
- [Epic: `FMeshPassProcessor`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FMeshPassProcessor)
- [Epic: `FMeshPassProcessorRenderState`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FMeshPassProcessorRenderState)
- [Epic: `FMeshDrawCommand`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Renderer/FMeshDrawCommand)
- [Epic: `FGraphicsMinimalPipelineStateInitializer`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FGraphicsMinimalPipelineStateIni-)
- [Epic: `FGraphicsMinimalPipelineStateId`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FGraphicsMinimalPipelineStateId)
- [Epic: `TStaticDepthStencilState`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/TStaticDepthStencilState)
- [Epic: `FGraphicsPipelineRenderTargetsInfo`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FGraphicsPipelineRenderTargetsIn-)
- [Epic: `ExtractRenderTargetsInfo`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/ExtractRenderTargetsInfo)
- [Epic: `FGraphicsPipelineStateInitializer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FGraphicsPipelineStateInitialize-)
- [Epic: `SetGraphicsPipelineState`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/SetGraphicsPipelineState/2)
- [NVIDIA NVRHI programming guide: pipelines, framebuffers, and graphics state](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [NVIDIA NVRHI tutorial: pipeline and framebuffer separation](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/Tutorial.md)

For ray tracing specifically, current Unreal 5.8 keeps the same shader-type center but separates three responsibilities that Sparkle must also keep separate:

1. `FShaderType`/`FShader` carry root-parameter metadata, compile hooks, and a ray-tracing payload hook.
2. `FRayTracingPipelineStateInitializer` relates ray-generation, miss, hit-group, and callable shader tables plus the shared binding layout. It is pipeline composition, not another shader declaration system.
3. `RayTraceDispatch` receives the materialized pipeline, selected ray-generation shader, shader binding table, global parameters, and dimensions. Unreal 5.5 began replacing the older ray-tracing binding writer with the normal batched shader-parameter path; Sparkle should start with its one typed `Parameters` path and copy no deprecated writer layer.

Sparkle retains those responsibilities but removes repetition that its smaller frontend does not need: `GlobalShader` stage classes, ray-generation-owned root `Parameters` and shared payload/attribute/recursion compile contract, one focused typed `RayTracingPipelineComposition` containing only membership and hit-group policy, and `TraceRays<RayGenerationShader>`. The compiler validates each registered stage contract, while materialization derives the composition ABI from the authoritative ray-generation registration and map entry. The frame graph/runtime retains the materialized pipeline, table, and binding state that Unreal's lower-level RHI call receives explicitly. This preserves Unreal's authoring experience while respecting Sparkle's graph and RHI ownership.

Primary sources:

- [Epic: `FShader` root parameters and ray-tracing payload hook](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FShader)
- [Epic: `FRHIRayTracingShader` stage hierarchy](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRHIRayTracingShader)
- [Epic: `FRayTracingPipelineStateInitializer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FRayTracingPipelineStateInitiali-)
- [Epic: `RayTraceDispatch`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRHIComputeCommandList/RayTraceDispatch)
- [Epic 5.5 release note for batched ray-tracing shader parameters](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-5-release-notes)

### NVIDIA Donut and NVRHI

Donut's `ShaderFactory` resolves bytecode using a filename plus entry point and selects an embedded DXBC, DXIL, or SPIR-V variant when available. It may use the filename as the shader object's default debug name. Its deferred-lighting implementation nevertheless creates the shader from `deferred_lighting_cs.hlsl` and separately emits the semantic GPU marker `DeferredLighting`.

NVRHI keeps `shaderType`, `debugName`, and `entryName` as distinct fields in `ShaderDesc`; a shader library resolves a shader by entry name and shader type. This is useful evidence that the low-level RHI description should not be responsible for Sparkle's source-package authoring policy.

NVRHI's RT API also confirms that pipeline description, shader table, command state, and ray dispatch are separate mechanism objects. Sparkle adopts that separation and paired D3D12/Vulkan contract, but not NVRHI's name-addressed mutable/versioned shader table. Sparkle's Renderer owns typed logical records, and backend-private RHI materializes an immutable table tied to one exact pipeline generation.

Adopt:

- source plus entry point as load/compile inputs
- automatic backend-bytecode selection
- separate semantic GPU markers and debug names
- a narrow RHI descriptor that consumes bytecode instead of inventing renderer package ownership

Sources:

- [NVIDIA Donut `ShaderFactory.cpp` at `bfdebdd`](https://github.com/NVIDIA-RTX/Donut/blob/bfdebdd7dd5455c503b2737a1967a4ef651c145b/src/engine/ShaderFactory.cpp)
- [NVIDIA Donut `DeferredLightingPass.cpp` at `bfdebdd`](https://github.com/NVIDIA-RTX/Donut/blob/bfdebdd7dd5455c503b2737a1967a4ef651c145b/src/render/DeferredLightingPass.cpp)
- [NVIDIA NVRHI `nvrhi.h` at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/include/nvrhi/nvrhi.h)
- [NVIDIA NVRHI programming guide at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [NVIDIA NVRHI ray-tracing tutorial at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/Tutorial.md)

### AMD Cauldron and FidelityFX

Cauldron's reviewed D3D12 paths compile shaders with an explicit file, entry point, target profile, and define list. It hashes shader source recursively through includes and hashes defines. Its rendering code uses separate user-marker strings such as `GltfPbrPass::DrawBatchList`.

FidelityFX uses an effect-specific pass enum and permutation flags to select generated shader blobs, then gives the created pipeline a separate human-readable name. Its useful precedent for Sparkle is only the separation of executable identity from the human-readable pipeline label. Sparkle's unified migration does not adopt FidelityFX's effect/pass enum or generated-variant selector; the concrete shader class and typed draw/dispatch/trace already provide the required identity.

AMD's reviewed Vulkan capability path keeps ray-tracing-pipeline and ray-query feature structures distinct, and FidelityFX's denoiser sample keeps reusable ray construction/traversal/material helpers in shared HLSL. Sparkle adopts those two boundaries: independent capability truth and shared semantic kernels with thin execution frontends. Cauldron/FidelityFX do not provide a typed engine shader-class frontend comparable to Unreal's, so they are not evidence for adding another authoring macro, pass enum, program alias, or string-addressed pipeline layer.

Adopt:

- include-aware and define-aware compile identity
- typed shader selection at draw/dispatch
- separate pipeline/debug presentation names

Study generated permutation lookup only in the separately approved future permutation proposal.

Sources:

- [AMD Cauldron `ShaderCompiler.h` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/common/base/ShaderCompiler.h)
- [AMD Cauldron `ShadowResolvePass.cpp` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/PostProc/ShadowResolvePass.cpp)
- [AMD Cauldron `GltfPbrPass.cpp` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/GLTF/GltfPbrPass.cpp)
- [AMD FidelityFX optical-flow program selection at `60f4ea8`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/FidelityFX/framegeneration/fsr3/internal/ffx_opticalflow.cpp)
- [AMD Cauldron ray-pipeline/ray-query capability separation at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/VK/base/ExtRayTracing.cpp)
- [AMD FidelityFX shared ray-tracing HLSL at `60f4ea8`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Samples/Denoisers/FidelityFX_Denoiser/dx12/shaders/raytracing_common.hlsl)

### Acceleration-Structure Binding Decision

The Phase 0 baseline shadow duplication was not a legitimate shader-authoring variant. `DirectShadowSignalCS` and `DirectShadowSignalDeviceAddressCS` executed the same entry and shared algorithm; the latter existed only to define `SPARKLE_RAY_TRACING_SCENE_TLAS_DEVICE_ADDRESS`, omit the semantic `SceneTlas` parameter, and reconstruct an opaque acceleration structure from two address words stored in an effect uniform. The separate class, source root, pass wrapper, package, feature flags, parameter schema, address fields, and selection enum were one backend representation leaking through every frontend layer.

Production API precedent removes the reason for that leak:

- DirectX declares one HLSL `RaytracingAccelerationStructure` resource. D3D12 may bind it through an acceleration-structure SRV or a root SRV whose native value is a GPU virtual address; the HLSL shader identity does not change.
- NVRHI exposes one semantic `ResourceType::AccelStruct` binding and lets its D3D12/Vulkan implementations materialize API-specific layouts and bindings.
- Vulkan defines both `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` and `VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV`. `VkWriteDescriptorSetPartitionedAccelerationStructureNV` carries the PTLAS device address inside the native descriptor write. Therefore a PTLAS address is backend descriptor data, not an HLSL constant or a reason for another shader class.
- Sparkle already resolves an acceleration-structure binding to a classic or partitioned native descriptor in Vulkan command recording. The missing contract is to select and validate that completed descriptor route consistently instead of advertising only the raw-address alternative.

The frozen target is one `SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)` field on `DirectShadowSignalCS` and every other shader that traces the scene. The graph binds one `FrameGraphAccelerationStructureHandle`. The selected TLAS provider is fixed before shader layout/pipeline materialization, and private RHI chooses the exact native descriptor type and write structure. Renderer shader/effect code never receives a TLAS GPU address or access-mode enum. If a provider cannot complete that semantic binding on the active device, that provider is unavailable and selection retains the supported classic provider; Sparkle does not compile a second shader or silently branch on a raw address.

The Phase 0 Vulkan mutable-descriptor feature/bootstrap/layout scaffold had no independent consumer outside this attempted classic/partitioned binding switch. Because provider selection is fixed before layout creation, Phase 1 deletes that scaffold and creates the exact descriptor layout for the selected provider. Sparkle does not retain a device feature, `pNext` chain, or generalized layout mechanism for hypothetical run-time switching.

This is not the postponed permutation system. The catalog and map retain one code record per `(ShaderTypeId, Target)`, the shader author writes no define or mode, and the shader bytecode does not fork for the binding representation. A future backend that truly cannot implement the semantic binding must extend the RHI capability and binding contract with evidence; it may not create `*DeviceAddressShader`, `*DescriptorShader`, or an internal pseudo-permutation by convention.

The no-query shadow program is deleted rather than generalized. Shadow visibility consumed by direct lighting is mandatory, so a shaderless clear is not a valid producer and the user cannot disable the only real frontend. Phase 1 requires inline-query capability before graph construction and schedules the one `DirectShadowSignalCS` producer unconditionally. Phase 8 replaces that inline-only precondition with selection between inline query and a complete pipeline/RGS implementation; absence of both still fails before scheduling. A compute shader that recompiles the same lighting code only to skip traversal is equally invalid.

Primary sources:

- [Microsoft DXR acceleration-structure resource binding](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [NVIDIA NVRHI acceleration-structure binding model at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [Khronos `VK_NV_partitioned_acceleration_structure`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_NV_partitioned_acceleration_structure.html)
- [Khronos partitioned-AS descriptor write](https://docs.vulkan.org/refpages/latest/refpages/source/VkWriteDescriptorSetPartitionedAccelerationStructureNV.html)

### Compiler, Capture, and Native Pipeline Precedent

Epic's development workflow treats a reproducible compile as a product feature: a debug dump contains the contributing sources and includes, preprocessed source, and a command file that replays the compiler invocation. Epic's cooked-shader debugging workflow can generate symbols separately from runtime shader data, so making a shader inspectable does not require permanently bloating the shipped bytecode.

The external GPU tools reinforce that provenance requirement. PIX resolves separate DXIL debug data by a compiler-suggested hash name and can show source and compile arguments from slim PDBs. NVIDIA Nsight Graphics needs source-level debug information and line mappings to correlate DXIL or SPIR-V hotspots and crashes back to HLSL. AMD Radeon GPU Analyzer inspects target ISA, register pressure, LDS, and scratch use. Sparkle therefore needs a bytecode-hash-to-source-symbol record and an opt-in analysis build, not only a text disassembly produced during a successful cook.

The native APIs also reinforce the separation between shaders and pipelines. Microsoft exposes cached D3D12 pipeline state and Vulkan exposes pipeline caches, while Epic layers a renderer-level precache policy above complete descriptors. Those are useful future precedents, not base-migration requirements. The current migration preserves correct lazy materialization before command recording and measures it; it does not add caches or precache orchestration without a demonstrated product hitch.

Adopt:

- write a self-contained replay bundle for failed as well as successful compile jobs
- store source mappings and separate debug symbols by shader/code hash so PIX, RenderDoc, and Nsight can resolve the exact cooked binary
- keep symbols and analysis products outside the lean runtime artifact unless an explicit development mode embeds them
- make representative DXIL and SPIR-V inspection first-class verification outputs
- feed exact shipped compile requests to RGA, Nsight, PIX, or equivalent analysis rather than recompiling an approximate shader by hand
- preserve complete D3D12/Vulkan pipeline descriptions and measure current lazy materialization so a later cache/precache proposal has evidence

Sources:

- [Epic: Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [Epic: Shader Debugging Workflows](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-debugging-workflows-unreal-engine)
- [PIX: automatic shader PDB resolution](https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/)
- [RenderDoc: shader debug-symbol handling](https://github.com/baldurk/renderdoc/releases)
- [NVIDIA: using shader debug information with Nsight Graphics](https://developer.nvidia.com/blog/harness-powerful-shader-insights-using-shader-debug-info-with-nvidia-nsight-graphics/)
- [NVIDIA: Nsight Graphics shader compilation guidance](https://developer.nvidia.com/docs/drive/drive-os/7.0.3/public/nsight/nsight-graphics/UserGuide/index.html)
- [AMD: Radeon GPU Analyzer](https://gpuopen.com/rga/)
- [Microsoft: managing D3D12 pipeline state](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)
- [Microsoft: D3D12 pipeline-state cache sample](https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-pipeline-state-cache-sample-win32/)
- [Khronos: Vulkan pipeline cache](https://docs.vulkan.org/guide/latest/pipeline_cache.html)

## Deferred PSO Prewarming Research

The strategies below are retained only as future decision context. None is selected or implemented by the base phases.

| Strategy | Strength | Cost/failure mode | Recommendation |
| --- | --- | --- | --- |
| compile synchronously on first use | minimal startup and enumeration work | visible frame hitch and nondeterministic driver work | Reject for required or common pipelines. Retain only as a classified late path. |
| enumerate every theoretical PSO | strongest nominal coverage | combinatorial cook/startup/memory bloat and unused work | Reject. A future proposal may consider only measured, reachable pipeline descriptions. |
| explicit precache declarations | deterministic and reviewable for bounded shaders | authors/collectors can omit a legal state | Defer until measured first-use evidence requires it. |
| recorded/bundled usage cache | captures content combinations actually exercised | misses unplayed branches and can go stale across shader/content changes | Add with material/content PSOs; use stable descriptions, never raw transient hashes alone. |
| native driver cache only | little engine work | opaque coverage, device/driver invalidation, first-run hitches | Use as an acceleration layer, never as the only plan. |
| hybrid explicit + recorded + native | covers known global state, observed content, and repeated driver work | most coordination and telemetry | Research option only; far beyond current need. |

A future prewarming proposal must define its own minimal state/evidence contract. This migration adds none of those states or diagnostics.
