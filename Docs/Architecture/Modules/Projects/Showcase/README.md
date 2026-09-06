# Showcase Product Capability Inventory

Status: current product/catalog snapshot; not evidence that every level is downloaded, cooked, runnable, or releasable

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; Showcase marker/CMake/entry points, level catalog, authored level files, tracked source content, Launcher use, and runtime startup path inspected; evidence `S` only

Scope: shipped project products, startup/selection behavior, cataloged workloads, content provenance/readiness, and their role as capability evidence

Owner: `Projects/Showcase`; Engine/Application/Launcher/Cooking own execution infrastructure

Evidence and disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

## Product Surface

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `SHOW-001` | Project discovery | Implemented path | `.sparkle-project` makes Showcase the current auto-discovered project and Launcher default content. | `S` |
| `SHOW-002` | Editor product | Implemented path | `ShowcaseEditor` is a minimal `main` over `RunEditorApplication`, links ApplicationEditor/GameFramework, stages declared runtime owners and Streamline, and outputs under the editor artifact root. | `S` |
| `SHOW-003` | Runtime product | Implemented path | `ShowcaseRuntime` is a minimal `main` over `RunRuntimeApplication`, does not link Editor/ApplicationEditor, and outputs under the runtime artifact root. | `S` |
| `SHOW-004` | Six-profile build naming | Implemented path | Both products participate in Debug/Development/Shipping profile convention; Launcher resolves the matching target/profile and project working directory. | `S` |
| `SHOW-005` | Startup level selection | Implemented path | `SPARKLE_STARTUP_LEVEL` requests a registered name; absent or unknown value selects/warns and falls back to `Empty`. Only catalog entries marked Selected and currently ready register. | `S` |
| `SHOW-006` | Runtime level switching | Implemented path | Editor menu/Launcher can choose registered catalog levels; LevelSession cancels an in-flight load when a new request arrives and rejects stale generation results. | `S` |
| `SHOW-007` | Authored level save | Implemented path | Editor Save All writes the active `.level` document through GameFramework. External source assets/cooked products are not authored by this operation. | `S` |

## Level Catalog

The catalog has 16 level records: 13 Selected and 3 not selected. Selection is intent, not readiness; external packs and cooked products still gate registration and launch.

| Level(s) | Selected | Source/readiness class | Intended evidence surface | Important limitation |
| --- | --- | --- | --- | --- |
| `Empty` | Yes | Built-in/authored level; no scene asset | Host startup, window/input/UI, scene reset, no-content baseline | Silent fallback can hide failed content unless logs/UX are checked. |
| `Sponza` | Yes | Tracked glTF/project textures | General static geometry, textures, PBR lighting | Runtime/cook evidence not produced here. |
| `ABeautifulGame` | Yes | Tracked glTF/project textures | Material variety, composition, static PBR | Exact extension/alpha coverage must be checked. |
| `DamagedHelmet` | Yes | Tracked glTF/project textures | Compact metallic-roughness, normal/AO/emissive material | One asset cannot prove broad material coverage. |
| `DiffuseTransmissionPlant` | Yes | Tracked glTF/project textures | Alpha/transmission-adjacent stress and layered plant source | True transmission/blend rendering is not implemented; the title must not imply it is. |
| `CesiumMan` | Yes | Tracked glTF/project texture | Skeleton, skinning, animation, coordinate normalization | Needs raster/ray deformation equivalence evidence. |
| `ModernSponza`, `ModernSponzaCandles`, `ModernSponzaKnight` | Yes | External packs marked download/runtime supported, including parent dependencies | Large scene/materials; emissive-instance stress; FBX skeletal animation | Multi-GB acquisition, license verification, cook/runtime and memory evidence remain open. |
| `BistroExterior`, `BistroInteriorWine` | Yes | External Bistro pack marked download/runtime supported | Large FBX scene, outdoor/indoor lighting/material variants | Import fidelity and memory/performance evidence remain open. |
| `LPSHead` | Yes | External pack marked download/runtime supported; conversion helper tracked | Skin/head shading | Conversion/provenance and final material correctness remain open. |
| `CornellBox` | Yes | External pack marked download/runtime supported; conversion helper tracked | Indirect/reference lighting and convergence | Conversion and reference oracle must be recorded. |
| `JungleRuins` | No | Download supported, runtime unsupported | Future dense USD/out-of-core scene | USD composition and virtualized/out-of-core geometry absent. |
| `SanMiguelHigh`, `SanMiguelLow` | No | Download supported, runtime unsupported | Future large-scene tiers | Deterministic OBJ/MTL/PNG-to-glTF conversion absent. |

## Asset-Pack Catalog

The 13 asset-pack records encode root/extraction/required path, parent relation, source URL/page, archive name/size/hash when known, version, license, and explicit download/runtime flags/blockers.

- 9 packs are marked DownloadSupported; 7 are also RuntimeSupported.
- Unsupported families are explicit: Modern Sponza Ivy (density/residency/scaling), Trees (alpha foliage/transparency), Flood (Alembic/water/sequence), Explosion (OpenVDB/volume/streaming), Jungle Ruins (USD/out-of-core), and San Miguel runtime conversion.
- External archive license strings are metadata, not proof that redistribution rights and notices are complete.

## Workload-To-Capability Coverage

| Capability slice | Best current Showcase source | What still must be observed |
| --- | --- | --- |
| No-content lifecycle | Empty | startup, minimize/restore, settings, clean exit, no silent content error |
| Compact static PBR | DamagedHelmet | all material channels in raster and ray GBuffer |
| General static scene | Sponza / ABeautifulGame | import/cook/load, variants/alpha boundaries, backend parity |
| Skeletal animation | CesiumMan / ModernSponzaKnight | playback, skinning, motion vectors, BLAS/ray identity |
| Instancing/emissive scale | ModernSponzaCandles | import grouping, draw savings, emissive contribution, memory/frame time |
| Indirect/reference | CornellBox | controlled convergence and ReSTIR/reference comparison |
| Large scene | Bistro / ModernSponza | bounded import/cook/load/residency, stable switch/reload/exit |
| Unsupported-content honesty | Plant, Jungle, San Miguel, future add-ons | blocked/unselected presentation and absence of misleading success |

## Vertical Launch Trace

Launcher loads `Levels.catalog` -> selected level/pack readiness is evaluated -> missing supported packs sync -> AssetCooker cooks the selected project -> Launcher verifies executable and cooked mesh/texture/shader roots -> `levels.run` sets project/level/API environment and starts ShowcaseEditor or ShowcaseRuntime in the project directory -> GameFramework registers only selected/ready levels -> requested level loads or explicit warning falls back to Empty -> Renderer consumes the scene.

## Explicit Non-Capabilities And Risks

- Showcase is an evidence application, not a general game: no game rules, audio, physics, networking, save data, installer, or end-user onboarding loop exists.
- A catalog description is product intent, not implementation proof. In particular, “Diffuse Transmission” and future volume/water/foliage descriptions exceed current Renderer capability.
- Catalog Selected does not guarantee source pack presence, successful cook, registered runtime level, or release disposition.
- No level in this pass was built, cooked, launched, captured, timed, or tested from packaged bytes.

