# Validation, Performance, and Evidence

Status: binding verification standard

Applies to: correctness checks, diagnostics, logging, instrumentation, comments, tests, benchmarks, captures, and AI-assisted work

## Minimal Correctness Checks

Validation belongs at the narrowest owner that can enforce an invariant once.

Use:

- a typed result for expected boundary rejection;
- an assertion for programmer misuse or impossible owner-state violations in an appropriate build;
- a fatal error only when continuing cannot preserve correctness;
- native D3D12/Vulkan validation for API state and ownership;
- a focused test for state-machine, ordering, lifetime, math, or ABI behavior.

Avoid repeated checks in forwarding layers, validation mirrors of authority, per-item counters/logs, broad diagnostics types exposing caches/queues, duplicate error paths, and permanent production code used only to prove a migration.

## Logging

Log one concise actionable failure, an important lifecycle transition needed by an existing workflow, or a bounded failure summary. Include enough stable identity and operation context to act.

Do not log every entity, asset, pass, task, descriptor, or packet; normal polling/progress per frame; the same error in every forwarding layer; or performance counters that belong in a profiler.

## Instrumentation

- Reuse existing profiler, PIX, Nsight, RenderDoc, frame-graph, allocator, and debugger hooks.
- Add only the label or scope needed to identify the changed critical path.
- Keep capture-time instrumentation private.
- Do not add public profiler frameworks, runtime log streams, task panels, cache browsers, default report files, or broad snapshot products without a current product owner.
- Remove disposable fault injectors, migration validators, and report builders after the invariant is encoded in its owner and regression test.

## Comments

Comments explain ownership/lifetime, dependency/order reasons, memory-order proofs, units and coordinate spaces, ABI/backend constraints, measured policy choices, and non-obvious failure or retirement behavior.

Comments do not narrate obvious code, preserve obsolete history, promise unspecified refactors, repeat a prompt, call unsupported behavior “thread safe” or “lock free,” or conceal compatibility without a deletion gate.

