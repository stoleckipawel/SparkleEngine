# Renderer Shader Program Catalog

**Status:** capability snapshot; exact registered-program ledger; not a successful cook, pipeline-creation record, or release approval

**Snapshot:** 2026-09- 19 at source input revision `a884b6946802e933fafc9fe4c6cdfb93c4cde7e4` plus the current visualization / presentation worktree; all files in `Engine/Renderer/ShaderRegistrations` reconciled with their typed shader declarations and principal frame-graph consumers; evidence `S` only

**Scope:** the exact Renderer global-program membership linked into the shader-contract target, including source, entry point, stage, consumer, traversal model, runtime target expectation, and important binding boundary

**Owners:** `Engine/Renderer/ShaderRegistrations` for registration membership, typed declarations and pass consumers in `Engine/Renderer`, and `Tools/Shaders` for compilation/publication

**Compilation inventory:** [Shader Compilation Capability Inventory](../../../../Tools/ShaderCompiler/README.md)

**Cross-system coverage:** [Graphics Feature Coverage Matrix](../../../../../CrossModule/GraphicsCoverageMatrix.md)

> [!TIP]
> Use this as an exact program registry, not a shader-system introduction. Search by program, virtual source, stage, or consumer. A registration row proves source/build membership only; it does not prove that cooking, lookup, pipeline creation, execution, or output succeeded.

