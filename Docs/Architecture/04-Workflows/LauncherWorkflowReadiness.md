# Launcher Workflow Readiness

## Purpose

This document defines how SparkleLauncher currently exposes workspace readiness, dependency state, tool resolution, generator selection, build/cook entrypoints, and backend/provider availability. The goal is to make the launcher a reliable review path for generate, build, cook, clean, validation, and recovery workflows without requiring a reviewer to reverse-engineer UI behavior from source.

## Non-goals

- This document does not add or rename launcher workflows.
- This document does not change dependency policy or generator behavior.
- This document does not redefine renderer architecture through launcher UX.
- This document does not claim backend readiness beyond what launcher source currently detects.
- If a desirable workflow is not yet implemented, it is marked as planned rather than presented as current behavior.

## Launcher Responsibilities

SparkleLauncher is responsible for workflow orchestration, not runtime rendering architecture.

Current responsibilities include:

- host toolchain audit
- source dependency sync
- generated build file freshness and reconfigure flow
- local build orchestration
- local cook orchestration
- clean operations for generated outputs
- project launch and smoke-test entrypoints
- recovery prompts when a prerequisite workflow should run first
- reviewer-facing status messaging for missing tools, missing source caches, and selected backend readiness

The launcher is not the owner of:

- RHI backend implementation
- renderer frame graph policy
- shader ABI rules
- provider-specific rendering behavior

## Tool Resolution Responsibilities

Tool resolution is owned by [ToolResolver.h](/C:/Users/stole/Documents/GitHub/SparkleEngine/Tools/Launcher/SparkleLauncher/Public/SparkleLauncher/ToolResolver.h:1) and [ToolResolver.cpp](/C:/Users/stole/Documents/GitHub/SparkleEngine/Tools/Launcher/SparkleLauncher/Private/Core/ToolResolver.cpp:1).

Known tools resolved today:

- `CMake`
- `MSBuild`
- `Ninja`
- `Rider`
- `Git`
- `ClangFormat`

Resolution policy visible in source:

- environment overrides are checked first, for example:
  - `SPARKLE_CMAKE_EXE`
  - `SPARKLE_MSBUILD_EXE`
  - `SPARKLE_NINJA_EXE`
  - `SPARKLE_RIDER_EXE`
  - `SPARKLE_GIT_EXE`
  - `SPARKLE_CLANG_FORMAT_EXE`
- common install roots are searched next
- JetBrains Rider discovery includes local app installs and Toolbox script paths
- Visual Studio and MSBuild discovery prefer newer install roots first
- `ResolveSparkleToolPath(...)` is used for repo-built tools

The launcher uses tool resolution to build a `BuildToolchainStatus`, not to make rendering-policy decisions.

## CMake Generator / Backend Relationship

The launcher separates IDE preference, CMake generator selection, and graphics backend selection.

Current source-backed behavior:

- `WorkspaceIde` is a launcher-facing preference with values:
  - `VisualStudio`
  - `Rider`
- CMake generator selection is part of `BuildToolchainStatus.Generator`
- the preferred IDE does not replace the need for a working build generator/toolchain
- Visual Studio generator selection still requires Visual Studio C++ tools and Windows SDK readiness
- Rider can be the selected IDE while CMake still uses a Visual Studio generator underneath
- generator overrides are possible through `SPARKLE_CMAKE_GENERATOR`
- the launcher resolves newer Visual Studio installs first and returns `Visual Studio 18 2026` when present, otherwise `Visual Studio 17 2022`

Graphics backend selection is a different concern:

- launch and smoke workflows can request a backend such as `d3d12` or `vulkan`
- backend availability depends on toolchain and dependency readiness
- Vulkan readiness is surfaced when backend/build/launch workflows need it
- backend selection must not be conflated with IDE selection

## Workflow Action Categories

The launcher workflow catalog currently groups actions as follows.

### Sync

Primary action ids:

- `toolchain.check`
- `workspace.sync-source-tiers`

Role:

- audit host-installed prerequisites
- inspect enabled dependency groups
- sync repository source dependency caches
- repair incomplete enabled caches
- prepare later generate/build/cook workflows

Notes:

- `toolchain.check` audits machine readiness without mutating source caches
- `workspace.sync-source-tiers` is the main repository dependency preparation workflow

### Generate

Primary action ids:

- `workspace.generate-build-files`
- `workspace.open-ide`

Role:

- refresh generated CMake and IDE files
- verify freshness against generator, platform, toolset, Vulkan SDK root, and enabled feature set
- open the selected IDE once generated files are current

Notes:

- generate is grouped under `Build` in the current workflow catalog even though it is a separate readiness concern
- build-file freshness tracks generator mismatch, feature mismatch, missing cache, missing solution, and stamp mismatch

### Build

Primary action ids:

- `workspace.build-all`
- `launcher.build.self`
- `project.build.editor`
- `project.build.runtime`
- `cook.tools.prepare`

Role:

- rebuild the launcher
- rebuild project editor/runtime targets
- rebuild cook tooling outputs
- provide a full local rebuild path through `workspace.build-all`

Notes:

- `workspace.build-all` is the reviewer-facing full local rebuild action
- build workflows depend on host tools, current build files, and relevant dependency groups

### Cook

Primary action ids:

- `cook.shaders`
- `cook.textures`
- `cook.assets`
- `cook.project`

Role:

- run shader, texture, asset, or combined project cook workflows

Notes:

- `cook.project` is the combined cook-all action
- cook workflows may require `cook.tools.prepare` first
- shader and texture cook coverage depends on enabled workspace features and dependency bundles

### Clean

Primary action ids:

- `workspace.clean`
- `quality.format`

Role:

- clean generated workspace/build/cook state through `workspace.clean`
- run formatting validation or apply formatting through `quality.format`

Notes:

- in the current launcher catalog, `quality.format` is intentionally grouped under `Clean`
- format check is therefore a maintenance/clean-adjacent action, not a build action

### Validation

There is no dedicated top-level `Validation` workflow category in the launcher catalog yet.

Current validation-relevant workflows are spread across:

- `toolchain.check`
- `workspace.generate-build-files`
- `workspace.build-all`
- `cook.project`
- `quality.format`
- `project.run` with smoke options

Planned direction:

- a more explicit reviewer-facing validation grouping would be useful, but it is not implemented in current source

## Dependency Policy

The launcher currently models dependencies through source dependency groups plus toolchain/tool status. The most useful reviewer-facing categories are:

### Required

Meaning:

- needed for the current workflow to proceed successfully

Examples:

- `core-workspace` dependency group for local source workflows
- `CMake`
- `Git`
- `Qt 6 MSVC kit`
- `MSBuild` when the selected generator is a Visual Studio family generator
- shader compiler runtime bundle when shader compiler support is enabled

### Optional

Meaning:

- available for feature/workflow expansion but not required for every checkout

Examples:

- `content-pipeline`
- `ktx-support`
- `shader-compiler` as a workspace feature group
- `ClangFormat` outside format operations
- Vulkan SDK when Vulkan-backed integrations are not enabled

### Hardware-gated

Meaning:

- enablement depends on detected machine hardware rather than only repo settings

Current source-backed case:

- `nvidia-streamline` is enabled when `GetLauncherWorkspaceFeatureSettings()` sees `HostGraphicsCapabilities.HasNvidiaAdapter`
- if AMD/Intel/non-NVIDIA hardware is detected, the group is skipped and the UI explains why

### Backend-gated

Meaning:

- workflow readiness changes when a selected graphics backend requires extra host prerequisites

Current source-backed case:

- Vulkan workflows depend on Vulkan SDK readiness
- the launch status page explicitly warns when `vulkan` is selected but `VulkanSdkRoot` is empty
- generate/build freshness also includes Vulkan SDK root in the tracked build-input set

### Provider-gated

Meaning:

- dependency group is tied to a specific provider integration rather than broad renderer permission

Current source-backed case:

- `nvidia-streamline` packages NVAPI and Streamline/DLSS runtime files
- this group is scoped to NVIDIA provider workflows and does not imply general renderer-native API permission

## NVIDIA Dependency Policy

Current source-backed behavior:

- dependency group id: `nvidia-streamline`
- label: `NVIDIA DLSS and NVAPI`
- purpose: optional NVIDIA SDK dependencies used by DLSS and D3D12 NVAPI integration
- packages include:
  - `nvidia-nvapi`
  - `nvidia-streamline`
- the group is enabled when a NVIDIA adapter is detected on the host
- the group is skipped when no NVIDIA adapter is detected

Policy interpretation:

- NVIDIA dependency sync is hardware-gated today
- this is appropriate for local reviewer/developer readiness because AMD- and Intel-only machines should not be forced through NVIDIA SDK setup
- once enabled, NVIDIA dependency readiness affects sync, generate, build-all, and project build workflows
- the launcher already treats NVIDIA SDK failures as recoverable sync/readiness problems with targeted recovery messaging

Important boundary note:

- this policy is about staging source/runtime dependencies
- it is not a statement that renderer-wide permissions should be broadened for NVIDIA-specific APIs

