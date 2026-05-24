# Sparkle Launcher Root Workflow Executive Summary

Date: 2026-05-24

## Decision Summary

Sparkle Launcher should become the first visible workflow surface for Sparkle Engine, but its source should stay inside the engine repository under `Tools/Launcher/SparkleLauncher`. For the first release, the release package should expose a root-level launcher entrypoint so a user who syncs or extracts Sparkle immediately understands what to run. The source tree should keep launcher implementation code in `Tools/Launcher/SparkleLauncher` and add only the smallest root bootstrap surface needed to build or start the launcher from a fresh checkout.

This keeps the launcher close to the engine contracts it orchestrates, avoids a separate-repository versioning problem, and makes the launcher responsible for engine workflows rather than for being a general-purpose external product.

## Repository Model

Sparkle Launcher should remain in the Sparkle Engine monorepo for the first release.

Reasons:
- Launcher workflows are tightly coupled to Sparkle target names, profiles, project markers, cook tools, shader package rules, and generated build layout.
- A separate launcher repository would need a version compatibility protocol before the engine has reached first release stability.
- The launcher is not an SDK-independent product yet; it is the engine's workflow console with a native GUI.
- First-release users benefit more from one synced repository and one root entrypoint than from a separate app lifecycle.

What should not happen for first release:
- Do not move launcher source to repository root.
- Do not duplicate workflow logic in root scripts.
- Do not make users manually navigate into `Tools/Launcher/SparkleLauncher`.
- Do not require the launcher to build itself while its executable is running.

## Root Entrypoint Strategy

There are two root-level experiences to support.

For release packages:
- Put the ready-to-run `SparkleLauncher.exe` at the package root.
- Put required Qt/runtime support files beside it or under a predictable `SparkleLauncher/` support directory, depending on deployment needs.
- The launcher should discover the repository root from its own location or current working directory using the existing repository locator behavior.
- The release package should open directly into launcher workflows, not into a console bootstrap.

For source checkouts:
- Provide a minimal root bootstrap entrypoint whose only job is to find an existing built launcher or build the `SparkleLauncher` target once.
- The bootstrap may be a tiny Windows script or a small native bootstrapper, but it must not own engine workflow logic.
- Once the GUI launcher exists, normal setup/build/cook/launch/maintenance work should happen inside the launcher.

Recommended first-release shape:
- `SparkleLauncher.exe` at release package root.
- `SparkleLauncher.bat` or equivalent minimal source-checkout bootstrap at repository root only if no checked-in binary is shipped with source.
- `Tools/Launcher/SparkleLauncher` remains the implementation home.
- `build/bin/<Profile>/SparkleLauncher.exe` remains the local build output.
- `build/Package/SparkleLauncher/<Profile>/SparkleLauncher.exe` remains a package validation staging path until the release package layout is finalized.

## Bootstrap Boundary

The launcher cannot be the only thing that builds the launcher from a clean machine. A clean checkout needs CMake, Visual Studio/MSBuild, Qt, and dependency fetch/configure before `SparkleLauncher.exe` exists. That creates a real bootstrap boundary.

The bootstrap lane should do only this:
- Locate CMake and the repository root.
- Configure the build tree if needed.
- Build `SparkleLauncher` in the default first-release profile, currently `DevelopmentEditor`.
- Start the built launcher.
- Print a concise failure if the toolchain is missing.

The bootstrap lane should not do this:
- Build projects.
- Cook assets.
- Run smoke tests.
- Clean the workspace.
- Duplicate launcher workflow options.
- Become the main user interface.

This keeps console interaction limited to the unavoidable first-build case. After the launcher is available, the GUI becomes the daily workflow surface.

## Launcher Responsibility

Sparkle Launcher owns workflow orchestration, not engine compilation logic itself.

The launcher should own:
- Discovering Sparkle projects under `Projects/` using `.sparkle-project` markers.
- Checking toolchain readiness.
- Generating Visual Studio/CMake project files.
- Building selected editor/runtime targets.
- Building cook tools.
- Cooking assets through native cook tools.
- Launching editor/runtime targets.
- Running graphics smoke validation.
- Formatting and explicit clean workflows.
- Showing readiness, activity, output, recovery guidance, and confirmations.

The launcher should not own:
- CMake target definitions.
- Engine module build rules.
- Shader compiler internals.
- Asset cooker internals.
- Project source structure.
- Dependency download policy beyond invoking configure/build workflows.

