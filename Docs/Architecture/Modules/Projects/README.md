# Project Architecture

**Status:** Projects module index

**Current readiness:** **50/100** for the current Showcase product route — project/catalog/source integration exists, while executable workload, package, adoption, and release evidence remains open. See [Current Feature Readiness](../../../Acceptance/CurrentReadiness.md#product-build-and-delivery).

Projects compose engine and tool modules into user-facing products and representative workloads. Product-owned knowledge stays here rather than being assigned to an engine module.

## At A Glance

The repository currently has one discovered product project, Showcase. Its thin targets select the shared editor/runtime hosts and its catalog chooses representative content; it must not contain scene-specific engine fixes or duplicate importer, world, Renderer, RHI, or delivery policy.

| Question | Owner |
| --- | --- |
| Which executable and level does the product select? | project dossier |
| How is source content imported/cooked? | Tools module dossiers |
| How does the world or frame work? | GameFramework and Renderer/RHI dossiers |
| What workload must the product prove? | Acceptance workloads and feature reports |
| What ships and how is it installed? | Build/Packaging and release acceptance |

## Project Routes

| Project | Owns | Project documentation |
| --- | --- | --- |
| Showcase | editor/runtime targets, startup and selection, level catalog, asset-pack readiness, and workload coverage | [Showcase](Showcase/README.md) |

Reusable implementation policy belongs with the relevant Engine or Tools module; completion criteria belong in [Acceptance](../../../Acceptance/README.md).
