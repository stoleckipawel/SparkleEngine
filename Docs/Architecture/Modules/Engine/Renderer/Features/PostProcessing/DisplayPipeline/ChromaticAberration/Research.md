# Chromatic Aberration Research

**Status:** research; revision-pinned implementation precedent and product comparison, not SparkleEngine design or proof

**Responsibility:** identify transferable and rejected choices for a bounded real-time lens-fringe effect and its validation

**Authority boundary:** [Discovery](Discovery.md) owns local decisions; [Semantics](Semantics.md) owns accepted equations; [README](README.md) owns scope and acceptance

**Researched:** 2026-09-10

**Current readiness:** Not applicable — research adds no readiness credit.

## Research Decision

Primary sources support entering discovery with two materially different model families: fixed three-channel displacement and variable/multi-sample spectral approximation. They do **not** choose Sparkle's model, coordinate units, target-linear domain, start-offset curve, or error budget. Those remain `CHRD-00` decisions.

## Research Questions

1. Which low-cost lens models are concrete enough to compare rather than describe aesthetically?
2. Which stage, extent, aspect, edge, activation, and UI decisions recur across mature implementations?
3. What authoring surface is useful without admitting a spectral/profile framework?
4. Which numeric and image fixtures can distinguish intentional output from incidental renderer fringing?
5. Which external algorithms/assets could create license or provenance obligations if transferred?

## Research Method

Current Sparkle source establishes absence and live seams. Revision-pinned AMD and Unity code establishes concrete implementation precedent; publisher manuals establish intended placement/cost/product vocabulary; Unreal documentation supplies product-surface precedent only. Each finding is tagged with a source ID and a transfer limit. External output, screenshots, tests, or sample assets are never reused as local acceptance evidence.

## Source Ledger

| ID | Primary source | Question answered | Transfer limit |
| --- | --- | --- | --- |
| `CHR-REF-AMD-01` | AMD FidelityFX SDK Lens shader header at tag `v1.1.4`, commit `c6efa6bf7f2027b3ec94f28578bb5965eabb9e55` | how does a fixed RGB wavelength-inspired model compute channel-dependent magnification? | algorithm comparison only; no code/formula transfer without license review |
| `CHR-REF-AMD-02` | AMD Lens sample module, same tag | what settings/resources/dispatch lifecycle does the sample expose? | source-shape precedent, not Sparkle ownership or UX |
| `CHR-REF-AMD-03` | AMD FidelityFX Lens manuals | where is the pass intended relative to upscaling/UI and which APIs/resolution relation are supported? | stage/cost precedent, not proof of Sparkle backend or domain |
| `CHR-REF-UNITY-01` | Unity HDRP `ChromaticAberration.cs`, commit `a7e4c051d256a781ab362c64316b125a1e104694` | how are intensity, spectral LUT, quality, and activation exposed? | comparison; spectral LUT/quality are excluded candidates |
| `CHR-REF-UNITY-02` | Unity HDRP `UberPost.compute`, same commit | how does a multi-sample radial/spectral implementation bound work and sample edges? | implementation precedent, not local equation or oracle |
| `CHR-REF-UNITY-03` | Unity PostProcessing v2 component/docs, commit `32c3155207d3fac4138b24af5c7e0e43805c6f2b` | what older fast/quality product tradeoff is exposed? | product comparison only |
| `CHR-REF-UE-01` | Epic post-process documentation | which compact intensity/start-offset vocabulary is user-visible? | mutable product documentation; no shader/unit/backend inference |

## Completion Vocabulary

| Term | Meaning here |
| --- | --- |
| `three-channel model` | one independently displaced filtered sample for each output R/G/B component; not spectral integration |
| `multi-sample spectral approximation` | multiple radial samples weighted by a spectral lookup/curve; still not physical wavelength transport |
| `target-linear input` | the explicitly named SDR or HDR display-target domain after target tone/gamut work and before transfer encoding |
| `reference-height strength` | authored pixel displacement scaled from an output height of 1080; a local candidate requiring proof |
| `intentional fringe` | output produced by the named active lens model and carrying matching stage/View/settings identity |
| `incidental fringe` | channel/color error from reconstruction, filtering, motion, sampling, encoding, or capture with no admitted active model |
| `identity` | exact no-pass/no-resource output at neutral strength, not approximate visual equality |

