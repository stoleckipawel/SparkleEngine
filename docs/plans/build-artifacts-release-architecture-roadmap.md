# Sparkle Build Artifacts And Release Architecture Roadmap

Date: 2026-06-02

## Purpose

This document defines the target architecture for Sparkle build outputs, binary artifacts, dependency handling, daily developer iteration, and versioned feature releases.

The goal is to make Sparkle feel like a serious graphics/tooling product:

- simple for new users to sync and start
- safe for daily programming work
- explicit about host prerequisites versus synced source dependencies
- ready for precompiled dependency packs later
- predictable for CI and release packaging
- aligned with mature graphics SDK/toolkit expectations from NVIDIA and AMD ecosystems

This document is the release-layout contract. CMake, launcher workflows, packaging scripts, and CI should converge on this model instead of each inventing path policy independently.

## Industry Signals

Sparkle should borrow the shape, not the scale, of mature graphics tooling releases.

### NVIDIA Patterns To Emulate

NVIDIA CUDA and Nsight releases separate several concerns that Sparkle should also separate:

- component versions are documented independently inside a larger toolkit release
- tool/runtime prerequisites are explicit, including required display driver versions
- Windows host toolchain and installer behavior are treated as part of the product contract
- release notes distinguish new features, compatibility changes, known issues, and platform support
- redistributable/runtime pieces are not confused with source build dependencies

Relevant examples:

- CUDA Toolkit release notes list major component versions and driver compatibility guidance.
- CUDA 13.1+ notes that the Windows display driver is not bundled with the toolkit and users must install the appropriate driver separately.
- Nsight Graphics release notes publish required display driver versions for the tooling to function correctly.

### AMD Patterns To Emulate

AMD ROCm and FidelityFX releases show useful patterns for dependency and artifact control:

- ROCm release notes include release highlights, supported hardware/OS changes, component versioning, known issues, and binaries.
- FidelityFX SDK releases publish versioned SDK packages with signed/prebuilt binaries.
- FidelityFX separates the minimal SDK package from optional sample/media assets that can be downloaded later.
- FidelityFX release notes list minimum prerequisites such as CMake, Visual Studio, Windows SDK, and Vulkan SDK.

Sparkle should follow the same spirit:

- publish the minimal useful package first
- keep optional assets and optional source dependencies explicit
- document exactly what a user needs for running versus rebuilding
- record component versions and package contents in manifests

## Core Decision

Sparkle should adopt three primary output roots:

```text
build/
artifacts/
dist/
```

Their meanings must stay strict.

`build/`:
- disposable CMake/generator state, object files, dependency subbuilds, compiler intermediates, and local build cache
- safe to delete at any time
- never a release source

`artifacts/`:
- runnable local and CI outputs
- developer-facing staged products
- diagnostics, symbols, logs, cooked content, and validation output
- ignored by git unless a narrow checked-in manifest/template is intentionally added

`dist/`:
- final release packages only
- zip/install/archive staging
- versioned packages, checksums, manifests, licenses, and release notes
- the only root release jobs should publish from

The important boundary is this: users run from `artifacts/` during development, but users receive packages assembled from `dist/`.

## End-To-End Pipeline

The architecture is one connected pipeline, not a set of independent cleanup tasks.

Recommended pipeline:

```text
source checkout
  -> verify host environment
  -> sync selected source dependency groups
  -> configure named build tree
  -> build product-owned artifacts
  -> cook project-owned assets
  -> assemble runtime/development/symbols packages
  -> validate package-root first-run behavior
```

Primary outputs by stage:

- `Verify Host Environment` proves the machine can rebuild, recook, or package.
- `Sync Source Dependencies` populates only the source dependency groups needed for selected capabilities.
- build workflows produce product-owned executables, libraries, runtime support files, and symbols under `artifacts/`.
- cook workflows produce project-owned generated assets under `artifacts/dev/projects/<Project>/cooked/`.
- package workflows assemble reviewable runtime, development, and symbols packages under `dist/releases/<version>/`.
- final validation proves the runtime package launches before local rebuild or recook, while the development path can still rebuild from source.

This pipeline is the reference for all phase work. If an implementation choice does not support one of these stages or makes the stage boundary harder to explain, it should be reconsidered.

## Dependency Model

Sparkle has four different dependency categories. The launcher must display them differently and must not duplicate them across actions.

### 1. Host Prerequisites

Host prerequisites are installed on the user's machine and are not synced into the repo.

Examples:

- Visual Studio 2022 with MSVC C++ workload
- Windows SDK
- CMake
- Git
- Qt MSVC kit for rebuilding the launcher
- optional Clang/clang-cl support
- optional graphics SDKs such as Vulkan SDK when a feature requires them
- GPU driver requirements for validation/profiling workflows

Launcher action:

- `Verify Host Environment`

This action should audit installed tools, versions, paths, and recovery guidance. It should not sync source dependencies.

### 2. Synced Source Dependencies

Synced source dependencies are fetched or initialized into the workspace because Sparkle builds against them.

Recommended groups:

- `Core Runtime`: required engine/runtime dependencies
- `Launcher`: dependencies needed to build the Sparkle Launcher from source, including Qt integration expectations
- `Content Pipeline`: mesh, texture, scene, and asset cooking dependencies
- `Shader Toolchain`: shader compilation, DXC/Slang/SPIR-V-related dependencies
- `Texture Containers`: KTX or similar optional texture-container support
- `Samples And Validation Assets`: sample media, smoke-test assets, screenshots, and validation-only content

Launcher action:

- `Sync Source Dependencies`

This action should show optional groups and explain what each unlocks. It should not claim the user needs every group for every workflow.

### 3. Runtime Redistributables

Runtime redistributables are files required beside a built executable or inside a package.

Examples:

- Qt runtime DLLs and plugins for `SparkleLauncher.exe`
- MSVC runtime redistributables if not statically linked
- shader compiler DLLs if tools require them at runtime
- graphics API loader DLLs where legally redistributable and required
- license files for packaged third-party components

Launcher action:

- package/deploy workflows should validate these
- `Verify Host Environment` may mention missing rebuild prerequisites, but should not treat packaged runtime DLLs as machine installs

### 4. Bundled Runtime Components

Bundled runtime components are prebuilt or pre-cooked files shipped with a runtime package so a new user can run Sparkle before rebuilding or recooking anything.

Examples:

- package-root `SparkleLauncher.exe`
- launcher Qt runtime DLLs and plugins
- prebuilt editor/runtime executables for the included sample project
- prebuilt cook tools when launcher workflows need them
- pre-cooked sample meshes, textures, shaders, and scene data
- package manifests that describe which components were prebuilt and how to regenerate them

