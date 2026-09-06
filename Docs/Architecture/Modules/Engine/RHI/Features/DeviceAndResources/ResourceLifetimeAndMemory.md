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

## Object And Identity Model

| Layer | Current owner | Identity/lifetime rule |
| --- | --- | --- |
| neutral description | `RhiTextureResourceDesc`, `RhiBufferResourceDesc`, view and sampler descriptions | complete creation intent must be validated before native allocation; a description is not storage |
| resource handle/object | RHI texture/buffer/sampler interfaces and backend implementation | stable neutral identity owns or references the native resource/allocation; a valid-looking handle cannot wrap failed creation |
| view | SRV/UAV/RTV/DSV description plus backend descriptor/view object | view format/range/use must be compatible with the referenced resource and cannot extend its lifetime illegally |
| recording use | command recording resource/upload tables | keeps every referenced object and temporary allocation alive through command-list ownership |
| submitted use | queue submission/completion token | GPU completion, not local scope or command-list close, authorizes retirement/reuse |
| transient placement | frame-graph lifetime/alias plan lowered through RHI block and placed-resource contracts | two logical resources may share storage only with disjoint execution lifetimes and required alias/state barriers |

Generation belongs to the higher-level asset, graph, provider, or viewport product when semantic replacement matters. The RHI owns native object lifetime; it does not infer that two same-shaped resources represent the same scene or history generation.

## Resource Description Coverage

| Resource family | Current neutral description | Current explicit boundary |
| --- | --- | --- |
| texture | width, height, format, mip levels, array size, sample count, 2D/cube dimension, initial state, optional optimized clear, RT/DS/UAV uses | no public 3D texture kind found; format vocabulary does not prove every usage/sample combination |
| buffer | byte size, stride, generic/vertex/index/structured role, initial state, UAV and ray-tracing-input uses | size/stride/use/alignment boundaries need backend-correlated proof |
| views | texture/buffer SRV and UAV; texture RTV and DSV | subresource/range/format compatibility and descriptor lifetime require validation on both backends |
| sampler | point/linear min-mag-mip, wrap/clamp/mirror, anisotropy x1 through x16 | capability/quality at every combination is unproved; sampler state is separate from texture lifetime |
| samples | public sample-count vocabulary 1, 2, 4, and 8 where descriptor validation admits it | Renderer currently does not establish an end-to-end multisample attachment/resolve feature; see its resolution/sampling dossier |

## Transfer And Readback Trace

```text
CPU bytes + destination description/subresource
  -> validate extent, format, row/slice layout and bounds
  -> reserve upload storage
  -> record copy and required state transitions
  -> submit with resource/upload lifetime retention
  -> completion token releases staging ownership

GPU resource + requested readback region
  -> validate readable use/format/layout
  -> record transition and copy to staging
  -> submit and poll completion
  -> decode row pitch/channel semantics into the declared result
  -> release staging after result transfer or terminal failure
```

Synchronous-looking helper calls do not waive queue completion. Texture capture owns the product/result state machine; this dossier owns whether staging and referenced GPU storage remain valid long enough.

## Allocation, State, And Retirement Matrix

| Axis | Persistent | Transient | Upload/readback staging |
| --- | --- | --- | --- |
| allocation owner | D3D12MA/VMA-backed resource service | transient block/placed-resource materialization from Renderer graph plan | upload/readback service and recording/submission lifetime tables |
| state owner | caller/Renderer declares use; RHI executes explicit transitions | graph compiler derives versions/barriers; RHI validates/records | copy transitions surround transfer and restore/hand off declared state |
| aliasing | no implicit alias between independently allocated resources | allowed only for non-overlapping compiled lifetimes with alias barrier | ring/suballocation reuse only after the owning submission completes |
| pressure | budget/allocation diagnostics expose observed state; global degradation policy is not owned here | graph placement may reduce peak memory; savings are unproved | reservation failure must not publish a completed upload/readback |
| retirement | last queue consumer token | graph execution retirement token(s) | transfer completion plus result ownership |

## Capacity, Failure, And Non-Claims

- The 23-format vocabulary and per-adapter support query define candidates; they do not prove sampled/storage/RT/DS/copy/filtering support for every format.
- D3D12MA/VMA presence proves allocator integration, not graceful oversubscription, eviction, priority, defragmentation, or equal memory behavior.
- Renderer mesh/texture residency budgets are higher-level cache policy. RHI memory budgets and allocation observations do not choose which scene asset to evict.
- Transient alias capability does not prove the frame graph chose a legal or beneficial placement.
- Readback/capture support does not prove color space, channel interpretation, row pitch, or asynchronous failure UX.
- Public multisample vocabulary does not prove Renderer MSAA, an MSAA resolve, or anti-aliasing quality.

## Acceptance Criteria