**Current readiness:** **50/100** for the shader-system route this catalog feeds; registration membership is source evidence, not executable or delivery proof. See [Current Feature Readiness](../../../../../../Acceptance/CurrentReadiness.md#foundation-world-content-shaders-and-tools).

## Count And Meaning

There are exactly 35 registrations: 27 Compute, one Vertex, one Pixel, three RayGeneration, one Miss, one ClosestHit, and one AnyHit. A row means the program is registered in current source and build membership. It does not mean its two runtime variants cooked successfully or that a driver created and executed its pipeline.

## Utility, GBuffer, And Debug

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `ComputeClearCS` | `/Engine/Passes/Compute/ComputeClear.hlsl` | `main` | Compute | Generic frame-graph texture clear used for lighting/reservoir initialization. |
| `GBufferVS` | `/Engine/Passes/GBuffer/GBufferVS.hlsl` | `main` | Vertex | Raster GBuffer mesh pass; consumes mesh/instance/deformation data through the graphics layout. |
| `GBufferPS` | `/Engine/Passes/GBuffer/GBufferPS.hlsl` | `main` | Pixel | Raster GBuffer material evaluation; bindful eight-role material textures; opaque/alpha-mask coverage. |
| `RayTracingGBufferInlineCS` | `/Engine/Passes/ GBuffer /RayTracing/RayTracingGBufferInline.hlsl` | `RayTracingGBufferInline` | Compute | Automatically selected inline GBuffer adapter; requires inline ray query, TLAS, hit buffers, and fixed material texture array. |
| `RayTracingGBufferRGS` | `/Engine/Passes/ GBuffer /RayTracing/RayTracingGBufferPipeline.hlsl` | `RayTracingGBufferRayGeneration` | RayGeneration | Native-pipeline GBuffer adapter; owns global typed parameters and trace dispatch. |
| `RayTracingMaterialMiss` | `/Engine/RayTracing/RayTracingMaterialPipeline.hlsl` | `RayTracingMaterialMiss` | Miss | Shared full-hit miss behavior for native GBuffer, direct-shadow, and Reference traversal. |
| `RayTracingMaterialClosestHit` | `/Engine/RayTracing/RayTracingMaterialPipeline.hlsl` | `RayTracingMaterialClosestHit` | ClosestHit | Shared hit distance, instance ID, primitive index, barycentrics, and facing payload for native GBuffer, direct-shadow, and Reference traversal. |
| `RayTracingMaterialAnyHit` | `/Engine/RayTracing/RayTracingMaterialPipeline.hlsl` | `RayTracingMaterialAnyHit` | AnyHit | Shared alpha-mask rejection over the same material policy as inline traversal. |
| `SceneDepthCS` | `/Engine/Passes/GBuffer/SceneDepth.hlsl` | `main` | Compute | Converts frontend-specific device depth into common linear R32F scene depth. |
| `SkyMotionVectorCS` | `/Engine/Passes/GBuffer/SkyMotionVector.hlsl` | `main` | Compute | Completes background motion vectors from current/previous view transforms. |
| `GBufferVisualizationCS` | `/Engine/Passes/Visualization/GBufferVisualization.hlsl` | `main` | Compute | Reads only GBuffer products for the eight GBuffer visualization modes; output still enters presentation. |
| `LightingVisualizationCS` | `/Engine/Passes/Visualization/LightingVisualization.hlsl` | `main` | Compute | Reads five lighting lobes plus GBuffer alpha for lighting visualization modes; output still enters presentation. |
| `GpuSceneVisualizationCS` | `/Engine/Passes/Visualization/GpuSceneVisualization.hlsl` | `main` | Compute | Publishes the instance palette already authored into GBuffer base color for the GPU-scene visualization mode. |

## ReSTIR Direct Lighting And Shadows

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `DirectLightReservoirTemporalCS` | `/Engine/Passes/Lighting/Direct/DirectLightReservoirTemporal.hlsl` | `main` | Compute | Direct-light temporal candidate/reprojection stage; reads prior direct reservoir history. |
| `DirectLightReservoirSpatialCS` | `/Engine/Passes/Lighting/Direct/DirectLightReservoirSpatial.hlsl` | `main` | Compute | Direct-light spatial reuse stage. |
| `DirectShadowSignalCS` | `/Engine/Passes/Lighting/Shadows/DirectShadowSignal.hlsl` | `main` | Compute | Inline-query visibility adapter; TLAS/hit material/fixed texture table. |
| `DirectShadowSignalRGS` | `/Engine/Passes/Lighting/Shadows/DirectShadowSignalPipeline.hlsl` | `DirectShadowSignalRayGeneration` | RayGeneration | Native-pipeline visibility adapter and typed global parameters. |
| `DirectLightingCS` | `/Engine/Passes/Lighting/Direct/DirectLighting.hlsl` | `main` | Compute | Resolves reservoir plus visibility against directional, point, spot, and rect light buffers into direct lobes. |

## ReSTIR Indirect Lighting

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `RestirIndirectTemporalCS` | `/Engine/Passes/ Lighting / Restir / Indirect/RestirIndirectTemporal.hlsl` | `main` | Compute | Inline-query indirect temporal stage; reads history, TLAS, hit/deformation/material/light/sky resources. |
| `RestirIndirectSpatialCS` | `/Engine/Passes/ Lighting / Restir / Indirect/RestirIndirectSpatial.hlsl` | `main` | Compute | Inline-query spatial reuse stage over the current indirect reservoir. |
| `RestirIndirectResolveCS` | `/Engine/Passes/ Lighting / Restir / Indirect/RestirIndirectResolve.hlsl` | `main` | Compute | Inline-query resolve; writes indirect lobes and the four DLSS RR guide targets. |

See [Indirect Lighting](../Lighting/IndirectLighting/README.md) for the current prototype audit, target algorithm, history, inputs, limits, and evidence boundary.

## Reference Path Tracer Programs

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `ReferencePathTracerInlineCS` | `/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerInline.hlsl` | `ReferencePathTracerInline` | Compute | Inline RayQuery adapter that invokes the shared Reference transport kernel. |
| `ReferencePathTracerRGS` | `/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerPipeline.hlsl` | `ReferencePathTracerRayGeneration` | RayGeneration | Native-pipeline adapter that invokes the same Reference transport kernel and composes shared material hit shaders. |
| `ReferencePathTracerDisplayCS` | `/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerDisplay.hlsl` | `ReferencePathTracerDisplay` | Compute | Publishes the committed accumulation derivative to the ordinary viewport product. |

These rows are current source and registration membership only. Shader cooking, pipeline creation, four-route GPU parity, and accepted-reference behavior remain unproved. See the [Reference Path Tracer](../Lighting/ReferencePathTracer/README.md).

## Lighting Composite And Sky

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `LightingCompositeCS` | `/Engine/Passes/Lighting/LightingComposite.hlsl` | `main` | Compute | Combines direct diffuse/specular/subsurface, indirect diffuse/specular, and GBuffer material/emissive data into HDR scene color. |
| `SkyCS` | `/Engine/Passes/Lighting/Sky/Sky.hlsl` | `main` | Compute | Fills background using linear scene depth, sky texture, and view/sky uniforms. |

## Exposure

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `ExposureReduceSceneCS` | `/Engine/Passes/PostProcessing/Exposure/ExposureReduceScene.hlsl` | `main` | Compute | First automatic-exposure reduction directly from scene color. |
| `ExposureReduceTextureCS` | `/Engine/Passes/PostProcessing/Exposure/ExposureReduceTexture.hlsl` | `main` | Compute | Subsequent parallel reduction over a moments texture. |
| `ExposureDownsampleSceneCS` | `/Engine/Passes/PostProcessing/Exposure/ExposureDownsampleScene.hlsl` | `main` | Compute | First mip-chain metering downsample from scene color. |
| `ExposureDownsampleTextureCS` | `/Engine/Passes/PostProcessing/Exposure/ExposureDownsampleTexture.hlsl` | `main` | Compute | Subsequent mip-chain downsample over intermediate moments. |
| `ExposureCS` | `/Engine/Passes/PostProcessing/Exposure/Exposure.hlsl` | `main` | Compute | Resolves manual/automatic settings, luminance moments, prior history, clamps, compensation, and adaptation into current 1x1 exposure/history. |

See [Exposure](../PostProcessing/DisplayPipeline/Exposure.md) for metering, history, scheduling, and evidence ownership.

## Image Reconstruction And Upscaling

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `LinearUpscaleCS` | `/Engine/Passes/Presentation/Upscaling/LinearUpscale.hlsl` | `main` | Compute | Engine baseline render-extent to output-extent resolve. External providers are not represented by a shader registration. |
| `PointUpscaleCS` | `/Engine/Passes/Presentation/Upscaling/PointUpscale.hlsl` | `main` | Compute | Exact debug-view resolve that preserves authored scalar, normal, material-color, and category values across extents. |

DLSS Super Resolution and DLSS Ray Reconstruction are external provider evaluations rather than global shader registrations. See [Image Reconstruction and Upscaling](../PostProcessing/ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md).

## Tone Mapping

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `ToneMappingCS` | `/Engine/Passes/Presentation/Display/ToneMapping.hlsl` | `main` | Compute | Exposure plus selected mapper converts HDR resolved scene color to display-linear RGBA16F. |

See [Tone Mapping](../PostProcessing/DisplayPipeline/ToneMapping.md) for the three operators and the presentation-owned exact-debugbypass boundary.

## Presentation And Output

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `OutputEncodingCS` | `/Engine/Passes/Presentation/Display/OutputEncoding.hlsl` | `main` | Compute | Applies Automatic/Linear/sRGB encoding into the linear-format counterpart of the selected output target before copy. |

## Explicitly Unregistered Post-Processing Stages

| Feature | Current program coverage | Owning dossier |
| --- | --- | --- |
| Color grading | No grading or LUT shader registration | [Color Grading](../PostProcessing/DisplayPipeline/ColorGrading/README.md) |
| Chromatic aberration | No lens/channel-distortion shader registration | [Chromatic Aberration](../PostProcessing/DisplayPipeline/ChromaticAberration/README.md) |
| Frame generation | No engine shader or external DLSS-G/frame-generation evaluation | [Frame Generation](../PostProcessing/ReconstructionAndGeneration/FrameGeneration.md) |

## Registered Stage Coverage Versus Vocabulary

| Stage | Registered programs | Current honest status |
| --- | ---: | --- |
| Compute | 24 | Broad engine workhorse; includes raster-adjacent, ray-query, lighting, history, debug, and presentation programs. |
| Vertex | 1 | Raster GBuffer only. |
| Pixel | 1 | Raster GBuffer only. |
| RayGeneration | 3 | Ray GBuffer, direct shadow, and Reference Path Tracer entry points. |
| Miss | 1 | Shared material miss program consumed by all three pipeline compositions. |
| ClosestHit | 1 | Shared material triangle-hit program consumed by all three pipeline compositions. |
| AnyHit | 1 | Shared material alpha-mask program consumed by all three pipeline compositions. |
| Geometry, Hull, Domain | 0 | Compiler/RHI stage vocabulary without current Renderer registrations or pipeline consumers. |
| Intersection | 0 | Procedural-hit vocabulary exists; no current registered procedural geometry program. |
| Callable | 0 | Ray composition vocabulary exists; no current callable program. |
| Mesh, Task | 0 | RHI capabilities explicitly report unsupported on both backends. |

## Runtime Variant Closure

Each one of the 32 logical registrations must have both `DxilSm66` and `SpirV16` cooked entries before the current paired-backend runtime publication is complete. That is 64 logical registration-target entries, subject to content-blob deduplication in `CookedShaderLibrary.slib`. Other tool targets are explicit compiler vocabulary, not required runtime variants.

For native ray compositions, registration count is not sufficient. Runtime materialization additionally checks compatible ray metadata, the global parameter owner, hit-group composition, recursion/payload/attribute limits, and shader-table records. Miss/hit programs do not own an independent pass or root parameter structure.

## Publication And Lifetime

```text
typed declaration + registration membership + virtual source/entry/stage
  -> ShaderCompiler discovery and both runtime-target variants
  -> reflected parameter/RT metadata validation
  -> transactional cooked library publication
  -> Renderer whole-generation materialization
  -> graph/pipeline rebuild and atomic active-generation swap
  -> old program/pipeline/library retirement after last queue completion
```

The compiler/tool owns deterministic compilation and library publication. [Pipeline Materialization and Typed Binding](PipelineMaterializationAndTypedBinding.md) owns runtime ABI validation, pipeline/cache identity, whole-generation activation, and retirement. This catalog owns only exact registration membership and its required variant/stage accounting; a row is neither a cooked artifact nor a runtime-ready program.

## Acceptance Criteria

- `AC-SHD-01` — the catalog contains every and only CMake-linked Renderer registration, with unique logical name/source/entry/stage identity and an owning typed declaration/consumer or explicit utility role.
- `AC-SHD-02` — the derived stage totals equal the registration ledger and both required runtime targets exist for every logical program before paired-backend publication.
- `AC-SHD-03` — every registration declares the reflection, thread-group/stage, and ray metadata required for its owning typed declaration and consumer; runtime agreement is delegated to `AC-PIP-01`.
- `AC-SHD-07` — absent Geometry/Hull/Domain/Intersection/Callable/Mesh/Task stages remain explicit zero-registration capability cells and are not inferred from compiler/RHI vocabulary.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-SHD-01` | duplicate/remove registration or change source/entry/stage without catalog/consumer update | reconciliation fails before cook/publication | `CHK-SHD-01` |
| `FM-SHD-02` | omit one required DXIL/SPIR-V variant or corrupt declared reflection/RT metadata | catalog/cook reconciliation reports the exact program/target/metadata defect; runtime activation behavior stays pipeline-owned | `CHK-SHD-02` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-SHD-01` | mechanically enumerate CMake-linked registration objects and compare unique name/source/entry/stage/consumer rows and totals to this catalog | `AC-SHD-01`, `AC-SHD-02`, `AC-SHD-07`; `FM-SHD-01` |
| `CHK-SHD-02` | focused registration validation and two-target cook with one omitted/corrupted variant/declared-metadata fixture; compare the result to the catalog | `AC-SHD-02`, `AC-SHD-03`; `FM-SHD-02` |

This catalog contract is **defined but unproved**. It completes only exact membership and declared target/metadata accounting. Runtime ABI, materialization, generation, and retirement evidence is owned by [Pipeline Materialization and Typed Binding](PipelineMaterializationAndTypedBinding.md); tool publication evidence is owned by [Shader System Acceptance](../../../../../CrossModule/ShaderSystem/Acceptance.md).

## Change Checklist

When adding, removing, or changing a program:

1. update the typed declaration and `ShaderRegistrations` entry;
2. update every frame-graph producer/consumer and pipeline composition;
3. update this exact catalog and the stage counts in [Shader Compilation](../../../../Tools/ShaderCompiler/README.md);
4. run registration validation, both runtime-target cooks, reflection/ABI checks, and the smallest consuming runtime path;
5. record candidate-bound results through [Capability Evidence](../../../../CapabilityEvidencePlan.md) and [Feature Completion Reports](../../../../../../Acceptance/FeatureCompletionReports.md).
