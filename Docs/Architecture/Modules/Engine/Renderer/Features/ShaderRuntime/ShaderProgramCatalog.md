# Renderer Shader Program Catalog

Status: capability snapshot; exact registered-program ledger; not a successful cook, pipeline-creation record, or release approval

Snapshot: 2026-09-06 at committed `master` revision `d236da11`; all files in `Engine/Renderer/ShaderRegistrations` reconciled with their typed shader declarations and principal frame-graph consumers, and `Engine/Renderer` is unchanged from the earlier `8414b5dc` audit; evidence `S` only

Scope: the exact Renderer global-program membership linked into the shader-contract target, including source, entry point, stage, consumer, traversal model, runtime target expectation, and important binding boundary

Owners: `Engine/Renderer/ShaderRegistrations` for registration membership, typed declarations and pass consumers in `Engine/Renderer`, and `Tools/Shaders` for compilation/publication

Compilation inventory: [Shader Compilation Capability Inventory](../../../../Tools/ShaderCompiler/README.md)

Cross-system coverage: [Graphics Feature Coverage Matrix](../../../../../CrossModule/GraphicsCoverageMatrix.md)

## Count And Meaning

There are exactly 35 registrations: 25 Compute, one Vertex, one Pixel, two RayGeneration, two Miss, two ClosestHit, and two AnyHit. A row means the program is registered in current source and build membership. It does not mean its two runtime variants cooked successfully or that a driver created and executed its pipeline.

## Utility, GBuffer, And Debug

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `ComputeClearCS` | `/Engine/Passes/Compute/ComputeClear.hlsl` | `main` | Compute | Generic frame-graph texture clear used for lighting/reservoir initialization. |
| `GBufferVS` | `/Engine/Passes/GBuffer/GBufferVS.hlsl` | `main` | Vertex | Raster GBuffer mesh pass; consumes mesh/instance/deformation data through the graphics layout. |
| `GBufferPS` | `/Engine/Passes/GBuffer/GBufferPS.hlsl` | `main` | Pixel | Raster GBuffer material evaluation; bindful eight-role material textures; opaque/alpha-mask coverage. |
| `RayTracingGBufferInlineCS` | `/Engine/Passes/RayTracing/RayTracingGBufferInline.hlsl` | `RayTracingGBufferInline` | Compute | Strict/automatic inline GBuffer adapter; requires inline ray query, TLAS, hit buffers, and fixed material texture array. |
| `RayTracingGBufferRGS` | `/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl` | `RayTracingGBufferRayGeneration` | RayGeneration | Native-pipeline GBuffer adapter; owns global typed parameters and trace dispatch. |
| `RayTracingGBufferMiss` | `/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl` | `RayTracingGBufferMiss` | Miss | Native GBuffer miss behavior; pipeline composition only, no independent pass. |
| `RayTracingGBufferClosestHit` | `/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl` | `RayTracingGBufferClosestHit` | ClosestHit | Records hit distance, instance ID, primitive index, and barycentrics; ray generation then runs the shared surface-store/evaluation path. |
| `RayTracingGBufferAnyHit` | `/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl` | `RayTracingGBufferAnyHit` | AnyHit | Alpha-mask rejection for native GBuffer traversal. |
| `SceneDepthCS` | `/Engine/Passes/GBuffer/SceneDepth.hlsl` | `main` | Compute | Converts frontend-specific device depth into common linear R32F scene depth. |
| `SkyMotionVectorCS` | `/Engine/Passes/GBuffer/SkyMotionVector.hlsl` | `main` | Compute | Completes background motion vectors from current/previous view transforms. |
| `VisualizeBuffersCS` | `/Engine/Passes/Debug/VisualizeBuffers.hlsl` | `main` | Compute | Reads the GBuffer and five lighting lobes for non-Lit view modes; output still enters presentation. |

