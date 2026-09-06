# RHI Backend Selection and Device Capabilities

Status: current feature dossier; source-backed, not build, device, parity, or release evidence

Verified: 2026-09-06 at committed `master` revision `8414b5dc`

Scope: `RHI-BACK-*` and `RHI-DEV-*`; compiled backend availability, request/default selection, adapter/device bootstrap, queue topology, and neutral capability reporting

## Feature Promise

A valid request selects one compiled backend, creates one device/adapter topology, and reports capabilities from that active device. Unavailable backends and unsupported requirements fail before Renderer schedules work; backend names are never a substitute for capability queries.

## Ownership And Current Boundary

- `RhiBackendSelection` owns parsing and availability; `RenderDeviceServices` owns the neutral service facade and active capability snapshot.
- CMake owns whether D3D12 and Vulkan targets exist and which backend is the default. Runtime cannot activate a backend omitted from the build.
- D3D12 requests feature level 12_1; Vulkan selection requires Vulkan 1.3 and scores eligible physical devices.
- The capability record owns API identity, shader target, queue kinds/independence, descriptor indexing, ray tracing, presentation, format/use support, and optional external-feature readiness.
- Renderer may select policy from neutral fields only. Adapter/vendor-specific branches remain backend/private or provider-owned.

## Acceptance Criteria

- `AC-RHI-BACK-01` — every compiled/default/requested combination either creates exactly the requested backend or rejects it with the unavailable prerequisite; no different backend silently activates.
- `AC-RHI-BACK-02` — capability and queue reports match the selected native device and remain stable for its lifetime across both backends.
- `AC-RHI-BACK-03` — missing SDK, API version, feature, queue, adapter, or device creation fails before partial RHI publication and names the rejecting boundary.
- `AC-RHI-BACK-04` — Renderer decisions use neutral capability fields, while optional vendor/provider capability is reported separately from core backend support.

## Controlled Failures And Checks

| Failure | Safe result | Check |
| --- | --- | --- |
| `FM-RHI-BACK-01` unavailable or uncompiled backend requested | device services remain unpublished; exact availability error is returned | `CHK-RHI-BACK-01` configure/build/runtime selection matrix |
| `FM-RHI-BACK-02` required adapter feature or queue missing | candidate adapter is rejected without partially active services | `CHK-RHI-BACK-02` capability/queue fault matrix |
| `FM-RHI-BACK-03` capability report differs from native query | startup validation fails and no Renderer feature consumes the false claim | `CHK-RHI-BACK-02` native query correlation on named adapters |

Check coverage: `CHK-RHI-BACK-01` covers `AC-RHI-BACK-01`, `AC-RHI-BACK-03`, and `FM-RHI-BACK-01`; `CHK-RHI-BACK-02` covers `AC-RHI-BACK-02` through `AC-RHI-BACK-04`, `FM-RHI-BACK-02`, and `FM-RHI-BACK-03`.

Definition of done: both backend target shapes, runtime selection/failure cases, native capability correlation, and candidate evidence pass; source inspection establishes none of those results.

## Primary Source Routes

- `Engine/RHI/CMakeLists.txt`
- `Engine/RHI/Public/Core`, `Engine/RHI/Public/Device`, and `Engine/RHI/Public/Commands/RhiQueueCapabilities.h`
- `Engine/RHI/Private/Device`, `Engine/RHI/Private/D3D12/Device`, and `Engine/RHI/Private/Vulkan/Device`
