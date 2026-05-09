# Asset Pipeline Tool Architecture Plan

This document defines the target architecture for Sparkle's asset, texture, mesh, material, scene, and shader cooking tools. The target is intentionally narrow and should not drift back into script-driven orchestration, a monolithic asset tool, or source-format-aware runtime loading.

The central shape is:

```text
User / CI / Editor
  |
  v
Launch shim only
  |
  v
AssetCooker
  - discover inputs
  - validate capabilities
  - build ProjectCookPlan
  - dispatch existing tools
  - aggregate diagnostics
  |
  +--> Source import adapters -> normalized records
  +--> ShaderCompiler         -> shader packages + registry
  +--> TextureCooker          -> .stex
  +--> Mesh cooker phase      -> cooked meshes
  +--> Material cooker phase  -> cooked materials
  +--> Scene cooker phase     -> scene, cameras, lights
```

Everything in the plan should support this structure.

## Hard Rules

```text
Source formats end at the cook boundary.
Runtime starts at Sparkle cooked assets.
```

- Runtime loads only Sparkle cooked contracts.
- Runtime does not know whether an asset came from glTF, GLB, FBX, DDS, PNG, JPG, EXR, HDR, HLSL, Slang, or any future source format.
- Batch and PowerShell are launch shims only.
- `AssetCooker` owns planning and orchestration.
- `AssetCooker` is usable both as a standalone executable and as a small editor-callable DLL/API facade.
- Category cookers own cooking work.
- Source import adapters own source-format parsing.
- Shared cook services own cross-cooker mechanics.
- Editors and other apps call only documented public API headers or the executable CLI. They must not include private tool headers.

Rejected directions:

- No monolithic asset tool that directly imports, compiles, cooks, and writes every output internally.
- No script-owned project planning, request merging, generated defaults, cook ordering, backend selection, or source-format policy.
- No runtime dependency on source loaders or cook tools.
- No category cooker should parse source formats as its architectural contract.

## Simplicity And Deletion Rules

The architecture should deliver value and stay easy to extend, maintain, and read. Complexity is a cost, even when it looks organized.

Implementation work should follow these rules:

- Prefer the simplest design that preserves clear ownership and current required capabilities.
- Keep enough separation for SOLID-style boundaries: one owner per reason to change, explicit dependency direction, and no hidden cross-cooker coupling.
- Before preserving any class, file, helper, wrapper, script, command, or compatibility layer, ask whether it still serves the target plan.
- If code does not serve the new plan, remove it or move the useful part to the correct owner. Do not keep stale structures just because they already exist.
- Do not keep wrappers whose only purpose is to preserve an old name or old folder after ownership has moved.
- Do not add abstractions speculatively. Add a shared service only when at least two owners need the same mechanic and the shared service will not own category semantics.
- Deleting obsolete complexity is expected. Preserve user-facing capabilities and direct debug surfaces, not dead implementation paths.
- When a removal would affect a supported format, backend, cooked contract, or public entrypoint, replace that behavior at the correct new owner before deleting the old path.

## Target Folder Structure

Folder names should make the architecture visible. A developer should be able to tell who owns planning, source import, shared mechanics, and each cooked output category from the tree alone.

Target tool-side layout:

```text
Tools/
  AssetCooker/
    Source/
      main.cpp
    Public/
      AssetCookerApi.h
      AssetCookerDll.h
      AssetCookRequest.h
      AssetCookResult.h
      AssetCookTypes.h
    Private/
      Api/
      Cli/
      Discovery/
      Planning/
      Dispatch/
      Diagnostics/

  CookCommon/
    Public/
      Artifact/
      Capabilities/
      Diagnostics/
      Paths/
      Plan/
        ProjectCookPlan.h
      Versioning/
    Private/
      Artifact/
      Capabilities/
      Diagnostics/
      Paths/
      Plan/
      Versioning/

  SourceImportAdapters/
    Public/
      SourceImportAdapter.h
      SourceImportResult.h
    Private/
      Gltf/
      Fbx/
      NormalizedRecords/

  ShaderCompiler/
    ... existing focused shader package tool and direct debug surface ...

  TextureCooker/
    ... existing focused texture cooker ...

  MeshCooker/
    Public/
    Private/

  MaterialCooker/
    Public/
    Private/

  SceneCooker/
    Public/
    Private/
```

Folder rules:

