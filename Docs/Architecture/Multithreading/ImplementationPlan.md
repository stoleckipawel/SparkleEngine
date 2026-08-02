# K. Multithreaded Engine Implementation Plan

Status: internal execution plan; not architecture authority or implementation proof

Last consolidated: 2026-08-02

Architecture: [J. Multithreaded Engine Architecture](MultithreadedEngineArchitecture.md)

Implementation contract: [L. Integration Style Guide](../../Engineering/Standards/IntegrationStyleGuide.md)

## Purpose and Boundary

This document owns the ordered work packages that move the repository toward J. It deliberately does not repeat J's design, Engineering Standards, the `PGE-*` matrix, or workload contracts.

A prompt ID is a stable planning reference, not a completion claim. Historical prompt wording and completion narratives remain available in Git history. Current code, tests, captures, and measurements determine actual state.

Before starting or resuming a prompt:

1. inspect the current ownership path and replacement history;
2. classify each old criterion as enduring, transitional, or superseded;
3. preserve the underlying correctness/ownership/evidence invariant without restoring obsolete intermediate architecture;
4. apply the current [Change Process](../../Engineering/Standards/ChangeProcess.md), applicable domain standards, [PGE requirements](../../Strategy/Requirements.md), and [acceptance workloads](../../Engineering/BistroAndSanMiguelWorkloads.md);
5. stop if prerequisites, scope, serial parity, ownership, deterministic output, or required backend behavior cannot be proven.

## Shared Execution Contract

Every work package:

- starts from repository search and an explicit use/extend/refactor/replace/add decision;
- names mutable/lifetime owners and cross-thread publication;
- preserves a final-contract serial/reference path before parallel execution;
- uses bounded memory, queues, tasks, and cancellation;
- deletes replaced paths and stale aliases in the same accepted change;
- preserves supported D3D12/Vulkan and rendering/tool behavior in scope;
- validates the narrow owner plus the complete touched path;
- reports exact evidence, limitations, unavailable checks, and `PASS` or `BLOCKED`.

The detailed rule owner is the [Engineering Standards index](../../Engineering/Standards/README.md). Do not copy its checklists into a prompt.

## Dependency Shape

```text
00
 |
01 -> 02 -> 03 -> 03R -> 04
                         |
05 -> 06 -> 07 -> 08 -> 09 -> 10 -> 11 -> 12 -> 12D
                                                  |
13 -> 14 -> 15 -> 16 -> 17 -> 18 -> 19 -> 20 -> 21 -> 22
                                                       |
23 -> 23A -> 24 -> 25 -> 26 -> 27 -> 28 -> 29
                                           |
30 -> 31 -> 32 -> 33 -> 34
```

Dependencies describe architectural readiness, not necessarily one commit per prompt. Combine adjacent packages only when ownership and evidence remain independently reviewable.

## Phase 0 — Baseline and Task Runtime

| ID | Outcome | Depends on | Minimum evidence |
| --- | --- | --- | --- |
| `00` | Capture current serial owners, frame order, thread roles, queues, waits, feature paths, baseline workload, and deletion/naming ledgers. | none | source-backed map, reproducible baseline, identified hazards, no implementation claim |
| `01` | Build the immutable serial task-graph contract and deterministic execution state. | `00` | topology validation, cycle/bounds rejection, repeated serial equality, failure settlement |
| `02` | Add one fixed worker executor without changing graph meaning. | `01` | serial/1/2/N equivalence, exclusive outputs, queue bounds, clean shutdown |
| `03` | Complete scopes, cancellation, failure, events, lanes, and owner-lifetime settlement. | `02` | cancel/fail/destroy stress, exactly-once settlement, no worker waits |
| `03R` | Reconcile task-runtime folders, files, public/private APIs, names, and orchestration readability. | `03` | boundary build, stale-name search, responsibility audit, no behavior regression |
| `04` | Prove the runtime in bounded real tool work and remove competing worker mechanisms. | `03R` | deterministic tool outputs, weighted memory bounds, cancellation, old pool/thread deletion |

