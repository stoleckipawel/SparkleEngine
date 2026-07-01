# Sparkle Documentation

This folder keeps only active engineering contracts and the current PBR rendering plan. Old review notes, superseded staged plans, and standalone validation notes are intentionally removed so the remaining docs are useful during implementation.

## PBR Rendering

- `Rendering/PBR/01-PBR-Reference-Requirements.md`: reference-backed PBR requirements and target contracts.
- `Rendering/PBR/03-PBR-Implementation-Action-Plan.md`: staged implementation backlog.
- `Rendering/PBR/04-PBR-Renderer-Signal-Contract.md`: current renderer signal semantics.
- `Rendering/PBR/05-Renderer-Reference-Quality-Gap-Audit.md`: current priority gate, focused first on editor-visible SIGMA and DLRR integration.

## Architecture Contracts

- `Architecture/02-Contracts/RHIContract.md`: RHI ownership, resource, queue, descriptor, and interop rules.
- `Architecture/02-Contracts/RendererFrameGraph.md`: frame construction, pass/resource ownership, render-path split, and frame graph rules.
- `Architecture/02-Contracts/RendererProviderContract.md`: DLSS, future DLRR, denoiser, and provider boundary rules.
- `Architecture/02-Contracts/ShaderPipeline.md`: shader source/cook/reflection/runtime package contract.
- `Architecture/02-Contracts/RuntimeSceneData.md`: scene snapshot and render-scene data ownership.
- `Architecture/02-Contracts/ApplicationLifecycle.md`: startup, frame execution, tool, smoke, and shutdown lifecycle.

Keep new docs only when they describe an active contract, an implementation plan, or a reference-backed rendering decision that the source cannot make obvious by itself.
