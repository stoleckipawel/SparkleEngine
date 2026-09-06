# Renderer UI And Viewport Composition

Status: current feature dossier; source-backed, not blend, color, DPI, input, lifetime, stress, or release evidence

Verified: 2026-09-06 against source revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

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
- `REN-E21` owns blend/color/DPI correctness, viewport generation, stale/wrong textures, registry bounds, and attribution. The feature-local proof contract is defined below.

## Acceptance Criteria

- `AC-UVC-01` — None, HostOverlay, and EditorViewport modes execute only their documented post-graph/pre-submit behavior and do not alter scene products when no UI work is requested.
- `AC-UVC-02` — immutable packet replay preserves vertex/index/command order, clipping, texture selection, premultiplied/straight-alpha contract, color transfer, and draw-data lifetime through submission.
- `AC-UVC-03` — host overlay composition matches the declared blend/color result across transparent, opaque, nested clip, empty, high-DPI, and resize fixtures without changing input ownership.
- `AC-UVC-04` — editor viewport draw occurs only when packet and product viewport generations match; missing/stale product or texture retires/refuses the handle and never draws another viewport's image.
- `AC-UVC-05` — final viewport color transitions Common -> shader read -> Common around UI use, with correct queue ordering and completion lifetime on D3D12 and Vulkan.
- `AC-UVC-06` — repeated viewport create/resize/destroy/recreate and non-viewport texture registration have a measured/documented bound and stale-handle behavior; append-only growth is not treated as unlimited support.
- `AC-UVC-07` — two simultaneous viewports with different generations, DPI, extents, packets, and textures remain isolated through rapid switching and level reload.
- `AC-UVC-08` — UI diagnostics/captures identify composition mode, viewport generation, source product, texture handle, and failure reason sufficiently to distinguish no-op, refusal, and successful draw.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-UVC-01` | empty/malformed packet, invalid clip/index range, or missing texture | reject/skip the invalid command with visible error policy; scene output remains valid | `CHK-UVC-01` |
| `FM-UVC-02` | packet/product generation mismatch or stale viewport handle | draw is refused and stale mapping retired; no wrong viewport texture appears | `CHK-UVC-02` |
| `FM-UVC-03` | resize/destroy/recreate during in-flight UI use | old resources survive to completion, new generation is isolated, and registry remains bounded by its declared policy | `CHK-UVC-02`, `CHK-UVC-03` |
| `FM-UVC-04` | wrong resource state/queue ordering | native validation or state-contract check fails before the candidate can pass | `CHK-UVC-03` |
| `FM-UVC-05` | DPI/color/blend convention mismatch | decoded reference image comparison identifies the mismatched cell | `CHK-UVC-01` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-UVC-01` | deterministic UI pattern suite over packet order, alpha/color, clip, texture, empty/malformed, and DPI cells; compare decoded output | `AC-UVC-01`–`AC-UVC-03`; `FM-UVC-01`, `FM-UVC-05` |
| `CHK-UVC-02` | dual-viewport generation stress across resize/destroy/recreate/level reload with deliberately stale/missing packet/product/texture identities | `AC-UVC-04`, `AC-UVC-06`, `AC-UVC-07`; `FM-UVC-02`, `FM-UVC-03` |
| `CHK-UVC-03` | paired-backend native-validation run inspecting transitions, queue tokens, registry/texture retirement, and shutdown drain | `AC-UVC-05`, `AC-UVC-06`; `FM-UVC-03`, `FM-UVC-04` |
| `CHK-UVC-04` | inspect emitted diagnostics/capture metadata for None, successful overlay/editor draws, and each refusal path | `AC-UVC-08`; `FM-UVC-01`, `FM-UVC-02` |

This contract is **defined but unproved**. The absence of a crash or the presence of UI pixels does not prove correct blending, transfer, DPI, identity, transitions, or long-session lifetime.

## Primary Source Routes

- [`UiFrameRenderer.cpp`](../../../../../../Engine/Renderer/Private/UI/UiFrameRenderer.cpp)
- [`UiRenderPacketPlayer.cpp`](../../../../../../Engine/Renderer/Private/UI/UiRenderPacketPlayer.cpp)
- [`EditorTextureRegistry.cpp`](../../../../../../Engine/Renderer/Private/Editor/EditorTextureRegistry.cpp)
