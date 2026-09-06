# Engine Assets Capability Inventory

Status: capability snapshot; current source corpus, not proof of cooked or packaged inclusion

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; tracked `Engine/Assets` files and their known compiler/cooker/runtime consumers inspected; evidence `S` only

Scope: engine-owned shader source, default textures, sky environments, and small geometry fixtures

Owner: `Engine/Assets`; the shader compiler, asset cooker, GameFramework, and Renderer own transformation and consumption

Evidence and disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

`Engine/Assets` is a source corpus, not a C++ module or asset-manager implementation. Its capabilities are useful only when the corresponding cook and runtime consumer are present.

## Tracked Corpus

| ID | Family | Current content and purpose | Coverage boundary | Evidence |
| --- | --- | --- | --- | --- |
| `EASSET-001` | Shader entry sources | 29 `.hlsl` files implement registered compute, raster, ray-tracing, lighting, post, presentation, and debug entry points. Exact registrations are in the [Shader Program Catalog](../Renderer/Features/ShaderRuntime/ShaderProgramCatalog.md). | An `.hlsl` file without a typed registration is not a runtime shader capability. | `S` |
| `EASSET-002` | Shader includes/contracts | 91 `.hlsli` files cover BRDF, common math/color/random/sampling, geometry/skinning/morphing, light/reservoir logic, materials, GBuffer packing, ray traversal/hit semantics, and CPU/GPU uniform layouts. | Macro alternatives and unused includes are not advertised features. | `S` |
| `EASSET-003` | Default textures | Eight PNGs: black, blue, checkerboard, cubemap, green, normal, red, and white. Asset cooking supplies fallback texture products and Renderer binds them when material slots are absent. | Fallback correctness and package presence require execution/package evidence. | `S` |
| `EASSET-004` | Sky environments | Four 4K EXR panoramas: autumn hill, evening road, Goegap road, and red church. | Presence does not prove every level uses or packages them. | `S` |
| `EASSET-005` | Cube fixtures | Cube `.gltf`, `.glb`, `.bin`, license, plus repeated-cube and `EXT_mesh_gpu_instancing` glTF fixtures. | These are compact import/instancing workloads, not a general sample library. | `S` |

## Shader Contract Families

| Family | Implemented subject |
| --- | --- |
| BRDF | GGX distribution/specular sampling, Smith geometry, Schlick Fresnel, Burley diffuse, subsurface, occlusion, and shared shading data/config. |
| Geometry | Vertex/pixel layouts, transforms, tangent basis, skinning, morphing, screen space, fullscreen triangle, and motion vectors. |
| Material | Material uniform/texture roles, normal reconstruction, and the fixed ray/path material texture table. |
| Lighting | Punctual/area/sky evaluation, direct-light sampling/reservoirs, ReSTIR indirect reservoirs/uniforms, visibility, reconstruction guides, and composite outputs. |
| Ray tracing | Inline queries, native pipeline trace/SBT layout, surface/hit decoding, path sampling, debug modes, and shadow semantics/signals. |
| Display/debug | Exposure, tone mapping, output encoding, instance/debug view constants and visualization. |
| ABI resources | Frame, camera, temporal view, scene lighting, mesh/object, light, and sampler layouts mirrored by Renderer registrations and RHI pipelines. |

## Vertical Asset Routes

- Shader source/include -> typed Renderer registration -> ShaderCompiler dependency scan/compile/reflect/validate -> cooked map/library -> runtime generation -> pass pipeline.
- Default/sky source image -> AssetCooker texture request -> TextureCooker decode/mips/compress/publish -> cooked texture reference -> GameFramework resource table -> Renderer residency/descriptors.
- Cube/instancing source -> SourceImporters -> mesh/material/scene cookers -> cooked scene registry -> LevelSession -> world -> Renderer.

## Explicit Non-Capabilities And Risks

- This corpus does not itself provide discovery, import, versioning, dependency tracking, cooking, residency, hot reload, or packaging.
- The source tree contains no volume, terrain, particle, decal, font, audio, animation-clip, or neural-model asset family.
- Counts are a dated tracked-source snapshot; generated/cooked products are deliberately not counted as source capability.
