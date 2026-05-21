# Sparkle Launcher Design Proposal

## Purpose

Sparkle Launcher should become the normal operator-facing front door for SparkleEngine. The goal is to remove the need to remember batch scripts, environment variables, build profiles, target names, cook modes, and log locations for common engine work.

The launcher is not a menu over scripts. The production direction is to move workflow orchestration into typed C++ engine tooling and reserve scripts only for unavoidable bootstrap or platform handoff cases.

## Current Friction

The repository currently has a workable but script-heavy command surface:

- `SetupWorkspace.bat` validates tools and prepares the workspace.
- `GenerateSolution.bat` owns CMake generator/toolset selection.
- `BuildProject.bat` builds editor/runtime targets from profile names.
- `CookAllAssets.bat` performs full cook orchestration through `AssetCooker`.
- `Scripts/Cook/CookShaders.bat`, `CookTextures.bat`, and `CookAssets.bat` provide focused cook entrypoints.
- `RunClangFormat.bat` and `CleanWorkspace.bat` handle maintenance tasks.

The script layer hides many details, but users still need to know which script to run, what arguments mean, where logs go, when build files are stale, and which profile maps to which executable or tool target.

Sparkle Launcher should turn those flows into discoverable actions with explicit state, progress, logs, and safe defaults.

## Chosen User Experience: Project-Centric Launcher

The chosen product direction is the project-centric launcher surface. The first screen should answer three questions quickly:

- What Sparkle projects exist in this repository?
- What is the selected project's readiness state?
- What operation should I run next?

The launcher opens to a project list with compact project tiles. Selecting a project reveals grouped operations for Setup, Build, Cook, Quality, Clean, and Launch. This keeps the mental model simple: users choose a project first, then choose what they want to do with that project.

Primary layout:

```text
+--------------------------------------------------------------------------------+
| Sparkle Launcher                                                               |
+--------------------------------------------------------------------------------+
| Projects                                                                       |
| +----------------------+ +----------------------+ +----------------------+      |
| | Showcase             | | Sandbox              | | TemplateProject       |      |
| | DevelopmentEditor    | | DebugEditor          | | Not configured        |      |
| | Last built: today    | | Last built: 2d ago   | | Setup required        |      |
| +----------------------+ +----------------------+ +----------------------+      |
|                                                                                |
| Selected: Showcase                                                             |
| +--------------------+--------------------+--------------------+-------------+ |
| | Setup              | Build              | Cook               | Maintenance | |
| +--------------------+--------------------+--------------------+-------------+ |
| | Setup Workspace    | Compile Editor     | Cook All Assets    | Clean       | |
| | Generate Solution  | Compile Runtime    | Cook Shaders       | Format      | |
| | Check Toolchain    | Build Cook Tools   | Textures / Meshes  | Validate    | |
| +--------------------+--------------------+--------------------+-------------+ |
|                                                                                |
| Recent Activity                                                                |
|  23:54  Cook Shaders failed: GBuffer SpirV16 binding mismatch                  |
|  23:41  TextureCooker built successfully                                       |
+--------------------------------------------------------------------------------+
```

Default selectors:

- Project: `Showcase` when present, otherwise first discovered `.sparkle-project`.
- Editor configuration: `DevelopmentEditor`.
- Runtime/cook configuration: `DevelopmentGame`.
- RHI/backend: repository default, with D3D12/Vulkan selection available in advanced project controls.
- Cook mode: incremental by default; force recook as an explicit destructive toggle.
- Toolchain: newest supported Visual Studio generator by default, with advanced override hidden behind settings.

The launcher should feel like a production project manager, not a generic dashboard. Project tiles may use compact cards because they represent repeated selectable items. Page sections should stay unframed and operational: tables, status chips, icon buttons, compact controls, and a persistent log/activity viewer.

## Project Tile Model

Each project tile should summarize enough state that the user can pick the right project without opening logs:

- Project name.
- Preferred profile or last used profile.
- Last successful editor build time.
- Last successful asset cook time.
- Current readiness state: ready, needs setup, stale build files, cook failed, build failed, or blocked.
- Latest issue summary when relevant.

Example states:

```text
+----------------------+
| Showcase             |
| DevelopmentEditor    |
| [Ready]              |
| Built: today 23:41   |
| Cooked: today 23:52  |
+----------------------+

+----------------------+
| Sandbox              |
| DevelopmentGame      |
| [Needs Setup]        |
| Build files missing  |
| Action: Setup        |
+----------------------+

+----------------------+
| Showcase             |
| DevelopmentGame      |
| [Cook Failed]        |
| GBuffer / SpirV16    |
| Action: Open Log     |
+----------------------+
```

## Selected Project Workspace

After a project is selected, the lower workspace becomes the main control surface. It should group operations by user intent, not by executable or script name:

- Setup: Setup Workspace, Generate Solution, Check Toolchain.
- Build: Compile Editor, Compile Runtime, Build Cook Tools, Build Selected Target.
- Cook: Cook Shaders, Build Textures, Build Meshes / Scene Assets, Cook All Assets.
- Maintenance: Run Clang Format, Run Validation Gates, Clean Workspace.
- Launch: Run Editor, Run Runtime.

Each operation row should show readiness and its next effect:

```text
Cook Shaders      [Ready]       Incremental cook for Showcase / DevelopmentGame
Build Textures    [Ready]       Cook changed texture requests
Build Meshes      [Blocked]     AssetCooker is missing; build tool first
Clean Workspace   [Warning]     Select clean scope before running
```

## Required Operations

The launcher should expose these first-class operations:

| Operation | User Intent | Production Owner |
| --- | --- | --- |
| Setup Workspace | Fresh clone or repair local prerequisites | Launcher workflow service plus toolchain/build modules |
| Generate Solution | Refresh CMake build files intentionally | Build orchestration module |
| Build Textures | Cook texture assets for selected project/profile | `AssetCooker` + `TextureCooker` |
| Build Meshes | Cook scene mesh/material/scene assets | `AssetCooker` + mesh/material/scene cook libraries |
| Cook Shaders | Validate registrations and emit shader packages | `ShaderCompiler` |
| Compile Editor | Build `<Project>Editor` for editor profiles | CMake build orchestration |
| Compile Projects | Build selected runtime/editor project targets | CMake build orchestration |
| Clean Workspace | Remove generated outputs with clear scopes | Workspace maintenance module |
| Run Clang Format | Apply or check formatting | Formatting module / CMake target |

Optional later operations:

- Run editor after successful compile.
- Run project runtime.
- Run validation gates.
- Inspect cooked shader package registry.
- Open latest logs and cooked output folders.
- Export a support bundle with logs, config, and tool versions.

## Script Policy

Sparkle Launcher should treat scripts as a migration liability, not as the implementation backend.

The Phase 0 operation inventory lives in [sparkle-launcher-operation-inventory.md](sparkle-launcher-operation-inventory.md). It maps current public scripts to native `SparkleLauncherCore` operations, identifies script-owned behavior to port, and records deletion conditions.

Native C++ ownership is required for:

- Repository and project discovery.
- Toolchain detection.
- CMake generator/toolset selection.
- Build-file freshness checks.
- Build target selection and invocation.
- Cook tool discovery and invocation.
- Cook mode selection and scoped cleanup.
- Clang-format invocation.
- Validation target invocation.
- Log indexing and operation history.
- Failure classification and recovery suggestions.

Scripts are acceptable only when they satisfy one of these conditions:

- They bootstrap a fresh clone before a launcher binary exists.
- They perform a narrow OS shell integration that C++ cannot reasonably own.
- They are temporary wrappers over `Sparkle.exe` during cutover.
- They support external CI while CI is migrating to `Sparkle.exe` or direct CMake/tool commands.

Any script that remains after launcher MVP must document:

- Why C++ cannot own it yet.
- Which launcher or `SparkleLauncherCore` operation replaces it.
- What condition allows deletion.

The target end state is that normal engine work does not require opening `Scripts/` at all.

## Production Patterns To Borrow

Unreal Engine Project Launcher / Unreal Frontend:

- Uses named launch/build/cook profiles.
- Separates target platform, configuration, cook mode, and launch mode.
- Shows staged operations as a pipeline rather than one opaque command.
- Useful pattern: profile-driven workflows and visible cook/build phases.

Unity Hub plus Unity Build Profiles:

- Presents projects first, then editor version/toolchain context.
- Makes common operations discoverable without requiring command-line knowledge.
- Useful pattern: project list, recent workspaces, install/toolchain state, and build-profile presets.

Godot Project Manager and Editor Export dialog:

- Lightweight project discovery and launch UX.
- Export presets keep build settings reproducible.
- Useful pattern: small native-feeling UI with explicit presets and validation before execution.

Visual Studio / Rider solution build UX:

- Separates solution generation/configuration from target build.
- Gives configuration/platform selectors and errors linked to logs.
- Useful pattern: target/config selectors, incremental build feedback, and output navigation.

Android Studio / Gradle task UI:

- Exposes named tasks while keeping task graph execution structured.
- Useful pattern: every action has a deterministic task id, inputs, dependencies, status, and log stream.

Recommended synthesis for Sparkle:

- Use Unreal-style build/cook profiles.
- Use Unity/Godot-style project discovery.
- Use Gradle-style typed operations and job graph execution.
- Use Visual Studio-style target/config/log feedback.

## Backend Architecture Options

These options describe how launcher operations execute. They are separate from the chosen project-centric GUI direction.

### Backend Option A: Thin GUI Over Existing Scripts

The launcher invokes current `.bat` scripts and displays their output.

Pros:

- Fastest MVP.
- Low risk to existing workflows.
- Can validate UX before moving orchestration.

Cons:

- Does not satisfy the long-term goal of removing scripts.
- Keeps batch parsing, environment quirks, and hidden control flow alive.
- Harder to provide structured progress, cancellation, and typed diagnostics.

Avoid for Sparkle Launcher. This path is useful only as a short-lived diagnostic fallback if a native operation is not ready yet.

### Backend Option B: Launcher Calls CMake And Tool Executables Directly

The launcher replaces public scripts by directly invoking CMake, MSBuild, `AssetCooker`, `ShaderCompiler`, `TextureCooker`, and maintenance commands.

Pros:

- Removes user-facing script dependency.
- Reuses existing C++ tools where cook ownership already lives.
- Easier to display structured workflow state than raw script output.

Cons:

- Some script-owned behavior must be ported first: toolchain validation, generator selection, freshness checks, clean scopes, and logging bootstrap.
- Still process-oriented rather than library-oriented.

This is the minimum acceptable near-term production path. It removes public script dependency, but it should still be treated as a step toward a shared C++ workflow library.

### Backend Option C: Shared C++ Workflow Library Plus Launcher UI

Create a shared `SparkleLauncherCore` or `WorkspaceOps` library that owns toolchain checks, build-file freshness, project discovery, CMake invocation, clean scopes, and process execution. The launcher UI calls this library; a future command-line `Sparkle` tool can call the same library.

Pros:

- Best long-term architecture.
- Avoids duplicating workflow logic between GUI and CLI.
- Enables structured progress events, diagnostics, dry runs, cancellation, and tests.
- Makes script removal realistic instead of cosmetic.

Cons:

