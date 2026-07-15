# NVIDIA Multi-Queue Submission Research And Sparkle Integration Direction

## Scope and pinned references

This record activates Priority 2A and captures the external evidence used to evolve Sparkle's graphics-only frame submission into a backend-neutral multi-queue model. It is intentionally implementation-facing: every adopted concept must have one owner in Sparkle, and concepts that do not yet serve a workload are recorded without adding duplicate managers or compatibility layers.

The primary references were inspected at these exact revisions on 2026-07-15:

- [NVIDIA NVRHI `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/tree/8e8c36e37558acec333204619b95d9d2fcdc4a79), including its [public queue/submission contract](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/include/nvrhi/nvrhi.h#L3073-L3165), [D3D12 submission and queue waits](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/src/d3d12/d3d12-device.cpp#L617-L653), and [Vulkan per-queue timeline submission](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/src/vulkan/vulkan-queue.cpp#L115-L207).
- [NVIDIA Donut `bc1ea24b`](https://github.com/NVIDIA-RTX/Donut/tree/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937), especially the [render-thread texture finalization path](https://github.com/NVIDIA-RTX/Donut/blob/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937/src/engine/TextureCache.cpp#L669-L721).
- [NVIDIA RTXDI 3.0](https://github.com/NVIDIA-RTX/RTXDI), which uses Donut and NVRHI as the D3D12/Vulkan application and RHI layers rather than defining a second submission system.

NVRHI and Donut are references for responsibility boundaries and proven synchronization shapes, not APIs to reproduce mechanically. Sparkle keeps its own value handles, frame graph, descriptor services, resource allocator, and backend interop contracts.

## Findings from NVRHI

### Queue identity belongs to command-list creation

NVRHI defines `Graphics`, `Compute`, and `Copy` queue identities. A command list is created with one queue type, and validation rejects submission to a mismatched queue. Copy and compute command lists expose restricted operation subsets. This prevents a pass from being recorded generically and assigned to an incompatible physical queue after the fact.

Sparkle direction:

- add one RHI queue enum shared by capabilities, command-list identity, submissions, waits, and diagnostics;
- make every `RenderCommandList` report its queue type;
- have Renderer select the queue before recording;
- validate pass kind and resource state compatibility against that queue.

### A submission token is `(queue, monotonically increasing instance)`

`executeCommandLists` returns the submitted queue instance. `queueWaitForCommandList(waitQueue, executionQueue, instance)` makes one GPU queue wait for a particular instance from another queue. Queue-local ordering handles dependencies on the same queue without an explicit wait.

Sparkle direction:

- use a value-like `RhiSubmissionToken { Queue, Value }`;
- never use a CPU pointer, native fence address, or frame index as submission identity;
- ignore invalid tokens and same-queue waits, and reject unsupported queues or future/unsubmitted values;
- expose completion queries and CPU waits by token for retirement and exceptional synchronization, while ordinary dependencies remain GPU waits.

### Physical synchronization is per queue

NVRHI D3D12 owns one native command queue and fence progression per exposed queue. A wait calls `ID3D12CommandQueue::Wait` on the waiting queue using the producing queue's fence and submitted value.

NVRHI Vulkan owns a timeline semaphore and monotonically increasing counter per queue. A submission includes accumulated timeline waits and signals the queue's own tracking semaphore. Completion is read from that semaphore counter. This remains valid when multiple logical queue roles alias the same Vulkan queue family/handle, although Sparkle should omit redundant waits when roles resolve to the same physical queue.

Sparkle direction:

- D3D12: one queue/fence progression for each enabled queue role;
- Vulkan: select graphics, compute, and transfer families with dedicated-family preference and legal alias fallback; use timeline semaphores for queue tokens and binary semaphores only for acquire/present;
- keep native fences and semaphores private to the backend.

### Lifetime follows submitted references, not the current frame

NVRHI command lists retain referenced resources. On submission, the command-list instance is stamped with its queue submission value and held until that queue completes it. Vulkan staging allocations additionally record their last-use queue and command-list instance.

Sparkle direction:

- command recording must track logical owned resources, views, tables, and transient memory blocks actually referenced;
- submission stamps those records with the returned token;
- release moves ownership into retirement with the record's last-use token;
- retirement drains by querying that token's producing queue;
- resources never submitted can retire immediately;
- descriptor-table retirement follows the same rule and must not remain a frame-index ring once multi-queue recording is active.

This is stricter than conservatively stamping everything with the next graphics fence. Conservative graphics stamping becomes incorrect once the last use can occur on compute or copy.

## Findings from Donut and NVIDIA samples

Donut does not add a second queue scheduler over NVRHI. Its texture cache performs CPU work asynchronously, then the render thread obtains an NVRHI command list, records final resource creation/uploads, submits it, and runs device garbage collection. Most Donut rendering continues to use the default graphics queue even though NVRHI exposes compute and copy.

The production lesson is separation, not maximum queue count:

- content systems prepare work and own asset/cache state;
- the renderer decides when and where GPU work is recorded;
- NVRHI owns physical submission and lifetime;
- queue selection is opt-in per workload;
- unused queue capability does not force every pass through a scheduler abstraction.

RTXDI and related NVIDIA samples consume the Donut/NVRHI split. They do not introduce feature-local fences or queue managers. Sparkle features must likewise declare queue intent to the frame graph rather than owning native synchronization.

## Sparkle target architecture

```text
Renderer feature/pass intent
        |
        v
FrameGraph compiler
  - resolves queue for each pass
  - builds queue-local batches
  - emits cross-queue token dependencies
        |
        v
RhiCommandSubmissionService
  - creates/opens queue-typed command lists
  - submits batches and returns value tokens
  - installs GPU queue waits
        |
        v
D3D12 queue+fence / Vulkan queue+timeline semaphore
        |
        v
Token-driven resource, descriptor, upload, and command-list retirement
```

There is one policy owner and one execution owner:

- Renderer/frame graph owns pass queue selection, batching, and dependency compilation.
- RHI owns command-list legality, native queues, submission counters, waits, completion, and retirement execution.

`TextureManager`, material caches, ray-tracing features, and individual passes must not acquire queues, signal fences, or wait on tokens directly. They receive a queue-typed recording context selected by the frame graph or frame coordinator.

## Integration sequence

1. Introduce value contracts: queue type, submission token, command-list queue identity, capability queries, and GPU wait/completion operations.
2. Replace graphics-global fence assumptions with backend queue state and token-aware retirement.
3. Build reusable command-list pools per queue; do not hard-code one list per queue or a maximum submission count per frame.
4. Preserve the current graphics frame through the new submission path before assigning non-graphics work.
5. Move texture and device-local buffer uploads onto copy-capable command lists and insert a graphics wait only when the uploaded resources are consumed.
6. Add frame-graph queue assignment and batch compilation. Raster and external-provider work remains graphics. Transfer work prefers copy. Compute work remains graphics unless explicitly marked asynchronous and the backend exposes compute.
7. Validate multi-frame replacement/destruction, resize, cancellation, device idle, capture/interoperability, D3D12, and Vulkan.

## Implementation checkpoint — 2026-07-15

Implemented in this batch:

- backend-neutral `ERhiQueueType`, `RhiSubmissionToken`, queue capabilities, queue-typed command lists, GPU queue waits, CPU token waits, and completion queries;
- D3D12 graphics/compute/copy queues with one monotonically increasing fence progression per queue;
- Vulkan dedicated-family selection with truthful alias fallback, per-queue timeline semaphores, and binary acquire/present bridging;
- concurrent Vulkan resource sharing across selected queue families, avoiding an implicit queue-family ownership bug in the initial integration;
- command-list resource retention from recording through submission, per-queue last-use state, and token-driven resource, transient-memory, and staging retirement;
- texture uploads recorded on copy where supported, followed by a graphics token wait and graphics-side shader-resource transition without a CPU stall;
- preservation of the existing graphics frame through the generic submission path.

Deliberately not claimed complete:

- the frame graph still records its existing pass plan as one graphics command stream;
- reusable command-list pools and arbitrary queue-local submission batches are required before opt-in asynchronous-compute passes can execute correctly;
- descriptor allocation retirement is still conservatively protected by waiting every queue that used a recycled frame slot before its descriptor frame pool is reset. Resource retirement is token-exact; descriptor retirement should move to the same explicit last-use representation with frame-graph multi-batch integration;
- backend runtime parity and multi-frame destruction stress tests remain to be run. Static builds are recorded separately from runtime evidence.

This checkpoint avoids a misleading half-scheduler: the copy workload is real and uses the new contract, while existing compute passes remain graphics work until the frame graph can compile and execute arbitrary interleaved queue batches.

## Explicit non-goals

- No generic job system or CPU task scheduler.
- No feature-local queue/fence manager.
- No native queue, fence, or semaphore in Renderer data.
- No automatic migration of every compute dispatch to asynchronous compute.
- No fixed array of arbitrary submission phases.
- No flush after ordinary upload or cross-queue dependency.
- No compatibility wrapper preserving the old graphics-only submission implementation once all callers migrate.

## Acceptance criteria

- D3D12 and Vulkan expose truthful graphics/compute/copy capabilities and queue-typed command lists.
- Every successful submission returns a durable logical token; queue waits consume tokens without a CPU stall.
- The existing graphics-only frame renders through the generic submission path.
- Copy uploads can overlap CPU and graphics work and synchronize through a token dependency without `Flush`, `WaitForIdle`, `vkQueueWaitIdle`, or per-upload fence creation.
- Async-compute passes are opt-in, compiled by the frame graph, and fall back to graphics when unsupported.
- Resource, descriptor, staging, transient-memory, and command-list retirement use the last submission token that referenced the object.
- Acquire/present synchronization stays backend-owned and is bridged into the graphics submission without leaking native semaphores or queues.
- Queue diagnostics report logical role, physical aliasing, submitted value, and completed value.
