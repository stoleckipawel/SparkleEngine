# Renderer Plans

**Status:** Renderer plan index

These plans are primarily owned by `Engine/Renderer`. Links name collaborating modules without obscuring the primary delivery owner.

Current feature baselines are **40/100** for existing debug views and **0/100** for deferred decals. Planned target work does not increase either score. See [Current Feature Readiness](../../Acceptance/CurrentReadiness.md#renderer).

For closure of all 26 Renderer families admitted to `v0.1.0`, use the [First Release Renderer Plans](../FirstRelease/Renderer/README.md). This folder retains focused feature/migration plans; deferred decals are explicitly selected by `GR-5`, while plan presence alone does not admit other work.

## Choose By Outcome

| Outcome | Current feature readiness | Current architecture state | Plan purpose |
| --- | ---: | --- | --- |
| trustworthy per-view debug presentation | **40/100** | debug modes exist but exact-display/HDR domain handling and viewport show flags are partial/target | order the clean break from process-global intent to typed per-view presentation |
| deferred GBuffer decals | **0/100** | current capability is absent; target composition design and feature-local acceptance exist | deliver authored data, scene/GPU publication, raster/ray composition, and evidence in bounded slices |

Plans do not upgrade either feature. Candidate results remain in release-level completion reports after the feature-local contract is executed.

## Plans

| Plan | Delivers | Architecture owner |
| --- | --- | --- |
| [Debug View Presentation](DebugViewPresentation.md) | per-view show flags, display mapping, editor controls, and capture metadata | [View Modes And Show Flags](../../Architecture/Modules/Engine/Renderer/Features/DebugViews/ViewModesAndShowFlags.md) and [Debug View Presentation Architecture](../../Architecture/Modules/Engine/Renderer/Features/DebugViews/PresentationArchitecture.md) |
| [Deferred GBuffer Decals](DeferredGBufferDecals.md) | decal authoring, scene/GPU data, raster composition, and ray integration | [Deferred Decal Composition Architecture](../../Architecture/Modules/Engine/Renderer/Features/DeferredDecals/CompositionArchitecture.md) |