- `AssetCooker` contains project-level planning, capability validation, dispatch, and summary diagnostics only.
- `AssetCooker/Public` is the only supported in-process API surface for the editor and other apps.
- `AssetCooker/Source/main.cpp` is only a CLI adapter over the same public/private implementation used by the DLL.
- `CookCommon` contains reusable mechanics only: versioning, artifact identity, paths, diagnostics, capabilities, plan serialization, and dependency edges.
- `SourceImportAdapters` contains source-format parsing and normalized records. It does not write cooked assets.
- `ShaderCompiler`, `TextureCooker`, `MeshCooker`, `MaterialCooker`, and `SceneCooker` map one-to-one to cooked output categories.
- Category cookers must not include private headers from each other. They exchange data through `ProjectCookPlan`, normalized source records, shared cook services, or cooked asset contracts.
- `Public` folders expose contracts needed by other owners. `Private` folders contain implementation details for that owner only.
- Only `AssetCooker/Public` is public API for editor and other apps. Other `Public` folders are tool-module contracts inside the repository unless a phase explicitly promotes them to exported API.
- Avoid catch-all folders like `Utils`, `Helpers`, `Common`, or `Misc` unless the folder is under a precise owner and its contents are still cohesive.
- During extraction, temporary bridge folders are allowed only when a phase explicitly says so. Each bridge must have a clear deletion target in the next phase.

## Public API Boundary

The cook system has two supported entry surfaces:

```text
Standalone tools / CI / scripts
  -> AssetCooker executable CLI

Editor / other in-process tools
  -> AssetCooker DLL public API
```

Both surfaces must go through `AssetCooker`. The CLI and DLL should share the same internal implementation path so behavior does not fork.

Public API goals:

- Keep the API small enough that editor integration is obvious.
- Make recook requests explicit and structured.
- Return changed cooked outputs and diagnostics in a form the editor can use for hot reload.
- Hide planning, source import, category cooker internals, cache details, temporary files, and backend-specific objects.
- Preserve direct standalone executable use for CI, scripts, and debugging.

Only these concepts should be public to other apps:

| Public Concept | Purpose |
|---|---|
| `AssetCookerContext` | Opaque handle or small owner object for initialized cook state. |
| `AssetCookerConfig` | Project root, project name, cooked output root, shader target/profile policy, and optional diagnostics path. |
| `AssetCookRequest` | Full project cook, selected asset cook, or recook request. |
| `AssetRecookRequest` | Minimal editor hot-reload request for textures, shaders, meshes, materials, or scenes. |
| `AssetCookResult` | Success/failure, changed outputs, cooked asset ids, reload categories, and diagnostic summary. |
| `AssetCookDiagnostic` | Structured error/warning/info messages with asset/source context. |
| `AssetCookedOutput` | Output path, asset id, category, version, and hot-reload hint. |

Everything else is private unless a later phase proves it must be shared.

Minimal DLL-facing operations:

```text
CreateContext(config) -> context
DestroyContext(context)
CookProject(context, request) -> result
RecookAssets(context, recookRequest) -> result
QueryCapabilities(context) -> capabilities
GetLastDiagnostics(context) -> diagnostics
```

The concrete implementation may be C++ classes for internal use, but the DLL boundary should avoid exposing STL containers, exceptions, raw ownership rules, or private engine/tool types across module boundaries. Prefer opaque handles, POD-style structs, spans/count pairs, or serialized result blobs at the binary boundary. A thin C++ convenience wrapper may live in `AssetCooker/Public` for editor code, but it should wrap the stable DLL surface rather than expand it.

Private implementation details:

- Source import adapter classes.
- Category cooker classes and their intermediate records.
- `ProjectCookPlan` mutation internals.
- Cache and artifact lookup internals.
- Shader backend objects, texture codec objects, mesh/material translation state, and scene hierarchy builders.
- Temporary request file formats used only during bridge phases.

Editor hot reload flow:

```text
Editor detects source asset change or user requests recook
  -> AssetCooker DLL RecookAssets
       -> update or build the relevant ProjectCookPlan subset
       -> dispatch texture/shader/mesh/material/scene cooker work
       -> publish changed cooked outputs
       -> return hot-reload hints and diagnostics
  -> Editor reloads cooked assets through runtime cooked loaders
```

The editor should not receive source importer objects, texture encoder objects, shader compiler objects, or mesh cooker internals. It receives cooked outputs and diagnostics, then asks runtime/editor asset systems to reload those cooked outputs.

Hot reload categories should be explicit:

| Category | Recook Input | Result For Editor |
|---|---|---|
| Texture | Source texture path, texture asset id, or material dependency edge | `.stex` output path and texture asset id to reload. |
| Shader | Shader package id, source shader path, or shader registry entry | Shader package/registry outputs and affected material or pipeline hints. |
| Mesh | Source scene path plus mesh id or normalized mesh record id | Cooked mesh output path and mesh asset id to reload. |
| Material | Source material id or material dependency edge | Cooked material output path plus referenced texture/shader changes. |
| Scene | Source scene path or scene asset id | Cooked scene output and affected mesh/material/camera/light references. |

API rules:

- Do not expose category cooker classes directly to the editor.
- Do not expose source-format-specific types through the public API.
- Do not expose separate texture, shader, mesh, material, or scene recook APIs. Expose one selected-recook request model and route by category internally.
- Do not expose mutable `ProjectCookPlan` internals. If inspection is needed, expose a read-only serialized plan or summary.
- Do not make the editor construct texture/shader/mesh/material cook internals. It should request intent, not assemble implementation details.
- Keep standalone CLI commands as adapters over `AssetCookRequest` and `AssetRecookRequest` where possible.
- Treat any new public header as a long-term compatibility commitment. Add public API only when another app genuinely needs it.

## Current Problems

Current cooking is split across scripts and tools:

```text
Scripts
  CookAllAssets.bat
    |
    +--> AssetCooker
           +--> ShaderCompiler
           +--> TextureCooker
           +--> SourceImportAdapters
           +--> MeshCooker / MaterialCooker / SceneCooker
```

Problems removed by the completed phases:

- Scripts own too much architecture.
- Scene enumeration happens more than once.
- Texture request list aggregation and generated/default request policy lived in the temporary `AssetConverter` bridge.
- Project-level scene, mesh, material, and texture orchestration lived outside `AssetCooker`.
- There was no explicit owner for project-level planning.

## Target Responsibilities

### Launch Shims

Launch shims are `.bat`, PowerShell, or CI entrypoints. They are deliberately boring:

```text
CookAllAssets.bat / CookAssets.ps1 / CI entrypoint
  -> establish environment
  -> locate AssetCooker
  -> forward arguments
  -> preserve exit code
```

They must not:

- Discover project assets.
- Merge request files.
- Create generated/default cook requests.
- Decide cook order.
- Select shader targets or shader backends.
- Contain source-format policy.
- Contain category-specific cook policy.

### AssetCooker

`AssetCooker` is the project-level orchestrator.

```text
AssetCooker
  -> discover inputs
  -> validate capabilities
  -> build ProjectCookPlan
  -> invoke source import adapters
  -> dispatch category cookers or current focused tools
  -> aggregate logs and diagnostics
  -> report outputs
```

`AssetCooker` should not directly decode images, compile shaders, process mesh buffers, translate material payloads, or convert scene hierarchy details. It owns the plan and the execution order, not category algorithms.

`AssetCooker` also owns the public API facade. The executable CLI and editor-callable DLL must both enter through this facade and then flow into the same private planning/dispatch implementation. The facade is intentionally small: project cook, selected recook, capability query, results, and diagnostics.

### Source Import Adapters

Source import adapters parse source formats and emit normalized records.

```text
Source import adapters
  glTF / GLB adapter
  FBX adapter
  future scene adapters
        |
        v
Normalized records
  mesh source records
  material records
  texture binding records
  scene hierarchy records
  camera records
  light records
```

Source import adapters must preserve all current source support:

- `.gltf` and `.glb` through the current glTF importer.
- `.fbx` through the current FBX importer.

They should not write cooked mesh, material, texture, scene, camera, or light outputs directly as their long-term responsibility.

### Category Cookers

The target rule is one separate cooker per major cooked output category.

| Category | Cooker | Owns |
|---|---|---|
| Shaders | `ShaderCooker` / existing `ShaderCompiler` | Shader packages, registry, backend selection, reflection, debug artifacts. |
| Textures | `TextureCooker` | Texture decode, channel extraction, mip generation, compression, `.stex`. |
| Meshes | `MeshCooker` | Mesh buffers, layout normalization, bounds, mesh identity, mesh cache keys. |
| Materials | `MaterialCooker` | Material parameters, texture references, shader requirements, material identity. |
| Scenes/Levels | `SceneCooker` | Hierarchy, transforms, instances, cameras, lights, references to cooked assets. |

Near-term, `AssetCooker` can dispatch existing tools and focused phases while these cookers are extracted. Long-term, each category should have a clear cooker module or executable-compatible wrapper.

## Compatibility Preservation

Architecture cleanup must not remove supported formats, backends, targets, debug surfaces, or cooked contracts.

| Area | Current Support To Preserve | Target Owner |
|---|---|---|
| Shader source/backend | `.slang` through `slang` backend | `ShaderCooker` / `ShaderCompiler` |
| Shader source/backend | `.hlsl` through `dxc` backend | `ShaderCooker` / `ShaderCompiler` |
| Shader targets | `DxilSm60` through `DxilSm67` | `ShaderCooker` / `ShaderCompiler` |
| Shader targets | `SpirV14` through `SpirV16` | `ShaderCooker` / `ShaderCompiler` |
| Shader CLI/debug | `cook --backend`, `cook --target`, `list-backends`, `list-targets`, package/shader inspection, debug artifacts, analysis passes | `ShaderCooker` / `ShaderCompiler` direct debug surface |
| Source scene import | `.gltf`, `.glb` | Source import adapters |
| Source scene import | `.fbx` | Source import adapters |
| Source texture import | `.dds` | `TextureCooker` |
| Source texture import | `.exr` | `TextureCooker` |
| Source texture import | `.hdr`, `.hdri` | `TextureCooker` |
| Source texture import | `.png`, `.jpg`, `.jpeg`, `.bmp`, `.tga`, `.gif`, `.psd`, `.pic`, `.pnm`, `.ppm`, `.pgm` | `TextureCooker` |