## Vulkan SDK Policy

Current source-backed behavior:

- Vulkan SDK is detected from:
  - `VULKAN_SDK`
  - `VK_SDK_PATH`
  - common install roots such as `C:\VulkanSDK`
- detection validates required files such as:
  - `Include/vulkan/vulkan.h`
  - `Lib/vulkan-1.lib`
- shader compiler support also reuses the Vulkan SDK root for DXC and Slang bundle detection

Policy interpretation:

- Vulkan SDK absence must be shown at backend/build readiness time
- when NVIDIA Streamline is enabled, Vulkan SDK becomes required
- when NVIDIA Streamline is not enabled, Vulkan SDK is reported as optional unless Vulkan-backed integrations are enabled
- launch workflows explicitly warn when the user selects Vulkan but the SDK is not ready
- build-file freshness records Vulkan SDK root changes so configure/generate can rerun when the SDK state changes

Reviewer takeaway:

- Vulkan SDK readiness is not just a launch concern
- it influences shader compiler readiness, configure inputs, and provider-backed renderer builds

## Reviewer Workflow Path

Recommended reviewer path through the launcher:

1. `toolchain.check`
   - confirm CMake, generator prerequisites, Rider/Visual Studio discovery, Qt kit, shader compiler SDK bundle, and Vulkan SDK state
2. `workspace.sync-source-tiers`
   - confirm enabled dependency groups and cache completeness
3. `workspace.generate-build-files`
   - confirm generator/platform/toolset/build-input freshness
4. `workspace.build-all`
   - run the full local rebuild path
5. `cook.project`
   - validate combined cook readiness
6. `quality.format`
   - optional maintenance validation
7. `project.run` with smoke settings
   - validate runtime/editor launch and selected backend readiness

This path makes the launcher a viable review surface for:

- dependency readiness
- generator/build readiness
- cook readiness
- backend readiness
- recovery messaging quality

## Failure Messaging Expectations

Current source behavior shows a strong expectation that failures should route the user to the next useful workflow rather than only report raw process failure.

Failure messages should identify:

- which prerequisite is missing
- whether the issue is host-installed, repo-cached, backend-specific, or provider-specific
- which workflow should be run next
- whether the failure is blocked, repairable, partial, or optional

Current source-backed examples:

- missing Vulkan SDK guidance tells the user to install/expose the SDK, then reopen Sync and retry
- missing Streamline guidance tells the user to rerun sync and, if necessary, clean the dependency cache
- missing NVAPI guidance tells the user to rerun sync against a clean source dependency cache
- missing build-file freshness routes users toward `workspace.generate-build-files`
- missing toolchain readiness routes users toward `toolchain.check`

Reviewer-facing expectation:

- failure text should explain why a workflow is blocked, not only that it failed

## New Workflow Action Checklist

1. Assign the workflow to an existing category:
   - sync
   - generate
   - build
   - cook
   - clean
   - validation-like existing flow
2. Define a stable action id and display name.
3. Decide whether the action is host-tool gated, source-dependency gated, backend-gated, provider-gated, or hardware-gated.
4. Add prerequisite routing:
   - `toolchain.check`
   - `workspace.sync-source-tiers`
   - `workspace.generate-build-files`
   - `cook.tools.prepare`
   as appropriate.
5. Ensure status pages can explain readiness in terms of:
   - ready
   - blocked
   - needs refresh
   - partial
   - optional/disabled
6. If the action depends on a backend selection, surface backend-specific readiness text before launch/build failure.
7. If the action depends on provider packages, keep the dependency scoped to that provider group.
8. If the action changes configure inputs, include it in build-file freshness reasoning.
9. Add recovery guidance that tells the user which workflow to run next.
10. Update reviewer docs and validation docs when the action becomes part of the recommended path.

## Known Gaps

- There is no dedicated top-level validation category in the workflow catalog yet.
- Generate is currently grouped under `Build`, even though it is a distinct readiness phase.
- The current dependency policy is source-backed and useful, but the category names in this document are still human-facing interpretation rather than first-class enums in source.
- Backend readiness is surfaced well for Vulkan selection, but there is not yet a single exported launcher readiness report artifact for reviewers.
- Hardware-gated NVIDIA sync is present in source, but the launcher still relies on the current host-adapter heuristic rather than a broader user policy override surface.
- The launcher is a strong workflow surface, but it does not yet replace deeper architecture contracts such as [RHIContract.md](../02-Contracts/RHIContract.md), [RendererProviderContract.md](../02-Contracts/RendererProviderContract.md), or [ValidationMatrix.md](../03-Validation/ValidationMatrix.md).
