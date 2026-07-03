# C. Validation And Diagnostics Cleanup Map

Status: source-backed cleanup map for staged deletion review  
Date: 2026-07-03  
Scope: validation, diagnostics, smoke testing, debug artifacts, logging observers, debug view code, wrapper layers, and adjacent C++/shader duplication across the engine and tools

## Intent

This document maps the highest-value cleanup targets found in a broad repository scan. The focus is code that increases reading cost, extension cost, or build/runtime surface area without being core to the engine.

The preferred end state is a thin fatal guardrail layer plus first-class GPU debugging/profiling support:

- Keep checks that prevent corrupt data, undefined GPU access, broken package loads, invalid API results, and device/backend failures from continuing silently.
- Keep D3D12 debug layer support and Vulkan validation/debug layer support as essential graphics API development features.
- Keep GPU markers, object names, event scopes, and timestamp timing support needed by PIX, RenderDoc, and NVIDIA Nsight.
- Keep the smallest logging layer required to understand fatal startup/runtime failures.
- Remove validation flows, smoke orchestration, debug artifact systems, detailed counters, optional diagnostics, and wrapper-only abstractions that are not part of normal engine operation.

This is intentionally staged. Each group is a selectable cleanup unit with concrete files, cleanup steps, and acceptance criteria.

## Executive Read

The biggest cleanup surface is not in one huge file. It is spread across runtime/editor launch paths, launcher smoke orchestration, renderer diagnostics, RHI diagnostics, shader debug artifacts, importer counters, and duplicated debug view tables.

Approximate scan weights:

| Area | Files | Lines | Cleanup signal |
| --- | ---: | ---: | --- |
| Renderer diagnostics/debug code | 40 | 2166 | High. Smoke/report plumbing is deletable; profiler marker/timing support must be preserved and cleaned. |
| RHI diagnostics/validation code | 20 | 1963 | High. Wrapper shape can be simplified; API validation/debug layers must remain. |
| Application RHI smoke validation | 21 | 1502 | Very high. Runtime/editor host carries test harness code. |
| Importer diagnostics | 23 | 987 | Medium-high. Feature counters and logging wrappers. |
| Launcher smoke orchestration | 10 | 933 | Very high. Local process orchestration and artifact validation. |
| Shader validation/debug artifacts | 12 | 913 | Medium. Some fatal contract checks are useful, optional artifacts are noisy. |
| Shader debug view code | 8 | 539 | Medium-high. Duplicated enum/name/menu/HLSL tables. |
| Named diagnostic/debug/validation cluster total | 134 | 9003 | Large enough to justify staged deletion. |

The highest-confidence deletion is `FrameGraphPlanDiagnostics`: it is effectively a no-op diagnostics subsystem with environment-variable plumbing and a runtime call site.

Second-pass scan added more cleanup candidates outside the first diagnostics-only sweep:

- stale architecture boundary sentinels for retired tool layouts
- product-registered HelloWorld sample shaders
- PTLAS "future GPU pack" placeholder resources that appear separate from the current CPU-pack PTLAS feature
- smoke-only launcher UI/visual remnants and BMP capture plumbing
- cook plan/timing diagnostic artifacts
- unused public headers and C-style bridge headers with no external consumer

The biggest bloat cluster is the RHI smoke system. It crosses:

- `Engine/Application/Private/Validation/RhiSmoke*`
- `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/*`
- renderer smoke snapshot builders and public smoke diagnostics headers
- shader debug view names and artifact metadata

Deleting that cluster would remove a lot of surface area while preserving normal editor/runtime rendering.

## Preserve Line

Do not delete these unless there is a separate design decision:

| Keep | Why |
| --- | --- |
| `Engine/Core/Public/Diagnostics/Verify.h` | Small fatal surface: `Diagnostics::Fail`, `CheckHResult`, `CHECK(hr)`. |
| `Engine/Core/Private/Diagnostics/Verify.cpp` | Fatal write/fallback/abort/debug break implementation. |
| API result checks in RHI backends | HRESULT/VkResult failures must stay fatal. |
| D3D12 debug layer support | The engine must expose/enable native D3D12 debug-layer diagnostics for development and backend bring-up. |
| Vulkan validation/debug layer support | The engine must expose/enable native Vulkan validation layers and debug messenger output for development and backend bring-up. |
| PIX/RenderDoc/Nsight marker support | GPU event scopes, object names, debug labels, and timestamp timing are essential profiling features, not cleanup targets. |
| Shader package/header/version/bounds validation | Prevents corrupted cooked shader data from loading. |
| Asset package/header/version/bounds validation | Prevents corrupted cooked asset data from loading. |
| Truly fatal frame graph contract checks | Invalid resource usage should fail loudly, but should not require a large diagnostics subsystem. |
| Basic logger bootstrap | Keep console/file debug output if needed for fatal failures and startup diagnosis. |

The cleanup direction is not "no validation". It is "fatal guardrails plus strong graphics debugging/profiling support, with smoke/test/report-only layers deleted unless they are actively used."

## Deletion Groups

### DG-00: Remove Tracked Generated Log

Priority: immediate  
Confidence: very high  
Blast radius: tiny  
Primary value: repository hygiene

Files:

| Path | Action |
| --- | --- |
| `Projects/Showcase/StreamlineLogs/sl.log` | Delete from source control. |

Why:

- Generated log output is already ignored by `.gitignore`.
- Scan found `build`, `artifacts`, and `logs` are ignored; this one log is the visible tracked exception.
- It adds noise to reviews and risks unrelated diffs.

Cleanup steps:

1. Delete `Projects/Showcase/StreamlineLogs/sl.log`.
2. Confirm `.gitignore` continues to ignore `Projects/**/StreamlineLogs/` and `*.log`.

Acceptance criteria:

- [ ] `git ls-files Projects/Showcase/StreamlineLogs/sl.log` prints nothing.
- [ ] Running the Showcase project may recreate Streamline logs locally, but they remain untracked.
- [ ] No engine behavior changes.

Preserve option:

- None recommended.

### DG-01: Delete Dead Frame Graph Plan Diagnostics

Priority: immediate  
Confidence: very high  
Blast radius: low  
Primary value: remove dead code from a high-touch renderer path

Files:

| Path | Action |
| --- | --- |
| `Engine/Renderer/Private/FrameGraph/Diagnostics/FrameGraphPlanDiagnostics.h` | Delete. |
| `Engine/Renderer/Private/FrameGraph/Diagnostics/FrameGraphPlanDiagnostics.cpp` | Delete. |
| `Engine/Renderer/Private/FrameGraph/FrameGraph.cpp` | Remove include and `FrameGraphPlanDiagnostics::LogIfEnabled(m_compiledPlan);`. |
| `Engine/Renderer/CMakeLists.txt` or renderer source lists | Remove deleted files if listed explicitly. |

Why:

- The implementation reads `SPARKLE_FRAMEGRAPH_DIAGNOSTICS` and `SPARKLE_FRAMEGRAPH_DIAGNOSTICS_FILTER`.
- It walks the compiled frame graph and builds helper strings.
- The actual logging loops are empty, so enabling the environment variable produces no meaningful output.
- `alivePassCount` is computed but not used.
- The only visible behavior is a no-op call from `FrameGraph.cpp`.

Cleanup steps:

1. Remove the include from `FrameGraph.cpp`.
2. Remove the call to `FrameGraphPlanDiagnostics::LogIfEnabled`.
3. Delete the header and source file.
4. Remove the files from the renderer build if explicitly listed.
5. Search for `SPARKLE_FRAMEGRAPH_DIAGNOSTICS` and delete any now-dead docs/config references.

Acceptance criteria:

- [ ] `rg "FrameGraphPlanDiagnostics|SPARKLE_FRAMEGRAPH_DIAGNOSTICS"` returns no source references.
- [ ] Renderer builds.
- [ ] Running with `SPARKLE_FRAMEGRAPH_DIAGNOSTICS=1` no longer changes anything, which matches current effective behavior.
- [ ] Frame graph compile and execute behavior is unchanged.

Preserve option:

- If plan diagnostics are wanted later, rewrite them as a small explicit dump tool instead of preserving this no-op runtime subsystem.

### DG-02: Delete Application RHI Smoke Validation Stack

Priority: very high  
Confidence: high, if the goal is a lean engine host  
Blast radius: medium  
Primary value: remove test harness code from runtime/editor launch paths

Files:

| Path | Action |
| --- | --- |
| `Engine/Application/Private/Validation/RhiSmokeValidation.*` | Delete launch-level smoke entry point. |
| `Engine/Application/Private/Validation/RhiSmokeEditorValidation.*` | Delete editor smoke loop and env-var orchestration. |
| `Engine/Application/Private/Validation/RhiSmokeSession.*` | Delete smoke session wrapper. |
| `Engine/Application/Private/Validation/RhiSmokeFrameControl.*` | Delete frame stepping helper. |
| `Engine/Application/Private/Validation/RhiSmokeCameraMotion.*` | Delete camera movement helper. |
| `Engine/Application/Private/Validation/RhiSmokeLevelSwitching.*` | Delete level switching helper. |
| `Engine/Application/Private/Validation/RhiSmokeViewportCapture.*` | Delete viewport capture helper and no-op evidence logging. |
| `Engine/Application/Private/Validation/RhiSmokeCaptureArtifacts.*` | Delete JSON/CSV/BMP artifact writer. |
| `Engine/Application/Private/Validation/RhiSmokeRendererEvidence.*` | Delete renderer smoke evidence checks. |
| `Engine/Application/Private/Validation/RhiSmokeRenderViewModeNames.*` | Delete duplicated render view mode parse/name table. |
| `Engine/Application/Private/RuntimeApplicationLaunch.cpp` | Remove smoke validation include and branch. |
| `Engine/Application/Private/EditorApplicationLaunch.cpp` | Remove smoke validation include and branch. |
| `Engine/Application/CMakeLists.txt` | Remove explicit smoke/editor validation source filtering and appends. |

