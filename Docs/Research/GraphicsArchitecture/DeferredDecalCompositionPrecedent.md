# Deferred Decal Composition Precedent

**Status:** research; external-source comparison, not Sparkle architecture or acceptance authority

**Scope:** external precedent for projected deferred decals, programmable GBuffer composition, arbitrary ray-hit lookup, and representative workload selection

**Local decision owner:** [Deferred GBuffer Decal Composition Architecture](../../Architecture/Modules/Engine/Renderer/Features/DeferredDecals/CompositionArchitecture.md)

## Research Question And Answer

The question is not merely how to draw a projected box. It is how one authored material overlay can preserve receiver material meaning across a deferred raster GBuffer, ray-produced primary visibility, and later arbitrary ray hits without creating independent composition systems.

The precedents support a two-part answer:

1. use screen-space depth reconstruction and programmable GBuffer composition for primary visible pixels because it naturally fits a deferred pipeline;
2. reuse the projection, sampling, ordering, and material functions at arbitrary ray hits, while allowing candidate lookup to differ because a screen tile cannot cover an off-camera reflection hit.

No reference supplies Sparkle's complete design directly. The local architecture still has to decide ownership, cooked data, receiver rules, GBuffer semantics, candidate capacity, shared raster/ray code, failure behavior, and proof.

## Precedent Comparison

| Approach | Strong fit | Cost or limitation | Local disposition |
| --- | --- | --- | --- |
| projected deferred volume | efficient primary-view coverage; reconstructs the receiver from depth | screen-space only; clipping, gradients, and overlap need explicit handling | selected for primary visible pixels |
| DBuffer-style intermediate material data | can separate decal accumulation from base-pass consumption and support baked-lighting interactions | extra storage, pass complexity, and receiver shader work | rejected until a baked-lighting requirement justifies it |
| fixed-function GBuffer blending | simple for a single independent channel | cannot correctly express coupled normal/material rules or arbitrary ordering | rejected in favor of programmable read/modify/write |
| acceleration-structure decal volumes | reaches arbitrary world-space ray hits | structure build/update, traversal, overlap, and memory cost | later measured alternative, not the first candidate route |
| receiver-owned spatial candidate spans | reuses scene bounds and can serve arbitrary hits without a second TLAS | candidate growth and update cost require strict bounds | first ray-hit hypothesis; acceptance must prove scale |

The decisive tradeoff is deliberate asymmetry in lookup with symmetry in semantics. Raster tiles and ray-hit receiver candidates may be different acceleration mechanisms; their projection and material result may not drift.

## Findings And Transferable Choices

| Reference finding | Transferable local choice |
| --- | --- |
| Epic documents projected decal boxes, ordered overlap, receiver response, and GBuffer application after the Base Pass and before lighting. Its DBuffer path partly supports baked-lighting interaction and adds receiver-material work and storage. | Keep projected volumes, ordering, receiver opt-out, and pre-lighting material composition. Do not add a DBuffer without a baked-lighting requirement that justifies it. |
| Frostbite's classic deferred method reconstructs world/local position from depth inside a convex volume, samples the decal, and blends GBuffer data. It identifies fixed-function alpha limitations and derivative/LOD hazards. | Use depth-reconstructed projection with programmable read/modify/write composition and explicit gradient-based sampling. |
| The i3D ray-tracing decal work shows that view-frustum grids do not serve arbitrary reflection hits and that a ray-tracing acceleration structure can enumerate decals anywhere at higher cost. | Keep the screen-space primary path. Begin arbitrary-hit support with receiver candidate spans from existing world bounds; consider a dedicated AABB structure only from measured need. |
| Ray Tracing Gems II surveys triangle/procedural decal approaches and single/multiple overlap costs. | Treat mesh/procedural AS decals as measured later alternatives, not a first-delivery prerequisite or parallel implementation. |
| Intel Modern Sponza is a high-resolution PBR workload with separately listed add-ons; the published list does not include decals. | Use a small Sparkle-authored decal fixture and label it accurately rather than attributing decals to Intel's content. |

## Adoption Limits

- Unreal's documented behavior is product precedent, not proof that Sparkle needs Unreal's feature breadth, DBuffer layout, or material/editor taxonomy.
- The Frostbite presentation explains an influential deferred technique, but its historical API and GBuffer choices are not a current backend contract for Sparkle.
- The ray-tracing references establish viable search techniques and costs; they do not choose the smallest structure for Sparkle's content scale.
- Modern Sponza is representative scene context only. A small analytic fixture must isolate projection, channel composition, overlap, receiver exclusion, capacity, and raster/ray parity before the scene is useful evidence.
- Published screenshots and reference timings do not define Sparkle tolerances or budgets. The feature acceptance contract owns those local proof obligations.

## Sources

- Epic Games, [Decal Materials](https://dev.epicgames.com/documentation/en-us/unreal-engine/decal-materials-in-unreal-engine) and [Decal Actors](https://dev.epicgames.com/documentation/unreal-engine/decal-actors-in-unreal-engine?lang=en-US)
- Johan Andersson and Daniel Kihl, [Destruction in Frostbite](https://advances.realtimerendering.com/s2010/Kihl-Destruction%20in%20Frostbite%28SIGGRAPH%202010%20Advanced%20RealTime%20Rendering%20Course%29.pdf), SIGGRAPH 2010 course material
- Sidney Hansen and Christoph Peters, [Rendering Decals and Many Lights with Ray Tracing Acceleration Structures](https://i3dsymposium.org/2021/posters/hansen2021_rendering_decals_and_many_lights_paper.pdf), i3D 2021
- Wessam Bahnassi, [Ray Tracing Decals](https://link.springer.com/chapter/10.1007/978-1-4842-7185-8_27), Ray Tracing Gems II
- Intel, [GPU Research Samples](https://www.intel.com/content/www/us/en/developer/topic-technology/graphics-research/samples.html)
