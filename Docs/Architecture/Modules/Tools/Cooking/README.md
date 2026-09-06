# Asset Cooking Capability Inventory

Status: capability snapshot; current, but not deterministic-cook, package, or runtime evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; AssetCooker, TextureCooker, MeshCooker, MaterialCooker, SceneCooker, source import, publication, and CMake membership inspected; evidence `S` only

Scope: project/category orchestration, texture/mesh/material/scene/skeleton/animation products, identity, validation, concurrency, publication, and runtime handoff

Owner: `Tools/Cooking`; shader cooking is separately owned by [Shader Compilation](../ShaderCompiler/README.md)

Evidence and disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

## Orchestration And Products

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `COOK-001` | Project cook CLI | Implemented path | `AssetCooker` supports `cook-project`, `cook-shaders`, `cook-textures`, and `cook-assets`; project name or `ALL` for full project cooking; all six build profiles; optional tool profile and repository root. | `S` |
| `COOK-002` | Project discovery | Implemented path | Discovers marked projects, reads `Levels.catalog` and selected level documents, collects scene IDs, resolves project/cooked/tool roots, and builds an ordered Shaders -> Textures -> SceneAssets plan. | `S` |
| `COOK-003` | Capability preflight | Implemented path | Required `ShaderCompiler` and `TextureCooker` executables are checked before their stages; diagnostics identify category and optional source path. | `S` |
| `COOK-004` | Shader stage delegation | Implemented path | Runs `ShaderCompiler cook` in project context and records global map/library outputs. Exact shader capability lives in the shader inventory. | `S` |
| `COOK-005` | Texture request planning | Implemented path | Re-imports selected scenes, gathers material texture requests, adds Engine default textures, deduplicates by stable 64-bit asset ID, and rejects conflicting definitions for one ID. | `S` |
| `COOK-006` | Scene generation | Implemented path | Imports scenes and builds/publishes manifests, meshes, materials, skeletons, animations, and scene registry as one file set. | `S` |
| `COOK-007` | Tool/output reporting | Implemented path | Category diagnostics, child tool output, nonzero exit propagation, and output records are exposed to CLI/Launcher. It does not produce a signed release manifest. | `S` |

## Texture Cooking

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `COOK-008` | Source texture formats | Implemented path | DDS, EXR, Radiance `.hdr`/`.hdri`, plus PNG/JPEG/BMP/TGA/GIF/PSD/PIC/PNM/PPM/PGM. Backend decoders validate each format; KTX is not a current source loader here. | `S` |
| `COOK-009` | Texture dimensions | Implemented path | 2D and cube request vocabulary, array slices/faces and mip chains. No 3D/volume texture cooking path is present. | `S` |
| `COOK-010` | Mip policy | Implemented path | Generate, preserve existing, or no mips; filters Regular, Kaiser, NormalAware, and Angular. sRGB inputs can be linearized for filtering. | `S` |
| `COOK-011` | Channel extraction | Implemented path | RGBA or R/G/B/A masks support packed AO/roughness/metallic source reuse and scalar products. | `S` |
| `COOK-012` | Semantic format policy | Implemented path | Diffuse/emissive/subsurface color use BC1 when opaque or uncompressed when alpha is meaningful; normals BC5; scalar maps BC4 when greyscale; HDR BC6H; fallback uncompressed RGBA8 or RGBA32F. BC7 code exists but current automatic semantic policy does not select it. | `S` |
| `COOK-013` | Parallel batch | Implemented path | Independent requests run on 1..4 background workers (hardware threads minus one, clamped) with one frame-critical worker and one execution. | `S` |
| `COOK-014` | Memory bound | Implemented path | Working pixel data obtains leases from a 1 GiB batch memory limiter; an individual request exceeding the budget fails. This bounds leased decoded pixel data, not every decoder/compressor allocation or total tool RSS. | `S` |
| `COOK-015` | Generation publication | Implemented path | Every texture cooks to `.cook-generation` staging; any item failure removes staged outputs; success publishes the requested file set through Core publication helpers. | `S` |
| `COOK-016` | Request inspection | Implemented path | `TextureCooker inspect-request-file` and `cook-request-file` parse the text request list and return distinct usage/load/inspect/cook exit codes. | `S` |

## Scene Product Coverage

| ID | Product | Current payload | Publication/consumer | Evidence |
| --- | --- | --- | --- | --- |
| `COOK-017` | Mesh | Vertices/indices, static/skeletal kind, skin influences, morph targets/defaults, mesh metadata | Staged by MeshCooker; GameFramework mesh loader | `S` |
| `COOK-018` | Material | Compact PBR factors, alpha/double-sided state, semantic texture asset references | Staged by MaterialCooker; GameFramework material translator | `S` |
| `COOK-019` | Scene manifest | Mesh instances/groups, material bindings/variants, camera/light records, metadata, references to external cooked assets | Staged with registry by SceneCooker; LevelSession loader | `S` |
| `COOK-020` | Skeleton | Joint names/hierarchy and bind/reference transforms | Scene generation file set; skeleton loader/resource store | `S` |
| `COOK-021` | Animation | Clips, samplers/keyframes/channels, duration, skeleton target | Scene generation file set; animation loader/evaluator | `S` |
| `COOK-022` | Registry | Scene ID -> relative manifest path for the complete selected generation | Published with manifests; runtime registry | `S` |

## Vertical Project Cook Trace

Project/profile/category request -> project/catalog discovery -> selected level files yield scene IDs -> source importer produces semantic scenes -> material pass builds a deduplicated texture request generation -> TextureCooker decodes/transforms/compresses/stages/publishes -> mesh/material/scene/skeleton/animation builders stage one scene generation -> Core publishes files and scene registry -> GameFramework later opens only the cooked products.

## Explicit Non-Capabilities And Risks

- No explicit schema version is stored in the shared cooked asset header; compatibility currently relies on magic and exact layout validation plus recooking.
- No package/stage/archive/signing operation, dependency-complete release manifest, remote cache, distributed cook, or general incremental content graph was found.
- `CookMode::Force` is a Launcher operation concept; AssetCooker itself exposes category commands and overwrites complete current outputs rather than a versioned DDC.
- KTX support is a build option/dependency surface but no KTX source loader or final KTX product path was found in the inspected cooker.
- Determinism, stale-output removal, crash atomicity, corrupt-input safety, and runtime consumption must be executed before release claims.
