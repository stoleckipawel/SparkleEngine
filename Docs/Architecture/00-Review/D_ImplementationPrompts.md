# D. Implementation Prompt Pack

Status: ready-to-use prompt pack  
Date: 2026-06-20  
Purpose: copy/paste prompts for incremental foundation work using models with weaker reasoning depth

## Universal Rules For Every Prompt

Use these rules at the top of every implementation session unless the prompt already includes them.

```text
You are working in the SparkleEngine repository. This task is foundation/refactor/documentation work, not feature expansion.

Hard constraints:
- Do not add new rendering features.
- Do not add new third-party SDK integrations.
- Do not change runtime behavior unless the prompt explicitly asks for a validation or diagnostics hook.
- Do not remove existing architecture checks.
- Do not weaken RHI/Renderer/GameFramework/Application/Editor module boundaries.
- Prefer existing module structure, naming, and CMake conventions.
- Read the relevant source files before editing.
- Keep edits scoped to the requested deliverables.
- If you discover a larger issue, document it as a follow-up instead of solving it opportunistically.

Before editing:
- Inspect the current repository layout.
- Inspect any existing docs under Docs/Architecture.
- Inspect relevant CMake files and public headers for the target module.

After editing:
- Run a lightweight validation appropriate for the change.
- Report changed files, acceptance criteria status, and any follow-ups.
```

## Prompt 01: Architecture README And Module Map

```text
Task: Create the top-level architecture README and module dependency map for SparkleEngine.

Goal:
Make the existing engine architecture understandable to a principal-level NVIDIA/AMD rendering reviewer in the first 10 minutes.

Context to read first:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/C_FoundationStagedPlan.md
- CMake/ArchitectureBoundaryCheck.cmake
- Engine/RHI/CMakeLists.txt
- Engine/Renderer/CMakeLists.txt
- Engine/GameFramework/CMakeLists.txt
- Engine/Application/CMakeLists.txt
- Engine/Editor/CMakeLists.txt
- Tools/Launcher/SparkleLauncher/CMakeLists.txt if present
- Tools/Shaders/ShaderCompiler/CMakeLists.txt

Deliverables:
- Create or update Docs/Architecture/README.md.
- The README must include:
  - One-paragraph purpose of the architecture docs.
  - Module dependency direction.
  - A text diagram of the major modules.
  - Short module briefs for Core, Platform, RHI, Renderer, GameFramework, Application, Editor, Launcher, ShaderCompiler, and AssetCooker if present.
  - "Where to look first" section for reviewers.
  - "What this engine is not" non-goals section.
  - Links to the A/B/C/D architecture docs.
  - Links to future contract docs even if they do not exist yet, marked as planned.

Explicit architecture shape:
- RHI is below Renderer.
- Renderer may consume RHI and runtime scene data.
- GameFramework owns runtime scene/asset data and must not depend on Renderer or RHI.
- Application hosts runtime/editor lifecycle.
- Editor may inspect Renderer/RHI through tool-facing panels.
- Launcher owns workflow/tool/dependency orchestration, not renderer architecture.
- ShaderCompiler is tooling and may depend on shader contracts/RHI/renderer registrations as currently configured.

Acceptance criteria:
- Docs/Architecture/README.md exists.
- A reviewer can identify the RHI, Renderer, GameFramework, Application, Editor, Launcher, and ShaderCompiler responsibilities without opening source files.
- The dependency direction is explicit and does not contradict CMake/ArchitectureBoundaryCheck.cmake.
- The README links to A_PrincipalRoleRequirements.md, B_EngineArchitectureScorecard.md, C_FoundationStagedPlan.md, and D_ImplementationPrompts.md.
- The README names at least five future/follow-up contract docs.
- No code files are changed.
- Markdown is readable in plain text.

Validation:
- Run git diff for Docs/Architecture/README.md.
- Run a simple search to ensure the README links resolve to existing files where applicable.

Report:
- Summarize changed files.
- List acceptance criteria as pass/fail.
- List any follow-ups without implementing them.
```