If a capability is unavailable on a machine, `AssetCooker` should report a capability validation error. It should not silently remove the feature from the plan.

## Shared Cook Services

Common behavior belongs in shared cook services instead of being duplicated across category cookers. The goal is a small common foundation, not a god service.

Shared services should live under `CookCommon` or an equivalent tool-side shared module:

| Shared Service | Responsibility | Used By |
|---|---|---|
| `CookVersionRegistry` | Register cooker versions, cooked format versions, request/plan schema versions, and compatibility policy. | `AssetCooker`, category cookers, inspection tools. |
| `CookArtifactService` | Artifact identity, source/content hashes, settings hashes, output sidecar metadata, skip/publish decisions. | All category cookers. |
| `CookPathService` | Project-relative output paths, cache paths, diagnostics paths, debug artifact paths, temp paths. | `AssetCooker`, all category cookers. |
| `CookDiagnosticsService` | Structured errors, warnings, per-asset diagnostics, summary reporting, machine-readable logs. | `AssetCooker`, source import adapters, all category cookers. |
| `CookCapabilityRegistry` | Available source adapters, shader backends, shader targets, texture codecs, cooker features. | `AssetCooker`, CLI inspection. |
| `CookPlanSerializer` | Versioned `ProjectCookPlan` read/write, validation, and inspection. | `AssetCooker`, CI, editor integration, debugging tools. |
| `CookDependencyBuilder` | Dependency edge construction and validation between source records and cooked outputs. | `AssetCooker`, future incremental planning. |

Category cookers still own category semantics. For example:

- `MeshCooker` decides which mesh fields affect a mesh content hash.
- `TextureCooker` decides which texture policies affect a texture settings hash.
- `MaterialCooker` decides which material inputs affect material identity.
- `SceneCooker` decides which hierarchy, camera, and light fields affect scene output identity.

Shared services own the mechanics:

- How hashes are represented.
- How versioned metadata is written.
- How cache hits are checked.
- How output paths are resolved.
- How diagnostics are reported.
- How plan files are versioned and inspected.

Unified versioning model:

```text
CookVersionRegistry
  tool versions
  category cooker versions
  cooked format versions
  request/plan schema versions
  compatibility and migration policy

CookArtifactService
  source/content hash
  settings hash
  format version
  cooker version
  dependency version stamp
  sidecar metadata
```

## ProjectCookPlan Contract

`ProjectCookPlan` is the central data model owned by `AssetCooker`.

It should describe:

- Source roots and discovered source assets.
- Source import adapter choices.
- Required capabilities.
- Shader packages, backends, and targets.
- Texture cook entries and texture transforms.
- Mesh cook entries.
- Material cook entries.
- Scene cook entries.
- Camera and light records.
- Generated/default assets.
- Dependency edges between source records and cooked outputs.
- Output paths.
- Cache identities.
- Diagnostics and debug artifact paths.

Data flow:

```text
Authoring inputs
  source scenes
  source textures
  shader sources/registrations
  project cook config
        |
        v
AssetCooker
  discover inputs
  validate capabilities
        |
        v
Source import adapters
  glTF adapter
  FBX adapter
  future adapters
        |
        v
Normalized records
  mesh records
  material records
  texture binding records
  scene hierarchy records
  camera records
  light records
        |
        v
ProjectCookPlan
        |
        v
Category cookers
  ShaderCompiler / ShaderCooker
  TextureCooker
  MeshCooker
  MaterialCooker
  SceneCooker
        |
        v
Sparkle cooked outputs
  shader packages + registry
  .stex textures
  cooked meshes
  cooked materials
  cooked scenes / cameras / lights
        |
        v
Runtime cooked loaders only
```

## Naming Direction

| Current/Conceptual Role | Target Name | Notes |
|---|---|---|
| Project-level cook entrypoint | `AssetCooker` | Owns planning and orchestration. Does not cook category payloads directly. |
| Editor/public cook API | `AssetCooker/Public` | Minimal DLL-facing facade for project cook, selected recook, capabilities, results, and diagnostics. |
| glTF/GLB/FBX source reading | `SourceImportAdapters` | Replaces the broad meaning of `AssetConverter`; emits normalized records only. |
| Shader package cooking | `ShaderCooker` / `ShaderCompiler` | `ShaderCompiler` remains current focused tool and direct debug surface. |
| Texture cooking | `TextureCooker` | Already correctly named for `.stex` output cooking. |
| Mesh cooking | `MeshCooker` | New category cooker for cooked mesh assets. |
| Material cooking | `MaterialCooker` | New category cooker for cooked material assets. |
| Scene/level cooking | `SceneCooker` | New category cooker for hierarchy, instances, cameras, and lights. |
| Batch/PowerShell entrypoints | launch shims | Environment setup and process invocation only. |

