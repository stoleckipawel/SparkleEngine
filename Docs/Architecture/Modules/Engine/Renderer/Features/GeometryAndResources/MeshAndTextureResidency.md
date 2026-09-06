# Renderer Mesh and Texture Residency

Status: current feature dossier; source-backed, not streaming-throughput, memory-pressure, or release evidence

Verified: 2026-09-06 through committed `master` revision `c28b33bd`; current mesh, texture, residency, diagnostic, and submission owners inspected; executable source is unchanged from the earlier `8414b5dc` audit

Scope: `REN-SCENE-08`, `REN-SCENE-09`, and the residency mechanics observed by `REN-DIAG-02` through `REN-DIAG-04`; owns asynchronous mesh/texture admission, CPU preparation/decode, upload, activation, generation replacement, cancellation, and completion-safe eviction

## Feature Contract

Residency converts immutable asset generations into GPU resources without publishing a resource before its upload completes or destroying it before its last use completes. `GpuMeshCache` and `TextureCache` each own an `AssetResidency` state machine. Scene/view preparation consumes only the cache's active generation; diagnostics observe the same state and counters rather than creating another authority.

```text
asset key + generation + wanted set
  -> Reading -> Decoding -> ReadyForUpload -> Uploading
  -> completion token -> Resident -> active cache binding
  -> no longer wanted -> Evicting -> completion -> Retired
```

Cancellation before upload can retire immediately. Cancellation during upload changes the generation to Evicting and waits for recorded completion. A replaced or stale completion must never displace a newer active generation.

The benefit is bounded background work with generation-safe publication: frame preparation can request assets without owning I/O or upload lifetime, while diagnostics can distinguish backlog, decoded memory, pending upload bytes, resident memory, eviction, and retirement instead of reducing every miss to a generic unavailable resource.

## Shared Residency State And Budgets

| Contract | Current default | Meaning and limit |
| --- | --- | --- |
| request backlog | 256 generations | admission count across non-retired work for that cache-owned `AssetResidency` instance |
| decoded CPU bytes | 512 MiB | bytes held after preparation/decode and before upload completion releases CPU accounting |
| pending upload bytes | 256 MiB | upload bytes admitted concurrently |
| resident bytes | 2 GiB | accounted GPU-resident bytes for the cache-owned state machine |
| mesh preparation concurrency | 16 tasks | maximum simultaneous immutable-mesh preparation tasks |
| texture load concurrency | 16 tasks | maximum simultaneous cooked-texture read/decode tasks |

These defaults are instantiated separately by the mesh and texture caches; they are not one global Renderer memory budget. The 16-task limits bound concurrency, not total queued assets or decoded memory. Default textures follow their synchronous bootstrap path and therefore must not be assumed to be covered by the asynchronous scene-texture state machine without evidence.

## Mesh And Texture Routes

| Stage | Mesh path | Texture path | Shared invariant |
| --- | --- | --- | --- |
| identity/admission | asset ID + generation maps to stable `GpuMeshHandle`; an invalid handle is fatal, backlog exhaustion returns no handle | normalized cooked path + scene texture generation; invalid path/generation or failed residency admission is fatal | zero IDs/generations and older-than-known generations cannot enter as current |
| CPU work | background `GpuMeshPreparation::Build` produces decoded/resident sizes and upload payload | background cooked file read plus strict header/mip/layout decode | task result and payload must both be valid before ReadyForUpload |
| upload | `GpuMesh::Upload` records buffers/BLAS work | `RendererTextureFactory::Create` records texture upload and returns native resource identity | upload cannot start without pending/resident capacity |
| activation | upload submission token completes, then handle/cache maps publish the mesh | token completes, then path/generation becomes active and binding revision increments | Uploaded is not Resident until completion is observed |
| replacement | keyed generation prevents older mesh request from becoming active | newer texture generation replaces active path; older completion is released | latest wanted generation is authoritative |
| retirement | retain-set removal captures last submitted queue state and evicts | wanted-set/binding revision queues replacement or removal; last-use queue state gates release | resources remain alive across every queue that can still reference them |

## Failure Policy And Current Risks

- Mesh backlog exhaustion returns an invalid/empty handle, while several mesh preparation/state/capacity failures are fatal. Texture admission, decode, upload, or invalid scene-generation failures are fatal in the inspected path. This asymmetry needs an intentional product policy before heavy-content release.
- Missing/not-yet-resident material textures resolve through semantic defaults where the consuming path requests them; the dossier does not claim that every absent mesh/texture has a visually acceptable placeholder.
- Cooked textures reject bad magic, zero dimensions/mips/array size, unknown format/intent, unsupported dimension, wrong cubemap face count, invalid mip pitches/sizes, truncated data, trailing bytes, and invalid upload layout.
- Resident accounting depends on valid upload tokens. A completion stall delays activation/eviction and can retain requests/resources; no timeout, priority, LRU, or pressure-driven eviction policy was found.
- Budgets are fixed defaults in the inspected owner. No public setting, workload-derived tuning, global cross-cache arbitration, or graceful quality degradation was found.
- Texture binding revision prevents native resources from being released before consumers stop using the old table, but revision churn and long-session retained high-water remain unproved.

## Horizontal Coverage

