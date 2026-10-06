# External Capture Integration Research

**Status:** research; primary-source precedent and a dated source audit

**Scope:** PIX, Nsight, RenderDoc, Unreal, Unity, Godot, and the remaining specialist-tool routes that inform Sparkle's external capture integration.

**Observed:** 2026-10-06; Sparkle revision `82528cbe5edbcf729464fbefe6998435730920eb`, initially clean working tree.

**Authority boundary:** this page records evidence and transfer limits. [Discovery](Discovery.md) closes choices; [Semantics](Semantics.md) and [Execution Architecture](ExecutionArchitecture.md) define the target; [Plan](Plan.md) orders delivery.

**Current readiness:** Not applicable; research adds no implementation or verification credit to the [dossier](README.md).

## Current Sparkle Baseline

These are source/build observations, with no build, installed-tool, capture, GPU, or package result.

| Inspected owner | Finding | Integration consequence |
| --- | --- | --- |
| [RendererExternalRuntime](../../../../../Engine/Renderer/Private/Integrations/RendererExternalRuntime.cpp) and its header | Application-thread process integration resolves backend and initializes shared Streamline; destructor shuts it down. | Extend composition with a neutral pre-graphics bootstrap; reconcile Streamline ordering rather than inject from viewport UI. |
| [RendererBackendConfiguration](../../../../../Engine/Renderer/Private/Host/RendererBackendConfiguration.h) | Only backend API and interposer hooks. | No existing external-capture launch contract to reuse unchanged. |
| [RhiDiagnostics](../../../../../Engine/RHI/Public/Diagnostics/RhiDiagnostics.h) | Object, timing, message, failure, and memory services; no external frame-capture capability/request/result. | Extend this diagnostics composition, keeping texture readback separate. |
| [D3D12PixEvents](../../../../../Engine/RHI/Private/D3D12/Diagnostics/D3D12PixEvents.cpp) | Lazy bare-name DLL load and manually declared event exports; no GPU capturer bootstrap. | Marker availability is not capture readiness. Replace the touched marker ABI/loading path using the selected official header/runtime. |
| [RHI build membership](../../../../../Engine/RHI/CMakeLists.txt) | Backend sources are separate; no PIX/NGFX/RenderDoc SDK configuration was found in this target. | Add eligible-profile dependencies and sources explicitly; prove Shipping exclusion. |
| [LevelRunOperations](../../../../../Tools/Launcher/SparkleLauncher/Public/SparkleLauncher/LevelRunOperations.h) | Level/run/profile/API intent, no provider selection. | Extend this typed launch operation and its process-request producer, not a second launcher. |
| [ViewportTopPanel](../../../../../Engine/Editor/Private/Panels/ViewportTopPanel.cpp) | View/settings/camera/status UI, no provider action. | Compose a small capture presenter; keep runtime state out of this panel. |
| [UiFrameRenderer](../../../../../Engine/Renderer/Private/UI/UiFrameRenderer.cpp) and [FramePipeline](../../../../../Engine/Renderer/Private/Frame/FramePipeline.cpp) | Scene product is exposed as a texture, then composed into host UI; submission uses device services. | An Editor scene viewport is not itself proof of a native swapchain. Freeze scene-view to present-surface mapping first. |
| [D3D12 device services](../../../../../Engine/RHI/Private/D3D12/Device/D3D12RenderDeviceServices.cpp), [Vulkan device services](../../../../../Engine/RHI/Private/Vulkan/Device/VulkanRenderDeviceServices.cpp) | Device initialization precedes presentation/hardware-interface construction. | Bootstrap must precede this sequence; capture delimiters must trace real submission/present ownership. |

Targeted searches used `rg --files` and separate `rg -e` expressions for `PIX`, `pix`, `RenderDoc`, `renderdoc`, `Nsight`, `nsight`, `WinPix`, and launch/capture counterparts in Engine, Tools, and CMake. No capture provider was found. This is a bounded audit, not a proof that every optional vendor feature is supported by every tool.

## Primary Source Ledger

Live manuals are dated observations, not immutable releases. Stage 0 retains the installed header/tool hashes and relevant manual sections before implementation. A release tag anchors source precedent; never substitute `master` for that tag in the decision record.