Avoid names that imply the wrong ownership:

- Avoid `AssetConverter` as the long-term name for cooking meshes/materials/scenes.
- Avoid `AssetTool` for the orchestrator, because it sounds like a monolithic implementation owner.
- Avoid script names that imply planning, merging, or cook policy ownership.

## Integration Prompt Plan

These prompts are intended to be used one at a time during implementation. Each prompt should leave the repository closer to the target architecture without running build, cook, or validation passes yet. Build validation and broader validation are intentionally deferred to the final prompt after the architecture is fully implemented.

Phase status must be updated in this document as work lands. When a phase is complete, mark both the table row and that phase heading as `Done` before ending the implementation pass.

| Phase | Status | Completion Rule |
|---|---|---|
| Phase 1: Thin The Launch Shims | Done | Scripts hand scene-list cooking and texture request aggregation to `AssetConverter`; deleted script orchestration helpers are gone. |
| Phase 2: Introduce AssetCooker As Planner | Done | Mark `Done` only after `AssetCooker` is the visible project cook entrypoint, scripts delegate to it, and the minimal public API facade exists. |
| Phase 3: Extract Source Import And Category Cookers | Done | Mark `Done` only after source import and mesh/material/scene cooker ownership is explicit in folders, names, and contracts. |
| Final: Deferred Validation And Fix Pass | Done | Mark `Done` only after the deferred build, cook, and validation pass succeeds or remaining risks are documented. |

### Phase 1 Prompt: Thin The Launch Shims (Status: Done)

Historical bridge shape for this completed phase:

```text
Launch shim
  -> current ShaderCompiler
  -> current AssetConverter texture-request bridge
  -> current TextureCooker
  -> current AssetConverter scene-cook bridge
```

Copy-ready prompt:

```text
Implement Phase 1 of the Sparkle asset pipeline architecture plan.

Goal:
Thin the batch/PowerShell layer so scripts become launch shims only, while preserving the current behavior and current executable entrypoints. Move scene-list iteration, texture request aggregation, and generated/default texture request construction into current tool boundaries where it is safe. Do not introduce the full AssetCooker yet.

Read first:
- docs/architecture/asset-pipeline-tool-architecture-plan.md
- Scripts/README.md
- Scripts/CookAllAssets.bat
- Scripts/CookAllAssets.bat dependencies under Scripts/Cook and Scripts/Internal
- Tools/AssetConverter command entrypoints
- Tools/TextureCooker command entrypoints
- Tools/ShaderCompiler command entrypoints

Implementation rules:
- Do not remove existing format or backend support.
- Do not change runtime loading behavior.
- Do not add source-format awareness to runtime.
- Do not run builds, tests, cook validation, or broader validation in this phase.
- Keep all scripts as environment/setup/process handoff only.
- Move script-owned planning logic toward a tool-owned or shared implementation boundary when it can be done without changing external behavior.
- Do not leave scripts responsible for generated/default request policy, cook order policy, backend selection policy, or source-format policy.
- Keep source scene enumeration as the only temporary script-side discovery bridge for this phase if moving it would require introducing `AssetCooker` early.
- Keep public script names stable unless the architecture plan explicitly says otherwise.
- Delete script helpers, wrappers, and stale code paths that no longer serve the Phase 1 bridge.
- When the phase is complete, update this document's phase status table and the Phase 1 heading to `Done`.

Expected end state:
- The script layer is visibly closer to launch-shim-only behavior.
- Any remaining script orchestration is documented as temporary and mechanically narrow.
- `AssetConverter` owns scene-list iteration for texture request collection and scene cooking.
- `AssetConverter` writes texture request files through the current `TextureCookRequestList` writer instead of scripts constructing request lines.
- Existing tools can still be invoked directly for debugging.
- No new monolithic AssetTool or AssetActions abstraction is introduced.
- The direction remains compatible with a later AssetCooker owning ProjectCookPlan creation.

Stop when:
- The scripts are simplified as far as safely possible for this phase.
- Remaining work is clearly identified as Phase 2 AssetCooker planning ownership.
- Phase 1 is marked `Done` in this plan.
- You have not run build/cook/test validation; save that for the final validation phase.
```

### Phase 2 Prompt: Introduce AssetCooker As Planner (Status: Done)

Target shape for this phase:

```text
Launch shim only
  -> AssetCooker
       -> discover inputs
       -> validate capabilities
       -> build ProjectCookPlan
       -> dispatch existing tools
       -> aggregate diagnostics
```

Copy-ready prompt:

```text
Implement Phase 2 of the Sparkle asset pipeline architecture plan.

Goal:
Introduce AssetCooker as the single project-level planner and orchestrator. AssetCooker should own input discovery, capability validation, ProjectCookPlan construction, dispatch to current focused tools, and diagnostics aggregation. Category algorithms must stay inside the existing focused tools or focused phases.

Read first:
- docs/architecture/asset-pipeline-tool-architecture-plan.md
- Tools/CMakeLists.txt
- Tools/CookCommon
- Tools/AssetConverter
- Tools/TextureCooker
- Tools/ShaderCompiler
- Scripts/CookAllAssets.bat and related launch shims

Implementation rules:
- AssetCooker is the canonical architecture name. Do not introduce AssetActions or AssetTool as competing names.
- AssetCooker owns planning and orchestration only.
- AssetCooker must not directly decode images, compile shaders, process mesh buffers, translate material payloads, or write cooked scene hierarchy details.
- AssetCooker may dispatch existing executables or existing focused tool code as the bridge implementation.
- AssetCooker must expose a minimal public API facade that can be built as a DLL for editor integration.
- The standalone executable CLI must call through the same public/private AssetCooker implementation path as the DLL facade.
- Keep the editor-facing API limited to context/config, project cook, selected recook, capability query, results, changed cooked outputs, and diagnostics.
- Do not expose source importer objects, category cooker objects, mutable ProjectCookPlan internals, cache internals, shader backend objects, texture codec objects, or mesh/material/scene builder internals through the public API.
- Keep ShaderCompiler, TextureCooker, and existing AssetConverter commands available as direct debug surfaces.
- Preserve Slang, DXC, DXIL, SPIR-V, glTF/GLB, FBX, and all existing texture source support.
- Scripts should call AssetCooker and preserve exit codes.
- Delete or move old planning/orchestration paths once AssetCooker owns them. Do not leave parallel project cook flows behind.
- Keep the folder structure aligned with the Target Folder Structure section as files move.
- When the phase is complete, update this document's phase status table and the Phase 2 heading to `Done`.
- Do not run builds, tests, cook validation, or broader validation in this phase.

Expected end state:
- A real AssetCooker target/module/entrypoint exists.
- AssetCooker can be used as a standalone executable and through a minimal DLL/editor-facing API.
- Launch shims delegate project cooking to AssetCooker.
- Editor hot reload requests can be represented as selected recook requests for textures, shaders, meshes, materials, and scenes.
- AssetCooker can represent a ProjectCookPlan or the first version of that plan contract.
- AssetCooker can dispatch the current shader, texture, and scene/asset cooking steps without absorbing their category logic.
- Diagnostics are aggregated at the AssetCooker boundary.
- Runtime remains cooked-only.

Stop when:
- AssetCooker is the single visible project cook entrypoint.
- Public API and private implementation boundaries are documented in headers/folders and match this plan.
- Any remaining direct script orchestration has been removed or reduced to temporary handoff details.
- The next clean step is extracting category cookers and shared CookCommon services.
- Phase 2 is marked `Done` in this plan.
- You have not run build/cook/test validation; save that for the final validation phase.
```

### Phase 3 Prompt: Extract Source Import And Category Cookers (Status: Done)

Target shape for this phase:

```text
AssetCooker
  -> SourceImportAdapters
  -> ShaderCooker / ShaderCompiler
  -> TextureCooker
  -> MeshCooker
  -> MaterialCooker
  -> SceneCooker
  -> CookCommon shared services
```

Copy-ready prompt:

```text
Implement Phase 3 of the Sparkle asset pipeline architecture plan.

Goal:
Split the broad AssetConverter responsibilities into SourceImportAdapters and one focused cooker per major cooked output category: MeshCooker, MaterialCooker, and SceneCooker. Keep ShaderCompiler and TextureCooker as focused category tools. Add or extend CookCommon shared services only for cross-cooker mechanics.

Read first:
- docs/architecture/asset-pipeline-tool-architecture-plan.md
- Tools/AssetConverter/Private/Assets/Import
- Tools/AssetConverter/Private/Cooking
- Tools/CookCommon
- Tools/TextureCooker
- Tools/ShaderCompiler
- Engine/GameFramework cooked asset contracts
- Engine/Renderer material and scene consumption paths where cooked contracts are consumed

Implementation rules:
- SourceImportAdapters parse source formats and emit normalized records only.
- SourceImportAdapters and category cookers must remain private implementation details behind AssetCooker unless a specific stable contract is required by another owner.
- MeshCooker owns cooked mesh assets.
- MaterialCooker owns cooked material assets and references to cooked textures/shader requirements.
- SceneCooker owns hierarchy, transforms, instances, cameras, lights, and references to cooked meshes/materials.
- TextureCooker owns texture decode, channel extraction, mip generation, compression, and .stex writing.
- ShaderCompiler or ShaderCooker owns shader packages, registry, backend selection, reflection, debug artifacts, and analysis surfaces.
- CookCommon shared services may own versioning, artifact identity, paths, diagnostics, capability registration, plan serialization, and dependency edge construction.
- CookCommon shared services must not own category semantics.
- Do not introduce a monolithic asset tool.
- Do not let runtime know about source scene or source texture formats.
- Do not remove existing direct tool debug entrypoints.
- Delete broad AssetConverter ownership, stale bridge folders, and old wrappers after their useful behavior has moved to SourceImportAdapters or the category cookers.
- Keep the editor-facing public API stable and minimal while moving implementation details behind it.
- Keep files under folders named for the owner that has the reason to change.
- When the phase is complete, update this document's phase status table and the Phase 3 heading to `Done`.
- Do not run builds, tests, cook validation, or broader validation in this phase.

Expected end state:
- AssetConverter no longer reads as the long-term owner for mesh/material/scene cooking.
- Source import is separated from cooking output categories.
- MeshCooker, MaterialCooker, and SceneCooker boundaries are explicit in names, files, CMake structure, and ownership.
- MeshCooker, MaterialCooker, SceneCooker, TextureCooker, ShaderCompiler, and SourceImportAdapters are not directly exposed as editor DLL API.
- CookCommon contains reusable mechanics instead of duplicated cache/version/path/diagnostic behavior.
- AssetCooker orchestrates the new boundaries through ProjectCookPlan.
- Existing source formats, shader backends, shader targets, texture formats, and debug surfaces are preserved.

Stop when:
- The code structure matches the ownership table in this document.
- The remaining work is final validation, cleanup, and any compile-fix pass discovered by validation.
- Phase 3 is marked `Done` in this plan.
- You have not run build/cook/test validation; save that for the final validation phase.
```

### Final Prompt: Deferred Validation And Fix Pass (Status: Done)

Use this only after the implementation phases are complete.

Copy-ready prompt:

```text
Run the final validation and fix pass for the Sparkle asset pipeline architecture implementation.

Goal:
Now that the architecture changes are fully implemented, validate the whole integration and fix issues found by the validation passes. This is the first phase where build, cook, and broader validation should be run.

Read first:
- docs/architecture/asset-pipeline-tool-architecture-plan.md
- The changed files from the implementation phases
- Existing build tasks and Scripts/README.md

Validation rules:
- Use the repository's existing build and validation entrypoints.
- Prefer existing VS Code tasks or CMake/MSBuild project targets already used by this repo.
- Validate AssetCooker integration, ShaderCompiler support, TextureCooker support, source import support, MeshCooker, MaterialCooker, SceneCooker, and runtime cooked-only boundaries.
- Validate both AssetCooker invocation modes: standalone executable and editor-callable DLL/public API.
- Validate selected recook and hot-reload output reporting for textures, shaders, and meshes at minimum.
- Verify Slang, DXC, DXIL, SPIR-V, glTF/GLB, FBX, and existing texture source support were not removed.
- Verify scripts are launch shims only.
- Verify runtime does not depend on source format loaders or cook tools.
- Verify direct debug entrypoints for focused tools still exist where intended.

Fix rules:
- Fix validation failures at the owning boundary instead of patching around them.
- Do not reintroduce script-owned orchestration.
- Do not reintroduce AssetConverter as a broad category cooker.
- Do not introduce AssetActions, AssetTool, graph scheduler, or category service names as competing architecture concepts.
- Do not expand public API to fix validation issues unless the editor or another app genuinely needs that contract.
- Keep fixes scoped to issues found by validation.
- Delete temporary bridges and obsolete compatibility code that validation proves are no longer needed.
- When validation and fixes are complete, update this document's phase status table and the Final Prompt heading to `Done`.

Expected end state:
- The implemented architecture builds.
- Cook validation succeeds through AssetCooker.
- Standalone AssetCooker and DLL/editor-facing AssetCooker paths produce equivalent cook behavior.
- Editor hot reload receives changed cooked outputs and diagnostics without seeing private cooker internals.
- Direct focused tool debug surfaces remain usable.
- Runtime consumes only cooked Sparkle assets.
- The code structure and names match this plan.
- All phase prompts are marked `Done` in this plan.
- Any remaining risks are documented clearly as follow-up work.
```

## Ownership Table

