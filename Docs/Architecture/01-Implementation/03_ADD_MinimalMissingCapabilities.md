# 03. ADD - Minimal Missing Capabilities

Status: constrained implementation prompt
Date: 2026-07-04
Source set: `Docs/Architecture/00-Review`
Use this as: the only allowed add-list before cleanup is complete

## Purpose

This document lists the small additions that are allowed because they preserve real capabilities, unlock deletion, or make an existing product path usable.

The default rule is still deletion-first:

- do not add documentation as a substitute for code shape
- do not add logs
- do not add validation systems
- do not add diagnostic panels
- do not add wrappers
- do not add thick abstraction layers
- do not add future scaffolding
- do not add profiling systems before feature cleanup
- do not add code before searching for an existing owner/capability that can be reused, merged, or simplified
- do not add duplicate responsibility
- do not hardcode project, level, asset, content-pack, or sample names in compiled code
- do not add fallback chains that hide missing required data

An addition is allowed only when it satisfies at least one condition:

1. It lets us delete more code or depot weight.
2. It hardens a preserved capability.
3. It makes D3D12/Vulkan, shader ABI, cook, runtime, high-level user concepts, or content selection coherent.
4. It replaces a broader system with a smaller direct path.
5. It is a real renderer feature slice, not a placeholder.
6. It moves content-specific names into catalog, config, level, or project data instead of compiled code.

## Add Gate

Before adding anything, answer:

- [ ] Which existing code paths, helpers, tools, or ownership boundaries already solve part of this problem?
- [ ] Can this be done by deleting, moving, or merging existing code instead of adding new code?
- [ ] If similar responsibility already exists, which path becomes the single owner and which path is removed?
- [ ] What existing code or depot weight does this enable us to remove?
- [ ] Which persona pillar does this develop?
- [ ] Which preserved engine capability does it support?
- [ ] Is the addition smaller than the code it replaces?
- [ ] Is the batch net code-neutral or net code-negative; if not, where is the paired deletion recorded?
- [ ] Does compiled code avoid hardcoded project, level, asset, optional-pack, and sample names?
- [ ] Does required missing data fail clearly instead of walking a fallback chain?
- [ ] Does it avoid new wrappers, thick abstraction layers, diagnostics, logs, validation, and scaffolding?
- [ ] Does it preserve or improve D3D12/Vulkan parity?
- [ ] Does it preserve offline cooked shader packages with reflection data, or update them coherently?
- [ ] Does it preserve multi-level support?
- [ ] Does it preserve screenshot/BMP capture if touched?
- [ ] Does it preserve classic TLAS and PTLAS if touched?

If the answer is weak, do not add it.

## Allowed Addition 1: Level And Content Catalog Metadata

Persona pillar: product/content hygiene.

Why add:

- Enables deletion or externalization of heavy Showcase content.
- Preserves multiple levels while reducing depot pollution.

Minimal shape:

- project/level catalog with:
  - level id
  - display name
  - default-workflow inclusion
  - optional pack id
  - path to level/scene data
  - tags if already useful
  - optional URL/hash/version only if an external pack workflow exists

Implementation prompt:

1. Identify how Showcase levels are currently discovered.
2. Add the smallest catalog/resolver path needed by launcher/cooker/runtime.
3. Mark the curated default level set.
4. Mark heavyweight levels/assets as optional.
5. Delete or move heavy content only after the optional path is represented.

Do not add:

- asset database
- content browser rewrite
- package manager
- network downloader unless already needed
- large metadata schema

Acceptance:

- [ ] Multiple levels remain selectable.
- [ ] Default levels cook and run without optional packs.
- [ ] Optional missing packs fail gracefully.
- [ ] Heavy content can be removed from core repo.

## Allowed Addition 2: Narrow TLAS/PTLAS Selection Policy

Persona pillar: ray tracing depth.

Why add:

- Users must be able to select classic TLAS or PTLAS where supported.
- Selection should be explicit without spreading feature flags through the renderer.

Minimal shape:

- one enum or setting if one does not already exist
- one capability query path
- one renderer decision point
- backend-specific support remains backend-owned

Implementation prompt:

1. Find current TLAS/PTLAS strategy selection.
2. Remove duplicate or implicit selection paths.
3. Add only the missing user-facing selection needed for classic TLAS or PTLAS.
4. Keep D3D12 and Vulkan support checks explicit.
5. Delete future GPU-pack and CPU validation scaffolding around selection.

Do not add:

- new acceleration structure abstraction layer
- broad strategy factory
- debug UI panel
- telemetry around selection

Acceptance:

- [ ] Classic TLAS and PTLAS can be selected where supported.
- [ ] Unsupported backend path fails gracefully or falls back explicitly.
- [ ] PTLAS code is smaller after the change.

## Allowed Addition 3: Hardened Screenshot Capture Entry Point

Persona pillar: debugging and tool fluency.

Why add:

- Screenshot/BMP capture is preserved.
- It may need one clear owner if current ownership is smoke/ad hoc.

Minimal shape:

- one editor/tool-owned capture command or service entry
- RHI readback/BMP writer remains the implementation detail
- capture path has no report artifact pipeline

Implementation prompt:

1. Find all `CaptureTextureToBmp` and BMP writer call sites.
2. Identify smoke/ad hoc owners.
3. Add or preserve one product-owned capture entrypoint.
4. Delete smoke-owned capture paths.
5. Keep backend readback behavior working.

Do not add:

- capture report system
- screenshot gallery
- validation screenshot comparisons
- broad public diagnostic capture API

Acceptance:

- [ ] Capture still writes BMP through the intended path.
- [ ] Smoke/ad hoc call sites are removed.
- [ ] Public surface is smaller or narrowly justified.

## Allowed Addition 4: Minimal Backend Capability Facts

Persona pillar: explicit graphics API ownership.

Why add:

- Some features need honest D3D12/Vulkan support checks.
- Capability facts are allowed when they drive behavior.

Minimal shape:

- capability bits or small structs consumed by runtime decisions
- no report schema
- no JSON dump by default
- no validation matrix system

Implementation prompt:

1. Identify the feature decision that needs a capability fact.
2. Add the smallest fact to the existing capability model.
3. Consume it immediately in renderer/RHI behavior.
4. Delete any duplicated or stale capability paths.

Allowed use cases:

- PTLAS support
- shader binary format support
- ray tracing support
- provider resource requirements
- screenshot/capture readback support if needed

Do not add:

- broad hardware database
- new diagnostics output
- capability report page

Acceptance:

- [ ] Capability fact drives product behavior.
- [ ] No new report artifact appears.
- [ ] Duplicated checks are removed.

## Allowed Addition 5: Renderer Feature Slice That Replaces Existing Complexity

Persona pillar: renderer feature depth.

Why add:

- Some features may require new shader/pass code.
- New feature code is allowed only when it replaces ambiguity or obsolete code.

Allowed examples:

- denoising pass that replaces a weaker or ambiguous post-process path
- post-processing cleanup that removes duplicate pass ownership
- upscaling/ray reconstruction integration hardening that deletes fallback scaffolding
- GI/path tracing improvement that shares material/light policy and removes divergence

Implementation prompt:

1. Name the old code path that will be deleted or simplified.
2. Implement the smallest vertical slice:
   - setting if needed
   - resources
   - pass
   - shader
   - frame graph integration
   - backend capability if needed
3. Delete the old path in the same batch when possible.
4. Keep public API unchanged unless a smaller product API replaces it.

Do not add:

- placeholder pass
- future provider shell
- debug panel
- benchmark format
- broad abstraction
- thick high-level layer that hides rendering behavior

Acceptance:

- [ ] Feature path renders or is buildable in product path.
- [ ] Old code is deleted or simplified.
- [ ] No new default diagnostics/logs/reports.
- [ ] Net code is negative or explicitly justified.

## Allowed Addition 6: Optional Package Manifest Only If Packaging Is Owned

Persona pillar: product engineering discipline.

Why add:

- Optional content packs may require a minimal manifest.
- Runtime/editor/dev/content package ownership may need a real output contract.

Minimal shape:

- package id
- package type
- file list or content root
- version/hash only if used
- no release note generator unless consumed

Implementation prompt:

1. Decide if runtime/editor/launcher/dev tools/symbols/optional content packages are real.
2. Add only the manifest fields consumed by build/package code.
3. Delete unused future manifest fields.
4. Keep optional content out of runtime package.

Do not add:

- release portal
- package UI unless launcher owns packaging
- unused checksums
- future platform package fields

Acceptance:

- [ ] One package command produces only owned outputs.
- [ ] Manifest fields are consumed.
- [ ] Package code or output size decreases.

## Late Additions Only

These are deferred until after feature cleanup:

- CPU/GPU profiling systems
- benchmark report formats
- workload CSV/JSON summaries
- new performance dashboards
- broad memory reports
- validation matrices

Late measurement is allowed only when:

- feature paths are stable
- existing debugger/profiler hooks are insufficient
- the new measurement replaces multiple old diagnostics
- the net code/API impact is non-positive or explicitly approved

## Forbidden Additions

Do not add:

- new docs unless replacing existing planning text at the user's request
- logging frameworks
- validation frameworks
- diagnostic panels
- report generators
- wrapper layers
- render graph replacement
- broad ML runtime dependencies
- training pipelines
- uncataloged sample content
- shader demos that do not serve renderer features
- future GPU-pack scaffolding
- package scaffolding for unowned packages

## Done State

The add plan is healthy when additions are rare and surgical:

- content catalog exists only if it unlocks depot reduction
- TLAS/PTLAS selection is clear and small
- screenshot capture has one hardened owner
- capability facts drive runtime behavior
- any new feature slice deletes old ambiguity
- profiling additions wait until the late stage
