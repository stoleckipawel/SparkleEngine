# Showcase Product Capability Inventory

**Status:** capability snapshot; current product/catalog view, not evidence that every level is downloaded, cooked, runnable, or releasable

**Snapshot:** 2026-09-08 at committed `master` revision `ffe60e3a`; Showcase marker/CMake/entry points, level catalog, authored level files, tracked source content, Launcher use, runtime startup/fallback, and product membership inspected; evidence `S` only

**Scope:** shipped project products, startup/selection behavior, cataloged workloads, content provenance/readiness, and their role as capability evidence

**Owner:** `Projects/Showcase`; Engine/Application/Launcher/Cooking own execution infrastructure

**Evidence and disposition:** [Capability Evidence Plan](../../CapabilityEvidencePlan.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

**Current readiness:** **50/100** for the tracked runtime product route — project/catalog/source integration exists; content readiness, executable workloads, package, first-use, and release evidence remain open. See [Current Feature Readiness](../../../../Acceptance/CurrentReadiness.md#product-build-and-delivery).

## At A Glance

| Product concern | Current state | Evidence boundary |
| --- | --- | --- |
| project products | separate minimal ShowcaseEditor and ShowcaseRuntime targets | source/build membership, not a successful or packaged launch |
| selection | environment/Launcher selects only registered ready catalog levels; unknown falls back to Empty with warning | fallback can hide missing content unless diagnostics are reviewed |
| content ladder | compact tracked scenes plus larger downloadable Modern Sponza, Bistro, LPS Head, and Cornell Box routes | selection/download support is not cook/runtime/readiness evidence |
| future workloads | Jungle Ruins and San Miguel catalog targets remain runtime unsupported | catalog vocabulary does not establish USD/OBJ conversion or out-of-core features |
| acceptance role | levels exercise import, world, Renderer/RHI, diagnostics, performance, and product journeys | one visible level cannot approve the whole feature matrix |

The project should stay thin: it selects content and composes engine/application products, while engine modules and tools retain feature ownership. Scene-specific fixes belong in the owning importer, world, Renderer, or RHI contract unless the behavior is intentionally authored content.

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

## `FCR-PROD-01` Runtime Consumer Contract

This is the Architecture acceptance owner for the Showcase consumer journey. [Application](../../Engine/Application/README.md#product-contract-boundary) owns host construction and settlement; the [frozen product contract](../../../../Acceptance/FirstRelease.md#frozen-product-and-output-classification) owns release values; [release gates](../../../../Acceptance/FirstRelease.md#release-gates) are linked, not repeated.

| ID | Binary acceptance criterion |
| --- | --- |
| `AC-PROD01-01` | From the included archive as a standard user, with no repository, environment override, administrator right, network, or developer tool, `bin/ShowcaseRuntime.exe` reaches the intentional first-run surface within the frozen startup budget and reports the exact product/package identity. |
| `AC-PROD01-02` | Selecting each included example publishes the requested catalog identity and first correct frame within its load budget; missing, corrupt, unsupported, or unregistered content produces a distinct failure and never claims success through built-in `Empty`. |
| `AC-PROD01-03` | The consumer can discover controls/help, inspect requested versus active settings, restore documented defaults, return to the first-run surface, and quit without a console command; persistence and reset touch only the frozen per-user paths. |
| `AC-PROD01-04` | `ShippingGame` contains and exposes no runtime/editor console, CVar authoring route, shader recook, editor panel, source importer/cooker, repository marker dependency, or developer-only command. |
| `AC-PROD01-05` | Close/quit during idle and level load settles level/task/Renderer/RHI work within the shutdown budget, returns one success/failure exit status, and leaves immutable package/source bytes unchanged. |

| ID | Cause/injection, detection boundary, safe result, and recovery |
| --- | --- |
| `FM-PROD01-01` | Remove all `SPARKLE_*` overrides and launch from an arbitrary working directory. Package/Application discovery must still resolve the product; any environment or repository dependency blocks `AC-PROD01-01`. |
| `FM-PROD01-02` | Remove or corrupt a copied included level/product. LevelSession/consumer UI must keep the last accepted state or stop, name the failed identity, and offer retry/back; silent `Empty` activation fails `AC-PROD01-02`. |
| `FM-PROD01-03` | Press tilde/backtick and enumerate imports/files in `ShippingGame`. Any developer console or authoring/debug surface fails `AC-PROD01-04`; quit must remain available through consumer UI/window close. |
| `FM-PROD01-04` | Truncate settings, deny the mutable root, then invoke reset. Invalid state is rejected/quarantined, the user gets one actionable path, and neither package nor repository files change; corruption or source mutation fails `AC-PROD01-03`. |
| `FM-PROD01-05` | Quit during load and repeat launch/exit. New work stops, in-flight work settles/cancels, and no process or partial user file remains after the budget; hang/leak fails `AC-PROD01-05`. |

| Check | Claims falsified | Smallest route and fixed oracle |
| --- | --- | --- |
| `CHK-PROD01-01` | `AC-PROD01-04`; `FM-PROD01-03` | Build the exact `ShippingGame` product when authorized, inspect link/import/file/string membership, then launch and exercise the console shortcut. Zero developer surface is the oracle; any match or overlay is failure. |
| `CHK-PROD01-02` | `AC-PROD01-01`, `AC-PROD01-03`, `AC-PROD01-05`; `FM-PROD01-01`, `FM-PROD01-04`, `FM-PROD01-05` | On a clean standard-user minimum machine, verify/extract the candidate, launch from an unrelated working directory with overrides absent and network blocked, complete help/settings/reset/quit, and diff created/modified paths and process tree against the frozen path/budget contract. |
| `CHK-PROD01-03` | `AC-PROD01-02`; `FM-PROD01-02` | For every included map, capture requested/active identity and first frame; on isolated archive copies inject missing and corrupt products. Success requires the requested map; each fault must produce the predeclared non-`Empty` failure/recovery. |

Every criterion and failure above maps to at least one check. Candidate results belong in `FCR-PROD-01`; this source/documentation inspection does not pass any row.

## Explicit Non-Capabilities And Risks

- Showcase is an evidence application, not a general game: no game rules, audio, physics, networking, save data, installer, or end-user onboarding loop exists.
- A catalog description is product intent, not implementation proof. In particular, “Diffuse Transmission” and future volume/water/foliage descriptions exceed current Renderer capability.
- Catalog Selected does not guarantee source pack presence, successful cook, registered runtime level, or release disposition.
- No level in this pass was built, cooked, launched, captured, timed, or tested from packaged bytes.
