# I. Bistro And San Miguel Acceptance Workloads

Status: canonical flagship workload contract
Date: 2026-07-26

Last performance-contract reconciliation: 2026-08-12
Scope: content ingestion, material and lighting correctness, raster/ray/path-traced quality, whole-system performance, neural rendering evidence, and portfolio presentation

This document owns scene selection and exact workload-specific proof gates. [Principal Graphics Requirements](../Strategy/Requirements.md) owns capability/evidence meaning, [Engineering Standards](Standards/README.md) owns reusable implementation and measurement rules, and [Performance Diagnostics Architecture](../Architecture/PerformanceDiagnosticsArchitecture.md) owns metric/population/provenance semantics used by these gates. Revalidate external capture capability through the [External Performance Profiler Runbook](../Architecture/DiagnosticsProfilerRunbook.md).

## Decision

The ORCA Bistro scene is SparkleEngine's primary product acceptance workload for the next six to twelve months. San Miguel 2.0 is the supported secondary acceptance workload.

This is not a decision to add another sample. It is a decision to use one production-shaped scene to force the renderer, content pipeline, tools, measurement practice, and public evidence to converge on a credible result.

The desired outcome is:

> Sparkle loads the complete Bistro exterior and interior through a documented content path, renders its declared material classes and lighting correctly, sustains a predeclared interactive budget on named hardware, diagnoses its limiting CPU/GPU/memory behavior, and exposes the result through a short demo plus reproducible specialist evidence.

Current Sponza remains the fast smoke and regression workload. A Sponza-only result cannot close a Bistro gate or prove flagship readiness.

The intentional user-facing scene set is:

| Tier | Scene | Product role |
| --- | --- | --- |
| Tier 0 | Sponza | Fast smoke, CI, and short regression loop. |
| Tier 1 primary | Bistro exterior and interior | Flagship material, lighting, scale, streaming/residency, paired-API, ray-tracing, performance, and publication workload. |
| Tier 1 secondary | San Miguel 2.0 | Supported beautiful interior/courtyard workload for difficult indirect light, visibility, texture pressure, path-tracing convergence, and cross-scene generalization. |

This trio is recognizable in graphics practice and varied enough to make measurements informative. It is a supported gallery, not three competing flagship narratives.

This workload is the shared proof surface for [the canonical requirements](../Strategy/Requirements.md), [the gap assessment](../Strategy/GapAssessment.md), [the execution roadmap](../Strategy/Roadmap.md), [the executive direction](../Strategy/ExecutiveSummary.md), and [the engineering persona](../Strategy/EngineerPersona.md).

## Why Bistro

The official ORCA distribution is a professionally created scene released under Creative Commons Attribution 4.0. It supplies separate interior and exterior content in FBX plus Falcor scene files. The published geometry counts are:

| Content | Published triangle count |
| --- | ---: |
| Interior | 1,046,609 |
| Interior with wine | 1,293,691 |
| Exterior | 2,832,120 |