## Source Ownership

Current ownership should stay clear and enforceable.

`SparkleLauncherCore`:
- Owns workflow definitions, planning, readiness checks, process requests, and execution records.
- Exposes stable operation APIs from `Public/SparkleLauncher`.
- Uses `SparkleCore` privately for foundation utilities only.
- Must remain usable by probes/tests without the Qt GUI.

`SparkleLauncher` GUI:
- Owns native Qt presentation, workflow selection, inline options, activity drawer, icons, dialogs, and accessibility.
- Calls `LauncherBackend`, which adapts core operations into Qt signals.
- Must not duplicate planning rules that belong in `SparkleLauncherCore`.

`SparkleLauncherProbe`:
- Should remain the lightweight validation surface for core workflow planning and command generation.
- Should be expanded before adding fragile GUI-only validation for nonvisual behavior.

Root bootstrap:
- Owns only first-run discovery/build/start of the launcher.
- Must stay small enough that it can be replaced or deleted without changing engine workflow logic.

## Development Workflow Changes

Before launcher adoption, development habits naturally centered on direct CMake, Visual Studio, and console tools. After launcher adoption, daily work should shift to launcher-first workflows while preserving direct CMake as the fallback and contributor escape hatch.

Expected habit changes:
- New users start at the repository root and run the launcher entrypoint.
- Users generate solutions through the launcher instead of remembering configure commands.
- Users build editor/runtime targets through workflow options instead of manually selecting targets in Visual Studio for common cases.
- Users cook assets through launcher cook workflows instead of invoking cook tools directly.
- Users inspect failures in launcher activity output first, then drop to terminal only when deeper debugging is needed.
- Engine contributors still use direct CMake/MSBuild for narrow development and CI parity.

The launcher should become the happy path, not the only path.

## Iterating On The Launcher Itself

Launcher development has a bootstrap exception: the running launcher should not be responsible for rebuilding or replacing itself.

Recommended iteration loop:
1. Edit launcher code under `Tools/Launcher/SparkleLauncher`.
2. Validate nonvisual workflow changes through `SparkleLauncherCore` or `SparkleLauncherProbe` where practical.
3. Build the GUI target directly with CMake/MSBuild: `cmake --build build --target SparkleLauncher --config DevelopmentEditor`.
4. Close any running launcher before replacing packaged/staged executables.
5. Start the rebuilt launcher from `build/bin/DevelopmentEditor/SparkleLauncher.exe` or the package staging path.
6. Promote to the root release entrypoint only through the packaging/release workflow.

Why this rule matters:
- Windows file locks make replacing a running executable unreliable.
- Self-update behavior is separate product infrastructure and should not block first release.
- Direct build validation keeps launcher development fast and understandable.

For first release, do not build an in-app self-update or self-rebuild feature. Add a visible recovery instruction when the launcher detects it is out of date only after there is a real versioning mechanism.

## Workflow Sections For First Release

Setup:
- User outcome: repository is configured and ready for coding.
- Code owner: `Private/Build`.
- Entry workflows: `Setup Workspace`, `Generate Solution`, `Check Toolchain`.
- Release expectation: a fresh user can reach a generated solution without knowing CMake command lines.

Build:
- User outcome: selected project editor/runtime targets compile.
- Code owner: `Private/Build` plus CMake target definitions.
- Entry workflows: `Compile Editor`, `Compile Runtime`, `Build Cook Tools`.
- Release expectation: common target/profile choices are visible and understandable.

Cook:
- User outcome: project runtime assets are prepared.
- Code owner: `Private/Cook` plus cook tools under `Tools/Cooking` and shader compiler code.
- Entry workflows: `Cook All Assets`, `Cook Shaders`, `Cook Textures`, `Cook Scene Assets`.
- Release expectation: incremental cook is the default; clean cook requires explicit confirmation.

Launch:
- User outcome: project editor/runtime starts without manual path hunting.
- Code owner: `Private/Launch`.
- Entry workflows: `Run Editor`, `Run Runtime`, `Editor Smoke Test`, `Runtime Smoke Test`.
- Release expectation: missing executable guidance points users back to the matching build workflow.

Maintenance:
- User outcome: formatting and generated-output cleanup are explicit and recoverable.
- Code owner: `Private/Maintenance`.
- Entry workflows: `Format Code`, `Clean Workspace`.
- Release expectation: destructive scopes are visible, scoped, and confirmed at run time.

