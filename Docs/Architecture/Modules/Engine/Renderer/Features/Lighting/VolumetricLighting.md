# Renderer Volumetric Lighting

Status: feature dossier; current negative capability and source-backed absence record, not a target architecture, delivery plan, or release approval

Verified: 2026-09-06 against committed `master` revision `8414b5dc`

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

## Inspected Source Routes

- [`BuildRenderFrameGraph.cpp`](../../../../../../../Engine/Renderer/Private/Frame/Graph/BuildRenderFrameGraph.cpp) contains no volumetric stage between lighting and presentation.
- [`Lighting.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Lighting.cpp) creates only the five surface-lighting lobes, composite/reference sample, sky, and optional reconstruction path.
- [`LightingRenderTargets.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/LightingRenderTargets.cpp) defines no volumetric product.
- [`ShaderRegistrations`](../../../../../../../Engine/Renderer/ShaderRegistrations) contains no volumetric/fog/atmosphere program registration.
- [`GltfMaterialImporter.cpp`](../../../../../../../Tools/Import/SourceImporters/Private/Gltf/GltfMaterialImporter.cpp) explicitly discards optional transmission and volume data under current product policy.
