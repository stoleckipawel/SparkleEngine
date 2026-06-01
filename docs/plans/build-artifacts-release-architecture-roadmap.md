# Sparkle Build And Release Architecture Roadmap

Date: 2026-06-01

## Purpose

This document defines the required changes to move Sparkle from the current mixed-use `build/` layout to a clearer, release-ready output architecture.

The goal is not cosmetic cleanup. The goal is to make the repository:

- obvious to navigate for developers
- safe for day-to-day local iteration
- predictable for CI
- easy to package for release
- aligned with the expectations users bring from mature desktop tooling, engine toolchains, and graphics tooling ecosystems such as NVIDIA Nsight and AMD developer tools

This document focuses on output architecture, staging boundaries, and migration order. It does not try to redesign build logic, launcher workflows, or asset pipelines beyond what is required to place outputs in the right locations.

## Executive Summary

Sparkle currently uses `build/` as:

- CMake generator state
- intermediate build workspace
- runtime output root
- tool output root
- cooked-content root
- launcher state root
- validation/screenshot root
- partial packaging root

That is the main architectural problem.

For release preparation, Sparkle should adopt three clearly separated roots:

1. `build/`
Purpose: disposable build-system workspace and intermediates only

2. `artifacts/`
Purpose: runnable outputs for local development, CI collection, testing, and staging

3. `dist/`
Purpose: final release-ready payloads only

This separation is the biggest improvement because it removes ambiguity between:

- things the compiler owns
- things developers run
- things users receive

## Current-State Diagnosis

### What Exists Today

The current repository contains a `build/` tree with mixed responsibilities, including:

- `build/bin`
- `build/lib`
- `build/Cooked`
- `build/Launcher`
- `build/Package`
- `build/Validation`
- `build/LauncherScreenshots`
- `build/_deps`
- CMake/MSBuild state such as `CMakeFiles`, `x64`, `*.dir`, `ZERO_CHECK.dir`

Example of the current runtime output problem:

- `build/bin/DevelopmentEditor` contains `SparkleLauncher.exe`
- the same folder also contains `ShowcaseEditor.exe`
- the same folder also contains `AssetCooker.exe`, `TextureCooker.exe`, `ShaderCompiler.exe`
- the same folder also contains Qt deployment payloads and other DLLs

This makes the output tree hard to reason about and hard to ship cleanly.

### Why This Is A Problem

The current structure creates the following issues:

- release boundaries are unclear
- app ownership is unclear
- internal tools and user-facing products are mixed together
- it is not obvious which folders are disposable
- launcher/runtime lookup code has to know too much about accidental layout details
- self-update, restart, and packaging behavior become fragile
- CI output publishing is harder than it should be
- new contributors cannot quickly tell what is source-owned versus build-owned versus release-owned

### The Biggest Issue

The single biggest issue is that `build/` is being used as both:

- an internal build workspace
- a product output/distribution workspace

That should be split first.

## Design Principles

The target structure should follow these principles.

### 1. Build Roots Must Be Disposable

Anything under `build/` should be safe to delete and recreate from source plus configuration.

If a folder under `build/` contains something users run, package, archive, or inspect as a release artifact, it is in the wrong place.

### 2. Product Outputs Must Be Grouped By Product, Not By Toolchain Accident

Users expect:

- launcher files together
- editor files together
- runtime files together
- tools together

They do not expect a generic `bin/<Config>` bucket that mixes all of them.

### 3. Release Payloads Must Be Separate From Developer Outputs

`artifacts/` is for:

- local running
- CI capture
- staging
- testing

`dist/` is for:

- zips
- installers
- signed deliverables
- versioned release folders

These should not be the same location.

### 4. Folder Names Must Explain Intent

Names like `artifacts/launcher`, `artifacts/tools`, `dist/SparkleEngine-<version>` are clearer than a generic `bin/`.

`bin` is acceptable as an implementation detail, but not ideal as the main external mental model during release prep.

### 5. Shared Runtime State Must Not Live Beside Shippable Binaries

Launcher state, logs, screenshots, validation output, and local caches should not live in the same product folder as release payloads.

## Desired Architecture

## Recommended Top-Level Model

```text
build/
  cmake/
  obj/
  cache/
  logs/

artifacts/
  dev/
    launcher/
    tools/
    projects/
      Showcase/
        editor/
        runtime/
        cooked/
  symbols/
    dev/
      launcher/
      tools/
      projects/
        Showcase/
          editor/
          runtime/

dist/
  SparkleEngine-<version>/
    Launcher/
    Tools/
    Projects/
      Showcase/
        Editor/
        Runtime/
        Cooked/
  zips/
  installers/
  manifests/
```

This structure gives Sparkle:

- a clean build root
- a stable developer output root
- a dedicated final release root