## ReSTIR Direct Lighting And Shadows

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `DirectLightReservoirTemporalCS` | `/Engine/Passes/Lighting/Direct/DirectLightReservoirTemporal.hlsl` | `main` | Compute | Direct-light temporal candidate/reprojection stage; reads prior direct reservoir history. |
| `DirectLightReservoirSpatialCS` | `/Engine/Passes/Lighting/Direct/DirectLightReservoirSpatial.hlsl` | `main` | Compute | Direct-light spatial reuse stage. |
| `DirectShadowSignalCS` | `/Engine/Passes/Lighting/Shadows/DirectShadowSignal.hlsl` | `main` | Compute | Inline-query visibility adapter; TLAS/hit material/fixed texture table. |
| `DirectShadowSignalRGS` | `/Engine/Passes/Lighting/Shadows/DirectShadowSignalPipeline.hlsl` | `DirectShadowSignalRayGeneration` | RayGeneration | Native-pipeline visibility adapter and typed global parameters. |
| `DirectShadowSignalMiss` | `/Engine/Passes/Lighting/Shadows/DirectShadowSignalPipeline.hlsl` | `DirectShadowSignalMiss` | Miss | Native unoccluded result. |
| `DirectShadowSignalClosestHit` | `/Engine/Passes/Lighting/Shadows/DirectShadowSignalPipeline.hlsl` | `DirectShadowSignalClosestHit` | ClosestHit | Native opaque occluder result. |
| `DirectShadowSignalAnyHit` | `/Engine/Passes/Lighting/Shadows/DirectShadowSignalPipeline.hlsl` | `DirectShadowSignalAnyHit` | AnyHit | Native alpha-mask occluder rejection. |
| `DirectLightingCS` | `/Engine/Passes/Lighting/Direct/DirectLighting.hlsl` | `main` | Compute | Resolves reservoir plus visibility against directional, point, spot, and rect light buffers into direct lobes. |

## ReSTIR Indirect Lighting

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `RestirIndirectTemporalCS` | `/Engine/Passes/RayTracing/RestirIndirectTemporal.hlsl` | `main` | Compute | Inline-query indirect temporal stage; reads history, TLAS, hit/deformation/material/light/sky resources. |
| `RestirIndirectSpatialCS` | `/Engine/Passes/RayTracing/RestirIndirectSpatial.hlsl` | `main` | Compute | Inline-query spatial reuse stage over the current indirect reservoir. |
| `RestirIndirectResolveCS` | `/Engine/Passes/RayTracing/RestirIndirectResolve.hlsl` | `main` | Compute | Inline-query resolve; writes indirect lobes and the four DLSS RR guide targets. |

See [Indirect Lighting](../Lighting/IndirectLighting.md) for the algorithm, history, inputs, limits, and evidence boundary.

## Reference Direct And Indirect Lighting

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `PathTracedDirectLightingCS` | `/Engine/Passes/RayTracing/PathTracedDirectLighting.hlsl` | `main` | Compute | Reference direct sampler; inline ray query and fixed material texture table. |
| `PathTracedIndirectLightingCS` | `/Engine/Passes/RayTracing/PathTracedIndirectLighting.hlsl` | `main` | Compute | Reference indirect sampler; inline ray query, sky, deformation/hit/material inputs. |
| `ReferenceLightingAccumulationCS` | `/Engine/Passes/RayTracing/ReferenceLightingAccumulation.hlsl` | `main` | Compute | Motion/validity-aware RGBA32F reference-history accumulation; does not trace rays itself. |

The three ReSTIR-indirect and three reference traversal/accumulation programs have no registered RayGeneration/Miss/Hit equivalents. The native-pipeline claim must therefore remain limited to GBuffer and direct-shadow visibility. See the [Offline Path Tracer](../Lighting/OfflinePathTracer/README.md) for why the reference route is not yet an accepted oracle.

## Lighting Composite And Sky

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `LightingCompositeCS` | `/Engine/Passes/Lighting/LightingComposite.hlsl` | `main` | Compute | Combines direct diffuse/specular/subsurface, indirect diffuse/specular, and GBuffer material/emissive data into HDR scene color. |
| `SkyCS` | `/Engine/Passes/Lighting/Sky/Sky.hlsl` | `main` | Compute | Fills background using linear scene depth, sky texture, and view/sky uniforms. |

