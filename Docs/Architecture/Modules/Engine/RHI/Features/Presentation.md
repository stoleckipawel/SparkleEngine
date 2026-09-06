# RHI Presentation

Status: current feature dossier; source-backed, not resize, pacing, color, HDR, or release evidence

Verified: 2026-09-06 at committed `master` revision `8414b5dc`

Scope: `RHI-PRES-*`; swapchain construction, back-buffer identity/state, acquire, resize, frame count, pacing, VSync, submit/present, and the explicit HDR output gap

## Feature Promise

For a valid native window and supported configuration, RHI owns one swapchain lifecycle that acquires a current back buffer, exposes its neutral render state, submits/presents it in order, and rebuilds safely on size changes. The current contract is SDR-oriented and does not claim HDR negotiation or output.

## Ownership And Lifecycle

- Presentation defaults and runtime request determine back-buffer count, frames in flight, VSync, and related neutral configuration within documented bounds.
- Backend swapchains own native buffers, views, acquisition state, present mode/flags, and pacing primitives.
- Resize/minimize must settle or preserve in-flight ownership before replacing buffers; a zero-sized window is not a renderable surface.
- Renderer owns what color it writes and when UI composition occurs. RHI owns swapchain format/state and present mechanics, not tone mapping or output encoding policy.

## Acceptance Criteria

- `AC-RHI-PRES-01` — create/acquire/render/submit/present preserves back-buffer index, frame identity, state, and completion ordering across supported buffer/frame counts on both backends.
- `AC-RHI-PRES-02` — resize, minimize/restore, rapid resize, surface loss, and shutdown replace or retain native buffers without stale views, use-after-free, deadlock, or fabricated presentation.
- `AC-RHI-PRES-03` — VSync and pacing requests produce the documented active mode and observable fallback/rejection; measurements record backend/device configuration.
- `AC-RHI-PRES-04` — device/present failure is surfaced with recoverable state or bounded shutdown; the prior frame is not reported as a new success.
- `AC-RHI-PRES-05` — HDR requests remain explicitly unavailable until color space, format, metadata, negotiation, Renderer encoding, and display evidence have an owned contract.

## Controlled Failures And Checks

| Failure | Safe result | Check |
| --- | --- | --- |
| `FM-RHI-PRES-01` zero/invalid extent or lost surface | no new back-buffer work; wait/rebuild/failure is explicit | `CHK-RHI-PRES-01` resize/minimize/surface-loss loop |
| `FM-RHI-PRES-02` present/device error | exact failure propagates; no success counter/identity advances | `CHK-RHI-PRES-02` injected backend failure |
| `FM-RHI-PRES-03` unsupported pacing/VSync/HDR request | requested-versus-active result is explicit or request rejects | `CHK-RHI-PRES-03` mode/capability matrix |

Check coverage: `CHK-RHI-PRES-01` covers `AC-RHI-PRES-01`, `AC-RHI-PRES-02`, and `FM-RHI-PRES-01`; `CHK-RHI-PRES-02` covers `AC-RHI-PRES-02`, `AC-RHI-PRES-04`, and `FM-RHI-PRES-02`; `CHK-RHI-PRES-03` covers `AC-RHI-PRES-03`, `AC-RHI-PRES-05`, and `FM-RHI-PRES-03`.

Definition of done: long resize/minimize/restore runs, buffer/frame-count matrices, pacing measurements, device-loss behavior, native validation, color/format checks, and both-backend evidence pass.

## Primary Source Routes

- `Engine/RHI/Public/Presentation`
- `Engine/RHI/Private/D3D12/SwapChain` and `Engine/RHI/Private/Vulkan/SwapChain`
- [Renderer Presentation and Output](../../Renderer/Features/PostProcessing/PresentationAndOutput.md)
