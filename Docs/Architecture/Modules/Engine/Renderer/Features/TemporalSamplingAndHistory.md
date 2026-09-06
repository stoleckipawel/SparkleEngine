# Renderer Temporal Sampling and History

Status: current feature dossier; source-backed, not visual-stability, reconstruction-quality, or release evidence

Verified: 2026-09-06 against committed `master` revision `baa0adc9`; current `Engine/Renderer` source inspected

Scope: `REN-TEMP-01` through `REN-TEMP-05`; owns per-view jitter, previous-camera publication, history validity, invalidation causes, and the motion/reprojection convention shared by temporal consumers

## Feature Contract

Temporal sampling is a view-owned contract, not an implementation detail of one upscaler. `RenderViewState` turns frame/view/scene/provider/topology identity into one current/previous temporal uniform. GBuffer, motion-vector, ReSTIR, reference accumulation, exposure history, and external reconstruction consume that shared identity; no consumer may invent a second camera-history or jitter convention.

```text
view identity + frame ID + camera + render extent + generations
  -> observe discontinuities
  -> select deterministic Halton sample
  -> publish current/previous jitter and previous matrices
  -> invalidate or admit history
  -> GBuffer/motion/reprojection/history/provider consumers
```

The motivation is consistency. A sharp current sample, correct unjittered motion, reservoir reprojection, accumulation, and provider inputs all depend on agreeing which pixel grid and which previous view a frame means. Centralizing that agreement prevents feature-local history from surviving a scene, camera, shader, provider, or topology change that made it incompatible.

## Current Sampling Policy

| Concern | Current behavior | Boundary |
| --- | --- | --- |
| Active sequence | Halton bases 2 and 3, indexed from 1 and repeated over a 16-frame window | hard-coded by `RenderViewState`; there is no public selector |
| Pattern domain | sample offsets are centered around `[-0.5, 0.5]` before extent normalization | sequence quality and period adequacy are unproved |
| NDC conversion | `(2*x/width, -2*y/height)` | the Y sign is part of the shader/provider ABI |
| Invalid extent | width or height at most 1 produces zero jitter | no fabricated divide-by-zero sample |
| Source-only alternatives | 8-sample MSAA, R2, deterministic white noise, and None exist in `TemporalJitterPatterns` | vocabulary/internal callable paths only; no inspected active setting or graph choice selects them |

The source alternatives must not be advertised as user-selectable anti-aliasing modes. The current product path is the Halton sequence only.

## Persistent View State And Invalidation

`RenderViewState` retains view identity, prior camera pose/matrices, prior NDC jitter, generation observations, two lighting-history hashes, the ray partition planner, and pending/last invalidation flags. A continuing frame publishes previous matrices and sets `HistoryValid = 1` only when a previous pose exists and no invalidation is pending.

| Invalidation cause | Detection owner | Required consequence |
| --- | --- | --- |
| view identity | viewport ID, selection, or view kind changes | reset shared temporal history and view-local RT planning |
| scene generation | persistent render scene changes generation | reject prior scene history |
| explicit camera cut/teleport | submitted view flags | reject prior camera/history immediately |
| inferred camera discontinuity | position delta over 5 m, view-direction dot below `0.8660254`, or FOV delta over 6 degrees | reject likely incompatible history |
| projection change | projection kind, FOV, near/far, or orthographic height differs beyond the current tolerance | reject prior projection history |
| shader generation | replacement shader runtime becomes active | reject history produced by incompatible programs |
| image-provider generation | active reconstruction/upscale provider changes generation | reset provider and shared temporal continuity |
| graph-topology generation | pass/resource topology changes | reject histories tied to the previous graph |
| explicit reset | construction or owner-requested invalidation | begin with invalid history |

An invalidation resets the reference/ReSTIR history hashes and the per-view ray partition planner as well as the common validity flag. Feature-specific history can add a narrower invalidation hash, but it cannot override an invalid common view history.

## Consumer And Coordinate Contract

| Consumer | Use of the shared temporal state |
| --- | --- |
| raster GBuffer | applies the current jitter to rasterized clip position |
| motion-vector shader | emits zero when common history is invalid; removes current jitter so provider motion is unjittered |
| ReSTIR reprojection | applies geometric motion, then moves from current to previous jittered pixel grid using the jitter delta |
| frame histories | invalidates exposure, reservoir, reference accumulation, and provider histories when common validity is false; lighting-specific hashes can invalidate further |
| Streamline providers | convert current NDC jitter to pixels, publish previous/current clip transforms only when valid, declare motion vectors unjittered, and set provider reset when requested or invalid |

Current and previous matrices are zero/default when history is invalid and must not be consumed as meaningful transforms. `HistoryValid`, not nonzero matrix contents, is the authority.

## Horizontal Coverage