Rules:

- bundled runtime components are runtime/product artifacts, not source dependencies
- rebuild and recook workflows should be optional for users who only want to launch and explore
- source checkout users may still need a bootstrap path if binaries are not shipped with the development package
- runtime packages should prefer "launch first, rebuild later" as the first-run experience
- launcher readiness checks should distinguish "ready from bundled runtime component" from "ready from local rebuild"
- users should be able to replace bundled runtime components with locally rebuilt/recooked outputs without changing the product model

## Daily Developer Workflow

Daily development must stay fast and boring. The architecture should help developers iterate without forcing full dependency syncs.

Recommended local flow:

1. Install host prerequisites.
2. Sync only the dependency groups needed for the task.
3. Configure with a named preset/profile.
4. Build into `artifacts/dev/...`.
5. Run launcher/editor/runtime from stable artifact paths.
6. Cook only the asset groups needed for the selected project.
7. Delete `build/` freely when generator state becomes stale.

Recommended developer artifact layout:

```text
artifacts/
  dev/
    launcher/
      SparkleLauncher.exe
      ...
    tools/
      AssetCooker.exe
      TextureCooker.exe
      ShaderCompiler.exe
      ...
    projects/
      Showcase/
        editor/
          ShowcaseEditor.exe
        runtime/
          ShowcaseRuntime.exe
        cooked/
          Shared/
            Meshes/
            Textures/
            Shaders/
  diagnostics/
    launcher/
    validation/
    screenshots/
  symbols/
    dev/
      launcher/
      tools/
      projects/
```

Rules:

- `build/` can be regenerated without losing the intended runtime layout.
- generated output folders such as `build-*`, `artifacts/`, and `dist/` should normally be git-ignored.
- the launcher should show stable artifact paths, not accidental CMake generator paths.
- local state, logs, screenshots, and validation captures must not live beside shippable binaries.

### Navigation Contract

The final folder structure should answer "what is where?" at a glance. A new contributor or user should be able to open the repository or package root and infer where to go without reading implementation code.

Top-level root meanings:

- `build/`: disposable build-system state, object files, generator cache, and local build intermediates
- `artifacts/`: local runnable outputs, diagnostics, symbols, and generated developer-facing results
- `dist/`: final packaged release outputs only
- `Engine/`: engine source and runtime code
- `Tools/`: development tools and launcher source
- `Projects/`: project content and project source roots
- `docs/`: user, developer, and release documentation

Lookup rules:

- if you want to run a locally built launcher, go to `artifacts/dev/launcher/`
- if you want to run local development tools, go to `artifacts/dev/tools/`
- if you want to run a project's editor or runtime, go to `artifacts/dev/projects/<ProjectName>/`
- if you want cooked/generated assets, go to `artifacts/dev/projects/<ProjectName>/cooked/`
- if you want logs, screenshots, or validation captures, go to `artifacts/diagnostics/`
- if you want debug symbols, go to `artifacts/symbols/`
- if you want a packaged release, go to `dist/releases/<version>/`
- if you want disposable configure/build state, go to `build/<preset-or-toolchain>/`

Naming rules:

- folder names should describe ownership or role, not implementation accidents
- folder names should prefer product nouns such as `launcher`, `tools`, `projects`, `editor`, `runtime`, `cooked`, `diagnostics`, and `symbols`
- avoid ambiguous buckets such as `bin`, `output`, `misc`, `temp`, or `stuff` as primary navigation surfaces
- keep case and plurality consistent across roots
- if a folder name needs explanation in a meeting, it probably needs renaming

Quick finding guide:

| If you are looking for... | Go here |
| --- | --- |
| disposable configure/build state | `build/<preset-or-toolchain>/` |
| locally built launcher | `artifacts/dev/launcher/` |
| locally built development tools | `artifacts/dev/tools/` |
| locally built project editor | `artifacts/dev/projects/<ProjectName>/editor/` |
| locally built project runtime | `artifacts/dev/projects/<ProjectName>/runtime/` |
| cooked/generated project assets | `artifacts/dev/projects/<ProjectName>/cooked/` |
| logs and validation captures | `artifacts/diagnostics/` |
| symbol archives | `artifacts/symbols/` |
| packaged runtime release | `dist/releases/<version>/<package-name>/` |
| package manifests and release metadata | `dist/releases/<version>/<package-name>/manifests/` |

## Build Tree Naming

Sparkle should support multiple local build trees without confusing CMake generator state.

Recommended build tree names:

```text
build/windows-msvc-x64-dev/
build/windows-msvc-x64-lite/
build/windows-msvc-x64-full/
build/windows-clangcl-x64-dev/
build/windows-clangcl-x64-full/
```

Historical migration names that are no longer final architecture:

```text
build/
build-ninja-msvc-qt/
build-ninja-msvc/
build-msvc/
```

Rules:

- one build directory must not switch generators or platform after it has a `CMakeCache.txt`
- the launcher should detect stale generator/platform cache mismatch and offer a clean-regenerate recovery
- users should be able to choose MSVC-first while still preserving clang-cl support
- build directory selection should be explicit through launcher settings, environment variables, or CMake presets
- final launcher discovery must not silently fall back to ambiguous legacy build output folders

## Product Artifact Model

### Launcher

Developer output:

```text
artifacts/dev/launcher/
  SparkleLauncher.exe
  Qt6Core.dll
  Qt6Gui.dll
  Qt6Widgets.dll
  platforms/
  styles/
  imageformats/
  licenses/
  manifests/
```

Rules:

- no editor executable here
- no runtime executable here
- no cook tools here unless intentionally packaged as launcher support tools
- Qt deployment should target this folder explicitly
- launcher state should live under `artifacts/diagnostics/launcher` or a dedicated state root, not beside the executable

### Development Tools

Developer output:

```text
artifacts/dev/tools/
  AssetCooker.exe
  TextureCooker.exe
  ShaderCompiler.exe
  dxcompiler.dll
  dxil.dll
  slang.dll
  licenses/
  manifests/
```

Rules:

- tools are grouped together
- tool runtime DLLs are explicit
- tools can be packaged for developers without making them public user-facing apps

### Projects

Developer output:

```text
artifacts/dev/projects/<ProjectName>/
  editor/
    <ProjectName>Editor.exe
  runtime/
    <ProjectName>Runtime.exe
  cooked/
    Shared/
      Meshes/
      Textures/
      Shaders/
```

Rules:

- group by project before role
- keep editor, runtime, and cooked content discoverable from the same project root
- make shared cooked domains obvious in the path

## Product Boundary Model

