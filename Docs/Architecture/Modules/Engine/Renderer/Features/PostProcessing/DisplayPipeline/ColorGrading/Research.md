# Color Grading Research

**Status:** research; primary-source precedent and discovery input, not SparkleEngine design authority, implementation proof, or acceptance evidence

**Responsibility:** compare current Sparkle source with revision-pinned color-grading, CDL, LUT, ownership, and validation precedent that can inform `CGRD-00`

**Authority boundary:** [Discovery](Discovery.md) owns local decisions; [Semantics](Semantics.md) owns the selected math; [Execution Architecture](ExecutionArchitecture.md) owns target system shape; [README](README.md) owns the bounded feature and proof contract

**Researched:** 2026-09-10; external repositories are pinned below, and mutable documentation is cited with its publisher

**Current readiness:** Not applicable — research adds no readiness credit. The feature remains **0/100** and `Blocked` in [Current Feature Readiness](../../../../../../../../Acceptance/CurrentReadiness.md#renderer).

## Research Questions

1. Which semantics are stable enough to adopt for slope/offset/power and saturation?
2. What must a safe, reproducible 3D LUT source/cooked/runtime contract record?
3. Where do mature renderers place grading state and work, and which choices are incompatible with Sparkle's admitted scene-referred target?
4. Which defects must the evidence protocol expose independently of the implementation?

## Research Method

This study uses four evidence classes and keeps them separate:

1. **Current Sparkle source** establishes what exists, which owner is live, and which seams are absent.
2. **Revision-pinned implementation source** supplies concrete precedent for math, parsing, lifetime, and test shape.
3. **Publisher documentation** explains the intended public contract where code alone is ambiguous.
4. **Security advisories and negative fixtures** identify hostile-input and capacity obligations that a happy-path renderer study would miss.

Every transferable finding below names a source ID. A source ID is precedent only; it becomes a Sparkle decision only when [Discovery](Discovery.md) records its disposition and the accepted semantic or architecture page incorporates it. Mutable `main` or product-marketing pages are not used as implementation or conformance proof.

## Source Ledger

| ID | Primary source and frozen identity | Question answered | Transfer limit |
| --- | --- | --- | --- |
| `CGR-REF-OCIO-01` | OpenColorIO `CDLTransform` / `CDLStyle`, commit `5a808fb57a94c7229640a97835c420c9a1fbd1fe` | which SOP styles and negative-value choices exist? | precedent for explicit behavior, not an OCIO dependency or local working-space answer |
| `CGR-REF-OCIO-02` | OpenColorIO `FileFormatIridasCube.cpp`, same commit | how does a mature `.cube` reader tokenize directives, dimensions, domains, and samples? | parser audit input only; Sparkle must freeze a narrower grammar and its own limits |
| `CGR-REF-OCIO-03` | OpenColorIO `.cube` CPU tests, same commit | which valid and malformed cases can seed independent fixtures? | test-shape precedent; external tests and expected values do not become Sparkle evidence |
| `CGR-REF-SEC-01` | OpenColorIO advisory `GHSA-28jr-x9w2-5pc4` | what can go wrong when text tokens are not bounded? | security obligation only; it does not establish that Sparkle is currently vulnerable |
| `CGR-REF-FIL-01` | Filament `ColorGrading.h`, commit `5a90ed9838a8a7115120fe6887d21b138d21c39d` | what state, defaults, quality, and View relationship are public? | View/resource precedent; not Sparkle feature scope |
| `CGR-REF-FIL-02` | Filament `ColorGrading.cpp`, same commit | how are ordering, LUT creation, resource lifetime, and precision made concrete? | ownership/cost precedent; Filament output-domain placement is not adopted |
| `CGR-REF-UNITY-01` | Unity Graphics `ColorLookup.cs`, commit `a7e4c051d256a781ab362c64316b125a1e104694` | which shape and activation checks precede LUT use? | validation precedent; not a volume-system requirement |
| `CGR-REF-UNITY-02` | Unity Graphics `ColorGradingLutPass.cs`, same commit | how are authored settings, generated resources, and application separated? | pipeline-shape precedent; not proof of Sparkle ordering or correctness |

## Completion Vocabulary

| Term | Meaning in this package |
| --- | --- |
| `source LUT` | the author-provided file bytes plus logical asset identity; never loaded directly by a packaged rendering pass |
| `canonical LUT` | the parser result after grammar, count, finite-value, dimension, and domain validation |
| `cooked LUT generation` | deterministic serialized bytes with semantic schema, source-content identity, provenance, and declared resource requirements |
| `resident LUT generation` | one immutable GPU resource plus descriptor and completion/retirement identity owned by residency |
| `requested grade` | the author or manifest intent for one View, whether or not its dependencies are ready |
| `active grade` | the exact resolved immutable generation used to produce the current graded product |
| `neutral` | exact identity parameters with no active LUT; requires pass omission rather than an identity substitute |
| `independent oracle` | a CPU evaluator/parser path that does not reuse production shader helpers, packed layouts, or GPU sampling |
| `proved` | a claim backed by the predeclared check, oracle, candidate identity, observation, and artifact; source presence alone is not proof |

## Local Baseline

Sparkle's current post path has explicit reconstruction/debug, tone-map, encoding, and publication seams but no grade. State types expose tone, exposure, and encoding only; generic texture cooking has no `.cube` semantic parser. This makes a clean feature possible, but the present labels do not establish working primaries, the exposure/grade relation, or a LUT contract.

The absence is useful: no legacy grade, LUT cache, selector, or compatibility representation needs preservation. The implementation should extend existing View settings, frame-graph, shader-registration, asset-cook, residency, editor, and package owners rather than introduce a color-management subsystem.

## Current Sparkle Source Trace

| Surface | Current source truth | Consequence for discovery |
| --- | --- | --- |
| frame ordering | [`PostProcessing.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp) applies exposure before reconstruction and routes reconstructed scene color through debug/presentation; it has no grade edge | `CGRD-02/03` must freeze the new grade edge rather than infer it from a nonexistent pass |
| tone and encoding | [`Presentation.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp) produces separately named `ToneMappedSceneColor` and `EncodedSceneColor` products | preserve separate scene grade, target tone/gamut, and output encoding authorities |
| public display state | [`EngineRenderingDisplayTypes.h`](../../../../../../../../../Engine/Renderer/Public/Settings/EngineRenderingDisplayTypes.h) exposes tone, exposure, and encoding vocabulary only | add the smallest grade contract; do not imply current support |
| settings persistence | [`EngineRenderingSettings.h`](../../../../../../../../../Engine/Renderer/Public/Settings/EngineRenderingSettings.h) is the existing display-settings route | extend it once; no grade singleton or shadow config |
| per-view selection | [`ViewportContracts.h`](../../../../../../../../../Engine/Renderer/Public/View/ViewportContracts.h) has no grade request/result | discovery must establish global/default versus per-view precedence and identity |
| assets | the repository has generic texture/cook routes but no `.cube` semantic source/cooked contract | parser, deterministic cook, dependency discovery, and package reachability are first-class work |

This trace was verified at revision `30597d7d`. It is a source snapshot, not a guarantee that concurrently changing user-owned files remain unchanged during later implementation.

## Primary-Source Findings

### OpenColorIO

OpenColorIO's `CDLTransform` implements ASC CDL-style slope/offset/power and documents different negative-value behavior for configuration versions and CDL styles.[^1] Its API distinguishes clamp and no-clamp styles, so the phrase “ASC CDL” alone does not settle Sparkle's negative/HDR behavior.[^2]

OpenColorIO treats LUT files as transforms with parsing, interpolation, cache, path, and metadata concerns rather than as an arbitrary texture.[^3] This supports a semantic asset type and strict source/cooked identity. It does not require Sparkle to embed OpenColorIO, accept every format, or inherit its global cache model.

The pinned Iridas `.cube` reader and its CPU tests expose concrete lexical and structural concerns: title/domain/LUT-size directives, exactly counted table data, 1D-versus-3D distinctions, comments, and rejected malformed forms.[^8][^9] They are useful for building an explicit grammar and hostile corpus, but not a specification that Sparkle should copy wholesale.

The OpenColorIO project disclosed a stack-buffer overflow in its `.cube` parsing path for versions through 2.5.1, caused by unbounded token scanning; the advisory records 2.5.2 as the patched release.[^10] Sparkle does not currently have a `.cube` parser, so this is not a claim of a local vulnerability. It is direct evidence that source parsing must use bounded tokens, checked arithmetic, input/line/token ceilings, deterministic errors, and no allocation or publication before structural validation.

**Permitted transfer:** explicit transform style; strict metadata/domain parsing; independent CPU reference; provenance and invalidation.

**Forbidden inference:** OpenColorIO support, full `.cube` compatibility, ACES compliance, or correct local working-space semantics.

### Google Filament

Filament associates a `ColorGrading` object with a View, publishes explicit neutral defaults, defines transform ordering, and varies generated 3D-LUT dimension/precision by quality.[^4] The implementation generates LUT data under an engine-owned resource lifetime and applies ordered CDL, saturation, tone/gamut, transfer, and optional custom-LUT work.[^5]

This is strong precedent for immutable grade generations, explicit order, bounded dimension/precision, and one View-facing selection. Filament's custom LUT is documented as an LDR/sRGB operation after its output transfer, while Sparkle's admitted LUT is scene-referred before target tone mapping. That placement is a deliberate non-transfer.

**Permitted transfer:** View association; neutral defaults; generated-resource lifetime; explicit ordered pipeline; quality/memory accounting.

**Forbidden inference:** copying Filament's full control surface, combining grading and tone mapping into one Sparkle authority, or using its LDR custom-LUT placement.

### Unity Graphics

Unity URP validates lookup-texture dimensions against the configured LUT size and makes activation depend on both nonzero contribution and valid texture shape.[^6] Its LUT generation and post-process paths keep authored settings, generated LUT resources, and application stages separately visible.[^7]

The repository also retains cross-backend/editor/player reference-image coverage for HDR/LDR neutral and mixed grading scenes. Those images are evidence for Unity's candidate, not portable Sparkle goldens, but their matrix is useful precedent for separating domain, backend, profile, and neutral/non-neutral cases.

**Permitted transfer:** pre-activation validation; separate authored/generated/applied identities; asymmetric known-cell fixtures; matrix-based image review.

**Forbidden inference:** Unity volume blending, 2D strip layout, exact control ranges, reference images, or local pass correctness.

## Comparison

| Concern | OpenColorIO | Filament | Unity Graphics | Sparkle discovery implication |
| --- | --- | --- | --- | --- |
| SOP negative behavior | explicit style/version choice | implementation-specific pipeline | pipeline-specific | `CGRD-04` must choose; “CDL” is insufficient |
| State owner | transform/config graph | View references an engine resource | volume/profile state | use View-owned selection with immutable runtime generation; no volume system |
| LUT identity | file transform plus metadata/cache | generated 3D texture and optional custom data | generated LUT plus lookup texture | define source, cooked, runtime, and active identities separately |
| Placement | configurable transform graph | grading/tone/output largely co-generated | pipeline/profile dependent | preserve Sparkle's separate grade/tone/output owners and freeze exact domain |
| Invalid data | parser/transform rejection | builder validation/assertions | activation validation | transactionally reject; never silently label identity fallback active |
| Evidence | processor/reference comparisons | unit/internal renderer checks | cross-profile reference scenes | use independent CPU math, known cells, raw stage captures, and backend/package matrix |

## Recommended Discovery Direction

These are evidence-based recommendations, not accepted decisions:

- Keep a narrow first release: one global parametric grade plus one optional 3D LUT generation, no runtime OCIO and no volume/blend framework.
- Treat `.cube` as a typed source asset whose parser emits a canonical checked representation; do not upload an opaque generic texture and reconstruct semantics in the shader.
- Preserve separate scene-grade, target tone/gamut, and output-encoding authorities. A fused shader optimization may be considered only after independent products and semantics are proved.
- Use a View-owned immutable grade selection that references a residency-owned LUT generation; generation changes must not reset unrelated temporal history.
- Freeze negative/HDR handling, working primaries, exposure relation, LUT axis/domain/interpolation, and artifact meaning before code.

## Initial Missing-And-Unknown Ledger

| ID | Unknown at this revision | Why source/research does not answer it | Required closure |
| --- | --- | --- | --- |
| `CGR-U-01` | exact scene-working primaries and white convention at the grade edge | current labels do not encode this contract | targeted shader/resource trace plus `CGRD-02` decision |
| `CGR-U-02` | exposure before/after grade and exact unit meaning of offset | precedent differs by pipeline | analytic-order probe plus `CGRD-03` |
| `CGR-U-03` | clamp, signed, or rejection behavior for negative powered values | all are plausible and externally precedented | CPU hand cases plus `CGRD-04` |
| `CGR-U-04` | admitted `.cube` lexical grammar and directive subset | `.cube` behavior is ecosystem convention, not one normative standard adopted here | grammar/corpus review plus `CGRD-06` |
| `CGR-U-05` | axis order, texel-center mapping, interpolation, and storage precision | APIs and engines choose differently | asymmetric-cell GPU/CPU experiment plus `CGRD-07` |
| `CGR-U-06` | parameter/LUT composition order and whether a contribution blend exists | external pipelines vary | explicit product decision plus `CGRD-08` |
| `CGR-U-07` | per-view selection, residency join, and replacement retirement owner | current source has no grade generation | owner trace plus `CGRD-09/10` |
| `CGR-U-08` | maximum source bytes, line/token length, LUT dimension, persistent bytes, and live-edit high-water | research exposes hazards but cannot choose Sparkle budgets | measured budget experiment plus `CGRD-12` |
| `CGR-U-09` | raw capture format/tolerance and backend equivalence oracle | screenshots lose stage/precision information | defect-control trial plus evidence decision |
| `CGR-U-10` | licenses/provenance for any future copied fixture or formula | research only references external material | file-level rights ledger before adoption |

No unknown in this ledger may be silently resolved during a production stage. The accepted `CGRD-00` revision either closes it, explicitly excludes the affected surface, or remains `Blocked` with an owner and next experiment.

## Oracle Ladder

Evidence should escalate from the cheapest semantic falsifier to the broadest product claim:

1. checked parser/serializer probes over valid, malformed, hostile, and deterministic-recook fixtures;
2. double-precision CPU hand cases for SOP, saturation, domain mapping, and trilinear interpolation;
3. shader compilation/reflection and typed-binding checks for both backends;
4. GPU raw pre-grade/post-grade buffer comparisons against the independent CPU oracle;
5. seeded axis, half-texel, order, luma-weight, negative-rule, stale-generation, and double-application defects;
6. frame-graph/product/capture checks for stage placement, identity omission, alpha, two views, resize, and replacement;
7. packaged-runtime dependency and clean-machine operation for the exact candidate;
8. end-to-end SDR/HDR visual review only after the numeric and stage products pass.

A later rung cannot repair a failed earlier semantic claim. A pleasing final image does not prove LUT order, and matching D3D12/Vulkan output does not prove that both are correct.

## Research Handoff

Research is sufficient to enter discovery when the source IDs above are reproducible, each local unknown maps to a decision/experiment, and no source is being treated as local proof. Discovery must return:

- accepted or rejected transfer for every recommendation;
- one frozen semantic revision and feature/support matrix;
- parser/resource/cook budgets and hostile-input policy;
- ownership, lifetime, publication, invalidation, and clean-break decisions;
- an oracle/fixture/defect-control manifest with candidate-independent expected results;
- a file-level rights disposition for anything proposed for inclusion.

Until that handoff passes, this page supports planning only and earns no implementation, runtime, or release readiness.

## Rights And Provenance

The sources are used as precedent and validation ideas. Any copied formula, parser behavior, code structure, test data, or LUT asset needs a file-level license and provenance review before implementation. No external source or artifact is proposed for direct inclusion by this research.

## Sources

[^1]: Academy Software Foundation, OpenColorIO, [`CDLTransform` API at `5a808fb57a94c7229640a97835c420c9a1fbd1fe`](https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/5a808fb57a94c7229640a97835c420c9a1fbd1fe/include/OpenColorIO/OpenColorTransforms.h), accessed 2026-09-10.
[^2]: Academy Software Foundation, OpenColorIO, [`CDLStyle` definitions at `5a808fb57a94c7229640a97835c420c9a1fbd1fe`](https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/5a808fb57a94c7229640a97835c420c9a1fbd1fe/include/OpenColorIO/OpenColorTypes.h), accessed 2026-09-10.
[^3]: Academy Software Foundation, OpenColorIO, [`FileTransform` and configuration APIs at `5a808fb57a94c7229640a97835c420c9a1fbd1fe`](https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/5a808fb57a94c7229640a97835c420c9a1fbd1fe/include/OpenColorIO/OpenColorIO.h), accessed 2026-09-10.
[^4]: Google, Filament, [`ColorGrading` contract at `5a90ed9838a8a7115120fe6887d21b138d21c39d`](https://github.com/google/filament/blob/5a90ed9838a8a7115120fe6887d21b138d21c39d/filament/include/filament/ColorGrading.h), accessed 2026-09-10.
[^5]: Google, Filament, [`ColorGrading` implementation at `5a90ed9838a8a7115120fe6887d21b138d21c39d`](https://github.com/google/filament/blob/5a90ed9838a8a7115120fe6887d21b138d21c39d/filament/src/details/ColorGrading.cpp), accessed 2026-09-10.
[^6]: Unity Technologies, Graphics, [`ColorLookup` validation at `a7e4c051d256a781ab362c64316b125a1e104694`](https://github.com/Unity-Technologies/Graphics/blob/a7e4c051d256a781ab362c64316b125a1e104694/Packages/com.unity.render-pipelines.universal/Runtime/Overrides/ColorLookup.cs), accessed 2026-09-10.
[^7]: Unity Technologies, Graphics, [`ColorGradingLutPass` at `a7e4c051d256a781ab362c64316b125a1e104694`](https://github.com/Unity-Technologies/Graphics/blob/a7e4c051d256a781ab362c64316b125a1e104694/Packages/com.unity.render-pipelines.universal/Runtime/Passes/ColorGradingLutPass.cs), accessed 2026-09-10.
[^8]: Academy Software Foundation, OpenColorIO, [Iridas `.cube` reader at `5a808fb57a94c7229640a97835c420c9a1fbd1fe`](https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/5a808fb57a94c7229640a97835c420c9a1fbd1fe/src/OpenColorIO/fileformats/FileFormatIridasCube.cpp), accessed 2026-09-10.
[^9]: Academy Software Foundation, OpenColorIO, [Iridas `.cube` CPU tests at `5a808fb57a94c7229640a97835c420c9a1fbd1fe`](https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/5a808fb57a94c7229640a97835c420c9a1fbd1fe/tests/cpu/fileformats/FileFormatIridasCube_tests.cpp), accessed 2026-09-10.
[^10]: OpenColorIO project, [stack-buffer-overflow advisory `GHSA-28jr-x9w2-5pc4`](https://github.com/AcademySoftwareFoundation/OpenColorIO/security/advisories/GHSA-28jr-x9w2-5pc4), published 2026-04-09 and accessed 2026-09-10.