## Phase 1 — World, ECS, Loading, and Publication

| ID | Outcome | Depends on | Minimum evidence |
| --- | --- | --- | --- |
| `05` | Establish generational `EntityId` and private sparse-set component storage with serial behavior. | `04` | stale-ID rejection, packed-store invariants, deterministic create/destroy |
| `06` | Add typed queries, frozen structural epochs, access declarations, and deferred commands. | `05` | mutation rejection during views, stable command merge, hazard validation |
| `07` | Convert existing scene instance state into authoritative ECS components without a parallel world. | `06` | preservation ledger, old authority deletion, identical extracted behavior |
| `08` | Make transform/derived state explicit and publish a sequenced world change journal/read view. | `07` | known transform results, previous-frame semantics, stale/full-resync tests |
| `09` | Add transactional asynchronous scene loading through owned immutable packages. | `08` | cancel/fail/close tests, previous-state preservation, no partial publication |
| `10` | Build the ECS-aware system DAG and first measured parallel animation/morph/skinning work. | `09` | declared hazards, serial threshold, deterministic output, grain/crossover evidence |
| `11` | Convert Editor panels to immutable models, stable IDs, semantic commands, and bounded transactions. | `10` | no live world pointers, stale command rejection, close/in-flight safety |
| `12` | Establish immutable structural and dynamic GameFramework-to-Renderer streams. | `11` | renderer runs without `GameWorld`, generation/order checks, feature preservation |
| `12D` | Prove the two streams' access-driven layout, memory bounds, identity, and deterministic transform. | `12` | data/access inventory, layout alternatives, bytes/allocations/high-water, falsifier |

## Phase 2 — Render Ownership and Persistent State

| ID | Outcome | Depends on | Minimum evidence |
| --- | --- | --- | --- |
| `13` | Establish `RenderCoordinator` ownership and a bounded `RenderFrameQueue`. | `12D` | backpressure policy, producer/consumer stress, ordered shutdown, latency baseline |
| `14` | Move UI draw data, viewports, settings, and capture through owned render packets/commands. | `13` | no live ImGui/editor pointers, bounded capture/readback, late-result rejection |
| `15` | Build persistent render-object and GPU-scene slots driven by structural/dynamic deltas. | `14` | stable identity, dirty-range updates, no routine full rebuild/upload |
| `16` | Complete residency, capacity growth, reload, generation rejection, and token retirement. | `15` | delayed-GPU tests, memory high-water, replacement/retirement correctness |
| `17` | Decompose renderer preparation into a dependency DAG with exclusive outputs and deterministic join. | `16` | serial/parallel equality, measured grain, critical-path trace, preserved frame graph |

## Phase 3 — Backend Recording and Feature Closure

| ID | Outcome | Depends on | Minimum evidence |
| --- | --- | --- | --- |
| `18` | Add exclusive D3D12 allocator/list recording leases and bounded transient storage. | `17` | D3D12 validation, wrong-owner rejection, delayed retirement, recording crossover |
| `19` | Add equivalent Vulkan pool/buffer recording leases and synchronization ownership. | `18` | Vulkan validation/synchronization, parity with D3D12 contract, retirement stress |
| `20` | Compile eligible frame-graph recording groups and record them concurrently; keep ordered single-owner submission. | `19` | compiled-order equality, barrier/queue correctness, tiny/heavy crossover, both backends |
| `21` | Reconcile raster, classic TLAS/PTLAS, reservoir/path modes, temporal/providers, shader packaging, capture, and fallback. | `20` | explicit feature matrix, paired backend validation, no hidden compatibility architecture |
| `22` | Close editor/tools/package reliability and delete transition diagnostics, pools, adapters, and aliases. | `21` | clean package/run/shutdown, transactional tool outputs, zero intended stale references |

## Phase 4 — Performance, Reliability, and Forensics

