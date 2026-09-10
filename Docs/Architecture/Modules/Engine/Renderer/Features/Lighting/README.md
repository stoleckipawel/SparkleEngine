# Renderer Lighting

**Status:** Renderer lighting feature-family index; source-backed, not numerical, convergence, visual, performance, or release evidence

**Verified:** 2026-09-06 against committed `master` revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

**Responsibility:** define the shared lighting boundary and route Direct, Indirect, Volumetric, and Reference Path Tracer lighting without treating them as one undifferentiated capability

**Current readiness:** **28/100** across described lighting capabilities — direct/indirect surface paths are **45/100**, the Reference Path Tracer is **20/100**, and volumetric lighting is **0/100**. See [Current Feature Readiness](../../../../../../Acceptance/CurrentReadiness.md#renderer).

## At A Glance

| What Sparkle currently has | What remains absent or blocked |
| --- | --- |
| Ray-dependent direct lighting for directional, point, spot, and rectangular lights | Shadow-map or other fully non-ray direct-light fallback |
| ReSTIR direct/indirect products plus a per-view Reference Path Tracer session contract | Reference transport, accumulation, and accepted numerical, convergence, temporal, visual, or performance proof |
| Five separate scene-linear lobe products joined with emissive and sky | Participating-media/volumetric lighting |
| Debug access to direct and indirect lobe products | Exact presentation for every diagnostic lobe |
| A defined Reference Path Tracer target and discovery gate | An authorized, implemented, independent reference oracle |

The most important design choice is product separation: direct diffuse, direct specular, direct subsurface, indirect diffuse, and indirect specular remain distinct until one composite. That improves diagnosis and comparison, at the cost of more resources, histories, bandwidth, and synchronization.

## Lighting Taxonomy

| Domain | Current result | State | Owning dossier |
| --- | --- | --- | --- |
| Direct lighting | Direct diffuse, direct specular, and direct subsurface radiance from directional, point, spot, and rect lights with ray-traced visibility. | Implemented, capability-gated; executable correctness and limits remain unproved. | [Direct Lighting](DirectLighting.md) |
| Indirect lighting | Indirect diffuse and indirect specular radiance from ReSTIR reuse, plus the environment sky/background boundary. | Implemented, capability-gated; convergence, bias, and history behavior remain unproved. | [Indirect Lighting](IndirectLighting.md) |
| Volumetric lighting | Participating media, fog volumes, extinction, in-scattering, transmittance, atmospheric scattering, and aerial perspective. | Not implemented in the inspected Renderer. | [Volumetric Lighting](VolumetricLighting.md) |
| Reference Path Tracer | A bounded, deterministic, independently defined transport oracle implemented as a specialized route inside the existing per-view frame architecture rather than reuse of the interactive GBuffer surface. | Stage 1 contract-only and deliberately unavailable; estimator, accumulation, UI, parity, and oracle proof remain later work. | [Reference Path Tracer](ReferencePathTracer/README.md) and [`PTD-00` discovery](ReferencePathTracer/Discovery.md) |

This classification is semantic, not merely a source-folder preference. A lighting feature belongs to one domain according to the transport result it produces. Shared material evaluation, history, composite, and presentation remain common infrastructure and are not copied into three implementations.

## Shared Lighting Contract

The ordinary surface-lighting producer is ReSTIR: direct reservoir temporal/spatial reuse, selected-light visibility, indirect reservoir reuse/resolve, and the shared five-lobe composite. It requires ray-tracing capability; Sparkle currently has no shadow-map, lightmap, probe-only, or non-ray deferred-lighting fallback.

`RenderViewMode::ReferencePathTracer` is the sole reference semantic and enters the same `FramePipeline`, Scene, View, frame-graph, and RHI architecture through a per-view request. Stage 1 carries only the dormant semantic to one Private feature owner and leaves the ordinary Lit presentation visible with a generic unavailable observation; it does not invent a session identity, refusal taxonomy, or silently call that presentation reference output. Later stages add the independent camera-ray transport as a specialized frame route rather than a separate renderer. [Reference Path Tracer](ReferencePathTracer/README.md) owns its completion contract and staged implementation.

The ordinary ReSTIR route writes five semantic lobe products. The Reference Path Tracer target must preserve compatible raw comparison semantics where the frozen estimator requires them, but Stage 1 does not yet produce these textures:

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
- ReSTIR histories are invalidated by the scene/view/settings/extent/topology identities relevant to that algorithm. Stage 1 does not reuse that history validity or manufacture Reference Path Tracer generations; Stage 2 must establish the feature-local immutable Scene/View leases and reasoned reset identity.
- `LightingComposite` and `Sky` rejoin the selected producer into scene-linear color before exposure and presentation.
- Graph and provider generations retire only after their last queue submissions complete.

## Selection, Failure, And Evidence

- Invalid GBuffer or traversal selectors fail graph construction rather than selecting an arbitrary route. Invalid Reference Path Tracer requests reject before allocation through their per-view session contract.
- Absence of required ray capability prevents ordinary surface lighting and also rejects Reference Path Tracer activation; Stage 1 additionally reports transport unavailable even when hardware capabilities exist.
- Direct-shadow strict traversal cannot silently substitute; Automatic may choose only a documented supported frontend and must report the resolved choice.
- Debug views expose the five lobe products, but current presentation can modify them through exposure, tone mapping, and encoding.
- Exact selectors and persistence live in [Feature Selector Catalog](../RuntimeConfiguration/FeatureSelectorCatalog.md). Row-level states live in the [Capability Inventory](../../CapabilityInventory.md). Release proof remains in `REN-E06` through `REN-E10`, `REN-E18`, and the dedicated volumetric absence check `REN-E24`.

The ordinary lighting family is complete only when [Direct Lighting](DirectLighting.md#acceptance-criteria) and [Indirect Lighting](IndirectLighting.md#acceptance-criteria) pass, the common composite/sky join preserves their documented scene-linear products, and active capability limitations remain visible. [Volumetric Lighting](VolumetricLighting.md) and [Reference Path Tracer](ReferencePathTracer/README.md) retain independent negative/blocked dispositions and cannot inherit that verdict.

## Primary Source Route

- [`Lighting.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Lighting.cpp) owns ordinary target creation, ReSTIR producer dispatch, composite, sky, and reconstruction placement.
- [`LightingRenderTargets.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/LightingRenderTargets.cpp) owns the five ordinary-lighting lobe products.
- [`LightingComposite.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/LightingComposite.cpp) owns the direct/indirect/emissive join.
- [`Sky.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Sky/Sky.cpp) owns background sky fill, not volumetric transport.