## Prompt 02: Architecture Boundary Rules And ADR Index

```text
Task: Document SparkleEngine's executable architecture boundary rules and create the ADR index.

Goal:
Make the CMake architecture boundary checks understandable to a human reviewer and create a place for intentional exceptions.

Context to read first:
- CMake/ArchitectureBoundaryCheck.cmake
- Docs/Architecture/README.md if it exists
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Engine/RHI/CMakeLists.txt
- Engine/Renderer/CMakeLists.txt
- Engine/Application/CMakeLists.txt

Deliverables:
- Create Docs/Architecture/01-Boundaries/BoundaryRules.md.
- Create Docs/Architecture/01-Boundaries/ADR/README.md.
- Create Docs/Architecture/01-Boundaries/ADR/0001-renderer-native-api-provider-exceptions.md.

BoundaryRules.md must include:
- Purpose of the boundary check.
- Each enforced rule by name.
- What the rule prevents.
- Which module owns the dependency.
- How to fix a violation.
- How counted exceptions work.
- How to add a new exception only through an ADR.

ADR 0001 must include:
- Context.
- Decision.
- Current counted exception location.
- Why the exception is allowed.
- Risk.
- Retirement path.
- Acceptance criteria for removing or tightening the exception.

Acceptance criteria:
- The rule names in BoundaryRules.md match the names used in CMake/ArchitectureBoundaryCheck.cmake.
- The NVIDIA DLSS/Streamline native API exception is documented as a temporary/provider-scoped exception, not as general renderer permission.
- ADR README explains how to add future ADRs.
- No architecture rule is weakened.
- No code files are changed unless only adding a comment that links to the docs; if code comments are added, keep them minimal.

Validation:
- Run a search for boundary rule names in BoundaryRules.md and CMake/ArchitectureBoundaryCheck.cmake.
- Run git diff and confirm only docs changed unless explicitly justified.

Report:
- Changed files.
- Acceptance criteria pass/fail.
- Any boundary ambiguity discovered.
```

## Prompt 03: RHI Contract

```text
Task: Write the RHI contract document for SparkleEngine.

Goal:
Make RHI ownership, lifetime, backend responsibilities, and extension rules clear before adding more rendering features.

Context to read first:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Engine/RHI/CMakeLists.txt
- Engine/RHI/Public/**/*.h
- Engine/RHI/Private/D3D12/**/*.h and a representative set of .cpp files
- Engine/RHI/Private/Vulkan/**/*.h and a representative set of .cpp files if present
- CMake/ArchitectureBoundaryCheck.cmake

Deliverable:
- Create Docs/Architecture/02-Contracts/RHIContract.md.

Required sections:
- Purpose and non-goals.
- Public RHI surface overview.
- Backend ownership model.
- Device lifecycle.
- Adapter/backend selection.
- Resource lifetime model.
- Resource state and barrier model.
- Command list, queue, submit, and fence model.
- Descriptor ownership model.
- Pipeline and shader binding model.
- Memory allocation and budget model.
- Ray tracing service ownership.
- UI/presentation ownership.
- Diagnostics and validation ownership.
- Native handle and interop policy.
- Backend parity matrix for D3D12 and Vulkan.
- New backend checklist.
- New native interop checklist.
- Known gaps.

Explicit constraints:
- Do not invent behavior that is not visible in code. If behavior is uncertain, mark it as "Needs source confirmation" or "Planned contract".
- Do not change RHI code.
- Do not claim Vulkan parity where the source does not prove it.
- Keep D3D12/Vulkan backend-native details in backend sections, not renderer sections.

Acceptance criteria:
- RHIContract.md exists.
- It answers who owns devices, queues, command lists, fences, descriptors, resources, barriers, memory allocation, pipeline state, diagnostics, and native handles.
- It contains a backend parity table with D3D12 and Vulkan columns.
- It contains checklists for adding a backend and adding native interop.
- It distinguishes current behavior from planned/desired behavior.
- It references D3D12MA and VMA where appropriate.
- It does not require adding new rendering features.

Validation:
- Run a search for key public RHI service names and ensure the document uses source-backed names where possible.
- Run git diff and confirm the change is documentation-only.

Report:
- Changed files.
- Acceptance criteria pass/fail.
- Any source areas that need deeper follow-up.
```

