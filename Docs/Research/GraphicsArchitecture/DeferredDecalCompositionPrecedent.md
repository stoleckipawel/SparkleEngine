# Deferred Decal Composition Precedent

Status: research; external-source comparison, not Sparkle architecture or acceptance authority

Scope: external precedent for projected deferred decals, programmable GBuffer composition, arbitrary ray-hit lookup, and representative workload selection

Local decision owner: [Deferred GBuffer Decal Composition Architecture](../../Architecture/Modules/Engine/Renderer/Features/DeferredDecals/CompositionArchitecture.md)

## Findings And Transferable Choices

| Reference finding | Transferable local choice |
| --- | --- |
| Epic documents projected decal boxes, ordered overlap, receiver response, and GBuffer application after the Base Pass and before lighting. Its DBuffer path partly supports baked-lighting interaction and adds receiver-material work and storage. | Keep projected volumes, ordering, receiver opt-out, and pre-lighting material composition. Do not add a DBuffer without a baked-lighting requirement that justifies it. |
| Frostbite's classic deferred method reconstructs world/local position from depth inside a convex volume, samples the decal, and blends GBuffer data. It identifies fixed-function alpha limitations and derivative/LOD hazards. | Use depth-reconstructed projection with programmable read/modify/write composition and explicit gradient-based sampling. |
| The i3D ray-tracing decal work shows that view-frustum grids do not serve arbitrary reflection hits and that a ray-tracing acceleration structure can enumerate decals anywhere at higher cost. | Keep the screen-space primary path. Begin arbitrary-hit support with receiver candidate spans from existing world bounds; consider a dedicated AABB structure only from measured need. |
| Ray Tracing Gems II surveys triangle/procedural decal approaches and single/multiple overlap costs. | Treat mesh/procedural AS decals as measured later alternatives, not a first-delivery prerequisite or parallel implementation. |
| Intel Modern Sponza is a high-resolution PBR workload with separately listed add-ons; the published list does not include decals. | Use a small Sparkle-authored decal fixture and label it accurately rather than attributing decals to Intel's content. |

## Sources

- Epic Games, [Decal Materials](https://dev.epicgames.com/documentation/en-us/unreal-engine/decal-materials-in-unreal-engine) and [Decal Actors](https://dev.epicgames.com/documentation/unreal-engine/decal-actors-in-unreal-engine?lang=en-US)
- Johan Andersson and Daniel Kihl, [Destruction in Frostbite](https://advances.realtimerendering.com/s2010/Kihl-Destruction%20in%20Frostbite%28SIGGRAPH%202010%20Advanced%20RealTime%20Rendering%20Course%29.pdf), SIGGRAPH 2010 course material
- Sidney Hansen and Christoph Peters, [Rendering Decals and Many Lights with Ray Tracing Acceleration Structures](https://i3dsymposium.org/2021/posters/hansen2021_rendering_decals_and_many_lights_paper.pdf), i3D 2021
- Wessam Bahnassi, [Ray Tracing Decals](https://link.springer.com/chapter/10.1007/978-1-4842-7185-8_27), Ray Tracing Gems II
- Intel, [GPU Research Samples](https://www.intel.com/content/www/us/en/developer/topic-technology/graphics-research/samples.html)