| ID / observed reference | Narrow finding | Permitted transfer / non-inference | Rights and refresh trigger |
| --- | --- | --- | --- |
| `SRC-PIX-API`: [Microsoft programmatic capture](https://devblogs.microsoft.com/pix/programmatic-capture/), GPU prerequisites and target-window sections | GPU capturer setup precedes D3D12 calls; event runtime and capturer are separate. Next-frame capture uses Present delimiters; targeting a window does not exclude all other windows' work. | Use early setup and explicit target mapping. Do not infer viewport-only content or finalization from a successful request call. | Vendor documentation, paraphrase only; refresh on PIX/runtime/Agility/OS changes. |
| `SRC-PIX-EVENT`: [official PixEvents repository](https://github.com/microsoft/PixEvents), usage and license | Microsoft recommends its NuGet runtime or shared runtime build; headers/runtime are MIT. | Prefer official headers plus one shared runtime. Do not hand-maintain event encoding or infer permission to redistribute PIX's capturer. | Preserve package notices if included; freeze package version/hash at discovery. |
| `SRC-PIX-HDR`: [pix3.h](https://raw.githubusercontent.com/microsoft/PixEvents/main/include/pix3.h) and [WinPixEventRuntime manual](https://devblogs.microsoft.com/pix/winpixeventruntime/) | Official header controls enabled instrumentation and includes Windows capture facilities. | Inspect matching package headers for attachment/status/open APIs and compile defines; marker names are data, not format strings. | MIT header; mutable source requires package pin and refresh. |
| `SRC-NGFX-GUIDE`: [Nsight Graphics SDK guide](https://docs.nvidia.com/nsight-graphics/UserGuide/sdk.html), initialization and usage scenarios | Header-only beta SDK; activity initialization is early and only one activity is initialized per process. Graphics Capture and GPU Trace have distinct workflows. | Treat the SDK route as experimental and freeze one activity per launch. Tool-managed launch remains useful when procedural control cannot be accepted. | NVIDIA SDK terms govern headers/libraries; beta/manual/driver changes invalidate the matrix. |
| `SRC-NGFX-API`: [NGFX API reference](https://docs.nvidia.com/nsight-graphics/NsightGraphicsSdk/group___n_g_f_x___a_p_i___c_o_r_e.html) | Current API favors NGFX over the older Injection API and exposes activity/artifact operations. | Compile against one pinned external API; record disagreement between guide and installed headers. Do not add Sparkle compatibility versions. | Installed SDK terms and hash mandatory; refresh on SDK changes. |
| `SRC-NGFX-TARGET`: [D3D12 request parameters](https://docs.nvidia.com/nsight-graphics/NsightGraphicsSdk/struct_n_g_f_x___graphics_capture___request_capture___d3_d12___params___v1.html), [Vulkan request parameters](https://docs.nvidia.com/nsight-graphics/NsightGraphicsSdk/struct_n_g_f_x___graphics_capture___request_capture___vulkan___params___v1.html) | Request structures specify capture delimiters/counts. | Verify the installed target-filter mechanism rather than assuming PIX-like per-window controls. A procedural boundary needs a production-frame proof. | Vendor API references; refresh with SDK/tool/backend. |
| `SRC-NGFX-CLI`: [Graphics Capture CLI](https://docs.nvidia.com/nsight-graphics/UserGuide/graphics-capture-cli.html) | Tool launches an injected process and creates replayable graphics captures. | Offer documented tool-managed workflow; do not relabel a launch or replay as native timing. | Vendor tool terms; refresh CLI help/version and artifact format. |
| `SRC-RD-API`: [RenderDoc v1.46 application header](https://raw.githubusercontent.com/baldurk/renderdoc/v1.46/renderdoc/api/app/renderdoc_app.h) | Queried function table, explicit capture device/window pairs, capture enumeration, replay UI, and Vulkan instance-derived device pointer. Wildcard ambiguity and overlapping captures are unsafe. | Use one dynamic API boundary and explicit target; enumerate actual completed paths. `IsTargetControlConnected` concerns UI connection, not capture capability. | Application header MIT; RenderDoc distribution has its own notices. Refresh header/tool/driver/API feature. |
| `SRC-UE-PIX`: [Unreal PIX integration](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-pix-on-windows-with-unreal-engine), observed UE 5.8 wrapper | Startup attachment flag and upper-right Level Viewport capture action. | Adopt discovery and conditional action placement. No claim about Unreal's private implementation, simultaneous tools, or Sparkle compatibility. | Documentation-only precedent; no proprietary source copied. Refresh integration guidance. |
| `SRC-UE-RD`: [Unreal RenderDoc integration](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-renderdoc-with-unreal-engine), observed UE 5.8 wrapper | Plugin/startup attachment and Level Viewport capture entry. | Keep provider setup distinct from capture interaction; do not import Unreal's plugin architecture. | Documentation-only precedent; refresh UI/startup guidance. |
| `SRC-UNITY`: [Unity 6.0 RenderDoc integration](https://docs.unity3d.com/6000.0/Documentation/Manual/RenderDocIntegration.html), loading/capturing sections | Loaded compatible tool adds Game/Scene View action; late loading recreates the graphics device. | Adopt view-context action and explicit readiness. Reject late device recreation for Sparkle's initial scope. The page's shader/API examples do not define today's Sparkle support. | Proprietary manual, paraphrase only; refresh Unity/tool guidance. |
| `SRC-GODOT-CODE`: [Godot 4.5-stable D3D12 driver](https://github.com/godotengine/godot/blob/4.5-stable/drivers/d3d12/rendering_device_driver_d3d12.cpp), `command_begin_label` / `command_end_label`; [SCsub](https://github.com/godotengine/godot/blob/4.5-stable/drivers/d3d12/SCsub), PIX build block | Backend uses official PIX macros behind a build capability; build script supplies its include path. | Put native annotations in backend lowering. This marker precedent proves neither an in-editor PIX capture button nor a Nsight/RenderDoc provider. | MIT source; no code copied. Record resolved tag commit when implementing. |
| `SRC-GODOT-DOC`: [Godot 4.5 GPU optimization](https://raw.githubusercontent.com/godotengine/godot-docs/4.5/tutorials/performance/gpu_optimization.rst) | Profiling/bottleneck guidance separates measurement from changes. | Use controlled experiments; do not assume frame-debugger time is representative throughput. | Documentation repository terms; pinned branch observation, retain commit at discovery. |
| `SRC-AMD`: [AMD tools suite](https://gpuopen.com/tools/), [RGP](https://gpuopen.com/rgp/) | Separate profiling, memory, ray-tracing, crash, and static-analysis tools; RGP collection uses Radeon Developer Panel/driver. | Keep different activities/artifacts separate. Rest-of-tools delivery can be marker/runbook handoff rather than a fictional common capture API. | Per-tool notices/requirements; current RGP page shows v2.7.1, superseding older runbook v2.7 snapshot. Refresh before use. |
| `SRC-SYSTEMS`: [Nsight Systems guide](https://docs.nvidia.com/nsight-systems/UserGuide/index.html), [Microsoft ETW](https://learn.microsoft.com/en-us/windows/win32/etw/event-tracing-portal), [PresentMon repository](https://github.com/GameTechDev/PresentMon) | System traces, OS events, and presentation measurement answer different questions from native frame replay. | Reuse existing thread/ETW identities and explicit run provenance; do not embed these viewers or treat presentation measurements as full input-to-photon proof. | Each tool's terms apply; refresh platform/privilege/trace settings and installed version. |

The RenderDoc documentation site and guessed Godot RenderDoc tutorial URLs did not return readable content in this research pass. RenderDoc's release-tagged official header supplied its API contract. Godot conclusions use the readable tagged driver/build sources and official performance document; no native Godot capture UI is asserted.

## Comparison And Transfer Decisions

| Question | Unreal | Unity | Godot | Sparkle design consequence |
| --- | --- | --- | --- | --- |
| Where does a user capture? | Conditional Level Viewport action | Game/Scene View action | No capture-button claim from inspected sources | Use selected scene view context, expose containing present surface and scope limitation. |
| When is a tool loaded? | Startup attachment route | Startup or device-recreating late route | PIX include/build capability | Initial integration is immutable at process start. |
| Where are native markers emitted? | Public guidance is insufficient for internal placement claims | Manual is insufficient for internal placement claims | Backend driver source is inspectable | Keep semantic labels in existing graph/owner route and native encoding in RHI. |
| How many tools can coexist? | Separate tool pages do not prove coexistence | RenderDoc page does not prove coexistence | Not established | Default-deny untested injected combinations; exclusive requests alone cannot make two hooks safe. |
| What proves success? | Tool workflow is precedent | Tool workflow is precedent | Source is precedent | Candidate-bound native artifact, matching scene/view/frame markers, actual replay inspection, and negative controls. |

## Options Rejected By The Proposed Design

- One in-engine profiler abstraction for captures, hardware traces, static shader analysis, screenshots, and crash dumps: lifetimes and success meanings differ.
- Automatic injection because a DLL is installed: changes the workload and makes no-provider behavior depend on machine contents.
- One giant phase implementing three SDKs and all tools together: prevents a reviewable PIX first-use result.
- A permanent SDK plugin registry or provider-specific pass annotations: the initial provider set is closed and existing backend composition owns lowering.
- Late tool loading/device recreation: requires a renderer-recreation product not established by this task.
- Sharing another engine's flags/classes or copying source: use the transferable invariant, not its architecture or licensing assumptions.

## Remaining Research-To-Execution Boundary

The source-backed design is established in this package. Installed PIX/NGFX/RenderDoc versions, safe cancellation/status APIs, exact interposer/hook compatibility, target delimiters, SDK redistribution rights, and measured overhead are not established by online manuals. [Discovery](Discovery.md) assigns those decisions and prevents them from being guessed during implementation.
