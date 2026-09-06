# Source Importers Capability Inventory

Status: capability snapshot; current, but not fidelity certification or runtime evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; `Tools/Import/SourceImporters` public/private source, dependencies, and cooking consumers inspected; evidence `S` only

Scope: accepted source scene formats, geometry, instances, transforms, materials, textures, cameras, lights, skins, morphs, animation, validation, and known losses

Owner: `Tools/Import/SourceImporters` / `SourceImporters`

Evidence and disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

## Format Boundary

| ID | Source format | State | Parser/dependency | Exact boundary | Evidence |
| --- | --- | --- | --- | --- | --- |
| `IMP-001` | glTF 2.0 JSON | Implemented path | `cgltf`; `.gltf` | External URI buffers/images are loaded and the document is validated before translation. | `S` |
| `IMP-002` | GLB | Implemented path | `cgltf`; `.glb` | Embedded geometry buffer data is accepted; embedded material images are explicitly rejected. | `S` |
| `IMP-003` | FBX | Implemented path | Assimp FBX path; `.fbx` | Scene is triangulated, validated, cache-optimized, globally scaled, converted left-handed, and requires a valid source-units-to-metres conversion. | `S` |
| `IMP-004` | OBJ/USD/Alembic/OpenVDB | Not found | None in `SourceSceneImporter` | Catalog entries may name these as future/source-only workloads, but they are not current importer formats. | `S` |

## Semantic Coverage

| ID | Capability | glTF/GLB coverage | FBX coverage | Important limit | Evidence |
| --- | --- | --- | --- | --- | --- |
| `IMP-005` | Triangle geometry | Positions, indices, normals, UV0, vertex color, tangents; validated accessors | Assimp triangle meshes; normals/tangents generated when needed | No line/point primitive product; unsupported/malformed attributes fail import. | `S` |
| `IMP-006` | Tangent generation | MikkTSpace, including seam remap and morph-target tangent deltas | Assimp tangent calculation | glTF rejects non-finite/irreconcilable tangent frames instead of dropping fidelity silently. | `S` |
| `IMP-007` | Coordinate normalization | Reflects source X, flips triangle winding, converts matrices/quaternions/tangents, rotates camera/light forward convention once | Assimp left-handed conversion plus declared metres-per-unit scaling | World placement happens after import; reapplying conversion downstream is a defect. | `S` |
| `IMP-008` | Mesh instances | Node instances plus shared-mesh grouping | Node/mesh instances | Instance records preserve source node identity, transform, primitive, material, skeleton, and morph weights. | `S` |
| `IMP-009` | GPU-authored instancing | `EXT_mesh_gpu_instancing` translation/count validation | Not found | Only translation/rotation/scale attribute combinations represented by the importer are accepted. | `S` |
| `IMP-010` | Metallic-roughness material | Base color/alpha, metallic, roughness, emissive, double-sided, alpha mode/cutoff, IOR-to-F0 | Supported Assimp shading models mapped to the same compact material | Clearcoat, sheen, specular extension, transmission, volume, anisotropy, iridescence, and dispersion are not represented. | `S` |
| `IMP-011` | Material textures | Base color, normal, AO red, emissive, packed roughness green/metallic blue | Diffuse/base, normal, AO/light-map, roughness, metallic, emissive paths where source representation is accepted | UV set must be 0; texture transform, BasisU, WebP, and embedded glTF images are rejected. Normal scale and AO strength are not persisted. | `S` |
| `IMP-012` | Alpha | Opaque, Mask, Blend imported | Scalar/diffuse alpha can produce Blend | Downstream Renderer does not implement true blend/transmission; imported vocabulary is broader than render support. | `S` |
| `IMP-013` | Material variants | `KHR_materials_variants` names and primitive mappings | Not found | Runtime can select imported variant mappings; authoring/editing variants is not provided. | `S` |
| `IMP-014` | Cameras | Perspective and orthographic schema from glTF | Perspective only | Invalid projection/clip data fails. Renderer/editor evidence must confirm each projection path before advertising both. | `S` |
| `IMP-015` | Lights | Directional, point, spot through punctual-light extension; physical fields mapped | Directional, point, spot, rect when Assimp source kind is supported | Unsupported/incomplete kind fails rather than degrading. | `S` |
| `IMP-016` | Skeleton/skin | Joint hierarchy, inverse binds, reference space, JOINTS/WEIGHTS pairs up to eight influences | Skeleton/bones and up to eight influences | Duplicate/invalid joints, zero weights, non-invertible transforms, or over-eight influences fail without truncation. | `S` |
| `IMP-017` | Morph targets | Position/normal/tangent deltas, names/default weights, per-instance weights | Explicitly rejected | glTF tangent-seam remap carries skin and morph data together. | `S` |
| `IMP-018` | Animation | Translation/rotation/scale/weights; Linear, Step, CubicSpline; skeleton binding | Skeleton-owned transform channels; source keys validated and represented without loss | Node-only FBX animation is not playable; cross-skeleton or lossy channels are rejected. | `S` |
| `IMP-019` | Embedded FBX textures | Not applicable | Identified compressed embedded resources can be extracted | Unknown/unsupported embedded encoding fails. Texture cook support still determines final acceptance. | `S` |

## Vertical Import Trace

`SourceSceneImporter` selects by lower-case extension -> parser validates file/document -> format-specific translators normalize coordinates and append materials/textures/geometry/instances/cameras/lights/skeletons/animations/variants -> one `SourceImportOutput` owns the complete imported scene plus diagnostics -> Mesh/Material/Scene cookers consume it in the same tool process -> no source-import object crosses into runtime.

## Explicit Non-Capabilities And Risks

- “FBX support” does not include FBX morph targets or every Assimp shading/texture mapping; “glTF support” does not include every Khronos extension.
- There is no current OBJ, USD, Alembic, OpenVDB, Draco, Meshopt, BasisU, WebP, multi-UV, texture-transform, or embedded-glTF-image path.
- Imported Blend survives into the material schema but is not a completed rendering capability.
- Source-only inspection does not establish round-trip fidelity, malformed-file hardening, deterministic asset IDs, or exact parity between glTF and FBX representations.
