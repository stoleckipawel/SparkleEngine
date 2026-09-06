# Renderer Geometry And Resources

Status: Renderer feature-family index

Scope: route the resource-residency and surface-contract documents that carry immutable asset generations into renderable geometry and material products

| Document | Open it for |
| --- | --- |
| [Mesh And Texture Residency](MeshAndTextureResidency.md) | admission, preparation, upload, active generations, budgets, fallback, eviction, and completion-safe retirement |
| [Visibility And Draw Preparation](VisibilityAndDrawPreparation.md) | visibility policy, culling and LOD boundaries, sorting, draw classification, batching, failure, and explicit unsupported cases |
| [Geometry, Materials, And GBuffer](GeometryMaterialsAndGBuffer.md) | geometry/frontend coverage, material semantics, GBuffer products, limits, and raster/ray agreement |

Residency owns whether a resource generation is safely active. Visibility owns which prepared instances and batches proceed. Geometry/material documentation owns how an active visible generation becomes a surface result. None may borrow another's acceptance claim. The parent [Renderer Feature Dossiers](../README.md) index owns capability routing.
