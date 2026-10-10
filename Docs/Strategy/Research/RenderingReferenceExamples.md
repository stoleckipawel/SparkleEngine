# Rendering Reference Cards

**Status:** inspected primary-source locations and delivery precedent; no SDK, algorithm replacement or production implementation is authorized

**Inspected:** 2026-10-10. Sixteen repositories were resolved to exact commits; selected source and root license files were retrieved. This is a targeted source review, not a whole-repository audit, sample build or native performance reproduction. [Engineer profiles](GraphicsEngineerProfiles.md) own affiliation/attribution, [role study](GraphicsRoleDepth.md) explains expectations and [catalog](../FeatureDeliveryCatalog.md) owns selected outputs.

## How To Use A Card

`NVR-*` identifies a research reference, not a feature, acceptance gate or requirement. Each card states **what** to inspect, **where** in immutable source, **why** it helps, the existing local owner, a check and limits. Before delivery, record the selected card, the specific assumption borrowed, exact local consumer, source/license decision and a falsifying experiment in the owning discovery/stage. Revalidate tool/platform requirements before execution. A source refresh changes provenance, not a previous result.

License labels below identify the inspected document, not a legal conclusion or permission to copy arbitrary files/assets/submodules. No external code is incorporated by this research. Algorithms, source code, binaries, data, screenshots and assets may have different terms. Resolve actual reuse through existing dependency/rights owners.

## Pinned Source Cards

**Local owner navigation:** [Reference Path Tracer](../../Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/Plan.md), [Direct Lighting](../../Architecture/Modules/Engine/Renderer/Features/Lighting/DirectLighting/Discovery.md), [Indirect Lighting](../../Architecture/Modules/Engine/Renderer/Features/Lighting/IndirectLighting/Discovery.md), [Shader System](../../Architecture/CrossModule/ShaderSystem/Plan.md), [Neural discovery](../../Architecture/CrossModule/NeuralGraphics/Discovery.md), [Workload Studies](../../Architecture/CrossModule/PerformanceDiagnostics/WorkloadStudies/Plan.md), [Visibility](../../Architecture/Modules/Engine/Renderer/Features/GeometryAndResources/VisibilityAndDrawPreparation.md) and [Residency](../../Architecture/Modules/Engine/Renderer/Features/GeometryAndResources/MeshAndTextureResidency.md). Use the listed owner's accepted stage, rather than creating a new implementation route from this research.

### NVR-01 - Portable RHI and resource lifetime

**Source:** `NVIDIA-RTX/NVRHI@6b96fb03e07539f08327aea76c56d55f1de9d906`.

