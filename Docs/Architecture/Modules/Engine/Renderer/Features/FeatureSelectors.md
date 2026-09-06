# Renderer Feature Selector Catalog

Status: capability snapshot; current selector catalog, source-backed but not proof that every valid/invalid value activates, persists, recovers, or ships correctly

Verified: 2026-09-06 against committed `master` revision `d236da11`; `Engine/Renderer` and the routed RHI controls are unchanged from the earlier `8414b5dc` source audit

Scope: every Renderer feature CVar discovered in current Renderer source plus RHI presentation/adapter/TLAS controls consumed by the Renderer settings surface; maps request, persistence, active consumer, restart/topology effect, and feature dossier

## Selection Contract

A registered name is not automatically a feature. A trustworthy selector has a producer (console/config/editor/API), parser and valid domain, owned runtime consumer, requested-versus-active result, topology/history/invalidation behavior, visible failure/fallback, persistence/restart contract, and release classification. `REN-E23` keeps the selector inventory open until each row has that closure.

## Persisted Renderer Settings Section

`EngineRenderingSettingsPersistence` owns 27 names under `/Script/SparkleRenderer.EngineRenderingSettings` in workspace `Config/DefaultEngine.ini`. The editor settings section captures/applies the same state. View mode is exposed in the settings state but deliberately not persisted by this file.

| Feature | Persisted selectors | Default/request boundary | Active owner and effect |
| --- | --- | --- | --- |
| Presentation/device | `r.VSync`, `r.BackBufferFormat`, `r.PreferHighPerformanceAdapter` | VSync on; backend default format; high-performance preference on | RHI present/device selection. Format and adapter preference are reported pending restart; VSync is live. |
| Tone mapping/output | `r.ToneMapper`, `r.OutputColorEncoding` | ACES approximation; Automatic encoding | resolved view, [Tone Mapping](PostProcessing/ToneMapping.md), and [Presentation/Output](PostProcessing/PresentationAndOutput.md) passes; graph parameters update per frame |
| Exposure | `r.Exposure.Mode`, `r.Exposure.MeteringMethod`, `r.Exposure.Manual`, `r.Exposure.Compensation`, `r.Exposure.TargetLuminance`, `r.Exposure.Min`, `r.Exposure.Max`, `r.Exposure.AdaptationSpeedUp`, `r.Exposure.AdaptationSpeedDown` | Automatic, ParallelReduction, 1.0, 0 EV, 0.18, 0.000001, 65536, 3 EV/s, 1 EV/s | view display resolution and [Exposure](PostProcessing/Exposure.md) pass/history |
| Upscaling | `r.Upscaler.Provider`, `r.Upscaler.QualityMode` | Linear, NativeAA | [reconstruction/upscaling](PostProcessing/ImageReconstructionAndUpscaling.md) provider stack and graph key; external provider may resolve back to Linear |
| Ray reconstruction | `r.RayReconstruction.Mode` | Off | [reconstruction/upscaling](PostProcessing/ImageReconstructionAndUpscaling.md) provider stack; ReSTIR only; unavailable path resolves Off |
| GBuffer | `r.GBuffer.Algorithm`, `r.GBuffer.RayTracingExecution` | Rasterized, Automatic | graph topology plus ray-GBuffer execution plan |
| Lighting | `r.Lighting.Mode` | ReSTIR path-traced | graph topology; ReSTIR or reference branch |
| Mesh work | `r.MeshAutoBatching` | on | per-view raster batch construction |
| Classic TLAS | `r.RayTracing.Tlas.Refit` | on | classic TLAS strategy after initial build |
| PTLAS | `r.RayTracing.PreferPartitionedTlas`, `r.RayTracing.Ptlas.PartitionsPerAxis`, `r.RayTracing.Ptlas.PartitionUpdateMode`, `r.RayTracing.Ptlas.MarkAllDynamicInPartition`, `r.RayTracing.Ptlas.ModeChangeDistance` | off, 8, AlwaysUpdatePartition, false, 100 | RHI capability preference plus per-view partition planner; actual current execution remains the narrow one-operation/no-update/no-translation strategy |

The settings writer replaces only its owned INI section. Loading silently ignores names outside its allowlist and currently discards `TrySetValueFromString` error text; malformed persisted-value diagnostics remain a gap.

## Session And Developer Selectors

