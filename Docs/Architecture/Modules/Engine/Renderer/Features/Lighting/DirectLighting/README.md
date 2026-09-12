# Direct Lighting Feature Dossier

**Status:** current source audit plus target completion contract; implementation, numerical, visual, performance, backend, and release proof remain open

**Responsibility:** own the bounded direct-lighting promise, current state, capability decomposition, acceptance, controlled failures, checks, and definition of done

**Authority boundary:** [Research](Research.md) owns precedent; [Discovery](Discovery.md) owns `DIR-D0`; [Sampling And Shading](SamplingAndShading.md) owns correctness rules; [Execution Architecture](ExecutionArchitecture.md) owns system shape; [User Experience](UserExperience.md) owns human and automation behavior; [Plan](Plan.md) owns delivery order; code/build owns implementation; `FCR-REN-06` owns candidate results

**Verified:** 2026-09-12 against revision `8b650c7450f8a59fb3bcc18edbb4d217a7b11ed5`; the user reports the current image is visually broken, but this documentation pass did not run the renderer or localize one runtime defect

**Scope:** scalable direct illumination, analytic and emissive light selection, PBR surface evaluation, visibility, temporal/spatial reuse, denoising, diagnostics, and adoption

**Current readiness:** **45/100** per [Current Feature Readiness](../../../../../../../Acceptance/CurrentReadiness.md#renderer); source-present does not mean correct or release-ready

## Outcome

Sparkle needs one reliable direct-lighting feature that is analytically checkable at small light counts and scales to dense overlapping lights without changing the authored light contract. “MegaLights-like” is a product goal—many dynamic shadowed lights at bounded per-pixel work—not a request to copy Unreal internals. RTXDI and ReSTIR are algorithmic precedents, not automatic proof that Sparkle's current reservoir is correct.

The delivery order is correctness first, controlled stochastic scaling second, denoising third, and quality/performance tuning last. A plausible image, a stable temporal image, or an SDK-derived implementation cannot replace an independent oracle.

## Start Here

| Need | Owner |
| --- | --- |
| unresolved decisions, experiments, and admission gate | [Discovery](Discovery.md) |
| primary papers, SDK/engine precedent, and alternative comparison | [Research](Research.md) |
| radiometry, BRDF, sampling, reservoir, and visibility equations | [Sampling And Shading](SamplingAndShading.md) |
| target owner, resources, passes, lifetime, and integration hooks | [Execution Architecture](ExecutionArchitecture.md) |
| first use, quality selection, diagnostics, failure, and automation | [User Experience](UserExperience.md) |
| staged production work, gates, deletion ledger, and copy-ready prompts | [Plan](Plan.md) |
| independent surface-transport oracle | [Reference Path Tracer](../ReferencePathTracer/README.md) |

## Current Source Truth

The ordinary Lit recipe currently provides:

- directional, point, spot, and rectangular lights with inspected capacities `2/1024/1024/1024`;
- physical-looking author fields—directional illuminance in lux, point/spot luminous intensity in candela, and rectangular luminance in candela per square metre—without executed unit-conformance proof;
- four initial uniform candidates per pixel, one temporal reuse pass, four fixed spatial neighbors, and a scalar luminance target;
- a selected-light visibility signal through Inline ray queries or the Pipeline ray-tracing frontend;
- separate `DirectDiffuse`, `DirectSpecular`, and `DirectSubsurface` scene-linear textures, later joined by `LightingComposite`;
- no shadow-map/non-ray fallback, emissive-mesh light inventory, environment-direct candidate, ReGIR structure, dedicated direct denoiser, or accepted numerical/visual/performance result.

The reservoir stores light type/index, a shape sample, weight sum, target, effective sample count, and validity. Temporal compatibility currently uses reprojected motion plus packed normal and view distance. The inspected path does not expose previous-to-current light-index translation, a selectable bias-correction mode, conservative visibility-reuse state, material/roughness/object identity, denoiser confidence, or disocclusion-specific candidate policy. The whole lighting-scene hash can reset history after broad mutations; it does not establish mathematically valid reuse for the histories that survive.

The active direct BRDF is Cook–Torrance/GGX/Smith/Schlick plus Burley diffuse and a wrap-subsurface approximation. The implementation must be tested for lobe energy allocation—especially simultaneous diffuse and subsurface—rather than assumed correct from familiar function names.

## Capability Decomposition

| ID | Feature | First dependable contract | Later scale/quality path | Current state |
| --- | --- | --- | --- | --- |
| `DIR-FS-01` | light semantics | finite validated directional/point/spot/rect records with frozen units, range, cone, shape, and sidedness | photometric profiles and textured emitters only after asset/product admission | source-present, unproved |
| `DIR-FS-02` | surface response | energy-accounted diffuse/specular/subsurface evaluation with matching evaluation and sampling contracts | admitted OpenPBR-like lobes without altering existing semantics accidentally | source-present, unproved |
| `DIR-FS-03` | deterministic baseline | exhaustive small-light evaluation and one visibility query per contributing sample | retained as test oracle, not a shipping many-light path | absent |
| `DIR-FS-04` | initial light sampling | uniform and power-weighted distributions with exact PMF accounting | environment, emissive triangles, and optional world-space guiding | partial |
| `DIR-FS-05` | reservoir reuse | canonical ReSTIR DI reservoir, explicit target, `M`, normalization, light identity, and bias mode | fused passes, pairwise/advanced reuse only after evidence | partial, conformance unknown |
| `DIR-FS-06` | visibility and shadows | finite segment semantics, alpha/two-sided agreement, robust ray offsets, strict provider status | opacity micromaps or non-ray provider only after independent admission | partial, ray-only |
| `DIR-FS-07` | stochastic reconstruction | raw noisy signal, confidence/history contract, and classical vendor-neutral baseline | optional DLSS Ray Reconstruction behind the same semantic inputs | absent/optional only |
| `DIR-FS-08` | many-light product | bounded work with explicit degradation under overlap and reported active quality | ReGIR only if distributed-scene evidence beats simpler PDFs | absent |
| `DIR-FS-09` | diagnostics and adoption | raw lobe/sample/visibility/reservoir captures through existing generic routes | focused editor controls after developer workflow is proved | partial |

## Target Quality Tiers

| Tier | Purpose | Required result |
| --- | --- | --- |
| `DIR-Q0` analytic | falsify units, geometry terms, BRDF, PDFs, visibility, and capacity behavior | deterministic CPU/closed-form cases and exhaustive GPU small-light agreement |
| `DIR-Q1` reference | establish unbiased/high-sample scene-linear comparison | accepted Reference Path Tracer direct AOV or an independently generated reference with complete manifest |
| `DIR-Q2` interactive correctness | establish ReSTIR normalization, temporal safety, frontend parity, and finite output | raw-lobe statistical/metamorphic checks before denoising |
| `DIR-Q3` reconstructed quality | make the sparse signal temporally useful without hiding invalid history | motion/disocclusion/animated-light sequences with raw and reconstructed evidence |
| `DIR-Q4` product scale | demonstrate many-light usefulness inside frozen quality, memory, and time budgets | Sponza, Bistro, San Miguel, and a synthetic overlap stress scene on both supported backends |

## Acceptance Criteria

- `AC-DIR-01` — four analytic light families satisfy frozen unit, attenuation, cone, shape-measure, range, and finite-boundary cases against predeclared analytic values.
- `AC-DIR-02` — evaluation and sampling of each admitted BRDF lobe agree on direction convention, support, PDF measure, delta classification, and energy allocation; diffuse/subsurface cannot double-spend the same energy.
- `AC-DIR-03` — exhaustive small-light GPU output agrees with the analytic/reference baseline within frozen absolute/relative/statistical tolerances before reservoir reuse is enabled.
- `AC-DIR-04` — initial, temporal, and spatial reservoir stages pass normalization, `M` accounting, target-density, selected-sample replay, previous-light translation, and bias-mode tests.
- `AC-DIR-05` — camera/light/geometry/material/alpha/extent/provider/shader mutations invalidate or translate exactly the affected history; disocclusions cannot reuse incompatible samples.
- `AC-DIR-06` — Inline and Pipeline visibility agree for miss, opaque, alpha-mask, double-sided, finite segment, grazing, self-intersection, and area-light cases; strict unavailability is explicit.
- `AC-DIR-07` — raw noisy lobes remain separately observable; the denoiser consumes declared motion/depth/normal/roughness/confidence identities and never turns non-finite or stale input into a pass.
- `AC-DIR-08` — analytic, emissive, and environment candidate classes admitted by discovery use one light identity and exact selection/source PDFs; unsupported classes are rejected rather than sampled as black.
- `AC-DIR-09` — quality degrades predictably as overlapping important lights exceed the sample budget; the active budget and resolved algorithm are visible and no “unlimited lights” claim is made.
- `AC-DIR-10` — D3D12 and Vulkan pass the same raw-lobe, visibility, temporal, failure, and workload matrix with native validation; quality, time, and memory are reported separately.
- `AC-DIR-11` — the feature is enclosed in one predictable owner and all external edits appear in the integration-hook ledger with a defect-detecting check.
- `AC-DIR-12` — `FCR-REN-06` records the exact candidate, configuration, references, thresholds, artifacts, and `PASS/BLOCKED/EXCLUDED/SUPERSEDED` verdicts; documentation completion cannot pass it.

## Controlled Failure Modes

| ID | Failure | Safe response | Detecting check |
| --- | --- | --- | --- |
| `FM-DIR-01` | invalid/non-finite light, degenerate rect basis, cone, range, or material | reject before publication or produce the frozen finite boundary value | `CHK-DIR-01` |
| `FM-DIR-02` | light add/remove/reorder makes a retained index refer to another light | translate stable identity or invalidate; never shade the wrong light | `CHK-DIR-03` |
| `FM-DIR-03` | receiver or occluder changes while a reservoir/visibility sample survives | reject affected reuse and lower confidence | `CHK-DIR-03`, `CHK-DIR-04` |
| `FM-DIR-04` | zero/near-zero target, extreme weight, large `M`, or long-running frame identity | remain finite with bounded `M`; surface a failed invariant | `CHK-DIR-02`, `CHK-DIR-05` |
| `FM-DIR-05` | strict traversal/program/SBT/material table unavailable | fail selection before dispatch and name the missing capability | `CHK-DIR-04` |
| `FM-DIR-06` | denoiser input identity or guide is missing/stale | bypass or reject according to frozen policy; do not silently reuse | `CHK-DIR-06` |
| `FM-DIR-07` | too many overlapping high-energy lights for the ray budget | bounded noise/blur response and visible quality status, not silent light loss | `CHK-DIR-07` |
| `FM-DIR-08` | backend/compiler change alters raw semantics | hold only the affected evidence cell and retain artifacts | `CHK-DIR-08` |

## Required Checks

| Check | Cheapest claim-falsifying exercise | Main coverage |
| --- | --- | --- |
| `CHK-DIR-01` | CPU/closed-form light and BRDF hand cases plus shader-kernel equivalent inputs | `AC-DIR-01/02`; `FM-DIR-01` |
| `CHK-DIR-02` | deterministic reservoir stream tests covering selection frequency, normalization, clamped `M`, zeros, and extremes | `AC-DIR-04`; `FM-DIR-04` |
| `CHK-DIR-03` | scripted temporal mutation matrix with sample identity and raw-lobe capture | `AC-DIR-04/05`; `FM-DIR-02/03` |
| `CHK-DIR-04` | paired Inline/Pipeline visibility fixture and injected capability faults | `AC-DIR-06`; `FM-DIR-03/05` |
| `CHK-DIR-05` | long finite/stability sequence with counters and reservoir decode | `AC-DIR-04/05`; `FM-DIR-04` |
| `CHK-DIR-06` | raw-versus-reconstructed motion, disocclusion, light-toggle, and cut sequences | `AC-DIR-07`; `FM-DIR-06` |
| `CHK-DIR-07` | fixed-camera overlap sweep from one to thousands of important lights at frozen budgets | `AC-DIR-08/09`; `FM-DIR-07` |
| `CHK-DIR-08` | D3D12/Vulkan candidate matrix with native validation and raw artifacts | `AC-DIR-10/12`; `FM-DIR-08` |
| `CHK-DIR-09` | enclosure/build/link/stale-path audit | `AC-DIR-11` |

## Definition Of Done

Direct Lighting is done only when `DIR-D0` is accepted, every included `DIR-FS-*` maps to passing `AC-DIR-*`/`CHK-DIR-*`, controlled failures reach their safe states, the clean-break/enclosure and adoption contracts pass, supported D3D12/Vulkan quality-time-memory cells pass, and `FCR-REN-06` records the exact candidate verdict. RTXDI, MegaLights, NRD, FidelityFX, the current source path, and a future Reference Path Tracer are dependencies or precedents—not inherited evidence. First-release work remains subject to the repository release gate; unadmitted expansions such as photometric profiles, a shadow-map fallback, or emissive animation do not enter production merely because they are researched here.

## Primary Source Routes

- [`RestirDirectLighting.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Lighting/Restir/RestirDirectLighting.cpp)
- [`DirectLightReservoir.hlsli`](../../../../../../../../Engine/Assets/Shaders/Lighting/DirectLightReservoir.hlsli)
- [`DirectLightSampling.hlsli`](../../../../../../../../Engine/Assets/Shaders/Lighting/DirectLightSampling.hlsli)
- [`DirectShadowSignal.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Lighting/Shadows/DirectShadowSignal.cpp)
- [`SurfaceLighting.hlsli`](../../../../../../../../Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli) and [`BRDF.hlsli`](../../../../../../../../Engine/Assets/Shaders/BRDF/BRDF.hlsli)
- [`RenderGpuLightingPayloadBuilder.cpp`](../../../../../../../../Engine/Renderer/Private/Scene/GpuScene/RenderGpuLightingPayloadBuilder.cpp)
