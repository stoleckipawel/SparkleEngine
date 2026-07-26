# I. Bistro And San Miguel Acceptance Workloads

Status: canonical flagship workload contract
Date: 2026-07-26
Scope: content ingestion, material and lighting correctness, raster/ray/path-traced quality, whole-system performance, neural rendering evidence, and portfolio presentation

## Decision

The ORCA Bistro scene is SparkleEngine's primary product acceptance workload for the next six to twelve months. San Miguel 2.0 is the supported secondary acceptance workload.

This is not a decision to add another sample. It is a decision to use one production-shaped scene to force the renderer, content pipeline, tools, measurement practice, and public evidence to converge on a credible result.

The desired outcome is:

> Sparkle loads the complete Bistro exterior and interior through a documented content path, renders its declared material classes and lighting correctly, sustains a predeclared interactive budget on named hardware, diagnoses its limiting CPU/GPU/memory behavior, and exposes the result through a short demo plus reproducible specialist evidence.

Current Sponza remains the fast startup, smoke, and regression workload. A Sponza-only result cannot close a Bistro gate or prove flagship readiness.

The intentional user-facing scene set is:

| Tier | Scene | Product role |
| --- | --- | --- |
| Tier 0 | Sponza | Fast startup, smoke, CI, and short regression loop. |
| Tier 1 primary | Bistro exterior and interior | Flagship material, lighting, scale, streaming/residency, paired-API, ray-tracing, performance, and publication workload. |
| Tier 1 secondary | San Miguel 2.0 | Supported beautiful interior/courtyard workload for difficult indirect light, visibility, texture pressure, path-tracing convergence, and cross-scene generalization. |

This trio is recognizable in graphics practice and varied enough to make measurements informative. It is a supported gallery, not three competing flagship narratives.

This workload is the shared proof surface for [the canonical requirements](A_PrincipalRenderingRequirements.md), [the gap assessment](C_CandidateAndRepositoryGapAssessment.md), [the execution roadmap](F_6_12_MonthPrincipalGraphicsRoadmap.md), [the executive direction](G_AdvancedGraphicsEngineExecutiveSummary.md), and [the engineering persona](H_AdvancedGraphicsEngineerPersona.md).

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

## Why San Miguel Is Required Too

San Miguel is an iconic rendering-research scene: it appeared on the cover of the second edition of *Physically Based Rendering* and remains in use throughout the book. The maintained 523 MB 2.0 archive provides corrected high- and lower-polygon OBJ/PNG versions under Creative Commons Attribution 3.0.

Authoritative context and sources:

- [Physically Based Rendering, fourth-edition preface](https://www.pbr-book.org/4ed/Preface);
- [San Miguel 2.0 in the McGuire Computer Graphics Archive](https://casual-effects.com/g3d/data10/index.html);
- [pbrt scene collection](https://www.pbrt.org/scenes-v3).

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

- complete Bistro exterior/interior and San Miguel high/low ingestion;
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

Bistro and San Miguel remain external optional content packs.

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

Commit the small level/catalog/configuration records; keep the media external.

| Level ID | Pack | Variant | Default behavior |
| --- | --- | --- | --- |
| `Sponza` | core | fast regression | Required startup fallback and CI/smoke route. |
| `BistroExterior` | `Bistro` | complete exterior | Selectable when the external pack is present; primary hero and scale route. |
| `BistroInterior` | `Bistro` | interior with the declared complete prop set | Selectable when present; primary indirect-lighting/material route. |
| `SanMiguelHigh` | `SanMiguel` | corrected high-detail scene | Selectable when present; secondary hero/reference route. |
| `SanMiguelLow` | `SanMiguel` | matched lower-polygon scene | Selectable when present; controlled scaling and lower-end fallback experiment. |

Target pack roots remain project-owned and predictable:

- `Bistro` -> `Projects/Showcase/Assets/Meshes/Bistro`;
- `SanMiguel` -> `Projects/Showcase/Assets/Meshes/SanMiguel`.

When a pack is absent, the launcher/level picker should show it as unavailable with provenance/acquisition instructions; default build, cook, CI, and Sponza launch must remain usable. When present, discovery must not require source edits or a scene-specific executable.

## Stage 1: Inventory Before Rendering

The first deliverable for each Tier 1 scene is an importer/cooker inspection report, not a beauty shot. Build the reusable inspection path on Bistro, then run the same path on San Miguel without adding a scene-specific importer branch.

The inventory must report for Bistro exterior, Bistro interior, San Miguel high detail, and San Miguel low detail separately:

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
| Exposure/display | Freeze auto/manual exposure policy, adaptation timing, tone mapper, gamut/color space, output transfer, and door-transition behavior. |
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
| `BIS-DOOR-TRANSITION` | Indoor/outdoor exposure, streaming/residency, disocclusion, history rejection. | Continuous traversal with frame-time and residency trace. |
| `BIS-INT-WIDE` | Indirect lighting, shadowed interior, many props, material variety. | Hero still, reference comparison, convergence study. |
| `BIS-INT-CLOSE` | Wine/glass/fabric/food/wood detail and difficult material response. | Lobe/fallback explanation and specialist close-ups. |
| `BIS-INT-MOTION` | Temporal denoising/reconstruction under geometry and lighting change. | Baseline/neural side-by-side video and temporal metric plot. |
| `SMG-COURTYARD-WIDE` | Recognizable courtyard composition, sun/sky response, global visibility, and geometry scale. | Hero still, raster/path split, timing and memory summary. |
| `SMG-ARCADE` | Deep indirect light, columns/arches, shadow transitions, and convergence. | Reference comparison, convergence plot, cropped failure cases. |
| `SMG-DETAIL` | Dense props, texture/material transitions, and grazing response. | Material/debug contact sheet and close-up comparison. |
| `SMG-WALK` | Cross-scene temporal behavior, culling, residency, and neural generalization. | Deterministic traversal, frame-time trace, baseline/neural comparison. |
| `SMG-HIGH-LOW` | Controlled geometry scaling with the matched high/low versions. | Same camera/settings on both variants; CPU extraction, draw/dispatch, memory, BLAS/TLAS, traversal, and quality delta. |

At least one Bistro route must traverse rather than teleport between exterior and interior. Camera scripts for both scenes must be deterministic and usable by benchmark, screenshot, capture, and video workflows.

## Performance Contract

Performance is reported for named machines, not as an unqualified "60 FPS" claim.

The first benchmark machine becomes the `Reference System`; one materially different GPU architecture becomes the `Comparison System` when available. Exact hardware is recorded in the result, not embedded in the engine design.

Provisional gates, to be revised once the first honest baseline is captured:

| Mode | Resolution | Initial gate | Stretch gate | Notes |
| --- | ---: | ---: | ---: | --- |
| Raster/hybrid interactive | 2560×1440 | p95 GPU frame ≤ 16.67 ms | p95 GPU frame ≤ 12.5 ms | Includes full declared material/lighting envelope after warm-up. |
| Ray/path interactive | 1920×1080 | p95 GPU frame ≤ 33.33 ms | p95 GPU frame ≤ 16.67 ms | Fixed sample budget plus reconstruction/denoising. |
| Reference | 1920×1080 or higher | deterministic convergence | lower time-to-quality | Not required to be real-time. |
| Door traversal | 2560×1440 | no incorrect/missing resident asset | no hitch > 50 ms after initial load | Report worst frame and cause; do not average it away. |
| San Miguel cross-scene check | same settings as corresponding Bistro mode | correct output and complete metrics | within the declared quality/performance envelope | A different cost distribution is expected and must be explained, not normalized away. |

The benchmark protocol records:

- cold launch, cold content load, warm content load, and time to first correct frame;
- at least 300 warm frames per fixed route segment and at least three runs;
- CPU and GPU p50/p95/p99 frame times plus worst frame;
- per-pass GPU times and queue overlap;
- scene extraction, culling, frame-graph compile, command recording, submission, and present CPU times;
- draw, dispatch, pipeline, shader package, descriptor, barrier, and queue counts;
- committed and resident RAM/VRAM high-water marks;
- texture upload, eviction, mip/residency, and missing-resource events;
- BLAS/TLAS count, source geometry, build/update/compaction time, scratch/result memory, and traversal-sensitive experiments;
- compilation/cache state and pipeline creation hitches;
- image-quality setting, sample count, reconstruction mode, and dynamic-resolution state;
- exact engine/content/configuration hashes.

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
- texture cooking, upload, residency, descriptor access, and door-transition hitches;
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
- one continuous 60–90 second traversal;
- three headline measurements with hardware/configuration;
- links to the case studies, captures, code landmarks, and limitations.

The specialist path contains the detail. Do not place a wall of subsystem names in the recruiter path.

## Publication Sequence

The articles are written from completed evidence:

1. **Making Bistro Reproducible** — asset provenance, deterministic conversion, material losses, and first correct frame.
2. **One Bistro Frame, Two Explicit APIs** — workload structure, backend differences, capture-led incident, and scoped conclusions.
3. **Where Bistro Actually Spends a Frame** — CPU/GPU/memory/RT-build distributions and the top causal optimizations.
4. **Reference to Real Time** — path-traced ground truth, sampling, reconstruction, time-to-quality, and temporal failure.
5. **From Model Graph to GPU Shader** — operator math, layout/precision/fusion, numerical validation, classical fallback, and held-out generalization.
6. **What Was Deleted** — rejected optimizations, removed scaffolding, simplified boundaries, and product judgment.

Every article includes exact reproduction metadata, before/after evidence, limitations, and a short ownership statement.

## Companion Scene Decision

The engine should not accumulate flagship scenes.

| Workload | Distinct value | Decision for the next year |
| --- | --- | --- |
| Current Sponza | Small, fast, already integrated; catches startup, basic material, raster, and RT regressions. | **Keep as Tier 0.** Run frequently. Never use alone for flagship claims. |
| Modern Sponza base/add-ons | High-resolution PBR, 4K textures, alpha-card trees, emissive-light package, and multiple interchange formats. | **Optional compatibility check.** Use only if it closes a measured Bistro material/import gap; do not replace the flagship. Official sample library: [GPU Research Samples](https://www.intel.com/content/www/us/en/developer/topic-technology/graphics-research/samples.html). |
| San Miguel 2.0 | Iconic PBRT scene; different interior/courtyard visibility, dense detail, and indirect-light transport in a 523 MB corrected high/low OBJ/PNG package. | **Support as Tier 1 secondary.** It receives import, material, reference-quality, benchmark, deterministic-route, high/low scaling, gallery, and neural held-out gates, but Bistro remains the flagship story. Source: [McGuire Computer Graphics Archive](https://casual-effects.com/g3d/data10/index.html). |
| Barcelona Pavilion | Recognizable architectural scene with day/night setups; the daytime case is environment-lit and the night case sends substantial light through glass. | **Future targeted light-transport test, not current support.** Add only when transmission/specular-path correctness becomes a measured Tier 1 blocker. Source: [official pbrt scene collection](https://www.pbrt.org/scenes-v3). |
| Sun Temple | Recognizable detailed PBR environment with about 1.64 million published vertices in FBX/Falcor form. | **Do not add now.** It overlaps the current material/architecture scene set and has non-commercial share-alike terms. Source: [ORCA scene page](https://developer.nvidia.com/ue4-sun-temple). |
| Emerald Square | Approximately 10.0 million triangles and city-scale geometry; useful for AS memory, culling, and streaming beyond Bistro. | **External post-Bistro benchmark only.** Its CC BY-NC-SA 3.0 terms conflict with uncomplicated future commercial bundling. Do not ship it as product content. Source: [ORCA scene page](https://developer.nvidia.com/orca/nvidia-emerald-square). |
| Jungle Ruins | More than one trillion total triangles in a PBR environment; targets instancing, virtual geometry, and out-of-core rendering. | **Defer.** Adopt only after Bistro if virtualized geometry becomes an explicit product/research target. USD/Blender support is a separate program, not a prerequisite for this roadmap. Source: [GPU Research Samples](https://www.intel.com/content/www/us/en/developer/topic-technology/graphics-research/samples.html). |
| Moana Island Scene | 93 GB unpacked base scene, extensive instancing, and complex volumetric light transport. | **Defer beyond the current real-time engine goal.** It is valuable for USD/offline/volumetric research, but its scale, formats, and research/software-development license create a different product. Source and terms: [official dataset](https://www.disneyanimation.com/resources/moana-island-scene/). |
| ALab | Complete production USD scene with hundreds of assets, animation, fur, fabric, variants, and shot structure. | **Defer.** It is an asset-pipeline/USD adoption test, not the shortest path to the target rendering evidence. |

Scene-adoption rule:

> Add another workload only when it exposes a measured failure mode that Bistro and the fast regression set cannot expose, and when fixing that failure advances a named `PGE-*` requirement more than it delays the flagship package.

## Six-Month Workload Gates

| Deadline | Scene gate |
| --- | --- |
| End of week 2 | Provenance manifest, immutable source archive, automated inspection, import/cook baseline, and complete loss/warning inventory. |
| End of week 4 | Bistro exterior/interior load deterministically; frozen cameras; no silent missing assets; first material support matrix. San Miguel provenance plus pre/post-conversion high/low inventories are complete. |
| End of week 6 | Correct Bistro raster/hybrid baseline and high-sample references; San Miguel high/low content loads through the same pipeline with its first reference camera and controlled scaling record; material/debug contact sheet and honest unsupported lists exist. |
| End of week 9 | Deterministic routes, benchmark harness, first D3D12/Vulkan captures, bottleneck ranking, and one difficult incident log. |
| End of week 14 | Neural data/reference generation with disjoint splits, classical baseline, model card, and held-out-scene protocol. |
| End of week 19 | Real shader inference, numerical checks, latency/memory profile, Bistro quality frontier, and classical fallback. |
| End of week 24 | Three completed bottleneck studies, regression thresholds, clean acquisition/build/cook/run, and external reproduction attempt. |
| End of week 26 | Hero stills, traversal video, three specialist case studies, model-to-shader result, limitations, and reviewer-ready routing. |

## Completion Gate

Bistro is complete for the six-month portfolio only when:

- Bistro exterior/interior and San Miguel high/low content are reproducibly acquired, converted or imported, cooked, and launched;
- each Tier 1 inventory accounts for every material and texture and classifies every loss;
- the flagship cameras have deterministic high-sample references;
- raster/hybrid and ray/path modes meet their declared correctness and measured performance budgets or clearly report the remaining miss;
- D3D12 and Vulkan have comparable semantic output and an explained workload delta;
- CPU, GPU, RAM/VRAM, descriptor, residency, pipeline, and acceleration-structure behavior are measured;
- three causal bottleneck studies are complete;
- the neural feature is real runtime inference with a classical fallback and held-out generalization result;
- a clean reviewer path, source attribution, video, captures, tables, and limitations exist;
- another engineer can reproduce at least one case without private guidance.

San Miguel support is complete when its high/low acquisition/import/cook path is deterministic, all material losses are classified, its routes render correctly in raster/hybrid and reference modes, the controlled high/low performance record exists, and its high-detail gallery hero plus neural held-out result are published. It does not wait for all Bistro-specific case studies to be repeated.

A visually attractive screenshot without these conditions is a milestone, not completion.