Launcher Development:
- User outcome: contributors can modify the launcher without mystery.
- Code owner: `Tools/Launcher/SparkleLauncher`.
- Entry workflow: direct CMake build of `SparkleLauncher`, plus probe/core validation.
- Release expectation: first release documents and preserves the bootstrap exception.

## First Release Readiness Bar

The launcher is release-ready when these are true:
- A release user can see and start the launcher from the root immediately.
- A source user has a minimal root bootstrap path to build and start the launcher.
- The launcher can generate solution files from a fresh configured checkout.
- The launcher can build editor and runtime targets for discovered projects.
- The launcher can build cook tools and run cook workflows.
- The launcher can launch editor/runtime workflows or clearly explain missing build prerequisites.
- Destructive operations require explicit scoped confirmation.
- Activity output is copyable and gives useful recovery guidance.
- Direct CMake/MSBuild remains available for launcher development and CI.
- Packaging validates the same root-level experience that users receive.

## Release Packaging Implications

The first release should produce a user-facing package layout, not just a build tree.

Target package behavior:
- Root contains `SparkleLauncher.exe`.
- Required runtime files are deployed with it.
- The package includes `Engine/`, `Tools/`, `Projects/`, `CMake/`, `CMakeLists.txt`, and `.sparkle` or another durable root marker.
- Launcher-generated output continues to live under `build/`.
- The launcher should never require users to open the build directory to find the app.

Packaging validation should include:
- Delete or ignore an old package staging directory.
- Build `SparkleLauncher`.
- Deploy/copy runtime dependencies.
- Start the package-root launcher.
- Confirm it finds the repository root.
- Confirm it discovers projects.
- Confirm `Generate Solution` previews/runs.
- Confirm at least one build workflow reaches a correct command plan.

## Process Rules Going Forward

When adding or changing a launcher workflow:
- Add the operation definition and planning logic in `SparkleLauncherCore` first.
- Add process request construction in the owning workflow folder.
- Add executor behavior only when the operation needs new runtime behavior.
- Add GUI options only after the core request model exists.
- Validate with probe/core checks before relying on GUI smoke.
- Keep root bootstrap unchanged unless the first-run build path itself changes.

When changing engine build/cook/launch behavior:
- Update the owning engine/tool code first.
- Update launcher planning only to reflect new targets, options, readiness, or recovery guidance.
- Do not make the launcher a workaround for missing CMake or tool ownership.

When preparing a release:
- Build and validate the launcher target directly.
- Package it into the release root.
- Smoke the package-root executable.
- Verify first-run workflows from the package, not only from the developer build tree.

## Open Implementation Decisions

These should be resolved before first release implementation work starts:
- Whether the source checkout root entrypoint is a minimal `.bat` handoff or a tiny native bootstrapper.
- Whether release packaging puts Qt support files beside `SparkleLauncher.exe` or under a support folder.
- Whether `.sparkle` becomes the primary repository marker for launcher root detection, instead of relying only on `CMakeLists.txt`, `Engine/`, `Tools/`, and `Projects/`.
- Whether package staging should move from `build/Package/SparkleLauncher/<Profile>/` to a final release-layout directory that mirrors the shipped zip root.
- Whether `SparkleLauncherProbe` needs formal CI coverage before first release.

## Recommended Next Implementation Passes

Pass 1: Root Entrypoint Contract
- Decide source checkout bootstrap shape.
- Add a root entrypoint that only builds/starts the launcher.
- Ensure it does not duplicate workflow logic.

Pass 2: Release Package Layout
- Define the package-root layout with `SparkleLauncher.exe` at the top level.
- Deploy Qt/runtime dependencies into that layout.
- Add a package-root smoke check.

Pass 3: Bootstrap-Aware Launcher Detection
- Teach launcher startup and repository location to prefer the explicit root marker if needed.
- Ensure running from root, build output, and package root all resolve the same repository.

Pass 4: Launcher Development Validation
- Strengthen `SparkleLauncherProbe` around operation definitions, readiness, and command plans.
- Keep GUI validation for visual and interaction behavior.

Pass 5: First Release Workflow Gate
- Validate setup, build, cook, launch, maintenance, and package-root startup as one release checklist.
- Keep direct CMake/MSBuild as the accepted path for rebuilding the launcher itself.