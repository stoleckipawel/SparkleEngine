# Reference Path Tracer Stage 1 Evidence

**Status:** **`PTD-01-R2 PASS`** evidence retained and independently accepted on 2026-09-10

**Scope:** exact production-file ledger, focused command record, generated-product identities, and independent-review disposition for [Plan Stage 1](Plan.md#stage-1---establish-one-ordinary-view-mode-and-a-feature-local-owner)

**Input identity:** branch `master`; committed base `ca55e7d8aced5579a12b7058bc06788af9d333e8`; Git blob identity of `git diff --binary -- Engine Config/DefaultEngine.ini` is `567b7f170afaaebf4ce4d76b7ec38aaefa66a780`. Documentation and ignored generated products are identified separately below.

This report proves only the Stage-1 selector, ownership, clean-break, generic-progress, build-membership, and feature-enclosure claims. It is not transport, image, GPU, backend-parity, statistical, package, Shipping, or accepted-reference evidence.

## `ARCH-RPT-1` Exact Production-File Ledger

Every production/configuration path in the scoped diff is listed once. `M` means modified and `D` means deleted.

### Private feature capsule

| Status and path | Classification |
| --- | --- |
| `M Engine/Renderer/Private/Passes/Lighting/ReferencePathTracer/ReferencePathTracer.h` | Sole Private feature entry/owner; stateless Stage-1 unavailable decision. |
| `M Engine/Renderer/Private/Passes/Lighting/ReferencePathTracer/ReferencePathTracer.cpp` | Sole feature recognition and generic `Unavailable`, `0/0` production. |
| `D Engine/Renderer/Private/Passes/Lighting/ReferencePathTracer/ReferencePathTracerAccumulation.cpp` | Clean-break deletion of the rejected GBuffer-seeded accumulation route. |
| `D Engine/Renderer/Private/Passes/Lighting/ReferencePathTracer/ReferencePathTracerAccumulation.h` | Clean-break deletion of the rejected accumulation contract. |
| `D Engine/Renderer/Private/Passes/Lighting/ReferencePathTracer/ReferencePathTracerInvalidation.cpp` | Clean-break deletion of temporal-history-derived invalidation. |
| `D Engine/Renderer/Private/Passes/Lighting/ReferencePathTracer/ReferencePathTracerInvalidation.h` | Clean-break deletion of temporal-history-derived invalidation. |
| `D Engine/Renderer/Private/Passes/Lighting/ReferencePathTracer/ReferencePathTracerSample.cpp` | Clean-break deletion of the rejected GBuffer sample adapter. |
| `D Engine/Renderer/Private/Passes/Lighting/ReferencePathTracer/ReferencePathTracerSample.h` | Clean-break deletion of the rejected GBuffer sample adapter. |

### Ordinary selector and transport hooks

| Status and path | Classification |
| --- | --- |
| `M Engine/Renderer/Public/Debug/RenderViewMode.h` | One ordinary contiguous C++ view-mode row. |
| `M Engine/Assets/Shaders/Debug/RenderViewModeConstants.hlsli` | Exact HLSL mirror of the ordinary row and shifted contiguous values. |
| `M Engine/Renderer/Public/Viewport/ViewportContracts.h` | Existing per-viewport request gains ordinary `ViewMode`; existing product boundary gains feature-neutral mode/state/work progress. |
| `M Engine/Renderer/Private/Viewport/ViewportContracts.cpp` | Clears the feature-neutral progress value with the product snapshot. |
| `M Engine/Renderer/Private/View/RenderView.h` | Existing View carries only ordinary `RenderViewMode`; no feature state. |
| `M Engine/Renderer/Private/View/RenderViewBuilder.cpp` | Copies request mode into View and its existing shader uniform. |

### One frame composition hook

| Status and path | Classification |
| --- | --- |
| `M Engine/Renderer/Private/Frame/FramePipeline.h` | Permitted Private incomplete-type owner member; also removes the obsolete Lighting selector cache. |
| `M Engine/Renderer/Private/Frame/FramePipeline.cpp` | One owner construction and one unconditional feature update/publication call; no feature switch or policy. |

### Editor selection and proved generic presentation hooks

| Status and path | Classification |
| --- | --- |
| `M Engine/Editor/Private/Viewport/EditorViewportSession.h` | Existing per-viewport session owns the ordinary view-mode value. |
| `M Engine/Editor/Private/Viewport/EditorViewportSession.cpp` | Bounds and changes the session-owned view mode. |
| `M Engine/Editor/Private/UI.cpp` | Copies the session mode into the existing viewport request owner. |
| `M Engine/Editor/Public/Panels/ViewportPanel.h` | Existing viewport declares mode input and a generic progress overlay helper. |
| `M Engine/Editor/Private/Panels/ViewportPanel.cpp` | Carries mode into the request and consumes feature-neutral progress; mismatched modes are suppressed. |
| `M Engine/Editor/Private/Panels/ViewportTopPanel.cpp` | Reads/writes `EditorViewportSession`, retains exhaustive unavailable label/icon handling, and deliberately does not list the Reference Path Tracer option. |
| `M Engine/Editor/Private/Panels/RenderingSettingsPanel.cpp` | Clean-break deletion of the global Lighting selector row; no view-mode UI is stored in rendering settings. |

### Old selector, settings, frame-graph, history, and implementation clean break

| Status and path | Classification |
| --- | --- |
| `M Config/DefaultEngine.ini` | Deletes `r.Lighting.Mode`; no alias or persistence fallback. |
| `M Engine/Renderer/Private/Debug/RendererCVars.cpp` | Deletes the Lighting CVar; keeps the existing diagnostic default view-mode adapter. |
| `M Engine/Renderer/Public/Debug/RendererCVars.h` | Deletes the Lighting CVar declaration. |
| `M Engine/Renderer/Public/Settings/EngineRenderingRayTracingTypes.h` | Deletes `LightingMode`. |
| `M Engine/Renderer/Public/Settings/EngineRenderingSettings.h` | Deletes Lighting and ViewMode settings state/setters. |
| `M Engine/Renderer/Private/Settings/EngineRenderingSettings.cpp` | Deletes obsolete settings mutations. |
| `M Engine/Renderer/Private/Settings/EngineRenderingSettingsPersistence.cpp` | Deletes obsolete persisted Lighting selector. |
| `M Engine/Renderer/Private/Settings/EngineRenderingSettingsRuntime.cpp` | Deletes Lighting/ViewMode settings capture and application. |
| `M Engine/Renderer/Private/Frame/FramePipelineGraph.cpp` | Removes Lighting selector topology/provider branches and retains the ordinary ReSTIR Lit recipe. |
| `M Engine/Renderer/Private/Passes/Lighting/Lighting.cpp` | Removes the GBuffer-seeded Reference Path Tracer graph branch and restores one ordinary Lit composition. |
| `M Engine/Renderer/Private/Resources/History/FrameHistory.h` | Deletes the rejected reference accumulation history member. |
| `M Engine/Renderer/Private/Resources/History/FrameHistory.cpp` | Deletes creation/retirement/lookup of that history. |
| `M Engine/Renderer/Private/View/RenderViewState.h` | Deletes the rejected reference-history invalidation hash/API. |
| `M Engine/Renderer/Private/View/RenderViewState.cpp` | Deletes reference-history invalidation mutation. |
| `D Engine/Renderer/Private/Passes/Lighting/Direct/ReferencePathTracerDirectLighting.cpp` | Deletes rejected GBuffer direct-light producer. |
| `D Engine/Renderer/Private/Passes/Lighting/Direct/ReferencePathTracerDirectLighting.h` | Deletes rejected producer declaration. |
| `D Engine/Renderer/Private/Passes/Lighting/ReferencePathTracerIndirectLighting.cpp` | Deletes rejected GBuffer indirect-light producer. |
| `D Engine/Renderer/Private/Passes/Lighting/ReferencePathTracerIndirectLighting.h` | Deletes rejected producer declaration. |
| `D Engine/Renderer/Private/Passes/RayTracing/ReferencePathTracerAccumulationShader.h` | Deletes rejected shader parameter wrapper. |
| `D Engine/Renderer/Private/Passes/RayTracing/ReferencePathTracerDirectLightingShader.h` | Deletes rejected shader parameter wrapper. |
| `D Engine/Renderer/Private/Passes/RayTracing/ReferencePathTracerIndirectLightingShader.h` | Deletes rejected shader parameter wrapper. |
| `D Engine/Renderer/Private/RayTracing/Effects/ReferencePathTracer/ReferencePathTracerAccumulationUniformData.h` | Deletes rejected shared-history uniform. |
| `D Engine/Renderer/Private/RayTracing/Effects/ReferencePathTracer/ReferencePathTracerCVars.cpp` | Deletes feature CVar implementation. |
| `D Engine/Renderer/Private/RayTracing/Effects/ReferencePathTracer/ReferencePathTracerCVars.h` | Deletes feature CVar declaration. |
| `D Engine/Renderer/Private/RayTracing/Effects/ReferencePathTracer/ReferencePathTracerSettings.cpp` | Deletes rejected global feature settings. |
| `D Engine/Renderer/Private/RayTracing/Effects/ReferencePathTracer/ReferencePathTracerSettings.h` | Deletes rejected global feature settings. |
| `D Engine/Renderer/Private/RayTracing/Effects/ReferencePathTracer/ReferencePathTracerUniformData.h` | Deletes rejected GBuffer-seeded transport uniform. |
| `D Engine/Assets/Shaders/Passes/RayTracing/ReferencePathTracerAccumulation.hlsl` | Deletes rejected accumulation shader. |
| `D Engine/Assets/Shaders/Passes/RayTracing/ReferencePathTracerDirectLighting.hlsl` | Deletes rejected direct-light shader. |
| `D Engine/Assets/Shaders/Passes/RayTracing/ReferencePathTracerIndirectLighting.hlsl` | Deletes rejected indirect-light shader. |
| `D Engine/Assets/Shaders/RayTracing/ReferencePathTracerUniform.hlsli` | Deletes rejected shared uniform include. |
| `D Engine/Renderer/ShaderRegistrations/ReferencePathTracerAccumulationShaders.cpp` | Deletes rejected typed registration. |
| `D Engine/Renderer/ShaderRegistrations/ReferencePathTracerDirectLightingShaders.cpp` | Deletes rejected typed registration. |
| `D Engine/Renderer/ShaderRegistrations/ReferencePathTracerIndirectLightingShaders.cpp` | Deletes rejected typed registration. |

The ledger contains all 57 paths returned by `git diff --name-status -- Engine Config/DefaultEngine.ini`. No Scene, RHI, Application, frame-resource, generic job, filesystem, or feature-specific public contract was added.

## Bounded Removal Proof

Deleting the Stage-1 feature requires deleting the Private two-file capsule; removing the `FramePipeline` forward declaration/member/construction/call; removing the C++/HLSL enum row and dormant Editor label/icon cases; and, while it has no other producer, deleting the generic progress payload/getter/setter/clear/overlay. The ordinary request/View/Editor-session mode transport remains because every existing view mode uses it. The clean-break deletions above are not reversed. No second renderer, session framework, copied Scene/View state, generic RHI abstraction, or settings representation remains.

## Executed Commands And Results

All commands used PowerShell unless a working directory is stated otherwise.

| Claim | Working directory and exact command | Exit/result retained |
| --- | --- | --- |
| Renderer and Editor compile | repository root: `cmake --build build --config DevelopmentEditor --target SparkleRenderer SparkleEditor -j 8` | Exit 0. Final run compiled the affected Renderer source set including `FramePipeline.cpp`, `ReferencePathTracer.cpp`, `ViewportContracts.cpp`, then `ViewportPanel.cpp`, `ViewportTopPanel.cpp`, `UI.cpp`, and `EditorViewportSession.cpp`; both libraries linked. |
| Boundary direction | repository root: `cmake --build build --config DevelopmentEditor --target architecture_boundary_check` | Exit 0: `Architecture boundary check passed with no new violations.` |
| Shader tool membership | repository root: `cmake --build build --config DevelopmentEditor --target ShaderCompiler -j 8` | Exit 0. Rebuilt `SparkleRendererShaderCookRegistrations` and linked `ShaderCompiler.exe` after the registration deletions. |
| Shared global shader regeneration | repository root: `artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe cook` | Exit 0: `shaderTypes=32`, `compileJobs=64`, `mapEntries=64`, `uniqueCodeRecords=60`, targets `DxilSm66,SpirV16`. |
| Showcase global shader regeneration | `Projects/Showcase`: `C:\Users\stole\Documents\GitHub\SparkleEngine\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe cook` | Exit 0 with the same 32/64/64/60 counts and targets. |
| Generated clean break | repository root: `rg -a -l "ReferencePathTracerAccumulation|ReferencePathTracerDirectLighting|ReferencePathTracerIndirectLighting|ReferencePathTracerUniform" artifacts/dev/projects/Shared/cooked/Shaders artifacts/dev/projects/Showcase/cooked/Shaders` | Exit 1, interpreted as the required zero matches after both successful global cooks. |
| Exact selector/owner clean break | repository root: the retained PowerShell probe compares the ordered C++/HLSL values, requires exactly one `m_referencePathTracer->Update`, forbids a `FramePipeline` Reference Path Tracer mode comparison, requires the Editor `GetProgress()` reader, and requires zero legacy selector/name hits | Exit 0: `PASS: enum parity/order, one frame hook, real progress reader, and clean-break legacy absence`. |
| Formatting | repository root: `clang-format 22.1.3 --dry-run --Werror --style=file -- <all modified non-deleted Engine .cpp/.h/.hlsl/.hlsli paths>` using `C:\Users\stole\.vscode\extensions\ms-vscode.cpptools-1.33.8-win32-x64\LLVM\bin\clang-format.exe` | Exit 0, no diagnostics. |
| Whitespace | repository root: `git diff --check` | Exit 0; only Git working-copy CRLF conversion warnings were printed. |

## Generated Product Identities

These ignored local products are disposable runtime inputs, not committed evidence or reference output. Hashes identify the exact products checked for the clean break.

| Product | SHA-256 |
| --- | --- |
| `artifacts/dev/projects/Shared/cooked/Shaders/ShaderDependencies.sdep` | `944F57E814443DE75DBF888F3AB8A954C7FEDC4F506C6620F9E172DC4198264D` |
| `artifacts/dev/projects/Shared/cooked/Shaders/GlobalShaderMap.smap` | `ECDAAD288E7BF6011F9AD13CA04BA796A7B748C68362BB5C66426A5961ACC91C` |
| `artifacts/dev/projects/Shared/cooked/Shaders/CookedShaderLibrary.slib` | `EFDED9528D1BEBC8C2A8370BB125EFE3878DB57F4E72D3415BAA5FB332C1ED02` |
| `artifacts/dev/projects/Showcase/cooked/Shaders/ShaderDependencies.sdep` | `944F57E814443DE75DBF888F3AB8A954C7FEDC4F506C6620F9E172DC4198264D` |
| `artifacts/dev/projects/Showcase/cooked/Shaders/GlobalShaderMap.smap` | `0C3EBF48288154B45C28915075949A4E6ED5EBE6B56EBBAEDCA66E1E105306F0` |
| `artifacts/dev/projects/Showcase/cooked/Shaders/CookedShaderLibrary.slib` | `713D57B5B545BFD7A2321845FDB3F4481F139056132010C64489CD8EEE7B93EF` |

## Independent Review And Limits

- Independent architecture re-review returned **PASS** for `CHK-RPT-17`, confirmed all 57 live paths occur exactly once with no missing/extra/duplicate entry, independently reproduced the production diff identity and six generated-product hashes, and confirmed `PTD-00-R1` remains compatible.
- Independent UX/evidence re-review returned **PASS** after the pseudo-generation was removed and stale current-state claims were corrected. It verified no clickable route, truthful generic `Unavailable` `0/0`, Lit-pixel non-authority, selected-mode filtering, and settings/UI/feature separation.
- No runtime/editor launch, GPU dispatch, representative-map render, image, statistical convergence, paired-backend, accessibility interaction, Shipping/package, release-map, or accepted-reference check ran. Stage 1 makes none of those claims.

`PTD-01-R2 PASS` accepts only this exact Stage-1 production identity and evidence record. Stage 2 is authorized; later acceptance gates and every non-claim above remain binding.
