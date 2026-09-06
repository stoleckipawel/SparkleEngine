# RHI Presentation And Interop

Status: RHI feature-family index

Scope: route window presentation, immutable ImGui lowering, and deliberately narrow external-native integration

| Document | Open it for |
| --- | --- |
| [Presentation](Presentation.md) | swapchain acquisition, state, resize/minimize, pacing, VSync, present, and device-loss boundary |
| [ImGui Rendering](ImGuiRendering.md) | immutable draw-data lowering, textures/descriptors, clipping, blend/color behavior, and completion lifetime |
| [External Interop](ExternalInterop.md) | native identity/state/hooks, provider eligibility, generation, fallback, and package boundary |

Presentation owns swapchain images, ImGui owns UI command lowering, and interop owns exceptional external access. The parent [RHI Feature Dossiers](../README.md) index owns capability routing.
