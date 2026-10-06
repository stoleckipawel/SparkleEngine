# Debug Views

**Status:** feature dossier; Show-flags implementation complete, family acceptance remains bounded

**Current readiness:** **40/100** for the family; no family/release closure is claimed. Bounded delivery, admission and direct/indirect-family execution results are recorded by [Discovery](Done/Discovery.md#current-candidate-evidence-and-permission), not inferred for the remaining mode/control matrix.

**Responsibility:** feature-local navigation and current evidence posture for Renderer debug-view controls, presentation, delivery, and acceptance

**Verified:** Show delivery at `82528cbe5edbcf729464fbefe6998435730920eb` on 2026-10-06, accepted by the user after bounded serial/threaded D3D12 controls checks and DXIL/SPIR-V shader cooking. Historical mode and lighting baselines retain their original evidence boundaries.

**Architecture:** [Viewport Rendering Controls](Controls/README.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

**Current proof contract:** [Acceptance](Acceptance.md). [Completed delivery records](Done/README.md) retain the plan and discovery history.

**Show delivery:** complete for the implemented feature set. The compact grouped menu, globally editable controls, initialized disabled contributions, scene/light classifications, and shader-parameter emissive control are implemented. Future controls extend their feature owner and the current [Show contract](Controls/ShowFlags.md). Historical delivery prompts are archived, not queued. This does not advance whole-family or release readiness.

## Current Source Shape

Sparkle has one host-independent `RenderViewMode` contract:

- Lit is value `0`;
- Reference Path Tracer is value `1`;
- Wireframe is value `2`;
- GBuffer, lighting, and GPU-scene inspection modes occupy contiguous values `3` through `17`;
- `Count` is `18`.

The ordinary `ViewportRenderRequest` carries one selected mode. `RenderViewBuilder` freezes it into `RenderView`; frame composition, raster GBuffer, and debug resolve consume that same value at their owning decisions. The HLSL constant contract mirrors the C++ values for shader consumers.

Editor uses the same enum directly while retaining presentation ownership in `ViewportTopPanel`. Game/runtime viewport owners may submit the same rendering semantic without importing Editor. RHI remains unaware.

The former process-global visualization selection, Editor mirror enum/preset resolver, proposed visualization-target/show-flag split, and Reference selector CVar are clean-break deletions. They are not compatibility routes.

The debug resolve is partitioned into GBuffer, lighting, and GPU-scene families. Each family privately owns its activation predicate, early return, pass parameters, and shader; scene-level composition simply invokes the family entry points in order. Adding or reordering an enum value cannot activate a family by ordinal accident.

## Current Modes

| Capability | Modes | Current producer |
| --- | --- | --- |
| `REN-DBG-01` final/material | Lit, Wireframe | Ordinary frame; Wireframe changes raster fill |
| `REN-DBG-02` GBuffer | Diffuse, World Normal, World Tangent, Roughness, Metallic, Emissive, Ambient Occlusion, Subsurface Color, Subsurface Strength | `GBufferVisualizationCS` reads only GBuffer products; World Normal and World Tangent map signed world-space XYZ to display-linear RGB in `[0, 1]` without changing axes or orientation |
| `REN-DBG-03` lighting | Direct Diffuse, Direct Specular, Direct Subsurface, Indirect Diffuse, Indirect Specular | one `LightingVisualizationCS` reads five initialized lighting products plus GBuffer alpha; the View uniform selects the lobe; disabled direct and indirect raw modes report unavailable |
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

The [Show menu](Controls/ShowFlags.md#editor-interaction) presents fifteen feature-owned CVars: Sky, Static Meshes, Skinned Meshes, Emissive, four analytic light types, five lighting lobes, and two shadow controls. Open Show beside Viewmode to edit leaves, bulk groups, or defaults; console edits refresh its checks. Sky controls background and environment illumination independently of analytic lights. Emissive uses ordinary shader parameters while retaining authored material data. There is no Renderer Show set or request/View transport. Decals and Indirect Subsurface are not advertised because they have no implemented producer. No measured GPU savings or release result is claimed.