| Selector | Default/domain | Current consumer and effect | Persistence/reachability boundary | Dossier |
| --- | --- | --- | --- | --- |
| `r.ViewMode` | Lit plus 15 debug modes | view uniform/debug pass; Wireframe also alters raster fill | session state; editor UI exposes it; not in the 27-name persisted set | [Debug Views](DebugViews/README.md) |
| `r.RayTracing.Shadows.Execution` | Automatic, Inline, Pipeline | direct-shadow execution plan and graph topology | console surface; not mirrored by `EngineRenderingSettingsState` | [Direct Lighting](Lighting/DirectLighting.md) and [Ray Tracing](RayTracing/README.md) |
| `r.RayTracedShadows.NormalBias` | 0.01 world units | shadow ray input | console only; inspected frame binding does not clamp it | [Direct Lighting](Lighting/DirectLighting.md) |
| `r.RayTracedShadows.MaxDistance` | 100000 world units | directional shadow ray maximum | console only; inspected frame binding does not clamp it | [Direct Lighting](Lighting/DirectLighting.md) |
| `r.RayTracing.Restir.Indirect.Bounces` | 2 requested; active clamp 1..8 | ReSTIR indirect candidate path | console only; requested value can differ from active clamped value | [Indirect Lighting](Lighting/IndirectLighting.md) |
| `r.RayTracing.Restir.Indirect.NormalBias` | 0.01 requested; active minimum 0 | ReSTIR indirect ray spawn | console only | [Indirect Lighting](Lighting/IndirectLighting.md) |
| `r.RayTracing.Restir.Indirect.MaxDistance` | 100000 requested; active minimum 0.001 | ReSTIR indirect traversal | console only | [Indirect Lighting](Lighting/IndirectLighting.md) |
| `r.RayTracing.PathTracedLighting.SamplesPerPixel` | 64 requested; active clamp 1..4096 | reference accumulation sample count | console only; high values are not performance-approved | [Indirect Lighting](Lighting/IndirectLighting.md) |
| `r.RayTracing.PathTracedLighting.Bounces` | 8 requested; active clamp 1..16 | reference secondary bounce count | console only | [Indirect Lighting](Lighting/IndirectLighting.md) |
| `r.RayTracing.PathTracedLighting.NormalBias` | 0.01 requested; active minimum 0 | reference ray spawn | console only | [Indirect Lighting](Lighting/IndirectLighting.md) |
| `r.RayTracing.PathTracedLighting.MaxDistance` | 100000 requested; active minimum 0.001 | reference traversal | console only | [Indirect Lighting](Lighting/IndirectLighting.md) |
| `r.Diagnostics.MarkerVerbosity` | FramePass; Off/FramePass/Detailed | frame/pass/detailed GPU marker emission | developer console; package exposure must be classified | [Diagnostics](DiagnosticsProductsAndCapture.md) |
| `r.Diagnostics.GpuTiming` | off | timestamp collection/resolution | developer console; observer cost unmeasured | [Diagnostics](DiagnosticsProductsAndCapture.md) |
| `r.FrameGraph.ParallelRecording` | on | Tasks-backed recording chunks | developer console; equivalence/scaling unproved | [Frame Graph](FrameGraphAndScheduling.md) |
| `r.Material.BindingMode` | RayTracingOnly; also registers Everything | no runtime consumer found; capability report supports RayTracingOnly only | ineffective registered selector; `Everything` is not a feature | [Geometry/Materials](GeometryMaterialsAndGBuffer.md#material-contract) |

## RHI Console Controls Outside The Renderer Settings State

| Selector | Current contract | Boundary |
| --- | --- | --- |
| `r.BackBufferCount` | supported values 2 or 3; renderer/device recreation required | registered/owned by RHI; absent from `EngineRenderingSettingsState` and its persisted 27-name section |
| `r.MaximumFramesInFlight` | supported values 1..3 and no greater than back-buffer count; recreation required | registered/owned by RHI; absent from Renderer settings UI/persistence |

These controls affect frame storage and presentation but are not proof that every combination is exposed or stable. Their current lack of Renderer settings integration must not be described as “back-buffer/frame-count settings support.”

## Deliberately Absent Feature Selectors

| Capability | Current selector state | Documentation owner |
| --- | --- | --- |
| Volumetric lighting, fog, atmosphere, aerial perspective | no Renderer/RHI setting, CVar, viewport request, or editor control was found | [Volumetric Lighting](Lighting/VolumetricLighting.md); `REN-E24` audits continued absence |
| Deferred decals | no authored/editor/runtime selector was found because no current decal feature exists | [Deferred Decals](DeferredDecals/README.md); `REN-E25` prevents the target design from becoming an implied feature |
| Color grading | no grading parameter, LUT, transform, or editor selector exists | [Color Grading](PostProcessing/ColorGrading.md); `REN-E26` audits continued absence |
| Chromatic aberration | no lens/channel effect or viewport selector exists | [Chromatic Aberration](PostProcessing/ChromaticAberration.md); `REN-E27` audits continued absence |
| Frame generation | no provider, quality, pacing, or presentation selector exists; Reflex/PCL controls are not frame synthesis | [Frame Generation](PostProcessing/FrameGeneration.md); `REN-E28` audits continued absence |
| HDR display output | no PQ/scRGB/HDR10/display-nit/swapchain-HDR selection exists | [Presentation and Output](PostProcessing/PresentationAndOutput.md) |
| Non-ray lighting fallback | no shadow-map/lightmap/probe-only/deferred-raster lighting mode exists | [Lighting](Lighting/README.md) |

An absent selector is not a UI omission when the feature itself is absent. Adding a selector before its producer, active-state reporting, failure behavior, and completion contract exist would create misleading public vocabulary.

## Change And Review Rule

When adding or changing a selector:

1. update this catalog and the owning feature dossier/capability ID;
2. verify the producer, parser/domain, runtime consumer, requested/active diagnostic, topology/history/restart effect, and Shipping reachability;
3. add or update the smallest `REN-E23` case and the feature-specific evidence item;
4. remove unused names/values rather than retaining misleading compatibility vocabulary under the current clean-break policy.

## Acceptance Criteria

- `AC-SEL-01` — every Renderer-owned registered CVar and every RHI control surfaced by Renderer settings has exactly one catalog row with parser/domain, default, producer, active consumer or explicit no-consumer state, persistence, restart/topology/history effect, and dossier.
- `AC-SEL-02` — every enum value and numeric boundary is accepted, clamped, rejected, or marked vocabulary-only exactly as documented; malformed values produce actionable diagnostics rather than disappearing silently.
- `AC-SEL-03` — requested and active state remain separately inspectable for capability-gated traversal/provider/PTLAS choices, including fallback/refusal reason.
- `AC-SEL-04` — the 27-name owned INI section round-trips valid settings without modifying other sections; non-persisted/session/RHI-only controls remain absent by design.
- `AC-SEL-05` — live changes apply on the next permitted frame, topology/history changes rebuild/reset their owners, and restart-required changes do not claim live activation.
- `AC-SEL-06` — Editor, Runtime, Debug/Development/Shipping, workspace/package, D3D12/Vulkan reachability is independently classified; hidden console reachability still counts unless erased/locked.
- `AC-SEL-07` — absent feature selectors remain absent and ineffective `r.Material.BindingMode` vocabulary is removed or visibly nonfunctional, never advertised as an active feature.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-SEL-01` | malformed/unknown persisted name or value | load reports/records the invalid input according to the declared policy and does not mutate unrelated state | `CHK-SEL-02` |
| `FM-SEL-02` | strict capability-gated request unavailable | active state reports unavailable/refused; Automatic only uses documented fallback | `CHK-SEL-03` |
| `FM-SEL-03` | concurrent file edit/write failure or packaged unwritable location | save fails visibly and preserves a valid previous file/section | `CHK-SEL-02` |
| `FM-SEL-04` | selector registered without consumer or undocumented control becomes reachable | source/runtime enumeration reports orphan and scope freeze fails | `CHK-SEL-01` |
| `FM-SEL-05` | restart/topology/history-affecting value is treated as ordinary live state | lifecycle check detects mismatched requested/active generation and holds the cell | `CHK-SEL-03` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-SEL-01` | mechanically enumerate Renderer CVars, Renderer-settings allowlist/state/UI, and routed RHI controls; compare names/domains/defaults/consumers to every catalog row and negative selector | `AC-SEL-01`, `AC-SEL-06`, `AC-SEL-07`; `FM-SEL-04` |
| `CHK-SEL-02` | round-trip exact 27-name section with boundary/malformed/unknown values, unrelated sections, concurrent edit, write failure, and packaged path cases | `AC-SEL-02`, `AC-SEL-04`; `FM-SEL-01`, `FM-SEL-03` |
| `CHK-SEL-03` | runtime matrix over valid/invalid/strict/Automatic/restart/topology/history changes; compare requested/active state, reason, graph/history generation, and next-frame result | `AC-SEL-02`, `AC-SEL-03`, `AC-SEL-05`; `FM-SEL-02`, `FM-SEL-05` |

This selector contract is **defined but unproved**. A registered or persisted name is not feature evidence; release results must demonstrate the active consumer and observable result in every advertised cell.

## Primary Source Routes

- [`RendererCVars.cpp`](../../../../../../Engine/Renderer/Private/Debug/RendererCVars.cpp)
- [`EngineRenderingSettingsRuntime.cpp`](../../../../../../Engine/Renderer/Private/Settings/EngineRenderingSettingsRuntime.cpp) and [`EngineRenderingSettingsPersistence.cpp`](../../../../../../Engine/Renderer/Private/Settings/EngineRenderingSettingsPersistence.cpp)
- [`ViewportDisplayCVars.cpp`](../../../../../../Engine/Renderer/Private/View/ViewportDisplayCVars.cpp)
- [`UpscalerSettings.cpp`](../../../../../../Engine/Renderer/Private/Upscaling/UpscalerSettings.cpp) and [`RayReconstructionSettings.cpp`](../../../../../../Engine/Renderer/Private/RayReconstruction/RayReconstructionSettings.cpp)
- [`RayTracedShadowCVars.cpp`](../../../../../../Engine/Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowCVars.cpp), [`RestirIndirectLightingCVars.cpp`](../../../../../../Engine/Renderer/Private/RayTracing/Effects/RestirLighting/RestirIndirectLightingCVars.cpp), and [`PathTracedLightingCVars.cpp`](../../../../../../Engine/Renderer/Private/RayTracing/Effects/PathTracedLighting/PathTracedLightingCVars.cpp)
- [`MaterialCVars.cpp`](../../../../../../Engine/Renderer/Private/Scene/Materials/MaterialCVars.cpp)
- [`RHICVars.cpp`](../../../../../../Engine/RHI/Private/CVars/RHICVars.cpp)