Sparkle may ship one repo-wide release package at first, but the package must still contain enclosed products with clear boundaries. The user should be able to tell which app they are running, which support files belong to it, which tools are private implementation details, and which pieces are safe public integration surfaces.

### First-Run Ready-To-Use Model

The preferred first-release experience is:

1. User downloads or syncs the prepared runtime package.
2. User runs `SparkleLauncher.exe` from the package root.
3. Launcher detects included bundled runtime components.
4. User can open the included sample project.
5. User can run editor/runtime using prebuilt binaries and pre-cooked assets.
6. Rebuild, recook, and dependency sync workflows are offered as optional developer paths.

This model keeps Sparkle approachable. A user should not have to install Qt, rebuild the launcher, sync all source dependencies, build cook tools, and cook assets just to see the engine running.

Rules:

- runtime package users should be able to launch first
- development package users should be able to rebuild from source with documented host prerequisites
- launcher should clearly label whether an action uses bundled runtime components or locally rebuilt outputs
- if bundled editor/runtime/cooked components are missing, launcher should explain exactly which optional workflow recreates them
- bundled runtime components must have manifest entries so users can audit version, source commit, build profile, dependency groups, and package ownership
- bundled runtime components should be replaceable by locally rebuilt outputs without changing launch workflow names
- package assembly should not silently mix stale local artifacts with current bundled runtime components

Recommended launcher first-run statuses:

- `Ready To Launch`: packaged or locally built executable and required runtime files exist
- `Ready From Package`: bundled runtime component exists and can be used immediately
- `Rebuild Available`: source/build prerequisites are present or can be installed
- `Recook Available`: cook tools and asset dependencies are present or can be synced
- `Missing Optional Payload`: action is unavailable until the matching build/cook/sync workflow runs

### Product Hierarchy

Recommended first-release hierarchy:

```text
SparkleEngine Release
  SparkleLauncher
  SparkleEditor Projects
    Showcase Editor
    Showcase Runtime
  Sparkle Build Tools
    AssetCooker
    TextureCooker
    ShaderCompiler
  Sparkle Runtime Libraries
  Sparkle Developer Files
    headers
    CMake config
    docs
    samples
  Sparkle Diagnostics And Symbols
```

Rules:

- a release package can contain multiple products, but each product must have a named owner and folder
- public user-facing apps should be obvious at the package root or under `Apps/`
- development tools should be grouped under `Tools/` and marked as developer/internal surfaces
- runtime libraries should be grouped by owning product or shared runtime role
- diagnostics, symbols, logs, and validation output should never be mixed into app folders

### Public, Private, And Internal Surfaces

Sparkle should classify every shipped file into one of these visibility levels.

`Public`:
- intended for users or external developers
- examples: `SparkleLauncher.exe`, public headers, public CMake package files, user docs, sample project files, redistributable runtime DLLs

`Internal`:
- shipped because Sparkle tools need it, but not promised as a stable external API
- examples: cook tools, helper CLIs, shader compiler support DLLs, package manifests, internal config templates

`Private`:
- not shipped in user packages unless a development package intentionally includes it
- examples: object files, CMake generator state, temporary scripts, local caches, test-only generated output, developer logs

Rules:

- public files need stable names, release notes, and compatibility expectations
- internal files may change between releases, but their owning product should still be clear
- private files must stay in `build/`, local caches, or ignored diagnostics roots
- runtime packages and development packages may expose different visibility sets

### Binary Type Classification

Every binary should be classified by type and owner.

Recommended classes:

- `App`: user-started executable, such as `SparkleLauncher.exe`
- `Developer Tool`: developer-started or launcher-started executable, such as cook tools
- `Runtime DLL`: required beside an app or tool at run time
- `Plugin DLL`: loaded through a product/plugin system
- `Import Library`: link-time file used to build against a DLL
- `Static Library`: linked into another binary
- `Symbol File`: debug information, archived separately
- `Generated Asset`: cooked data consumed by editor/runtime

Rules:

- DLLs should not float in an unexplained shared folder unless they are truly shared runtime dependencies
- each DLL needs an owning component and a list of consumers
- plugin DLLs should live under a plugin/product folder, not beside unrelated apps
- symbol files should not be shipped in the default user package
- import libraries and static libraries belong in development packages, not runtime-only packages

### Dependency Direction

Dependencies should flow inward toward stable lower-level components, not sideways through accidental output folders.

Recommended dependency direction:

```text
SparkleLauncher
  depends on Launcher Core
  depends on Qt runtime
  invokes build/cook/launch tools through workflows

Project Editor
  depends on Engine Runtime
  depends on project modules
  consumes cooked/generated assets when required

Project Runtime
  depends on Engine Runtime
  depends on project runtime modules
  consumes cooked/generated assets

Cook Tools
  depend on Engine/Core libraries
  depend on optional content-pipeline libraries
  produce cooked assets

Shader Tools
  depend on shader toolchain libraries
  produce shader artifacts

Engine Runtime Libraries
  should not depend on launcher GUI
  should not depend on editor-only or cook-only tools
```

Rules:

- launcher may orchestrate tools, but tools must not depend on launcher GUI
- engine runtime must not depend on build-system state or launcher state
- editor can depend on runtime, but runtime should not depend on editor-only code
- cook tools can depend on asset pipeline libraries, but app launch should only require their outputs, not the tools themselves
- package assembly should copy declared dependencies, not scan unrelated neighboring DLLs

### Rebuild Boundaries

Sparkle should make rebuild impact understandable.

Recommended rebuild groups:

- `Launcher Rebuild`: launcher GUI, launcher core, Qt deployment
- `Engine Runtime Rebuild`: core engine libraries and runtime targets
- `Editor Rebuild`: editor-only modules and selected project editor target
- `Tools Rebuild`: cook tools, shader tools, and tool support libraries
- `Content Recook`: generated mesh, texture, shader, and scene assets
- `Package Reassembly`: copies already-built outputs into `dist/` and regenerates manifests/checksums

Rules:

- changing launcher UI should not require recooking assets
- changing shader compiler code should not require rebuilding launcher unless launcher APIs changed
- changing project runtime code should not require rebuilding unrelated tools
- changing asset source data should trigger recook, not full source rebuild
- changing runtime redistributable deployment rules should trigger package reassembly
- launcher UI should explain whether an action rebuilds code, recooks assets, or only repackages outputs

### Package Inclusion Matrix

Recommended first-release inclusion model:

