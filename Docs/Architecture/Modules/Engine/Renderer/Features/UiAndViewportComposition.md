# Renderer UI And Viewport Composition

Status: current feature dossier; source-backed, not blend, color, DPI, input, lifetime, stress, or release evidence

Verified: 2026-09-06 against source revision `8414b5dc`

Scope: `REN-UI-01` through `REN-UI-04`; immutable UI packets, host overlays, editor viewport presentation, texture handles, and the post-graph/pre-submit composition boundary

## Feature Promise

`UiFrameRenderer` consumes one immutable UI packet after scene graph execution and before final frame submission.

| Mode | Current operation | Refusal/safe state |
| --- | --- | --- |
| None | no UI work | scene output continues unchanged |
| HostOverlay | replay packet through the RHI ImGui renderer inside a presentation overlay pass | empty draw data produces no overlay |
| EditorViewport | transition final viewport color to shader read, resolve/publish a texture handle, require packet/product viewport-generation agreement, draw the editor presentation pass, then return the product to Common state | missing graph/product/texture retires the viewport texture; empty or generation-mismatched packet is not drawn |

## Ownership And Lifetime

- Application/Editor produces immutable draw data; Renderer owns when it is replayed relative to graph execution and submit.
- RHI owns the actual ImGui/native resource rendering mechanism and transitions requested by Renderer.
- `EditorTextureRegistry` gives the viewport a generation-derived handle and maps other native texture IDs for editor/diagnostic consumers.
- The inspected non-viewport registration list appends unique bindings and has only the `uint32` slot-space bound; no explicit unregister/reclamation owner was found. Long-session growth and stale-handle behavior are therefore Partial capability, not an unlimited-lifetime guarantee.

## Failure, Tradeoffs, And Evidence

- Missing products, generation mismatch, stale handles, empty packets, resize, level switch, and repeated texture registration need controlled behavior.
- Immutable packets preserve threading and ownership clarity, but can still carry stale identity if producer/consumer generation checks are incomplete.
- Shared host/editor composition reduces duplicated UI backends while increasing the importance of explicit mode and texture lifetime.
- `REN-E21` owns blend/color/DPI correctness, viewport generation, stale/wrong textures, registry bounds, and attribution. Stable feature-local `AC-*`/`FM-*`/`CHK-*` identities remain an `INV-009` gap.

## Primary Source Routes

- [`UiFrameRenderer.cpp`](../../../../../../Engine/Renderer/Private/UI/UiFrameRenderer.cpp)
- [`UiRenderPacketPlayer.cpp`](../../../../../../Engine/Renderer/Private/UI/UiRenderPacketPlayer.cpp)
- [`EditorTextureRegistry.cpp`](../../../../../../Engine/Renderer/Private/Editor/EditorTextureRegistry.cpp)