**Where:** [src/common/state-tracking.cpp](https://github.com/NVIDIA-RTX/NVRHI/blob/6b96fb03e07539f08327aea76c56d55f1de9d906/src/common/state-tracking.cpp); [src/validation/validation-commandlist.cpp](https://github.com/NVIDIA-RTX/NVRHI/blob/6b96fb03e07539f08327aea76c56d55f1de9d906/src/validation/validation-commandlist.cpp).

**What:** Read state requirements and validation alongside the authored portable-rendering explanation; connect command-list recording, submission and retirement.

**Why / local owner / check:** RHI/FrameGraph and ShaderSystem; PGD-03/04/07. Check one existing resource from declared use through backend transition and GPU completion; compare recorded/submitted order and a rejected bad-state control.

**Limits:** Do not add a second resource-state tracker or adopt NVRHI as another RHI. Its abstraction choices are precedent, not proof of Sparkle correctness.

**Rights route:** [MIT-style permission; LICENSE.txt](https://github.com/NVIDIA-RTX/NVRHI/blob/6b96fb03e07539f08327aea76c56d55f1de9d906/LICENSE.txt).

### NVR-02 - Many-light sampling and application bridge

**Source:** `NVIDIA-RTX/RTXDI@a6efab966b7c3b272da0461578eb56ac61c7cbff`.

**Where:** [Doc/RtxdiApplicationBridge.md](https://github.com/NVIDIA-RTX/RTXDI/blob/a6efab966b7c3b272da0461578eb56ac61c7cbff/Doc/RtxdiApplicationBridge.md); [Samples/FullSample/Shaders/LightingPasses/DI/TemporalResampling.hlsl](https://github.com/NVIDIA-RTX/RTXDI/blob/a6efab966b7c3b272da0461578eb56ac61c7cbff/Samples/FullSample/Shaders/LightingPasses/DI/TemporalResampling.hlsl).

**What:** Study the surface/material/light bridge and temporal pass: current/previous surface, motion conversion, sample validity and explicit reservoir buffer indices.

**Why / local owner / check:** DirectLighting DIR-D0-05..08 and PGD-02/03. Derive target/proposal PDFs and normalization; freeze bias policy and test light changes, disocclusion, thin geometry and invalid history against the independent baseline.

**Limits:** DI bridge semantics do not supply GRIS path shifts or prove an indirect estimator. The SDK includes submodules not exhaustively audited here; copying sample APIs is not the objective.

**Rights route:** [NVIDIA RTX SDKs license; LICENSE.txt; proprietary-labelled sample headers](https://github.com/NVIDIA-RTX/RTXDI/blob/a6efab966b7c3b272da0461578eb56ac61c7cbff/LICENSE.txt).

### NVR-03 - Classical denoiser input and guide contract

**Source:** `NVIDIA-RTX/NRD@e06852bd545973cb224eb28820b891205cfc1624`.

**Where:** [Include/NRDDescs.h](https://github.com/NVIDIA-RTX/NRD/blob/e06852bd545973cb224eb28820b891205cfc1624/Include/NRDDescs.h); [Integration/NRDIntegration.hpp](https://github.com/NVIDIA-RTX/NRD/blob/e06852bd545973cb224eb28820b891205cfc1624/Integration/NRDIntegration.hpp).

**What:** Inspect ResourceType noisy/guide encodings and integration resources. Diffuse/specular radiance and hit distance have denoiser-specific packing obligations.

**Why / local owner / check:** Direct/Indirect reconstruction and Neural NG-D01/02/06; PGD-03/06. Document guide units, spaces, motion, demodulation, invalid history and reset. A deliberately wrong guide must fail before a quality verdict.

**Limits:** NRD is not an owned neural model. No SDK is adopted here; classical baseline selection and rights remain discovery decisions.

**Rights route:** [NVIDIA RTX SDKs license; LICENSE.txt](https://github.com/NVIDIA-RTX/NRD/blob/e06852bd545973cb224eb28820b891205cfc1624/LICENSE.txt).

### NVR-04 - Research path tracer and explicit outputs

**Source:** `NVIDIAGameWorks/Falcor@759aad033ff610fb0d82c74f7e0a508d0096d5f2`.

**Where:** [Source/RenderPasses/PathTracer/PathTracer.cpp](https://github.com/NVIDIAGameWorks/Falcor/blob/759aad033ff610fb0d82c74f7e0a508d0096d5f2/Source/RenderPasses/PathTracer/PathTracer.cpp).

**What:** Inspect pass properties, option validation, shader reset and declared outputs, including separate denoiser guide products.

**Why / local owner / check:** ReferencePathTracer recovery and Neural NG-D02; PGD-02/06. Make raw transport output, settings and history identity inspectable; compare equivalent domains before using output as truth.

**Limits:** A shared estimator or guide implementation is not an independent oracle. The large research framework is not Sparkle's desired runtime structure.

**Rights route:** [BSD-style NVIDIA license; LICENSE.md; dependencies separately scoped](https://github.com/NVIDIAGameWorks/Falcor/blob/759aad033ff610fb0d82c74f7e0a508d0096d5f2/LICENSE.md).

### NVR-05 - A path tracer becomes a game renderer

**Source:** `NVIDIA/Q2RTX@f2526e9a165949f66e91e82f0d63aa7bb2567b4d`.

**Where:** [src/refresh/vkpt/path_tracer.c](https://github.com/NVIDIA/Q2RTX/blob/f2526e9a165949f66e91e82f0d63aa7bb2567b4d/src/refresh/vkpt/path_tracer.c); [src/refresh/vkpt/shader/asvgf_temporal.comp](https://github.com/NVIDIA/Q2RTX/blob/f2526e9a165949f66e91e82f0d63aa7bb2567b4d/src/refresh/vkpt/shader/asvgf_temporal.comp).

**What:** Follow production ray-tracing setup and temporal denoising of an older game; separate ray workload, reconstruction and content assumptions.

**Why / local owner / check:** ReferencePathTracer, Direct/Indirect discovery and PGD-02/03/08. Retain raw/filtered products, temporal compatibility and a failure gallery for motion/material/content changes.

**Limits:** Credit original Q2VKPT and later team work. This game's material/light domain and NV-era evolution do not establish Sparkle parity or portable shader semantics.

**Rights route:** [GPL version 2 stated in license.txt; assets/dependencies have separate rights](https://github.com/NVIDIA/Q2RTX/blob/f2526e9a165949f66e91e82f0d63aa7bb2567b4d/license.txt).

### NVR-06 - Explicit Vulkan ray-tracing setup

**Source:** `nvpro-samples/vk_raytracing_tutorial_KHR@952a1a8e8b85f25781db7014d4d2bda124435e79`.

**Where:** [raytrace_tutorial/02_basic_nvvk/02_basic_nvvk.cpp](https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR/blob/952a1a8e8b85f25781db7014d4d2bda124435e79/raytrace_tutorial/02_basic_nvvk/02_basic_nvvk.cpp).

**What:** Inspect the current nvvk tutorial route for scene resources, acceleration structures, pipeline/binding and native commands; start from its README prerequisites.

**Why / local owner / check:** RHI ray-tracing and ShaderSystem; PGD-02/04/07. Trace the shader binding and acceleration-structure identity actually used by one capture; retain capability-negative and stale-generation controls.

**Limits:** The inspected tutorial now has numbered Slang-based directories; old hello-vulkan paths are not current. Do not introduce a second tutorial framework or claim a sample is native validation.

**Rights route:** [Apache-2.0; LICENSE](https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR/blob/952a1a8e8b85f25781db7014d4d2bda124435e79/LICENSE).

### NVR-07 - Historical parallel recording comparison

**Source:** `nvpro-samples/gl_vk_threaded_cadscene@ed0cd468c84dc49ff726dbd4c14df0e91ec0e717`.

**Where:** [rendererthread_vk.cpp](https://github.com/nvpro-samples/gl_vk_threaded_cadscene/blob/ed0cd468c84dc49ff726dbd4c14df0e91ec0e717/rendererthread_vk.cpp).

**What:** Compare worker-recording/main-submit and worker-submit variants and chunking. The README explicitly marks the sample DEPRECATED.

**Why / local owner / check:** Workload WS-S0/WS-S2; PGD-03. Use historical hypothesis design and recorded-versus-submitted order to explain a CPU bottleneck; NVR-16 is the maintained successor reference.

**Limits:** Archived/deprecated, with old Vulkan/OpenGL integration assumptions. Do not build an implementation on this sample or carry its extensions into Sparkle.

**Rights route:** [Apache-2.0; LICENSE](https://github.com/nvpro-samples/gl_vk_threaded_cadscene/blob/ed0cd468c84dc49ff726dbd4c14df0e91ec0e717/LICENSE).

### NVR-08 - Meshlet/task culling and geometry granularity

**Source:** `nvpro-samples/gl_vk_meshlet_cadscene@4f6f7f19f34482a5c4c0424116bdfe3f5baa9db8`.

**Where:** [drawmeshlet_ext.task.glsl](https://github.com/nvpro-samples/gl_vk_meshlet_cadscene/blob/4f6f7f19f34482a5c4c0424116bdfe3f5baa9db8/drawmeshlet_ext.task.glsl); [drawmeshlet_ext_cull.mesh.glsl](https://github.com/nvpro-samples/gl_vk_meshlet_cadscene/blob/4f6f7f19f34482a5c4c0424116bdfe3f5baa9db8/drawmeshlet_ext_cull.mesh.glsl).

**What:** Inspect EXT task/mesh paths, culling and work granularity; distinguish the repository's NV variants.

**Why / local owner / check:** Conditional GeometryAndResources visibility study; PGD-03/09. First measure existing draw/visibility costs and primitive distributions; compare equivalent images, GPU/CPU cost and capability failures.

**Limits:** Study-only optional extension. Mesh shaders are not mandatory portfolio scope; an EXT source does not prove a D3D12 implementation or installed feature support.

**Rights route:** [Apache-2.0; LICENSE](https://github.com/nvpro-samples/gl_vk_meshlet_cadscene/blob/4f6f7f19f34482a5c4c0424116bdfe3f5baa9db8/LICENSE).

### NVR-09 - Memory budget as a concrete consumer

**Source:** `nvpro-samples/vk_mini_samples@1ca74a9c2b459848fded141dc14392dcc9e88029`.

**Where:** [samples/memory_budget/memory_budget.cpp](https://github.com/nvpro-samples/vk_mini_samples/blob/1ca74a9c2b459848fded141dc14392dcc9e88029/samples/memory_budget/memory_budget.cpp).

**What:** Read the VK_EXT_memory_budget example and how allocation/deletion follows visibility and budget pressure.

**Why / local owner / check:** Existing residency owner and Workload Studies; PGD-03/04. Distinguish residency budget, allocation and measured peak; demonstrate bounded pressure behavior without changing the comparison workload.

**Limits:** A reported heap budget is neither physical free memory nor a frame-local high-water measure. Do not add a generic monitor or assume the extension exists on every tuple.

**Rights route:** [Apache-2.0; LICENSE](https://github.com/nvpro-samples/vk_mini_samples/blob/1ca74a9c2b459848fded141dc14392dcc9e88029/LICENSE).

### NVR-10 - Small neural networks and fused GPU kernels

**Source:** `NVlabs/tiny-cuda-nn@3daa6e5b8b8e0fa17971a1a20acfbb3754494b38`.

**Where:** [src/fully_fused_mlp.cu](https://github.com/NVlabs/tiny-cuda-nn/blob/3daa6e5b8b8e0fa17971a1a20acfbb3754494b38/src/fully_fused_mlp.cu).

**What:** Inspect fully fused MLP shared activations, matrix layouts, hardware limits and training/inference paths. Use the source to reason about traffic and temporary storage.

**Why / local owner / check:** Neural NG-D04/05/07 and PGD-06/07. Derive the selected fixed operators, establish FP32 fixtures and compare one layout/fusion change against numerical and whole-frame budgets.

**Limits:** CUDA/WMMA implementation is not a directly reusable HLSL kernel. No CUDA inference service or generic tensor framework is admitted; network shape and capability differences must be proved.

**Rights route:** [BSD-style NVIDIA license; LICENSE.txt](https://github.com/NVlabs/tiny-cuda-nn/blob/3daa6e5b8b8e0fa17971a1a20acfbb3754494b38/LICENSE.txt).

### NVR-11 - Research experiment with explicit artifacts

**Source:** `NVlabs/instant-ngp@abe236ee00cf90cfca6e36e65c00435d5b21f50a`.

**Where:** [scripts/run.py](https://github.com/NVlabs/instant-ngp/blob/abe236ee00cf90cfca6e36e65c00435d5b21f50a/scripts/run.py).

**What:** Study CLI-configured scene/network inputs, snapshots and test-transform evaluation as a concrete training-to-artifact workflow.

**Why / local owner / check:** Neural NG-D03/04/08 and PGD-06/08. Record immutable splits, recipe/environment, model identity and held-out evaluation; distinguish snapshot bytes from reproducible numerical training.

**Limits:** NeRF reconstruction is a different product from the selected diffuse-indirect denoiser. Do not add NeRF/hash-grid functionality just to resemble a public portfolio.

**Rights route:** [Custom NVIDIA source-code license; LICENSE.txt; no blanket redistribution assumption](https://github.com/NVlabs/instant-ngp/blob/abe236ee00cf90cfca6e36e65c00435d5b21f50a/LICENSE.txt).

### NVR-12 - A useful HDR comparison tool

**Source:** `Tom94/tev@809b02c456787bc105e9aef0f2cd4720d9baa495`.

**Where:** [src/Image.cpp](https://github.com/Tom94/tev/blob/809b02c456787bc105e9aef0f2cd4720d9baa495/src/Image.cpp).

**What:** Read image/channel/color/orientation handling and GPU-resource release on the owning thread; the README describes a focused HDR viewing/comparison user workflow.

**Why / local owner / check:** Workload analyzer and image evidence; PGD-02/05/08. Keep raw linear data, display transform and comparison masks explicit, and join visual inspection to numeric artifacts.

**Limits:** External viewing may aid inspection; it is not an oracle or an embedded editor viewer requirement. Do not copy its implementation into Sparkle without a rights decision.

**Rights route:** [GPL-3.0 text; LICENSE.txt; file-level and distribution route must be checked on adoption](https://github.com/Tom94/tev/blob/809b02c456787bc105e9aef0f2cd4720d9baa495/LICENSE.txt).

### NVR-13 - Typed host/device ray-tracing example

**Source:** `ingowald/owl@781068838b5be423b5316e2f761942ade3e4fece`.

**Where:** [samples/cmdline/s01-simpleTriangles/hostCode.cpp](https://github.com/ingowald/owl/blob/781068838b5be423b5316e2f761942ade3e4fece/samples/cmdline/s01-simpleTriangles/hostCode.cpp).

**What:** Trace context, module, geometry declarations, buffers and launch/SBT assembly in the smallest typed example.

**Why / local owner / check:** ShaderSystem ABI and PGD-02/07/08. Explain one host-to-device parameter and geometry binding through executable reflection/layout and actual native dispatch.

**Limits:** OWL is OptiX-oriented convenience code, not a required Sparkle backend or a reason to add wrappers. API launch semantics cannot be substituted for D3D12/Vulkan contracts.

**Rights route:** [Apache-2.0; LICENSE](https://github.com/ingowald/owl/blob/781068838b5be423b5316e2f761942ade3e4fece/LICENSE).

### NVR-14 - Small product-facing image utility

**Source:** `apanteleev/FisheyeUnwarp@1645c5d9313b0daccaf28e7429e3d8e1d5ccf9ed`.

**Where:** [FisheyeUnwarp.cpp](https://github.com/apanteleev/FisheyeUnwarp/blob/1645c5d9313b0daccaf28e7429e3d8e1d5ccf9ed/FisheyeUnwarp.cpp).

**What:** Inspect a narrow Adobe plugin entry point and two user controls. The README explicitly says the implementation is not GPU accelerated.

**Why / local owner / check:** Developer technology and PGD-05/08. Define a named user problem, minimal inputs, transparent math/domain and reproducible useful output instead of a broad tools framework.

**Limits:** The claimed plugin speed is not a reproduced benchmark. This is a CPU utility precedent, not a request for Adobe integration or CUDA work.

**Rights route:** [MIT; LICENSE.txt; Adobe SDK rights are separate](https://github.com/apanteleev/FisheyeUnwarp/blob/1645c5d9313b0daccaf28e7429e3d8e1d5ccf9ed/LICENSE.txt).

### NVR-15 - Modern path-tracing integration decomposition

**Source:** `NVIDIA-RTX/RTXPT@f08d1c739071e0faad0c7c274d861124c511abab`.

**Where:** [Rtxpt/Shaders/IntroSample/IntroPathTracer.hlsl](https://github.com/NVIDIA-RTX/RTXPT/blob/f08d1c739071e0faad0c7c274d861124c511abab/Rtxpt/Shaders/IntroSample/IntroPathTracer.hlsl); [Rtxpt/Shaders/PathTracer/Lighting/LightSampler.hlsli](https://github.com/NVIDIA-RTX/RTXPT/blob/f08d1c739071e0faad0c7c274d861124c511abab/Rtxpt/Shaders/PathTracer/Lighting/LightSampler.hlsli).

**What:** Compare approachable intro transport with the full light-sampling path and the README's raw/reference, real-time and denoiser-guide separation.

**Why / local owner / check:** ReferencePathTracer recovery and Direct/Indirect discovery; PGD-02/03. Document the actual material/light/transport domain and isolate wrong raw radiance before reconstruction.

**Limits:** The sample derives from Falcor, so these are not automatically independent oracles. Vendor caches, DLSS-RR, SER and OMM are not mandatory additions; declared SDK support is not local support.

**Rights route:** [NVIDIA RTX SDKs license; LICENSE.txt; assets separately scoped](https://github.com/NVIDIA-RTX/RTXPT/blob/f08d1c739071e0faad0c7c274d861124c511abab/LICENSE.txt).

### NVR-16 - Current recording and binding comparison successor

**Source:** `nvpro-samples/vk_device_generated_cmds@2f308ba6d1c21c8c36e92a1ddb728a2318fcd734`.

**Where:** [rendererthread_vk.cpp](https://github.com/nvpro-samples/vk_device_generated_cmds/blob/2f308ba6d1c21c8c36e92a1ddb728a2318fcd734/rendererthread_vk.cpp); [threadpool.cpp](https://github.com/nvpro-samples/vk_device_generated_cmds/blob/2f308ba6d1c21c8c36e92a1ddb728a2318fcd734/threadpool.cpp).

**What:** Use the successor's reused/threaded command comparisons and README's NV versus EXT generated-command and parameter-binding variants. Separate preprocessing from whole-frame results.

**Why / local owner / check:** Workload WS-S0/WS-S2 and existing draw-preparation owner; PGD-03/04. Investigate one measured recording/binding bottleneck without changing geometry, settings or production parallel topology.

**Limits:** The sample uploads generated-command inputs from the CPU; it is not evidence of a complete GPU scene-traversal product. DGC adoption is conditional, not a prerequisite for this study.

**Rights route:** [Apache-2.0; LICENSE](https://github.com/nvpro-samples/vk_device_generated_cmds/blob/2f308ba6d1c21c8c36e92a1ddb728a2318fcd734/LICENSE).

## Authored Production And Teaching Cards

### NVR-17 - ReSTIR Mathematics Meets Production

**Where:** Wyman/Panteleev's [2021 production paper](https://research.nvidia.com/publication/2021-07_rearchitecting-spatiotemporal-resampling-production); [ReSTIR course notes](https://intro-to-restir.cwyman.org/presentations/2023ReSTIR_Course_Notes.pdf), revised 2024-03-04, especially estimator foundations, correlation/bias cautions and integration advice; Kozlowski/De Francesco's [Cyberpunk integration slides](https://intro-to-restir.cwyman.org/presentations/2023ReSTIR_Course_Cyberpunk_2077_Integration.pdf).

**What / why:** compare an estimator's theoretical assumptions with game integration, sampling budgets and content constraints. **Owner / check:** Direct `DIR-D0-06..08`, Indirect `IND-D0-05..08`, `PGD-02/03/08`; derive the actual local sample/shift/weight and test a deliberately invalid reuse boundary against an independent baseline. **Limits:** paper speedups are not Sparkle targets. DI reasoning does not establish a GI path-space estimator. Conference material rights do not grant redistribution of game assets or slides.

### NVR-18 - Shader Source Correlation And A Rejected Tradeoff

**Where:** Louis Bavoil's [2024 shader debug-info article](https://developer.nvidia.com/blog/harness-powerful-shader-insights-using-shader-debug-info-with-nvidia-nsight-graphics/), original sections on missing/available source correlation and the random-sequence instruction-cache example.

**What / why:** join source/debug data to executed bytecode before attributing stalls. A precomputed sequence changes memory and quality as well as instructions. **Owner / check:** ShaderSystem provenance, ExternalCapture symbol checks and `WS-S2`, `PGD-03/04/07`; preserve exact optimized shader identity, compare one measured liveness/code-size/layout hypothesis and retain loss/inconclusive evidence. **Limits:** the article uses a named 2023.4 tool/RTX 4080 tuple. Its flags and clock policy are precedent to verify locally. GPU Trace remains separate from frame capture; no release symbol payload is added by this card.

### NVR-19 - Shipped Ray-Tracing Live-State Investigation

**Where:** Bavoil's [2025 Indiana Jones live-state study](https://developer.nvidia.com/blog/path-tracing-optimization-in-indiana-jones-shader-execution-reordering-and-live-state-reductions/), original native-trace tables and ray-tracing live-state analysis. The original body was retrieved directly from NVIDIA when the browser fetch timed out.

**What / why:** variables live across native ray/reordering calls can create memory traffic; inspect call-site state and whole-pass consequences instead of treating a vendor feature as an automatic win. **Owner / check:** ShaderSystem/ray-tracing plus `WS-S2`, `PGD-03/04`; freeze a liveness/payload hypothesis, retain source/bytecode/native identity and compare equivalent rendering with quality, memory and whole-frame controls. **Limits:** the published game's gains are not local acceptance thresholds. SER is an optional capability, not a required optimization or a hidden fallback change.

### NVR-20 - Advanced Shader Knowledge Transferred Through Exercises

**Where:** Bickford/Hebert/Lorach's [SIGGRAPH 2025 Slang lab slides](https://developer.download.nvidia.com/ProGraphics/nvpro-samples/SlangLab/Slides.pdf), setup and hands-on shader exercises.

**What / why:** short tasks, observable output and debuggable setup let another engineer learn an advanced shader concept. **Owner / check:** `PGD-08/09`; create one focused explanation/exercise from a delivered Sparkle case, have a non-author execute it and record the confusing boundary and resulting revision. **Limits:** teaching evidence requires real feedback, not a slide deck alone. This does not authorize Slang migration, autodiff runtime or copying presentation assets.

## Delivery Reference-Use Record

Use this small record inside the existing feature discovery or iteration card, never as another result registry:

| Field | Required value |
| --- | --- |
| Local question | Named product/workload defect or hypothesis; `PGD/PGE`, current owner and stage. |
| Reference | `NVR-*`, exact commit/file/symbol or publication revision and selected passage. |
| Transfer | The mathematical/API/experimental assumption used, and differences from Sparkle. |
| Rights | Study only, independent implementation or actual copied dependency/artifact; approved source and notices if reused. |
| Falsifier | Oracle/negative control, frozen numeric rule and observation that would reject the transfer. |
| Result route | Candidate/scene/API/tool/shader identities and existing acceptance report; absence stays explicit. |
| Retirement | Replaced local path/authority, or no production change; how its removal is proved. |

```text
Research one selected NVR card for its existing Sparkle owner. Inspect the pinned source and current producer/consumer/lifetime before proposing code. Record what/where/why, exact local differences, mathematical/API assumptions, rights, falsifier and measurement identity in the owning discovery/stage. Keep release-first and feature prerequisites authoritative. Do not import a vendor framework or create a second tracker/controller/analyzer merely because the source has one. Handoff an actionable decision and bounded first slice, or unresolved owner; no local acceptance is inferred from the reference.
```
