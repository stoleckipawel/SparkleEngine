# Renderer Chromatic Aberration

Status: feature dossier; current negative capability and source-backed absence, not a delivery plan

Verified: 2026-09-06 against source revision `d236da11`; inspected Renderer/shader/view paths are unchanged from the earlier `8414b5dc` audit

Scope: `REN-POST-12`; intentional wavelength/channel-dependent lens distortion as a post-processing effect

Parent family: [Post Processing](README.md)

## Current Capability

Chromatic aberration was not found. Sparkle has no per-view lens/aberration settings, selector, pass, shader, radial or spectral distortion model, center/falloff controls, edge sampling policy, history, diagnostic mode, or editor authoring surface.

Color fringes caused by reconstruction, filtering, motion, texture sampling, or display encoding are artifacts, not an implemented chromatic-aberration feature. No neighboring post stage may be advertised as this effect.

## Required Boundary Before A Future Claim

A future feature would need to define:

- its exact position relative to reconstruction, tone mapping, UI, output encoding, and capture;
- the lens/channel model, units, viewport center/aspect behavior, sampling/filtering, guard bands, alpha policy, and quality modes;
- per-camera/per-viewport settings, defaults, serialization, editor controls, requested/active diagnostics, and disable/identity behavior;
- extent/resize/history behavior, compute or graphics pass ownership, backend requirements, cost, and controlled failure;
- reference patterns for zero-strength identity, center stability, radial symmetry, edge behavior, temporal stability, and resolution independence.

Until those owners exist, the feature remains unavailable rather than Experimental.

## Evidence And Source Audit

- `REN-E27` owns the negative source/build/selector/pass/shader/asset/editor/documentation audit for chromatic aberration.
- No runtime test is implied by this source-only absence finding.
- Adjacent source routes inspected: [`PostProcessing.cpp`](../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp), [`Presentation.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp), and [`ViewportDisplaySettings.h`](../../../../../../../Engine/Renderer/Private/View/ViewportDisplaySettings.h).

### Current Negative Acceptance

- `AC-CHR-NEG-01` — no selector, view/camera setting, lens model, shader/pass, history, diagnostic, editor control, or package claim advertises chromatic aberration.
- `AC-CHR-NEG-02` — reconstruction, filtering, motion, sampling, and encoding fringes remain classified as defects/artifacts rather than intentional feature output.

`FM-CHR-NEG-01` occurs when a reachable aberration-like control/path lacks the complete owner/result contract or an artifact is advertised as support. `CHK-CHR-NEG-01`/`REN-E27` covers `AC-CHR-NEG-01`, `AC-CHR-NEG-02`, and `FM-CHR-NEG-01` by auditing source, CMake, shaders, settings, frame graph, view/editor UI, diagnostics, packages, and documentation; any unmatched result fails the negative contract.
