# HDR Display Output Research

**Status:** research; current primary-source platform/standard/implementation study, not SparkleEngine architecture, implementation, or HDR evidence

**Responsibility:** compare normative HDR signal semantics, current Windows guidance, D3D12 source precedent, and Vulkan presentation contracts for `HDRD-00`

**Authority boundary:** [Discovery](Discovery.md) owns local decisions; [Semantics](Semantics.md) owns accepted math; [Execution Architecture](ExecutionArchitecture.md) owns Renderer/RHI shape; [README](README.md) owns feature acceptance

**Researched:** 2026-09-10

**Current readiness:** Not applicable — research adds no readiness credit.

## Research Decision

The primary sources establish enough normative and platform vocabulary to design discovery probes, but they expose a genuine product fork: the admitted HDR10/UINT10/PQ signal is narrower than the Windows general-purpose FP16/scRGB Advanced Color recommendation, especially where alpha/composition/UI are involved. Therefore the research result is **proceed to `HDRD-00`, do not proceed to implementation**.

## Research Questions

1. Which signal/color/luminance rules are normative, and which remain Sparkle creative/product policy?
2. Which Windows swapchain route is eligible for each editor/runtime/window/UI profile?
3. Which facts must D3D12 and Vulkan expose without moving color policy into RHI?
4. Which events invalidate output association, tuple, SDR white, metadata, and Renderer transform?
5. Which artifacts prove pixel construction, native activation, OS/display behavior, and fallback independently?

## Research Method

The study separates standards, current platform guidance, revision-pinned implementation samples, API/extension contracts, and current Sparkle source. Standards define signal terms; Microsoft/Khronos define platform/API obligations and limitations; samples demonstrate lifecycle precedent; only current Sparkle source proves local presence/absence. No one class substitutes for another, and external screenshots or display results never become Sparkle goldens.

## Source Ledger

| ID | Primary source | Question answered | Transfer limit |
| --- | --- | --- | --- |
| `HDR-REF-ITU-PQ` | ITU-R BT.2100-3 | what PQ/HLG HDR-TV signal parameters and transfer definitions are normative? | signal semantics only; no Sparkle scene/tone/target policy |
| `HDR-REF-ITU-2020` | ITU-R BT.2100-3 plus BT.2408-6 | which wide-gamut/D65/reference-white terms constrain the candidate? | no display capability or UI implementation proof |
| `HDR-REF-MS-AC-01` | Microsoft Advanced Color guidance | when are FP16/scRGB versus UINT10/HDR10 routes recommended and how should dynamic output state be handled? | mutable current Windows guidance; profile eligibility must be probed locally |
| `HDR-REF-MS-AC-02` | Microsoft `DISPLAYCONFIG_SDR_WHITE_LEVEL` guidance | how is current SDR white reported relative to nominal 80 nits? | query semantics; no universal white or Sparkle fallback value |
| `HDR-REF-DX-01` | DirectX Graphics Samples `D3D12HDR.cpp`, commit `213dd4fd4918ea009dd8f35adee1aff1f2ecaba4` | what concrete format/color-space/resize/output lifecycle has a Microsoft sample used? | lifecycle precedent, not current product policy or local proof |
| `HDR-REF-DX-02` | Microsoft `IDXGISwapChain4::SetHDRMetaData` guidance | can static metadata be relied on as display or activation truth? | metadata limitation; does not decide omit/best-effort/required policy |
| `HDR-REF-VK-01` | `VK_EXT_swapchain_colorspace`, Vulkan Docs commit `f84d432d5b8912362f96f581f29bbc4f3c8c7843` | how is HDR10 color space enumerated and paired with a surface format? | API contract; extension presence is not local usable support |
| `HDR-REF-VK-02` | Vulkan WSI surface/swapchain contracts at the same docs revision | when must surface capabilities/formats and swapchain state be re-evaluated? | mechanism/lifecycle; Windows display facts still require a platform owner |
| `HDR-REF-VK-03` | `VK_EXT_hdr_metadata`, same docs revision and current manual | what does metadata set and what does it explicitly not control? | metadata semantics only; no guarantee of presentation-engine/display effect |