- More up-front work.
- Requires careful bootstrapping, because a fresh clone may not have a built launcher yet.

This should be the target architecture.

### Backend Option D: Web/Electron App

Build the launcher as a web desktop app.

Pros:

- Fast UI iteration.
- Rich interface ecosystem.

Cons:

- Adds a large runtime dependency for a C++ engine workspace tool.
- Poor fit for first-run bootstrap and low-friction repository operations.
- More moving parts than the problem requires.

Not recommended for the first production launcher.

## Recommended Direction

Build Sparkle Launcher as a project-centric desktop app over a shared workflow layer:

1. `SparkleLauncherCore` C++ workflow layer.
2. `SparkleLauncher` project-centric desktop UI over that workflow layer.
3. Optional later `Sparkle.exe` CLI surface using the same workflow layer.

The launcher should not depend on batch scripts for normal operation. The preferred production path is direct operation execution through `SparkleLauncherCore`, CMake, and existing C++ tools. Any script bridge must be exceptional, temporary, and tracked as technical debt with a planned native replacement.

Recommended first implementation stack:

- Language: C++20.
- Build: CMake target under `Tools/SparkleLauncher`.
- Workflow library: static library under `Tools/SparkleLauncher` or `Tools/WorkspaceOps`.
- UI: Dear ImGui for the first production tool UI unless a native Windows UI toolkit is explicitly chosen. ImGui fits a compact project manager, keeps the implementation inside the C++ toolchain, and avoids a large Electron-style runtime.
- Process execution: explicit process runner with stdout/stderr capture, exit code, cancellation, and log teeing.
- Configuration storage: JSON or INI under `build/Launcher/` for local state; no generated state under source asset folders.
- Project presets: optional project-owned preset files only after the local-state model is proven.

Important bootstrap note:

- If the launcher is built by the build system, a fresh clone still needs a way to obtain it.
- Production options are: ship a prebuilt launcher release artifact, provide a tiny checked-in bootstrapper, or keep a temporary setup script until the launcher can self-update/install.
- The cleanest long-term user story is: download Sparkle Launcher, open repository, click Setup Workspace.

## Functional Model

Represent every launcher action as a typed operation:

```text
OperationId
DisplayName
Inputs
PreflightChecks
ExecutionSteps
GeneratedOutputs
LogChannels
DestructiveScope
CanCancel
CanDryRun
```

Example: Cook Shaders

```text
OperationId: cook.shaders
Inputs: project, configuration, backend, targets, cache mode
Preflight: build files current, ShaderCompiler exists or can be built, shader registrations valid
Steps:
  1. Ensure build files are current
  2. Build ShaderCompiler with matching editor tool profile if needed
  3. Run ShaderCompiler list-shaders --validate
  4. Run ShaderCompiler cook for selected package scope
Outputs: build/Cooked/<Project>/Shaders, shader registry, latest log
```

Example: Compile Editor

```text
OperationId: build.editor
Inputs: project, configuration
Preflight: project has <Project>Editor target, build files current
Steps:
  1. Ensure build files are current
  2. Build <Project>Editor
Outputs: build/bin/<Configuration>/<Project>Editor.exe, latest build log
```

## Workflow Graph

Actions should not be one-off buttons with hidden side effects. They should be small operation graphs.

Example graph for full first-run setup:

```text
Detect Repository
Validate Toolchain
Sync Dependencies During Configure
Generate Build Files
Build Cook Tools Optional
Report Ready State
```

Example graph for full project readiness:

```text
Ensure Build Files
Compile Editor
Cook Shaders
Build Textures
Build Meshes / Scene Assets
Report Runnable Editor State
```

Each node should produce:

- Status: pending, running, succeeded, failed, skipped, canceled.
- Start/end time.
- Exit code when process-backed.
- Primary log path.
- Human-readable failure summary.
- Suggested recovery action.

## Implementation Roadmap

Minimize scripts from the start. Replace behavior intentionally by moving orchestration into typed C++ operations and using the launcher as the primary surface once each operation reaches parity.

Scripts should be allowed only for:

- Fresh-clone bootstrap before a launcher binary exists.
- OS shell handoff that C++ cannot reasonably perform by itself.
- Temporary compatibility wrappers after the native operation already exists.
- CI transition glue while automation moves to `Sparkle.exe` or direct CMake/tool calls.

Every retained script should have an owner, a reason, and a removal condition.

### Phase 0: Product Contract

Goal: lock the project-centric UX and operation contracts before code starts.

Deliverables:

- This design document updated around the project-centric launcher path.
- Operation inventory that maps every public script to a launcher operation.
- Initial UI wireframe for project tiles, selected project operations, job output, and recent activity.
- Decision on UI toolkit: Dear ImGui or native Windows UI.
- Decision on bootstrap story: temporary script, prebuilt launcher artifact, or checked-in bootstrapper.
- Script retention policy listing which scripts are unavoidable, which are temporary wrappers, and which should be deleted after native parity.

Exit criteria:

- We can name the first 10 operations and their inputs.
- We know which script-owned behaviors must be ported before script removal.
- The first implementation milestone is small enough to build without touching cook logic.
- No first-class launcher operation is planned to permanently call a `.bat` file.

### Phase 1: `SparkleLauncherCore` Foundation

Goal: create the non-UI workflow layer the launcher will depend on.

Deliverables:

- `Tools/SparkleLauncher` folder with a `SparkleLauncherCore` static library target.
- Repository root detection.
- Project discovery from `Projects/*/.sparkle-project`.
- Build profile catalog for the six supported profiles.
- Local launcher state directory under `build/Launcher/`.
- Basic operation model: id, display name, inputs, status, log path, start/end time, exit code.
- Process runner with captured output, cancellation hook, and log teeing.
- Native path/environment utilities for invoking CMake, MSBuild, Git, clang-format, and Sparkle tool executables without batch wrappers.