Why:

- The application host should launch the engine. It should not carry a large test harness.
- This stack is driven by many `SPARKLE_SMOKE_*` environment variables.
- `RhiSmokeCaptureArtifacts.cpp` manually writes JSON/CSV artifacts and duplicates rows/escaping logic.
- `RhiSmokeRenderViewModeNames.cpp` duplicates renderer enum names and aliases.
- `RhiSmokeViewportCapture.cpp` has helper functions that accept loggers but do not log meaningful evidence.
- The stack depends on renderer smoke diagnostics and debug view code, keeping other cleanup targets alive.

Cleanup steps:

1. Remove smoke branches from `RuntimeApplicationLaunch.cpp` and `EditorApplicationLaunch.cpp`.
2. Delete all `RhiSmoke*` validation files under `Engine/Application/Private/Validation`.
3. Remove corresponding CMake filters/appends from `Engine/Application/CMakeLists.txt`.
4. Remove references to `SPARKLE_SMOKE_VALIDATE_RHI`, `SPARKLE_SMOKE_*`, and RHI smoke artifact directories from docs/config/scripts.
5. Build runtime and editor application targets.

Acceptance criteria:

- [ ] `rg "RhiSmoke|SPARKLE_SMOKE"` returns no application runtime/editor launch references.
- [ ] Runtime app launches normally.
- [ ] Editor app launches normally.
- [ ] No `artifacts/validation/rhi-raytracing` output is created by normal launch.
- [ ] No renderer smoke diagnostics API is required by application code.

Preserve option:

- If this remains useful, move it out of `Engine/Application` into a separate external test executable. The engine host should expose only ordinary launch and fatal failure behavior.

### DG-03: Delete Launcher RHI Smoke Orchestration

Priority: very high  
Confidence: high, if DG-02 is accepted  
Blast radius: medium  
Primary value: remove local test matrix and artifact validation from user-facing launcher

Files:

| Path | Action |
| --- | --- |
| `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/RhiSmokeScenarioValidation.*` | Delete BMP parsing, artifact checks, log scanning, CSV/Markdown summary writing. |
| `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/RhiSmokeTestCatalog.*` | Delete hard-coded parity/benchmark suites, cases, and debug view matrices. |
| `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/RhiSmokeScenarioPlan.*` | Delete process plan/environment variable builder. |
| `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/RhiSmokeProcessRequestBuilder.*` | Delete smoke-specific process request wrapper. |
| `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/RhiSmokeLaunchOperations.*` | Delete launcher smoke operations. |
| `Tools/Launcher/SparkleLauncher/CMakeLists.txt` | Remove smoke source/header lists. |
| Launcher UI/actions that expose smoke commands | Remove commands such as ray tracing parity and PTLAS benchmark runs. |

Why:

- The launcher should help users launch/build/cook projects. It should not own a large validation suite.
- `RhiSmokeScenarioValidation.cpp` includes custom BMP parsing/diffing, log fatal marker search, metadata text checks, CSV/Markdown escaping, and benchmark summary writing.
- `RhiSmokeTestCatalog.cpp` hard-codes many view modes, suites, artifact paths, and cases.
- `RhiSmokeScenarioPlan.cpp` pushes many `SPARKLE_SMOKE_*` environment variables and render CVars into launched processes.
- This keeps debug view and smoke artifact code alive outside the engine.

Cleanup steps:

1. Remove smoke actions from launcher command/operation registration.
2. Delete `Private/Launch/Smoke/*`.
3. Remove smoke source/header groups from `Tools/Launcher/SparkleLauncher/CMakeLists.txt`.
4. Remove UI state, settings, or persisted launcher config values that exist only for RHI smoke.
5. Search docs/scripts for launcher smoke commands and remove them.

Acceptance criteria:

- [ ] `rg "RhiSmoke|SmokeRunRayTracingParity|SmokeRunPtlasBenchmark|SPARKLE_SMOKE" Tools/Launcher` returns no live code references.
- [ ] Launcher still builds.
- [ ] Launcher still supports ordinary project launch/build/cook flows.
- [ ] No BMP comparison, benchmark CSV, or smoke Markdown summary code remains.

Preserve option:

- Keep one narrow "launch with graphics API" command if useful. Do not preserve the matrix/artifact validator inside the launcher.

### DG-04: Delete Renderer Smoke Snapshot API

Priority: high  
Confidence: high, after DG-02  
Blast radius: medium  
Primary value: remove renderer API surface whose main consumer is smoke validation

Files:

| Path | Action |
| --- | --- |
| `Engine/Renderer/Public/Diagnostics/RendererSmokeDiagnostics.h` | Delete if no non-smoke consumer remains. |
| `Engine/Renderer/Public/Diagnostics/RendererSmokeRayTracingSnapshot.h` | Delete if no non-smoke consumer remains. |
| `Engine/Renderer/Private/Diagnostics/RendererSmokeDiagnosticsBuilder.*` | Delete. |
| `Engine/Renderer/Private/Diagnostics/RendererSmokeRayTracingSnapshotBuilder.*` | Delete. |
| `Engine/Renderer/Private/Frame/Core/FramePipelineDiagnostics.cpp` | Remove smoke snapshot accessors. |
| `Engine/Renderer/Public/Core/FramePipeline.h` | Remove public smoke capture methods if declared there. |
| Any application/editor smoke call sites | Should already be removed by DG-02. |

Why:

- Renderer smoke diagnostics exist primarily to feed `Engine/Application/Private/Validation/RhiSmoke*`.
- They encode a broad snapshot of ray tracing/frame graph/provider state as a validation product rather than engine behavior.
- Keeping this API encourages new runtime features to add diagnostic fields instead of being directly inspectable through normal code paths.

Cleanup steps:

1. Confirm DG-02 removed application consumers.
2. Remove `CaptureSmokeDiagnostics` style APIs from renderer public headers.
3. Delete smoke diagnostics structs and builders.
4. Remove fields from `FramePipelineDiagnostics.cpp` that exist only for smoke snapshots.
5. Remove CMake entries.

Acceptance criteria:

- [ ] `rg "RendererSmoke|CaptureSmokeDiagnostics|SmokeDiagnostics"` returns no renderer/application/tool source references.
- [ ] Renderer builds.
- [ ] Runtime/editor frame rendering behavior is unchanged.
- [ ] Ray tracing feature state remains represented by actual renderer/provider state, not a smoke snapshot object.

Preserve option:

- If a compact renderer status panel is desired, design a separate small read-only view model for editor UI. Do not preserve the smoke snapshot shape as a general renderer API.

### DG-05: Reshape RHI Diagnostics Without Removing Profiler Or API Debug Support

Priority: high  
Confidence: medium-high  
Blast radius: medium-high  
Primary value: simplify the diagnostics service shape while preserving essential graphics debugging capabilities

Files:

| Path | Action |
| --- | --- |
| `Engine/RHI/Public/Diagnostics/RhiDiagnostics.h` | Narrow and clarify the public surface; do not delete marker/object-name/timing/API debug capabilities. |
| `Engine/RHI/Private/D3D12/Diagnostics/D3D12RenderDiagnostics.cpp` | Split or simplify aggregation if noisy; preserve debug layer, PIX event, object naming, timing, and failure support. |
| `Engine/RHI/Private/Vulkan/Diagnostics/VulkanRenderDiagnostics.cpp` | Split or simplify aggregation if noisy; preserve validation layers, debug messenger/event labels, object naming, timing, and failure support. |
| `Engine/RHI/Private/D3D12/Diagnostics/D3D12DebugLayer.cpp` | Preserve. Clean message filtering/reporting quality only. |
| `Engine/RHI/Private/D3D12/Diagnostics/D3D12PixEvents.cpp` | Preserve. This is required for PIX/RenderDoc/Nsight marker visibility on D3D12. |
| `Engine/RHI/Private/Vulkan/Diagnostics/VulkanDebugEvents.cpp` | Preserve. This is required for Vulkan debug labels used by RenderDoc/Nsight. |
| `Engine/RHI/Private/Vulkan/Diagnostics/VulkanDebugNames.cpp` | Preserve object naming. Clean naming paths if duplicated. |
| `RenderHardwareInterface::GetDiagnostics()` declarations/implementations | Keep or replace with a clearer owned service boundary; do not remove essential features. |
| Backend `m_diagnostics` fields | Keep if they own debug/profiler services; split into smaller named members if the aggregate is hard to read. |

Current public hierarchy:

| Interface/type | Cleanup view |
| --- | --- |
| `RhiDiagnosticLabelColor` | Preserve for GPU markers/debug events. Simplify names/colors only if the palette is noisy. |
| `RhiTimestampQueryHandle` | Preserve for GPU timing/profiler measurement. |
| `RenderObjectDiagnostics` | Preserve object naming for PIX/RenderDoc/Nsight. Clean duplicated wrappers only. |
| `RenderTimingDiagnostics` | Preserve timestamp query service for engine profiling. |
| `RenderMessageDiagnostics` | Preserve native API debug/validation messages; improve filtering/ownership. |
| `RenderFailureDiagnostics` | Preserve device failure, crash, and live-object support where available. |
| `RenderDiagnostics` | Optional aggregate wrapper. This can be split/renamed if it obscures ownership, but must not remove capabilities. |

Why:

- The public RHI contract exposes diagnostics as a first-class service tree.
- Some aggregation/wrapper layers may make ownership harder to follow.
- Vulkan failure diagnostics currently includes no-op methods for some functions, which is a sign the abstraction may be wider than useful behavior.
- Object names, markers, timestamp queries, message polling, validation messages, live objects, and crash reports are essential engine development and profiling features.
- The cleanup target is wrapper shape, no-op methods, duplication, and unclear ownership, not capability removal.

Cleanup steps:

1. Decide whether memory diagnostics stay. If `RendererMemoryMonitor` remains, keep only a small memory stats path.
2. Audit each public diagnostics interface and mark it as profiler marker, object naming, timing, API debug/validation, failure reporting, or removable wrapper.
3. Remove only no-op methods and wrapper-only indirection.
4. Preserve fatal API result checks through `CHECK(hr)`, `Diagnostics::Fail`, and backend-specific failure paths.
5. Preserve D3D12 debug layer, Vulkan validation/debug layers, D3D12 PIX events, Vulkan debug labels, object naming, and timestamp query plumbing.
6. If the aggregate `RenderDiagnostics` remains, document it as the RHI profiler/debug service. If it is split, keep an equally discoverable replacement.

Acceptance criteria:

- [ ] PIX/RenderDoc/Nsight can still show meaningful GPU event scopes and object names for D3D12 and Vulkan.
- [ ] GPU timing remains available for engine-side profiling where supported.
- [ ] D3D12 debug layer support still works in development configurations.
- [ ] Vulkan validation/debug layer and debug messenger support still works in development configurations.
- [ ] Normal D3D12 and Vulkan rendering still works.
- [ ] Fatal HRESULT/VkResult checks still fail loudly.
- [ ] Device creation and swapchain failures still produce enough log/fatal context to diagnose startup.
- [ ] No no-op backend diagnostics implementations remain unless explicitly documented as unsupported backend capability.

Preserve option:

- Preserve the capability set by default. Only reshape ownership/API boundaries.

### DG-06: Preserve And Clean GPU Marker And Timing Stack

Priority: high  
Confidence: high
Blast radius: high  
Primary value: keep PIX/RenderDoc/Nsight support while reducing wrapper noise and unclear ownership

Files:

| Path | Action |
| --- | --- |
| `Engine/Core/Public/Diagnostics/Trace.h` | Preserve if it keeps marker naming consistent. Rename/simplify only if it is pure ceremony. |
| `Engine/Renderer/Private/Diagnostics/FrameExecutionDiagnostics.*` | Preserve GPU timing/marker coordination. Clean responsibilities and remove unused branches only. |
| `Engine/Renderer/Private/Diagnostics/ScopedGpuDiagnostics.cpp` and related headers | Preserve RAII scopes if they prevent missing end events. Simplify names/ownership if noisy. |
| `Engine/Renderer/Private/Diagnostics/PassExecutionDiagnostics.*` | Preserve pass event labeling. Remove only duplicated formatting or unused color/name layers. |
| `Engine/Renderer/Private/Diagnostics/FrameGraphExecutionDiagnostics.*` | Preserve frame graph/pass markers. Trim excessive barrier detail only if profiler captures stay useful. |
| `Engine/Renderer/Private/Debug/RendererCVars.*` | Preserve useful runtime controls for marker verbosity/GPU timing. Rename/document if unclear. |
| `Engine/RHI/Public/Commands/*` | Preserve diagnostic begin/end event and timestamp command methods needed by profilers/timing. |
| D3D12/Vulkan command list implementations | Preserve PIX events, Vulkan debug labels, and timestamp implementations. |

Why:

- Marker/timing code crosses core renderer flow because profilers need the render graph and pass structure to be visible.
- PIX, RenderDoc, and NVIDIA Nsight support is an essential engine feature.
- GPU timings are required to measure pass and frame cost while developing renderer features.
- The cleanup target is noisy naming layers, duplicate formatting, no-op paths, and unclear toggles, not the marker/timing capability itself.

Cleanup steps:

1. Define the minimum required profiler contract: frame scope, pass scope, resource/object names, backend command markers, optional barrier/detail scopes, and timestamp timing.
2. Audit labels in PIX/RenderDoc/Nsight captures and keep the names that make captures readable.
3. Remove only duplicate label builders, unused color mappings, and no-op marker/timing branches.
4. Preserve RAII/event-scope helpers where they prevent mismatched begin/end calls.
5. Preserve D3D12 PIX event and Vulkan debug-label implementations.
6. Preserve timestamp query lifecycle and GPU timing toggles where supported.
7. Document any remaining CVar controls for marker verbosity and GPU timing.

Acceptance criteria:

- [ ] PIX captures show meaningful frame/pass markers on D3D12.
- [ ] RenderDoc/Nsight captures show meaningful debug labels/object names on Vulkan and D3D12 where supported.
- [ ] GPU timestamp timing still works when enabled and supported by the backend.
- [ ] Renderer builds and renders with marker/timing objects.
- [ ] No deleted wrapper removes profiler readability or timing accuracy.
- [ ] `rg "DiagnosticName|DiagnosticGpuTiming|DiagnosticMarkerVerbosity|BeginGpu|EndGpu|TimestampQuery|PixEvents|DebugEvents"` shows intentional profiler/debug support, not dead no-op code.

Preserve option:

- Default to preserve. Future cleanup can narrow APIs, but profiler marker/timing capability must remain first-class.

### DG-07: Reduce Custom RHI Validation Wrappers While Keeping API Debug Layers

Priority: medium-high  
Confidence: medium  
Blast radius: medium-high  
Primary value: remove custom report-only validation clutter while preserving fatal checks and native API validation/debug layers

Files:

| Path | Action |
| --- | --- |
| `Engine/RHI/Public/Validation/RhiValidation.h` | Delete or reduce custom report-only helpers. Do not remove native API validation/debug layer support. |
| `Engine/RHI/Private/Validation/RhiValidation.cpp` | Delete detailed custom reporting that duplicates backend validation layers. Keep fatal helpers if needed. |
| `Engine/RHI/Private/Validation/RhiRayTracingValidation.cpp` | Delete custom report-only descriptor validation or convert critical cases to fatal checks near use. |
| `Engine/RHI/CMakeLists.txt` | Remove or rename `ENGINE_GPU_VALIDATION` only if it controls custom wrappers. Preserve config for D3D12 debug layer and Vulkan validation layers. |
| RHI call sites in resource/binding/ray tracing services | Inline fatal checks or delete optional validation calls. |
| Renderer call sites such as pipeline/frame graph contract reports | Replace with `Diagnostics::Fail` only where continuing would be unsafe. |

Why:

- `ENGINE_GPU_VALIDATION` may create a second custom validation policy layer on top of fatal API checks and native backend validation layers.
- `RhiValidation::ReportContractViolation` logs and/or asserts policy issues from several distant call sites.
- The user goal explicitly deprioritizes non-essential validation and diagnostics.
- D3D12 debug layer and Vulkan validation layers are not non-essential; they are required backend development features.

Cleanup steps:

1. Classify each validation call site as fatal safety, debug-only convenience, or redundant.
2. Move fatal safety checks next to the code that cannot safely continue.
3. Delete debug-only convenience validation.
4. Remove `ENGINE_GPU_VALIDATION` and related build definitions only if they control deleted custom wrappers.
5. Preserve explicit D3D12 debug layer and Vulkan validation-layer configuration.
6. Delete or shrink the custom validation files.

Acceptance criteria:

- [ ] Invalid cooked data, invalid API results, and impossible backend states still fail loudly.
- [ ] D3D12 debug layer can still be enabled and emits messages in development configurations.
- [ ] Vulkan validation/debug layers can still be enabled and emit messages in development configurations.
- [ ] Optional descriptor/texture/binding validation no longer creates separate runtime policy code.
- [ ] `rg "ENGINE_GPU_VALIDATION|RhiValidation"` returns no references if fully deleted.
- [ ] D3D12 and Vulkan backend tests/manual launches still render a frame.

Preserve option:

- Keep a tiny `RhiContract` helper with only fatal checks that protect memory safety or API undefined behavior. Keep native API validation/debug layers as first-class backend features. Do not keep custom logging/reporting-only validation paths that duplicate those native layers.

### DG-08: Trim Debug View Mode Matrix And Duplicate Tables

