# Codex Stage Runbook

Status: implementation prompt protocol
Date: 2026-06-13

Use this file at the start and end of every implementation stage. It turns the architecture contracts into a repeatable Codex workflow.

## Prime Context

Open these first:

1. [stage-prompt-packets.md](stage-prompt-packets.md)
2. [../rhi-renderer-review-ready-implementation-plan.md](../rhi-renderer-review-ready-implementation-plan.md)
3. [../architecture-review-acceptance-rubric.md](../architecture-review-acceptance-rubric.md)
4. [../../architecture/after/system-design-index.md](../../architecture/after/system-design-index.md)
5. [../../architecture/after/repository-threading-readiness.md](../../architecture/after/repository-threading-readiness.md)
6. [../../architecture/after/repository-target-folder-architecture.md](../../architecture/after/repository-target-folder-architecture.md)

Then open the current stage's target docs from [Required Target Documents By Stage](../rhi-renderer-review-ready-implementation-plan.md#required-target-documents-by-stage).

## Session Prompt Template

Use this shape when starting work on a stage:

```text
Implement Stage <N>: <stage name>.

Required source context:
- Open the original Stage <N> section in docs/plans/rhi-renderer-review-ready-implementation-plan.md.
- Open the Stage <N> row in docs/plans/implementation/stage-prompt-packets.md.
- Open all Stage <N> target docs from Required Target Documents By Stage.
- Open any touched subsystem row in docs/architecture/after/system-design-index.md.

Implementation constraints:
- Preserve the canonical design; do not weaken boundaries or acceptance criteria.
- Complete only the current stage or one mandatory split checkpoint.
- If the checkpoint is too large, stop and split it into a new numbered stage before implementation continues.
- Keep existing excellent code, improve code with good foundations, and redesign/remove code that cannot support the target architecture.
- Every retained complexity must earn its right to exist.
- Every changed edge must name mutable owner, phase, handoff shape, isolation, ordering/synchronization expectation, diagnostics identity, and deterministic output/report behavior.

Required output:
- Code/docs/CMake changes scoped to the stage.
- Stage completion packet.
- Validation commands and results.
- Remaining risks and follow-up owner.
```

## Stage Completion Packet

Every stage implementation must end with this packet, even if the stage is docs-only:

| Field | Required answer |
| --- | --- |
| Stage/checkpoint | Stage number, title, and checkpoint if applicable. |
| Target docs opened | Required target docs plus touched subsystem rows. |
| Contract surfaces touched | RhiContracts, RenderContracts, ShaderContracts, AssetContracts, ToolContracts, ThreadingReadiness, folder architecture, guardrails, or reviewer evidence. |
| Refactor disposition | Keep/refine, improve/extract, or replace/redesign for every touched module/folder/target/schema/command. |
| Complexity right to exist | What stayed, why it stayed, smaller alternative considered, and validation value. |
| Folder architecture | Source folders, target folders, forbidden destinations, CMake target changes, cleanup/deletion path. |
| Data transfer | Producer, contract shape, id/generation/path, consumer, diagnostics, validation command. |
| Threading readiness | Mutable owner, phase, handoff, isolation, ordering/synchronization, deterministic output/report. |
| Acceptance proof | File/code/CMake/docs state proving the stage goal. |
| Validation | Commands run, results, artifacts/logs, commands not run and why. |
| Stage status update | New status and evidence note for [../after/repository-refactor-stage-map.md](../after/repository-refactor-stage-map.md). |

## Always Preserve

- Public layer direction: `Core -> Platform -> RHI -> Renderer -> GameFramework -> Editor/Application`.
- Runtime modules do not depend on tool internals.
- RHI does not own renderer pass policy.
- Renderer ordinary passes do not require RHI edits.
- GameFramework emits runtime/cooked data and render snapshots; it does not own source import, cooking algorithms, renderer pass data, or backend-native behavior.
- Tools produce DTOs, manifests, cooked artifacts, package data, job requests, reports, and diagnostics.
- LauncherCore owns workflow/process state; Qt GUI owns presentation.
- Future threading readiness is achieved by ownership and data shape, not broad locks.

## Stop And Split Rules

Stop implementation and split a checkpoint into a new numbered stage when any of these happen:

- More than one contract surface changes and one surface lacks independent validation.
- A compatibility path would survive beyond the stage without a named removal stage.
- A proposed fix needs a broad allowlist, service locator, global mutable registry, or private include.
- A future worker/render/tool job would need producer-private mutable state.
- A stage wants both a new architecture and full backend parity proof in the same unreviewable change.

## Validation Defaults

- Docs-only: run markdown local link scan and report no runtime validation.
- Boundary/folder/CMake policy: run `cmake -DSPARKLE_REPO_ROOT="$PWD" -P CMake/ArchitectureBoundaryCheck.cmake`; run configure if target wiring changes.
- Shader compiler change: build `ShaderCompiler`; run list/inspect/package validation when commands exist.
- Cook/import change: build affected cooker/tool; run focused sample cook/inspect when available.
- Launcher workflow change: build `SparkleLauncher` or the smallest probe target; inspect operation reports/history.
- RHI/Renderer runtime change: run targeted build/smoke for the affected backend(s), with D3D12/Vulkan parity evidence at milestone stages.

