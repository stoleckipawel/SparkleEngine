# Data-Oriented Design

Status: binding data and memory design standard

Applies to: material sources, transforms, streams, packets, stores, caches, tables, uploads, and hot traversal

Data-oriented design starts from work and access patterns, not from choosing ECS, SoA, or templates.

## Data/Access Inventory

For every material data path touched, record:

- producer and consumers;
- authoritative or derived ownership;
- semantic fields, frequency, cardinality, and expected high-water mark;
- shape, distribution, mutation points, publication, and reclamation lifetime;
- access order and fields read together;
- stable key and CPU/GPU destination;
- bandwidth, latency, and allocation behavior;
- deterministic transform and ordering;
- current measured cost or concrete falsifier.

If these facts are unknown, inspect or measure them before choosing a layout.

## Authority and Projection

- Keep one authoritative mutable source.
- Read models, render worlds, caches, packets, and GPU tables are derived projections.
- Projection is one-way; mutation does not flow backward from renderer, editor, or cache.
- Incremental projections use explicit sequence/generation and a full-resync fallback when lag is possible.
- Regenerated content publishes only the newest supported representation.
- A generation for stale-handle or lifetime rejection is not content compatibility versioning.

## Single-Truth and Copy Budget

> **Default rule: reference, view, handle, or move. A data copy requires a named boundary reason.**

Do not introduce a struct, DTO, cache, snapshot, resolved-state object, copied default, or forwarding packet merely because passing the owner's data is inconvenient. Every additional value holder increases the state that readers and debuggers must classify and creates a possible stale truth.

For each material copy or new holder, identify:

- the authoritative source and the single direction in which data flows;
- why a reference, `std::span`, view, stable handle/ID, or ownership move cannot satisfy the consumer;
- the exact scope or epoch, producer, consumers, publication point, and invalidation or reclamation rule;
- copied bytes, element cardinality, and copy cadence when the path is large, repeated, or frame-hot.

Copies are justified only by a real consumption boundary such as immutable frame/task publication, thread isolation, source-lifetime decoupling, atomic editing/commit, serialization, or a required CPU/GPU ABI. Keep such values immutable after publication, name the boundary or lifetime in the type or variable, resolve them once per epoch, and do not permit mutation or writeback to create a second authority.

Reject convenience mirrors, persistent copies of another owner's settings, parallel mutable read models, repeated resolution of the same snapshot within one epoch, and wrapper objects that only relay copied fields. A short-lived parser buffer or UI edit draft is not authority: keep it local, publish it through the owner once, and discard it. When no boundary reason survives review, remove the holder and read the owner directly.

## Layout Selection

Choose AoS, SoA, AoSoA, sparse set, archetype chunk, indexed table, flat stream, packed record, or object ownership from observed access:

- Split hot/cold, structural/dynamic, authoring/runtime, and CPU-policy/GPU-packed data when consumers or lifetimes differ.
- Keep fields together when consumers read them together.
- Structural operations often fit compact typed AoS records.
- Dense frame work may fit columns or cohesive AoSoA blocks.
- Sparse sets support packed component iteration plus stable lookup; they are not a universal answer.
- Introduce archetype/chunk complexity only when current workloads show simpler stores lose.
- Record the accepted layout, rejected alternatives, and evidence that could overturn the decision.

## Identity and References

Across owner, frame, task, editor, load, or render boundaries:

- use generational IDs or immutable typed handles;
- never retain vector indices as identity;
- never retain pointers, iterators, spans, views, dense indices, or arena addresses beyond their documented epoch;
- map persistent authoring GUIDs to compact runtime IDs once;
- keep `EntityId` and `RenderObjectId` distinct;
- key temporal, render, and retirement state by stable identity rather than current row order.

## Variable-Length and Hot Records

Use bounded flat arrays plus offsets/counts for packet and GPU variable-length data. Hot records MUST NOT contain owning vectors or strings, callbacks, mutexes/atomics without a measured per-record protocol, allocators, service pointers, heavyweight assets, or editor-only metadata.

Preallocate stable slots or bounded arenas when cardinality is known. Per-execution and per-frame storage needs an explicit reset and lifetime rule.

## Parallel Transforms

- Partition a leading dense component, query, or input range.
- Give each task a private output or exclusive preassigned range.
- Do not push into shared vectors, bump one frame-hot cursor, mutate assets, or update shared caches per item.
- Use task-local pages, arenas, command buffers, reductions, buckets, or fixed slices.
- Merge with a stable documented key such as `(phase, system, entity/object, partition, local sequence)`.
- Completion order is never semantic.
- Apply structural changes at the owner commit after readers join.
- Views and ranges do not escape the execution epoch.

## Memory Budget

State the governing equation for concurrency and buffering:

```text
workers * frames in flight * queues * per-worker/per-frame capacity
```

Include packet arenas, scratch, command allocators/pools/lists, upload/readback pages, descriptor blocks, decoded/cooked assets, persistent/transient GPU buffers, and retirement backlog as applicable.

For each material allocation family define expected capacity, measured high-water, reuse point, overflow/spill/failure policy, cancellation cleanup, and GPU-token retirement. Do not exchange a modest CPU improvement for uncontrolled peak memory.

## Evidence

Compare against the replaced path using the measurements that matter:

- bytes read/written and working-set/high-water memory;
- allocations/reallocations, cache misses, bandwidth, branches, and pointer indirection;
- query candidates/matches and traversal time;
- extraction, apply, merge, and commit time;
- packet and dirty/upload bytes;
- GPU table updates;
- serial/parallel crossover and critical path.

“Uses ECS,” “uses SoA,” “cache friendly,” and “parallel” are not evidence.

Use external precedents precisely. Data-oriented references support inventories and access-driven layouts, not universal SoA. Epic MassEntity supports typed queries, transient views, and deferred structural change, not a requirement to reproduce Unreal scale. NVIDIA and AMD renderers support persistent/indexed render data and backend-explicit ownership, not a general gameplay ECS.

For each material choice record the exact source location, adopted behavior, deliberately un-copied behavior, and falsifying measurement/test.

## Neural Data and Artifacts

When a real neural feature is selected, inventory offline and runtime data:

- dataset license/provenance and train/validation/test boundaries;
- preprocessing, normalization, augmentation, coordinates, and color;
- model/operator topology, shapes, layouts, precision, numerical range, loss, and metrics;
- immutable weights/constants plus deterministic export/cook metadata;
- runtime inputs, intermediates, history, outputs, persistent storage, and per-frame storage;
- CPU preparation, upload, dispatch access, classical baseline, and fallback;
- artifact/package lifetime and capability requirements.

Feature-private bounded descriptors are preferred over a generic engine tensor primitive. Artifacts are immutable, validated, provenance-tracked, and contain no training framework state. Parse and resolve them outside frame-hot work; keep training dependencies out of runtime packages. Persistent weights follow renderer/RHI retirement, dynamic intermediates use declared frame-graph resources, and the classical path remains a tested product fallback.

Quality metrics require visual/temporal failure cases and dataset scope. Choose a model/kernel configuration from the quality-performance-memory frontier rather than one isolated metric.
