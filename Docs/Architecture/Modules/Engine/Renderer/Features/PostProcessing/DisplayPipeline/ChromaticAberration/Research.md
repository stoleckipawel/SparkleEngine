# Chromatic Aberration Research

**Status:** research; revision-pinned implementation precedent and product comparison, not SparkleEngine design or proof

**Responsibility:** identify transferable and rejected choices for a bounded real-time lens-fringe effect and its validation

**Authority boundary:** [Discovery](Discovery.md) owns local decisions; [Semantics](Semantics.md) owns accepted equations; [README](README.md) owns scope and acceptance

**Researched:** 2026-09-10

**Current readiness:** Not applicable — research adds no readiness credit.

## Local Baseline

Sparkle has a clean output-resolution seam between tone mapping and encoding but no lens state or pass. The most valuable external evidence is therefore not feature count; it is how mature engines bound controls, omit zero work, keep sampling in bounds, and distinguish a cheap RGB effect from a higher-sample spectral approximation.

## Unity HDRP

Unity HDRP's current `ChromaticAberration` component exposes an intensity, optional spectral LUT, and a sample-count quality control; `IsActive` is strictly intensity greater than zero.[^1] The Uber post shader computes a radial endpoint, derives sample count from displacement length, samples along the segment, and weights samples with a spectral LUT under clamp sampling.[^2]

This is useful precedent for zero-work activation, radial/aspect/output-size reasoning, bounded sample count, and explicit quality cost. It is not the selected Sparkle algorithm: the first-release candidate intentionally seeks one fixed low-cost RGB model without a spectral LUT or quality tiers.

## Unity PostProcessing v2

The archived official package exposes intensity and a fast-mode choice, and combines the effect in an Uber post stage.[^3] Its documentation describes color fringing as lens simulation and warns that higher-quality spectral sampling costs more.[^4]

This reinforces two decisions that must remain visible: quality is an algorithm choice, and combining passes is an optimization shape rather than semantic authority. Sparkle should first prove a distinct stage and only consider fusion after raw products and boundaries remain observable.

## Unreal Engine Product Precedent

Unreal's official post-process documentation exposes **Scene Fringe Intensity** and **Chromatic Aberration Start Offset**, with the latter defining where the effect begins relative to the image center.[^5] This is good product vocabulary for a compact authored surface, but the documentation does not specify Sparkle's shader equation, pixel units, aspect normalization, edge policy, or backend behavior.

## Comparison

| Concern | Unity HDRP | Unity PostProcessing v2 | Unreal documentation | Sparkle implication |
| --- | --- | --- | --- | --- |
| activation | intensity > 0 | intensity/fast mode | authored intensity | zero must omit work exactly |
| model | multi-sample spectral weighting | quality-dependent Uber effect | intensity plus start offset | Stage 0 must explicitly choose the simpler RGB model |
| cost | sample count varies | fast versus regular | not specified | one fixed budget and no quality tier for first release |
| edge behavior | clamp sampler in Uber shader | implementation-specific | not specified | make clamp/texel-center/bounds contractual |
| author units | normalized intensity | normalized intensity | intensity/start offset | Sparkle's 1080-line pixel strength is local policy requiring its own proof |
| evidence | source and engine tests | package tests/docs | product docs | use analytic CPU displacement, raw stage captures, and backend/package checks |

## Recommended Discovery Direction

- Keep the admitted model small: fixed three-channel sampling, one strength, center, and start offset; no spectral LUT or sample-count setting.
- Express strength in output pixels referenced to 1080 lines and verify displacement, rather than borrowing an opaque engine-specific intensity scale.
- Normalize radial geometry deliberately across aspect ratios and viewport subrects; never infer aspect correctness from a fullscreen screenshot.
- Use a distinct pass and raw pre/post products for initial proof. Fusion is a later performance decision with unchanged semantics.
- Preserve exact debug products and UI by explicit classification, not by relying on current ordering accidents.

## Rights And Non-Claims

The sources inform decisions only. No shader code, spectral LUT, reference image, or control range is approved for copying. Any future transfer needs a file-level license/provenance review. Agreement with Unity or Unreal output would not prove Sparkle's local units or correctness.

## Sources

[^1]: Unity Technologies, Graphics, [`ChromaticAberration` component at `a7e4c051d256a781ab362c64316b125a1e104694`](https://github.com/Unity-Technologies/Graphics/blob/a7e4c051d256a781ab362c64316b125a1e104694/Packages/com.unity.render-pipelines.high-definition/Runtime/PostProcessing/Components/ChromaticAberration.cs), accessed 2026-09-10.
[^2]: Unity Technologies, Graphics, [`UberPost.compute` at `a7e4c051d256a781ab362c64316b125a1e104694`](https://github.com/Unity-Technologies/Graphics/blob/a7e4c051d256a781ab362c64316b125a1e104694/Packages/com.unity.render-pipelines.high-definition/Runtime/PostProcessing/Shaders/UberPost.compute), accessed 2026-09-10.
[^3]: Unity Technologies, PostProcessing, [`ChromaticAberration` component and renderer at `32c3155207d3fac4138b24af5c7e0e43805c6f2b`](https://github.com/Unity-Technologies/PostProcessing/blob/32c3155207d3fac4138b24af5c7e0e43805c6f2b/PostProcessing/Runtime/Effects/ChromaticAberration.cs), accessed 2026-09-10.
[^4]: Unity Technologies, PostProcessing, [`Chromatic Aberration documentation at `32c3155207d3fac4138b24af5c7e0e43805c6f2b`](https://github.com/Unity-Technologies/PostProcessing/blob/32c3155207d3fac4138b24af5c7e0e43805c6f2b/Documentation~/Chromatic-Aberration.md), accessed 2026-09-10.
[^5]: Epic Games, [Post Process Effects — Chromatic Aberration](https://dev.epicgames.com/documentation/unreal-engine/post-process-effects-in-unreal-engine#chromaticaberration), accessed 2026-09-10.