| ID | Outcome | Depends on | Minimum evidence |
| --- | --- | --- | --- |
| `23` | Characterize the full system before broad tuning. | `22` | exact config, CPU/GPU timelines, p50/p95/p99, memory high-water, tiny/Tier 1 workloads |
| `23A` | Audit whether each multithreaded stage beats its speed-of-light and complexity limits. | `23` | serial controls, overhead model, retained/removed parallel paths, negative results |
| `24` | Close atomic publication, wait predicates, reclamation, cancellation, and shutdown protocols. | `23A` | state-machine documentation, race stress, spurious/lost wakeup tests, sanitizers where available |
| `25` | Establish conservative worker, lane, third-party, SMT/NUMA, and oversubscription policy. | `24` | topology/config sweep, queue delay, tail latency, product default rationale |
| `26` | Close deterministic fan-in and evidence gates for all retained parallel algorithms. | `25` | randomized completion, stable bytes/state/images/order, injected-defect detection |
| `27` | Bound staged I/O, compiler/process work, cold PSO/resource creation, and publication hitches. | `26` | cold/warm traces, weighted memory/backpressure, transactional failure behavior |
| `28` | Prove or reject GPU queue concurrency and frame-pipeline overlap with correlated pacing/latency evidence. | `27` | queue timelines, synchronization/bandwidth costs, input-to-present result, rollback path |
| `29` | Complete production forensics and an expert-defensible architecture result. | `28` | reduced incidents/reproducers, exact claims/limits, reviewer-ready code/capture trail |

## Phase 5 — Principal Path Tracing and Neural Graphics

| ID | Outcome | Depends on | Minimum evidence |
| --- | --- | --- | --- |
| `30` | Freeze the path-tracing math/reference, workload, hardware/driver, and partner-shaped baseline. | `29` | known-value math tests, Bistro/San Miguel references, paired APIs, issue/reproducer template |
| `31` | Select one product neural feature and establish dataset/model/operator/artifact/fallback contracts. | `30` | provenance, splits, baseline, deterministic export/cook, shapes/layout/precision, package boundary |
| `32` | Integrate renderer-owned runtime inference through frame graph/RHI with a real classical fallback. | `31` | offline/runtime conformance, capability failure, D3D12/Vulkan behavior, clean runtime package |
| `33` | Tune model, kernels, memory, system scheduling, and driver interaction on the quality-performance-memory frontier. | `32` | ablations, per-stage profiles, counters/disassembly where causal, pacing and failure cases |
| `34` | Complete adoption handoff and reconcile all applicable principal evidence. | `33` | reproducible integration, fallback/debug/tuning guide, peer review, demo/note, honest claims |

## Changelist Design Gate

Before Prompts `13`–`34`, identify the exact final files and owners for frame transfer, renderer state, GPU state, frame-graph integration, backend recording, editor/tool products, model artifacts, and feature policy. Reject parallel compatibility trees and numbered/catch-all files. The final path must be navigable by module → subsystem → capability.

## Completion and Resumption

A work package is complete only when its product outcome exists in current code, applicable preservation/deletion ledgers reconcile, required validation passes, and the [Change Process](../../Engineering/Standards/ChangeProcess.md#completion-report) reports `PASS`.

When current architecture has superseded an intermediate prompt mechanism, retain the prompt ID as traceability, update the outcome/evidence row if necessary, and prove the enduring invariant through the current owner. Do not restore obsolete code to satisfy historical wording.

## Final Plan Gate

The program is complete when:

- J's owner, publication, task, render, RHI, editor/tool, failure, and shutdown contracts are implemented and evidenced;
- each retained parallel path has a useful measured crossover and bounded memory cost;
- both supported backends and declared feature paths pass their relevant validation;
- current workloads satisfy their applicable quality/performance gates without scene-specific architecture;
- one path-traced and one real neural vertical slice meet the canonical `PGE-*` evidence bar;
- replaced architecture, temporary diagnostics, stale prompts, and compatibility aliases are removed or explicitly archived;
- another engineer can reproduce, debug, and adopt the result from the current documentation and code.