## Prompt 04: Renderer Frame Graph And Pass Contract

```text
Task: Write the Renderer frame graph and pass authoring contract.

Goal:
Make renderer extension safe and predictable before adding new passes, SDK features, ray tracing features, or neural rendering paths.

Context to read first:
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Engine/Renderer/CMakeLists.txt
- Engine/Renderer/Public/**/*.h
- Engine/Renderer/Private/FrameGraph/**/*.h
- Engine/Renderer/Private/FrameGraph/**/*.cpp
- Engine/Renderer/Private/FramePipeline/**/*.h
- Engine/Renderer/Private/Passes/**/*.h
- Engine/Renderer/Private/RayTracing/**/*.h
- Engine/Renderer/Private/Temporal/**/*.h
- Engine/Renderer/Private/Diagnostics/**/*.h

Deliverable:
- Create Docs/Architecture/02-Contracts/RendererFrameGraph.md.

Required sections:
- Purpose and non-goals.
- Renderer module responsibilities.
- Relationship between Renderer, RHI, GameFramework, and Application.
- Frame lifecycle.
- Pass lifecycle.
- Pass registration and discovery.
- Resource declaration rules.
- Transient versus persistent resource ownership.
- History resource ownership.
- Render target/depth/motion vector/exposure/normal/color contracts.
- Barrier and scheduling expectations.
- GPU/CPU timing and diagnostics expectations.
- Ray tracing pass expectations.
- "How to add a pass" checklist.
- "How to add a frame resource" checklist.
- Failure modes and validation expectations.
- Known gaps.

Explicit constraints:
- Do not add a new renderer pass.
- Do not add new runtime behavior.
- Do not move code.
- If the current implementation is unclear, mark it as "Needs source confirmation".
- Keep native D3D12/Vulkan details out of this doc except when explaining why Renderer must not own them.

Acceptance criteria:
- RendererFrameGraph.md exists.
- A developer can follow the "How to add a pass" checklist without guessing which module owns resources.
- The doc names the required provider/temporal resources: color, depth, normals, motion vectors, exposure, history, jitter, camera matrices, and frame index.
- The doc states which responsibilities belong to RHI versus Renderer.
- The doc explicitly avoids feature implementation.

Validation:
- Run git diff and confirm documentation-only changes.
- Search for FrameGraph and pass-related names to ensure the doc uses existing terminology where possible.

Report:
- Changed files.
- Acceptance criteria pass/fail.
- Any unclear source areas.
```

## Prompt 05: Renderer Provider Contract

```text
Task: Write the provider contract for SDK integrations such as DLSS, Streamline, FidelityFX, denoisers, frame generation, and future neural rendering providers.

Goal:
Prevent SDK integrations from defining renderer architecture accidentally.

Context to read first:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Engine/Renderer/CMakeLists.txt
- Engine/Renderer/Private/Upscaling/NvidiaDlss/**/*.h
- Engine/Renderer/Private/Upscaling/NvidiaDlss/**/*.cpp
- Engine/RHI/Public/**/Interop*.h if present
- Engine/RHI/Public/**/*.h related to native handles/resources
- Tools/Launcher/SparkleLauncher/Private/Gui/Models/LauncherDependencyUiModel.cpp
- Tools/Launcher/SparkleLauncher/Private/Core/HostGraphicsCapabilities.cpp if present

Deliverable:
- Create Docs/Architecture/02-Contracts/RendererProviderContract.md.

Required sections:
- Purpose and non-goals.
- Provider categories: upscaler, denoiser, frame generation, ray tracing extension, neural rendering.
- Provider lifecycle.
- Capability states:
  - unavailable
  - missing dependency
  - unsupported hardware
  - available
  - enabled
  - runtime failed
- Required resource contract table:
  - color
  - depth
  - motion vectors
  - exposure
  - normals
  - history
  - jitter
  - camera matrices
  - frame index
- Backend/native handle policy.
- RHI interop policy.
- Error reporting policy.
- Debug UI/diagnostics expectations.
- Dependency sync and launcher reporting expectations.
- New provider checklist.
- Known gaps.

Explicit constraints:
- Do not add FidelityFX or a new provider implementation.
- Do not change Streamline/DLSS behavior.
- Do not let provider-native API details become general Renderer permission.
- Treat Streamline/DLSS as one provider implementation, not the architecture.

Acceptance criteria:
- RendererProviderContract.md exists.
- It defines the six capability states exactly.
- It contains the required resource contract table.
- It includes a new provider checklist.
- It states that native API usage must be backend/provider-scoped and documented through boundary rules/ADRs.
- It covers launcher/dependency reporting expectations.

Validation:
- Run git diff and confirm documentation-only changes.
- Confirm capability state names are spelled consistently.

Report:
- Changed files.
- Acceptance criteria pass/fail.
- Any follow-up code changes that should be separate tasks.
```