| Component | Runtime Package | Development Package | Symbols Package |
| --- | --- | --- | --- |
| SparkleLauncher app | Yes | Optional | Symbols only |
| Qt runtime for launcher | Yes | No, if user rebuilds with installed Qt | No |
| Prebuilt sample editor/runtime | Yes for first-run demos | Optional | Symbols only |
| Engine source | Optional for runtime package, yes for development package | Yes | No |
| Public headers/CMake config | Yes if supporting rebuilds | Yes | No |
| Cook tools | Yes if launcher workflows need local cooking | Yes | Symbols only |
| Shader compiler/runtime DLLs | Yes if workflows need them | Yes if source-built or fetched | Symbols only |
| Sample projects | Yes for first release | Yes | No |
| Cooked sample assets | Yes for first-run demos | Optional | No |
| Private build state | No | No | No |
| Logs/screenshots/diagnostics | No | No | Optional CI artifact only |
| PDB/symbol files | No | No | Yes |

Rules:

- the first release may ship one combined repo/tooling package, but the matrix still decides what belongs in that package
- runtime packages should include enough bundled runtime components to launch the launcher and included sample editor/runtime immediately
- later releases can split launcher-only, engine-source, sample-content, symbols, and dependency-pack downloads without redesigning ownership
- every included component should be traceable in `sparkle-package-files.json`

## Release Channels

Sparkle should use release channels instead of treating every zip as the same kind of release.

Recommended channels:

- `local`: developer machine output, never published
- `nightly`: automated build from an integration branch, may be unstable
- `feature`: named feature branch/candidate build for review
- `preview`: manually promoted release candidate
- `stable`: public release

Recommended version examples:

```text
0.3.0-dev.20260602+g1a2b3c4
0.3.0-nightly.20260602+g1a2b3c4
0.3.0-feature.launcher-deps.4+g1a2b3c4
0.3.0-preview.1+g1a2b3c4
0.3.0
```

Rules:

- stable releases should use semantic versioning: `major.minor.patch`
- prerelease builds should include a channel label
- build metadata should include commit SHA, dirty state, toolchain, Qt version, CMake preset, and dependency groups
- package filenames should include product, version, platform, architecture, and channel when relevant

Example filenames:

```text
SparkleEngine-0.3.0-preview.1-windows-x64.zip
SparkleLauncher-0.3.0-preview.1-windows-x64.zip
SparkleEngineSymbols-0.3.0-preview.1-windows-x64.zip
SparkleDependencies-content-pipeline-0.3.0-windows-x64.zip
```

## Release Package Layout

Recommended final release layout:

```text
dist/
  releases/
    0.3.0-preview.1/
      SparkleEngine-0.3.0-preview.1-windows-x64/
        SparkleLauncher.exe
        SparkleLauncher/
          runtime/
            Qt/
            plugins/
          licenses/
        Apps/
          ShowcaseEditor/
            ShowcaseEditor.exe
            runtime/
          ShowcaseRuntime/
            ShowcaseRuntime.exe
            runtime/
        Engine/
        Tools/
          Cookers/
        Projects/
          Showcase/
            Cooked/
              Shared/
                Meshes/
                Textures/
                Shaders/
        CMake/
        CMakeLists.txt
        docs/
        licenses/
        manifests/
          sparkle-release-manifest.json
          sparkle-build-manifest.json
          sparkle-dependency-manifest.json
          sparkle-package-files.json
          sparkle-bundled-runtime-components.json
        README.md
        RELEASE_NOTES.md
      SparkleEngine-0.3.0-preview.1-windows-x64.zip
      SparkleEngine-0.3.0-preview.1-windows-x64.sha256
      SparkleEngineSymbols-0.3.0-preview.1-windows-x64.zip
```

Rules:

- release jobs assemble from `artifacts/` into `dist/`
- release jobs publish from `dist/` only
- the package root should expose the launcher entrypoint clearly
- runtime packages should include prebuilt launcher, sample editor/runtime, and pre-cooked sample assets when available
- rebuild and recook instructions should be documented as optional paths, not first-run requirements
- development and runtime packages may be separate once the project matures
- symbols should be archived separately from user packages
- license and manifest files should be included with every package

Package navigation rules:

- package root should contain the primary user entrypoint: `SparkleLauncher.exe`
- app executables should live under `Apps/`
- app-specific runtime support files should live under that app's folder or explicit runtime subfolder
- cooked sample assets should live under `Projects/<ProjectName>/Cooked/`
- buildable source material should remain under `Engine/`, `Tools/`, `Projects/`, `CMake/`, and `docs/`
- manifests should live only under `manifests/`
- licenses should live under `licenses/` or app-local license folders when required
- package root should never become a flat DLL landfill

## Manifest Requirements

Every release package should include machine-readable manifests.

### `sparkle-release-manifest.json`

Records the package identity:

- product name
- package version
- release channel
- platform and architecture
- commit SHA
- dirty flag
- build timestamp
- package format
- package checksum

### `sparkle-build-manifest.json`

Records how the binaries were produced:

- CMake version
- CMake preset/configure options
- generator
- compiler family and version
- Windows SDK version
- Qt version and kit
- build configuration/profile
- enabled Sparkle feature flags

### `sparkle-dependency-manifest.json`

Records dependency state:

- host prerequisites required to rebuild
- synced source dependency groups included
- source dependency revisions
- runtime redistributables included
- optional dependencies omitted
- licenses that apply to packaged content

### `sparkle-package-files.json`

Records package contents:

- relative file paths
- sizes
- hashes
- owning component
- whether the file is source, binary, asset, runtime, license, manifest, or documentation

### `sparkle-bundled-runtime-components.json`

Records prebuilt and pre-cooked components that allow first-run usage without rebuilding.

- launcher executable and runtime support files
- sample editor/runtime executable paths
- pre-cooked sample asset roots
- build profile and toolchain used to create each component
- source commit and dependency groups used to create each component
- whether the component can be replaced by rebuild, recook, sync, or package assembly
- matching launcher workflow that regenerates or replaces the component

## Feature Release Workflow

A feature release is a versioned build of a meaningful branch, not an untracked local archive.

Recommended flow:

1. Choose feature version and channel, for example `0.3.0-feature.launcher-deps.4`.
2. Sync only required dependency groups.
3. Configure with the intended preset.
4. Build products into `artifacts/feature/<version>/...`.
5. Cook required project assets into the same feature artifact root.
6. Run smoke validation against staged artifacts.
7. Assemble the final package into `dist/releases/<version>/...`.
8. Generate manifests, checksums, and release notes.
9. Archive symbols separately.
10. Validate the package-root `SparkleLauncher.exe`.

Success criteria:

- package can be extracted onto a fresh machine with only runtime prerequisites
- package documents what is needed to rebuild from source
- launcher shows missing optional dependency groups as unlockable capabilities, not as universal failures
- generated manifests explain exactly what was built and shipped

