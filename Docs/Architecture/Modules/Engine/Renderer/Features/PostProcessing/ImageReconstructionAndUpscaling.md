# Renderer Image Reconstruction And Upscaling

Status: current feature dossier; source-backed, not provider validation, visual-quality, latency, performance, packaging, or release evidence

Verified: 2026-09-06 against source revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Scope: `REN-POST-04` through `REN-POST-06`; render-to-output extent resolution through Linear upscale, NVIDIA DLSS Super Resolution, or NVIDIA DLSS Ray Reconstruction

Parent family: [Post Processing](README.md)

## Feature Promise

Sparkle converts the scene-linear render result at render extent into one `ResolvedSceneColor` at output extent. Ray Reconstruction may own that result for the ReSTIR route; otherwise upscaling owns it. The frame never intentionally applies both reconstruction and a second upscale to the same resolved product.

| Feature | Requested choices | Active/failure rule | Inputs |
| --- | --- | --- | --- |
| Linear upscale | baseline provider | always-available internal path | scene color |
| NVIDIA DLSS Super Resolution | NativeAA, Quality, Balanced, Performance, UltraPerformance | Streamline/capability/native-interop gated; initialization failure resolves to Linear | color, device depth, motion, exposure, camera/temporal/extents |
| NVIDIA DLSS Ray Reconstruction | Off or NVIDIA RR | ReSTIR-only; initialization failure resolves Off | color, depth, motion, diffuse/specular albedo, normal, roughness, specular hit distance, exposure |

Vulkan currently refuses the external Streamline evaluation route instead of claiming parity with D3D12 native interop. Provider source presence does not prove supported hardware, driver behavior, input tagging, binary staging, quality, performance, or redistribution.

## Ownership And Lifetime

- Renderer settings/CVars express requested provider and quality; the provider stack owns readiness and resolved active state.
- Provider key/generation contributes to graph topology and prevents a graph from binding stale provider state.
- Old provider generations retire after their last queue submissions complete.
- Reconstruction consumes ReSTIR guide products only when that topology is active. Linear/DLSS SR consume the normal scene/depth/motion/exposure inputs.
- Tone mapping consumes the one resolved output; reconstruction/upscaling does not own tone mapping, color grading, chromatic aberration, output encoding, or frame generation.

## Failure, Tradeoffs, And Evidence

- Unavailable provider initialization must reset to a named supported alternate and must not continue reporting the vendor path as active.
- Resize, scene/view discontinuity, provider/shader/topology generation changes must reset affected temporal state.
- The built-in Linear path provides a deterministic ownership fallback but has an unmeasured quality/cost envelope.
- External providers reduce in-engine algorithm ownership but add adapter, SDK, binary, interop, packaging, and attribution dependencies.
- DLSS Super Resolution and Ray Reconstruction reconstruct the current frame; neither is evidence of DLSS Frame Generation.
- `REN-E14`, `REN-E15`, and `REN-E16` own Linear, DLSS SR, and DLSS RR execution evidence respectively; the proof contract is defined below.

## Horizontal Coverage

| Provider/result | D3D12 | Vulkan | Required distinction |
| --- | --- | --- | --- |
| Linear | implemented baseline | implemented baseline | render/output extent, filtering, alpha, and cost |
| DLSS Super Resolution | capability/SDK/driver/interposer gated | external evaluation refused | requested quality versus active provider, tagged inputs, reset, fallback to Linear |
| DLSS Ray Reconstruction | capability-gated and ReSTIR-only | external evaluation refused | requested versus active mode, guide completeness, reset, fallback Off |

NativeAA and render-to-output scaling ratios need separate quality/cost cells. Unsupported provider/backend/lighting combinations require explicit inactive results rather than borrowed success from Linear or D3D12.

## Acceptance Criteria

- `AC-IRU-01` — one and only one producer writes `ResolvedSceneColor` at the requested output extent and documented scene-linear format for every supported combination.
- `AC-IRU-02` — Linear produces finite in-bounds output for unity, down/upscale, odd dimensions, one-pixel edges, resize, and alpha fixtures with a predeclared filtering oracle.
- `AC-IRU-03` — DLSS SR activates only with matching adapter/driver/SDK/interposer/backend readiness and complete color/depth/motion/exposure/camera/extent tags; requested quality and actual active provider are inspectable.
- `AC-IRU-04` — DLSS RR activates only for ReSTIR with every required guide; its active state and output ownership prevent a second upscale.
- `AC-IRU-05` — initialization/evaluation failure resolves SR to Linear and RR to Off exactly as documented, reports the reason, rebuilds affected topology, and never reports the vendor path as active.
- `AC-IRU-06` — camera cut, resize, scene/view/provider/shader/topology change resets temporal provider state and prevents mixed-generation inputs or outputs.
- `AC-IRU-07` — old provider generations remain alive through last-use queue tokens and are reclaimed within a declared bound during repeated switches/failures/shutdown.
- `AC-IRU-08` — package/runtime evidence identifies binaries, versions, provenance, redistribution boundary, adapter/driver, backend, quality mode, extents, input tags, latency, quality metric, and observer configuration.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-IRU-01` | SDK/interposer/binary/capability absent or initialization fails | SR resolves Linear; RR resolves Off; reason and requested/active state remain visible | `CHK-IRU-02` |
| `FM-IRU-02` | missing/wrong-format/wrong-extent/stale guide or motion/depth/exposure input | provider evaluation rejects before publication; no stale product is presented | `CHK-IRU-02`, `CHK-IRU-03` |
| `FM-IRU-03` | resize/cut/provider or lighting-mode switch while work is in flight | temporal state resets, graph generation changes, and old provider retires by completion | `CHK-IRU-03` |
| `FM-IRU-04` | external provider requested on Vulkan | active result explicitly refuses/falls back according to the documented provider rule, never claims parity | `CHK-IRU-02` |
| `FM-IRU-05` | provider claims output while upscale also runs | graph/resource contract detects duplicate `ResolvedSceneColor` producer | `CHK-IRU-01` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-IRU-01` | graph/resource audit plus Linear numeric pattern tests over extent/edge/alpha matrix | `AC-IRU-01`, `AC-IRU-02`; `FM-IRU-05` |
| `CHK-IRU-02` | provider readiness matrix varying backend, adapter capability, SDK/binary/interposer, quality, lighting mode, and each missing input | `AC-IRU-03`–`AC-IRU-05`, `AC-IRU-08`; `FM-IRU-01`, `FM-IRU-02`, `FM-IRU-04` |
| `CHK-IRU-03` | temporal churn across motion, camera cut, resize, scene/view/provider/shader/topology changes and repeated failure/recovery; inspect tags, generations, queue tokens, and retention | `AC-IRU-06`, `AC-IRU-07`; `FM-IRU-02`, `FM-IRU-03` |
| `CHK-IRU-04` | representative quality/performance/package run with predeclared metric and baseline at each supported extent/quality cell | `AC-IRU-02`, `AC-IRU-03`, `AC-IRU-04`, `AC-IRU-08` |

This contract is **defined but unproved**. Linear, DLSS SR, and DLSS RR receive independent verdicts; a valid fallback proves recovery, not the requested vendor feature.

## Primary Source Routes

- [`Upscaling.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Presentation/Upscaling.cpp)
- [`RendererImageProviderStack.cpp`](../../../../../../../Engine/Renderer/Private/Providers/RendererImageProviderStack.cpp)
- [`RestirRayReconstruction.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Restir/RestirRayReconstruction.cpp)