| Axis | Required cells | Required observation |
| --- | --- | --- |
| asset kind | static/skinned/morphed mesh; 2D/cube/default/scene texture; each supported format/mip shape | correct sizes, generation, upload product, and semantic default behavior |
| lifecycle | request, duplicate request, queued, cancel before work, cancel during work/upload, activate, replace, unload, scene reset, shutdown | one state transition sequence and completion-safe reclamation |
| pressure | exact and over backlog/decoded/upload/resident limits; more than 16 jobs; delayed queue completion | bounded counters, explicit refusal/failure, no counter underflow or unbounded retention |
| concurrency | serial/threaded Renderer, background task success/failure/cancellation | no partial payload publication or stale completion activation |
| backend | D3D12/Vulkan uploads and all used queues | same semantic generation/result; backend allocation and token mechanics remain RHI-owned |
| observability | mesh/texture/memory snapshots during every state | snapshot identity/count/bytes match cache and native evidence within declared timing |

## Acceptance Criteria

- `AC-RES-01` — every admitted mesh/texture generation follows only legal state transitions and becomes resolvable as current only after a valid upload token completes.
- `AC-RES-02` — duplicate requests reuse the intended active/pending identity, newer generations win, and stale completions are discarded/released without replacing current state.
- `AC-RES-03` — backlog, decoded, pending-upload, and resident counters accept exact capacity, reject the first excess without overflow/underflow, and return to baseline after cancellation, eviction, reset, and shutdown.
- `AC-RES-04` — more than 16 requested meshes/textures remain bounded to 16 active CPU tasks per cache while total decoded and resident bytes remain independently bounded.
- `AC-RES-05` — decode/task/upload/token failures produce the declared fatal/refusal/placeholder result and publish no partial or falsely resident resource.
- `AC-RES-06` — last-use state across graphics/compute/copy prevents early destruction; delayed completion cannot cause use-after-free or stale binding reuse.
- `AC-RES-07` — the residency owner exposes one authoritative attributable generation/state/count/byte snapshot input consistent with cache and RHI state; diagnostic presentation observes it without recalculating residency authority.
- `AC-RES-08` — representative D3D12 and Vulkan uploads preserve asset semantics, and shutdown settles background work and GPU retirement exactly once.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-RES-01` | request/backlog or any byte budget exceeds capacity | refuse/fail at the documented boundary; counters and prior residents remain valid | `CHK-RES-02` |
| `FM-RES-02` | task launch, read, decode, payload, upload, or token failure | no active resource or binding revision claims success; failure identifies asset and stage | `CHK-RES-03` |
| `FM-RES-03` | cancel or replace while CPU/GPU work is in flight | completion is ignored/released for stale generation and current generation remains authoritative | `CHK-RES-01`, `CHK-RES-03` |
| `FM-RES-04` | one queue's completion is delayed through unload/reset/shutdown | resource remains alive until all last-use tokens complete; retained state stays measurable and bounded | `CHK-RES-04` |
| `FM-RES-05` | malformed cooked texture header/mip/layout/payload | strict decoder rejects it before upload with no partially active texture | `CHK-RES-03` |

| Check | Cheapest claim-falsifying exercise | Covers |
| --- | --- | --- |
| `CHK-RES-01` | focused state-machine harness for duplicate/newer/older generations, cancellation at every state, activation, replacement, and retirement | `AC-RES-01`, `AC-RES-02`; `FM-RES-03` |
| `CHK-RES-02` | exact-boundary and boundary-plus-one fixtures for all four budgets plus 17/256/257 request sequences; inspect counters after drain | `AC-RES-03`, `AC-RES-04`; `FM-RES-01` |
| `CHK-RES-03` | inject task/read/decode/upload/token failures and malformed cooked textures; compare active handles, binding revision, defaults, diagnostics, and cleanup | `AC-RES-05`; `FM-RES-02`, `FM-RES-03`, `FM-RES-05` |
| `CHK-RES-04` | paired-backend stream/unload/reset/shutdown run with delayed graphics/compute/copy completion and native validation; reconcile the owner snapshot with cache/RHI memory and retained high-water | `AC-RES-06`–`AC-RES-08`; `FM-RES-04` |

This contract is **defined but unproved**. The source state machine and fixed limits do not establish streaming smoothness, useful memory-pressure behavior, budget adequacy, or release-safe fallback.

## Primary Source Routes

- [`AssetResidency.h`](../../../../../../../Engine/Renderer/Private/Resources/Residency/AssetResidency.h) and [`AssetResidency.cpp`](../../../../../../../Engine/Renderer/Private/Resources/Residency/AssetResidency.cpp)
- [`GpuMeshCache.h`](../../../../../../../Engine/Renderer/Private/Meshes/GpuMeshCache.h), [`GpuMeshCache.cpp`](../../../../../../../Engine/Renderer/Private/Meshes/GpuMeshCache.cpp), and [`GpuMeshCacheResidency.cpp`](../../../../../../../Engine/Renderer/Private/Meshes/GpuMeshCacheResidency.cpp)
- [`TextureCache.h`](../../../../../../../Engine/Renderer/Private/Textures/TextureCache.h), [`TextureCacheRequests.cpp`](../../../../../../../Engine/Renderer/Private/Textures/TextureCacheRequests.cpp), and [`TextureCacheResidency.cpp`](../../../../../../../Engine/Renderer/Private/Textures/TextureCacheResidency.cpp)
- [`CookedTextureLoader.cpp`](../../../../../../../Engine/Renderer/Private/Textures/CookedTextureLoader.cpp)
- [`Renderer.h`](../../../../../../../Engine/Renderer/Public/Renderer.h)
