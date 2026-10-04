# Renderer Geometry, Materials, and GBuffer

**Status:** current feature dossier; source-backed, not visual fidelity, raster/ray parity, performance, or release evidence

**Verified:** 2026-09-06 against committed `master` revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

**Scope:** `REN-SCENE-03` through `REN-SCENE-10`, `REN-MAT-01` through `REN-MAT-10`, `REN-GBUF-01` through `REN-GBUF-08`, and `REN-FRONT-01` through `REN-FRONT-07`

**Current readiness:** **50/100** for the current raster/GBuffer scope — integrated source paths exist; PBR/content/motion/backend/visual and draw-cost evidence does not. See [Current Feature Readiness](../../../../../../Acceptance/CurrentReadiness.md#renderer).

## At A Glance

| Axis | Current coverage | Explicit limit |
| --- | --- | --- |
| geometry | static, instanced, skinned, morphed, and combined skin/morph triangle meshes | no procedural/intersection, tessellation, mesh/task, or general non-triangle path |
| material | opaque and alpha-tested base color, normal, roughness/metallic/F0, emissive, AO, subsurface, textures | transparent/transmissive and broader lobes are incomplete or absent |
| frontend | raster GBuffer or capability-gated ray primary visibility | wireframe is raster-only; raster/ray parity remains unproved |
| products | shared BaseColor, Normal, Material, Emissive, Subsurface, DeviceZ, MotionVector meanings | successful attachment writes do not establish PBR or motion correctness |
| downstream join | lighting, temporal reconstruction, debug views, and capture consume the same semantics | every consumer depends on consistent identity, extent, format, and generation |

The core design deliberately shares the deferred *result* rather than forcing raster and ray traversal to share implementation. This enables semantic comparison but leaves alpha testing, deformation, motion, and hit/material mapping as explicit parity obligations.

## Feature Promise

Sparkle converts visible triangle meshes and their current/previous deformation/material state into one deferred surface contract. The frontend is selected by `r.GBuffer.Algorithm`: rasterized geometry or ray-traced primary visibility. Both are intended to produce the same base-color, normal, material, emissive, subsurface, motion, and depth meanings for downstream lighting.

This promise currently covers opaque and alpha-tested static, instanced, skinned, and morphed triangle geometry. It does not cover a complete transparent, transmissive, procedural/intersection, mesh-shader, tessellation, or non-triangle surface pipeline.

## Geometry Features

[Visibility and Draw Preparation](VisibilityAndDrawPreparation.md) owns which prepared primitive identities reach raster batches, their deterministic order/grouping, and the current absence of occlusion/LOD/GPU-driven draw generation. This dossier begins at the accepted batch and owns what geometry/material work produces in the GBuffer.

| Feature | Current path | Boundary |
| --- | --- | --- |
| Static meshes | GPU mesh cache plus prepared primitives and raster batches; static BLAS cache for ray use | Residency, batching benefit, and raster/ray parity unproved |
| Flat instancing | Explicit instance groups plus renderer-side compatible auto batching controlled by `r.MeshAutoBatching` | Current batching is for compatible flat instances; ordering/cost evidence open |
| Skeletal meshes | Up to eight imported/cooked influences flow as current/previous joint matrices to raster and ray hit/deformation data | Visual and motion-vector parity unproved; ray geometry rebuild cost open |
| Morph targets | Sparse morph deltas and current/previous weights feed raster/ray deformation and motion | Combined skin+morph edge cases and bounds need evidence |
| Frustum visibility | Per-view bounds test produces raster-visible indices | Invalid bounds conservatively remain visible; occlusion culling is not claimed |
| Ray geometry | Triangle BLASes, shared hit vertices/indices/material/instance records, classic or partitioned TLAS | Deforming BLAS refit is not exposed; procedural intersection is absent |

## Material Contract

| Component | Current authored/GPU meaning | Raster and ray coverage |
| --- | --- | --- |
| Base color | factor plus texture | shared |
| Normal | tangent-space normal texture cooked as linear BC5; shaders decode stored XY, reconstruct the positive tangent-space Z hemisphere, apply authored strength to XY, normalize, and transform through the canonical tangent frame | shared raster/ray decoder |
| Roughness | factor plus texture | shared |
| Metallic | factor plus texture | shared |
| Ambient occlusion | factor plus texture | shared |
| Dielectric F0 | material value packed in material GBuffer | shared |
| Emissive | factor plus texture | shared |
| Subsurface | color and strength, each with material/texture contribution | shared |
| Alpha mask | mode/cutoff; raster discard and ray any-hit/candidate rejection | opaque and alpha-tested only |
| Double-sided | culling and normal-orientation semantics | shared intent; parity evidence open |

Raster GBuffer uses a bindful per-material layout for eight texture roles: base color, normal, roughness, metallic, ambient occlusion, emissive, subsurface color, and subsurface strength. Ray consumers use a fixed-capacity material texture descriptor array only when non-uniform indexing and partially-bound array capabilities are available and the capacity reaches 4096. This is bounded descriptor-array indexing, not engine-wide runtime-sized bindless.

`r.Material.BindingMode` currently registers `RayTracingOnly` and `Everything` with `RayTracingOnly` as default, but the inspected CVar has no runtime consumer and the material-table capability report supports only `RayTracingOnly`. Treat `Everything` as unreachable vocabulary until the selector is either removed or connected to a defined producer/consumer/failure path.

## GBuffer Products

| Product | Format | Default/clear meaning | Downstream role |
| --- | --- | --- | --- |
| Base color | `R8G8B8A8_UNorm` | black, alpha 1 | diffuse/albedo and debug |
| World normal | `R16G16B16A16_Float` | +Z default | signed normalized world-space shading normal in RGB (X, Y, Z); consumers normalize through the shared GBuffer decoder, and the debug view maps `[-1, 1]` to display-linear `[0, 1]` |
| World tangent | `R16G16B16A16_Float` | zero means no surface tangent | signed world-space shading tangent in RGB (X, Y, Z) from the raster or ray-hit tangent frame; alpha distinguishes a surface from sky, and the debug view maps a surface tangent from `[-1, 1]` to display-linear `[0, 1]` |
| Material | `R8G8B8A8_UNorm` | metallic 0, roughness 1, AO 1, F0 0.04 | PBR parameters and debug |
| Emissive | `R16G16B16A16_Float` | zero | lighting composite |
| Subsurface | `R8G8B8A8_UNorm` | zero | direct subsurface term |
| Motion vector | `R16G16_Float` | zero | temporal reuse, accumulation, providers; sky motion is written separately |
| Device Z | raster `D32_Float`; ray `R32_Float` | far/background by frontend convention | visibility depth and provider input |
| Scene depth | `R32_Float` | derived from Device Z | lighting, sky, debug/capture product |

Raster color attachments are ordered BaseColor, WorldNormal, WorldTangent, Material, Emissive, Subsurface, MotionVector;
the pixel-shader `SV_Target0` through `SV_Target6` semantics and C++ attachment bindings follow that same order.
Material channels remain metallic (R), roughness (G), ambient occlusion (B), and dielectric F0 (A).

The world-normal product is not display encoded and no consumer changes its axes. Lighting and shadow shaders use the shared
world-space decoder, the World Normal debug mode performs only the signed-to-display-linear mapping, and DLSS Ray Reconstruction
tags the raw texture as unpacked normals together with the world/view transforms required to interpret world-space input.

World tangent is a separate GBuffer product; the diagnostic never overwrites the normal used by lighting. The extra
`R16G16B16A16_Float` render target adds eight bytes per render-resolution pixel to the Lit GBuffer path, including when
the tangent view is not selected. This is a source-level cost classification, not measured GPU memory or timing evidence.

The different Device Z storage types are an implementation distinction, not permission for different depth semantics. `AddLinearizeDeviceZPass` is the common downstream boundary.

## Raster Frontend

The raster branch builds compatible mesh batches, binds vertex/index/instance/deformation/material data, chooses solid or wireframe fill, and issues instanced/indexed-instanced draws into seven color products plus depth. Wireframe is a raster fill-mode feature; it is not an equivalent ray-GBuffer view.

### Rotated-normal correction — 2026-10-04

`ITER-GBUF-NORMAL-01` starts at `bbb9f7ed9e574d36776d539c51a9de51218bfa12` with a clean tree.
Owner: Renderer shader-data boundary. Scope: correct normal-matrix interpretation without changing source assets,
level transforms, winding, lights, materials, frame composition, or backend code. Other concurrently appearing changes
are outside this iteration. Target: preserve geometry/material correctness and the existing raster/ray production routes;
no release-readiness score or Reference estimator acceptance is advanced.
Checked Git shader blobs: MeshInstanceShaderData `7c4038c33f1697ea3c65a35ca484e6dcca06dc90`;
ObjectShaderData `5825e103a406e38df2585b550f125f56fc349ca8`.

The DamagedHelmet glTF node contains a 90-degree X rotation. Import reflects source X, converts the node transform,
reverses triangle winding, and generates the missing MikkTSpace tangents. The CPU publishes the inverse-transpose
normal matrix with `XMStoreFloat3x4`. Previously the shared HLSL records interpreted those packed columns as rows,
so normals rotated differently from positions and tangents. This is a shader-data ABI defect, not a reason to flip
this model's normals or normal-map green channel. Identity transforms can conceal the defect. See the canonical
[normal-matrix storage contract](../../../../../Decisions/WorldCoordinateAndUnits.md#storage-and-abi).

The correction is confined to `Resources/MeshInstanceShaderData.hlsli` and `Resources/ObjectShaderData.hlsli`.
Existing raster GBuffer, ray-hit material reconstruction, geometric normals, and robust ray endpoints consume the
correct matrix through their unchanged multiplication expressions. The CPU's 272-byte mesh-instance record,
48-byte packed normal matrix, material offset, resource bindings, and allocation policy are unchanged.
Performance classification: preserves storage and semantic work; no added GPU checks, copies, resources, or diagnostics.

| Criterion / failure / check | Observation and evidence boundary |
| --- | --- |
| `AC-GN-01` / `FM-GN-01` / `CHK-GN-01`: imported vertex normals and geometry agree after the authored transform; detect transposed interpretation with the source mesh | Read `DamagedHelmet.gltf` and its binary accessors directly. All 14,556 source normals have lengths in `[0.9999519, 1]`. Across 15,452 triangles, the mean dot of normalized mean vertex normal with geometric normal changes from `-0.2998684` to `0.9973475`; opposing triangles change from 10,463 to zero. All 1,654 vertices whose correct world normal has Y below `-0.8` previously had Y above `0.8`. This is a numerical source-data probe, before normal-map perturbation, not a rendered-image result. |
| `AC-GN-02` / `FM-GN-02` / `CHK-GN-02`: storage decoding preserves inverse-transpose normals for identity, the helmet rotation, rotation plus non-uniform scale/translation, shear, and mirrored scale | PowerShell/System.Numerics reconstructs the packed columns by the `XMStoreFloat3x4` indexing contract and decodes the new declaration. Direction error against the uncompressed inverse transpose is zero in all five cases; absolute dot with the transformed perpendicular tangent is at most `8.95e-8` (`1e-6` tolerance). This does not establish mirrored tangent-frame or full deformation parity. |
| `AC-GN-03` / `FM-GN-03` / `CHK-GN-03`: DXIL and SPIR-V preserve normal-matrix and following-field ABI | A temporary shader includes both actual shared headers and consumes transformed normals plus following mesh fields. DXC `1.9.0.5347 (fe2615732)` compiles `cs_6_6`, HLSL 2021, strict mode, warnings-as-errors, and all-resources-bound for DXIL and `-spirv -fspv-target-env=vulkan1.3`. DXIL reflection and SPIR-V disassembly agree on mesh matrix offset 192, following material offset 240, and stride 272; the normal-matrix column stride is 16. `spirv-val --target-env vulkan1.3` passes. The temporary probe is removed; no test or diagnostic surface is shipped. |
| `AC-GN-04` / `FM-GN-04` / `CHK-GN-04`: actual viewport normals/light response and raster/ray agreement on D3D12 and Vulkan | Owner-run verification remains deferred: recook changed shaders, reopen DamagedHelmet, compare World Normal, World Tangent, Lit, and Reference, then repeat with ray GBuffer and Vulkan; use Sponza as the preservation check. A remaining black result escalates to the first incorrect lighting/visibility product rather than another normal flip. |

Risk `RISK-GN-01`: old cooked shaders retain the incorrect interpretation. Prevention/recovery: regenerate affected
DXIL and SPIR-V shader packages through the existing Launcher cook workflow and restart the application. Detection:
the unchanged green upward underside or black Lit response persists in the rebuilt shader generation. Owner: Renderer;
retirement evidence: `CHK-GN-04` on the newly cooked generation. Mesh/material/texture recooking is not required by
this correction because their representations did not change.

Decision: source/math/layout checks pass; visual/backend runtime acceptance remains unrun. `git diff --check` is the
final whitespace check. A full Editor build, full content cook, GPU render, and runtime capture were not performed.

Raster material descriptors remain per material. Transparent alpha is represented in source data, but the current GBuffer pipeline does not implement sorting, order-independent transparency, or transmission and therefore must not advertise transparent PBR output.

## Ray Frontend

When `r.GBuffer.Algorithm=RayTracing`, the Renderer uses the engine-wide automatic frontend resolver before graph construction:

| Capability state | Current active rule |
| --- | --- |
| shared requirements plus Pipeline | dispatch ray-generation program with shared miss/closest-hit/any-hit groups |
| shared requirements plus Inline only | dispatch the compute RayQuery frontend |
| neither complete route | reject ray-GBuffer graph construction |

The two frontends share scene identity, hit reconstruction, material lookup, alpha decision, and output meanings. Native pipeline currently authors opaque and alpha-tested triangle hit groups for the Surface ray type. See [Ray-Tracing Execution Architecture](../RayTracing/ExecutionArchitecture.md) for the SBT index and failure contracts.

No current pass applies deferred decals between GBuffer production and downstream consumers. [Deferred Decals](../DeferredDecals/README.md) owns that negative capability boundary and routes the separately labeled, first-release-admitted target architecture.

## Intent And Tradeoffs

- One deferred contract lets raster and ray visibility feed the same lighting, debug, temporal, and presentation stages. The cost is several full-resolution attachments and strict semantic parity work.
- Separating visibility from lighting makes lighting mode independent of the GBuffer frontend. It does not create a raster-only renderer because current lighting still traces rays.
- A fixed ray material table makes capacity/capability explicit and keeps native backend layouts tractable. It limits the visible texture set and is narrower than runtime-sized bindless.
- Current/previous deformation is published once for motion and ray hit reconstruction. It increases per-frame data and makes continuity/reset correctness essential.

## Failure, Diagnostics, And Evidence

- Invalid GBuffer enum values fail graph construction.
- Automatic resolution must reject when neither ray frontend is complete rather than silently become raster or publish a dummy GBuffer.
- Missing scene TLAS/hit/material bindings are fatal execution-contract failures.
- Over-capacity material textures/lights, missing resources, alpha edges, double-sided normals, skin+morph motion, Device Z equivalence, and every GBuffer channel need controlled raw-buffer evidence.
- Primary checks are `REN-E03`, `REN-E04`, `REN-E05`, `REN-E11`, `REN-E23`, `RHI-E06`, and `RHI-E07`.

## Horizontal Semantic Matrix

| Surface case | Raster | Ray inline | Ray pipeline | Required shared oracle |
| --- | --- | --- | --- | --- |
| opaque static/instanced | implemented | implemented when capable | implemented when capable | decoded GBuffer channels, depth, primitive/material identity |
| alpha-tested, double-sided | implemented | candidate rejection | any-hit rejection | coverage mask, normal orientation, hit/miss identity at cutoff edges |
| skinned and morphed | current/previous GPU deformation | updated ray positions and hit reconstruction | same scene/SBT identity | position, normal, depth, motion, and material agreement across frames |
| transparent/transmissive | not complete | not complete | not complete | explicit rejection/exclusion; never opaque-looking success |
| procedural/non-triangle | absent | absent | no intersection program | capability rejection before scene/pipeline use |
| wireframe | raster fill mode | not equivalent | not equivalent | requested-versus-active result reports the asymmetry |

Run the matrix across exact render extents, resize, scene reload, missing/pending textures, descriptor capacity boundaries, and D3D12/Vulkan. A visual final-color comparison does not replace raw attachment and identity checks.

## Acceptance Criteria

- `AC-GMG-01` — every supported material component and default texture decodes to the documented GBuffer channel meaning and format for opaque raster, ray-inline, and ray-pipeline surfaces.
- `AC-GMG-02` — raster, inline, and pipeline frontends agree within predeclared channel/depth tolerances for supported static, instanced, alpha-tested, double-sided, skinned, morphed, and combined deformation fixtures.
- `AC-GMG-03` — current/previous transforms and deformation produce correct rigid, skinned, morphed, combined, and sky motion vectors across continuity and reset cases.
- `AC-GMG-04` — the shared automatic resolver selects Pipeline when complete, otherwise Inline when complete, and rejects when neither route exists; no per-effect execution selector remains.
- `AC-GMG-05` — the fixed ray material texture table accepts its documented capacity, rejects overflow before dispatch, and preserves material/descriptor identity under add/remove/reload.
- `AC-GMG-06` — alpha cutoff edges, missing/default textures, invalid tangents/normals, invalid bounds, repeated geometry/material IDs, and double-sided orientation have deterministic documented results without stale data.
- `AC-GMG-07` — transparent/transmissive, procedural, mesh/task/tessellation, and ray-wireframe requests remain explicitly unavailable; dormant BRDF/material-binding vocabulary is not presented as an active path.
- `AC-GMG-08` — both backends create, transition, write, export/capture, and decode all eight GBuffer products without native validation errors or semantic drift.

## Controlled Failure Modes And Checks

| Failure ID | Injection and safe state | Detecting check |
| --- | --- | --- |
| `FM-GMG-01` | remove one capability/program/SBT binding required by the automatically selected ray frontend; graph creation rejects before dispatch | `CHK-GMG-02` |
| `FM-GMG-02` | exceed material table capacity or provide mismatched hit/material/descriptor indices; reject before GPU execution | `CHK-GMG-03` |
| `FM-GMG-03` | fail/pending texture, invalid tangent/bounds, or alpha value around cutoff; use the documented default/conservative/refusal result, never stale prior data | `CHK-GMG-01`, `CHK-GMG-03` |
| `FM-GMG-04` | change/remove/reload deformed geometry while prior work is in flight; new frames use new identity and old resources retire by completion | `CHK-GMG-04` |
| `FM-GMG-05` | request unsupported transparency/procedural/wireframe combination; requested-versus-active reporting rejects or marks unavailable | `CHK-GMG-02` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-GMG-01` | canonical material/channel ramp and alpha/double-sided fixtures; capture/decode every attachment and compare to analytic values/defaults | `AC-GMG-01`, `AC-GMG-06`, `AC-GMG-08`; `FM-GMG-03` |
| `CHK-GMG-02` | automatic frontend capability matrix, capability removal, unsupported geometry/transparency/wireframe, D3D12/Vulkan, and focused evidence for both adapters where available | `AC-GMG-02`, `AC-GMG-04`, `AC-GMG-07`, `AC-GMG-08`; `FM-GMG-01`, `FM-GMG-05` |
| `CHK-GMG-03` | exact descriptor capacity and capacity-plus-one; missing/default/reloaded textures and deliberately corrupted index fixtures | `AC-GMG-05`, `AC-GMG-06`; `FM-GMG-02`, `FM-GMG-03` |
| `CHK-GMG-04` | multi-frame rigid/skin/morph/combined motion plus cut/reset/remove/reload while in flight; compare raster/ray positions, depth, normals, motion, and retirement | `AC-GMG-02`, `AC-GMG-03`, `AC-GMG-05`; `FM-GMG-04` |

This contract is **defined but unproved**. Completion requires raw-product evidence for every applicable matrix cell and controlled rejection evidence for every excluded cell; final lit screenshots alone are insufficient.

## Primary Source Routes

- [`GBufferPasses.cpp`](../../../../../../../Engine/Renderer/Private/Passes/GBuffer/GBufferPasses.cpp), [`GBufferRenderTargets.cpp`](../../../../../../../Engine/Renderer/Private/Passes/GBuffer/GBufferRenderTargets.cpp), and [`GBufferFormats.h`](../../../../../../../Engine/Renderer/Private/Passes/GBuffer/GBufferFormats.h)
- [`RasterizedGBufferMesh.cpp`](../../../../../../../Engine/Renderer/Private/Passes/GBuffer/Raster/RasterizedGBufferMesh.cpp)
- [`RayTracingGBufferMesh.cpp`](../../../../../../../Engine/Renderer/Private/Passes/GBuffer/RayTracing/RayTracingGBufferMesh.cpp)
- [`LinearizeDeviceZ.cpp`](../../../../../../../Engine/Renderer/Private/Passes/GBuffer/LinearizeDeviceZ.cpp) and [`SkyMotionVector.cpp`](../../../../../../../Engine/Renderer/Private/Passes/GBuffer/SkyMotionVector.cpp)
- [`RenderViewPreparation.cpp`](../../../../../../../Engine/Renderer/Private/View/RenderViewPreparation.cpp)
- [`RenderGpuScene.cpp`](../../../../../../../Engine/Renderer/Private/Scene/GpuScene/RenderGpuScene.cpp)
- [`MaterialTextureTableCapability.h`](../../../../../../../Engine/Renderer/Private/Scene/Materials/MaterialTextureTableCapability.h)
