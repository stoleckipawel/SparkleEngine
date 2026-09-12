# Indirect Lighting And ReSTIR GI Feature Dossier

**Status:** current source audit plus target completion contract; implementation, estimator-conformance, convergence, visual, performance, backend, and release proof remain open

**Responsibility:** own the bounded indirect-lighting/ReSTIR GI promise, current prototype classification, feature scope, acceptance, failures, checks, and definition of done

**Authority boundary:** [Research](Research.md) owns precedent; [Discovery](Discovery.md) owns `IND-D0`; [Transport And Estimator](TransportAndEstimator.md) owns math; [Execution Architecture](ExecutionArchitecture.md) owns system shape; [User Experience](UserExperience.md) owns human and automation behavior; [Plan](Plan.md) owns order; code/build and `FCR-REN-07` own implementation/results

**Verified:** 2026-09-12 against revision `8b650c7450f8a59fb3bcc18edbb4d217a7b11ed5`; the existing route is source-present, not established as ReSTIR GI/GRIS-correct

**Scope:** bounced surface transport, ReSTIR GI/path resampling, environment illumination, temporal/spatial reuse, reconstruction, diagnostics, and adoption

**Current readiness:** **45/100** per [Current Feature Readiness](../../../../../../../Acceptance/CurrentReadiness.md#renderer)

## Outcome

Sparkle needs a correct real-time indirect-lighting feature whose first-bounce and multi-bounce claims are mathematically explicit, compare against an independent reference, and remain useful under camera/scene motion. ReSTIR is the selected research family, not a license to call any reservoir of random seeds “ReSTIR GI.”

The delivery path is:

1. a deterministic/statistical one-bounce baseline and accepted independent reference;
2. a real path-sample representation with exact proposal, shift mapping, Jacobian, target, and normalization;
3. temporal/spatial reuse that cannot replay a neighbor's RNG as though it were a transported path;
4. reconstruction with raw-signal access;
5. measured multi-bounce and glossy/specular expansion using ReSTIR GI or ReSTIR PT according to the frozen domain.

## Start Here

| Need | Owner |
| --- | --- |
| estimator/domain/oracle decisions and gate | [Discovery](Discovery.md) |
| primary research and alternative GI architecture comparison | [Research](Research.md) |
| path, proposal, shift, reservoir, reuse, and reconstruction math | [Transport And Estimator](TransportAndEstimator.md) |
| owner, data flow, histories, resources, and integration hooks | [Execution Architecture](ExecutionArchitecture.md) |
| first use, path-domain disclosure, diagnostics, failure, and automation | [User Experience](UserExperience.md) |
| clean-break staged delivery and copy-ready prompts | [Plan](Plan.md) |
| independent transport oracle and its limitations | [Reference Path Tracer](../ReferencePathTracer/README.md) |

## Current Source Truth

The current route clears/seeds an indirect reservoir, performs temporal and spatial reuse, resolves a path through inline ray queries, and writes `IndirectDiffuse` and `IndirectSpecular`. Bounce count is clamped to eight. Sky data is available to misses and is filled separately as the background.

The inspected reservoir sample is only:

```text
RandomPixel + SampleIndex + RandomFrameIndex
```

It does not store a path vertex/sample record, source receiver, source proposal density, connection/shift mapping, Jacobian, visibility terms, or generalized MIS metadata. Reuse replays that random identity through the path tracer at the current surface and weights it from scalar target/`M` state. This may be an experimental seed-reuse technique, but this audit found no proof that it implements Ouyang et al. ReSTIR GI, GRIS, or RTXDI ReSTIR PT. Its behavior may be biased or otherwise incorrect; only execution against independent oracles can decide.

The sample payload serializes integer pixel/sample/frame fields through `float4`. Frame indices beyond exact binary32 integer range are therefore a specific long-run identity risk. Current compatibility again uses only packed normal and view distance. There is no dedicated vendor-neutral indirect denoiser, accepted convergence/error study, or Pipeline traversal path for indirect transport.

## Feature Decomposition

| ID | Feature | First dependable contract | Advanced path | Current state |
| --- | --- | --- | --- | --- |
| `IND-FS-01` | independent baseline | one-bounce diffuse/glossy path with explicit NEE/MIS and statistical tests | multi-bounce reference AOV after RPT acceptance | absent |
| `IND-FS-02` | environment illumination | one environment mapping, emission, importance PDF, miss/background separation | atmospheric sky provides environment generation only through an admitted bridge | partial/unproved |
| `IND-FS-03` | initial path samples | actual secondary vertex/path record with source probability and contribution | multiple candidate strategies/ReGIR/path guiding | absent |
| `IND-FS-04` | shift mapping | documented reconnection/replay mapping, inverse/Jacobian/support tests | hybrid mappings selected by roughness/path structure | absent |
| `IND-FS-05` | reservoir estimator | GRIS/ReSTIR GI normalization and effective sample accounting | ReSTIR PT for glossy/longer paths | prototype only |
| `IND-FS-06` | temporal/spatial reuse | full receiver/path/light/material identity, disocclusion handling, explicit bias mode | reciprocal/multilayer/large-kernel reuse after evidence | partial, conformance unknown |
| `IND-FS-07` | transport robustness | self-intersection, alpha/two-sided, roulette, finite throughput, bounded path depth | transmission/caustics after material/path admission | partial/unproved |
| `IND-FS-08` | reconstruction | portable diffuse/specular baseline using declared guides/confidence | optional DLSS RR | absent/optional only |
| `IND-FS-09` | quality/performance | workload-specific sample/ray/path/memory budgets and graceful failure | path guiding/cache integration only if measured | absent |
| `IND-FS-10` | diagnostics/adoption | raw indirect lobes, path/reservoir identity, requested/active status | no separate dashboard | partial |

## Product Domains

| Domain | Required first claim | Not included by implication |
| --- | --- | --- |
| diffuse indirect | one and admitted multi-bounce surface transport | probes, baked lightmaps, AO, or volume scattering |
| glossy indirect | rough-specular paths inside frozen roughness/support | perfect mirrors, refraction, caustics, coat, anisotropy |
| emissive hits | emission encountered by a transported path | complete emissive-light NEE inventory |
| environment | miss emission and admitted direct/secondary importance sampling | physical atmosphere or aerial perspective |
| reconstruction | stable approximation of the declared raw estimator | estimator correctness or convergence proof |

Atmospheric sky and fog live in [Volumetric Lighting](../VolumetricLighting/README.md). That feature may publish an environment generation for surface transport, but the same sky radiance cannot be counted again as background, atmosphere, and path contribution without the composition contract.

## Quality Ladder

| Tier | Oracle and purpose |
| --- | --- |
| `IND-Q0` analytic/metamorphic | white furnace, diffuse energy, environment rotation, occluder insertion, light scaling, path-depth boundaries |
| `IND-Q1` independent reference | accepted RPT or external manifest-pinned reference with raw direct/indirect AOVs and uncertainty |
| `IND-Q2` initial estimator | one-bounce path samples agree statistically without reuse/reconstruction |
| `IND-Q3` reservoir correctness | shift/GRIS normalization and reuse pass static/mutation tests |
| `IND-Q4` reconstructed product | motion/disocclusion/lighting/material sequences retain detail without stale energy |
| `IND-Q5` workload product | Cornell/Sponza/Bistro/San Miguel quality-time-memory cells on supported backends |

## Acceptance Criteria

- `AC-IND-01` — the admitted transport equation, path domain, light/environment sampling, BRDF sampling, MIS, roulette, and emission accounting map to exact code and independent hand/statistical cases.
- `AC-IND-02` — initial one-bounce diffuse and glossy estimates are finite and statistically agree with analytic or accepted independent references before reservoir reuse and denoising.
- `AC-IND-03` — the reservoir stores or reconstructs every path/sample fact required by its selected shift mapping; integer identity and counts are not rounded through binary32.
- `AC-IND-04` — reconnection/replay/hybrid shifts pass support, inverse, Jacobian, roughness, visibility, and target-density tests; invalid shifts contribute no fabricated sample mass.
- `AC-IND-05` — temporal/spatial reuse passes the ratified GRIS/ReSTIR normalization and effective-`M` contract, including correlation/bias classification and disocclusion behavior.
- `AC-IND-06` — camera, motion, surface, material, normal, roughness, object, geometry, light, sky, extent, view, shader, traversal, and algorithm mutations invalidate exactly the required histories with no cross-view reuse.
- `AC-IND-07` — direct NEE, emissive hits, environment misses, and indirect bounces are mutually accounted; adding path depth cannot deterministically double an earlier contribution.
- `AC-IND-08` — raw diffuse/specular outputs and path/reservoir/confidence evidence remain observable before reconstruction; the denoiser has an explicit portable baseline and optional-provider failure contract.
- `AC-IND-09` — the supported glossy/roughness/path-depth domain is explicit; unsupported delta/transmission/caustic cases report exclusion rather than plausible but mislabeled output.
- `AC-IND-10` — quality, ray/path count, temporal lag, peak/history memory, and GPU time are recorded for frozen workloads and profiles; source-paper results do not fill the table.
- `AC-IND-11` — D3D12 and Vulkan supported cells pass identical raw/statistical/temporal/failure semantics with native validation.
- `AC-IND-12` — one feature capsule owns mechanism/history; every external hook has a reason and defect-detecting check; the seed-replay prototype is deleted in the replacement stage.
- `AC-IND-13` — `FCR-REN-07` owns the candidate verdict with complete identity, references, thresholds, artifacts, and explicit blocked/excluded cells.

## Controlled Failure Modes

| ID | Failure | Required response | Check |
| --- | --- | --- | --- |
| `FM-IND-01` | random seed is replayed at a different receiver without a valid path shift/PDF | reject conformance; no ReSTIR GI claim | `CHK-IND-02` |
| `FM-IND-02` | shift leaves BSDF/geometry/light support or has invalid Jacobian | reject candidate before reservoir update | `CHK-IND-02` |
| `FM-IND-03` | NEE/emissive/environment contribution is counted twice | fail analytic/metamorphic energy test | `CHK-IND-01` |
| `FM-IND-04` | history survives cut/disocclusion/material/sky/provider/shader change | reject/reset affected reservoir and reconstruction state | `CHK-IND-03` |
| `FM-IND-05` | NaN/Inf throughput, weight, target, PDF, or radiance | invalidate affected sample/history and surface invariant failure | `CHK-IND-04` |
| `FM-IND-06` | long frame/sample identity loses precision | integer round-trip check fails before product claim | `CHK-IND-04` |
| `FM-IND-07` | unsupported glossy/delta/transmission path is silently treated as diffuse | explicit excluded/degraded status | `CHK-IND-01`, `CHK-IND-05` |
| `FM-IND-08` | reconstruction hides lag, leaks, bias, or missing paths | raw-versus-reconstructed cell fails | `CHK-IND-05` |
| `FM-IND-09` | reference shares the same questioned code or is not accepted | mark comparison dependent/inconclusive | `CHK-IND-06` |
| `FM-IND-10` | backend/provider produces different path semantics | hold affected evidence cell | `CHK-IND-07` |

## Required Checks

| Check | Exercise |
| --- | --- |
| `CHK-IND-01` | CPU/analytic/metamorphic transport cases: furnace, diffuse box, glossy plane, environment rotation/scaling, occluder/emitter insertion, depth/roulette/emission accounting |
| `CHK-IND-02` | deterministic reservoir and shift suite with known proposals, inverse/Jacobian/support, duplicated candidates, zero targets, bounded `M`, and statistical frequencies |
| `CHK-IND-03` | temporal mutation matrix over every history identity, dual views, disocclusion, cut, reload, resize, and first-frame restart |
| `CHK-IND-04` | long-run integer identity, finite throughput/weights, extreme PDFs, path termination, cancellation, and generation-retirement stress |
| `CHK-IND-05` | raw/reconstructed diffuse/specular motion and quality sequences across the admitted roughness/path-depth domain |
| `CHK-IND-06` | manifest-pinned independent-reference comparisons with shared-code analysis and predeclared statistical/image metrics |
| `CHK-IND-07` | D3D12/Vulkan native-validation workload matrix with exact candidate, ray/path, memory, time, and artifact identity |
| `CHK-IND-08` | feature enclosure, integration-hook, build/package, generated surface, docs/link, and stale prototype audit |

## Definition Of Done

Indirect Lighting is done only when `IND-D0` is accepted, the seed-replay path is removed, every included `IND-FS-*` maps to passing `AC-IND-*`/`CHK-IND-*`, controlled failures reach their safe state, supported path/backend/quality-time-memory cells pass, and `FCR-REN-07` records the exact candidate. This dossier does not raise the current 45/100 readiness. ReSTIR GI papers, RTXDI, Lumen, ReSTIR PT, or a visually pleasing denoised image provide precedent—not Sparkle acceptance.

## Primary Source Routes

- [`RestirIndirectReservoir.hlsli`](../../../../../../../../Engine/Assets/Shaders/Lighting/RestirIndirectReservoir.hlsli)
- [`RestirIndirectTemporal.hlsl`](../../../../../../../../Engine/Assets/Shaders/Passes/RayTracing/RestirIndirectTemporal.hlsl), [`RestirIndirectSpatial.hlsl`](../../../../../../../../Engine/Assets/Shaders/Passes/RayTracing/RestirIndirectSpatial.hlsl), and [`RestirIndirectResolve.hlsl`](../../../../../../../../Engine/Assets/Shaders/Passes/RayTracing/RestirIndirectResolve.hlsl)
- [`PathLighting.hlsli`](../../../../../../../../Engine/Assets/Shaders/RayTracing/PathLighting.hlsli) and [`PathSampling.hlsli`](../../../../../../../../Engine/Assets/Shaders/RayTracing/PathSampling.hlsli)
- [`RestirIndirectLighting.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Lighting/Restir/RestirIndirectLighting.cpp)
- [`Sky.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Lighting/Sky/Sky.cpp) and [`LightingComposite.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Lighting/LightingComposite.cpp)
