# NVIDIA/AMD Multi-Queue Submission Research And Completed Sparkle Integration

> **Status: closed.** Priority 2A was implementation-audited and validated on 2026-07-16. The active graphics, compute, and copy workloads use this architecture; no foundational follow-up remains in this record.

## Scope and pinned references

This record closes Priority 2A and captures the external evidence used to evolve Sparkle's graphics-only frame submission into a backend-neutral multi-queue model. NVIDIA NVRHI/Donut are the primary backend and application references; AMD RPS provides an independent render-graph scheduling cross-check. It is intentionally implementation-facing: every adopted concept has one owner in Sparkle, without duplicate managers or compatibility layers.

The primary references were inspected at these exact revisions on 2026-07-15:

- [NVIDIA NVRHI `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/tree/8e8c36e37558acec333204619b95d9d2fcdc4a79), including its [public queue/submission contract](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/include/nvrhi/nvrhi.h#L3073-L3165), [D3D12 submission and queue waits](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/src/d3d12/d3d12-device.cpp#L617-L653), and [Vulkan per-queue timeline submission](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/src/vulkan/vulkan-queue.cpp#L115-L207).
- [NVIDIA Donut `bc1ea24b`](https://github.com/NVIDIA-RTX/Donut/tree/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937), especially the [render-thread texture finalization path](https://github.com/NVIDIA-RTX/Donut/blob/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937/src/engine/TextureCache.cpp#L669-L721).
- [NVIDIA RTXDI 3.0](https://github.com/NVIDIA-RTX/RTXDI), which uses Donut and NVRHI as the D3D12/Vulkan application and RHI layers rather than defining a second submission system.
- [AMD Render Pipeline Shaders `f3330f53`](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/tree/f3330f5306d15af8529a310f6255225c864b0961), including its [node queue requirements and async preference](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/f3330f5306d15af8529a310f6255225c864b0961/include/rps/runtime/common/rps_runtime.h#L197-L213), [queue capability input](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/f3330f5306d15af8529a310f6255225c864b0961/include/rps/runtime/common/rps_runtime.h#L590-L601), and [compiled queue-batch/fence layout](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/f3330f5306d15af8529a310f6255225c864b0961/include/rps/runtime/common/rps_runtime.h#L790-L821).

The queue-topology deep dive was cross-checked on 2026-07-16 against NVIDIA's [nvpro_core Vulkan queue selection](https://github.com/nvpro-samples/nvpro_core/blob/master/nvvk/context_vk.cpp), which scores specialized families and requests multiple queue indices when a family exposes them, and Streamline's [explicit Vulkan queue-family/queue-index contract](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideManualHooking.md).

NVRHI and Donut are references for responsibility boundaries and proven synchronization shapes, not APIs to reproduce mechanically. Sparkle keeps its own value handles, frame graph, descriptor services, resource allocator, and backend interop contracts.

## Findings from NVRHI

### Queue identity belongs to command-list creation

NVRHI defines `Graphics`, `Compute`, and `Copy` queue identities. A command list is created with one queue type, and validation rejects submission to a mismatched queue. Copy and compute command lists expose restricted operation subsets. This prevents a pass from being recorded generically and assigned to an incompatible physical queue after the fact.

Sparkle direction:

- add one RHI queue enum shared by capabilities, command-list identity, submissions, waits, and diagnostics;
- make every `RenderCommandList` report its queue type;
- have Renderer select the queue before recording;
- validate pass kind and resource state compatibility against that queue.

NVRHI also separates the logical command-list object from reusable native recording instances. D3D12 acquires an available allocator/native-list pair and Vulkan acquires an available command buffer from the owning queue. Sparkle follows that shape with dynamically growing per-frame, per-queue pools. The number of frame-graph batches is therefore not encoded as a backend constant, and a frame-slot pool is reset only after its actual last submission on that queue completes.

### A submission token is `(queue, monotonically increasing instance)`

`executeCommandLists` returns the submitted queue instance. `queueWaitForCommandList(waitQueue, executionQueue, instance)` makes one GPU queue wait for a particular instance from another queue. Queue-local ordering handles dependencies on the same queue without an explicit wait.

Sparkle direction:

- use a value-like `RhiSubmissionToken { Queue, Value }`;
- never use a CPU pointer, native fence address, or frame index as submission identity;
- ignore invalid tokens and same-queue waits, and reject unsupported queues or future/unsubmitted values;
- expose completion queries and CPU waits by token for retirement and exceptional synchronization, while ordinary dependencies remain GPU waits.

### Physical synchronization is per queue

NVRHI D3D12 owns one native command queue and fence progression per exposed queue. A wait calls `ID3D12CommandQueue::Wait` on the waiting queue using the producing queue's fence and submitted value.

NVRHI Vulkan owns a timeline semaphore and monotonically increasing counter per logical queue. A submission includes accumulated timeline waits and signals the queue's own tracking semaphore. Completion is read from that semaphore counter, while submission access to a native `VkQueue` is serialized. Sparkle preserves logical tokens when roles alias but gives every exact `(family index, queue index)` alias one shared native submission mutex. This satisfies Vulkan's external-synchronization rule without inventing a second queue manager.

Sparkle direction:

- D3D12: one queue/fence progression for each enabled queue role;
- Vulkan: select graphics, compute, and transfer locations by both family and queue index, preferring specialized families and otherwise using unused compatible queue indices before legal alias fallback; use timeline semaphores for queue tokens and binary semaphores only for acquire/present;
- keep native fences and semaphores private to the backend.

### Lifetime follows submitted references, not the current frame

NVRHI command lists retain referenced resources. On submission, the command-list instance is stamped with its queue submission value and held until that queue completes it. Vulkan staging allocations additionally record their last-use queue and command-list instance.

Sparkle direction:

- command recording must track logical owned resources, views, tables, and transient memory blocks actually referenced;
- submission stamps those records with the returned token;
- release moves ownership into retirement with the record's last-use token;
- retirement drains by querying that token's producing queue;
- resources never submitted can retire immediately;
- descriptor reuse must be gated by completed submissions. Per-object token stamping is exact; conservative frame-slot grouping is also valid when that slot cannot recycle until its actual last graphics, compute, and copy submissions have completed.

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

AMD RPS independently validates the same ownership split. Nodes state required graphics/compute/copy capability and may prefer asynchronous execution; the render graph receives the queues that actually exist, resolves valid/preferred queue masks, and produces same-queue command batches with explicit wait/signal fence indices. Sparkle intentionally implements the smaller subset its workloads require: explicit async preference, truthful capability fallback, stable topological batching, and cross-queue token dependencies. It does not copy RPS's general-purpose scoring scheduler.

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

## Integrated implementation — 2026-07-16

The multi-queue path is now active rather than capability-only:

- backend-neutral `ERhiQueueType`, `RhiSubmissionToken`, queue capabilities, queue-typed command lists, GPU queue waits, CPU token waits, and completion queries;
- D3D12 graphics/compute/copy queue owners with atomic wait/execute/signal submission and one monotonically increasing fence progression per queue;
- Vulkan queue topology selected by `(family index, queue index)`, with specialized-family preference, unused-index selection, legal alias fallback, accurate device queue counts, and shared synchronization for aliased native queues;
- per-logical-queue Vulkan timeline semaphores and swapchain-owned binary acquire/present bridging;
- concurrent Vulkan resource and swapchain-image sharing across selected queue families;
- separate physical queue owners and command recording contexts: queue classes own native submission synchronization and counters, while contexts own only reusable allocator/list or pool/buffer slots;
- dynamically growing command-list pools per frame slot and queue on both backends, replacing the fixed one-list-per-queue design;
- separate frame-graph pass kind and queue preference value types, with capability resolution, queue-local batch compilation, and producer/consumer token dependencies owned by the compiler;
- conservative cross-queue state handoff through `Common`: the producer performs the release transition, RHI installs a queue wait, and the consumer performs its legal queue-local transition;
- a separate graphics prologue whenever non-graphics batches exist, so async work waits for frame initialization/acquire state without inheriting unrelated graphics pass work;
- one highest timeline wait per producing queue in Vulkan submissions, avoiding duplicate semaphore entries;
- invalid queue identities and future/unsubmitted token values are rejected before a native CPU or GPU wait can be installed;
- command-list resource retention from recording through submission, per-queue last-use state, and token-driven resource, transient-memory, and staging retirement;
- texture uploads recorded on copy where supported, followed by a graphics token wait and graphics-side shader-resource transition without a CPU stall;
- copy upload lists opened only when the texture cache reports pending uploads, avoiding an empty copy submission and graphics wait on resident frames;
- transfer passes recorded on copy when the queue is independent, including the final encoded-color copy to the swapchain image;
- explicit async-compute opt-in for scene-depth linearization, sky motion vectors, the exposure moment chain, and exposure history update;
- automatic graphics fallback for every async pass/transfer when the corresponding independent queue is unavailable;
- one end-of-frame graphics join over the latest compute and copy values before presentation and frame-slot reuse;
- Vulkan submission and presentation both pass through the shared physical queue mutex, including aliased logical roles, and an out-of-date acquire rebuilds and retries once before cancellation;
- one public idle operation, `WaitForIdle()`, after deleting the duplicate `Flush()` facade operation.
- D3D12 CPU-only canonical resource-view descriptors feeding shader-visible tables, rather than illegally reading shader-visible descriptors as copy sources; the existing descriptor service remains the only descriptor owner.
- backend texture capture submits through the existing graphics queue owner and waits its returned token, eliminating feature-local D3D12 fences and Vulkan fence submissions.

The queue decision is deliberately not a generic scheduler. Pass authors choose ordinary compute or async-capable compute; transfer helpers declare async-copy eligibility. The frame graph resolves that intent against capabilities, owns dependency policy, and emits batches. Features do not acquire queues or manage fences.

Resource, staging, transient-memory, and command-list retirement is token-exact. Descriptor ranges and Vulkan descriptor pools remain owned by the existing backend descriptor services and are conservatively grouped by frame slot; recycling occurs only after the command context waits that slot's actual last submission on graphics, compute, and copy. This is token-gated, safe for in-flight replacement, and avoids a second descriptor-lifetime manager. Finer per-table token stamping is an optimization only if profiling shows frame-slot retention to be material.

`ShowcaseEditor` builds successfully in `DevelopmentEditor` with both backends compiled. Final D3D12 and Vulkan workload runs exercised startup copy uploads, async-compute passes, normal frame-slot rotation, and presentation. Each backend then remained active through 24 consecutive live window resizes and was stopped by the bounded harness with no standard-error output. The architecture-boundary target and `git diff --check` pass. Static ownership inspection confirms that only `D3D12CommandQueue` calls `ExecuteCommandLists`, only `VulkanCommandQueue` calls `vkQueueSubmit`/`vkQueuePresentKHR`, and capture uses those queue owners rather than feature-local fences.

## Explicit non-goals

- No generic job system or CPU task scheduler.
- No feature-local queue/fence manager.
- No native queue, fence, or semaphore in Renderer data.
- No automatic migration of every compute dispatch to asynchronous compute.
- No fixed array of arbitrary submission phases.
- No flush after ordinary upload or cross-queue dependency.
- No compatibility wrapper preserving the old graphics-only submission implementation once all callers migrate.

## Acceptance closure

| Criterion | Status | Verified implementation evidence |
| --- | --- | --- |
| D3D12 and Vulkan expose truthful graphics/compute/copy capability and queue-typed command lists. | Fulfilled | `RhiQueueCapabilities` separates support from independent execution; every `RenderCommandList` carries one `ERhiQueueType`; Vulkan topology uses both family and queue index. |
| Every successful submission returns a durable logical token and GPU waits do not stall the CPU. | Fulfilled | Both queue owners return `RhiSubmissionToken { Queue, Value }`; frame-graph dependencies become native queue waits; invalid and unsubmitted values are rejected before waiting. |
| The ordinary graphics frame uses the generic submission path. | Fulfilled | The prologue, queue-local batches, final graphics barriers, presentation join, uploads, and capture all submit through `RhiCommandSubmissionService` or the same backend queue owner. |
| Copy uploads synchronize without an ordinary `Flush`, `WaitForIdle`, `vkQueueWaitIdle`, or per-upload fence. | Fulfilled | `FramePipeline` selects copy only when independently supported, submits once, installs a graphics token wait, and performs the graphics-legal final transition. |
| Async compute is explicit and falls back safely. | Fulfilled | Scene-depth linearization, sky motion vectors, exposure moments, and exposure history explicitly call `DispatchAsync`; the compiler falls back to graphics unless compute is independent. |
| Resource, descriptor, staging, transient-memory, and command-list recycling is safe across all queues. | Fulfilled | Resources, heaps, staging, and command slots use per-queue last-use tokens. Descriptor recycling is conservatively frame-slot grouped and occurs only after that slot's last graphics, compute, and copy tokens complete. |
| Acquire and present synchronization remains backend-owned. | Fulfilled | Vulkan swapchain binary semaphores are bridged into graphics timeline submissions; presentation is serialized by the graphics queue owner. D3D12 performs the final compute/copy-to-graphics join before swapchain presentation. |
| Queue state is inspectable without exposing synchronization policy to Renderer features. | Fulfilled | Native queue/timeline objects carry logical-role debug names; Vulkan reports exact family/index topology and alias capability; submitted tokens and completion are queryable through the backend-neutral submission service. |

## Final decision

Priority 2A is complete and closed. Future streaming or compute features must declare work through the existing Renderer queue preference and frame-graph dependency model, while RHI continues to own physical queues, native synchronization, presentation bridging, and retirement. Such features extend this foundation; they do not require another scheduler, queue manager, lifetime manager, or reopening of this change list.