## Local Baseline

Sparkle has a clean output-resolution seam between tone mapping and encoding but no lens state or pass. The most valuable external evidence is therefore not feature count; it is how mature engines bound controls, omit zero work, keep sampling in bounds, and distinguish a cheap RGB effect from a higher-sample spectral approximation.

## Current Sparkle Source Trace

| Surface | Current truth at `ca55e7d8` | Research consequence |
| --- | --- | --- |
| post/display ordering | [`PostProcessing.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp) reaches debug then presentation; [`Presentation.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp) owns tone-mapped and encoded products | a candidate edge exists, but HDR/SDR domains and exact debug/UI/capture policy are not defined by absence |
| display types/settings | [`EngineRenderingDisplayTypes.h`](../../../../../../../../../Engine/Renderer/Public/Settings/EngineRenderingDisplayTypes.h) and [`EngineRenderingSettings.h`](../../../../../../../../../Engine/Renderer/Public/Settings/EngineRenderingSettings.h) contain no lens controls | extend one current settings path; do not infer a global post-process framework |
| per-view state | [`ViewportDisplaySettings.h`](../../../../../../../../../Engine/Renderer/Private/View/ViewportDisplaySettings.h) has no aberration request/result | discovery must freeze default/override, active truth, and extent identity |
| shader/pass/build | no matching shader, registered program, graph pass, or build member was found by the negative audit | all implementation and backend claims are absent |
| assets/history | no spectral LUT, lens profile, or effect history owner exists | a three-sample first release can remain stateless and asset-free |
| editor/package | no control, manifest field, or package claim exists | workflow/reachability must be planned as part of the feature, not inferred later |

## Unity HDRP

Unity HDRP's current `ChromaticAberration` component exposes an intensity, optional spectral LUT, and a sample-count quality control; `IsActive` is strictly intensity greater than zero.[^1] The Uber post shader computes a radial endpoint, derives sample count from displacement length, samples along the segment, and weights samples with a spectral LUT under clamp sampling.[^2]

This is useful precedent for zero-work activation, radial/aspect/output-size reasoning, bounded sample count, and explicit quality cost. It is not the selected Sparkle algorithm: the first-release candidate intentionally seeks one fixed low-cost RGB model without a spectral LUT or quality tiers.

## Unity PostProcessing v2

The archived official package exposes intensity and a fast-mode choice, and combines the effect in an Uber post stage.[^3] Its documentation describes color fringing as lens simulation and warns that higher-quality spectral sampling costs more.[^4]

This reinforces two decisions that must remain visible: quality is an algorithm choice, and combining passes is an optimization shape rather than semantic authority. Sparkle should first prove a distinct stage and only consider fusion after raw products and boundaries remain observable.

## AMD FidelityFX Lens

The pinned FidelityFX Lens shader source uses fixed red, green, and blue wavelength constants and a Cauchy-inspired wavelength-dependent magnification to choose separate channel sample positions.[^6] This is especially useful because it is a compact three-channel alternative to Unity's variable spectral accumulation: “three samples” still requires an explicit optical/empirical displacement model rather than arbitrary channel offsets.

The pinned sample module shows the effect as a separately dispatched resource-to-resource stage with settings carried into a small constant buffer and explicit source/output resources.[^7] AMD's manuals place Lens after upscalers and before UI, describe source and output as the same resolution, and document both DirectX 12 and Vulkan sample support.[^8][^9] That aligns with Sparkle's proposed output-resolution/UI boundary and paired-backend evidence shape. It does not establish Sparkle's target-linear domain, View ownership, pixel units, or acceptable image quality.

The main discovery value is a falsifiable comparison:

- candidate A: symmetric fixed channel scales such as `(+1,0,-1)` with a local radial falloff;
- candidate B: fixed wavelength/Cauchy-inspired magnification with no spectral LUT;
- excluded comparison C: variable multi-sample spectral accumulation.

Stage 0 should compare all three at identical maximum displacement and patterns. The selected model must win against predeclared quality, stability, complexity, and cost criteria—not familiarity.

