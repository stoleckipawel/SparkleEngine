# K. Multithreaded Engine Implementation Plan

Status: internal concurrency execution plan; not architecture authority or implementation proof

Last narrowed: 2026-08-11

Architecture: [J. Multithreaded Engine Architecture](MultithreadedEngineArchitecture.md)

Implementation contract: [L. Integration Style Guide](../../Engineering/Standards/IntegrationStyleGuide.md)

## Purpose and Boundary

This document sequences only the work needed to implement or prove J's multithreaded architecture: task execution, owner-thread commits, cross-thread publication, bounded queues, render coordination, recording leases, cancellation, shutdown, and concurrency evidence.

It does not schedule ECS or renderer feature development, content/loading capability, persistent GPU-data redesign, path tracing, neural graphics, portfolio work, or general product hardening. Those outcomes remain with the [domain standards](../../Engineering/Standards/README.md), [principal roadmap](../../Strategy/Roadmap.md), [requirements](../../Strategy/Requirements.md), and [acceptance workloads](../../Engineering/BistroAndSanMiguelWorkloads.md).

The `MT-*` IDs are stable references for this narrowed plan. Earlier numeric prompt IDs and completion narratives remain available in Git history; they are not current requirements or proof.

Before starting or resuming a package:

1. inspect the current owner, producer, consumer, wait, queue, captured lifetime, and replacement history;
2. determine whether the package is absent, partially implemented, implemented but unproved, or superseded;
3. preserve the current domain contract and supported behavior without reopening unrelated feature design;
4. apply the [Change Process](../../Engineering/Standards/ChangeProcess.md), [Concurrency standard](../../Engineering/Standards/Concurrency.md), and only the domain standards touched by the integration;
5. stop when serial parity, ownership, deterministic output, bounded lifetime, or a prerequisite cannot be proven.

## Shared Work-Package Contract

Every package:

- searches for existing runtimes, threads, queues, locks, atomics, callbacks, scopes, and producer/consumer paths;
- names mutable and lifetime owners plus every cross-thread transfer;
- retains a final-contract serial reference before enabling parallel execution;
- bounds tasks, queues, scratch/progress storage, frames in flight, and cancellation state;
- uses dependencies for correctness and deterministic owner-thread fan-in;
- removes the worker mechanism, alias, adapter, or synchronization path it replaces;
- validates the narrow mechanism and the complete touched ownership path;
- reports exact evidence, limitations, unavailable checks, and `PASS` or `BLOCKED`.

The [Engineering Standards index](../../Engineering/Standards/README.md) owns detailed rules. Work-package rows name only the concurrency outcome and its minimum proof.

## Dependency Shape

```text
MT-00 -> MT-01 -> MT-02 -> MT-03 -> MT-04
MT-04 -> MT-05 -> MT-06 -> MT-08 -> MT-09 -> MT-10 -> MT-11 -> MT-12
MT-04 -> MT-07
MT-07 + MT-12 -> MT-13 -> MT-14 -> MT-15
```

Dependencies express concurrency readiness, not one commit per package. Adjacent packages may be combined only when their ownership and evidence remain independently reviewable.

## Phase 0 - Task Runtime Foundation

| ID | Concurrency outcome | Depends on | Minimum evidence |
| --- | --- | --- | --- |
| `MT-00` | Inventory current thread owners, executors, pools, queues, waits, synchronization, captured lifetimes, serial behavior, and representative baselines. | none | source-backed concurrency map, reproducible serial baseline, hazard and replacement ledger, no implementation claim |
| `MT-01` | Establish the immutable task-graph contract and its serial executor. | `MT-00` | topology validation, cycle/bounds rejection, repeated serial equality, deterministic failure settlement |
| `MT-02` | Add one fixed worker executor without changing graph semantics. | `MT-01` | serial/1/2/N equality, exclusive outputs, bounded ready work, clean worker startup/shutdown |
| `MT-03` | Complete scopes, cancellation, failure, events, lanes, and owner-lifetime settlement. | `MT-02` | cancellation/failure/destruction stress, exactly-once settlement, no worker waits or detached work |
| `MT-04` | Reconcile runtime APIs and migrate one existing bounded workload; remove its competing worker mechanism. | `MT-03` | production-path use, serial/parallel equality, old path deletion, stale-name search, no behavior regression |

## Phase 1 - Owner and Publication Boundaries

