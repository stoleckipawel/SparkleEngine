# Tasks Capability Inventory

Status: capability snapshot; current, but not release approval, stress evidence, or performance evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; `Engine/Tasks` public/private source, scheduling paths, consumers, and CMake membership inspected; evidence `S` only

Scope: task graphs, lane topology, parallel ranges, execution handles, cancellation, scopes, events, shutdown, failure propagation, and tracing

Owner: `Engine/Tasks` / `SparkleTasks`

Evidence and disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

## Scheduler Contract

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `TASK-001` | Three-lane execution | Implemented path | `FrameCritical`, `Background`, and `BlockingIo` have separate worker pools, injection queues, per-worker local queues, and within-lane work stealing. No priority levels exist inside a lane. | `S` |
| `TASK-002` | Deterministic serial reference path | Implemented path | All worker counts at zero execute the compiled graph on the submitting caller. A mixed configuration with no frame-critical workers but other lanes is rejected. | `S` |
| `TASK-003` | Compiled DAG | Implemented path | Builder supports tasks, prerequisites, continuations, `WhenAll`, and nested-completion children. Compile checks handles, self/duplicate edges, cycles, policies, capacities, generations, and lane rules before execution. | `S` |
| `TASK-004` | Lane dependency safety | Implemented path | Graph compilation rejects invalid cross-lane dependency/nesting patterns, preventing frame-critical work from waiting through unsupported lane relationships. Exact admissible edges are enforced in `TaskGraphCompilation`. | `S` |
| `TASK-005` | Parallel ranges | Implemented path | `ParallelFor` partitions an item range using grain size, serial threshold, and maximum partitions; default policy is 64/128/64. Consumers override this for world and renderer work. | `S` |
| `TASK-006` | Logical nested completion | Implemented path | A parent's logical completion waits for its nested tasks, and dependents release only after prerequisite/nested settlement. This supports forked work without worker-side recursive submission. | `S` |
| `TASK-007` | Failure propagation | Implemented path | Results are Succeeded/Failed/Cancelled with messages; exceptions are caught and converted to failure; normal descendants of failed/cancelled prerequisites do not run, while cleanup nodes still settle. | `S` |
| `TASK-008` | Cooperative cancellation | Implemented path | Every task context receives a stop token. Execution handles, scopes, and shutdown request cancellation; queued normal bodies are suppressed, already-running bodies must cooperate and finish. | `S` |
| `TASK-009` | Structured scopes | Implemented path | Application, World, Document, AssetGeneration, Frame, and ToolInvocation scope kinds form parent/child lifetime trees with cancel, timed join, and destructor cancel-and-join behavior. | `S` |
| `TASK-010` | Execution observation | Implemented path | Move-only handles expose generation, status, aggregate result, first failing task name, per-node result, settled count, and timed wait. Waiting from a worker owned by the same executor is rejected to avoid self-deadlock. | `S` |
| `TASK-011` | Blocking event | Implemented path | Generation-tagged `TaskEvent` can signal/reset and wait with cancellation, but waits are accepted only on the `BlockingIo` lane. Tokens reject stale identity/generation. | `S` |
| `TASK-012` | Drain/cancel shutdown | Implemented path | Drain stops new work after active executions settle; Cancel requests cancellation before settlement. Shutdown invoked from one of the executor's own workers is rejected. | `S` |
| `TASK-013` | Capacity policy | Implemented path | Defaults: 1,024 tasks, 4,096 edges, 64 active executions. Hard graph maxima: 65,535 tasks and 262,144 edges. Task names max 96 chars; result messages max 512 chars. Invalid/exceeded limits reject work. | `S` |
| `TASK-014` | Windows task tracing | Capability-gated | ETW/TraceLogging records graph dependencies and task begin/end with generation, lane, worker, outcome, and duration on Windows; `advapi32` is private. No portable trace provider is present. | `S` |

## Current Consumers

| Consumer | How it uses Tasks | Important bound |
| --- | --- | --- |
| Application | Owns executor and application scope; configures worker counts; passes both to world and renderer. | Runtime defaults and shutdown ordering must be measured, not inferred. |
| GameFramework | Scene-load graphs plus an 11-stage compiled world-system graph and bounded parallel ranges. | World structure is frozen while query systems run. |
| Renderer | Asset work and selected parallel CPU preparation; threaded renderer itself has a separate ownership thread. | Task lanes do not substitute for RHI/render-thread ownership. |
| Launcher | Long-running operation service and cancellation around tool/build/content workflows. | Child processes need cancellation/handoff evidence. |
| Texture cooker | Parallel batch cooking with an additional memory limiter. | Worker count alone does not bound decoded memory. |

## Vertical Execution Trace

Builder records nodes/edges -> `Compile` validates the complete graph -> `Submit` takes a settled host-boundary path or `Launch` attaches execution to a scope -> ready nodes enter their declared lane -> a worker pops local/injection/steal work -> task body runs with generation/lane/cancellation context -> result releases nested tasks and dependents -> execution aggregates the first failure and final status -> scope and shutdown join before owner destruction.

## Explicit Non-Capabilities And Risks

- No coroutines, fibers, GPU task scheduler, arbitrary task priorities, dynamic graph mutation after compile, cross-process queue, or distributed execution was found.
- `TaskEvent::Wait` is deliberately blocking-I/O-only; it is not a general wait primitive for frame-critical work.
- Cancellation is cooperative for running bodies; bounded cancellation latency depends on each task checking its token.
- Work stealing is lane-local. The inventory does not prove load balance, fairness, determinism of parallel completion order, or absence of starvation.
- No active repository test target currently proves graph cycles, cancellation races, repeated shutdown, or capacity edges; these belong in evidence closure.