## Unreal Engine Product Precedent

Unreal's official post-process documentation exposes **Scene Fringe Intensity** and **Chromatic Aberration Start Offset**, with the latter defining where the effect begins relative to the image center.[^5] This is good product vocabulary for a compact authored surface, but the documentation does not specify Sparkle's shader equation, pixel units, aspect normalization, edge policy, or backend behavior.

## Comparison

| Concern | AMD FidelityFX Lens | Unity HDRP | Unity PostProcessing v2 | Unreal documentation | Sparkle implication |
| --- | --- | --- | --- | --- | --- |
| activation | effect dispatch controlled by sample | intensity > 0 | intensity/fast mode | authored intensity | Sparkle must centralize exact-zero omission itself |
| model | fixed RGB wavelength/Cauchy-inspired magnification | multi-sample spectral weighting | quality-dependent Uber effect | intensity plus start offset | Stage 0 must compare explicit fixed models against spectral reference |
| cost | fixed small shader/resource stage | sample count varies | fast versus regular | not specified | one fixed budget and no quality tier for first release |
| placement | after upscalers, before UI; same resolution | Uber post pipeline | Uber post pipeline | post process | output-resolution/pre-UI is precedented but domain/order remain local |
| edge behavior | inspect pinned shader/resource contract | clamp sampler in Uber shader | implementation-specific | not specified | make clamp/texel-center/bounds contractual |
| author units | dispersion/distortion parameters | normalized intensity | normalized intensity | intensity/start offset | Sparkle's 1080-line pixel strength is local policy requiring its own proof |
| evidence | sample/source/manual | source and engine tests | package tests/docs | product docs | use analytic CPU displacement, raw stage captures, and backend/package checks |

## Recommended Discovery Direction

- Keep the admitted model small: fixed three-channel sampling, one strength, center, and start offset; no spectral LUT or sample-count setting.
- Express strength in output pixels referenced to 1080 lines and verify displacement, rather than borrowing an opaque engine-specific intensity scale.
- Normalize radial geometry deliberately across aspect ratios and viewport subrects; never infer aspect correctness from a fullscreen screenshot.
- Use a distinct pass and raw pre/post products for initial proof. Fusion is a later performance decision with unchanged semantics.
- Preserve exact debug products and UI by explicit classification, not by relying on current ordering accidents.

## Initial Missing-And-Unknown Ledger

| ID | Unknown | Why precedent cannot answer it | Closure |
| --- | --- | --- | --- |
| `CHR-U-01` | simple symmetric versus AMD wavelength-inspired fixed model | both satisfy a low sample count but produce different displacement | `CHR-EXP-02`, quality/cost thresholds, `CHRD-01/05` |
| `CHR-U-02` | exact SDR/HDR target-linear domains and tone/gamut/encoding edges | external pipeline ordering is not Sparkle's color contract | source/domain trace and `CHRD-02` |
| `CHR-U-03` | center/subrect/aspect/radius convention | precedent varies and product docs omit equations | analytic geometry tables and `CHRD-03` |
| `CHR-U-04` | reference-height strength sign/range and start falloff | local policy has no external normative value | pattern/displacement study and `CHRD-04` |
| `CHR-U-05` | texel-center, bilinear, clamp, maximum displacement, and odd/zero extent | shader/API defaults are not a semantic oracle | boundary study and `CHRD-06` |
| `CHR-U-06` | invalid-value clamp versus reject and requested/active vocabulary | product choice, not determined by shader code | workflow dry run and `CHRD-07` |
| `CHR-U-07` | debug mode classification and raw artifact contract | current ordering does not define future intent | owner review and `CHRD-08` |
| `CHR-U-08` | compute/graphics/fusion route, time/transient thresholds | hardware and current graph shape must be measured locally | cost model and `CHRD-09` |
| `CHR-U-09` | numeric/filter/image tolerances and incidental-fringe classifier | external images are candidate-specific | seeded-defect evidence dry run and `CHRD-10` |
| `CHR-U-10` | whether any AMD/Unity formula or fixture is transferred | research references are not inclusion approval | file-level rights/provenance decision |

## Oracle Ladder