- `AC-RHI-RES-01` — every supported format/use combination creates, uploads, transitions, reads, and decodes with the documented byte/channel semantics on both backends; unsupported combinations reject before native work.
- `AC-RHI-RES-02` — buffers, textures, views, and samplers preserve complete neutral identity and do not outlive or alias their backing allocation illegally.
- `AC-RHI-RES-03` — upload/readback row pitch, extent, subresource, state, and completion behavior remain correct for boundary sizes and in-flight frames.
- `AC-RHI-RES-04` — transient placements alias only when lifetimes do not overlap and execute the required alias/state barriers.
- `AC-RHI-RES-05` — allocation budgets, categories, pressure, failure, and reclamation are observable and bounded; failed allocation never publishes a valid-looking resource.
- `AC-RHI-RES-06` — every admitted texture/buffer/view/sampler description preserves the complete neutral identity through native creation; every incompatible dimension/format/subresource/usage/sample/alignment combination rejects before publication.
- `AC-RHI-RES-07` — upload and readback retain source, destination, staging, row/slice layout, and state through the exact completion token; cancellation/failure releases them without publishing stale success.
- `AC-RHI-RES-08` — persistent, transient, and staging allocations report distinguishable category, committed/used/budget, delayed-retirement, and failure facts without implying higher-level asset eviction policy.

## Controlled Failures And Checks

| Failure | Safe result | Check |
| --- | --- | --- |
| `FM-RHI-RES-01` invalid format/use/size/sample/view request | rejected by neutral validation before native allocation | `CHK-RHI-RES-01` descriptor and format/use matrix |
| `FM-RHI-RES-02` allocator exhaustion or pressure | explicit failure/pressure result; no stale or null-backed valid handle | `CHK-RHI-RES-02` bounded allocation pressure exercise |
| `FM-RHI-RES-03` destroy/reuse while recording or GPU work is in flight | lifetime owner retains allocation until all consumers complete | `CHK-RHI-RES-03` multi-queue retirement stress |
| `FM-RHI-RES-04` overlapping alias or missing barrier | compile/validation rejects before execution | `CHK-RHI-RES-04` aliasing positive/negative capture |
| `FM-RHI-RES-05` incompatible view/subresource or sample-count pairing | neutral/backend validation rejects; no descriptor or resource is published | `CHK-RHI-RES-01` descriptor/view matrix |
| `FM-RHI-RES-06` short/overflowing/misaligned transfer or wrong row pitch | transfer rejects before copy or returns explicit terminal failure; adjacent bytes remain unchanged | `CHK-RHI-RES-05` boundary transfer/readback oracle |
| `FM-RHI-RES-07` staging or placed allocation reused before all queue consumers complete | lifetime/token assertion prevents reuse and native validation remains clean | `CHK-RHI-RES-03`, `CHK-RHI-RES-05` |

| Check | Cheapest claim-falsifying exercise | Covers |
| --- | --- | --- |
| `CHK-RHI-RES-01` | generated format/dimension/use/view/subresource/sample/alignment matrix correlated with native capability queries and invalid cases | `AC-RHI-RES-01`, `AC-RHI-RES-02`, `AC-RHI-RES-06`; `FM-RHI-RES-01`, `FM-RHI-RES-05` |
| `CHK-RHI-RES-02` | bounded persistent/transient/staging allocation pressure with category/budget/failure observations | `AC-RHI-RES-05`, `AC-RHI-RES-08`; `FM-RHI-RES-02` |
| `CHK-RHI-RES-03` | multi-queue create/use/destroy/reuse stress with completion-token and live-object correlation | `AC-RHI-RES-02`, `AC-RHI-RES-03`, `AC-RHI-RES-05`, `AC-RHI-RES-08`; `FM-RHI-RES-03`, `FM-RHI-RES-07` |
| `CHK-RHI-RES-04` | graph with known legal and overlapping placements; inspect alias/state barriers and native validation | `AC-RHI-RES-04`; `FM-RHI-RES-04` |
| `CHK-RHI-RES-05` | patterned buffer/texture upload and readback across zero/one/odd/max rows, mips, arrays, pitches, partial regions, failure and cancellation | `AC-RHI-RES-03`, `AC-RHI-RES-07`; `FM-RHI-RES-06`, `FM-RHI-RES-07` |

Definition of done: the complete description/view/use matrix, allocator/category pressure, boundary upload/readback, aliasing, multi-queue lifetime, failure cleanup, native validation, memory, and performance evidence pass independently on both backends.

## Primary Source Routes

- `Engine/RHI/Public/Formats`, `Resources`, `Textures`, `Samplers`, and `Memory`
- matching common, D3D12, and Vulkan `Resources`, `Memory`, and `Samplers` implementations
- [Command Submission and Synchronization](../PipelineAndExecution/CommandSubmissionAndSynchronization.md) for execution and completion authority