## Prompt 06: Shader Pipeline And ABI Document

```text
Task: Write the shader pipeline and shader ABI document.

Goal:
Make SparkleEngine's shader compiler/cook/runtime path reviewable as a principal-level tooling and rendering-system strength.

Context to read first:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Tools/Shaders/ShaderCompiler/CMakeLists.txt
- Tools/Shaders/ShaderCompiler/**/*.h
- Tools/Shaders/ShaderCompiler/**/*.cpp
- Tools/Shaders/ShaderContracts/**/* if present
- Engine/Renderer/ShaderRegistrations/**/*.h
- Engine/Renderer/ShaderRegistrations/**/*.cpp
- Engine/RHI/Public/**/*Shader*.h
- Engine/RHI/Public/**/*Pipeline*.h

Deliverable:
- Create Docs/Architecture/02-Contracts/ShaderPipeline.md.

Required sections:
- Purpose and non-goals.
- Source shader layout.
- Include closure model.
- DXC usage.
- Slang usage.
- Target formats: DXIL, SPIR-V, or source-confirmed equivalents.
- Reflection model.
- Shader contracts.
- Shader registration model.
- Cooked shader package layout.
- Cache key model.
- Runtime load and pipeline creation relationship.
- Feature/profile capability matrix.
- Ray tracing shader expectations if present.
- Future neural rendering profile gates.
- Inspection/debug commands.
- New shader feature checklist.
- Known gaps.

Explicit constraints:
- Do not change shader compiler code.
- Do not claim a target/profile is supported unless source confirms it.
- If a detail is unclear, mark it "Needs source confirmation".
- Keep future neural rendering notes as readiness notes, not implementation.

Acceptance criteria:
- ShaderPipeline.md exists.
- It traces the path from shader source to runtime pipeline use.
- It explains reflection, contracts, registration, cooked package, and cache key responsibilities.
- It contains a feature/profile matrix.
- It contains inspection/debug command placeholders or real commands if discoverable.
- It includes a new shader feature checklist.

Validation:
- Run git diff and confirm documentation-only changes.
- Search for DXC, Slang, reflection, registration, and cooked package names in source to avoid generic wording.

Report:
- Changed files.
- Acceptance criteria pass/fail.
- Any uncertain shader pipeline areas.
```

## Prompt 07: Runtime Scene Data Contract

