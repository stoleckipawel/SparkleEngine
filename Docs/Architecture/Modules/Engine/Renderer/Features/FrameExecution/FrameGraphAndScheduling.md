# Renderer Frame Graph and GPU Scheduling

Status: current feature dossier; source-backed, not native-validation, overlap, performance, or release evidence

Verified: 2026-09-06 against committed `master` revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Scope: `REN-FG-01` through `REN-FG-08` and the frame-graph portion of `REN-OWN-04`/`REN-OWN-06`; defines how Renderer feature declarations become resources, dependencies, barriers, queue submissions, and retirement

## Feature Contract

The frame graph is Renderer-owned scheduling policy over RHI mechanisms. A feature pass declares its kind, typed shader parameters, resource reads/writes, and queue preference. The graph compiler derives executable order, resource versions, queue placement, transient lifetimes and aliases, state transitions, cross-queue waits, recording chunks, and submission batches. RHI records/submits the resulting commands without choosing Renderer technique order.

This is infrastructure used by features, not a visual feature by itself. Its observable promise is that enabled feature products are produced with valid dependency/lifetime ordering and that replaced graph generations remain alive through their last GPU use.

## Declaration And Execution Lifecycle

| Phase | Current operation | Owned result |
| --- | --- | --- |
| Topology resolution | `FramePipeline` resolves extents, output target/format, lighting/GBuffer modes, ray frontends, provider key, shader generation, and relevant SBT-plan generation. | `RenderFrameGraphSettings` plus topology identity |
| Graph construction | `RenderFrameGraphFactory` calls `BuildRenderFrameGraph`; feature helpers create/import/reserve resources and add typed raster, compute, ray tracing, transfer, or external-provider passes. | Persistent `FrameGraph` declaration and `RenderFrameGraphResources` handles |
| Per-frame binding | Current TLAS, sky, GPU-scene buffers, frame/scene/view/display/shadow parameters, and resource-production callbacks are applied. | Current native resources and parameter instances bound to persistent graph handles |
| Setup and compile | Pass setup runs; texture histories and imported accesses synchronize; compiler builds resource versions, dependencies, queues, transient lifetimes/blocks, aliasing/state barriers, submission batches, and recording plan. | `FrameGraphPlan` for this execution |
| Materialize and record | Transients are materialized, initial barriers recorded, batches recorded serially or through bounded Tasks chunks, and waits derived from producer batch tokens. | One or more RHI command-recording batches |
| Submit and finish | Batches submit to graphics/compute/copy as assigned, final barriers record, texture histories commit, UI records, and device `SubmitFrame`/advance completes CPU submission. | Queue tokens and next history state |
| Retire | Rebuilt graphs and their frame slots retain the last submitted state for every queue and are destroyed only when all tokens complete. | Completion-safe reclamation |

Declaration order in [`BuildRenderFrameGraph`](../../../../../../../Engine/Renderer/Private/Frame/Graph/BuildRenderFrameGraph.cpp) expresses the intended pipeline, but the compiler's resource graph is the execution authority. A pass must not depend on incidental insertion order.

## Pass Kinds And Queues

| Pass kind | Current use | Scheduling boundary |
| --- | --- | --- |
| Raster | Raster GBuffer and graphics render state | Graphics queue |
| Compute | TLAS build declaration, most lighting/post/debug work, and compute ray-query frontends | Graphics or compute according to compiled preference/capability |
| RayTracing | Native ray pipeline `TraceRays` frontends | Queue/capability constrained by RHI contract |
| Transfer | Copy and product movement, including final encoded output to back buffer | Copy or graphics according to capability and dependency plan |
| ExternalProvider | DLSS/RR integration with explicitly tagged resources | Provider/native-interop capability and ordered dependencies |

Queue existence does not prove useful overlap. When any non-graphics queue is used, the executor first submits graphics initialization, then applies explicit producer-batch waits. `r.FrameGraph.ParallelRecording` controls recording concurrency, not dependency semantics or GPU execution order.

## Resource Ownership

| Resource class | Meaning | Lifetime rule |
| --- | --- | --- |
| Transient | Graph-created intermediate texture/buffer | Compiler derives first/last use and may assign an aliased physical block; alias barriers separate occupants. |
| Persistent imported | Renderer/RHI-owned object such as GPU-scene buffer, sky, TLAS, or back buffer | Bound each frame; graph tracks required and final state but does not own the external object's semantic lifetime. |
| History | Paired temporal resources owned by the graph generation | Prior/current roles commit only after graph execution; topology/scene/view invalidation resets validity. |
| Product root | Output explicitly retained for viewport/capture/presentation | Prevents dead-output removal and carries product identity to downstream consumers. |

## Rebuild And Invalidation

The graph rebuilds when output/render extent or output target/format changes, GBuffer/lighting mode changes, active ray execution plan changes, image-provider graph key changes, shader generation changes, or a graph using the scene SBT observes a new table-plan generation. Resize drains/rebuilds swapchain-coupled execution; other changes retire the old graph asynchronously.

Rebuild invalidates view/frame/provider history. The tradeoff is simple, inspectable immutable topology at the cost of rebuild/materialization churn. Current source still performs setup and compile work per executed frame; its CPU cost and the value of further caching are unmeasured.

## Failure, Diagnostics, And Limits

- Invalid handles, missing persistent bindings, contradictory resource use, or incomplete ray-scene bindings are contract errors, not recoverable visual fallbacks.
- Diagnostic scopes can emit frame/pass markers and optional GPU timestamp queries; detailed draw/dispatch markers are separately gated.
- Transient alias correctness, cross-queue synchronization, parallel recording equivalence, provider-pass interop, graph rebuild safety, and retained-generation boundedness remain unproved.
- The graph does not define shader semantics, PBR correctness, backend support, or feature release disposition; it only schedules declared work.