| ID | Concurrency outcome | Depends on | Minimum evidence |
| --- | --- | --- | --- |
| `MT-05` | Schedule eligible GameFramework system work from frozen owner inputs using declared hazards, exclusive outputs, and deterministic owner commit. | `MT-04` | mutation-during-work rejection, stable fan-in, serial/parallel equality, measured grain threshold |
| `MT-06` | Transfer immutable render input through a bounded `RenderFrameQueue` to one render coordinator. | `MT-05` | explicit full policy, producer/consumer stress, stale-generation rejection, ordered close and shutdown |
| `MT-07` | Consolidate existing Editor/tool background work onto owned `SparkleTasks` scopes without worker-side UI callbacks. | `MT-04` | bounded progress, late-result rejection, cancellation/close stress, replaced pool/thread deletion |
| `MT-08` | Express eligible renderer preparation as dependency-ready tasks with exclusive outputs and deterministic join. | `MT-06` | preserved serial result and frame-graph input, crossover evidence, critical-path trace, no shared renderer mutation |

Domain schema, UI behavior, import/cook behavior, renderer features, and data-layout redesign are prerequisites or separate work owned by their subject documents; they are not outcomes of these packages.

## Phase 2 - Backend Recording Concurrency

| ID | Concurrency outcome | Depends on | Minimum evidence |
| --- | --- | --- | --- |
| `MT-09` | Add exclusive D3D12 allocator/list recording leases with bounded per-recording transient storage and completion-token retirement. | `MT-08` | wrong-owner rejection, delayed-retirement stress, native validation, serial/parallel recording equality and crossover |
| `MT-10` | Implement the equivalent Vulkan pool/buffer lease and synchronization ownership contract. | `MT-09` | Vulkan validation/synchronization, parity with the public lease contract, delayed-retirement stress |
| `MT-11` | Record compiled dependency-independent frame-graph groups concurrently while preserving single-owner ordered submission and presentation. | `MT-10` | compiled-order equality, barrier/queue correctness, randomized completion, tiny/heavy crossover on both backends |

The canonical [Renderer/RHI boundary](../RendererRhiBoundary.md) continues to own pass scheduling, barriers, resource lifetime, backend responsibility, and feature policy. This phase changes only recording concurrency and its lifetime edges.

## Phase 3 - Concurrency Hardening and Evidence

| ID | Concurrency outcome | Depends on | Minimum evidence |
| --- | --- | --- | --- |
| `MT-12` | Prove or reject additional frame-pipeline or GPU-queue overlap independently of CPU task and recording concurrency. | `MT-11` | correlated timelines, synchronization/ownership-transfer cost, pacing and input-latency result, rollback or deletion of losing paths |
| `MT-13` | Close atomic publication, wait predicates, reclamation, cancellation, queue-close, and shutdown state machines across retained paths. | `MT-07`, `MT-12` | state/transition record, spurious/lost-wakeup tests, race stress, available sanitizers/native validation |
| `MT-14` | Establish bounded worker, lane, third-party-worker, SMT/NUMA, and oversubscription policy. | `MT-13` | topology/configuration sweep, queue delay and tail latency, memory ceiling, product-default rationale |
| `MT-15` | Complete deterministic and performance evidence for every retained parallel path and delete transition-only concurrency code. | `MT-14` | serial oracle, randomized completion, 1/2/N results, tiny/representative crossover, shutdown proof, stale-reference audit |

## Changelist Design Gate

Before a package edits code, identify the final owners and files for only the concurrency concepts it changes: executor, scope/execution state, owner commit, publication queue, coordinator handoff, recording lease, synchronization protocol, or shutdown path.

Reject a second runtime, per-subsystem pool, compatibility scheduler, duplicate queue, broad-lock fallback, numbered replacement, or public diagnostics framework. When a domain change becomes necessary to make the concurrency boundary valid, stop and route that change through the owning architecture or standard instead of absorbing it into this plan.

## Completion and Resumption

A package is complete only when its concurrency outcome exists in current code, its serial behavior and terminal states match, replaced concurrency paths are removed, bounds and owner lifetimes are tested, and the [Change Process completion report](../../Engineering/Standards/ChangeProcess.md#completion-report) records `PASS`.

When current architecture supersedes an earlier mechanism, prove the enduring concurrency invariant through the current owner. Do not restore obsolete code to satisfy historical prompt wording.

## Final Plan Gate

The multithreading plan is complete when:

- one bounded task runtime serves retained owned CPU work;
- all mutable cross-thread paths have named owners and explicit immutable-transfer, lease, or synchronization contracts;
- owner commits and fan-in are deterministic and serial-equivalent;
- frame publication, render recording, submission, cancellation, and shutdown remain bounded and settle correctly;
- each retained parallel mechanism beats its measured serial crossover without unacceptable memory, pacing, or latency cost;
- competing pools, schedulers, queues, waits, aliases, and transition diagnostics are removed;
- current code, tests, and traces reproduce the claimed concurrency behavior.

Completion of this plan makes no claim that unrelated renderer, content, neural, portfolio, or product-roadmap outcomes are complete.