| Axis | Required cells | Shared invariant |
| --- | --- | --- |
| view | game/editor, swapchain/offscreen, perspective/orthographic, two simultaneous view identities | each view owns independent prior pose/jitter/history |
| motion | static, camera translation/rotation/FOV, rigid object, skin, morph, sky/background | motion and reprojection use one unjittered-motion convention |
| discontinuity | first frame, explicit cut, teleport, inferred cut, projection change, scene reset, resize/topology, shader/provider change | first incompatible frame reports invalid history before any temporal consumer executes |
| frontend/provider | raster/ray GBuffer, ReSTIR/reference, Linear/DLSS/RR when reachable | a frontend/provider cannot reinterpret jitter sign, units, or validity |
| backend | D3D12 and Vulkan for native consumers; D3D12-only external provider route | common CPU/shader temporal data is backend-neutral; unsupported provider cells stay explicit |

## Acceptance Criteria

- `AC-TMP-01` — the active 16-frame Halton sequence is deterministic, centered, extent-normalized with the documented Y sign, repeats at the declared period, and produces zero for invalid extents.
- `AC-TMP-02` — a continuing unchanged view publishes the immediately prior matrices/jitter and valid history; two views never exchange prior state.
- `AC-TMP-03` — every documented invalidation cause clears common, lighting-specific, provider, and view-relative RT history before the first incompatible consumer and records the reason.
- `AC-TMP-04` — raster placement, rigid/deforming/sky motion vectors, ReSTIR reprojection, reference accumulation, and provider constants agree on current/previous jitter and unjittered-motion semantics.
- `AC-TMP-05` — source-only MSAA/R2/white-noise/None patterns remain unreachable from release settings or receive their own capability and evidence contract before exposure.
- `AC-TMP-06` — D3D12 and Vulkan native paths produce the same temporal uniforms and raw motion/reprojection oracle for identical submitted views.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-TMP-01` | first frame or reused identity with no compatible prior pose | `HistoryValid = 0`; consumers do not read default previous matrices as history | `CHK-TMP-01` |
| `FM-TMP-02` | cut/teleport/projection/scene/shader/provider/topology discontinuity | exact reason is recorded and all affected histories reset before use | `CHK-TMP-02` |
| `FM-TMP-03` | two views interleave or a viewport identity is reused | state stays isolated or explicitly resets; no cross-view ghost history | `CHK-TMP-02` |
| `FM-TMP-04` | jitter sign/unit or motion convention differs in one consumer | numerical oracle detects divergent raster, motion, reprojection, or provider constants | `CHK-TMP-03` |
| `FM-TMP-05` | unsupported pattern becomes externally reachable | selector/catalog audit fails until it has an owned active result and proof | `CHK-TMP-04` |

| Check | Cheapest claim-falsifying exercise | Covers |
| --- | --- | --- |
| `CHK-TMP-01` | pure sequence/extent test plus two ordinary frames; compare exact samples, prior matrices/jitter, and validity | `AC-TMP-01`, `AC-TMP-02`; `FM-TMP-01` |
| `CHK-TMP-02` | dual-view frame trace injecting every invalidation cause and inspecting reason flags plus all history/provider resets | `AC-TMP-02`, `AC-TMP-03`; `FM-TMP-02`, `FM-TMP-03` |
| `CHK-TMP-03` | controlled static/camera/rigid/skin/morph/sky motion fixture; compare raw raster position, motion, reprojection coordinate, and provider constants on reachable backends | `AC-TMP-04`, `AC-TMP-06`; `FM-TMP-04` |
| `CHK-TMP-04` | enumerate settings/CVars/public requests and assert only the documented Halton path is reachable | `AC-TMP-05`; `FM-TMP-05` |

This contract is **defined but unproved**. A stable-looking image is not enough: completion requires exact temporal values, invalidation order, cross-consumer agreement, and multi-view isolation artifacts.

## Primary Source Routes

- [`RenderViewState.h`](../../../../../../Engine/Renderer/Private/View/RenderViewState.h) and [`RenderViewState.cpp`](../../../../../../Engine/Renderer/Private/View/RenderViewState.cpp)
- [`TemporalJitterPatterns.cpp`](../../../../../../Engine/Renderer/Private/Temporal/TemporalJitterPatterns.cpp)
- [`ViewTemporalUniformData.h`](../../../../../../Engine/Renderer/Private/ShaderData/ViewTemporalUniformData.h)
- [`MotionVector.hlsli`](../../../../../../Engine/Assets/Shaders/Passes/GBuffer/MotionVector.hlsli)
- [`FrameHistory.cpp`](../../../../../../Engine/Renderer/Private/Resources/History/FrameHistory.cpp)
- [`StreamlineViewConstants.cpp`](../../../../../../Engine/Renderer/Private/Streamline/StreamlineViewConstants.cpp)