## CMake And Launcher Requirements

### CMake

Required changes:

- define shared path variables for build state, artifact roots, dist roots, launcher output, tools output, project output, cooked output, diagnostics, and symbols
- assign runtime output directories by product role, not one global `build/bin/<Config>` bucket
- preserve MSVC as the recommended Windows path while keeping clang-cl presets working
- keep optional dependency groups behind feature flags
- generate build/package manifests from configured variables
- make Qt deployment target the launcher artifact/package folder explicitly

Recommended variables:

```text
SPARKLE_BUILD_STATE_ROOT
SPARKLE_ARTIFACTS_ROOT
SPARKLE_DIST_ROOT
SPARKLE_LAUNCHER_OUTPUT_ROOT
SPARKLE_TOOLS_OUTPUT_ROOT
SPARKLE_PROJECT_OUTPUT_ROOT
SPARKLE_COOKED_OUTPUT_ROOT
SPARKLE_DIAGNOSTICS_ROOT
SPARKLE_SYMBOL_OUTPUT_ROOT
```

### Launcher

Required changes:

- make the launcher workflow-first, not function-first
- show host prerequisites separately from syncable source dependency groups
- show dependency groups as capability unlocks
- avoid duplicated prerequisite wording between `Verify Host Environment` and `Sync Source Dependencies`
- resolve build trees explicitly and warn about stale generator/platform caches
- show artifact and package output paths in previews
- validate runtime redistributables during package/deploy workflows
- report package/rebuild requirements separately for runtime-package users versus development-package users

Launcher wording should preserve this mental model:

- `Verify Host Environment`: "What is installed on this machine?"
- `Sync Source Dependencies`: "What source dependency groups are available in this workspace?"
- `Build`: "What binaries are compiled?"
- `Cook`: "What assets are generated?"
- `Package`: "What runtime or development package is ready to ship?"

### Launcher Workflow Product Requirements

The launcher should primarily support user workflows, not expose internal functions merely because they exist. A user should see outcomes they want to achieve, while implementation details remain secondary.

Recommended workflow groups:

- `Start`: open an existing project, run the launcher health check, and see what is possible now
- `Setup`: verify host environment, select dependency groups, sync source dependencies, and generate project files
- `Build`: build launcher, tools, editor, runtime, or selected product groups
- `Cook`: prepare assets by capability, such as scene assets, textures, and shaders
- `Run`: launch editor/runtime and run smoke tests
- `Package`: assemble runtime or development packages
- `Maintenance`: clean scoped generated outputs, format source, inspect diagnostics, and repair stale build state

Rules:

- the first-run path should prioritize launching included bundled runtime components before asking for rebuilds or recooks
- a launcher action should map to a user outcome, not just a CMake target or helper function
- advanced/internal operations can exist, but should be grouped under explicit advanced or diagnostics areas
- every workflow should explain what it will change: host machine, workspace dependencies, build outputs, cooked outputs, package outputs, or diagnostics
- every workflow should explain what unlocks it and what it unlocks next
- the launcher should prefer guided recovery actions over raw error dumps
- the launcher should show product boundaries, such as launcher app, tools, editor/runtime, cooked content, package, and symbols
- launch workflows should prefer bundled runtime components when local rebuild outputs are missing
- rebuild and recook workflows should be presented as ways to customize or refresh outputs, not as mandatory first-run steps
- package/release workflows should be framed as "assemble and verify a product", not "copy files"
- destructive workflows should be scoped, named by outcome, and confirmed

Examples:

- Prefer `Prepare Windows MSVC Workspace` over exposing unrelated configure/sync/check buttons as peers.
- Prefer `Build Showcase Editor` over `Run CMake Target`.
- Prefer `Cook Missing Runtime Assets` over forcing the user to infer which cook tool failed.
- Prefer `Assemble Preview Package` over `Copy Artifact Files`.
- Prefer `Repair Stale Build Directory` over telling the user to manually delete `CMakeCache.txt`.

## CI Requirements

CI should treat release output as a promoted product, not as whatever files happened to be in the build tree.

Recommended CI stages:

1. checkout
2. host toolchain audit
3. dependency group sync
4. configure
5. build
6. cook required assets
7. run smoke validation
8. stage artifacts
9. assemble `dist/`
10. generate manifests and checksums
11. archive symbols
12. publish release package

Rules:

- CI should upload `artifacts/` for diagnostics.
- CI should publish `dist/` for releases.
- CI should fail if release manifests are missing.
- CI should fail if package-root launcher cannot start or cannot locate the repository/package root.
- Signing can be future work, but the layout should leave room for signed executables and signed installers.

## What To Avoid

Do not:

- release directly from `build/`
- release directly from `artifacts/`
- mix launcher, editor, runtime, and cook tools into one generic `bin` folder
- treat optional dependency groups as required for every user
- duplicate machine prerequisite checks inside source dependency sync actions
- hide package runtime DLLs inside unexplained copy steps
- keep local state, screenshots, or logs beside release binaries
- change generator/platform in an existing CMake build directory without cleaning it
- ship packages without version, manifest, checksum, and release notes

## Goal Coverage Matrix

Every major goal in this document must be owned by at least one implementation phase. If a future goal is added, this matrix should be updated before implementation starts.