Public API documentation conventions beyond this baseline remain an explicit [coding-style decision](CodingStyle.md#decision-status).

## Performance Is a Delivery Property

Every material runtime, editor, renderer, RHI, task, cooking, import, launcher, shader, test-infrastructure, or build-pipeline change MUST classify performance as one of:

- **improves** — names the affected budget and supplies reproducible before/after evidence;
- **preserves** — explains why the affected cost remains bounded and supplies evidence proportional to regression risk;
- **no runtime exposure** — identifies why no shipped, authoring, loading, cooking, build, memory, or frame path changes;
- **blocked** — names the missing workload, tool, hardware, baseline, or measurement and does not claim acceptance.

Before implementation, identify applicable cost dimensions: work cardinality and complexity, scans/lookups/copies/uploads, allocation and high-water memory, cache/branch behavior, task and synchronization overhead, CPU/GPU critical path, input-to-present latency, loading/cooking/startup time, compile/link/package cost, and steady-state versus cold behavior.

Not every change needs a benchmark. Every change needs an explicit cost model, and any change with material exposure or an optimization claim needs a falsifiable measurement. Correctness, quality, determinism, memory, backend support, and latency are co-equal constraints; moving cost out of one reported metric is not an optimization.

Use existing counters, profiler scopes, captures, and harnesses. Temporary measurement code stays private and is removed unless an existing product workflow owns it. Do not pay permanent logging, diagnostics, snapshot, or reporting cost to demonstrate one changelist.

## Performance Order of Operations

Optimize in this order unless evidence shows otherwise:

1. remove unnecessary work;
2. remove full rebuilds, scans, copies, and uploads;
3. improve algorithm and data access;
4. remove allocation churn and pointer chasing;
5. make lifetime and caching persistent/incremental;
6. reduce synchronization and contention;
7. expose independent ranges;
8. parallelize above the measured crossover;
9. tune backend and GPU queue behavior from captures.

### CPU Evidence

Inspect complexity, cardinality, cache locality, working set, branch behavior, dispatch/indirection, repeated lookup/join/scans, allocation/contention, false sharing, task/grain/queue delay, critical path, tail imbalance, and p50/p95/p99.

### GPU Evidence

Inspect resource/descriptor lifetime, upload and dirty bytes, transient/persistent memory, barriers and queue ownership, command count and submission overhead, cold/warm pipelines and shaders, raster/RT build inputs, queue overlap, bubbles, bandwidth, occupancy, and temporal/provider correctness.

Do not improve CPU utilization by silently regressing GPU barriers, memory, commands, descriptors, residency, RT behavior, or input latency.

### Measurement Record

For each performance claim record:

- exact build/configuration;
- CPU/topology and worker policy;
- backend, adapter, driver, and validation state;
- workload/scene and cold/warm state;
- serial, 1, 2, and N modes as applicable;
- before/after values and critical path;
- memory high-water and GPU result;
- regressions, variance, and limitations.

Use tiny work to reveal overhead and representative heavy work to reveal useful parallelism. A trace, utilization, thread count, or FPS alone is not proof.

## Neural Training and Runtime Evidence

Measure offline preparation separately from runtime inference. Offline evidence may include dataset/preprocessing throughput, batch/precision sweeps, convergence/quality, optimizer choices, utilization/stalls, peak memory, and deterministic artifact time.

Runtime evidence MUST include preprocess, material operators/kernels, postprocess, artifact/weight/intermediate/history bytes, dispatch dimensions, CPU preparation/upload, GPU latency, end-to-end frame contribution, relevant counters, pacing/input latency under representative load, and classical-versus-neural quality and failures on identical inputs.

Choose the accepted configuration from a quality-performance-memory frontier, not only the fastest or highest-scoring point.

## Test Matrix

Required where applicable:

- serial oracle and deterministic state/byte/image/order comparison;
- 1, 2, and N workers plus randomized completion;
- cancellation at each stage and destruction with work in flight;
- stale handle/generation/sequence rejection;
- bounded queue/backpressure and memory ceiling;
- frozen-view mutation and wrong-thread/lease misuse rejection;
- D3D12 GPU validation and Vulkan validation/synchronization;
- delayed GPU completion and deferred retirement;
- cold/warm cache behavior and tiny-work threshold;
- feature-preservation matrix;
- math reference values and numerical error bounds;
- artifact provenance/shape/layout/precision rejection;
- offline/reference versus runtime neural tolerance;
- train/validation/test separation and deterministic export;
- classical/neural comparison with visual/temporal failures;
- exact hardware/driver/capability matrix and reduced reproducer;
- clean package proof excluding training dependencies/data;
- AI-assisted test defect-detection evidence.

Prefer controllable barriers, events, generations, and fault hooks over sleeps. Tests must detect an injected defect in the invariant they claim to prove. Report unavailable hardware, tools, sanitizers, and provider paths as unavailable rather than simulated or passed.

## AI-Assisted Engineering

AI tools may accelerate search, implementation, tests, shader/model exploration, and alternatives. They are never an authority.

For materially AI-assisted work:

- inspect every changed line and surrounding ownership path;
- verify APIs and citations against primary or local sources;
- independently check math, spaces, units, numerical assumptions, bounds, and ABI;
- compile generated C++/shaders with normal warnings and backend gates;
- check lifetime, synchronization, determinism, security, license/provenance, and package impact;
- prove generated tests fail under the defect they claim to detect;
- measure performance claims on actual workloads and hardware;
- remove generated boilerplate, speculative abstraction, excessive comments, logging, and validation;
- never commit private prompts, hidden reasoning, credentials, or generated chatter as project evidence;
- record which categories were independently verified.

The engineer remains accountable for architecture, correctness, attribution, data/model provenance, performance, and communication.
