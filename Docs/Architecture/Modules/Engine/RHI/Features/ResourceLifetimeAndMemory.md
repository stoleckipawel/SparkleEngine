# RHI Resource Lifetime and Memory

Status: current feature dossier; source-backed, not resource-correctness, pressure, or performance evidence

Verified: 2026-09-06 at committed `master` revision `8414b5dc`

Scope: `RHI-RES-*` and `RHI-FMT-*`; resource/view descriptions, formats, samplers, upload/readback, allocation, transient aliasing, memory diagnostics, recording use, and reclamation

## Feature Promise

A valid neutral resource description becomes backend storage with explicit format, usage, state, allocation, and view identity. CPU ownership changes never reclaim storage while a recorded or submitted queue consumer can still reference it.

## Cohesive Lifetime Contract

- Textures, buffers, views, samplers, uploads, readbacks, transient blocks, and aliased placements share one allocation/use/retirement invariant; this is why they form one dossier.
- Neutral format traits and use validation precede native creation. The public texture contract currently covers 2D/cube shapes, not a general 3D texture product.
- D3D12MA and VMA own backend allocation mechanics; RHI handles and memory diagnostics preserve neutral identity and categories.
- Upload/readback staging and recording-resource tables extend lifetime through recording and submission. Completion tokens, not C++ scope exit, authorize reuse/destruction.
- Transient aliasing requires non-overlapping compiled lifetimes plus explicit alias/state barriers supplied through the command contract.

## Acceptance Criteria

- `AC-RHI-RES-01` — every supported format/use combination creates, uploads, transitions, reads, and decodes with the documented byte/channel semantics on both backends; unsupported combinations reject before native work.
- `AC-RHI-RES-02` — buffers, textures, views, and samplers preserve complete neutral identity and do not outlive or alias their backing allocation illegally.
- `AC-RHI-RES-03` — upload/readback row pitch, extent, subresource, state, and completion behavior remain correct for boundary sizes and in-flight frames.
- `AC-RHI-RES-04` — transient placements alias only when lifetimes do not overlap and execute the required alias/state barriers.
- `AC-RHI-RES-05` — allocation budgets, categories, pressure, failure, and reclamation are observable and bounded; failed allocation never publishes a valid-looking resource.

## Controlled Failures And Checks

| Failure | Safe result | Check |
| --- | --- | --- |
| `FM-RHI-RES-01` invalid format/use/size/sample/view request | rejected by neutral validation before native allocation | `CHK-RHI-RES-01` descriptor and format/use matrix |
| `FM-RHI-RES-02` allocator exhaustion or pressure | explicit failure/pressure result; no stale or null-backed valid handle | `CHK-RHI-RES-02` bounded allocation pressure exercise |
| `FM-RHI-RES-03` destroy/reuse while recording or GPU work is in flight | lifetime owner retains allocation until all consumers complete | `CHK-RHI-RES-03` multi-queue retirement stress |
| `FM-RHI-RES-04` overlapping alias or missing barrier | compile/validation rejects before execution | `CHK-RHI-RES-04` aliasing positive/negative capture |

Check coverage: `CHK-RHI-RES-01` covers `AC-RHI-RES-01`, `AC-RHI-RES-02`, and `FM-RHI-RES-01`; `CHK-RHI-RES-02` covers `AC-RHI-RES-05` and `FM-RHI-RES-02`; `CHK-RHI-RES-03` covers `AC-RHI-RES-02`, `AC-RHI-RES-03`, `AC-RHI-RES-05`, and `FM-RHI-RES-03`; `CHK-RHI-RES-04` covers `AC-RHI-RES-04` and `FM-RHI-RES-04`.

Definition of done: the format/use matrix, allocator pressure, upload/readback, aliasing, multi-queue lifetime, native validation, memory, and performance evidence pass on both backends.

## Primary Source Routes

- `Engine/RHI/Public/Formats`, `Resources`, `Textures`, `Samplers`, and `Memory`
- matching common, D3D12, and Vulkan `Resources`, `Memory`, and `Samplers` implementations
- [Command Submission and Synchronization](CommandSubmissionAndSynchronization.md) for execution and completion authority
