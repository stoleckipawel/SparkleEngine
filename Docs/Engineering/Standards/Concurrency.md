# Concurrency

Status: binding concurrency and synchronization standard

Applies to: CPU tasks, threads, queues, locks, atomics, waits, callbacks, publication, cancellation, and shutdown

This standard owns reusable concurrency guardrails. [J. Multithreaded Engine Architecture](../../Architecture/Multithreading/MultithreadedEngineArchitecture.md) owns SparkleEngine's applied target topology and frame lifecycle; it must satisfy these rules without duplicating their full rationale.

## One Runtime

Use `SparkleTasks` for owned CPU task execution. Do not add subsystem pools, `std::async` integration paths, detached workers, hidden ECS schedulers, worker-side wait loops, or second cancellation/progress runtimes.

Blocking file, platform, and process work uses the configured blocking-I/O lane or an existing narrow platform operation. Third-party worker counts belong in the concurrency budget.

## Ownership First

Mutable data has one owner thread or task. Cross-thread data is immutable, transferred, exclusively leased, or explicitly concurrent under one documented protocol.

Atomics do not make an object graph thread-safe. A mutex around a broad scene, renderer, editor, or cache is not an ownership model.

## Task Rules

- Dependencies express correctness; lane and priority express policy only.
- Worker tasks do not wait for other tasks.
- Fan-in occurs through graph dependencies or a bounded host boundary.
- Tasks read immutable state and write exclusive ranges or task-local results.
- Cleanup/finalization settles exactly once on success, failure, or cancellation.
- Scopes settle before captured owners and storage are destroyed.
- Small workloads use the serial path.
- Grain size comes from scheduling cost, variance, cache behavior, and critical path.
- Nested engine/third-party parallelism must not oversubscribe the host.

## Publication and Atomics

Treat each atomic protocol as a state machine. Document:

- states and legal transitions;
- the writer or claimer for each state;
- release publication and acquire consumption;
- reuse/reclamation edges;
- ABA or generation behavior;
- cancellation and shutdown transitions;
- why each memory order is sufficient.

Use a mutex for a compound invariant, an atomic for a small independent transition, and a condition variable/semaphore for parking. Start with owner-only, mutex, or sequentially consistent reference behavior; weaken it only after measurement and a lifetime proof. Cross-module raw-pointer lock-free structures are forbidden.

## Locks and Waits

- Every mutex, atomic, wait, condition variable, semaphore, device-idle call, and queue has a named owner and invariant.
- Wait predicates handle spurious wakeups and lost-wakeup ordering.
- Lock order is explicit where more than one lock can be held.
- Do not call arbitrary callbacks, events, ImGui, renderer/RHI, logging, file I/O, driver APIs, or destruction while holding an engine lock.
- Spin only for a measured extremely short handoff whose owner is guaranteed to run.
- Routine worker waits, busy-yield loops, and help-while-waiting correctness are forbidden.
- Final shutdown or explicit backpressure may block only at a documented host boundary.

The target is zero unclassified synchronization, not zero synchronization.

## Scheduling and Cache Behavior

Audit false sharing; contention on vectors, logging, allocators, descriptors, and atomics; priority inversion; convoying; starvation; thundering herd; preemption; migration; SMT; NUMA; oversubscription; cost imbalance; global barriers; and unbounded producer queues.

Prefer fewer, coarser, dependency-correct tasks to high utilization with a longer critical path.

## Required Concurrency Evidence

Where applicable, validate:

- serial oracle and identical final contract;
- 1, 2, and N workers;
- randomized completion and controlled delay;
- cancellation at each stage;
- owner destruction/close with in-flight work;
- stale generation/sequence rejection;
- queue bounds, backpressure, and memory ceiling;
- wrong-thread and lease misuse;
- delayed GPU completion and retirement;
- tiny-work serial crossover;
- deterministic merge independent of task completion order.

Prefer explicit controllable barriers, events, generations, and fault hooks over sleeps. Every test must fail when its claimed invariant is deliberately broken.
