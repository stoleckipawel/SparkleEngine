# Renderer Color Grading

Status: feature dossier; current negative capability and source-backed absence, not a delivery plan

Verified: 2026-09-06 against source revision `8414b5dc`

Scope: `REN-POST-11`; artistic and technical color adjustments distinct from exposure, tone mapping, and output encoding

Parent family: [Post Processing](README.md)

## Current Capability

Color grading was not found. Sparkle has no grading pass or shader, per-view grading settings, lift/gamma/gain or slope/offset/power controls, saturation/contrast/hue controls, 1D or 3D grading LUT asset/import/cook path, LUT blend stack, working/display gamut selection, or grading selector/debug product.

The three tone-mapper operators are fixed HDR-to-display mappings. They do not constitute an authored color-grading stack, and output Linear/sRGB encoding is a transfer-function choice rather than grading.

## Required Boundary Before A Future Claim

A future feature would need to define:

- scene-referred versus display-referred placement and its relationship to exposure and tone mapping;
- grading parameter/LUT authoring, import, cook, color-space metadata, interpolation, precision, and blend ownership;
- per-view overrides, requested/active state, defaults, serialization, and editor UX;
- shader/pass inputs and outputs, gamut and alpha behavior, backend requirements, diagnostics, and failure when a LUT or transform is invalid;
- numerical references, identity-transform proof, LUT edge cases, artifact fixtures, cost, and candidate release criteria.

Until those owners exist, documentation and UI must report color grading as unavailable and must not use tone-mapper choice as a substitute.

## Evidence And Source Audit

- `REN-E26` owns the negative source/build/selector/pass/shader/asset/editor/documentation audit for color grading.
- No runtime test is implied by this source-only absence finding.
- Adjacent source routes inspected: [`PostProcessing.cpp`](../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp), [`Presentation.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp), and [`EngineRenderingDisplayTypes.h`](../../../../../../../Engine/Renderer/Public/Settings/EngineRenderingDisplayTypes.h).