## Completion Vocabulary

| Term | Meaning in this package |
| --- | --- |
| `HDR10 signal` | Rec.2020/D65 RGB encoded with ST 2084/PQ in the accepted bounded range and presented through an eligible HDR10 native tuple |
| `scRGB signal` | linear FP16 Advanced Color route with its own primaries/scale/composition contract; not another name for HDR10/PQ |
| `requested` | product/user wants HDR; establishes no capability or active pixels |
| `supported` | required static OS/backend/API feature exists; not yet current-window eligible |
| `eligible` | current output/window/composition/profile admits one complete native route |
| `active` | current output/swapchain generation has the accepted tuple and Renderer uses its matching transform generation |
| `fallback SDR` | HDR remains requested but known-good SDR presentation/transform is current, with a reason |
| `metadata disposition` | omitted/requested/submitted/call-failed/unknown-effect or other frozen state; never signal interpretation |
| `display evidence` | externally observed/measured emitted behavior tied to display/OS/settings/candidate; not inferred from raw buffers or screenshots |

## Local Baseline

Sparkle currently exposes 8-bit SDR presentation only. Renderer has tone-map and linear/sRGB encode passes; RHI has format-selected D3D12/Vulkan swapchains but no color-space/luminance/metadata state. D3D12 and Vulkan therefore need a new neutral presentation capability/result contract, while Renderer needs an explicit HDR output transform. Neither module can infer the other's active state.

## Current Sparkle Source Trace

