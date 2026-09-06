# Renderer Lighting

Status: Renderer lighting feature-family index; source-backed, not numerical, convergence, visual, performance, or release evidence

Verified: 2026-09-06 against committed `master` revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Responsibility: define the shared lighting boundary and route Direct, Indirect, Volumetric, and offline-reference lighting without treating them as one undifferentiated capability

## Lighting Taxonomy

| Domain | Current result | State | Owning dossier |
| --- | --- | --- | --- |
| Direct lighting | Direct diffuse, direct specular, and direct subsurface radiance from directional, point, spot, and rect lights with ray-traced visibility. | Implemented, capability-gated; executable correctness and limits remain unproved. | [Direct Lighting](DirectLighting.md) |
| Indirect lighting | Indirect diffuse and indirect specular radiance from either ReSTIR reuse or the accumulating reference path, plus the environment sky/background boundary. | Implemented, capability-gated; convergence, bias, history, and oracle status remain unproved. | [Indirect Lighting](IndirectLighting.md) |
| Volumetric lighting | Participating media, fog volumes, extinction, in-scattering, transmittance, atmospheric scattering, and aerial perspective. | Not implemented in the inspected Renderer. | [Volumetric Lighting](VolumetricLighting.md) |
| Offline reference path tracer | A bounded, deterministic, independently defined transport oracle rather than reuse of the interactive GBuffer/lighting path. | Discovery blocked; implementation is not authorized and current `ReferencePathTraced` is not this oracle. | [Offline Path Tracer](OfflinePathTracer/README.md) and [`PTD-00` discovery](OfflinePathTracer/Discovery.md) |

This classification is semantic, not merely a source-folder preference. A lighting feature belongs to one domain according to the transport result it produces. Shared material evaluation, history, composite, and presentation remain common infrastructure and are not copied into three implementations.

## Shared Lighting Contract

`r.Lighting.Mode` selects one complete surface-lighting producer:

| Mode | Direct domain | Indirect domain | Precision and traversal |
| --- | --- | --- | --- |
| ReSTIR path-traced | Direct reservoir temporal/spatial reuse, selected-light visibility, and surface resolve. | Indirect reservoir temporal/spatial reuse and inline-ray resolve. | `R16G16B16A16_Float`; direct visibility can resolve to Inline or Pipeline, indirect remains Inline. |
| Reference path-traced | One inline path-traced direct sample. | One inline path-traced indirect sample followed by history accumulation. | Lighting lobes and reference sample use `R32G32B32A32_Float`; candidate comparison path, not an accepted independent oracle. |

Both modes require ray-tracing capability. Sparkle currently has no shadow-map, lightmap, probe-only, or non-ray deferred-lighting fallback.

The current `ReferencePathTraced` branch is only a candidate comparison path. [Offline Path Tracer](OfflinePathTracer/README.md) owns the eventual feature definition and completion contract, while its [Discovery gate](OfflinePathTracer/Discovery.md) must settle transport scope, independence, estimator, and evidence design before implementation planning.

Both surface-lighting modes write the same five semantic lobe products:

```text
DirectDiffuse + DirectSpecular + DirectSubsurface
IndirectDiffuse + IndirectSpecular
                         |
                         v
              LightingComposite + Emissive
                         |
                         v
                 Sky background fill
                         |
                         v
                     SceneColor
```

The composite owns the join point. Direct and indirect producers do not independently tone-map, encode, present, or own a second scene-color path. Volumetric composition has no slot in this graph today and must not be implied by the sky or subsurface paths.

## Shared Ownership And Lifetime

- The persistent render scene and GPU scene own light records, material/geometry bindings, and sky resources.
- The frame-local prepared scene and view own visible light/surface state, camera identity, motion, and history inputs.
- `LightingRenderTargets` owns the five lobe textures for the selected graph generation.
- ReSTIR and reference histories are invalidated by the scene/view/settings/extent/topology identities relevant to their algorithms.
- `LightingComposite` and `Sky` rejoin the selected producer into scene-linear color before exposure and presentation.
- Graph and provider generations retire only after their last queue submissions complete.

## Selection, Failure, And Evidence

- Invalid lighting-mode values fail graph construction rather than selecting an arbitrary mode.
- Absence of required ray capability means neither current surface-lighting mode can truthfully activate.
- Direct-shadow strict traversal cannot silently substitute; Automatic may choose only a documented supported frontend and must report the resolved choice.
- Debug views expose the five lobe products, but current presentation can modify them through exposure, tone mapping, and encoding.
- Exact selectors and persistence live in [Feature Selectors](../FeatureSelectors.md). Row-level states live in the [Capability Inventory](../../CapabilityInventory.md). Release proof remains in `REN-E06` through `REN-E10`, `REN-E18`, and the dedicated volumetric absence check `REN-E24`.

The family is complete only when the selected surface-lighting mode passes both [Direct Lighting](DirectLighting.md#acceptance-criteria) and [Indirect Lighting](IndirectLighting.md#acceptance-criteria), the common composite/sky join preserves their documented scene-linear products, and the active mode/capability limitations remain visible. [Volumetric Lighting](VolumetricLighting.md) and [Offline Path Tracer](OfflinePathTracer/README.md) retain independent negative/blocked dispositions and cannot inherit that verdict.

## Primary Source Route

- [`Lighting.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Lighting.cpp) owns mode selection, target creation, producer dispatch, composite, sky, and reconstruction placement.
- [`LightingRenderTargets.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/LightingRenderTargets.cpp) owns the five lobe products and mode-dependent format.
- [`LightingComposite.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/LightingComposite.cpp) owns the direct/indirect/emissive join.
- [`Sky.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Sky/Sky.cpp) owns background sky fill, not volumetric transport.