## Minimum Acceptable Variant

If a full move is too large for the next iteration, this is the minimum acceptable transition shape:

```text
build/
  ... only build-system and intermediate state ...

artifacts/
  DevelopmentEditor/
    Launcher/
    Tools/
    Showcase/
      Editor/
      Cooked/
  DevelopmentGame/
    Showcase/
      Runtime/

dist/
  SparkleEngine-<version>/
```

This still fixes the main ambiguity without requiring the final release packaging model on day one.

## Product-Level Desired State

## Launcher

Desired developer output:

```text
artifacts/dev/launcher/
  SparkleLauncher.exe
  Qt6Core.dll
  Qt6Gui.dll
  Qt6Widgets.dll
  platforms/
  styles/
  imageformats/
  ...
```

Rules:

- no editor executable here
- no runtime executable here
- no cook tools here
- no random CI screenshots here
- no local launcher state mixed into the deployed app directory

## Internal Tools

Desired developer output:

```text
artifacts/dev/tools/
  AssetCooker.exe
  TextureCooker.exe
  ShaderCompiler.exe
  dxcompiler.dll
  dxil.dll
  slang.dll
  ...
```

Rules:

- tools are grouped together
- shared tool runtime dependencies live here unless a tool needs its own subfolder
- user-facing app deployment should not need to expose these unless shipping requires it

## Project Outputs

Desired developer output:

```text
artifacts/dev/projects/Showcase/
  editor/
    ShowcaseEditor.exe
    ...
  runtime/
    ShowcaseRuntime.exe
    ...
  cooked/
    Shared/
      Meshes/
      Materials/
      Textures/
      Shaders/
```

Rules:

- group by project before role
- keep editor/runtime/cooked content together under the project
- make it obvious which cooked content belongs to which project or shared cook domain

## Symbols

Desired developer output:

```text
artifacts/symbols/dev/
  launcher/
  tools/
  projects/
    Showcase/
      editor/
      runtime/
```

Rules:

- symbols are not mixed into app directories
- CI and crash analysis have one obvious place to collect them

## Final Release Payload

Desired release output:

```text
dist/SparkleEngine-<version>/
  Launcher/
  Tools/
  Projects/
    Showcase/
      Editor/
      Runtime/
      Cooked/
```

Alternative if tools are not a public release surface:

```text
dist/SparkleEngine-<version>/
  Launcher/
  Projects/
    Showcase/
      Editor/
      Runtime/
      Cooked/
```

The release root should contain only things that are intended to ship.

## Required Actions

## Phase 1: Define Ownership Boundaries

Required actions:

- Declare `build/` non-shippable and disposable.
- Define `artifacts/` as the only developer/CI runnable-output root.
- Define `dist/` as the only release packaging root.
- Document that launcher state, screenshots, validation output, logs, and caches are not release artifacts.

Success criteria:

- everyone on the project can answer what belongs in `build/`, `artifacts/`, and `dist/`
- no new feature work adds user-facing outputs directly under `build/`

## Phase 2: Move Product Outputs Out Of `build/bin`

Required actions:

- Move launcher runtime output to a dedicated launcher folder under `artifacts/`
- Move tool executables to a dedicated tools folder under `artifacts/`
- Move editor/runtime executables into per-project product folders under `artifacts/`
- update all runtime path lookups that currently assume `build/bin/<Profile>`

Success criteria:

- `SparkleLauncher.exe` is no longer deployed beside editor and cook tools
- a user can open `artifacts/` and immediately understand where launcher, tools, and project outputs live

## Phase 3: Move Cooked Content Out Of `build/Cooked`

Required actions:

- move cooked output root under `artifacts/dev/projects/<Project>/cooked` or equivalent
- explicitly define whether shader/texture/mesh outputs are project-local, shared, or hybrid
- update launcher readiness checks, maintenance operations, and clean scopes to use the new cooked root
- keep any shared cooked domain obvious in the path, e.g. `cooked/Shared/Shaders`

Success criteria:

- launch readiness no longer depends on incidental historical layout
- cooked content is discoverable without opening the build workspace

## Phase 4: Move Local State And Diagnostics Out Of Product Output Folders

Required actions:

- move launcher state from `build/Launcher` to either:
  - `build/state/launcher`, if you want it disposable with build state
  - `artifacts/state/launcher`, if you want it preserved across build wipes
- move screenshots, validation outputs, and polishing captures to a dedicated diagnostics root
- separate logs from release payloads

Recommended root:

```text
artifacts/diagnostics/
  launcher/
  validation/
  screenshots/
```

Success criteria:

- runtime product folders do not accumulate unrelated logs and screenshots
- validation output becomes easy to archive in CI

## Phase 5: Add Dedicated Release Assembly

Required actions:

- add a packaging/staging step that assembles final release payloads under `dist/`
- do not release directly from `artifacts/`
- add versioned release folder naming
- add manifest generation for packaged outputs
- make signing, zipping, and installer generation consume `dist/`, not raw build output

Success criteria:

- release jobs operate on `dist/` only
- release contents are deterministic and reviewable

## Phase 6: Make The Launcher Architecture-Aware

Required actions:

- update launcher binary discovery to use product folders, not generic config bins
- update self-rebuild/restart behavior to point at the launcher product folder
- update build/cook/run previews to display new artifact paths
- update clean actions to target the correct product-owned roots

Success criteria:

- launcher UI reflects the real architecture
- path displays reinforce the new mental model instead of leaking old paths

## Required CMake-Level Changes

These changes should happen after the ownership model is approved.

### 1. Stop Treating `build/bin` As The Universal Runtime Root

Current behavior sets runtime outputs under:

- `build/bin/<Config>`

Required change:

- assign product-specific runtime output directories per target group
- separate launcher, tools, editor, runtime, and package staging

### 2. Introduce Shared Path Variables

Add explicit CMake variables for:

- `SPARKLE_BUILD_STATE_ROOT`
- `SPARKLE_ARTIFACTS_ROOT`
- `SPARKLE_DIST_ROOT`
- `SPARKLE_LAUNCHER_OUTPUT_ROOT`
- `SPARKLE_TOOLS_OUTPUT_ROOT`
- `SPARKLE_PROJECT_OUTPUT_ROOT`
- `SPARKLE_COOKED_OUTPUT_ROOT`
- `SPARKLE_SYMBOL_OUTPUT_ROOT`

This removes hidden path policy from individual target files.

### 3. Group Targets By Deployment Role

Define deployment roles:

- launcher
- internal-tools
- editor
- runtime
- packaged-release

Then use those roles to drive output paths consistently.

### 4. Stop Per-Target Ad Hoc Deployment Rules

Qt deployment and other runtime copy rules should target the launcher output root explicitly, not whatever `TARGET_FILE_DIR` happens to be under an old shared bin layout.

## Day-To-Day Iteration Model

The architecture must not make local iteration painful.

For daily development:

- developers build into `artifacts/dev/...`
- products remain directly runnable
- cooked content remains reusable
- `build/` can be deleted without losing the intended runtime output model

Recommended workflow expectations:

- `build/` is recreated as needed
- `artifacts/dev/launcher` is the stable app location for local launcher use
- `artifacts/dev/projects/<Project>/editor` is the stable editor launch location
- `artifacts/dev/tools` is the stable tool invocation location

This supports both IDE iteration and launcher-driven iteration.

## Feature Release Model

For a feature release branch or candidate build:

- build into `artifacts/release-candidate/<version>/...` or a CI-specific equivalent
- assemble final payload into `dist/SparkleEngine-<version>/...`
- archive symbols separately
- run validation against staged `dist/` payloads where possible

This avoids the common mistake of shipping directly from a dev output tree.

## Navigation Rules

A good architecture should let a new contributor infer folder meaning quickly.

Required navigation rules:

- source lives under source roots such as `Engine/`, `Tools/`, `Projects/`
- disposable machine/build state lives under `build/`
- runnable outputs live under `artifacts/`
- ship-ready outputs live under `dist/`
- diagnostics and validation outputs are clearly named and not mixed into app folders

If a folder name cannot be explained in one sentence, it needs renaming or relocation.

## What To Avoid

Do not:

- keep launcher, editor, runtime, and tools in one generic runtime folder
- release directly from `build/`
- release directly from `artifacts/`
- keep local state beside release binaries
- let cooked-content lookup depend on legacy fallback behavior forever
- encode path policy separately in launcher code, CMake, scripts, and packaging jobs without one shared architecture document

## Immediate Next Steps

Recommended next actions in order:

1. Approve the three-root model: `build/`, `artifacts/`, `dist/`
2. Approve the product grouping model: launcher, tools, project editor/runtime/cooked
3. Add shared path variables in CMake
4. Move launcher output first
5. Move tools output second
6. Move editor/runtime outputs third
7. Move cooked outputs fourth
8. Update launcher path resolution and clean/readiness logic
9. Add `dist/` release assembly
10. Update CI to publish from `artifacts/` and package from `dist/`

## Final Recommendation

Sparkle should not use `build/` as its long-term public output architecture.

For release preparation, the correct model is:

- `build/` for disposable build state
- `artifacts/` for runnable developer and CI outputs
- `dist/` for final shipped payloads

That is the clearest, safest, and most scalable structure for both:

- daily software iteration
- feature release and final release preparation

It also gives the launcher a cleaner long-term foundation, because the launcher can become a product-aware workflow surface instead of a UI wrapped around a generic build dump.
