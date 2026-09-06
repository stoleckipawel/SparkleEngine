# Renderer Volumetric Lighting

Status: feature dossier; current negative capability and source-backed absence record, not a target architecture, delivery plan, or release approval

Verified: 2026-09-06 against committed `master` revision `d236da11`; inspected Renderer/shader/importer paths are unchanged from the earlier `8414b5dc` audit

Scope: `REN-VOL-01` through `REN-VOL-03`; explicitly defines what Sparkle does not currently implement for participating media, fog, atmospheric scattering, and volumetric composition

## Current State

Sparkle does not currently implement volumetric lighting.

The inspected Renderer frame graph, lighting targets, shader registrations, Renderer/RHI selectors, scene/GPU-scene payloads, and source/build membership contain no owned path for:

- homogeneous or heterogeneous participating media;
- local fog or volume primitives;
- extinction, absorption, scattering, or transmittance integration;
- phase-function evaluation;
- light injection into a froxel, voxel, or other volumetric representation;
- volumetric shadows or multiple scattering;
- atmospheric scattering, height fog, aerial perspective, or volumetric cloud lighting;
- temporal reconstruction/composition of a volumetric-lighting product.

There is consequently no volumetric render target, frame-graph pass, shader program, history, selector, diagnostic view, backend contract, content component, or acceptance evidence to advertise.

## Nearby Features That Are Not Volumetric Lighting

| Existing feature | Why it does not establish volumetric support |
| --- | --- |
| Sky background fill | Evaluates the environment for background pixels; it does not integrate scattering or transmittance along the camera ray. |
| Wrap subsurface lighting | Approximates a surface lobe from GBuffer material data; it does not transport light through a participating volume. |
| Alpha masking | Rejects surface-hit candidates; it is not partial transmittance or medium traversal. |
| Automatic exposure and bloom-like brightness perception | Operate on image values; they do not model a medium. |
| glTF material volume/transmission vocabulary | The current importer intentionally discards optional transmission and volume data, and the Renderer has no consuming material/transport path. |

## Required Ownership Before This Becomes A Feature

A future proposal must name, at minimum, the authored/world representation, cooked schema, render-scene/GPU-scene owner, view parameters, integration algorithm, direct and indirect coupling, shadow/transmittance contract, frame-graph resources and pass placement, temporal history/invalidation, raster/ray interaction, RHI/backend requirements, selectors and requested-versus-active reporting, diagnostics, limits, failure state, and acceptance oracle. Until those owners exist in code and evidence, Volumetric Lighting remains **Not implemented**, not Experimental or Partial.

## Evidence Boundary

`REN-E24` keeps the negative claim honest by auditing the public/editor/importer/runtime surface for implied fog, atmosphere, volume, transmission, or volumetric-lighting support. If any such selector or authored data becomes reachable, the inventory and this dossier must be updated before it can be advertised.

### Current Negative Acceptance

- `AC-VOL-NEG-01` — no selectable/imported/cooked world or material data is advertised as participating media, fog, atmosphere, or aerial perspective without an owned Renderer consumer.
- `AC-VOL-NEG-02` — no frame-graph resource/pass, shader registration, history, debug product, backend capability, or release claim implies volumetric integration.
- `AC-VOL-NEG-03` — sky, wrap subsurface, alpha masking, exposure, and discarded glTF transmission/volume vocabulary remain explicitly classified as non-volumetric.

| Failure mode | Required response | Check |
| --- | --- | --- |
| `FM-VOL-NEG-01` — a new selector/component/import field/program/pass/product becomes reachable | fail the negative contract; create/update capability rows and an owned target/completion contract before advertisement | `CHK-VOL-NEG-01` |
| `FM-VOL-NEG-02` — adjacent surface/image functionality is labeled volumetric | correct the claim and keep `REN-VOL-*` Not implemented | `CHK-VOL-NEG-01` |

`CHK-VOL-NEG-01` is `REN-E24`: search code, CMake, shaders, selectors/settings, importer/cooked data, scene/GPU-scene, frame graph, diagnostics, UI, package, and documentation for both accepted and rejected vocabulary. It covers `AC-VOL-NEG-01` through `AC-VOL-NEG-03` and `FM-VOL-NEG-01` through `FM-VOL-NEG-02`. The negative contract is defined but its audit result belongs in the candidate report.

## Inspected Source Routes

- [`BuildRenderFrameGraph.cpp`](../../../../../../../Engine/Renderer/Private/Frame/Graph/BuildRenderFrameGraph.cpp) contains no volumetric stage between lighting and presentation.
- [`Lighting.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Lighting.cpp) creates only the five surface-lighting lobes, composite/reference sample, sky, and optional reconstruction path.
- [`LightingRenderTargets.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/LightingRenderTargets.cpp) defines no volumetric product.
- [`ShaderRegistrations`](../../../../../../../Engine/Renderer/ShaderRegistrations) contains no volumetric/fog/atmosphere program registration.
- [`GltfMaterialImporter.cpp`](../../../../../../../Tools/Import/SourceImporters/Private/Gltf/GltfMaterialImporter.cpp) explicitly discards optional transmission and volume data under current product policy.