Priority: medium-high  
Confidence: high for duplication, medium for feature deletion  
Blast radius: medium  
Primary value: remove repeated enum/name/menu/shader debug tables and debug-only shader code

Files:

| Path | Action |
| --- | --- |
| `Engine/Renderer/Public/Debug/RenderViewMode.h` | Trim enum to supported view modes. |
| `Engine/Assets/Shaders/Debug/RenderViewModeConstants.hlsli` | Trim constants to match C++ enum. |
| `Engine/Application/Private/Validation/RhiSmokeRenderViewModeNames.*` | Delete with DG-02. |
| `Engine/Editor/Private/Panels/ViewportTopPanel.cpp` | Trim labels/tooltips/menu options. |
| `Engine/Assets/Shaders/Debug/ViewModes.hlsli` | Trim debug switches/functions. |
| `Engine/Assets/Shaders/Debug/PTLAS/RayTracingPtlasDebugVisualization.hlsli` | Delete if PTLAS debug views are cut. |
| `Engine/Assets/Shaders/RayTracing/RayTracingHitDebug.hlsli` | Delete if ray tracing hit debug views are cut. |
| `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuseDebug.hlsli` | Delete if indirect diffuse debug view is cut. |
| `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecularDebug.hlsli` | Delete if indirect specular debug view is cut. |
| `Engine/Assets/Shaders/Debug/RayTracingDebugModes.hlsli` | Delete or trim if ray tracing debug modes are cut. |

Why:

- View mode state is duplicated across C++ enum, HLSL constants, smoke parser/name table, and editor menu labels/tooltips.
- PTLAS and ray tracing debug modes are useful while building features but heavy for normal users.
- Debug shader code can quietly force rendering passes, descriptors, and UI to carry extra compatibility surface.

Recommended keep set:

| View mode | Keep? | Reason |
| --- | --- | --- |
| Lit | Yes | Core viewport. |
| Wireframe | Yes | Basic debugging, low cost. |
| GBuffer normals/albedo/roughness/metallic/depth | Maybe | Useful for renderer work, but can be trimmed if the editor should be lean. |
| Direct/indirect diffuse/specular debug | Maybe delete | Feature-debug specific. |
| Instance groups | Maybe delete | Narrow debugging value. |
| Ray tracing hit/PTLAS debug modes | Delete unless actively debugging PTLAS | Largest duplication and shader surface. |

Cleanup steps:

1. Choose the view modes to preserve.
2. Update C++ enum and HLSL constants in the same change.
3. Update editor menu labels/tooltips/options.
4. Delete unused debug shaders and includes.
5. Delete smoke parser/name table through DG-02.
6. Re-cook shaders.

Acceptance criteria:

- [ ] C++ enum and HLSL constants match exactly.
- [ ] Editor viewport menu contains only preserved modes.
- [ ] Shader cook succeeds with deleted debug files removed from includes/registrations.
- [ ] `rg "PtlasDebug|RayTracingHitDebug|IndirectSpecularDebug|IndirectDiffuseDebug"` returns only intentionally preserved references.

Preserve option:

- Keep PTLAS/ray tracing debug views behind a developer-only shader registration group, not a default editor menu and smoke matrix.

### DG-09: Delete Optional Shader Debug Artifact Bundle

Priority: medium  
Confidence: medium-high  
Blast radius: medium  
Primary value: reduce shader compiler/report clutter while keeping real compile/package validation

Files:

| Path | Action |
| --- | --- |
| `Tools/Shaders/ShaderCompiler/Private/Debug/ShaderDebugArtifactWriter.*` | Delete optional debug artifact bundle writer. |
| `Tools/Shaders/ShaderCompiler/Private/Debug/ShaderDebugArtifactSet.h` | Delete. |
| `Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderParameterStructCookVerifier.cpp` | Remove optional mismatch self-test/debug report path, or collapse into fatal mismatch reporting. |
| `Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderParameterStructVerifier.cpp` | Keep only actual mismatch detection; delete JSON report output if not needed. |
| Shader compiler CLI/options | Remove `write debug artifacts`, artifact directory, and forced mismatch validation flags. |
| Launcher GUI settings | Remove `ShaderWriteDebugArtifacts` and `ShaderDebugArtifactDirectory`. |
| `Engine/Application/Private/ShaderRecook/ShaderCompilerProcess.cpp` | Remove debug artifact root plumbing if only used by optional artifacts. |

Why:

- Shader compile failures must remain clear, but full request/cache/defines/preprocessed/reflection/disassembly/stderr/args artifact bundles are optional.
- Forced parameter-struct mismatch validation is a test harness path, not product behavior.
- Launcher and shader compiler both carry settings for debug artifacts, spreading the feature across tools and application code.

Cleanup steps:

1. Keep compile failure errors and shader package validation.
2. Remove debug artifact options from compiler CLI and launcher UI/settings.
3. Delete artifact writer and artifact set types.
4. Remove forced mismatch self-test path.
5. Keep parameter struct mismatch detection if it protects runtime ABI correctness.

Acceptance criteria:

- [ ] Shader compile failures still print enough message/context to fix the shader.
- [ ] Cooked shader package validation still rejects incompatible reflection/parameter layouts.
- [ ] `rg "ShaderWriteDebugArtifacts|ShaderDebugArtifactDirectory|ShaderDebugArtifact|forceParameterStructMismatch"` returns no live references.
- [ ] Shader cook succeeds.

Preserve option:

- Keep a single explicit compiler subcommand such as `inspect` or `dump` for manual shader investigation. Do not keep always-available artifact bundle plumbing in normal compile paths.

### DG-10: Collapse Shader Contract Catalog Validation If It Is Only A Report Layer

Priority: medium  
Confidence: medium  
Blast radius: medium  
Primary value: reduce shader pipeline abstractions while preserving ABI correctness

Files:

| Path | Action |
| --- | --- |
| `Tools/Shaders/ShaderContracts/Public/ShaderContractCatalog.h` | Collapse if it only feeds listing/report validation. |
| `Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractCatalogBuilder.cpp` | Delete or inline into compile/package validation. |
| `Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractValidator.cpp` | Delete report-layer validation, keep fatal ABI mismatch checks. |
| Shader compiler contract/list commands | Remove if not part of normal cook/load correctness. |

Why:

- Shader ABI validation is important, but a separate catalog/report layer may be more process than product.
- If compile/load already enforces parameter layout and reflection compatibility, a second catalog validator can become noisy.
- This should be reviewed after DG-09 because artifact/report paths may be the main consumers.

Cleanup steps:

1. Identify which contract catalog calls prevent bad cooked shaders from loading.
2. Preserve those checks inside compile/package load paths.
3. Delete report/list/catalog code that only creates additional inspection output.
4. Remove CLI options or launcher hooks that only expose catalog reports.

Acceptance criteria:

- [ ] Broken shader reflection/parameter layouts still fail the cook or package load.
- [ ] Optional contract listing/report commands are removed or clearly separated from normal compile.
- [ ] Shader cook and runtime package load still work.

Preserve option:

- Keep the catalog if it is the single source of truth for shader ABI. Delete only report/artifact writers around it.

### DG-11: Collapse Importer Diagnostics And Feature Counters

Priority: medium  
Confidence: medium-high  
Blast radius: medium  
Primary value: reduce import code wrappers and counter plumbing

Files:

| Path | Action |
| --- | --- |
| `Tools/Import/SourceImporters/Public/SourceImportDiagnostics.h` | Delete or collapse to fatal/import result messages. |
| `Tools/Import/SourceImporters/Private/SourceImportDiagnosticsRecorder.cpp` | Delete recorder/counter aggregation. |
| `Tools/Import/SourceImporters/Private/Gltf/GltfImportDiagnosticLog.cpp` | Collapse wrapper logs into direct local errors/warnings. |
| `Tools/Import/SourceImporters/Private/Fbx/FbxImportDiagnosticLog.cpp` | Collapse wrapper logs into direct local errors/warnings. |
| `Tools/Import/SourceImporters/Private/Gltf/GltfGeometryInstancingDiagnostics.cpp` | Delete detailed feature counters. |
| `Tools/Import/SourceImporters/Private/Gltf/GltfMaterialFeatureDiagnostics.cpp` | Delete detailed material feature counters. |
| `Tools/Import/SourceImporters/Private/Gltf/GltfSkinImportDiagnostics.cpp` | Delete detailed skin counters. |
| `Tools/Import/SourceImporters/Private/Gltf/GltfMorphImportDiagnostics.cpp` | Delete detailed morph counters. |

Why:

- Importers need clear fatal errors for unsupported/corrupt files.
- Detailed capability/counter diagnostics add many small files and wrapper functions.
- `GltfImportDiagnosticLog` and `FbxImportDiagnosticLog` mostly wrap logger calls with counter increments, which makes normal import code harder to read.

Cleanup steps:

1. Keep import result errors that explain why a file cannot be imported.
2. Remove feature counters and capability summary recording if not surfaced in product UI.
3. Inline important warnings locally where they occur.
4. Delete recorder and diagnostic log wrapper files.
5. Remove diagnostics objects from importer call signatures.

Acceptance criteria:

- [ ] Bad/missing/unsupported source files still produce actionable import errors.
- [ ] Importer public APIs no longer require diagnostics recorder objects for normal operation.
- [ ] `rg "SourceImportDiagnostics|DiagnosticRecorder|GltfImportDiagnosticLog|FbxImportDiagnosticLog"` returns no live references if fully deleted.
- [ ] GLTF and FBX import smoke/manual cases still import supported content.

Preserve option:

- Keep one per-import warning list in the import result if the editor displays it. Do not preserve a broad counter/capability diagnostics subsystem.

### DG-12: Remove Editor Log Observer Stream If Output Log Is Non-Core

Priority: medium  
Confidence: medium  
Blast radius: medium  
Primary value: simplify core logger and editor console coupling

Files:

| Path | Action |
| --- | --- |
| `Engine/Core/Public/Diagnostics/Logger.h` | Remove `LogRecord`, `LogRecordHandler`, `AddRecordHandler`, `RemoveRecordHandler` if no UI observer remains. |
| `Engine/Core/Private/Diagnostics/Logger.cpp` | Remove `LogObserverSink` and record handler registry. |
| `Engine/Editor/Private/Panels/OutputLogPanel.cpp` | Delete or decouple from live logger observer stream. |
| `Engine/Editor/Private/Console/EditorConsoleSystem.cpp` | Remove log subscription. |

Why:

- The core logger is otherwise a normal spdlog bootstrap with stderr/file/debug sinks.
- The observer sink makes core logging carry editor UI behavior.
- If non-essential diagnostics/logs are being cut, a live output log panel is optional.

Cleanup steps:

1. Decide whether the editor needs a live output log panel.
2. If not, delete the observer sink and record handler API.
3. Remove editor subscriptions and UI panel code.
4. Keep fatal logger bootstrap and file/stderr/debug sink behavior if desired.

Acceptance criteria:

- [ ] Fatal startup/runtime failures still print somewhere visible.
- [ ] Core logger no longer exposes observer registration API.
- [ ] Editor builds without live log stream panel.
- [ ] `rg "AddRecordHandler|RemoveRecordHandler|LogRecordHandler|LogObserverSink"` returns no live references if fully deleted.

Preserve option:

- Keep the output log panel if it is a core editor workflow. If preserved, treat it as a deliberate editor feature, not diagnostic clutter.

### DG-13: Collapse Pass Binding Override Wrapper

Priority: medium-low  
Confidence: medium  
Blast radius: high  
Primary value: remove a narrow abstraction in a high-touch pass path

Files:

| Path | Action |
| --- | --- |
| `Engine/Renderer/Private/PipelineRuntime/PassBindingOverrides.*` | Delete after inlining the one concrete override use. |
| `Engine/Renderer/Private/PipelineRuntime/PassBinder.*` | Remove override object plumbing. |
| `Engine/Renderer/Private/Passes/Deferred/GBufferMeshBatchDrawer.cpp` | Replace override object with direct descriptor binding or a small local helper. |

Why:

- Scan found the concrete override use in `GBufferMeshBatchDrawer.cpp`, where it overrides descriptor tables for mesh instances, skin influences, joint matrices, and previous joint matrices.
- `PassBindingOverrides` adds a general abstraction around what appears to be one special-case pass binding need.
- This is in a high-touch renderer path, so do not delete until diagnostics cleanup is finished.

Cleanup steps:

1. Confirm no other real users of `PassBindingOverrides` exist.
2. Inline the GBuffer mesh batch descriptor override into the drawer or pass binder with a small explicit path.
3. Delete `PassBindingOverrides`.
4. Simplify `PassBinder` signatures and binding code.

Acceptance criteria:

- [ ] `rg "PassBindingOverrides"` returns no live references.
- [ ] Skinned and non-skinned mesh rendering still works.
- [ ] GBuffer pass still binds mesh instance and skin buffers correctly.
- [ ] Pass binder is shorter and has fewer optional branches.

Preserve option:

- Preserve only if multiple passes start needing runtime descriptor substitutions. If so, document it as a pass authoring concept instead of leaving it as hidden plumbing.

### DG-14: Simplify Pass Runtime Validation Wrappers

Priority: medium-low  
Confidence: low-medium  
Blast radius: high  
Primary value: reduce pass authoring ceremony after safer deletion groups are done

Files:

| Path | Action |
| --- | --- |
| `Engine/Renderer/Private/PipelineRuntime/RenderPassDefinitionRuntime.h` | Merge or simplify validation/conversion wrapper if redundant. |
| `Engine/Renderer/Private/PipelineRuntime/RenderPassShaderRuntime.h` | Trim static helper class if it only forwards to `PipelineRuntimeLibrary`. |
| `Engine/Renderer/Private/ShaderPass/ShaderPass.h` | Review template validation and binding helpers for redundancy. |
| `Engine/Renderer/Private/PassUtilities.h` | Review broad inline helper set, especially helpers used by only one pass. |

Why:

- The pass system has useful structure, but several layers combine validation, descriptor naming, runtime conversion, and static helper forwarding.
- This is not the first place to delete because it is core to renderer extensibility.
- After smoke/diagnostics are removed, many validation-only branches may become easier to collapse.

Cleanup steps:

1. Do not start here.
2. After DG-01 through DG-11, re-scan pass runtime references.
3. Identify helpers with one caller and inline them.
4. Keep pass authoring APIs that make actual pass code clearer.
5. Remove validation/report-only helpers that no longer protect fatal contract boundaries.

Acceptance criteria:

- [ ] Adding a simple compute or graphics pass requires fewer concepts than before.
- [ ] Existing passes compile without extra wrapper-only types.
- [ ] Fatal resource/shader binding errors still fail clearly.
- [ ] No broad pass refactor happens in the same change as RHI/diagnostics deletion.

Preserve option:

- Preserve wrappers that materially reduce pass authoring mistakes. Delete wrappers that only rename or forward.

### DG-15: Remove Stale Architecture Boundary Sentinels

Priority: high  
Confidence: high  
Blast radius: low  
Primary value: remove retired-policy noise from configure/review flow

Files:

| Path | Action |
| --- | --- |
| `CMake/ArchitectureBoundaryCheck.cmake` | Remove stale checks for retired `Tools/Conversion/AssetConverter` layout. After DG-02, remove validation-directory rules that only protected the smoke stack. |
| `CMakeLists.txt` | Keep `architecture_boundary_check` only if the remaining checks are current and useful. Otherwise delete the custom target. |

Why:

- `Tools/Conversion/AssetConverter` no longer exists, but the CMake boundary script still carries a sentinel for it.
- Retired sentinels make review harder because they look like current architecture policy.
- A boundary check should protect active module ownership, not preserve memory of removed tool layouts.

Cleanup steps:

1. Delete the `Tools/Conversion/AssetConverter` scan and `NO_PARALLEL_ASSET_CONVERTER_PIPELINE` failure.
2. Re-run the architecture boundary target.
3. If the target only contains stale rules after smoke deletion, remove the target entirely.
4. If useful Renderer/RHI boundaries remain, keep them and rename comments/messages so they describe current ownership.

Acceptance criteria:

- [ ] `rg "Tools/Conversion|AssetConverter|NO_PARALLEL_ASSET_CONVERTER_PIPELINE"` returns no live references.
- [ ] `architecture_boundary_check` either passes with current repo policy or no longer exists.
- [ ] The check output describes current modules, not retired systems.

Preserve option:

- Keep the target if it continues to catch real Renderer/RHI ownership violations.

### DG-16: Delete PTLAS Future GPU-Pack Placeholder Passes

Priority: high-medium  
Confidence: medium-high  
Blast radius: medium-high  
Primary value: remove placeholder frame graph resources without deleting the working PTLAS feature

Files:

| Path | Action |
| --- | --- |
| `Engine/Renderer/Private/Frame/RayTracing/RayTracingScene.cpp` | Remove `RayTracingPtlasLogicalUpdates`, `RayTracingPtlasNativeOperationPack`, one-byte placeholder buffers, and reserved operation resources if no real GPU-pack path consumes them. |
| `Engine/Renderer/Private/Frame/RayTracing/RayTracingSceneFrameData.h` | Remove `PtlasFrameGraphResources` fields that only represent future operation buffers. |
| `Engine/Renderer/Private/Frame/RayTracing/RayTracingSceneFrameGraphResources.cpp` | Remove `HasPartitionedTlasOperationResources`/resource validity checks tied only to placeholder buffers. |
| `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.cpp` | Remove `BuildPartitionedTlasLogicalUpdateResources` and `PackPartitionedTlasNativeOperations` if they only feed future GPU pack paths. |
| `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPtlasGpuUpdateMetrics.h` | Delete or shrink metrics that only report unavailable/future GPU update paths. |
| `Engine/Renderer/Private/RayTracing/RenderRayTracingScene.cpp` | Remove capability/metric recording for unavailable GPU logical-update/native-pack paths. |
| `Engine/Renderer/Public/RayTracing/RayTracingCapabilityReport.h` | Remove capability fields that only advertise future GPU packing. |
| `Engine/Renderer/Private/RayTracing/RayTracingCapabilityReport.cpp` | Remove report formatting for future-only PTLAS GPU paths. |

Why:

- The current useful feature appears to be CPU-packed PTLAS construction.
- Separate passes and one-byte buffers exist to reserve contracts for future GPU logical-update/native-pack paths.
- Placeholder passes increase the mental model for ray tracing scene setup without providing runtime behavior today.

Cleanup steps:

1. Confirm the current active path is CPU pack inside `BuildPartitionedTlas`.
2. Delete the two future-only frame graph passes and their resources.
3. Remove metrics and capability fields that only say the future path is unavailable or reserved.
4. Keep the actual PTLAS CPU-pack path and classic TLAS fallback unchanged.
5. Re-run ray tracing scene startup/render path.

Acceptance criteria:

- [ ] `rg "RayTracingPtlasLogicalUpdates|RayTracingPtlasNativeOperationPack|kReservedPtlasContractBuffer"` returns no live references.
- [ ] `rg "SupportsGpuNativeOperationPacking|SupportsGpuLogicalUpdateRecordWrites"` returns no live references unless a real GPU implementation exists.
- [ ] PTLAS CPU-pack path still builds and renders.
- [ ] Classic non-PTLAS ray tracing path still works.

Preserve option:

- Preserve only if a near-term GPU-pack branch will land soon. If preserved, put it behind a clearly named experimental module or issue reference so reviewers know why placeholders exist.

### DG-17: Remove HelloWorld Sample Shader Registrations From Product RHI

Priority: high-medium  
Confidence: high  
Blast radius: medium  
Primary value: keep product shader bootstrap focused on real engine packages

Files:

| Path | Action |
| --- | --- |
| `Engine/RHI/Private/Shaders/HelloTriangleShaders.cpp` | Delete or move to a sample/test-only target. |
| `Engine/RHI/Private/Shaders/HelloInlineRayQueryCS.cpp` | Delete or move to a sample/test-only target. |
| `Engine/RHI/Private/Shaders/HelloRayGen.cpp` | Delete or move to a sample/test-only target. |
| `Engine/RHI/Private/Shaders/BuiltinGlobalShaders.cpp` | Remove HelloWorld package registrations from product bootstrap. |
| `Engine/RHI/Private/Shaders/ShaderAuthoring.cpp` | Stop registering sample packages during normal RHI startup if no product workflow needs them. |
| `Engine/Assets/Shaders/HelloWorld/**` | Delete or move under a samples/tests asset tree. |
| `Tools/Launcher/SparkleLauncher/Private/Gui/Shell/LauncherMainWindowOptionPages.cpp` | Remove HelloWorld shader packages from user-facing cook/package options. |

Why:

- HelloWorld shader packages are useful examples, but they are product-registered in RHI bootstrap.
- They add shader package options and source files that most engine users will read as supported runtime surface.
- Sample shaders belong in a sample/test area, not the core RHI registration path.

Cleanup steps:

1. Confirm no test, showcase, or launcher workflow depends on the Hello packages.
2. Delete product registration and launcher package entries.
3. Delete or relocate shader assets.
4. Delete RHI private sample registration files or move them to a sample-only build target.
5. Run shader package discovery/cook for real renderer packages.

Acceptance criteria:

- [ ] `rg "HelloWorld|HelloTriangle|HelloInlineRayQuery|HelloRayTracingLibrary|HelloRayGen"` returns no product references.
- [ ] Normal renderer shader packages still cook.
- [ ] Launcher no longer offers HelloWorld packages as first-class product choices.

Preserve option:

- Move samples into a clearly named sample/test target if they are still useful for backend bring-up.

### DG-18: Delete Launcher Smoke UI And Visual Remnants

Priority: high  
Confidence: high  
Blast radius: low-medium  
Primary value: finish smoke deletion beyond process orchestration files

Files:

| Path | Action |
| --- | --- |
| `Tools/Launcher/SparkleLauncher/Private/Gui/Shell/LauncherMainWindowStatusPages.cpp` | Remove smoke frame limit, view mode, capture path, level switching, PTLAS suite, parity, and benchmark controls. |
| `Tools/Launcher/SparkleLauncher/Private/Gui/Models/LauncherUiModel.cpp` | Remove smoke workflow visual/model entry. |
| `Tools/Launcher/SparkleLauncher/Assets/Visuals/workflow-smoke-test.png` | Delete smoke workflow visual asset. |
| `Tools/Launcher/SparkleLauncher/Assets/Visuals/README.md` | Remove smoke-test visual entry. |
| `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/*` | Delete with DG-03. |

Why:

- DG-03 covers smoke process orchestration, but the launcher also contains user-facing smoke configuration and a visual workflow banner.
- Leaving UI remnants after deleting the smoke backend would preserve dead user-facing settings.
- This is a high-review-value cleanup because launcher screens are interactable and frequently scanned.

Cleanup steps:

1. Delete smoke launcher backend files with DG-03.
2. Remove all smoke controls from status/options pages.
3. Delete smoke visual asset and README entry.
4. Remove persisted smoke settings and defaults if they have no remaining owner.

Acceptance criteria:

- [ ] `rg "workflow-smoke-test|SmokeFrameLimit|SmokeViewMode|SmokeCapturePath|SmokeRunRayTracingParity|SmokeRunPtlasBenchmark|smoke-test"` returns no launcher live references.
- [ ] Launcher still opens and product workflows still render.
- [ ] No empty smoke/status section remains.

Preserve option:

- Preserve only if smoke testing remains a first-class launcher workflow. In that case, keep the UI and do not delete DG-02/DG-03.

### DG-19: Delete RHI/Renderer BMP Capture Service If It Only Serves Smoke

Priority: high after DG-02/DG-03  
Confidence: medium-high  
Blast radius: medium-high  
Primary value: remove backend readback/BMP code that exists for smoke artifacts

Files:

| Path | Action |
| --- | --- |
| `Engine/RHI/Public/Capture/RhiCaptureService.h` | Delete capture request/result/service API if no product screenshot feature remains. |
| `Engine/RHI/Public/Device/RenderHardwareInterface.h` | Remove `GetCaptureService`. |
| `Engine/RHI/Private/D3D12/Capture/D3D12CaptureService.*` | Delete D3D12 capture service wrapper. |
| `Engine/RHI/Private/Vulkan/Capture/VulkanCaptureService.*` | Delete Vulkan capture service wrapper. |
| `Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp` | Remove `CaptureTextureToBmp`/`WriteDiagnosticBmp` readback path if only smoke used it. |
| `Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.cpp` | Remove `CaptureTextureToBmp`/`WriteRgbaFloatDiagnosticBmp` readback path if only smoke used it. |
| `Engine/Renderer/Public/Renderer.h` | Remove `CaptureViewportProductToBmp`. |
| `Engine/Renderer/Private/Renderer.cpp` | Remove capture forwarding. |
| `Engine/Renderer/Private/FramePipeline/FramePipelineViewportProducts.cpp` | Remove viewport product BMP capture helper. |
| `Engine/Application/Private/Validation/RhiSmokeViewportCapture.cpp` | Delete with DG-02. |

Why:

- The capture path appears tied to smoke BMP artifacts such as `logs/smoke/scene-color.bmp`.
- It adds public RHI API, backend services, renderer forwarding, image encoding, readback synchronization, and launcher settings.
- If the engine does not have a product screenshot/export workflow, this is diagnostics-only infrastructure.
- This group targets engine-owned BMP/readback artifact capture only. It must not remove PIX/RenderDoc/Nsight marker visibility, object naming, timestamp timing, native API debug layers, or external profiler capture support.

Cleanup steps:

1. Delete smoke capture call sites first.
2. Confirm no editor screenshot/product capture workflow calls `CaptureViewportProductToBmp`.
3. Remove renderer capture forwarding and RHI capture service API.
4. Delete backend capture service files and backend readback-to-BMP helpers.
5. Remove build file entries and includes.

Acceptance criteria:

- [ ] `rg "RhiCaptureService|RhiTextureCaptureRequest|RhiCaptureResult|CaptureTextureToBmp|CaptureViewportProductToBmp|WriteDiagnosticBmp|WriteRgbaFloatDiagnosticBmp"` returns no live references.
- [ ] D3D12 and Vulkan RHI no longer allocate capture service objects.
- [ ] Normal viewport presentation still works.

Preserve option:

- Preserve only if there is a real editor screenshot/export feature. If so, rename it away from diagnostics/smoke language and make it product-owned.
- Preserve all external profiler/debugger support regardless of this decision.

### DG-20: Consolidate Scene Manifest Validators Without Weakening Fatal Checks

Priority: medium  
Confidence: high for reorganization, low for deletion  
Blast radius: medium  
Primary value: reduce tiny-file validator fragmentation in a high-use asset loading path

Files:

| Path | Action |
| --- | --- |
| `Engine/GameFramework/Private/Assets/Loaders/SceneManifestFeatureValidator.*` | Fold into one scene manifest validation implementation. |
| `Engine/GameFramework/Private/Assets/Loaders/SceneManifestInstanceGroupValidator.*` | Fold into one scene manifest validation implementation. |
| `Engine/GameFramework/Private/Assets/Loaders/SceneManifestInstanceValidator.*` | Fold into one scene manifest validation implementation. |
| `Engine/GameFramework/Private/Assets/Loaders/SceneManifestMaterialVariantValidator.*` | Fold into one scene manifest validation implementation. |
| `Engine/GameFramework/Private/Assets/Loaders/SceneManifestMeshReferenceValidator.*` | Fold into one scene manifest validation implementation. |
| `Engine/GameFramework/Private/Assets/Loaders/SceneManifestMeshValidator.*` | Fold into one scene manifest validation implementation. |
| `Engine/GameFramework/Private/Assets/Loaders/SceneManifestMetadataValidator.*` | Fold into one scene manifest validation implementation. |
| `Engine/GameFramework/Private/Assets/Loaders/SceneManifestValidator.*` | Keep as the public internal entry point; move helper functions here or into `SceneManifestLoaderValidation.cpp`. |
| `Engine/GameFramework/Private/Assets/Loaders/CookedAssetLoaderDiagnostics.*` | Optional follow-up: keep only if shared load-error formatting remains valuable. |

Why:

- The manifest checks are useful fatal guardrails and should stay.
- The current implementation spreads small validation helpers across many tiny files and headers.
- This makes review and extension harder than a single cohesive validation implementation with internal helper functions.

Cleanup steps:

1. Preserve the actual invalid magic/version/count/reference checks.
2. Move validator helper implementations into one `.cpp` file.
3. Delete tiny validator headers and sources that only served as internal splits.
4. Keep one internal header only if multiple loader translation units need it.
5. Add a focused invalid-manifest smoke/unit check if one exists in the repo test style.

Acceptance criteria:

- [ ] `rg "SceneManifestFeatureValidator|SceneManifestMeshReferenceValidator|SceneManifestInstanceValidator|SceneManifestInstanceGroupValidator|SceneManifestMetadataValidator|SceneManifestMaterialVariantValidator|SceneManifestMeshValidator"` returns no live references.
- [ ] Invalid manifest header/version/count/reference failures still produce clear loader errors.
- [ ] Scene manifest loading succeeds for existing cooked showcase data.

Preserve option:

- Preserve the checks. The cleanup target is file and abstraction sprawl, not safety.

### DG-21: Remove AssetCooker Plan And Timing Diagnostic Artifacts

Priority: high-medium  
Confidence: medium-high  
Blast radius: medium  
Primary value: keep cooking output focused on assets and fatal failures

Files:

| Path | Action |
| --- | --- |
| `Tools/Cooking/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp` | Remove stage timing structs, summary JSON writing, top timing printouts, and feature/counter summaries if they are diagnostics-only. |
| `Tools/Cooking/AssetCooker/Private/Discovery/AssetCookerDiscovery.cpp` | Stop writing `asset-cooker-plan-v1` plan diagnostics and diagnostic summary paths. |
| `Tools/Cooking/AssetCooker/Private/Discovery/AssetCookerDiscovery.h` | Remove plan/summary path fields from cook plan types if unused after deletion. |
| `Tools/Cooking/AssetCooker/Private/Cli/AssetCookerCli.cpp` | Remove CLI options/output that only exist to expose plan/timing diagnostics. |

Why:

- Cook plans currently write diagnostic artifacts under `artifacts/diagnostics/cook/Plans` and `Summaries`.
- The dispatcher also records detailed per-stage timing summaries and feature/counter summaries.
- These are useful during tool development, but they add durable artifact schemas and code paths to the normal cook path.

Cleanup steps:

1. Decide whether any downstream workflow consumes `asset-cooker-plan-v1` or `asset-cooker-summary-v1`.
2. If not, delete plan text and summary JSON writers.
3. Keep direct console errors for failed import/cook operations.
4. Keep actual cooking outputs and dependency discovery.
5. Remove no-longer-used plan path fields.

Acceptance criteria:

- [ ] `rg "asset-cooker-summary-v1|asset-cooker-plan-v1|artifacts.*diagnostics.*cook|planPath|summaryPath|textureSummaryPath|AssetCookerStageTiming"` returns no live references unless intentionally preserved.
- [ ] A normal project cook still writes expected cooked assets.
- [ ] Failed cooks still report the failing source/path and reason.

Preserve option:

- Preserve one opt-in `--diagnostics` path if these artifacts are actively used, but keep it out of the default cook path.

### DG-22: Collapse AssetCooker Public Bridge And Capability Self-Report

Priority: medium-high  
Confidence: medium  
Blast radius: medium  
Primary value: remove API-shaped code around a single CLI-owned tool

Files:

| Path | Action |
| --- | --- |
| `Tools/Cooking/AssetCooker/Public/AssetCookResult.h` | Delete; scan found no live references. |
| `Tools/Cooking/AssetCooker/Public/AssetCookRequest.h` | Move private or merge into service/CLI if there is no external consumer. |
| `Tools/Cooking/AssetCooker/Public/AssetCookerTypes.h` | Move private or merge into service/CLI; remove unused `diagnosticsPath`. |
| `Tools/Cooking/AssetCooker/Private/Service/AssetCookerService.*` | Simplify public C-style request/result bridge if only the CLI calls it. |
| `Tools/Cooking/AssetCooker/Private/Cli/AssetCookerCli.cpp` | Remove `capabilities` command if it only prints hardcoded self-report flags. |
| `Tools/Cooking/AssetCooker/CMakeLists.txt` | Remove public header install/source entries that are no longer public API. |

Why:

- `AssetCookerCore` appears linked by the AssetCooker executable, not consumed as a stable external SDK.
- `AssetCookResult.h` is unused.
- Capability fields such as selected recook/hot reload are self-reported by the tool and can imply features that are not separate, product-owned workflows.
- Keeping request/result/capability headers public makes the tool look more extensible than it currently is.

Cleanup steps:

1. Confirm no external project links `AssetCookerCore` as an API.
2. Delete unused `AssetCookResult.h`.
3. Move request/config/result structs into `Private` or inline them into the CLI/service boundary.
4. Remove `diagnosticsPath` if no code uses it.
5. Remove hardcoded capability query/CLI command if it has no consumer.
6. Make `recook` either a real user workflow or a plain CLI alias to existing category cook behavior.

Acceptance criteria:

- [ ] `rg "AssetCookResult|diagnosticsPath|AssetCookerCapabilities|supportsSelectedRecook|supportsHotReloadOutputs"` returns no live references unless deliberately preserved.
- [ ] AssetCooker still supports actual cook commands.
- [ ] The public include surface of AssetCooker is smaller or gone.

Preserve option:

- Preserve public headers only if another executable/library includes them today or there is a committed plugin/API plan.

### DG-23: Delete Unused Public Headers

Priority: high  
Confidence: high  
Blast radius: low  
Primary value: reduce misleading public API surface

Files:

| Path | Action |
| --- | --- |
| `Engine/Core/Public/CoreMacros.h` | Delete if `SPARKLE_PP_CONCAT` remains unused. |
| `Engine/Core/Public/CoreMinimal.h` | Delete if no source includes it. |
| `Engine/Core/Public/Input/Mouse/MouseWheel.h` | Delete unused `MouseWheelAxis`, `MouseWheelState`, and wrapper type. |
| `Engine/GameFramework/Public/Assets/AssetId.h` | Delete unused `AssetId` and `_asset` literal if the engine uses `CookedAssetId`/strings instead. |
| `Tools/Cooking/AssetCooker/Public/AssetCookResult.h` | Delete with DG-22 or independently. |

Why:

- Public headers communicate supported extension surface.
- These headers currently appear to have zero live includes or only self-contained definitions.
- Deleting them removes misleading affordances before they become accidental dependencies.

Cleanup steps:

1. Re-run reference search immediately before deletion.
2. Delete headers with no live includes or symbol references.
3. Remove CMake/source-list entries if present.
4. Build targets that own the deleted public include directories.

Acceptance criteria:

- [ ] `rg "CoreMinimal.h|CoreMacros.h|MouseWheelState|MouseWheelAxis|class AssetId|operator\"\"_asset|AssetCookResult"` returns no live references.
- [ ] No replacement aggregate header is added.
- [ ] Engine/Core, GameFramework, and AssetCooker targets still compile.

Preserve option:

- Preserve only if an external include contract exists. If so, add a comment or doc entry that explains the public API owner.

### DG-24: Normalize Or Remove Root CMake Format Check

Priority: low-medium  
Confidence: medium  
Blast radius: low  
Primary value: avoid half-current quality tooling

Files:

| Path | Action |
| --- | --- |
| `CMakeLists.txt` | Either remove `SPARKLE_RUN_CLANG_FORMAT_ON_BUILD`/`clang_format_check`, or update it to match current repo layout. |
| `Tools/Launcher/SparkleLauncher` quality/format workflow | Prefer one canonical formatting workflow if launcher already owns quality tasks. |

Why:

- The root CMake formatting target scans `Engine` and `Projects`, but not `Tools`.
- It excludes `/third_party/` lower-case while the repo uses `ThirdParty` paths.
- A partial formatting check gives a false sense of coverage and adds configure/build complexity.

Cleanup steps:

1. Decide whether formatting belongs in root CMake or the launcher quality workflow.
2. If root CMake keeps it, include `Tools` deliberately and fix third-party filtering.
3. If launcher quality is canonical, delete the root build-time format option/target.
4. Document the single command reviewers should use.

Acceptance criteria:

- [ ] There is one obvious formatting workflow.
- [ ] It either covers `Engine`, `Projects`, and `Tools`, or clearly states its intended subset.
- [ ] Third-party paths are excluded by actual repo path casing.

Preserve option:

- Keep root CMake formatting if it is required by CI. In that case, make it accurate rather than partial.

## Cross-Cutting Duplicate Patterns

### Environment Variable Control Surface

Cleanup candidates:

| Prefix/name | Current role | Suggested action |
| --- | --- | --- |
| `SPARKLE_SMOKE_*` | Application/launcher smoke validation | Delete with DG-02/DG-03. |
| `SPARKLE_FRAMEGRAPH_DIAGNOSTICS` | Dead frame graph plan diagnostics | Delete with DG-01. |
| `SPARKLE_FRAMEGRAPH_DIAGNOSTICS_FILTER` | Dead frame graph plan diagnostics filter | Delete with DG-01. |
| `SPARKLE_LOG_LEVEL` | Basic logger | Keep if useful. |
| `SPARKLE_LOG_FILE` | Basic logger file path | Keep if useful. |

Acceptance criteria:

- [ ] Environment variables that remain are documented or obvious from startup code.
- [ ] Removed environment variables have no source references.

### Debug View Duplication

Current repeated tables:

| Table | Location |
| --- | --- |
| C++ enum | `Engine/Renderer/Public/Debug/RenderViewMode.h` |
| HLSL constants | `Engine/Assets/Shaders/Debug/RenderViewModeConstants.hlsli` |
| Smoke parser/name table | `Engine/Application/Private/Validation/RhiSmokeRenderViewModeNames.cpp` |
| Editor labels/tooltips/options | `Engine/Editor/Private/Panels/ViewportTopPanel.cpp` |

Cleanup rule:

- If a view mode remains, it must be declared once as the source of truth or trimmed consistently across all tables.
- If smoke is deleted, remove the smoke table entirely.

### Diagnostics As Public Product API

The repo repeatedly turns observation behavior into public API:

| API surface | Suggested direction |
| --- | --- |
| RHI `RenderDiagnostics` aggregate | Reshape or split if noisy, while preserving API debug layers, profiler markers, object names, and timing. |
| Renderer smoke snapshot headers | Delete with smoke stack. |
| RHI capture/BMP service | Delete if its only owner is smoke capture. |
| Core log observer handlers | Delete if output log is non-core. |
| Import diagnostics recorder | Delete if not surfaced as product import result. |
| Shader debug artifact writer | Delete from normal compile flow. |
| AssetCooker plan/timing schemas | Delete from default cook path unless a workflow consumes them. |
| AssetCooker capability self-report | Collapse if the cooker is CLI-private. |

Cleanup rule:

- Public APIs should describe engine behavior, not optional inspection workflows.

## Suggested Execution Order

Recommended order:

1. DG-00: Remove tracked generated log.
2. DG-15: Remove stale architecture boundary sentinels.
3. DG-23: Delete unused public headers.
4. DG-01: Delete dead frame graph plan diagnostics.
5. DG-02: Delete application RHI smoke validation stack.
6. DG-03: Delete launcher RHI smoke orchestration.
7. DG-18: Delete launcher smoke UI and visual remnants.
8. DG-04: Delete renderer smoke snapshot API.
9. DG-19: Delete RHI/renderer BMP capture service if it only serves smoke.
10. DG-08: Trim debug view mode matrix after smoke references are gone.
11. DG-17: Remove HelloWorld sample shader registrations from product RHI.
12. DG-09: Delete optional shader debug artifact bundle.
13. DG-21: Remove AssetCooker plan and timing diagnostic artifacts.
14. DG-22: Collapse AssetCooker public bridge and capability self-report.
15. DG-11: Collapse importer diagnostics and counters.
16. DG-12: Decide whether editor output log observer is core.
17. DG-05: Reshape RHI diagnostics hierarchy without removing profiler/API debug support.
18. DG-06: Clean GPU marker/timing stack while preserving PIX/RenderDoc/Nsight support.
19. DG-07: Reduce custom RHI validation wrappers while keeping D3D12/Vulkan debug layers.
20. DG-16: Delete PTLAS future GPU-pack placeholder passes after confirming CPU-pack PTLAS remains intact.
21. DG-20: Consolidate scene manifest validators without weakening fatal checks.
22. DG-10: Collapse shader contract catalog validation if it is only a report layer.
23. DG-24: Normalize or remove root CMake format check.
24. DG-13: Collapse pass binding override wrapper.
25. DG-14: Simplify pass runtime validation wrappers.

Rationale:

- Start with generated/stale/no-op and unused public surface.
- Then remove smoke test-harness code and all UI/API artifacts that only served it.
- Then delete APIs whose only consumer was the test harness.
- Then trim debug shader/view matrices.
- Then clean tool diagnostics and CLI-private bridge layers.
- Then clean broader RHI/renderer diagnostics only where profiler/API debug capability remains intact.
- Leave feature-adjacent PTLAS placeholder cleanup until after the smoke/diagnostics surface is quiet.
- Leave pass-runtime simplification for last because it touches the most interactable renderer authoring path.

## Thin Fatal Layer Target

After accepted cleanup groups, the desired validation/diagnostic shape is:

| Layer | Keep |
| --- | --- |
| Core diagnostics | `Diagnostics::Fail`, `CHECK(hr)`, basic logger. |
| RHI | Fatal API result checks, fatal device/backend creation failures, D3D12 debug layer support, Vulkan validation/debug layer support, profiler object names/markers/timing, minimal memory safety guards. |
| Renderer | Fatal frame graph/resource contract failures where continuing would be unsafe, plus profiler-visible frame/pass markers and GPU timing. |
| Shader pipeline | Cook/load ABI checks that prevent bad packages from running. |
| Import/cook tools | Clear import/cook failures for unsupported or corrupt input. |
| Editor/launcher | Product workflows only. Optional test matrices and debug artifacts live outside core tools. |

Profiler/API validation support is part of the target shape, not an exception. Everything else should need a specific owner and a current workflow to justify staying.

## Not Marked For Deletion

These appeared during the scan but are not marked as cleanup targets here:

| Area | Reason |
| --- | --- |
| `Engine/RHI/Private/D3D12/ThirdParty/d3dx12.h` | Third-party header, large but expected. |
| Basic file/path/logger infrastructure | Needed by many tools and runtime startup. |
| Cooked package validation | Protects against corrupted data and ABI mismatch. |
| Scene manifest validation checks | Keep the fatal checks; DG-20 targets file fragmentation. |
| Core frame graph resource contract checks | Fatal correctness checks are aligned with the requested preserve line. |
| PIX/RenderDoc/Nsight marker and timing support | Essential profiling/debugging feature; cleanup may simplify wrappers but must preserve capability. |
| D3D12 debug layer support | Essential backend validation/debug feature. |
| Vulkan validation/debug layer support | Essential backend validation/debug feature. |
| Current PTLAS CPU-pack implementation | DG-16 targets future GPU-pack placeholders, not the working PTLAS feature. |
| Normal AssetCooker outputs | DG-21 targets diagnostic plan/timing artifacts, not cooked assets. |
| Actual renderer/RHI feature implementation | This document targets diagnostic/test/wrapper bloat, not feature deletion unless the feature is debug-only. |

## Review Checklist For Each Accepted Group

Before deleting a group:

- [ ] Confirm the group has no active product workflow owner.
- [ ] Confirm fatal errors still fail loudly after the group is gone.
- [ ] Identify files/classes/functions touched by the cleanup and decide whether each still has enough responsibility to remain.
- [ ] Remove build file entries in the same change as deleted files.
- [ ] Remove docs/config/env vars/settings that only served the deleted group.
- [ ] Build at least the touched target.
- [ ] Run or manually exercise one normal editor/runtime path if the group touched launch/rendering.

After deleting a group:

- [ ] Search for the group name and related env vars.
- [ ] Search for deleted file basenames.
- [ ] Check that source lists do not mention deleted files.
- [ ] Check that no replacement abstraction was added unless it removed more complexity than it introduced.
- [ ] Re-scan modified files for leftover shells: empty classes, one-function wrapper files, pass-through helpers, unused methods, and types whose names now overstate their remaining job.
- [ ] Inline, merge, or delete leftover shells unless they are preserving a clear ownership boundary used by current code.

## First Cuts I Would Personally Approve

If the goal is fast, high-confidence cleanup, approve these first:

1. DG-00: Delete `Projects/Showcase/StreamlineLogs/sl.log`.
2. DG-15: Remove the stale `Tools/Conversion/AssetConverter` CMake sentinel.
3. DG-23: Delete unused public headers after one last reference scan.
4. DG-01: Delete `FrameGraphPlanDiagnostics`.
5. DG-02: Delete `Engine/Application/Private/Validation/RhiSmoke*`.
6. DG-03: Delete launcher `Private/Launch/Smoke/*`.
7. DG-18: Delete launcher smoke UI and visual remnants.
8. DG-04: Delete renderer smoke snapshot API.

Those groups remove the clearest bloat and unblock a cleaner second pass on debug view modes, shader artifacts, RHI capture, and broader diagnostics.