| Owner | Plans | Executes | Inputs | Outputs | Should Not Own |
|---|---|---|---|---|---|
| Launch shims | Nothing beyond command handoff | Environment setup and executable invocation | User/CI command-line arguments | Process exit code, console passthrough | Asset discovery, request merging, generated defaults, cook ordering, backend selection, source-format policy |
| AssetCooker | Project cook intent, dependency plan, validation order, public API facade | Dispatch only | Project config, asset roots, source asset list, editor recook requests | ProjectCookPlan, logs, validation report, changed cooked outputs, diagnostics | Texture compression internals, shader reflection internals, mesh processing, material translation, source format parsing details |
| Source import adapters | Source import subset of plan | glTF, GLB, FBX, and future source parsing into normalized records | Source scenes and source asset roots | Normalized mesh, material, texture binding, camera, light, transform, and hierarchy records | Cooked output writing, runtime asset loading |
| ShaderCompiler / ShaderCooker | Shader package subset of plan | Shader compile, reflection, registry/package writing | Shader registrations, shader sources, shader plan entries | Cooked shader packages, shader registry, shader logs | Scene import, texture decode, runtime binding |
| TextureCooker | Texture subset of plan | Decode, channel extraction, mip, compression, `.stex` writing | Texture plan entries, source images, cook policy | `.stex`, texture cook metadata, texture logs | Scene import, mesh processing, shader package planning |
| MeshCooker | Mesh subset of plan | Mesh validation, vertex/index normalization, bounds, mesh asset identity, mesh cache keys | Normalized mesh records | Cooked mesh assets, mesh diagnostics | Source scene parsing, texture compression, shader compilation, scene hierarchy policy |
| MaterialCooker | Material subset of plan | Material parameter normalization, cooked texture references, shader requirement references, material asset identity | Normalized material records, texture cook outputs, shader package requirements | Cooked material assets, material diagnostics | Image decoding, shader compilation, mesh buffer processing |
| SceneCooker | Scene subset of plan | Scene hierarchy, transforms, instances, camera records, light records, references to cooked assets | Normalized scene records plus cooked mesh/material references | Cooked scene/level assets, cooked camera/light records, scene diagnostics | Mesh buffer processing, texture compression, shader compilation |
| CookCommon shared services | Shared versioning, artifact, path, diagnostics, capability, and plan mechanics | Version registry, cache helpers, path resolution, diagnostics aggregation, plan serialization | Version records, cache keys, artifact paths, diagnostics events, capability descriptors | Cache metadata, version stamps, logs, validated plan files | Category semantics, source parsing, project planning policy |
| Runtime | Nothing tool-side | Load and validate cooked assets | `.stex`, cooked scene/mesh/material/camera/light assets, shader packages/registry | GPU resources, runtime asset objects | glTF, FBX, DDS source logic, AssetCooker, cook tools |

## Container Diagram Summary

Current script-heavy shape to move away from:

```text
User / CI
  |
  v
Batch / PowerShell scripts
  |          |              |
  v          v              v
ShaderCompiler  AssetConverter  TextureCooker
  |             |      |       |
  |             |      |       +--> .stex
  |             |      +----------> request files
  |             +-----------------> cooked scene/mesh/material
  +------------------------------> shader packages + registry

Problem:
  scripts own orchestration, request merging, generated defaults, and phase order
```

Target bridge:

```text
User / CI / Editor
  |
  v
Launch shim only or AssetCooker DLL API
  |
  v
AssetCooker
  - discover inputs
  - validate capabilities
  - build ProjectCookPlan
  - dispatch existing tools
  - aggregate diagnostics
  |
  +--> Source import adapters -> normalized records
  +--> ShaderCompiler         -> shader packages + registry
  +--> TextureCooker          -> .stex
  +--> Mesh cooker phase      -> cooked meshes
  +--> Material cooker phase  -> cooked materials
  +--> Scene cooker phase     -> scene, cameras, lights
```

Long-term target:

```text
User / CI / Editor
  |
  v
AssetCooker executable CLI / AssetCooker DLL public API
  |
  v
AssetCooker
  - minimal public facade
  - private planning and dispatch
  |
  +--> CookCommon shared services
  |      - CookVersionRegistry
  |      - CookArtifactService
  |      - CookPathService
  |      - CookDiagnosticsService
  |      - CookCapabilityRegistry
  |      - CookPlanSerializer
  |      - CookDependencyBuilder
  |
  +--> SourceImportAdapters -> normalized source records
  |
  +--> ShaderCompiler / ShaderCooker -> shader packages + registry
  +--> TextureCooker                 -> .stex
  +--> MeshCooker                    -> cooked mesh assets
  +--> MaterialCooker                -> cooked material assets
  +--> SceneCooker                   -> scene, instances, cameras, lights
                  |
                  v
Runtime cooked loaders only
```

## Design Checks

The plan is valid only if these remain true:

- `AssetCooker` owns the plan.
- `AssetCooker/Public` is the only editor/other-app API surface.
- Standalone CLI and DLL/editor calls share the same private cook implementation path.
- Hot reload returns changed cooked outputs and diagnostics, not private cooker internals.
- Launch shims only launch.
- Each major cooked output category has one focused cooker.
- Source import adapters emit normalized records and do not own cooked output writing.
- Shared cook services provide common mechanics across cookers.
- Current source formats, shader backends, shader targets, and debug commands remain supported.
- Runtime consumes only Sparkle cooked assets.
