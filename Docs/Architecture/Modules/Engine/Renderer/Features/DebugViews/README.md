# Debug Views

**Status:** feature dossier; source-present mode migration, executable validation deferred

**Current readiness:** **40/100**. The single per-view source route is present in the current changelist, but build, runtime, visual, backend, and release evidence is not claimed.

**Architecture:** [Render View Modes](ViewModes.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

**Delivery:** [Plan](Plan.md) and [Acceptance](Acceptance.md)

## Current Source Shape

Sparkle has one host-independent `RenderViewMode` contract:

- Lit is value `0`;
- Reference Path Tracer is value `1`;
- Wireframe is value `2`;
- GBuffer, lighting, and GPU-scene inspection modes occupy contiguous values `3` through `16`;
- `Count` is `17`.

The ordinary `ViewportRenderRequest` carries one selected mode. `RenderViewBuilder` freezes it into `RenderView`; frame composition, raster GBuffer, and debug resolve consume that same value at their owning decisions. The HLSL constant contract mirrors the C++ values for shader consumers.

Editor uses the same enum directly while retaining presentation ownership in `ViewportTopPanel`. Game/runtime viewport owners may submit the same rendering semantic without importing Editor. RHI remains unaware.

The former process-global visualization selection, Editor mirror enum/preset resolver, proposed visualization-target/show-flag split, and Reference selector CVar are clean-break deletions. They are not compatibility routes.

The debug resolve is partitioned into GBuffer, lighting, and GPU-scene families. Each family privately owns its activation predicate, early return, pass parameters, and shader; scene-level composition simply invokes the family entry points in order. Adding or reordering an enum value cannot activate a family by ordinal accident.

## Current Modes

| Capability | Modes | Current producer |
| --- | --- | --- |
| `REN-DBG-01` final/material | Lit, Wireframe | Ordinary frame; Wireframe changes raster fill |
| `REN-DBG-02` GBuffer | Diffuse, Normal, Roughness, Metallic, Emissive, Ambient Occlusion, Subsurface Color, Subsurface Strength | `GBufferVisualizationCS` reads only GBuffer products |
| `REN-DBG-03` lighting | Direct Diffuse, Direct Specular, Direct Subsurface, Indirect Diffuse, Indirect Specular | `LightingVisualizationCS` reads lighting lobes plus GBuffer alpha |
| `REN-DBG-04` scene | GPU Scene Instances | instance identity generation plus `GpuSceneVisualizationCS` |
| `REN-LGT-04` reference | Reference Path Tracer | private Reference middle/session with Editor menu and operational overlay; source-present, not executable-proved |

## Presentation Route

Sparkle now classifies each mode once in the private presentation owner. Scene-referred HDR views retain exposure, the selected tone curve, and the configured super-resolution provider. Display-linear exact views bypass exposure/tone mapping, use point reconstruction when render and output extents differ, and still receive output transfer encoding. Visualization shaders no longer contain a local HDR preview curve. Reference Path Tracer bypasses the independent Lit Ray Reconstruction denoiser without overriding the selected presentation upscaler or quality.

This is source presence, not pixel proof. The acceptance route must still exercise isolation, mode selection, both display domains, extent changes, backends, and Reference topology.

## Ownership

- Renderer Public owns only the generic enum and ordinary request field.
- Renderer Private owns View freezing and focused consumers.
- The private Reference Path Tracer folder owns its estimator, session, identity, resources, and passes.
- Editor owns menu presentation and selection interaction.
- RHI owns no mode or feature identity.

Any new mode must have a real production consumer. Any future independently selectable show control must be orthogonal to the selected mode and land with that consumer; it cannot recreate the removed parallel taxonomy.