Exit criteria:

- A tiny test or console harness can list projects and profiles.
- A dummy operation can run a process, capture output, and report success/failure.
- No UI code owns workflow decisions.
- No foundational operation shells out through public scripts.

### Phase 2: Project-Centric UI Shell

Goal: build the launcher window and project manager experience before wiring all real operations.

Deliverables:

- `Tools/SparkleLauncher` executable target.
- Project tile grid with discovered projects.
- Selected project workspace with Setup, Build, Cook, Maintenance, and Launch groups.
- Profile/configuration selectors.
- Recent activity panel fed by local launcher state and latest known logs.
- Job output panel.
- Dry-run mode that shows planned operations without executing them.

Exit criteria:

- Opening the launcher shows real discovered projects.
- Selecting `Showcase` shows the correct operation groups.
- Running a dummy/dry-run operation updates job status and activity history.

### Phase 3: Build And Workspace Operations

Goal: replace the most common setup/build scripts with direct launcher operations.

Deliverables:

- Toolchain status detection for CMake, Visual Studio/MSBuild, Windows SDK, Git, and clang-format.
- Native Generate Solution operation using ported generator/toolset rules from `GenerateSolution.bat`.
- Native build-file freshness check using the existing `build/BuildFilesFreshness.json` contract or a ported equivalent.
- Compile Editor operation for `<Project>Editor`.
- Compile Runtime/Project operation for `<Project>Runtime` and selected targets.
- Build Cook Tools operation for `AssetCooker`, `TextureCooker`, and `ShaderCompiler`.
- Native toolchain validation ported from script logic into `SparkleLauncherCore`.

Exit criteria:

- A user can open the launcher, generate build files, and compile `ShowcaseEditor` without calling a public script.
- Build logs are visible in the launcher and written under the existing structured log layout or a launcher-owned equivalent.
- Scripts may still exist, but setup and build launcher paths do not invoke them.

### Phase 4: Cook Operations

Goal: make asset preparation script-free from the launcher.

Deliverables:

- Cook Shaders operation: build/locate `ShaderCompiler`, run registration validation, cook selected packages or all packages.
- Build Textures operation: build/locate `AssetCooker` and `TextureCooker`, run focused texture cook.
- Build Meshes / Scene Assets operation: run scene, mesh, and material cook path through `AssetCooker`.
- Cook All Assets operation with incremental and force modes.
- Cook output discovery for `build/Cooked/<Project>/`.
- Failure summaries for shader/package/asset errors.

Exit criteria:

- A user can cook shaders, textures, and scene assets for `Showcase` without calling cook scripts.
- Force recook has explicit confirmation and scoped cleanup.
- Launcher failure summaries point to the relevant latest log.
- Cook launcher operations invoke C++ tools directly and never route through `Scripts/Cook/*.bat`.

### Phase 5: Maintenance Operations

Goal: bring quality and cleanup flows into the same project-centric surface.

Deliverables:

- Run Clang Format operation, with apply and check modes if supported.
- Run Validation Gates operation for known CMake validation targets.
- Clean Workspace operation with explicit scopes: cooked outputs, selected project cooked outputs, build tree, shader cache, third-party dependency cache, logs, or pristine generated cleanup.
- Locked-file and destructive-action diagnostics.

Exit criteria:

- A user can format, validate, and clean generated outputs without touching `Scripts/`.
- No clean action can delete broad generated state without showing its exact scope.

### Phase 6: Script Parity And Cutover

Goal: prove the launcher can replace public scripts before deleting them.

Deliverables:

- Parity table for each public script and matching launcher operation.
- Optional `Sparkle.exe` CLI over `SparkleLauncherCore` for automation and CI.
- Public scripts either removed, reduced to thin wrappers over `Sparkle.exe`, or marked deprecated with a removal condition.
- Documentation updated so Sparkle Launcher is the primary workflow entrypoint.
- Explicit list of any scripts retained for unavoidable bootstrap/handoff cases.

Exit criteria:

- Every public script has a launcher operation with equivalent behavior.
- CI and local automation have a non-GUI path if needed.
- No docs instruct normal users to run scripts for common workflows.
- Retained scripts are not required for launcher-driven setup, build, cook, format, clean, or launch operations.

### Phase 7: Remove Public Scripts

Goal: finish the repository transition.

Deliverables:

- Delete or archive public workflow scripts after parity is proven.
- Keep CMake modules, validation targets, and internal build infrastructure that still belong to the build system.
- Update onboarding docs, architecture docs, and troubleshooting docs.
- Publish prebuilt launcher or bootstrap instructions.

Exit criteria:

- Fresh clone workflow has a clear launcher-based path.
- Normal users can setup, build, cook, format, clean, and launch through Sparkle Launcher.
- Repository no longer depends on user-facing scripts for common engine operations.

### Phase 8: Production Polish

Goal: make the launcher feel durable, not just functional.

Deliverables:

- Named workflow presets.
- Support bundle export with logs, config, tool versions, and operation history.
- Update/version compatibility checks between launcher and repository schema.
- Richer failure classification and suggested recovery actions.
- Optional local-only usage metrics if explicitly wanted.

Exit criteria:

- Common failures are understandable without opening raw logs first.
- Project readiness is visible at a glance.
- Repeated workflows can be saved and rerun predictably.

## Implementation Prompt Pack

Use these prompts in order to implement the launcher from start to finish. The intent is to keep momentum through source changes without stopping for heavyweight builds between phases. Each phase should do source-only validation such as file reads, targeted searches, compile-error inspection when available, and consistency checks. Build and runtime validation are reserved for the final validation phase.

