# HDR Display Output Research

**Status:** research; current primary-source platform/standard/implementation study, not SparkleEngine architecture, implementation, or HDR evidence

**Responsibility:** compare normative HDR signal semantics, current Windows guidance, D3D12 source precedent, and Vulkan presentation contracts for `HDRD-00`

**Authority boundary:** [Discovery](Discovery.md) owns local decisions; [Semantics](Semantics.md) owns accepted math; [Execution Architecture](ExecutionArchitecture.md) owns Renderer/RHI shape; [README](README.md) owns feature acceptance

**Researched:** 2026-09-10

**Current readiness:** Not applicable — research adds no readiness credit.

## Local Baseline

Sparkle currently exposes 8-bit SDR presentation only. Renderer has tone-map and linear/sRGB encode passes; RHI has format-selected D3D12/Vulkan swapchains but no color-space/luminance/metadata state. D3D12 and Vulkan therefore need a new neutral presentation capability/result contract, while Renderer needs an explicit HDR output transform. Neither module can infer the other's active state.

## Normative Signal Semantics

ITU-R BT.2100-3 defines HDR-TV image parameters for PQ and HLG systems, including wide-color-gamut primaries and transfer behavior.[^1] The first-release candidate selects PQ, not HLG. BT.2408-6 defines HDR Reference White as the signal from a 100% reflectance white card, nominally 203 cd/m² for PQ or a 1000-nit HLG display.[^2]

These are signal/production references. They do not choose Sparkle's scene-working primaries, artistic tone mapper, mastering target, UI policy, or display capability handling.

## Current Windows Advanced Color Guidance

Microsoft documents two Advanced Color swapchain routes: FP16 with scRGB is recommended for general-purpose applications; UINT10 with HDR10/BT.2100 is a narrower performance path for HDR displays, Direct3D, and swapchains that do not require alpha/transparency. The HDR10 route requires explicit `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` selection.[^3]

Windows treats SDR reference white as user/system state. For self-composited SDR UI on HDR, the application should query the current SDR white level and scale from Windows' nominal 80-nit reference; desktop settings are often around 200 nits but 200 is not a universal constant.[^3][^4]

Microsoft now warns that applications should not rely on `SetHDRMetaData`: Windows may not send it to the display and monitors handle it inconsistently. The guidance is to tone-map into the display's reported gamut/luminance range; metadata does not change pixel interpretation, which requires color-space selection.[^5]

**Discovery consequence:** the existing fixed 200-nit UI and mandatory-static-metadata wording must be ratified or revised. Metadata cannot be an activation oracle.

## D3D12 Source Precedent

Microsoft's D3D12 HDR sample demonstrates separate 8-bit, 10-bit, and FP16 swapchain formats; chooses ST 2084 with `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` for 10-bit; calls `CheckColorSpaceSupport` before `SetColorSpace1`; reapplies color space after resize; and rechecks display HDR support from `IDXGIOutput6::GetDesc1` on display changes.[^6]

This is valuable native lifecycle precedent. It is a sample, not current platform policy or local proof. Its support heuristic and fixed choices must be reconciled with current Advanced Color guidance, Sparkle's interposer swapchain path, UI composition, Vulkan parity, and the exact release OS.

## Vulkan Contracts

`VK_EXT_swapchain_colorspace` adds surface color spaces including `VK_COLOR_SPACE_HDR10_ST2084_EXT`; the selected `VkSurfaceFormatKHR` pairs format and color space.[^7] `VK_EXT_hdr_metadata` supplies mastering/content metadata to presentation engines, but explicitly does not override color space/encoding, does not define how the presentation engine uses it, and defines absence as valid.[^8]

**Discovery consequence:** Vulkan activation requires a compatible enumerated surface tuple and extension/call policy; metadata is independent optional state, not proof that HDR pixels are interpreted correctly or displayed faithfully.

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