```text
Task: Write the runtime scene data contract for GameFramework and Renderer interaction.

Goal:
Keep runtime scene/asset ownership clear so future renderer features do not couple GameFramework to RHI or backend APIs.

Context to read first:
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Engine/GameFramework/CMakeLists.txt
- Engine/GameFramework/Public/**/*.h
- Engine/GameFramework/Private/**/*.h
- Engine/Renderer/Public/**/*Scene*.h
- Engine/Renderer/Private/SceneData/**/*.h
- Engine/Application/CMakeLists.txt

Deliverable:
- Create Docs/Architecture/02-Contracts/RuntimeSceneData.md.

Required sections:
- Purpose and non-goals.
- GameFramework responsibilities.
- Renderer responsibilities.
- Asset ownership.
- Cooked asset ownership.
- Scene mutation model.
- Frame snapshot model.
- Materials, meshes, textures, cameras, lighting, skeletons.
- Threading/update assumptions.
- IDs/handles/lifetime expectations.
- What Renderer may read.
- What Renderer must not mutate.
- What GameFramework must not know about RHI.
- New runtime data type checklist.
- Known gaps.

Explicit constraints:
- Do not change GameFramework or Renderer code.
- Do not add new runtime scene features.
- Do not create Renderer/RHI dependencies in GameFramework.
- Mark uncertain behavior as "Needs source confirmation".

Acceptance criteria:
- RuntimeSceneData.md exists.
- It states that GameFramework does not depend on Renderer/RHI.
- It describes the intended boundary between mutable scene data and renderer-consumed frame data.
- It includes a new runtime data type checklist.
- It identifies known gaps without implementing them.

Validation:
- Run git diff and confirm documentation-only changes.
- Check CMake dependency direction for GameFramework.

Report:
- Changed files.
- Acceptance criteria pass/fail.
- Any boundary risks discovered.
```

## Prompt 08: Application Lifecycle And Error Taxonomy

```text
Task: Write the Application lifecycle and error taxonomy document.

Goal:
Make runtime/editor startup, backend creation, validation, project load, frame loop, and shutdown easy to reason about.

Context to read first:
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Engine/Application/CMakeLists.txt
- Engine/Application/Public/**/*.h
- Engine/Application/Private/**/*.h
- Engine/Editor/CMakeLists.txt
- Tools/Launcher/SparkleLauncher/Public/SparkleLauncher/BuildWorkspaceOperations.h
- Tools/Launcher/SparkleLauncher/Private/Build/**/*.cpp

Deliverable:
- Create Docs/Architecture/02-Contracts/ApplicationLifecycle.md.

Required sections:
- Purpose and non-goals.
- Runtime host responsibilities.
- Editor host responsibilities.
- Startup sequence.
- Tool/dependency discovery sequence.
- Backend/device creation sequence.
- Project load sequence.
- Shader cook/recook relationship.
- Main loop/frame sequence.
- Validation/capture sequence.
- Shutdown sequence.
- Error taxonomy:
  - missing SDK
  - missing source dependency
  - unsupported hardware
  - invalid project
  - shader cook failure
  - backend creation failure
  - runtime validation failure
  - editor-only failure
- New lifecycle validation checklist.
- Known gaps.

Explicit constraints:
- Do not change application code.
- Do not add new launcher actions.
- Do not invent command names; use placeholders if commands are not obvious.
- Keep runtime/editor responsibilities separate.

Acceptance criteria:
- ApplicationLifecycle.md exists.
- It includes startup, frame, and shutdown sequences.
- It includes the listed error taxonomy.
- It distinguishes runtime and editor hosts.
- It includes validation/capture expectations.

Validation:
- Run git diff and confirm documentation-only changes.
- Search for application entrypoints and use source-backed names where possible.

Report:
- Changed files.
- Acceptance criteria pass/fail.
- Unclear lifecycle points.
```

## Prompt 09: Validation Matrix