Shared implementation contract to include with every prompt:

```text
You are implementing Sparkle Launcher as a project-centric C++ workflow tool. The target shape is SparkleLauncherCore owning workflow logic, SparkleLauncher owning UI, and optional Sparkle.exe owning CLI automation. Do not implement launcher workflows as wrappers around .bat files or public Scripts/ entrypoints. Port behavior into C++ and remove replaced legacy/backward-compatibility paths instead of adding shims.

Do not run full builds, CMake builds, cook commands, format commands, or launch commands during this phase. Make the requested source/documentation changes, keep edits scoped, and finish with source-only validation: targeted reads, grep/search checks, CMake file consistency checks, and editor diagnostics if available. Record final build/test commands for Prompt 10 only.

Use these references when relevant:
- docs/plans/sparkle-launcher-design.md for the product contract and phase goals.
- Scripts/README.md for current public workflow behavior that must be replaced.
- Scripts/SetupWorkspace.bat, GenerateSolution.bat, BuildProject.bat, CookAllAssets.bat, RunClangFormat.bat, CleanWorkspace.bat, and Scripts/Cook/*.bat for behavior to port, not call.
- Tools/CMakeLists.txt and existing tool CMakeLists files for target wiring patterns.
- CMake/SparkleBuildProfiles.cmake for supported profiles.
- build/BuildFilesFreshness.json contract and related freshness docs for build-file state.
- Existing C++ tools AssetCooker, ShaderCompiler, TextureCooker, MeshCooker, MaterialCooker, and SceneCooker for native operation ownership.

Positive guardrails:
- Prefer typed C++ data structures over stringly command glue.
- Keep implementation noise-free: every new file, public type, helper, target, and operation must have a clear current-phase purpose or a named next-phase consumer.
- Make each abstraction earn its right to exist; delete speculative wrappers, placeholders, and indirection that do not simplify the current implementation.
- Keep operation inputs, outputs, logs, status, and destructive scope explicit.
- Keep UI state separate from workflow decisions.
- Keep generated state under build/, logs/, or user-local app data.
- Delete replaced legacy code when parity exists.

Negative guardrails:
- Do not add code, files, targets, interfaces, or settings that only reserve future design space.
- Do not introduce anonymous namespaces, unnamed namespaces, or nested namespaces; use a single named namespace and justified private/static helpers instead.
- Do not add permanent script fallbacks.
- Do not introduce hidden compatibility shims.
- Do not run broad validation before Prompt 10.
- Do not write generated launcher state into Engine/Assets or Projects/*/Assets.
- Do not add one vague Clean action; every clean operation must have an explicit scope.
```

Each implementation prompt should produce three outputs in the final response for that phase:

- Changed files, what behavior they add, and why any new public type/helper/target earns its place.
- Source-only validation performed.
- Deferred final validation commands to run in Prompt 10.

### Prompt 0: Freeze The Launcher Contract

```text
Implement Phase 0 for Sparkle Launcher planning.

Goal: freeze the product and migration contract before any launcher code is written. The selected direction is a project-centric C++ launcher backed by SparkleLauncherCore, not a GUI over scripts.

References to inspect:
- docs/plans/sparkle-launcher-design.md
- Scripts/README.md
- Scripts/SetupWorkspace.bat
- Scripts/GenerateSolution.bat
- Scripts/BuildProject.bat
- Scripts/CookAllAssets.bat
- Scripts/Cook/CookShaders.bat
- Scripts/Cook/CookTextures.bat
- Scripts/Cook/CookAssets.bat
- Scripts/RunClangFormat.bat
- Scripts/CleanWorkspace.bat

Tasks:
1. Update the design doc if needed so the chosen UX is the project-centric launcher surface.
2. Create or update an operation inventory that maps each current public script to a future native SparkleLauncherCore operation.
3. For each script, mark the future state as remove, temporary Sparkle.exe wrapper, or unavoidable bootstrap/handoff.
4. Identify script-owned behavior that must be ported into C++ before script deletion.
5. Capture explicit deletion conditions for legacy scripts and compatibility wrappers.

Positive guardrails:
- Treat existing scripts as behavioral references, not implementation dependencies.
- Make native C++ ownership the default for all normal workflows.
- Keep bootstrap/handoff exceptions rare and justified.

Negative guardrails:
- Do not implement code yet.
- Do not run builds.
- Do not promise permanent `.bat` support for normal workflows.

Source-only validation:
- Read the updated docs/inventory.
- Search the new text for ambiguous phrases like "wrapper first" or "script backend" and clarify them.
- Confirm every public script has a native replacement or deletion condition.
```

Expected result:

- Launcher contract is unambiguous.
- Every public script has a planned native replacement or deletion condition.
- No normal workflow is planned to permanently call `.bat` files.

### Prompt 1: Create `SparkleLauncherCore` Foundation