| Surface | Current truth at `ca55e7d8` | Discovery implication |
| --- | --- | --- |
| pixel formats/defaults | [`PixelFormat.h`](../../../../../../../../../Engine/RHI/Public/Formats/PixelFormat.h) has floating formats but no packed R10G10B10A2; [`RhiPresentationDefaults.h`](../../../../../../../../../Engine/RHI/Public/Presentation/RhiPresentationDefaults.h) admits four 8-bit SDR defaults | exact admitted format/profile changes must be explicit and paired with color space |
| presentation service | [`RhiPresentationService.h`](../../../../../../../../../Engine/RHI/Public/Presentation/RhiPresentationService.h) reports current format but no output/capability/color-space/luminance/metadata/request-result generation | introduce one neutral backend-independent facts/request/result boundary before activation |
| D3D12 swapchain | [`D3D12SwapChain.cpp`](../../../../../../../../../Engine/RHI/Private/D3D12/SwapChain/D3D12SwapChain.cpp) creates/resizes/presents without Advanced Color policy | output association, capability, tuple, interface/interposer, recreation, and fallback are absent |
| Vulkan swapchain | [`VulkanSwapChain.cpp`](../../../../../../../../../Engine/RHI/Private/Vulkan/SwapChain/VulkanSwapChain.cpp) selects requested format only with `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR` | HDR surface tuple/extensions/metadata and truthful failure are absent |
| Renderer presentation | [`Presentation.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp) exposes separate tone-mapped/encoded products but no HDR target/PQ route | preserve one Renderer color owner and generation-join it to RHI active state |
| UI | [`UiFrameRenderer.cpp`](../../../../../../../../../Engine/Renderer/Private/UI/UiFrameRenderer.cpp) uses the presentation service/overlay route | editor composition/interposer/alpha eligibility is a first-class discovery question |
| settings/package/capture | no HDR request/result, native tuple, semantic capture identity, or product evidence route was found | source absence earns no runtime or release claim |

## Normative Signal Semantics

ITU-R BT.2100-3 defines HDR-TV image parameters for PQ and HLG systems, including wide-color-gamut primaries and transfer behavior.[^1] The first-release candidate selects PQ, not HLG. BT.2408-6 defines HDR Reference White as the signal from a 100% reflectance white card, nominally 203 cd/m² for PQ or a 1000-nit HLG display.[^2]

These are signal/production references. They do not choose Sparkle's scene-working primaries, artistic tone mapper, mastering target, UI policy, or display capability handling.

## Current Windows Advanced Color Guidance

Microsoft documents two Advanced Color swapchain routes: FP16 with scRGB is recommended for general-purpose applications; UINT10 with HDR10/BT.2100 is a narrower performance path for HDR displays, Direct3D, and swapchains that do not require alpha/transparency. The HDR10 route requires explicit `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` selection.[^3]

Windows treats SDR reference white as user/system state. For self-composited SDR UI on HDR, the application should query the current SDR white level and scale from Windows' nominal 80-nit reference; desktop settings are often around 200 nits but 200 is not a universal constant.[^3][^4]

Microsoft's guidance also treats Advanced Color capability as dynamic display state. Window/output association can change as a window moves; display-change notifications require re-query; and the guidance warns against relying on a stale containing-output result after display changes.[^3] This makes output identity and query generation part of active-state correctness, not a UI refresh concern.

Microsoft now warns that applications should not rely on `SetHDRMetaData`: Windows may not send it to the display and monitors handle it inconsistently. The guidance is to tone-map into the display's reported gamut/luminance range; metadata does not change pixel interpretation, which requires color-space selection.[^5]

**Discovery consequence:** the existing fixed 200-nit UI and mandatory-static-metadata wording must be ratified or revised. Metadata cannot be an activation oracle.

## D3D12 Source Precedent

Microsoft's D3D12 HDR sample demonstrates separate 8-bit, 10-bit, and FP16 swapchain formats; chooses ST 2084 with `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` for 10-bit; calls `CheckColorSpaceSupport` before `SetColorSpace1`; reapplies color space after resize; and rechecks display HDR support from `IDXGIOutput6::GetDesc1` on display changes.[^6]

This is valuable native lifecycle precedent. It is a sample, not current platform policy or local proof. Its support heuristic and fixed choices must be reconciled with current Advanced Color guidance, Sparkle's interposer swapchain path, UI composition, Vulkan parity, and the exact release OS.

## Vulkan Contracts

`VK_EXT_swapchain_colorspace` adds surface color spaces including `VK_COLOR_SPACE_HDR10_ST2084_EXT`; the selected `VkSurfaceFormatKHR` pairs format and color space.[^7] `VK_EXT_hdr_metadata` supplies mastering/content metadata to presentation engines, but explicitly does not override color space/encoding, does not define how the presentation engine uses it, and defines absence as valid.[^8]

**Discovery consequence:** Vulkan activation requires a compatible enumerated surface tuple and extension/call policy; metadata is independent optional state, not proof that HDR pixels are interpreted correctly or displayed faithfully.

The current Vulkan extension manual reiterates that HDR metadata does not override swapchain color space/encoding, that presentation-engine behavior lies outside the API's control, and that not supplying metadata is valid.[^9] The correct local abstraction therefore records metadata disposition separately from selected surface tuple and active result.

## Cross-Source Comparison

| Question | Current primary-source answer | Sparkle decision |
| --- | --- | --- |
| What defines PQ/BT.2100 signal math? | ITU-R BT.2100-3 | adopt exact constants/domain in Semantics after scene-domain decision |
| Is fixed 200-nit SDR UI universally correct? | no; Windows exposes current user/system SDR white, nominal base 80 nits | `HDRD-05` |
| Is 10-bit PQ always the recommended Windows route? | no; it is narrower than FP16/scRGB | `HDRD-03` per product/profile |
| Does 10-bit format alone activate HDR? | no; compatible color space and current output state are required | tuple-based RHI active result |
| Does metadata select or prove HDR? | no; Microsoft discourages reliance, Vulkan leaves use outside the API contract | `HDRD-09`, never an oracle |
| Must state be reapplied after swapchain/display changes? | yes; native sample and WSI lifecycle require reconstruction/re-evaluation | explicit transition state machine |
| Can a screenshot prove physical HDR appearance? | no; OS/display processing and capture path may differ | raw/native/measurement artifact separation |

## Recommended Discovery Direction

These recommendations are non-binding until `HDRD-00 PASS`:

- Preserve HDR10/PQ as the target signal, but make packed-10 eligibility explicit and evaluate whether DevelopmentEditor/UI composition needs an FP16/scRGB cell or an exclusion.
- Query and expose the current output's color/luminance state and current SDR white; use bounded policy defaults only when the query is unavailable and report that fallback.
- Treat static metadata as optional/best-effort diagnostic state unless product evidence proves a stricter requirement. Never gate active color interpretation on the call alone.
- Make RHI return a structured presentation result containing requested, supported/eligible, active, fallback, native tuple, output identity, and reason. Renderer selects transforms from the active result, never from request alone.
- Keep SDR as the atomic safe route through every failure and transition.

## Initial Missing-And-Unknown Ledger

| ID | Unknown | Why research does not answer it | Required closure |
| --- | --- | --- | --- |
| `HDR-U-01` | exact Windows versions, window modes, editor/runtime, alpha/UI/interposer profile matrix | platform guidance is conditional and local host composition matters | `HDR-EXP-02`, `HDRD-01/03/07` |
| `HDR-U-02` | scene primaries/white/units/exposure/reference-white relation | source labels do not establish a full scene color contract | current shader/resource trace plus `HDRD-02` |
| `HDR-U-03` | packed UINT10/PQ versus optional FP16/scRGB eligibility per cell | both are legitimate but semantically/native-distinct routes | platform probe and `HDRD-03` |
| `HDR-U-04` | fixed 1000-nit, display-adaptive, or bounded hybrid creative target | standards/display capabilities do not choose authored tone policy | `HDR-EXP-04`, `HDRD-04` |
| `HDR-U-05` | current SDR white query/event/failure/fallback bounds | Microsoft defines query meaning, not Sparkle product fallback | Windows probe and `HDRD-05` |
| `HDR-U-06` | exact tone/gamut/PQ/dither/quantization/alpha semantics | multiple valid output transforms exist | semantic oracle and `HDRD-06` |
| `HDR-U-07` | D3D12 interposer/swapchain interface and output association ownership | sample bypasses Sparkle's host route | source/native probe and `HDRD-07` |
| `HDR-U-08` | Vulkan Win32 output association and extension/surface usability on release drivers | extension publication is not machine support | `HDR-EXP-03`, `HDRD-08` |
| `HDR-U-09` | omit/best-effort/required static metadata policy | primary sources explicitly prevent treating it as authoritative | product decision `HDRD-09` |
| `HDR-U-10` | atomic transition protocol and maximum black/fallback latency | APIs provide mechanisms, not Sparkle transaction design | fault model and `HDRD-10` |
| `HDR-U-11` | UI/debug/capture composition paths and screenshot meaning for each profile | current overlay route may constrain tuple/alpha | workflow/product trace and `HDRD-11` |
| `HDR-U-12` | minimum physical measurement/display/backend/package evidence matrix | raw values and monitor appearance prove different claims | evidence dry run and `HDRD-12` |

No unknown may be hidden in a backend adapter or chosen after candidate output. Discovery closes, excludes, or blocks each one explicitly.

## Oracle Ladder

1. high-precision scalar/vector hand calculations for matrices, tone policy, PQ, UI-white scaling, packing, and edge values;
2. independent CPU image products for ramps, patches, gamut wedges, alpha/UI sentinels, black/diffuse/peak values, and defect mutations;
3. Renderer raw scene/target/PQ product comparison before native presentation;
4. native D3D12/Vulkan tuple/output-generation/capability/color-space/metadata inspection;
5. injected query/create/resize/set/metadata/present/display/device faults and atomic SDR recovery;
6. OS/display state and current SDR-white/output queries tied to window/output generation;
7. external capture/measurement/photograph only with calibrated interpretation, display/settings/environment identity, and limitations;
8. paired backend, SDR/HDR display, transition, packaged/clean-machine, UI, cost, and exclusion matrix.

Each rung proves a different claim. Correct raw PQ does not prove an active tuple; an active tuple does not prove emitted luminance; a monitor badge or photograph does not prove the Renderer math; metadata success proves only the accepted call disposition.

## Discovery Work Sequence

1. Freeze current source/release/profile terminology and negative boundary.
2. Probe Windows/D3D12/Vulkan/output/UI/interposer eligibility without changing production code.
3. Trace the complete scene-to-display and UI composition domains.
4. Compare target/SDR-white policies and freeze independent semantic fixtures.
5. Design the neutral presentation result, output generations, transition transaction, and fallback.
6. Dry-run UX and evidence on named hardware, including ambiguous/unavailable cases.
7. Review rights, support/exclusion matrices, budgets, stage estimates, and no-orphan traceability.
8. Record `PASS` only if every accepted target document can be implemented without choosing policy.

## Research Handoff

The handoff contains exact source IDs/revisions, current-source trace, unknown ledger, route/profile alternatives, recommended/non-transferable precedent, oracle ladder, hardware/probe requirements, and rights limitations. Discovery returns one accepted signal/profile/backend/UI/metadata/state/evidence contract with exact document revisions. This research remains planning input and contributes zero readiness.

## Rights And Evidence Limits

Standards are cited, not copied beyond small equations/constants needed for interoperability. External source remains under its own license; any code transfer needs file-level review. Platform queries, sample code, and extension presence do not prove Sparkle execution, monitor behavior, color accuracy, or release support.

## Sources

[^1]: ITU-R, [Recommendation BT.2100-3, Image parameter values for HDR television](https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2100-3-202502-I%21%21PDF-E.pdf), February 2025.
[^2]: ITU-R, [Report BT.2408-6, Guidance for operational practices in HDR television production](https://www.itu.int/dms_pub/itu-r/opb/rep/R-REP-BT.2408-6-2023-PDF-E.pdf), 2023.
[^3]: Microsoft, [Use DirectX with Advanced Color on high/standard dynamic range displays](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range), accessed 2026-09-10.
[^4]: Microsoft, [`DISPLAYCONFIG_SDR_WHITE_LEVEL`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_sdr_white_level), accessed 2026-09-10.
[^5]: Microsoft, [`IDXGISwapChain4::SetHDRMetaData`](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_5/nf-dxgi1_5-idxgiswapchain4-sethdrmetadata), accessed 2026-09-10.
[^6]: Microsoft, DirectX Graphics Samples, [`D3D12HDR.cpp` at `213dd4fd4918ea009dd8f35adee1aff1f2ecaba4`](https://github.com/microsoft/DirectX-Graphics-Samples/blob/213dd4fd4918ea009dd8f35adee1aff1f2ecaba4/Samples/Desktop/D3D12HDR/src/D3D12HDR.cpp), accessed 2026-09-10.
[^7]: Khronos Group, Vulkan Docs, [`VK_EXT_swapchain_colorspace` at `f84d432d5b8912362f96f581f29bbc4f3c8c7843`](https://github.com/KhronosGroup/Vulkan-Docs/blob/f84d432d5b8912362f96f581f29bbc4f3c8c7843/appendices/VK_EXT_swapchain_colorspace.adoc), accessed 2026-09-10.
[^8]: Khronos Group, Vulkan Docs, [`VK_EXT_hdr_metadata` at `f84d432d5b8912362f96f581f29bbc4f3c8c7843`](https://github.com/KhronosGroup/Vulkan-Docs/blob/f84d432d5b8912362f96f581f29bbc4f3c8c7843/appendices/VK_EXT_hdr_metadata.adoc), accessed 2026-09-10.
[^9]: Khronos Group, [current Vulkan `VK_EXT_hdr_metadata` reference page](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_hdr_metadata.html), accessed 2026-09-10.