Primary evidence: `REN-E19` for rebuild/recording/retirement, `RHI-E03` for queues, and `RHI-E05` for aliasing. See the [frame narrative](../../RenderingASparkleFrame.md) for its place in the complete frame.

## Acceptance Criteria

- `AC-FGS-01` — every live read has one dominating producer/import, every write creates the intended version, and dead unexported work is either removed or explicitly retained by a product root.
- `AC-FGS-02` — compiled dependencies, state/UAV/alias barriers, and cross-queue waits make each consumer observe its intended producer on D3D12 and Vulkan without relying on declaration order.
- `AC-FGS-03` — transient placements overlap only for non-overlapping lifetimes; first use after aliasing receives the required initialization/alias transition and produces the serial non-aliased oracle.
- `AC-FGS-04` — serial and parallel recording emit semantically identical pass order, parameters, barriers, submissions, and output for the same graph and frame identity.
- `AC-FGS-05` — graphics, compute, and copy assignments preserve dependencies; async exposure agrees with graphics-queue exposure and is never reported as a performance benefit without timing evidence.
- `AC-FGS-06` — every topology-key change rebuilds the graph exactly when required, invalidates affected history, and prevents old provider/shader/SBT resources from binding to the new graph.
- `AC-FGS-07` — rebuilt or failed graph generations, frame slots, histories, transients, providers, pipelines, and shader tables remain alive through their last queue token and are then reclaimed within a declared bound.
- `AC-FGS-08` — invalid handles, missing imports, contradictory use, unsupported queue/pass combinations, and incomplete ray bindings fail before partial submission and identify the offending pass/resource.

## Controlled Failure Modes

| ID | Injection or cause | Required safe behavior | Affected criteria |
| --- | --- | --- | --- |
| `FM-FGS-01` | remove a producer/import or introduce a read-before-produce edge | setup/compile rejects the graph with pass and resource identity; no batch submits | `AC-FGS-01`, `AC-FGS-08` |
| `FM-FGS-02` | omit/conflict a state declaration or create a cross-queue dependency | compiler rejects ambiguity or emits the exact transition/wait; native validation remains clean | `AC-FGS-02`, `AC-FGS-05` |
| `FM-FGS-03` | force two overlapping transient lifetimes into one block | planner rejects overlap or separates placement; corrupted reuse is never accepted | `AC-FGS-03` |
| `FM-FGS-04` | recording task or external provider fails before submission | execution reports failure, commits no invalid history, and retains/reclaims all materialized resources safely | `AC-FGS-04`, `AC-FGS-07`, `AC-FGS-08` |
| `FM-FGS-05` | resize/provider/shader/SBT/topology change while the prior generation is in flight | new work uses only the new generation; prior work retires after all recorded queue tokens complete | `AC-FGS-06`, `AC-FGS-07` |

## Checks And Completion

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-FGS-01` | focused compiler fixtures for producer/version/dependency/queue/barrier/dead-root rules, including each invalid injection | `AC-FGS-01`, `AC-FGS-02`, `AC-FGS-05`, `AC-FGS-08`; `FM-FGS-01`, `FM-FGS-02` |
| `CHK-FGS-02` | transient lifetime fixtures with aliased and deliberately non-aliased executions; compare outputs and inspect placement/alias barriers | `AC-FGS-03`; `FM-FGS-03` |
| `CHK-FGS-03` | execute the same representative graph with parallel recording off/on and async exposure unavailable/available; compare plans, decoded products, diagnostics, and native validation | `AC-FGS-02`, `AC-FGS-04`, `AC-FGS-05`; `FM-FGS-02`, `FM-FGS-04` |
| `CHK-FGS-04` | churn extent, provider, shader, lighting/frontend, and SBT-plan generations while work is in flight; assert history reset, binding identity, bounded retained generations, and completion-safe reclamation | `AC-FGS-06`, `AC-FGS-07`; `FM-FGS-04`, `FM-FGS-05` |

This contract is **defined but unproved**. `REN-E19`, `RHI-E03`, and `RHI-E05` may close it only with revision-pinned plans/captures, both applicable backends, controlled negative cases, and a candidate-bound verdict. Source presence, a successful compile, or GPU overlap inferred from queue assignment is insufficient.

## Primary Source Routes

- [`FramePipelineGraph.cpp`](../../../../../../../Engine/Renderer/Private/Frame/FramePipelineGraph.cpp): topology key, rebuild, invalidation, retirement.
- [`BuildRenderFrameGraph.cpp`](../../../../../../../Engine/Renderer/Private/Frame/Graph/BuildRenderFrameGraph.cpp): feature declaration order.
- [`ExecuteRenderFrameGraph.cpp`](../../../../../../../Engine/Renderer/Private/Frame/Graph/ExecuteRenderFrameGraph.cpp): per-frame bindings, setup, compile, execution.
- [`FrameGraphCompiler.cpp`](../../../../../../../Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphCompiler.cpp): compiled plan construction.
- [`FrameGraphExecution.cpp`](../../../../../../../Engine/Renderer/Private/FrameGraph/Execution/FrameGraphExecution.cpp): transient materialization, barriers, execution, history commit.
- [`FrameGraphSubmissionExecutor.cpp`](../../../../../../../Engine/Renderer/Private/FrameGraph/Execution/FrameGraphSubmissionExecutor.cpp): queue batches and waits.
- [`FrameExecutionRetirementQueue.cpp`](../../../../../../../Engine/Renderer/Private/Frame/Retirement/FrameExecutionRetirementQueue.cpp): completion-safe graph/frame-slot retirement.