Official source and attribution instructions: [Amazon Lumberyard Bistro, ORCA](https://developer.nvidia.com/orca/amazon-lumberyard-bistro).

Bistro is large enough to expose problems that the current Sponza loop may hide:

- source import fidelity and repeatability;
- large material and texture sets;
- opaque, cutout, transparent, emissive, foliage, glass, metal, painted, masonry, fabric, wood, and food-like surface categories;
- indoor/outdoor exposure range and difficult indirect lighting;
- dense visibility, overdraw, shadow, reflection, and ray-tracing work;
- CPU scene assembly, draw/dispatch generation, descriptor pressure, pipeline variation, texture residency, and upload behavior;
- BLAS/TLAS build time, memory, compaction, update policy, and traversal behavior;
- temporal stability while moving through disocclusion, fine alpha-tested geometry, glossy response, and lighting transitions;
- a visually legible before/after story for a reviewer who will not inspect the entire repository.

The list above defines categories to inventory; it is not a claim that every original asset or every advanced BRDF lobe is already supported.

## Bistro Source-Scene Structure

Bistro is one recognizable content family, but the official archive intentionally distributes separate render scenes:

| Source scene | Meaning | Sparkle use |
| --- | --- | --- |
| `BistroExterior` | Complete street/building exterior. | Primary scale, outdoor-lighting, foliage, visibility, streaming, and exterior hero workload. |
| `BistroInterior` | Original interior scene retained in the publisher archive. | Source/reference baseline only; not exposed as a separate Sparkle level because the Wine revision supersedes it. |
| `BistroInterior_Wine` | Modified version of the original interior with filled wine glasses and additional material parameters in the accompanying scene description. | Complete exposed interior workload and preferred difficult-material hero; it replaces rather than supplements `BistroInterior`. |

The exterior and interior are not currently proven to be spatially aligned pieces of one seamless Sparkle world. Expose `BistroExterior` and `BistroInteriorWine` as the two selectable workloads in one `Bistro` family. Keep the original interior only as publisher-source evidence for validating the Wine revision. Add a combined `BistroFull` level only after asset inspection proves compatible coordinates, scale, overlap, entrances, lighting, and acceptable duplicate content. Do not manually force them together merely to claim a continuous traversal.

The source README also distinguishes emissive surfaces intended for ray-traced illumination from analytic lights intended for raster rendering. The lighting configuration must prevent accidental double lighting.

## Why San Miguel Is Required Too

San Miguel is an iconic rendering-research scene: it appeared on the cover of the second edition of *Physically Based Rendering* and remains in use throughout the book. The maintained 523 MB 2.0 archive provides corrected high- and lower-polygon OBJ/PNG versions under Creative Commons Attribution 3.0.

Authoritative context and sources:

- [Physically Based Rendering, fourth-edition preface](https://www.pbr-book.org/4ed/Preface);
- [San Miguel 2.0 in the McGuire Computer Graphics Archive](https://casual-effects.com/g3d/data10/index.html).

San Miguel earns a permanent place because it exposes a different failure profile:

- enclosed and semi-enclosed light transport rather than Bistro's dominant street/exterior scale;
- difficult sun-to-interior paths, deep occlusion, indirect illumination, and convergence;
- dense architectural detail and repeated objects;
- many close material transitions in a coherent, visually recognizable composition;
- matched high- and lower-polygon versions that provide a controlled geometry-scaling experiment without changing the composition;
- a second scene on which to detect camera-specific optimization, reference-renderer mistakes, and neural overfitting.

Supporting San Miguel means deterministic acquisition/import/cook/load of both variants, declared material coverage, frozen camera routes, correct raster/hybrid output, high-sample reference output, benchmark results, and a public high-detail hero result. The lower-polygon variant is a diagnostic control, not a second beauty target. Supporting San Miguel does not require duplicating every Bistro article or optimization study.

## Repository Reality At Adoption

The 2026-07-26 source audit found:

| Area | Current evidence | Consequence |
| --- | --- | --- |
| Catalog | `Projects/Showcase/Levels.catalog` declares an external, unavailable `Bistro` optional pack rooted at `Assets/Meshes/Bistro`. | The intent exists, but there is no runnable level or acceptance contract. |
| Content | No Bistro asset directory or Bistro `.level` file is present. | Nothing currently proves download, import, cook, launch, or rendering. |
| Source import | The source importer accepts FBX through Assimp and glTF through cgltf. The current FBX path calls static geometry and material importers, not source camera/light importers. | The official FBX can be inspected directly, but geometry/material fidelity must be measured and Falcor camera/light intent must be translated explicitly rather than assumed. |
| FBX material fidelity | The current path maps base color/opacity, emissive, metallic, roughness, and common texture slots. It does not currently populate the imported material name or map FBX alpha cutoff/double-sided state. | Run a direct-FBX loss inventory before choosing it as the canonical path; a deterministic conversion may be more faithful. |
| Core material data | Base color, normal, roughness, metallic, ambient occlusion, emissive, index-of-refraction-derived F0, alpha mode/cutoff, and double-sided state have import/runtime representations. | A useful PBR base exists. It does not prove correct Bistro conversion or drawing. |
| Raster transparency | Alpha-mask clipping is implemented. Alpha-blend metadata reaches shaders, but the inspected D3D12 and Vulkan graphics pipelines disable render-target blending and no transparent raster draw path was found. | Transparent Bistro materials are currently a real P0 gap; do not call them supported until ordering/compositing, depth policy, paired-backend behavior, and reference images pass. |
| Advanced glTF material lobes | Clearcoat, transmission, volume, specular, sheen, iridescence, diffuse transmission, anisotropy, and dispersion are reported as unsupported and approximated. | "Full material coverage" must be a declared support matrix with truthful fallbacks, not an undefined claim. |
| Renderer | Paired explicit API backends, frame graph, ray-tracing scene paths, reference/path-traced lighting, ReSTIR, material buffers, and reconstruction plumbing exist. | Bistro should consolidate and validate existing depth before another broad rendering feature is added. |
| Existing scene | Sponza is required and the startup default. | Preserve it as the short loop while Bistro becomes the long acceptance run. |
| San Miguel | No catalog entry, assets, level, or verified import path is present. | Add it as an external optional pack after the shared provenance/inventory path is proven on Bistro. |

No document may upgrade these facts into an implemented or verified Tier 1 claim until the corresponding gate below has evidence.

## Product Scope And Non-Goals

### In scope

- complete Bistro exterior/wine-interior and San Miguel high/low ingestion;
- deterministic asset conversion/cooking with an inventory report;
- an explicit material support and fallback matrix;
- raster and ray/path-traced reference views;
- high-quality physically based lighting and exposure;
- D3D12/Vulkan correctness and workload comparison;
- whole-system CPU/GPU/memory/residency/streaming profiling;
- one neural GI denoising/reconstruction feature evaluated on Bistro;
- a short public demo, three specialist case studies, and reproducible artifacts.

### Not in scope

- checking the source asset into the main Git repository by default;
- hand-editing hundreds of materials without recording transformations;
- claiming every possible glTF material extension;
- building USD, virtual geometry, a general profiler, or a general ML runtime merely because a larger scene could use them;
- hiding unsupported materials behind visually convenient substitutions;
- optimizing one camera while regressing the rest of the scene;
- training and testing a neural model on the same cameras or only one scene;
- tuning for one vendor and presenting the result as architecture-independent.

## Asset Acquisition And Provenance Contract

Bistro and San Miguel remain external asset packs.

Before the first import:

1. record the official download page, asset version or archive name, download date, archive hash, license, and required attribution;
2. keep the original archive immutable outside the repository;
3. add a scripted or precisely documented transformation from original FBX/Falcor content to Sparkle source/cooked content;
4. store transformation tool versions, command lines, warnings, manual exceptions, and output hashes;
5. keep generated heavyweight media outside normal source history;
6. provide a small manifest and acquisition instructions in the repository;
7. include visible attribution in demo/video/article material when the scene is shown.

An undocumented converted glTF is not a reproducible content path. A direct FBX import is acceptable only if its material, transform, tangent, texture, light/camera, and instance results pass the same inventory and image gates.

Sparkle does not currently expose OBJ as a source-import extension. For San Miguel, start with a pinned, deterministic OBJ/MTL/PNG-to-glTF conversion and retain the semantic inventory before and after conversion. Add direct OBJ support only if it is measurably more faithful or simpler for actual users than the conversion path; supporting one scene is not sufficient reason to broaden the importer.

## Target Showcase Entries

Commit the small level/catalog/configuration records; keep the media external. The launcher should group records by scene family so benchmark variants do not look like unrelated worlds.

| Logical family | Stable variant ID | Pack | Meaning and default behavior |
| --- | --- | --- | --- |
| `Sponza` | `Sponza` | core | Repository-resident CI/smoke route; independently selectable like every other map. |
| `Bistro` | `BistroExterior` | `Bistro` | Selectable when present; primary exterior hero and scale route. |
| `Bistro` | `BistroInteriorWine` | `Bistro` | Complete modified interior with filled wine glasses; preferred difficult-material and interior hero variant after validation. |
| `Bistro` | `BistroFull` | `Bistro` | Conditional future combined level; unavailable until spatial composition is verified. |
| `SanMiguel` | `SanMiguelHigh` | `SanMiguel` | Same hacienda composition at high detail; default user-facing and hero/reference variant. |
| `SanMiguel` | `SanMiguelLow` | `SanMiguel` | Same composition at reduced geometry; controlled scaling and lower-end fallback experiment. |
| `ModernSponza` | `ModernSponza` | `ModernSponzaCurtains` | User-facing Modern Sponza base: the publisher base plus Colorful Curtains. The plain base is an internal acquisition dependency and is not exposed as a level. |
| `ModernSponza` | `ModernSponzaCandles` | `ModernSponzaCandles` | Base plus the Emissive Candles add-on; the parent base is acquired automatically when selected. |
| `ModernSponza` | `ModernSponzaKnight` | `ModernSponzaKnight` | Base plus the Animated Knight add-on; animation support remains subject to the normal importer/cooker gates. |

Target pack roots remain project-owned and predictable:

- `Bistro` -> `Projects/Showcase/Assets/Meshes/Bistro`;
- `SanMiguel` -> `Projects/Showcase/Assets/Meshes/SanMiguel`;
- `ModernSponza` and its add-ons -> `Projects/Showcase/Assets/Meshes/ModernSponza`;
- `JungleRuins` -> `Projects/Showcase/Assets/Meshes/JungleRuins`.

When a pack is absent, the launcher/level picker should show it as unavailable with provenance/acquisition instructions; default build, cook, CI, and Sponza launch must remain usable. When present, discovery must not require source edits or a scene-specific executable.

### Asset Pack Sync Contract

All external maps are absent from the repository by default. Quick Start presents the `Explore Sparkle` hero followed by one `Run` thumbnail action for each runtime-supported level, including the repository-resident asset-free `Empty` control, plus Sync All/Clean All actions for catalog-wide maintenance. A card Run makes its chosen level active, acquires only the packages referenced by that level when missing, resolves build and cook prerequisites, and launches the runtime with that level selected. `Sync All` acquires every downloadable package and excludes download-disabled future packs; it does not sync code dependencies. The sync path caches publisher archives in the per-repository user-local launcher state, validates published byte counts and catalog-pinned SHA-256 values before extraction, and transactionally publishes extracted content with its provenance manifest into gitignored project content roots. Every Modern Sponza acquisition resolves the publisher base and Colorful Curtains first; later add-ons layer on that user-facing baseline.

The launcher must distinguish `ready`, `selected`, `source ready`, and `future` states. A source archive being present does not make a workload runtime-supported. Runtime-unsupported packages remain visible with their official source page and exact blocker, but their Run action remains disabled. Catalog-wide Sync All may acquire a pack whose download metadata is supported without admitting a runtime-unsupported level to cook or launch paths. Packages with unsupported acquisition remain disabled and cannot enter a sync plan.

| Asset pack | Launcher state | Runtime/use contract |
| --- | --- | --- |
| Bistro | Opt-in download | Direct FBX source route; exterior and the superseding wine-interior level records are available after acquisition and cook. |
| San Miguel 2.0 | Opt-in source download | The high/low level records remain non-selectable until the deterministic OBJ/MTL/PNG-to-glTF route is implemented and verified. |
| Modern Sponza publisher base | Internal dependency | Plain glTF source archive; acquired transitively and never exposed as its own level. |
| Modern Sponza | Opt-in download | Publisher base plus Colorful Curtains; the single user-facing base map. |
| Emissive Candles | Opt-in add-on download | Layered on the base-plus-curtains composition; light-import losses must remain explicit. |
| Animated Knight | Opt-in add-on download | Layered on the base-plus-curtains composition through the FBX animation route. |
| Ivy | Visible, disabled, never downloaded | Future geometric-foliage density, residency, and scaling work. |
| Trees | Visible, disabled, never downloaded | Future alpha-card foliage and transparency work. |
| Flood | Visible, disabled, never downloaded | Future Alembic animation, water shading, and sequence playback work. |
| Volumetric Explosion | Visible, disabled, never downloaded | Future OpenVDB import, volume rendering, and volume-sequence streaming work. |
| Jungle Ruins | Opt-in source download | Source acquisition is supported now. Runtime selection remains blocked until USD composition plus virtualized or out-of-core geometry is an explicit implemented program. |

The catalog contains the official source/download URL, landing page, archive name, expected bytes, publisher version, license summary, extraction root, required payload path, parent relationship, and support blocker. Every enabled download also pins the expected SHA-256; disabled future packs do not claim an unverified digest. The license embedded in each downloaded archive remains authoritative.

### Current integration evidence (2026-08-02)

This is an integration smoke record, not a quality, fidelity, or performance-gate closure. It used `DevelopmentEditor` on Windows 11 build 26200, an AMD Ryzen 9 8940HX, 64 GiB system memory, and an NVIDIA GeForce RTX 5070 Ti Laptop GPU with Windows driver `32.0.16.1047`. Process-memory samples were taken after the editor had remained responsive for the stated window. No FPS, frame-time percentile, peak-memory, D3D12/Vulkan comparison, reference-image, or visual-fidelity claim was produced.

| Workload | Acquisition/cook evidence | Launch evidence | Honest status and remaining gaps |
| --- | --- | --- | --- |
| Historical built-in seven-level set | The pre-cleanup combined scene stage passed; all 163 referenced textures cooked. | Every then-present built-in level remained responsive in a six-second per-level sweep; working set was 697–700 MiB and private memory was 4.07–4.28 GiB. | This evidence predates removal of the duplicate `SponzaPtlas` level. The current six repository-resident maps build and cook, but this row is not a replacement launch sweep or a frame-time/image-quality result. |
| Bistro source archive | NVIDIA archive byte count matched `894377473`; SHA-256 `0d50e3c724c6c5da19f8eb99ad3f53e36fec37ffa2df9621f9ccf0603f3934e1`. The isolated scene stage and 408-texture stage passed. | Exterior, original interior, and Wine interior all remained responsive for 12 seconds in the historical combined-registry validation; working set was 697–702 MiB and private memory was 4.03–4.05 GiB. | The product now exposes only Exterior and the superseding Wine interior. Legacy FBX specular-color and separate opacity maps currently use declared scalar/material fallbacks; camera/light intent and transparent-material fidelity remain unproven. |
| Modern Sponza publisher base | Intel archive byte count matched `3987608266`; SHA-256 `b8bb853884ab1566b3beb35666bd09882a4e0dc16661e4684e103792cf0229b9`. The isolated scene stage and 104-texture stage passed. | Remained responsive for 12 seconds in the historical combined registry at about 697 MiB working set and 4.03 GiB private memory. | Retained as an internal source dependency, not a selectable level. Secondary UV sets and normal-strength scalars are accepted but not represented by the current material runtime; malformed authored tangents may be regenerated. |
| Modern Sponza | Intel Curtains archive byte count matched `786898766`; SHA-256 `3ba96e967c8f5ad0a133309cedb342e3563f9cccb42d04e188f55c0f2125bb65`. Base-plus-curtains scene cook and 114-texture stage passed. | Remained responsive for 12 seconds at about 699 MiB working set and 4.03 GiB private memory. | This is now the single user-facing base composition. Transparency and reference-image fidelity remain open gates. |
| Emissive Candles | Intel archive byte count matched `3190731713`; SHA-256 `f8a43d972f377e7eb25e52fdc92faed425ad001a4516e0ecef436ff2f8663396`. Base-plus-add-on scene cook and 104-texture stage passed. | Remained responsive for 12 seconds at about 697 MiB working set and 4.03 GiB private memory. | Usable as an add-on stress level. This proves loadability, not correct source-light import, emissive-GI contribution, or acceptable frame time for the intended candle count. |
| Animated Knight | Intel archive byte count matched `1202508298`; SHA-256 `9112d9789ab2da50c77529907833bd008e5fa602f89438a5c1e82d7d4bcde2a5`. Helper curves/points were excluded after Assimp primitive separation; renderable triangles remained strict. Mesh, skeleton, and animation products were emitted. | Remained responsive for 12 seconds at about 702 MiB working set and 4.03 GiB private memory. | Usable as an add-on level. Skeleton/animation cook and runtime loading passed; visible animation playback still needs a capture or deterministic pose/motion assertion. |
| All supported external levels together | One combined scene cook passed and 669 textures cooked, proving coexistence and deterministic output-path uniqueness. | All three Bistro source scenes (including the now-unexposed original interior) and the four historical Modern Sponza source compositions launched independently from the same registry with clean stderr. | The exposed Modern Sponza family now has three levels and always includes curtains. Large startup bursts are bounded to 16 concurrent mesh preparations and 16 concurrent texture loads; pending material textures use semantic defaults until residency publishes a new binding revision. |
| San Miguel 2.0 | Archive byte count matched `535519642`; SHA-256 `85874077735808150e679b3c71d70a37a270cb8833f4911325aa1099da3f7d4a`; required `san-miguel.obj` is present after staged acquisition. | Not launchable. | Acquisition is verified, but the scene cooker accepts only glTF, GLB, and FBX. The two level records remain non-selectable until a deterministic OBJ/MTL/PNG conversion or importer is implemented. |
| Jungle Ruins | Archive byte count matched `4254165506`; SHA-256 `f6b44e81af0515161eb9e2a5cf6f7c24bb82beda439fb8e82c4e5ad479881bee`; required `USD/JungleRuins_Karma.usda` is present after staged acquisition. | Not launchable. | The level is visible but non-selectable. USD composition and virtualized or out-of-core geometry are still absent. |
| Ivy, Trees, Flood, Volumetric Explosion | Not downloaded by this validation. Their publisher metadata remains cataloged. | Not launchable. | Controls remain disabled with explicit foliage-density, alpha-card/transparency, Alembic/water, and OpenVDB/volume-sequence blockers. |

The sync script was also rerun idempotently against acquired San Miguel content and rejected a root-traversal probe before download or extraction. Publishing now preserves the previous extraction in a pack-specific backup and restores it when the staged directory cannot be published.

After the final rebuild, Windows Application Control on this workstation began rejecting the newly linked, unsigned `DevelopmentEditor` `TextureCooker.exe` and `ShowcaseEditor.exe` hashes. This is a host execution-policy/tool-signing gate rather than a content failure: the affected sources compiled, the supported-level launch evidence above was captured before the refreshed hashes were rejected, and the restored default request was revalidated through the permitted `DebugEditor` texture cooker (163 textures). Developer-artifact signing or an explicit local trust policy remains separate follow-up work.

During runtime validation the configured NVIDIA upscaler and ray-reconstruction provider could not initialize in one launch configuration. Provider initialization now reports a warning and falls back to linear upscaling or disables ray reconstruction instead of terminating the scene. This fallback keeps the showcase usable; it is not evidence that NVIDIA reconstruction passed.

### Incremental Per-Level Verification Program

The combined smoke record above proves coexistence only. Visual quality and measured performance are accepted one level at a time. At most one level may be `In review`; the next level does not start until the current checkpoint is marked `Accepted`, `Accepted with follow-up`, `Deferred`, or `Rejected`. This prevents one broken camera, material, or performance result from being hidden inside a bulk pass.

#### Verification-code readiness audit

| Capability | Current state | Required action before measured map review |
| --- | --- | --- |
| Asset-pack acquisition and provenance | Ready for Bistro, Modern Sponza base/add-ons, San Miguel source, and Jungle Ruins source. Archive bytes and acquisition SHA-256 are recorded. | Reuse the existing transactional sync path; never download a disabled future add-on as part of verification. |
| Level catalog and per-map run | Ready. The launcher exposes Run per runtime-supported map and Sync All/Clean All for the catalog; one internal `Selected` state remains the active-set authority. | Snapshot and restore catalog state around every external-level checkpoint. Only the active family may be enabled for the run. |
| glTF/GLB/FBX import and cooking | Ready for the 11 runtime-supported catalog levels, including the built-in `Empty` fallback. The cooker command is project-scoped rather than level-scoped. | Record exactly which selected level assets were required and which products were reused. Do not call a project cook an isolated map cook until a level-filtered cook request exists. |
| Startup-level selection | Ready through `SPARKLE_STARTUP_LEVEL`. | Put the requested and actually active level names in the evidence manifest; a responsive process alone is insufficient. |
| Deterministic camera | Partial. Every supported level has a serialized initial camera, but named verification routes and reference-image alignment are not complete. | Freeze the initial camera for the first pass. Add further named cameras only as separate, reviewable evidence routes. |
| Level/streaming readiness | Partial. Level loading, bounded mesh preparation, and bounded texture loading exist, but automation has no single published `settled` signal. | Publish an evidence-ready state only after the requested level is active, scene generation is current, and mesh/texture preparation queues are empty. Warm-up starts after this signal. |
| Screenshot capture | Manual path ready through `Capture Viewport` and renderer readback. Output is BMP under `Saved/Captures` with no level/config sidecar. | Add a deterministic evidence request that writes a level-named PNG or BMP plus frame/config metadata. The capture must use the rendered viewport, not a desktop screenshot. |
| CPU frame timing | Partial. The editor displays an instantaneous ImGui delta only. | Export a post-warm-up sample window with CPU frame median and P95. Define CPU frame cost as unscaled application begin-to-begin time and record whether presentation wait is included. |
| GPU frame timing | Partial. D3D12/Vulkan timestamp scopes and `r.Diagnostics.GpuTiming` exist, but resolved values are private and not exported. | Aggregate top-level graphics-queue timestamp scopes into GPU-active milliseconds and export median and P95 for the same sample window. Do not infer GPU milliseconds from utilization. |
| Fixed launch resolution | Missing. The application currently starts maximized; the validation workstation produced a `5120 x 1392` client extent. A live D3D12 resize probe failed in `ResizeBuffers` with `0x80004004`. | Add a deterministic startup window/client extent and validate swap-chain creation at that extent. Do not resize during a measured run. |
| Evidence manifest | Missing. Existing logs, captures, hashes, and memory samples are separate. | Emit one machine-readable manifest containing level, commit, profile, API, adapter/driver, CPU, resolution, renderer CVars, provider/fallback state, warm-up/sample frames, timing summary, memory summary, logs, and capture path. |

The first implementation checkpoint is therefore `MAP-00 Evidence Harness`. It is not a scene-quality pass. It closes the readiness, resolution, timing-export, capture-naming, and manifest gaps above, then proves the harness on current Sponza without accepting Sponza itself.

#### Per-level checkpoint stages

Every supported map uses the following sequence. A checkpoint folder owns its own logs and evidence; results from a different level cannot satisfy a stage.

| Stage | Required work | Exit evidence |
| --- | --- | --- |
| `MAP-A Scope` | Select one level ID, expected source assets, API/profile, fixed initial camera, fixed renderer settings, and any asset pack/parent. Snapshot opt-in state. | Run manifest draft and catalog snapshot. |
| `MAP-B Acquire` | Verify required files, publisher/version/license metadata, archive bytes, and recorded SHA-256. Built-in content records `repository content` instead of fabricating an archive step. | Provenance block with every required path present. |
| `MAP-C Cook` | Run the required cook, capture warnings/errors, verify the selected scene/mesh/material/texture products, and state which project-wide products were reused. | Cook log, selected-product inventory, deterministic hashes where available, and zero uncategorized errors. |
| `MAP-D Load and settle` | Launch with `SPARKLE_STARTUP_LEVEL`, prove the requested level became active, wait for the evidence-ready signal, and reject device removal, fatal diagnostics, or unresolved selected assets. | Active-level identity, settled frame ID, load duration, warning/fallback list, and clean fatal-error check. |
| `MAP-E Stabilize and measure` | Hold the frozen camera for 300 warm-up frames after readiness, then sample at least 300 frames with VSync/presentation policy recorded. | CPU median/P95 ms, GPU-active median/P95 ms, derived FPS, process working/private memory, tracked/allocator/local/non-local GPU memory where available, valid/original/excluded sample counts, provenance, and raw timing artifact. |
| `MAP-F Capture and inspect` | Capture the lit viewport after the measured window. Inspect exposure, framing, geometry, transforms, materials, textures, normals/tangents, alpha, lighting, animation where applicable, and obvious temporal instability. | Level-named image, capture frame/config sidecar, and a `Pass`/`Warning`/`Fail` observation for every applicable visual category. |
| `MAP-G Review` | Present only this map's screenshot, numbers, logs, known fallbacks, and defects. Classify each defect as content, importer/cooker, renderer, camera/lighting, performance, or harness. | User decision: `Accepted`, `Accepted with follow-up`, `Deferred`, or `Rejected`. No next-map work begins here. |
| `MAP-H Restore` | Restore external packs and level defaults to their prior opt-in state. Preserve accepted evidence and create explicit follow-ups for warnings/failures. | Catalog diff check, evidence links, and closed checkpoint status. |

The first comparison profile is `DevelopmentGame`, D3D12, the discrete NVIDIA adapter, fixed startup resolution, VSync disabled, a frozen initial camera, and recorded image-provider fallbacks. Vulkan, editor overhead, alternate cameras, ray-traced/reference modes, and image-provider comparisons are later routes; they must not be silently mixed into the first per-map number.

`MAP-E` proves one harness run and its definitions. It is not a statistically definitive optimization result; `WL-04` and the Performance Contract below own repeated-run evidence.

#### One-map review order

The order grows from a known architectural baseline through small material/animation tests into the large optional families. `MAP-00` must pass before `MAP-01` begins.

| Checkpoint | Level | Primary purpose | Initial state |
| --- | --- | --- | --- |
| `MAP-00` | Evidence harness using Sponza as calibration input | Deterministic resolution, readiness, screenshot, timing export, and manifest validation. Not a Sponza acceptance result. | Next |
| `MAP-01` | Sponza | Tier-0 architecture, material, lighting, exposure, and performance baseline. | Waiting |
| `MAP-02` | Empty | Clear/presentation/sky/default-resource control with no scene geometry. | Waiting |
| `MAP-03` | Damaged Helmet | Compact metallic-roughness, normal, AO, and emissive material check. | Waiting |
| `MAP-04` | Cesium Man | Skinned mesh and animation baseline. | Waiting |
| `MAP-05` | Diffuse Transmission Plant | Alpha/transmission/two-sided foliage-like material stress. | Waiting |
| `MAP-06` | A Beautiful Game | Repeated meshes, material variants, instancing, and broader texture residency. | Waiting |
| `MAP-07` | Bistro Exterior | First flagship-scale FBX scene, exterior exposure, geometry scale, and streaming stress. | Waiting |
| `MAP-08` | Bistro Interior Wine | Complete interior visibility, exposure, material loss, dense lighting, glass, and wine-material stress. | Waiting |
| `MAP-09` | Modern Sponza | Base-plus-curtains composition, secondary attributes, tangent fallback, transparency, and high-resolution PBR load. | Waiting |
| `MAP-10` | Modern Sponza Emissive Candles | Curtains baseline plus emissive density, source-light loss, and lighting cost. | Waiting |
| `MAP-11` | Modern Sponza Animated Knight | Curtains baseline plus FBX skeleton, animation playback, and motion stability. | Waiting |

San Miguel High/Low and Jungle Ruins receive separate source-readiness checkpoints only after the supported sequence. They do not enter screenshot/performance review until their declared OBJ or USD runtime blockers close. Ivy, Trees, Flood, and Volumetric Explosion remain future entries and are excluded from this run.

#### Evidence layout and review ledger

Raw generated evidence belongs under `artifacts/validation/showcase-levels/<run-id>/<level-id>/`. An accepted checkpoint contains `manifest.json`, `cook.log`, `launch.log`, `timings.csv`, `summary.md`, and one or more level-named viewport captures. Only reviewed, intentionally selected images should be promoted into documentation; multi-gigabyte source archives and disposable raw captures remain outside version control.

For each checkpoint, add one ledger row before beginning the next:

| Checkpoint | Commit/run ID | Result | CPU median/P95 | GPU median/P95 | Capture | Findings/follow-ups | User decision |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `MAP-00` | Pending | Not run | Pending | Pending | Pending | Close harness gaps first. | Pending |

## Stage 1: Inventory Before Rendering

The first deliverable for each Tier 1 scene is an importer/cooker inspection report, not a beauty shot. Build the reusable inspection path on Bistro, then run the same path on San Miguel without adding a scene-specific importer branch.

The inventory must report for Bistro exterior, Bistro wine interior, San Miguel high detail, and San Miguel low detail separately. The original Bistro interior may be inspected only as a source comparison proving what the Wine revision inherits or changes:

- source files, nodes, mesh primitives, instances, vertices, indices, and triangles;
- materials and deduplicated material variants;
- texture references by semantic, format, dimensions, mip count, color space, and byte size;
- missing, ambiguous, embedded, duplicate, or non-portable texture paths;
- alpha modes, cutoffs, opacity values, and double-sided materials;
- emissive materials and source light/camera records;
- tangent/normal/UV availability and generated data;
- degenerate geometry, invalid bounds, unsupported primitives, and import warnings;
- source-to-engine coordinate, unit, handedness, and transform policy;
- cooked disk size, import/cook time, peak process memory, and deterministic output hashes;
- every unsupported or approximated material property.

Acceptance:

- repeated conversion with the same inputs produces the same semantic manifest and hashes;
- all losses and approximations are categorized;
- no manual edit exists only on one workstation;
- the report can identify which source asset produced a selected rendered primitive and material.

## Stage 2: Material Coverage Contract

"Full shading model and material coverage" means complete coverage of the declared Bistro inventory, not support for every material model in the industry.

Each discovered material is assigned to one of:

| Status | Meaning |
| --- | --- |
| `Exact` | Source parameters and textures map to the implemented Sparkle model without known semantic loss. |
| `Converted` | A documented deterministic conversion preserves the intended response within reference tolerances. |
| `Approximated` | Sparkle lacks a source lobe or blend behavior; the deliberate fallback and visible consequence are documented. |
| `Rejected` | The asset cannot be rendered honestly yet and blocks the relevant acceptance route. |

The first supported material envelope is:

- metallic-roughness PBR;
- base-color alpha;
- tangent-space normal mapping;
- roughness, metallic, ambient-occlusion, and emissive maps/factors;
- opaque, alpha-mask, and double-sided geometry;
- index-of-refraction/F0 where the source mapping is trustworthy;
- scene-relevant transparency only after correct sorting/compositing or a deliberately documented alternative exists;
- scene-relevant subsurface, transmission, clearcoat, sheen, or anisotropy only when the inventory proves that their absence materially harms an intended Tier 1 view.

The same implementation must shade San Miguel. Scene-specific material overrides are data and must be reproducible; scene-specific shader forks are not acceptable.

Do not implement advanced lobes by keyword. Rank them by:

`visible area × visual error × flagship-camera frequency × role evidence value ÷ implementation and validation cost`.

Required material evidence:

- a contact sheet grouped by material class;
- base color, normal, roughness, metallic, AO, emissive, alpha, and material-ID debug views;
- three close-ups where the reference response is difficult;
- the support/fallback matrix with counts and named examples;
- raster versus reference-path comparison under the same camera, exposure, and light state.

## Stage 3: Lighting And Image-Quality Contract

Bistro must have two separately versioned lighting configurations:

1. `Authored`: faithful use or documented translation of source camera/light intent.
2. `Stress`: a Sparkle-owned setup designed to expose indirect-lighting, shadow, glossy, emissive, transparency, and temporal failure.

The lighting contract accounts for:

| Domain | Required decision and proof |
| --- | --- |
| Source translation | Inventory source/Falcor lights, cameras, environment, units, transforms, and unsupported records; document every authored-to-Sparkle conversion. |
| Outdoor sun/sky | Declare sun direction/angular size/intensity, sky/environment representation, shadow method, exposure, and reference match on `BIS-EXT-WIDE`. |
| Indoor local/emissive light | Distinguish emissive appearance from sampled illumination; declare local-light types, units, attenuation, shadowing, and any proxy lights. |
| Diffuse indirect light | Compare reference multi-bounce transport with the real-time ReSTIR/path solution in deep interior and arcade crops; show convergence and leaks. |
| Glossy/specular response | Validate roughness/F0, normal mapping, reflection visibility, energy behavior, and denoising on metal/glass/paint/wood close-ups. |
| Alpha/transmission | Separate alpha mask, composited transparency, and physical transmission. Never use blend opacity as evidence of refractive transport. |
| Exposure/display | Freeze auto/manual exposure policy, adaptation timing, tone mapper, gamut/color space, output transfer, and the explicitly labeled exterior/interior scene-switch behavior. |
| Temporal behavior | Evaluate camera motion, disocclusion, history rejection, ghosting, fireflies, and recovery time on both scenes. |

Required reference paths:

- high-sample progressive/path-traced reference at fixed cameras;
- real-time raster/hybrid result;
- real-time ray/path result with the classical denoiser or reconstruction baseline;
- neural result only after its separate model contract is met.

Every comparison freezes:

- camera transform and lens;
- resolution and render scale;
- exposure, tone mapping, color space, and output encoding;
- light transforms, intensities, spectra/colors, and environment;
- material manifest version;
- random seed and sample count where applicable;
- engine commit, shader package hash, asset hash, hardware, operating system, driver, and API.

Quality evaluation combines:

- reference image difference and FLIP;
- temporal error on fixed camera paths;
- convergence curves against samples per pixel;
- cropped failure cases;
- human review of energy conservation, normal response, roughness, alpha edges, disocclusion, ghosting, fireflies, light leaks, and exposure transitions.

PSNR or SSIM may be reported as secondary metrics. A single average score cannot replace visible failure cases.

## Canonical Camera And Motion Routes

The exact transforms are frozen only after the source is loaded, but the route purposes are fixed now:

| Route | Purpose | Required presentation |
| --- | --- | --- |
| `BIS-EXT-WIDE` | Exterior scale, long visibility, sunlight, sky/exposure, draw and ray pressure. | Hero still, raster/path split, GPU capture, memory table. |
| `BIS-EXT-FOLIAGE` | Alpha-tested foliage/signage, fine geometry, temporal stability, overdraw and any-hit pressure. | Slow pan video, alpha/material debug view, before/after profile. |
| `BIS-EXT-MARKET` | Dense materials, glass/metal/paint, local lights and reflections. | Material contact sheet and reflection/denoising crops. |
| `BIS-ENTRY-PAIR` | Matched exterior-entry and interior-entry views; exposure, scene-switch/loading, residency, and history reset. | Explicit two-scene sequence with frame-time/residency trace; never present it as a seamless walk unless `BistroFull` is validated. |
| `BIS-INT-WIDE` | Indirect lighting, shadowed interior, many props, material variety. | Hero still, reference comparison, convergence study. |
| `BIS-INT-CLOSE` | Wine/glass/fabric/food/wood detail and difficult material response. | Lobe/fallback explanation and specialist close-ups. |
| `BIS-INT-MOTION` | Temporal denoising/reconstruction under geometry and lighting change. | Baseline/neural side-by-side video and temporal metric plot. |
| `SMG-COURTYARD-WIDE` | Recognizable courtyard composition, sun/sky response, global visibility, and geometry scale. | Hero still, raster/path split, timing and memory summary. |
| `SMG-ARCADE` | Deep indirect light, columns/arches, shadow transitions, and convergence. | Reference comparison, convergence plot, cropped failure cases. |
| `SMG-DETAIL` | Dense props, texture/material transitions, and grazing response. | Material/debug contact sheet and close-up comparison. |
| `SMG-WALK` | Cross-scene temporal behavior, culling, residency, and neural generalization. | Deterministic traversal, frame-time trace, baseline/neural comparison. |
| `SMG-HIGH-LOW` | Controlled geometry scaling with the matched high/low versions. | Same camera/settings on both variants; CPU extraction, draw/dispatch, memory, BLAS/TLAS, traversal, and quality delta. |

Camera scripts for all source-scene variants must be deterministic and usable by benchmark, screenshot, capture, and video workflows. Exterior-to-interior presentation uses an explicit scene cut or load transition until `BistroFull` passes its composition gate. San Miguel high and low use identical camera transforms and settings.

## Performance Contract

Performance is reported for named machines, not as an unqualified "60 FPS" claim.

The first benchmark machine becomes the `Reference System`; one materially different GPU architecture becomes the `Comparison System` when available. Exact hardware is recorded in the result, not embedded in the engine design.

Provisional gates, to be revised once the first honest baseline is captured:

| Mode | Resolution | Initial gate | Stretch gate | Notes |
| --- | ---: | ---: | ---: | --- |
| Raster/hybrid interactive | 2560×1440 | p95 GPU frame ≤ 16.67 ms | p95 GPU frame ≤ 12.5 ms | Includes full declared material/lighting envelope after warm-up. |
| Ray/path interactive | 1920×1080 | p95 GPU frame ≤ 33.33 ms | p95 GPU frame ≤ 16.67 ms | Fixed sample budget plus reconstruction/denoising. |
| Reference | 1920×1080 or higher | deterministic convergence | lower time-to-quality | Not required to be real-time. |
| Bistro entry pair / scene switch | 2560×1440 | no incorrect/missing resident asset or stale history | no hitch > 50 ms after the target scene is warm | Report the cold switch separately from warm traversal; do not imply one continuous level. |
| San Miguel cross-scene check | same settings as corresponding Bistro mode | correct output and complete metrics | within the declared quality/performance envelope | A different cost distribution is expected and must be explained, not normalized away. |

The benchmark protocol records:

- cold launch, cold content load, warm content load, and time to first correct frame;
- at least 300 warm valid frames per fixed route segment and at least three runs as the acceptance floor, with per-run identity retained;
- CPU and GPU p50/p95/p99 frame times plus worst frame;
- per-pass GPU times and queue overlap;
- scene extraction, culling, frame-graph compile, command recording, submission, and present CPU times;
- draw, dispatch, pipeline, shader package, descriptor, barrier, and queue counts;
- process working set/private commit, tracked GPU allocations/allocator blocks, local/non-local usage and mutable budget, residency where supported, and separately identified process-lifetime/session/run high-water marks;
- texture upload, eviction, mip/residency, and missing-resource events;
- BLAS/TLAS count, source geometry, build/update/compaction time, scratch/result memory, and traversal-sensitive experiments;
- compilation/cache state and pipeline creation hitches;
- image-quality setting, sample count, reconstruction mode, and dynamic-resolution state;
- exact engine/content/configuration hashes.

The minimum run/sample count does not make a result automatically definitive. Analysis follows the [comparison and regression contract](../Architecture/PerformanceDiagnosticsArchitecture.md#comparison-and-regression-contract): per-run distributions are primary, a combined view is secondary, exclusions and tail `FrameId` values remain visible, worst-to-worst comparison requires equal `N` or an explicit model, and the predeclared decision combines absolute/relative practical-effect bands with a correlation-aware uncertainty method. An interval overlapping the practical-effect band is `Inconclusive`; a p-value alone never closes the gate.

Every optimization case study requires:

1. a predeclared hypothesis;
2. a baseline and serial/control case;
3. a capture or counter that can distinguish competing causes;
4. one scoped change;
5. quality and correctness regression checks;
6. before/after distributions, not one frame;
7. an architecture-scoped conclusion and rejected alternatives.

## Required Bottleneck Studies

Complete at least three studies; they must be selected from the measured top costs.

Preferred study classes:

- CPU scene extraction/culling/command generation versus GPU-driven alternatives;
- texture cooking, upload, residency, descriptor access, and exterior/interior scene-switch hitches;
- raster material/overdraw/pipeline variation, especially alpha-tested geometry;
- ray-tracing BLAS/TLAS organization, compaction, instance strategy, build cost, memory, and traversal;
- direct/indirect lighting sample allocation and time-to-quality;
- denoising/reconstruction dispatch, layout, precision, fusion, memory traffic, and temporal stability;
- D3D12/Vulkan workload differences caused by engine/API behavior rather than vendor mythology.
- San Miguel high-versus-low geometry scaling to test whether the suspected bottleneck follows geometry, instance, material, memory, or ray-traversal pressure.

At least one study must end in "do not ship" or "not worth the complexity" if the measurements support that outcome.

## Neural Rendering Contract Across Bistro And San Miguel

Bistro is the flagship evaluation and presentation workload; it is not the sole training or validation data source.

For the fixed neural GI denoising/reconstruction feature:

- training, validation, and test camera/frame identities are disjoint;
- at least one distinct scene is held out for final generalization testing;
- the high-sample reference and noisy/low-sample inputs use frozen generation settings;
- input features, tensor layout, normalization, precision, operators, receptive field, temporal state, and output meaning are documented;
- PyTorch/reference output and HLSL/Slang inference output pass numerical checks on frozen tensors;
- the classical baseline remains available;
- Bistro routes report FLIP, temporal error, latency, memory, and failure cases;
- the held-out scene determines whether a result is a general feature or a Bistro-specific fit;
- training/offline dependencies do not become runtime dependencies.

San Miguel is the required first held-out light-transport scene because it is available in simple OBJ/PNG research formats and presents different interior visibility. It remains a fully supported user scene while its frozen final-test cameras remain excluded from neural training and model-selection decisions.

## Evidence Packages

One workload should produce several narrow stories rather than one enormous "engine tour."

| Package | Scene proof | Requirements advanced |
| --- | --- | --- |
| `CASE-01 Content to Correct Pixel` | Provenance, deterministic FBX/OBJ conversion paths, Bistro and San Miguel material inventories, support matrix, authored/reference cameras, debug views. | `PGE-07`, `PGE-09`, `PGE-13`, `PGE-15` |
| `CASE-02 One Frame, Two APIs` | Same route and settings on D3D12/Vulkan; frame/resource/barrier/descriptor/pipeline/RT-build comparison; difficult incident and reduced repro. | `PGE-05`, `PGE-06`, `PGE-09`, `PGE-10`, `PGE-14` |
| `CASE-03 Path-Traced Lighting Under Budget` | Reference convergence, real-time sample allocation, BLAS/TLAS and lighting cost, quality/performance frontier, failure cases. | `PGE-02`, `PGE-05`, `PGE-08`, `PGE-10`, `PGE-13` |
| `CASE-04 Model to Shader` | Dataset split, model/operator derivation, immutable export, numerical checks, optimized GPU kernels, classical fallback, Bistro presentation and held-out San Miguel results. | `PGE-03`, `PGE-04`, `PGE-08`, `PGE-11`, `PGE-12`, `PGE-13` |
| `CASE-05 Adoption Package` | Clean acquisition/build/cook/run, capability and fallback matrix, tuning guide, peer reproduction, issue template. | `PGE-01`, `PGE-07`, `PGE-13`, `PGE-14`, `PGE-15` |

The public front page shows only:

- one exterior hero result;
- one interior hero result;
- one San Miguel hero result that proves cross-scene breadth;
- one 60–90 second sequence that labels Bistro scene cuts and keeps San Miguel high/low comparisons camera-matched;
- three headline measurements with hardware/configuration;
- links to the case studies, captures, code landmarks, and limitations.

The specialist path contains the detail. Do not place a wall of subsystem names in the recruiter path.

## Companion Scene Decision

The engine should not accumulate flagship scenes.

| Workload | Distinct value | Decision for the next year |
| --- | --- | --- |
| Current Sponza | Small, fast, already integrated; catches startup, basic material, raster, and RT regressions. | **Keep as Tier 0.** Run frequently. Never use alone for flagship claims. |
| Modern Sponza base/add-ons | High-resolution PBR, 4K textures, curtains, animation, alpha-card trees, emissive-light, Alembic water, VDB volumes, and multiple interchange formats. | **Support as an opt-in compatibility family without flagship status.** The exposed Modern Sponza base always combines the publisher base with Colorful Curtains; Emissive Candles and Animated Knight layer on that composition. Ivy, Trees, Flood, and Volumetric Explosion remain visible disabled add-ons and are never downloaded until their named technology gates close. Official sample library: [GPU Research Samples](https://www.intel.com/content/www/us/en/developer/topic-technology/graphics-research/samples.html). |
| San Miguel 2.0 | Iconic PBRT scene; different interior/courtyard visibility, dense detail, and indirect-light transport in a 523 MB corrected high/low OBJ/PNG package. | **Support as Tier 1 secondary.** It receives import, material, reference-quality, benchmark, deterministic-route, high/low scaling, gallery, and neural held-out gates, but Bistro remains the flagship story. Source: [McGuire Computer Graphics Archive](https://casual-effects.com/g3d/data10/index.html). |
| Sun Temple | Recognizable detailed PBR environment with about 1.64 million published vertices in FBX/Falcor form. | **Do not add now.** It overlaps the current material/architecture scene set and has non-commercial share-alike terms. Source: [ORCA scene page](https://developer.nvidia.com/ue4-sun-temple). |
| Emerald Square | Approximately 10.0 million triangles and city-scale geometry; useful for AS memory, culling, and streaming beyond Bistro. | **External post-Bistro benchmark only.** Its CC BY-NC-SA 3.0 terms conflict with uncomplicated future commercial bundling. Do not ship it as product content. Source: [ORCA scene page](https://developer.nvidia.com/orca/nvidia-emerald-square). |
| Jungle Ruins | More than one trillion total triangles in a PBR environment; targets instancing, virtual geometry, and out-of-core rendering. | **Support opt-in source acquisition; defer runtime support.** Keep its level unavailable until USD composition and virtualized/out-of-core geometry gates are implemented. Source: [GPU Research Samples](https://www.intel.com/content/www/us/en/developer/topic-technology/graphics-research/samples.html). |
| Moana Island Scene | 93 GB unpacked base scene, extensive instancing, and complex volumetric light transport. | **Defer beyond the current real-time engine goal.** It is valuable for USD/offline/volumetric research, but its scale, formats, and research/software-development license create a different product. Source and terms: [official dataset](https://www.disneyanimation.com/resources/moana-island-scene/). |
| ALab | Complete production USD scene with hundreds of assets, animation, fur, fabric, variants, and shot structure. | **Defer.** It is an asset-pipeline/USD adoption test, not the shortest path to the target rendering evidence. |

Scene-adoption rule:

> Add another workload only when it exposes a measured failure mode that Bistro and the fast regression set cannot expose, and when fixing that failure advances a named `PGE-*` requirement more than it delays the flagship package.

Cataloging an optional source package or disabled future add-on is not a claim that it is an adopted runtime workload. Modern Sponza compatibility work and Jungle Ruins source acquisition must not displace the Bistro/San Miguel acceptance gates.

## Workload Gate Sequence

The roadmap owns dates. This contract owns the ordered acceptance states:

| Gate | Required state |
| --- | --- |
| `WL-01 Provenance` | Provenance manifest, immutable source archive, automated inspection, import/cook baseline, and complete loss/warning inventory. |
| `WL-02 Deterministic Content` | Bistro variants load deterministically with frozen cameras and a first material matrix; San Miguel high/low provenance and conversion inventories are complete. |
| `WL-03 Correct Baseline` | Correct Bistro raster/hybrid and high-sample references; San Miguel loads through the same pipeline with a reference camera, controlled scaling record, material/debug contact sheet, and honest unsupported lists. |
| `WL-04 Measured Frame` | Deterministic routes, benchmark harness, paired D3D12/Vulkan captures, bottleneck ranking, and one difficult incident log. |
| `WL-05 Neural Baseline` | Neural data/reference generation with disjoint splits, classical baseline, model card, and held-out-scene protocol. |
| `WL-06 Runtime Inference` | Real shader inference, numerical checks, latency/memory profile, Bistro quality frontier, and classical fallback. |
| `WL-07 Reproduction` | Three completed bottleneck studies, regression thresholds, clean acquisition/build/cook/run, and an external reproduction attempt. |
| `WL-08 Publication` | Hero stills, traversal video, three specialist case studies, model-to-shader result, limitations, and reviewer-ready routing. |

## Completion Gate

Bistro is complete for the six-month portfolio only when:

- Bistro exterior/wine-interior and San Miguel high/low content are reproducibly acquired, converted or imported, cooked, and launched;
- each Tier 1 inventory accounts for every material and texture and classifies every loss;
- the flagship cameras have deterministic high-sample references;
- raster/hybrid and ray/path modes meet their declared correctness and measured performance budgets or clearly report the remaining miss;
- D3D12 and Vulkan have comparable semantic output and an explained workload delta;
- CPU, GPU, process RAM, precise local/non-local GPU memory/residency, descriptor, pipeline, and acceleration-structure behavior are measured;
- three causal bottleneck studies are complete;
- the neural feature is real runtime inference with a classical fallback and held-out generalization result;
- a clean reviewer path, source attribution, video, captures, tables, and limitations exist;
- another engineer can reproduce at least one case without private guidance.

San Miguel support is complete when its high/low acquisition/import/cook path is deterministic, all material losses are classified, its routes render correctly in raster/hybrid and reference modes, the controlled high/low performance record exists, and its high-detail gallery hero plus neural held-out result are published. It does not wait for all Bistro-specific case studies to be repeated.

A visually attractive screenshot without these conditions is a milestone, not completion.