```text
Task: Create the validation matrix for SparkleEngine architecture, tools, RHI, renderer, shader, launcher, and workflow readiness.

Goal:
Make "the engine still works" demonstrable through commands, coverage, artifacts, and ownership.

Context to read first:
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/C_FoundationStagedPlan.md
- CMake/ArchitectureBoundaryCheck.cmake
- Engine/Application/Private/Validation/**/* if present
- Engine/RHI/Public/**/*Validation*.h
- Tools/Shaders/ShaderCompiler/**/*Verification* if present
- Tools/Launcher/SparkleLauncher/Private/Build/**/*.cpp
- CMakeLists.txt

Deliverable:
- Create Docs/Architecture/03-Validation/ValidationMatrix.md.

Required sections:
- Purpose and non-goals.
- Validation categories.
- Matrix columns:
  - validation name
  - purpose
  - command or planned command
  - backend coverage
  - artifact output
  - failure owner
  - current status
- Required validation rows:
  - architecture boundary check
  - configure/generate build files
  - build all
  - cook all
  - format check
  - RHI D3D12 startup/teardown
  - RHI Vulkan startup/teardown
  - swapchain/presentation smoke
  - shader compile
  - shader reflection verification
  - shader package inspection
  - renderer empty frame
  - runtime project load
  - editor startup
  - dependency sync/source availability
  - backend availability reporting
- Artifact directory policy.
- Failure triage policy.
- New validation checklist.
- Known gaps.

Explicit constraints:
- Do not implement tests yet unless the prompt is explicitly changed to ask for code.
- Do not invent fake commands. Mark unknown commands as "Planned command".
- Use source-backed command names where discoverable.

Acceptance criteria:
- ValidationMatrix.md exists.
- It contains the required matrix columns.
- It contains all required validation rows.
- Each row has a current status: existing, partial, planned, or unknown.
- It identifies failure owners by module/team area.
- It does not claim coverage that does not exist.

Validation:
- Run git diff and confirm documentation-only changes.
- Search repo for validation/build/cook command names and use accurate names when possible.

Report:
- Changed files.
- Acceptance criteria pass/fail.
- Missing commands that need future implementation.
```

## Prompt 10: Performance And Memory Diagnostics Plan

```text
Task: Create the performance and memory diagnostics plan.

Goal:
Prepare reviewable GPU/CPU/memory evidence before adding heavier rendering, SDK, ray tracing, or neural features.

Context to read first:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Engine/RHI/Public/**/*Diagnostic*.h
- Engine/RHI/Public/**/*Memory*.h
- Engine/RHI/Private/D3D12/**/*Memory*.h
- Engine/RHI/Private/Vulkan/**/*Memory*.h
- Engine/Renderer/Private/Diagnostics/**/*.h
- Engine/Editor/Private/**/*Profiler*.h
- Engine/Editor/Private/**/*Rendering*.h

Deliverable:
- Create Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md.

Required sections:
- Purpose and non-goals.
- Metrics to expose:
  - active backend
  - adapter/vendor/device
  - API version/feature level
  - validation state
  - enabled providers
  - memory budget
  - memory usage
  - residency pressure if available
  - allocation counts
  - descriptor heap/pool usage
  - pipeline cache stats
  - shader package load timing
  - pass GPU timings
  - CPU frame timings
  - upload pressure
- Data ownership by module.
- Editor display expectations.
- Text artifact expectations.
- Baseline scenarios:
  - empty frame
  - one static mesh/material
  - many materials
  - descriptor pressure
  - upload pressure
  - shader compile cache miss
  - shader compile cache hit
  - backend startup/shutdown
- New metric checklist.
- Known gaps.

Explicit constraints:
- Do not add telemetry code in this task.
- Do not add UI panels in this task.
- Do not claim metrics already exist unless source confirms them.
- Separate D3D12MA/VMA allocator-backed data from renderer-level summaries.

Acceptance criteria:
- PerformanceDiagnosticsPlan.md exists.
- It names the required metrics.
- It assigns data ownership by module.
- It includes baseline scenarios.
- It distinguishes existing, partial, planned, and unknown metrics.
- It explains how diagnostics should support principal-level review.

Validation:
- Run git diff and confirm documentation-only changes.
- Search existing diagnostics/profiler code so the doc uses current names where possible.

Report:
- Changed files.
- Acceptance criteria pass/fail.
- Existing metrics found.
- Planned metrics that require future implementation.
```

## Prompt 11: Reviewer Guide

