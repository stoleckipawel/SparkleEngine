# Renderer Lighting

**Status:** Renderer lighting feature-family index; source-backed, not numerical, convergence, visual, performance, or release evidence

**Verified:** 2026-09-13 against revision `709e04385c3d98aa3492fb3f09dc9caabe34cb3a`; Direct, Indirect, and Volumetric package depth was rechecked without renderer/runtime execution, while concurrent Reference Path Tracer production work remained outside this pass

**Responsibility:** define the shared lighting boundary and route Direct, Indirect, Volumetric, and Reference Path Tracer lighting without treating them as one undifferentiated capability

**Current readiness:** **29/100** across described lighting capabilities — direct/indirect surface paths are **45/100**, the Reference Path Tracer is **25/100**, and volumetric lighting is **0/100**. See [Current Feature Readiness](../../../../../../Acceptance/CurrentReadiness.md#renderer).

## At A Glance

| What Sparkle currently has | What remains absent or blocked |
| --- | --- |
| Ray-dependent direct lighting for directional, point, spot, and rectangular lights | Shadow-map or other fully non-ray direct-light fallback |
| Source-present direct and indirect reservoir products plus a per-view Reference Path Tracer seam | Direct ReSTIR conformance, indirect path-resampling conformance, reference transport, and accepted numerical, convergence, temporal, visual, or performance proof |
| Five separate scene-linear lobe products joined with emissive and sky | Participating-media/volumetric lighting |
| Debug access to direct and indirect lobe products | Exact presentation for every diagnostic lobe |
| Deep target packages for Direct, Indirect/ReSTIR GI, Volumetric/ReSTIR, fog, atmosphere, and sky | An authorized, implemented, independent reference oracle; post-`REL-11` admission for volumetric production |

Each of the three critical lighting packages now uses the full seven-role high-assurance route: dossier, discovery, primary research, mathematical semantics, execution architecture, user/automation experience, and staged plan. The extra depth changes no capability or evidence score.

The most important design choice is product separation: direct diffuse, direct specular, direct subsurface, indirect diffuse, and indirect specular remain distinct until one composite. That improves diagnosis and comparison, at the cost of more resources, histories, bandwidth, and synchronization.

## Lighting Taxonomy

| Domain | Current result | State | Owning dossier |
| --- | --- | --- | --- |
| Direct lighting | Direct diffuse, direct specular, and direct subsurface radiance from directional, point, spot, and rect lights with ray-traced visibility. | Source-present and capability-gated; current ReSTIR conformance, executable correctness, and limits remain unproved. | [Direct Lighting](DirectLighting/README.md) |
| Indirect lighting | Indirect diffuse and indirect specular radiance from a seed-replay reservoir prototype, plus the environment sky/background boundary. | Source-present and capability-gated; no ReSTIR GI/GRIS conformance, convergence, bias, or history proof exists. | [Indirect Lighting and ReSTIR GI](IndirectLighting/README.md) |
| Volumetric lighting | Target domain for participating media, fog, volumetric ReSTIR, physical atmosphere/sky, aerial perspective, heterogeneous volumes, and clouds. | Not implemented; production is excluded until `REL-11` closes and roadmap admission plus `VOL-D0` pass. | [Volumetric Lighting, Fog, Atmosphere, and Sky](VolumetricLighting/README.md) |
| Reference Path Tracer | A bounded, deterministic, independently defined transport oracle implemented as a specialized route inside the existing per-view frame architecture rather than reuse of the interactive GBuffer surface. | Stage 1 contract-only and deliberately unavailable; estimator, accumulation, UI, parity, and oracle proof remain later work. | [Reference Path Tracer](ReferencePathTracer/README.md) and [`PTD-00` discovery](ReferencePathTracer/Discovery.md) |

This classification is semantic, not merely a source-folder preference. A lighting feature belongs to one domain according to the transport result it produces. Shared material evaluation, history, composite, and presentation remain common infrastructure and are not copied into three implementations.

## Shared Lighting Contract

The ordinary surface-lighting producer is named ReSTIR in source: direct reservoir temporal/spatial reuse, selected-light visibility, indirect seed-reservoir reuse/resolve, and the shared five-lobe composite. Direct ReSTIR DI conformance and indirect ReSTIR GI/GRIS conformance are not established. The route requires ray-tracing capability; Sparkle currently has no shadow-map, lightmap, probe-only, or non-ray deferred-lighting fallback.

`r.ReferencePathTracer` is the sole Renderer selector for the reference lighting setup. `FramePipeline::BuildRenderFrameGraph` reads it once to choose the mutually exclusive Lit or feature-local Reference middle while retaining the same Scene, View, frame-graph, RHI, viewport, and presentation shell. Editor may present that selector as its ordered **Reference Path Tracer** view mode, but the UI enum, labels, and ordering do not enter Renderer or RHI contracts. [Reference Path Tracer](ReferencePathTracer/README.md) owns its completion contract and staged implementation.

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

The ordinary surface-lighting family is complete only when [Direct Lighting](DirectLighting/README.md#acceptance-criteria) and [Indirect Lighting](IndirectLighting/README.md#acceptance-criteria) pass, the common composite/sky join preserves their documented scene-linear products, and active capability limitations remain visible. [Volumetric Lighting](VolumetricLighting/README.md) and [Reference Path Tracer](ReferencePathTracer/README.md) retain independent negative/blocked dispositions and cannot inherit that verdict.

## Primary Source Route

- [`Lighting.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Lighting.cpp) owns ordinary target creation, ReSTIR producer dispatch, composite, sky, and reconstruction placement.
- [`LightingRenderTargets.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/LightingRenderTargets.cpp) owns the five ordinary-lighting lobe products.
- [`LightingComposite.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/LightingComposite.cpp) owns the direct/indirect/emissive join.
- [`Sky.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Sky/Sky.cpp) owns background sky fill, not volumetric transport.
