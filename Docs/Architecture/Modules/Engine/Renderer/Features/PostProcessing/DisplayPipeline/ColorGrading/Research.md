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

## Local Baseline

Sparkle's current post path has explicit reconstruction/debug, tone-map, encoding, and publication seams but no grade. State types expose tone, exposure, and encoding only; generic texture cooking has no `.cube` semantic parser. This makes a clean feature possible, but the present labels do not establish working primaries, the exposure/grade relation, or a LUT contract.

The absence is useful: no legacy grade, LUT cache, selector, or compatibility representation needs preservation. The implementation should extend existing View settings, frame-graph, shader-registration, asset-cook, residency, editor, and package owners rather than introduce a color-management subsystem.

## Primary-Source Findings

### OpenColorIO

OpenColorIO's `CDLTransform` implements ASC CDL-style slope/offset/power and documents different negative-value behavior for configuration versions and CDL styles.[^1] Its API distinguishes clamp and no-clamp styles, so the phrase “ASC CDL” alone does not settle Sparkle's negative/HDR behavior.[^2]

OpenColorIO treats LUT files as transforms with parsing, interpolation, cache, path, and metadata concerns rather than as an arbitrary texture.[^3] This supports a semantic asset type and strict source/cooked identity. It does not require Sparkle to embed OpenColorIO, accept every format, or inherit its global cache model.

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