| Goal | Primary Phase | Supporting Phases | Required Outcome |
| --- | --- | --- | --- |
| Separate disposable build state, runnable artifacts, and final releases | Phase 1 | Phase 3, Phase 5, Phase 6 | `build/`, `artifacts/`, and `dist/` have distinct meanings and matching code paths |
| Keep one repo-wide first release while preserving product boundaries | Phase 5 | Phase 0, Phase 1, Phase 3 | one package may include the repo/tools, but launcher, tools, runtime, projects, symbols, and docs remain separately owned |
| Provide bundled runtime components for first-run users | Phase 5 | Phase 2, Phase 3, Phase 4, Phase 6 | new users can launch the launcher and included sample editor/runtime from prebuilt and pre-cooked components before rebuilding or recooking |
| Classify public, internal, and private surfaces | Phase 1 | Phase 0, Phase 5 | each shipped file category has visibility rules and package inclusion policy |
| Classify apps, tools, DLLs, plugins, static libs, symbols, and generated assets | Phase 0 | Phase 1, Phase 3, Phase 5 | binaries and generated outputs have owner, type, consumers, and shipping destination |
| Define dependency direction between launcher, tools, runtime, editor, cook, shader, and project outputs | Phase 0 | Phase 2, Phase 3, Phase 4 | dependencies flow through declared relationships, not accidental shared output folders |
| Define rebuild boundaries | Phase 2 | Phase 3, Phase 4, Phase 5 | launcher actions explain whether they rebuild code, recook assets, repackage, or repair state |
| Separate host prerequisites, synced source dependencies, and runtime redistributables | Phase 2 | Phase 1, Phase 5, Phase 6 | `Verify Host Environment`, `Sync Source Dependencies`, and package validation do not duplicate responsibilities |
| Make the launcher workflow-first instead of function-first | Phase 2 | Phase 6 | launcher primary UI exposes user outcomes: Start, Setup, Build, Cook, Run, Package, Maintenance |
| Support partial dependency sync and capability unlocks | Phase 2 | Phase 4, Phase 6 | users can sync only what they need, and launcher shows what each group unlocks |
| Make rebuild and recook optional for runtime-package exploration | Phase 2 | Phase 5, Phase 6 | launcher distinguishes bundled runtime component readiness from local rebuild readiness and presents rebuild/recook as optional refresh/customization paths |
| Move developer runnable outputs into product-aware artifact roots | Phase 3 | Phase 1, Phase 6 | launcher, tools, editor/runtime, diagnostics, and symbols no longer rely on generic `build/bin` as the target architecture |
| Move cooked content into project artifact roots | Phase 4 | Phase 3, Phase 6 | cooked meshes, textures, shaders, and scenes have clear project/shared domains |
| Assemble versioned release packages from `dist/` only | Phase 5 | Phase 1, Phase 6 | release jobs consume artifacts and publish final packages from `dist/releases/<version>` |
| Generate manifests, checksums, release notes, and symbol packages | Phase 5 | Phase 6 | release outputs describe identity, build inputs, dependencies, files, hashes, and symbols |
| Support daily programming work without unnecessary full validation | Phase 3 | Phase 1, Phase 2, Phase 4 | local workflows stay fast, scoped, and compatible with direct CMake/MSBuild work |
| Support feature releases with versions and channels | Phase 5 | Phase 1, Phase 6 | feature/preview/stable releases have versioned folders, package names, manifests, and notes |
| Preserve MSVC-first Windows path while maintaining clang-cl support | Phase 1 | Phase 3, Phase 6 | MSVC remains recommended, clang-cl is supported or explicitly documented if blocked |
| Defer final build/package validation until the end | Phase 6 | all earlier phases | phases 0-5 avoid claiming release readiness; Phase 6 performs end-to-end validation |

## Phase Prompt Usage Rules

Each prepared prompt should be used as a phase contract, not as a vague request. The implementer should read this document first, then perform only the requested phase.

For phases 0-5, the implementer should return:

- files changed or inspected
- decisions made
- ownership mappings added or changed
- compatibility behavior preserved
- known risks or unresolved blockers
- lightweight checks performed, if any
- explicit confirmation that final build/package validation was not run

For Phase 6, the implementer should return:

- exact commands run
- tool versions detected
- build directories used
- artifacts and dist paths produced
- manifest/checksum results
- launcher/package smoke results
- failures, risks, and next fixes

## Phase Handoff Model

Each phase should leave behind something the next phase can directly consume.

| Phase | Produces | Consumed By |
| --- | --- | --- |
| Phase 0 | inventory of paths, actions, binaries, generated outputs, dependency groups, and ownership risks | Phase 1 naming/configuration and Phase 2 launcher UX |
| Phase 1 | shared vocabulary, root variables, package names, visibility classes, binary classes, and navigation contract | Phase 2 launcher labels, Phase 3 artifact roots, Phase 5 manifests |
| Phase 2 | workflow-first launcher presentation, action prerequisites, readiness hierarchy, and capability unlock model | Phase 3 artifact discovery, Phase 4 asset readiness, Phase 6 launcher validation |
| Phase 3 | product-owned developer artifact roots and declared runtime support-file ownership | Phase 4 cooked roots, Phase 5 package assembly |
| Phase 4 | project-owned cooked asset roots and exact cook recovery actions | Phase 5 bundled runtime component staging and Phase 6 launch validation |
| Phase 5 | runtime/development/symbols package assembly, manifests, checksums, release notes, and bundled runtime component records | Phase 6 final validation |
| Phase 6 | release readiness report with exact commands, package paths, validation results, risks, and follow-up fixes | release decision and next roadmap pass |

Implementation rule:

- do not start a later phase by rediscovering decisions that an earlier phase was supposed to produce
- if an earlier phase output is missing, record the gap and complete the missing handoff before continuing
- if a phase changes a contract from an earlier phase, update the goal coverage matrix and affected prompts in the same pass

## Implementation Roadmap

This roadmap is ordered so Sparkle reaches the target architecture without repeatedly validating half-migrated states. Each phase should include focused code review and lightweight checks, but full build/package validation is intentionally reserved for the final phase.

### Phase 0: Baseline Inventory And Freeze

Goal:

- capture the current state before changing layout policy
- identify every current path assumption in CMake, launcher code, scripts, docs, and CI
- prevent new work from adding more `build/bin` or `build/Cooked` assumptions while migration is active

Deliverables:

- inventory of all hardcoded output paths
- inventory of launcher actions and their prerequisites
- inventory of dependency groups and feature flags
- list of current package/deploy/copy commands
- list of currently generated local directories that should be ignored by git
- inventory of current apps, tools, DLLs, static/import libraries, symbols, and generated assets
- initial owner/consumer map for binaries and generated outputs

Scope rules:

- do not move outputs yet
- do not redesign packaging yet
- do not run final build validation in this phase

Completion criteria:

- every known path assumption is listed with an owner: CMake, launcher core, launcher GUI, script, doc, or CI
- every launcher action has an identified dependency category: host prerequisite, synced source dependency, runtime redistributable, build output, cooked output, or project selection
- every current binary/output class has an initial owner, visibility level, and likely shipping destination
- the team knows what will break when output roots move

Prepared prompt:

```text
Please perform Phase 0 of docs/plans/build-artifacts-release-architecture-roadmap.md. Do an inventory-only pass across CMake, launcher core/GUI, scripts, docs, and CI. List every current output path assumption, launcher action prerequisite, dependency group reference, package/deploy command, generated directory, app, development tool, DLL, library, symbol file, and generated asset. Assign each finding an owner, visibility level when possible, dependency category, likely shipping destination, and risk if it remains unchanged. Do not move files or change behavior. Return concise findings with file references and recommended owners. Confirm final build/package validation was not run.
```

### Phase 1: Naming Contract And Shared Configuration

Goal:

- make the architecture explicit before moving products
- introduce shared names for roots, channels, dependency categories, and package identities
- keep existing outputs working while adding the new vocabulary

