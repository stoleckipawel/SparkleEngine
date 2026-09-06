# Renderer Frame Generation

Status: feature dossier; current negative capability and source-backed absence, not a delivery plan

Verified: 2026-09-06 against source revision `d236da11`; inspected Renderer/Streamline/frame paths are unchanged from the earlier `8414b5dc` audit

Scope: `REN-POST-13`; generation and presentation of interpolated frames between normally rendered frames

Parent family: [Post Processing](../README.md)

## Current Capability

Frame generation was not found. Sparkle has no optical-flow or frame-generation provider, DLSS-G feature registration/evaluation, generated-frame resource contract, generated-versus-rendered frame identity, UI composition policy, swapchain/interposer path, pacing scheduler, present-count model, or frame-generation selector/diagnostic.

The Streamline runtime registers DLSS Super Resolution, DLSS Ray Reconstruction, PCL, and Reflex. PCL/Reflex markers and sleep calls support latency coordination for rendered frames; they do not synthesize or present an additional frame. Temporal upscaling and Ray Reconstruction also reconstruct the current rendered frame and are not frame generation.

## Required Boundary Before A Future Claim

A future feature would need to define:

- provider capability, backend/device/driver/SDK/binary gates and requested-versus-active fallback truth;
- rendered, generated, simulation, input, UI, capture, frame-token, and present identity;
- motion/depth/color/optical-flow inputs, disocclusion policy, resize/cut/reset behavior, and resource lifetime;
- where UI is composed, whether it is reprojected or independently overlaid, and how editor viewport products behave;
- latency markers, pacing, queue/swapchain ownership, generated-frame failures, telemetry, screenshots/captures, and performance reporting;
- artifact, latency, stability, fallback, packaging, and backend acceptance workloads.

Until those owners exist, Sparkle must report frame generation as unavailable even when NVIDIA Streamline and Reflex support are present.

## Evidence And Source Audit

- `REN-E28` owns the negative provider/build/runtime/selector/frame-pacing/presentation/documentation audit.
- No runtime test is implied by this source-only absence finding.
- Adjacent source routes inspected: [`StreamlineRuntimeSupport.cpp`](../../../../../../../../Engine/Renderer/Private/Streamline/StreamlineRuntimeSupport.cpp), [`RendererImageProviderStack.cpp`](../../../../../../../../Engine/Renderer/Private/Providers/RendererImageProviderStack.cpp), and [`FramePipeline.cpp`](../../../../../../../../Engine/Renderer/Private/Frame/FramePipeline.cpp).

### Current Negative Acceptance

- `AC-FGN-NEG-01` — no provider registration/evaluation, generated-frame resource/identity, selector, pacing/present route, UI policy, diagnostic, or package claim advertises frame generation.
- `AC-FGN-NEG-02` — DLSS SR, DLSS RR, PCL, Reflex, temporal history, and ordinary multi-buffered presentation remain explicitly distinguished from synthesized frames.

`FM-FGN-NEG-01` occurs when a provider symbol/control or extra present-like path appears without generated-versus-rendered identity and complete ownership, or adjacent NVIDIA functionality is mislabeled. `CHK-FGN-NEG-01`/`REN-E28` covers `AC-FGN-NEG-01`, `AC-FGN-NEG-02`, and `FM-FGN-NEG-01` by auditing registrations, binaries, runtime support, providers, frame/present counting, UI/capture identity, settings, package surfaces, and documentation; any unmatched result fails the negative contract.
