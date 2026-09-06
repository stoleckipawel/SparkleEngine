# Renderer Geometry, Materials, and GBuffer

Status: current feature dossier; source-backed, not visual fidelity, raster/ray parity, performance, or release evidence

Verified: 2026-09-06 against committed `master` revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Scope: `REN-SCENE-03` through `REN-SCENE-10`, `REN-MAT-01` through `REN-MAT-10`, `REN-GBUF-01` through `REN-GBUF-08`, and `REN-FRONT-01` through `REN-FRONT-07`

## Feature Promise

Sparkle converts visible triangle meshes and their current/previous deformation/material state into one deferred surface contract. The frontend is selected by `r.GBuffer.Algorithm`: rasterized geometry or ray-traced primary visibility. Both are intended to produce the same base-color, normal, material, emissive, subsurface, motion, and depth meanings for downstream lighting.

This promise currently covers opaque and alpha-tested static, instanced, skinned, and morphed triangle geometry. It does not cover a complete transparent, transmissive, procedural/intersection, mesh-shader, tessellation, or non-triangle surface pipeline.

## Geometry Features

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
| Normal | tangent-space normal texture | shared |
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
| Normal | `R16G16B16A16_Float` | +Z default | shading and reconstruction guide |
| Material | `R8G8B8A8_UNorm` | metallic 0, roughness 1, AO 1, F0 0.04 | PBR parameters and debug |
| Emissive | `R16G16B16A16_Float` | zero | lighting composite |
| Subsurface | `R8G8B8A8_UNorm` | zero | direct subsurface term |
| Motion vector | `R16G16_Float` | zero | temporal reuse, accumulation, providers; sky motion is written separately |
| Device Z | raster `D32_Float`; ray `R32_Float` | far/background by frontend convention | visibility depth and provider input |
| Scene depth | `R32_Float` | derived from Device Z | lighting, sky, debug/capture product |

The different Device Z storage types are an implementation distinction, not permission for different depth semantics. `AddLinearizeDeviceZPass` is the common downstream boundary.

## Raster Frontend

The raster branch builds compatible mesh batches, binds vertex/index/instance/deformation/material data, chooses solid or wireframe fill, and issues instanced/indexed-instanced draws into six color products plus depth. Wireframe is a raster fill-mode feature; it is not an equivalent ray-GBuffer view.

Raster material descriptors remain per material. Transparent alpha is represented in source data, but the current GBuffer pipeline does not implement sorting, order-independent transparency, or transmission and therefore must not advertise transparent PBR output.

## Ray Frontend

When `r.GBuffer.Algorithm=RayTracing`, the Renderer resolves `r.GBuffer.RayTracingExecution` before graph construction:

| Requested mode | Current active rule |
| --- | --- |
| Automatic | prefer native pipeline when complete; otherwise use inline when complete |
| Inline | require acceleration structure, ray-query, hit/material buffers, and fixed texture table; dispatch compute shader |
| Pipeline | require native RT pipeline plus valid scene SBT plan/table; dispatch ray-generation program with miss/closest-hit/any-hit groups |

The two frontends share scene identity, hit reconstruction, material lookup, alpha decision, and output meanings. Native pipeline currently authors opaque and alpha-tested triangle hit groups for the Surface ray type. See [Ray-Tracing Execution Architecture](RayTracing/ExecutionArchitecture.md) for the SBT index and failure contracts.

No current pass applies deferred decals between GBuffer production and downstream consumers. [Deferred Decals](DeferredDecals/README.md) owns that negative capability boundary and routes the separately labeled future architecture.

## Intent And Tradeoffs

- One deferred contract lets raster and ray visibility feed the same lighting, debug, temporal, and presentation stages. The cost is several full-resolution attachments and strict semantic parity work.
- Separating visibility from lighting makes lighting mode independent of the GBuffer frontend. It does not create a raster-only renderer because current lighting still traces rays.
- A fixed ray material table makes capacity/capability explicit and keeps native backend layouts tractable. It limits the visible texture set and is narrower than runtime-sized bindless.
- Current/previous deformation is published once for motion and ray hit reconstruction. It increases per-frame data and makes continuity/reset correctness essential.

## Failure, Diagnostics, And Evidence