```text
Implement Phase 1: create the SparkleLauncherCore foundation.

Goal: add a non-UI C++ workflow library that owns repository/project/workflow state. SparkleLauncher and Sparkle.exe will depend on this library later.

References to inspect:
- Tools/CMakeLists.txt
- Tools/CookCommon/CMakeLists.txt
- Tools/ShaderCompiler/CMakeLists.txt
- Tools/TextureCooker/CMakeLists.txt
- CMake/SparkleBuildProfiles.cmake
- Projects/*/.sparkle-project markers

Target shape:
- Add Tools/SparkleLauncher/CMakeLists.txt.
- Add a static library target named SparkleLauncherCore.
- Add Public/ and Private/ folders following existing tool conventions.
- Expose narrow public headers for repository location, project discovery, build profiles, launcher state paths, operation model, and process runner abstraction.
- Keep implementation private where possible.

Tasks:
1. Implement repository root detection using stable markers such as CMakeLists.txt, Engine/, Tools/, and Projects/.
2. Implement project discovery from Projects/*/.sparkle-project.
3. Implement a build profile catalog for DebugEditor, DebugGame, DevelopmentEditor, DevelopmentGame, ShippingEditor, and ShippingGame.
4. Implement launcher local-state path resolution under build/Launcher.
5. Add a basic Operation model: id, display name, inputs, status, log path, start/end time, exit code, dry-run text, and destructive scope metadata.
6. Add a ProcessRunner interface/stub only; real process execution comes in Prompt 2.
7. Wire SparkleLauncherCore into Tools/CMakeLists.txt.

Positive guardrails:
- Keep APIs small and testable.
- Use C++ types for paths, profiles, project ids, operation ids, and status.
- Keep workflow state independent from UI.
- Every public header and helper must support a current Phase 1 exit criterion or a named Prompt 2 consumer.

Negative guardrails:
- Do not call public scripts.
- Do not add backward-compatibility wrappers.
- Do not make SparkleLauncherCore depend on SparkleLauncher.
- Do not add anonymous namespaces, nested namespaces, placeholder interfaces, or unused extension points.
- Do not run builds.

Source-only validation:
- Read Tools/CMakeLists.txt and Tools/SparkleLauncher/CMakeLists.txt for target wiring.
- Search for accidental `Scripts/`, `.bat`, `cmd.exe /c`, or `powershell` references.
- Search for anonymous or nested namespaces and remove them.
- Confirm public headers do not expose unnecessary implementation details.
```

Expected result:

- `Tools/SparkleLauncher` exists.
- Workflow concepts are testable outside the UI.
- No UI or script dependency owns core workflow state.

### Prompt 2: Add Native Process And Environment Utilities

```text
Implement Phase 1B: native process and environment utilities for SparkleLauncherCore.

Goal: make SparkleLauncherCore capable of invoking tools directly without batch wrappers.

References to inspect:
- Existing Windows/platform helper patterns under Engine/Platform and Engine/Core if relevant.
- Scripts/Internal/Toolchain and Scripts/Internal/Build only as behavior references.
- CMake/Dependencies and CMake toolchain assumptions if relevant.

Target shape:
- ProcessRequest: executable path, arguments, working directory, environment overrides, log path, output routing, cancellation token placeholder.
- ProcessResult: exit code, captured stdout/stderr or merged output, start/end time, failure reason.
- ProcessRunner: interface plus Windows implementation boundary.
- Tool resolution helpers for CMake, MSBuild, Git, clang-format, and Sparkle build outputs.

Tasks:
1. Replace the ProcessRunner stub with a Windows-capable implementation using structured arguments rather than one concatenated command string.
2. Add log tee hooks so launcher output and disk logs can consume the same stream later.
3. Add cancellation-ready structure even if full UI cancellation is completed later.
4. Add native path/environment helpers for resolving CMake, MSBuild, Git, clang-format, and Sparkle tool executables.
5. Keep script fallback out of the normal code path. If a bootstrap placeholder is unavoidable, isolate it and document the deletion condition.

Positive guardrails:
- Prefer explicit argument arrays and quoting at the process boundary.
- Keep Windows-specific code behind a small owner.
- Preserve stdout/stderr and exit code for failure classification.

Negative guardrails:
- Do not invoke `.bat` files to resolve tools.
- Do not shell through `cmd.exe /c` or PowerShell for normal execution.
- Do not run builds or commands for validation.

Source-only validation:
- Search for shell-string execution and remove it from normal paths.
- Read Windows API usage for handle lifetime and argument quoting.
- Confirm ProcessRunner APIs can be consumed by build/cook operations without UI dependencies.
```

Expected result:

- SparkleLauncherCore can execute tools directly.
- Public scripts are not part of the execution backend.
- Process execution is structured enough for UI progress and logs.

### Prompt 3: Implement Native Toolchain And Build Discovery

```text
Implement Phase 3 foundation in SparkleLauncherCore without building. Port toolchain detection from script behavior into C++: detect CMake, Visual Studio/MSBuild, Windows SDK, Git, clang-format, and optional clang-cl/clang-tidy. Add CMake generator/toolset selection data structures, build directory discovery, CMake cache inspection, build-file freshness status using the existing build/BuildFilesFreshness.json contract or an equivalent native reader, and target-name derivation for <Project>Editor and <Project>Runtime. Do not invoke scripts. Do not run CMake or MSBuild yet. Finish with source-only validation and a documented final-phase validation command list.
```

Expected result:

- Setup/generate/build decisions can be made in C++.
- Existing script logic has a native destination.
- The launcher can explain blocked/missing toolchain states.

### Prompt 4: Implement Native Build Operations

```text
Implement native SparkleLauncherCore operations for Setup Workspace, Generate Solution, Compile Editor, Compile Runtime/Project, and Build Cook Tools. These operations should build operation graphs and process requests for CMake/MSBuild, but this phase must not execute real builds. Add dry-run output that shows exact executable, arguments, working directory, generated outputs, and log path. Integrate operation status, failure classification placeholders, and activity records. Do not call public scripts. Remove any temporary compatibility code that duplicates these operations. Finish with source-only validation of operation graph wiring and dry-run output paths.
```

Expected result:

- Build/workspace operations exist as native C++ workflows.
- Dry-run can show what will happen without running builds.
- Script-backed setup/build paths are not used.

### Prompt 5: Create Project-Centric Launcher UI Shell