```text
Task: Create the SparkleEngine reviewer guide.

Goal:
Package the architecture and validation path so a principal-level reviewer can evaluate the engine under time pressure.

Context to read first:
- Docs/Architecture/README.md
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/C_FoundationStagedPlan.md
- Docs/Architecture/02-Contracts/RHIContract.md if it exists
- Docs/Architecture/02-Contracts/RendererFrameGraph.md if it exists
- Docs/Architecture/02-Contracts/ShaderPipeline.md if it exists
- Docs/Architecture/02-Contracts/RendererProviderContract.md if it exists
- Docs/Architecture/03-Validation/ValidationMatrix.md if it exists

Deliverable:
- Create Docs/Architecture/00-Review/ReviewerGuide.md.

Required sections:
- Purpose.
- What to review in 10 minutes.
- What to review in 30 minutes.
- Deep-dive path.
- Build/generate/cook validation path.
- Shader inspection path.
- RHI/backend inspection path.
- Renderer/frame graph inspection path.
- SDK/provider inspection path.
- Diagnostics/performance inspection path.
- Known limitations.
- What is intentionally not implemented yet.
- How to judge future changes against the foundation.

Explicit constraints:
- Do not pretend planned docs already exist. Link existing docs normally and mark missing docs as planned.
- Do not add marketing language.
- Keep this guide direct and technical.
- Do not change code.

Acceptance criteria:
- ReviewerGuide.md exists.
- It contains 10-minute, 30-minute, and deep-dive paths.
- It links to the core architecture docs.
- It includes known limitations and non-goals.
- It tells reviewers where to inspect RHI, Renderer, ShaderCompiler, provider integration, validation, and diagnostics.

Validation:
- Run git diff and confirm documentation-only changes.
- Check links for existing docs and mark missing ones as planned.

Report:
- Changed files.
- Acceptance criteria pass/fail.
- Missing docs that should be created before the guide is considered complete.
```

## Prompt 12: Launcher Workflow Readiness

```text
Task: Document launcher workflow readiness and dependency policy.

Goal:
Make the launcher a reliable reviewer path for generating, building, cooking, validating, and understanding dependency/backend availability.

Context to read first:
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/C_FoundationStagedPlan.md
- Tools/Launcher/SparkleLauncher/Public/SparkleLauncher/BuildWorkspaceOperations.h
- Tools/Launcher/SparkleLauncher/Public/SparkleLauncher/ToolResolver.h
- Tools/Launcher/SparkleLauncher/Private/Build/**/*.cpp
- Tools/Launcher/SparkleLauncher/Private/Core/ToolResolver.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/Models/LauncherDependencyUiModel.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/Shell/LauncherMainWindowStatusPages.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/Shell/LauncherMainWindowSyncPages.cpp

Deliverable:
- Create Docs/Architecture/04-Workflows/LauncherWorkflowReadiness.md.

Required sections:
- Purpose and non-goals.
- Launcher responsibilities.
- Tool resolution responsibilities.
- CMake generator/backend relationship.
- Workflow action categories:
  - sync
  - generate
  - build
  - cook
  - clean
  - validation
- Dependency policy:
  - required
  - optional
  - hardware-gated
  - backend-gated
  - provider-gated
- NVIDIA dependency policy.
- Vulkan SDK policy.
- Reviewer workflow path.
- Failure messaging expectations.
- New workflow action checklist.
- Known gaps.

Explicit constraints:
- Do not change launcher code in this task.
- Do not add new workflow actions.
- Use current code behavior as source of truth.
- If a workflow is desirable but absent, mark it planned.

Acceptance criteria:
- LauncherWorkflowReadiness.md exists.
- It documents dependency categories and backend/provider gating.
- It covers generate, build all, cook all, format check, clean, and source sync workflows.
- It explains that Vulkan SDK absence should be shown at backend/build readiness time.
- It explains that NVIDIA SDK sync can be hardware-gated.
- It includes a new workflow action checklist.

Validation:
- Run git diff and confirm documentation-only changes.
- Search launcher source to verify action names and dependency model terms.

Report:
- Changed files.
- Acceptance criteria pass/fail.
- Follow-up launcher code improvements, if any.
```