1. binary64 coordinate/displacement/filter hand cases;
2. synthetic CPU image generator over impulse, line, checker, channel, alpha, edge, and gradient fixtures;
3. model-comparison metrics at matched maximum displacement and sample budget;
4. shader compile/reflection and typed-binding checks on both backend routes;
5. raw GPU pre/post product comparison with known patterns and seeded semantic defects;
6. graph/debug/UI/capture product and exact-zero omission traces;
7. two-view, resize/minimize, invalid, missing-pipeline, and device-recovery state checks;
8. paired-backend/package/performance/artifact-classification evidence;
9. subjective review only as a bounded product-quality supplement after analytic claims pass.

Agreement between backends is not correctness. A final screenshot cannot replace coordinate, filter, stage, alpha, or active-state evidence.

## Research Handoff

Discovery receives the source ledger, current-source trace, three model families, unknown ledger, recommended direction, oracle ladder, and rights constraints. It returns one accepted model/equation/range/domain/owner/workflow/budget/evidence revision or remains blocked. Any source transfer must name exact file/revision/license, copied idea/code/data, modification, destination, and reviewer before implementation.

## Rights And Non-Claims

The sources inform decisions only. No shader code, spectral LUT, reference image, or control range is approved for copying. Any future transfer needs a file-level license/provenance review. Agreement with Unity or Unreal output would not prove Sparkle's local units or correctness.

## Sources

[^1]: Unity Technologies, Graphics, [`ChromaticAberration` component at `a7e4c051d256a781ab362c64316b125a1e104694`](https://github.com/Unity-Technologies/Graphics/blob/a7e4c051d256a781ab362c64316b125a1e104694/Packages/com.unity.render-pipelines.high-definition/Runtime/PostProcessing/Components/ChromaticAberration.cs), accessed 2026-09-10.
[^2]: Unity Technologies, Graphics, [`UberPost.compute` at `a7e4c051d256a781ab362c64316b125a1e104694`](https://github.com/Unity-Technologies/Graphics/blob/a7e4c051d256a781ab362c64316b125a1e104694/Packages/com.unity.render-pipelines.high-definition/Runtime/PostProcessing/Shaders/UberPost.compute), accessed 2026-09-10.
[^3]: Unity Technologies, PostProcessing, [`ChromaticAberration` component and renderer at `32c3155207d3fac4138b24af5c7e0e43805c6f2b`](https://github.com/Unity-Technologies/PostProcessing/blob/32c3155207d3fac4138b24af5c7e0e43805c6f2b/PostProcessing/Runtime/Effects/ChromaticAberration.cs), accessed 2026-09-10.
[^4]: Unity Technologies, PostProcessing, [`Chromatic Aberration documentation at `32c3155207d3fac4138b24af5c7e0e43805c6f2b`](https://github.com/Unity-Technologies/PostProcessing/blob/32c3155207d3fac4138b24af5c7e0e43805c6f2b/Documentation~/Chromatic-Aberration.md), accessed 2026-09-10.
[^5]: Epic Games, [Post Process Effects — Chromatic Aberration](https://dev.epicgames.com/documentation/unreal-engine/post-process-effects-in-unreal-engine#chromaticaberration), accessed 2026-09-10.
[^6]: AMD, FidelityFX SDK, [`ffx_lens.h` at tag `v1.1.4` / commit `c6efa6bf7f2027b3ec94f28578bb5965eabb9e55`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/c6efa6bf7f2027b3ec94f28578bb5965eabb9e55/sdk/include/FidelityFX/gpu/lens/ffx_lens.h), accessed 2026-09-10.
[^7]: AMD, FidelityFX SDK, [Lens sample render module at tag `v1.1.4` / commit `c6efa6bf7f2027b3ec94f28578bb5965eabb9e55`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/c6efa6bf7f2027b3ec94f28578bb5965eabb9e55/samples/lens/lensrendermodule.cpp), accessed 2026-09-10.
[^8]: AMD GPUOpen, [FidelityFX SDK Lens technique manual](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/lens/), accessed 2026-09-10.
[^9]: AMD GPUOpen, [FidelityFX SDK Lens sample manual](https://gpuopen.com/manuals/fidelityfx_sdk/samples/lens/), accessed 2026-09-10.