```text
Implement Phase 2 of Sparkle Launcher. Add Tools/SparkleLauncher as a C++ executable using the selected UI stack. Build a project-centric shell: project tile grid, selected project workspace, profile/configuration selectors, operation groups for Setup/Build/Cook/Maintenance/Launch, recent activity panel, and job output panel. Wire it to SparkleLauncherCore discovery and dry-run operations only. Do not run builds. Do not implement script wrappers. Keep UI state separate from workflow decisions. Finish with source-only validation of CMake wiring, source layout, and references.
```

Expected result:

- Launcher opens around project selection conceptually.
- UI consumes `SparkleLauncherCore`; it does not own workflow behavior.
- Project-centric Option B UX is now represented in code.

### Prompt 6: Implement Native Cook Operations

```text
Implement Phase 4 cook operations in SparkleLauncherCore and connect them to SparkleLauncher dry-run/execution surfaces. Add Cook Shaders, Build Textures, Build Meshes / Scene Assets, and Cook All Assets operations. They should locate/build required tools through native build operations, then invoke ShaderCompiler, AssetCooker, TextureCooker, and related C++ tools directly. Preserve incremental and force recook modes, with explicit scoped cleanup for force recook. Do not call Scripts/Cook/*.bat or CookAllAssets.bat. Do not run cook commands yet. Remove any script compatibility routes replaced by these native operations. Finish with source-only validation of command construction, output paths, and log classification.
```

Expected result:

- Asset preparation is represented as native C++ operations.
- Shader/texture/mesh/full cook flows no longer route through scripts.
- Destructive cook cleanup is scoped and explicit.

### Prompt 7: Implement Maintenance And Launch Operations

```text
Implement Phase 5 maintenance and launch operations. Add native Run Clang Format, Run Validation Gates, Clean Workspace with explicit scopes, Run Editor, and Run Runtime operations. Clean scopes must be explicit and must not delete broad generated state without confirmation metadata. Format and validation should call clang-format/CMake targets directly through SparkleLauncherCore, not scripts. Launch operations should derive executable paths from selected project/configuration. Do not run builds, format, validation, or launches yet. Remove replaced legacy script-compatible helpers. Finish with source-only validation of operation definitions, clean scopes, and derived paths.
```

Expected result:

- Quality, cleanup, and launch flows are native launcher operations.
- Clean behavior is safe and inspectable.
- No normal maintenance action requires `Scripts/`.

### Prompt 8: Add `Sparkle.exe` CLI For Automation

```text
Implement a small Sparkle.exe CLI over SparkleLauncherCore for automation parity. It should expose the same operation ids used by the launcher, support dry-run, project/configuration selection, and structured exit codes. Keep it thin: all workflow logic stays in SparkleLauncherCore. Do not run builds. Do not preserve legacy command names unless they are clean operation aliases. Update docs to point automation at Sparkle.exe rather than public scripts. Finish with source-only validation of command parsing, operation dispatch, and documentation references.
```

Expected result:

- CI and power users have a non-GUI path.
- The CLI does not reintroduce script-style orchestration.
- Workflow logic remains shared.

### Prompt 9: Remove Legacy Script Surface

```text
Implement the script cutover cleanup. Remove public workflow scripts that are fully replaced by SparkleLauncher/Sparkle.exe operations, or reduce only unavoidable bootstrap/handoff scripts to minimal documented wrappers. Delete deprecated compatibility shims, stale docs, old quick workflow instructions, and references that tell normal users to run Scripts/*.bat. Update README/docs so Sparkle Launcher and Sparkle.exe are the primary entrypoints. Keep CMake modules and validation targets that are build-system infrastructure. Do not run builds. Finish with source-only validation using targeted search for removed script names, legacy compatibility language, and stale docs.
```

Expected result:

- Normal workflow docs no longer point at public scripts.
- Replaced script code is deleted, not left as parallel legacy behavior.
- Any retained script has a clear bootstrap/handoff reason and removal condition.

### Prompt 10: Final Build And Validation Pass

```text
Run the final validation phase for Sparkle Launcher. Now builds are allowed. Configure/generate if needed, build SparkleLauncherCore, SparkleLauncher, Sparkle.exe, existing cook tools, and representative project targets. Run focused validation for project discovery, dry-run operations, native Generate Solution, Compile Editor, Cook Shaders, Build Textures, Build Meshes, Run Clang Format check/apply mode as appropriate, Clean Workspace dry-run/safe scopes, and script-removal search checks. Fix any issues in the same slice and rerun the failing validation. End with a concise report of commands run, results, remaining risks, and any retained scripts with reasons.
```

Expected result:

- Full system builds only after all source phases are complete.
- Launcher/CLI/native workflows are validated end to end.
- Legacy script surface is gone or explicitly justified.

## Source-Only Validation Checklist

Use this checklist at the end of Phases 0 through 9:

- Search for accidental calls to `Scripts/`, `.bat`, `cmd.exe /c`, or `powershell` in launcher workflow code.
- Search for duplicated legacy/backward-compatibility paths.
- Search for anonymous namespaces, unnamed namespaces, nested namespaces, placeholder interfaces, and unused extension points.
- Read changed CMake files for target inclusion, include directories, and link dependencies.
- Read changed public headers for stable ownership and unnecessary abstractions.
- Confirm every new file, public type, helper, target, and operation has a current-phase purpose or named next-phase consumer.
- Check generated paths remain under `build/`, `logs/`, or user-local app data.
- Check destructive operations expose exact scope before execution.
- Check docs do not instruct normal users to run public scripts.
- Collect final-phase build/test commands but do not run them until Prompt 10.

## UX Details

Main actions should be grouped by intent:

- Workspace: Setup Workspace, Generate Solution, Clean Workspace.
- Build: Compile Editor, Compile Project Runtime, Build Selected Targets.
- Cook: Cook Shaders, Build Textures, Build Meshes, Cook All Assets.
- Quality: Run Clang Format, Run Validation Gates.
- Launch: Run Editor, Run Runtime.

Every action button should show readiness:

- Ready: all inputs valid.
- Needs setup: build files or tools missing.
- Warning: action is destructive or will rebuild large targets.
- Blocked: missing Visual Studio, CMake, SDK, or project target.

Destructive actions need explicit scope:

- Clean cooked outputs for selected project.
- Clean all cooked outputs.
- Clean build tree.
- Clean third-party dependency cache.
- Pristine generated workspace cleanup.

Avoid a single vague `Clean` button.

## Data And State

The launcher should discover state from the repository instead of relying on hardcoded assumptions:

- Repository root: marker files such as `CMakeLists.txt`, `Engine/`, `Tools/`, and `Projects/`.
- Projects: `.sparkle-project` markers under `Projects/`.
- Build profiles: `CMake/SparkleBuildProfiles.cmake` or a generated profile manifest.
- Build tree: `build/BuildFilesFreshness.json` and CMake cache.
- Tools: `build/bin/<EditorProfile>/AssetCooker.exe`, `ShaderCompiler.exe`, `TextureCooker.exe`.
- Logs: structured `logs/Prerequisites/.../Latest.txt` and timestamped logs.
- Cooked outputs: `build/Cooked/<Project>/`.

Local launcher settings should be generated under `build/Launcher/` or a user-local app-data folder. Do not write generated launcher state into source project folders unless it is an intentional, reviewed project preset.

## Error Handling

Failures should be summarized in launcher language, with logs still available.

Example shader cook failure summary:

```text
Cook Shaders failed while compiling package GBuffer for SpirV16.
Reason: reflected binding kind mismatch for MeshInstances.
Expected: StructuredBuffer
Actual: RWStructuredBuffer
Log: logs/Prerequisites/ShaderCompilationLog/Showcase/Latest.txt
```

The launcher should classify common failures:

- Missing toolchain.
- Build files stale or configure failed.
- Tool target failed to build.
- Shader registration validation failed.
- Cook package failed.
- Asset source missing or unsupported.
- Format check failed.
- Clean blocked by locked files.

Each class should have a suggested recovery action, preferably as another launcher button.

## Open Design Choices

These are the main alignment decisions before implementation:

| Choice | Option 1 | Option 2 | Recommendation |
| --- | --- | --- | --- |
| UI stack | Native Windows UI | Dear ImGui tool UI | Dear ImGui first unless native Windows UI is explicitly chosen |
| MVP execution | Wrap scripts temporarily | Direct CMake/tool invocation | Direct invocation; avoid script wrapping except unavoidable bootstrap/handoff |
| Backend shape | Launcher-only logic | Shared `SparkleLauncherCore` library | Shared library |
| Fresh clone story | Temporary setup script | Prebuilt launcher release | Prebuilt launcher release long term |
| Automation | GUI only | Shared CLI plus GUI | Shared CLI plus GUI |
| Clean behavior | One clean button | Explicit clean scopes | Explicit clean scopes |
| Presets | Hardcoded defaults | User/project workflow presets | Start with defaults, add presets after MVP |

## First Shippable MVP Scope

The first shippable MVP should cover Phases 1 through 3 plus a minimal project-centric UI:

1. Discover repository root and projects.
2. Show project tiles with basic readiness state.
3. Select project and configuration.
4. Show selected project operation groups.
5. Generate Solution.
6. Compile Editor.
7. Show active job output and latest log path.
8. Record recent activity.
9. Provide dry-run previews for cook, clean, and format operations even if they are not executable yet.
10. Execute setup/build operations through C++ workflow code, not public scripts.

The second MVP should add Phase 4 cook operations:

1. Cook Shaders.
2. Build Textures.
3. Build Meshes / Scene Assets.
4. Cook All Assets.
5. Force recook confirmation and scoped cleanup.

Defer for later:

- Multiple simultaneous jobs.
- Remote builds.
- Package/export pipeline.
- Marketplace/project templates.
- Rich telemetry.
- Complex plugin management.

## Proposed Repository Shape

```text
Tools/
  SparkleLauncher/
    Public/SparkleLauncher/
    Private/
    Probe/
    Source/
    Resources/
    CMakeLists.txt
```

Possible `SparkleLauncherCore` modules:

- `RepositoryLocator`
- `ProjectDiscovery`
- `BuildProfileCatalog`
- `ToolchainValidator`
- `BuildFileFreshness`
- `CMakeWorkflow`
- `CookWorkflow`
- `FormatWorkflow`
- `CleanWorkflow`
- `ProcessRunner`
- `LogIndex`
- `OperationGraph`

## Acceptance Criteria

The launcher is successful when a normal contributor can:

- Open Sparkle Launcher.
- Select `Showcase`.
- Click Setup Workspace on a fresh clone.
- Click Compile Editor.
- Click Cook Shaders, Build Textures, and Build Meshes.
- Run Clang Format.
- Clean generated outputs with a clear selected scope.
- Open the latest relevant log after any failure.
- Never touch `Scripts/` for normal engine work.

The architecture is successful when:

- Public scripts can be replaced by launcher/CLI operations without losing behavior.
- Workflow logic is testable outside the GUI.
- Every operation has typed inputs, explicit outputs, structured status, and logs.
- Destructive actions are scoped and confirmable.
- Fresh clone bootstrap has a clear answer.
- Any remaining scripts are exceptional bootstrap/handoff utilities, not the normal engine workflow surface.