- Invalid GBuffer enum values fail graph construction.
- Strict unavailable ray frontends must fail/reject rather than silently become raster or another ray frontend; Automatic may select its documented alternate.
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
- `AC-GMG-04` — Automatic selects and reports a complete ray frontend; strict Inline/Pipeline rejects when incomplete and never silently becomes raster or the other ray frontend.
- `AC-GMG-05` — the fixed ray material texture table accepts its documented capacity, rejects overflow before dispatch, and preserves material/descriptor identity under add/remove/reload.
- `AC-GMG-06` — alpha cutoff edges, missing/default textures, invalid tangents/normals, invalid bounds, repeated geometry/material IDs, and double-sided orientation have deterministic documented results without stale data.
- `AC-GMG-07` — transparent/transmissive, procedural, mesh/task/tessellation, and ray-wireframe requests remain explicitly unavailable; dormant BRDF/material-binding vocabulary is not presented as an active path.
- `AC-GMG-08` — both backends create, transition, write, export/capture, and decode all eight GBuffer products without native validation errors or semantic drift.

## Controlled Failure Modes And Checks

| Failure ID | Injection and safe state | Detecting check |
| --- | --- | --- |
| `FM-GMG-01` | request strict ray frontend with one required capability/program/SBT binding removed; graph creation rejects and names the missing requirement | `CHK-GMG-02` |
| `FM-GMG-02` | exceed material table capacity or provide mismatched hit/material/descriptor indices; reject before GPU execution | `CHK-GMG-03` |
| `FM-GMG-03` | fail/pending texture, invalid tangent/bounds, or alpha value around cutoff; use the documented default/conservative/refusal result, never stale prior data | `CHK-GMG-01`, `CHK-GMG-03` |
| `FM-GMG-04` | change/remove/reload deformed geometry while prior work is in flight; new frames use new identity and old resources retire by completion | `CHK-GMG-04` |
| `FM-GMG-05` | request unsupported transparency/procedural/wireframe combination; requested-versus-active reporting rejects or marks unavailable | `CHK-GMG-02` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-GMG-01` | canonical material/channel ramp and alpha/double-sided fixtures; capture/decode every attachment and compare to analytic values/defaults | `AC-GMG-01`, `AC-GMG-06`, `AC-GMG-08`; `FM-GMG-03` |
| `CHK-GMG-02` | frontend matrix across Automatic/Inline/Pipeline, capability removal, unsupported geometry/transparency/wireframe, D3D12/Vulkan | `AC-GMG-02`, `AC-GMG-04`, `AC-GMG-07`, `AC-GMG-08`; `FM-GMG-01`, `FM-GMG-05` |
| `CHK-GMG-03` | exact descriptor capacity and capacity-plus-one; missing/default/reloaded textures and deliberately corrupted index fixtures | `AC-GMG-05`, `AC-GMG-06`; `FM-GMG-02`, `FM-GMG-03` |
| `CHK-GMG-04` | multi-frame rigid/skin/morph/combined motion plus cut/reset/remove/reload while in flight; compare raster/ray positions, depth, normals, motion, and retirement | `AC-GMG-02`, `AC-GMG-03`, `AC-GMG-05`; `FM-GMG-04` |

This contract is **defined but unproved**. Completion requires raw-product evidence for every applicable matrix cell and controlled rejection evidence for every excluded cell; final lit screenshots alone are insufficient.

## Primary Source Routes

- [`GBuffer.cpp`](../../../../../../Engine/Renderer/Private/Passes/GBuffer/GBuffer.cpp) and [`GBufferFormats.h`](../../../../../../Engine/Renderer/Private/Passes/GBuffer/GBufferFormats.h)
- [`RasterizedGBuffer.cpp`](../../../../../../Engine/Renderer/Private/Passes/GBuffer/RasterizedGBuffer.cpp)
- [`RayTracingGBuffer.cpp`](../../../../../../Engine/Renderer/Private/Passes/GBuffer/RayTracingGBuffer.cpp)
- [`SceneDepth.cpp`](../../../../../../Engine/Renderer/Private/Passes/GBuffer/SceneDepth.cpp) and [`SkyMotionVectors.cpp`](../../../../../../Engine/Renderer/Private/Passes/GBuffer/SkyMotionVectors.cpp)
- [`RenderViewPreparation.cpp`](../../../../../../Engine/Renderer/Private/View/RenderViewPreparation.cpp)
- [`RenderGpuScene.cpp`](../../../../../../Engine/Renderer/Private/Scene/GpuScene/RenderGpuScene.cpp)
- [`MaterialTextureTableCapability.h`](../../../../../../Engine/Renderer/Private/Scene/Materials/MaterialTextureTableCapability.h)