## Exposure

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `ExposureReduceSceneCS` | `/Engine/Passes/PostProcessing/ExposureReduceScene.hlsl` | `main` | Compute | First automatic-exposure reduction directly from scene color. |
| `ExposureReduceTextureCS` | `/Engine/Passes/PostProcessing/ExposureReduceTexture.hlsl` | `main` | Compute | Subsequent parallel reduction over a moments texture. |
| `ExposureDownsampleSceneCS` | `/Engine/Passes/PostProcessing/ExposureDownsampleScene.hlsl` | `main` | Compute | First mip-chain metering downsample from scene color. |
| `ExposureDownsampleTextureCS` | `/Engine/Passes/PostProcessing/ExposureDownsampleTexture.hlsl` | `main` | Compute | Subsequent mip-chain downsample over intermediate moments. |
| `ExposureCS` | `/Engine/Passes/PostProcessing/Exposure.hlsl` | `main` | Compute | Resolves manual/automatic settings, luminance moments, prior history, clamps, compensation, and adaptation into current 1x1 exposure/history. |

See [Exposure](../PostProcessing/DisplayPipeline/Exposure.md) for metering, history, scheduling, and evidence ownership.

## Image Reconstruction And Upscaling

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `LinearUpscaleCS` | `/Engine/Passes/Presentation/LinearUpscale.hlsl` | `main` | Compute | Engine baseline render-extent to output-extent resolve. External providers are not represented by a shader registration. |

DLSS Super Resolution and DLSS Ray Reconstruction are external provider evaluations rather than global shader registrations. See [Image Reconstruction and Upscaling](../PostProcessing/ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md).

## Tone Mapping

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `ToneMappingCS` | `/Engine/Passes/Presentation/ToneMapping.hlsl` | `main` | Compute | Exposure plus selected mapper converts HDR resolved scene color to display-linear RGBA16F. |

See [Tone Mapping](../PostProcessing/DisplayPipeline/ToneMapping.md) for the three operators and current no-bypass boundary.

## Presentation And Output

| Program | Virtual source | Entry | Stage | Runtime consumer and boundary |
| --- | --- | --- | --- | --- |
| `OutputEncodingCS` | `/Engine/Passes/Presentation/OutputEncoding.hlsl` | `main` | Compute | Applies Automatic/Linear/sRGB encoding into the linear-format counterpart of the selected output target before copy. |

## Explicitly Unregistered Post-Processing Stages

| Feature | Current program coverage | Owning dossier |
| --- | --- | --- |
| Color grading | No grading or LUT shader registration | [Color Grading](../PostProcessing/DisplayPipeline/ColorGrading.md) |
| Chromatic aberration | No lens/channel-distortion shader registration | [Chromatic Aberration](../PostProcessing/DisplayPipeline/ChromaticAberration.md) |
| Frame generation | No engine shader or external DLSS-G/frame-generation evaluation | [Frame Generation](../PostProcessing/ReconstructionAndGeneration/FrameGeneration.md) |

## Registered Stage Coverage Versus Vocabulary

| Stage | Registered programs | Current honest status |
| --- | ---: | --- |
| Compute | 25 | Broad engine workhorse; includes raster-adjacent, ray-query, lighting, history, debug, and presentation programs. |
| Vertex | 1 | Raster GBuffer only. |
| Pixel | 1 | Raster GBuffer only. |
| RayGeneration | 2 | Ray GBuffer and direct shadow only. |
| Miss | 2 | Same two pipeline compositions. |
| ClosestHit | 2 | Same two triangle-hit compositions. |
| AnyHit | 2 | Same two alpha-mask compositions. |
| Geometry, Hull, Domain | 0 | Compiler/RHI stage vocabulary without current Renderer registrations or pipeline consumers. |
| Intersection | 0 | Procedural-hit vocabulary exists; no current registered procedural geometry program. |
| Callable | 0 | Ray composition vocabulary exists; no current callable program. |
| Mesh, Task | 0 | RHI capabilities explicitly report unsupported on both backends. |

## Runtime Variant Closure

Each one of the 35 logical registrations must have both `DxilSm66` and `SpirV16` cooked entries before the current paired-backend runtime publication is complete. That is 70 logical registration-target entries, subject to content-blob deduplication in `CookedShaderLibrary.slib`. Other tool targets are explicit compiler vocabulary, not required runtime variants.

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
5. record candidate-bound results through [Capability Evidence](../../../../../../Plans/CapabilityEvidence.md) and [Feature Completion Reports](../../../../../../Acceptance/FeatureCompletionReports.md).
