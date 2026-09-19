# Debug View Presentation Architecture

**Status:** source-present architecture; executable validation deferred

**Responsibility:** debug-view signal domains, display mapping, output encoding, and producer requirements

## Decision

Every `RenderViewMode` has one presentation domain: scene-referred HDR or display-linear exact. Both domains use one display-mapping owner and one output-encoding owner.

The selected mode is already the complete stock policy. Presentation does not need a second target enum, show-flag set, or Editor preset translation. The presentation owner resolves the selected mode to its signal domain and applies the corresponding mapping.

## Domains

### Scene-Referred HDR

The producer publishes linear scene radiance or a lighting contribution. The common display mapper applies the current scene exposure and selected tone curve exactly once. Lit, Reference Path Tracer, emissive, and direct/indirect lighting views belong here.

### Display-Linear Exact

The producer publishes a bounded diagnostic value in linear display space. Exposure and the tone curve are bypassed; output transfer encoding still runs exactly once. Scalar material values, encoded normals, material colors, and stable instance palettes belong here.

“Exact” does not mean writing linear numbers into an encoded target. It means no content-dependent exposure or filmic curve changes the producer-authored display-linear value before the required output transfer.

## Route

```text
RenderView::viewMode
        |
        v
selected producer-domain color
        |
        v
DisplayMappingPass(mode domain, exposure, tone mapper)
        |
        v
DisplayLinearColor
        |
        v
OutputEncodingPass
        |
        v
viewport / back buffer
```

The presentation resolver is Renderer-private and exhaustive over `RenderViewMode`. It owns only signal-domain classification. Editor does not duplicate this table, and shaders do not infer it independently.

## Mode Classification

| Mode | Domain | Producer requirement |
| --- | --- | --- |
| Lit | Scene-referred HDR | composed linear scene color |
| Reference Path Tracer | Scene-referred HDR | Reference display derivative from raw accumulation |
| Wireframe | Scene-referred HDR | current Lit shading with raster wireframe fill |
| GBufferDiffuse | Display-linear exact | saturated linear base color |
| GBufferNormal | Display-linear exact | normalized normal mapped from `[-1, 1]` to `[0, 1]` |
| GBufferRoughness | Display-linear exact | bounded scalar replicated to RGB |
| GBufferMetallic | Display-linear exact | bounded scalar replicated to RGB |
| GBufferEmissive | Scene-referred HDR | raw non-negative emissive value; no local preview curve |
| GBufferAmbientOcclusion | Display-linear exact | bounded scalar replicated to RGB |
| GBufferSubsurfaceColor | Display-linear exact | saturated linear material color |
| GBufferSubsurfaceStrength | Display-linear exact | bounded scalar replicated to RGB |
| DirectDiffuse | Scene-referred HDR | raw non-negative contribution |
| DirectSpecular | Scene-referred HDR | raw non-negative contribution |
| DirectSubsurface | Scene-referred HDR | raw non-negative contribution |
| IndirectDiffuse | Scene-referred HDR | raw non-negative contribution |
| IndirectSpecular | Scene-referred HDR | raw non-negative contribution |
| GpuSceneInstances | Display-linear exact | stable hashed instance palette |

## Producer And Extent Rules

The GBuffer, lighting, and GPU-scene visualization families independently produce their selected diagnostic color. Each family privately resolves its mode and binds only its own inputs; the scene-level composition has no activation logic and does not infer family membership from enum ordering. The family shaders output raw HDR for scene-referred modes and one intentional bounded mapping for exact modes. They contain no exposure or tone-mapper policy.

When render and output extents differ, exact views use an explicit point selection so reconstruction does not invent category IDs, material values, or false colors. The producer must not assume source and destination extents match.

The existing exposure owner continues to meter the ordinary Lit scene even while an exact diagnostic is visible. Exact presentation ignores the resulting exposure value, but keeping history warm prevents an unrelated adaptation reset when returning to Lit.

## Optional Independent Controls

Stock presentation is derived from `RenderViewMode`; no presentation flag is required. A future expert override for exposure or tone mapping is a separate product decision and may be added only with a real user workflow, explicit Custom state, reset behavior, capture provenance, and a dedicated consumer. It must not change the meaning or identity of the selected mode.

## Capture

Renderer/RHI capture transport remains neutral. A higher-level evidence record may join completed product identity with the selected mode, resolved signal domain, exposure/tone-mapper application, exposure value, output encoding, extent, and any explicit expert override. This metadata does not belong in the RHI readback contract.

## Rejected Alternatives

- Keep a producer-local HDR preview curve: rejected because it hides magnitude and creates double mapping.
- Bypass all presentation for exact views: rejected because output transfer encoding is still required.
- Add mode-shaped show flags or a visualization target: rejected because the selected mode already owns this stock policy.
- Add a process-global “debug views bypass tone mapping” CVar: rejected because it cannot describe independent viewports and makes evidence ambiguous.

## Evidence Boundary

The source route implements this architecture. Numeric fixed-value checks, extent cases, dual-viewport isolation, output encoding, shader cook, D3D12/Vulkan execution, and captured pixels remain required evidence under [Acceptance](Acceptance.md).