Deliverables:

- documented path variables for `build/`, `artifacts/`, and `dist/`
- CMake cache variables or presets for artifact and dist roots
- launcher-visible names for host prerequisites, source dependency groups, and runtime redistributables
- shared vocabulary for public, internal, and private surfaces
- shared vocabulary for app, developer tool, runtime DLL, plugin DLL, import library, static library, symbol file, and generated asset
- release channel and version naming helpers or constants
- git ignore coverage for generated build/artifact/dist roots
- documented navigation contract for repository and package roots

Scope rules:

- compatibility with current output paths may remain
- no final package assembly yet
- no final build validation yet

Completion criteria:

- CMake and launcher code can refer to shared path/category names instead of inventing strings locally
- docs, launcher labels, and CMake variables use the same terminology
- dependency categories are not duplicated between `Verify Host Environment` and `Sync Source Dependencies`
- product visibility and binary type names are available for manifests, launcher descriptions, and package rules
- repository roots and package roots have a documented, human-navigable meaning

Prepared prompt:

```text
Please perform Phase 1 of docs/plans/build-artifacts-release-architecture-roadmap.md using the Phase 0 inventory as input. Add the shared naming/configuration contract for `build/`, `artifacts/`, `dist/`, dependency categories, product visibility, binary type classifications, release channels, version/package naming, and repository/package navigation. Preserve existing behavior for now. Update docs, launcher labels, CMake variables, and package concepts so they use consistent terminology. Avoid final build validation; use only lightweight compile/config sanity checks if needed. Confirm final build/package validation was not run.
```

### Phase 2: Launcher Dependency And Workflow UX

Goal:

- refactor launcher presentation around user outcomes instead of internal functions
- make launcher actions explain what users actually need
- separate machine-installed prerequisites from syncable dependency groups
- make optional dependency groups visible as capability unlocks instead of mandatory setup
`
Deliverables:

- workflow-first launcher grouping: Start, Setup, Build, Cook, Run, Package, and Maintenance
- action labels and descriptions rewritten around outcomes, not implementation details
- `Verify Host Environment` or equivalent action focused on installed tools
- `Sync Source Dependencies` focused only on workspace dependency groups
- prerequisite text for each launcher action audited and corrected
- runtime redistributables described only where package/deploy workflows need them
- first-run readiness states for bundled runtime components versus local rebuild outputs
- recovery guidance for stale CMake generator/platform mismatch
- advanced/internal operations grouped away from primary user workflows
- launcher action metadata that explains rebuild, recook, repackage, repair, or diagnostic impact

Scope rules:

- do not migrate all product outputs yet
- do not add package release validation yet
- do not require full dependency sync to run launcher-only workflows
- do not expose new buttons just because a helper function exists

Completion criteria:

- launcher primary actions describe user outcomes and next steps
- each action explains what it changes: host machine, workspace dependencies, build outputs, cooked outputs, package outputs, or diagnostics
- each build/cook/package action explains its rebuild boundary and dependent outputs
- workflows show what is currently available and what dependency groups would unlock more
- launch actions can use bundled runtime components when local rebuild outputs are absent
- rebuild and recook actions are shown as optional ways to replace or refresh bundled runtime components
- setup/build/cook/launch/maintenance/package actions list real prerequisites
- no action says "generate solution to generate solution"
- no full machine prerequisite list is duplicated inside source dependency sync
- users can understand what more they unlock by syncing each group

Prepared prompt:

```text
Please perform Phase 2 of docs/plans/build-artifacts-release-architecture-roadmap.md using the Phase 0 inventory and Phase 1 naming contract as input. Refactor Sparkle Launcher presentation around user workflows, not internal functions: Start, Setup, Build, Cook, Run, Package, and Maintenance. Audit dependency/workflow UX so host prerequisites, syncable source dependency groups, runtime redistributables, bundled runtime components, build outputs, cooked outputs, and project selection are distinct. Make primary views show outcome, readiness, blockers, next action, and what the action changes. Move detailed inventories, raw paths, and advanced operations into secondary/detail surfaces. Make launch workflows prefer bundled runtime components when local rebuild outputs are absent, and present rebuild/recook as optional refresh/customization paths. Fix confusing or circular prerequisites. Do not move product output roots yet. Confirm final build/package validation was not run.
```

### Phase 3: Developer Artifact Roots

Goal:

- move daily runnable outputs out of generic build bins
- make launcher, tools, editor, runtime, diagnostics, and symbols discoverable by product role
- preserve developer speed and direct CMake/MSBuild escape hatches

Deliverables:

- launcher output root under `artifacts/dev/launcher`
- development tool output root under `artifacts/dev/tools`
- project editor/runtime roots under `artifacts/dev/projects/<Project>/...`
- diagnostics root under `artifacts/diagnostics`
- symbol root under `artifacts/symbols`
- no compatibility lookup for old output locations in the final strict architecture
- declared owner/consumer relationships for product runtime DLLs and support files
- no primary developer workflow depends on ambiguous top-level `bin`-style browsing

Scope rules:

- do not assemble final `dist/` packages yet
- do not move cooked content unless required for path wiring
- do not run final build/package validation yet

Completion criteria:

- CMake target output policy is product-aware
- launcher discovery prefers artifact roots
- Qt deployment targets the launcher artifact folder explicitly
- development tools no longer appear as accidental siblings of launcher/editor outputs in the target architecture
- runtime DLLs and support files are copied by declared product ownership rather than accidental folder scanning
- a contributor can locate launcher, tools, projects, cooked assets, diagnostics, and symbols by folder name alone

Prepared prompt:

```text
Please perform Phase 3 of docs/plans/build-artifacts-release-architecture-roadmap.md using the Phase 1 path contract and Phase 2 launcher readiness model as input. Move developer runnable outputs to product-aware artifact roots: launcher, development tools, project editor/runtime, diagnostics, and symbols. Declare owner/consumer relationships for runtime DLLs and support files so products copy dependencies intentionally, not by accidental shared-bin scanning. Update launcher discovery and previews to use artifact roots as the source-of-truth output locations. Do not assemble `dist/` packages. Confirm final build/package validation was not run.
```

### Phase 4: Cooked Content And Asset Capability Roots

Goal:

- move generated asset outputs into the artifact model
- make asset readiness checks align with dependency groups
- make missing cooked content guidance point to the exact cook action

Deliverables:

- cooked content root under `artifacts/dev/projects/<Project>/cooked`
- clear shared versus project-local cooked domains
- launcher readiness checks updated for mesh, texture, shader, and full cook workflows
- clean scopes updated so users can clean generated assets without destroying source or host dependencies
- package-facing cooked content staging rules drafted

Scope rules:

- do not package final releases yet
- do not require optional cook groups for launcher-only or editor-only workflows
- do not run final build validation yet

Completion criteria:

- launch workflows know which cooked outputs are missing
- missing mesh/texture/shader outputs point to the matching cook action
- clean actions are scoped and recoverable
- optional asset/toolchain dependency groups unlock the correct cook workflows

Prepared prompt:

```text
Please perform Phase 4 of docs/plans/build-artifacts-release-architecture-roadmap.md using the Phase 3 artifact roots as input. Move cooked content and asset readiness into the artifact model under project-owned cooked roots, with clear project/shared domains. Update launcher readiness and recovery so missing meshes, textures, shaders, and full project cooked content point to the exact matching cook action. Keep optional cook groups optional and preserve launcher-only/editor-only paths that do not need cooking. Do not package or run final build validation. Confirm final build/package validation was not run.
```

### Phase 5: Release Assembly And Manifests

Goal:

- create the final package assembly model without treating local artifacts as releases
- make every package explain what it contains and how it was produced
- prepare for runtime packages and development packages

Deliverables:

- `dist/releases/<version>/...` assembly layout
- package-root `SparkleLauncher.exe` contract
- bundled runtime component staging for launcher, sample editor/runtime, and pre-cooked sample assets
- runtime redistributable deployment into package layout
- release, build, dependency, and package-file manifests
- bundled runtime component manifest generation
- checksum generation
- release notes template
- separate symbols archive rule
- initial dependency pack naming/versioning rules
- package inclusion rules for public, internal, private, runtime, development, and symbols packages
- package navigation rules for apps, runtime support, cooked assets, manifests, and licenses

Scope rules:

- implementation can add package scripts/targets, but full final validation waits for Phase 6
- do not publish from `artifacts/`
- do not make package assembly depend on hidden local state

Completion criteria:

- package assembly consumes `artifacts/` and writes `dist/`
- runtime package can be inspected and shows the intended launcher/editor/runtime/cooked bundled runtime components
- package manifests record version, commit, toolchain, Qt kit, dependency groups, runtime redistributables, and file hashes
- bundled runtime component manifest records how each prebuilt/pre-cooked component was produced and how it can be regenerated
- the package layout is reviewable before publishing
- symbols are separate from user-facing packages
- package inclusion is driven by product ownership, visibility, binary type, and package kind
- a new user can locate the launcher, apps, cooked assets, manifests, and licenses from folder names alone

Prepared prompt:

```text
Please perform Phase 5 of docs/plans/build-artifacts-release-architecture-roadmap.md using Phase 3 artifact roots and Phase 4 cooked roots as input. Add release assembly into `dist/releases/<version>`, including package-root launcher layout, bundled runtime component staging for launcher, sample editor/runtime, and pre-cooked sample assets, runtime redistributable deployment, manifests including bundled runtime component manifests, checksums, release notes template, symbols archive rules, dependency-pack naming rules, and package inclusion rules for runtime, development, and symbols packages. Ensure package inclusion follows product ownership, visibility, binary type, declared dependencies, and package navigation rules. Do not publish and do not claim release readiness. Confirm final build/package validation was not run.
```

### Phase 6: Final Build, Package, And Fresh-Machine Validation

Goal:

- validate the entire architecture end to end only after the migration is complete
- prove daily development, package assembly, and source rebuild requirements all work
- produce the release readiness report

Deliverables:

- clean configure using the recommended Windows MSVC preset/profile
- clang-cl configure/build smoke if supported by current presets
- launcher build from source
- editor/runtime/tool builds for the selected validation project
- required cook workflows
- package assembly into `dist/releases/<version>/...`
- package-root launcher startup smoke
- first-run launcher/editor/runtime smoke using bundled runtime components before any rebuild or recook
- manifest/checksum verification
- dependency report for runtime-package users versus development-package users
- release readiness report with failures, risks, and next fixes

Final validation checklist:

1. Delete or isolate stale build directories that would hide generator/platform problems.
2. Configure from a clean build tree.
3. Build `SparkleLauncher`.
4. Build required development tools.
5. Build selected editor/runtime targets.
6. Cook required validation assets.
7. Launch package-root or artifact-root launcher.
8. Confirm launcher finds the repository/package root.
9. Confirm launcher lists host prerequisites and dependency groups correctly.
10. Assemble `dist/releases/<version>/...`.
11. Verify manifests and checksums exist.
12. Verify package-root launcher starts.
13. Verify included editor/runtime can launch from bundled runtime components before rebuild or recook.
14. Verify bundled runtime component manifest explains how each component was produced and regenerated.
15. Verify release package documents rebuild dependencies as optional developer requirements.
16. Verify package contents follow product ownership, visibility, binary type, and dependency rules.
17. Verify launcher workflows communicate user outcomes rather than internal function names.
18. Record exact commands, versions, paths, and failures.

Completion criteria:

- runtime package works from `dist/`
- runtime package supports launch-first exploration before local rebuild or recook
- source checkout path still supports rebuilding the launcher
- MSVC remains the recommended Windows path
- clang-cl support is preserved or clearly documented as pending if blocked by real implementation limits
- dependency groups match launcher capabilities
- no release package is built directly from `build/`

Prepared prompt:

```text
Please perform Phase 6 of docs/plans/build-artifacts-release-architecture-roadmap.md only after Phases 0-5 have produced their handoff outputs. Run the final end-to-end validation: clean configure, build launcher/development tools/editor/runtime, cook required assets, assemble the `dist/` runtime package, verify manifests/checksums, smoke package-root launcher startup, verify first-run launcher/editor/runtime behavior from bundled runtime components before any rebuild or recook, verify package contents against product ownership/visibility/binary-type/dependency rules, verify launcher workflows communicate user outcomes and present rebuild/recook as optional refresh paths, and produce a release readiness report. Preserve MSVC as the recommended Windows path and verify clang-cl support where current presets allow it. Return exact commands, tool versions, build/artifact/dist paths, package paths, results, failures, risks, and recommended next fixes.
```

## Final Recommendation

Sparkle should treat build outputs like a product contract:

- `build/` is disposable machine state
- `artifacts/` is runnable developer and CI output
- `dist/` is final release packaging
- dependency groups unlock capabilities instead of forcing full syncs
- release packages carry manifests, checksums, versions, and clear rebuild requirements

That gives Sparkle a professional release foundation while keeping day-to-day programming flexible. It also puts the launcher in the right role: a workflow surface that explains what the user can do now, what dependency groups would unlock more, and what exact package is ready to ship.
