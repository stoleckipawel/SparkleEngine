# SparkleEngine Architecture Boundary Rules

This document explains the executable architecture checks in [ArchitectureBoundaryCheck.cmake](/C:/Users/stole/Documents/GitHub/SparkleEngine/CMake/ArchitectureBoundaryCheck.cmake:1). The CMake check is the enforcement layer; this document is the human-readable contract that explains why the rules exist, what they prevent, and how to respond when they fail.

## Purpose

The boundary check exists to keep SparkleEngine reviewable and hard to break as the renderer, RHI, tooling, and provider integrations grow. It protects a few architectural ideas that matter a lot for this engine:

- `RHI` owns backend-native graphics API details.
- `Renderer` consumes backend-neutral RHI contracts and runtime scene data.
- `Application` validation must not quietly grow native backend dependencies.
- Cross-backend contamination between D3D12 and Vulkan must fail fast.
- Vendor/provider exceptions must stay narrow, counted, and documented.

The check is not just style lint. It is enforcing module ownership.

## Scope

The current check scans:

- `Engine/RHI`
- `Engine/Renderer`
- `Engine/Application/Private/Validation`

It also explicitly rejects the retired broad `Tools/Conversion/AssetConverter` production path.

## Enforced Rules

### `RHI_NO_RENDERER_PRIVATE`

What it prevents:

- `RHI` including `Renderer/Private` headers.

Why it exists:

- `RHI` is below `Renderer`. If `RHI` depends on renderer-private code, the engine loses the clean boundary between backend-facing services and high-level rendering orchestration.

Dependency owner:

- `Renderer` owns renderer-private implementation details.
- `RHI` owns backend abstraction and backend-private implementation details.

How to fix a violation:

- Remove the renderer-private include from `RHI`.
- Move the shared contract downward into a backend-neutral public surface if it is truly shared.
- If the data is renderer-specific, keep it in `Renderer` and expose a backend-neutral request through the public RHI seam instead of reaching upward.

### `RENDERER_NO_BACKEND_NATIVE`

What it prevents:

- `Renderer` code depending directly on native D3D12 or Vulkan identifiers.

Why it exists:

- Renderer architecture should be expressed in backend-neutral RHI terms, not native API details.
- If native API usage leaks into general renderer code, every future backend/provider addition becomes harder to reason about and easier to break.

Dependency owner:

- `RHI` owns backend-native graphics API details.
- Renderer provider code may have tightly scoped, documented exceptions.

How to fix a violation:

- Replace native API use with backend-neutral RHI contracts.
- If the native dependency belongs to a provider bridge, move it into a provider-scoped implementation and document it through a counted exception plus ADR.
- Do not treat an existing provider exception as general permission for renderer-wide native API access.

### `RENDERER_NO_NATIVE_PTLAS`

What it prevents:

- `Renderer` using native partitioned TLAS identifiers such as Vulkan partitioned AS types, NVAPI D3D12 RTAS identifiers, or similar backend-native PTLAS symbols.

Why it exists:

- Partitioned TLAS behavior is backend-private RHI work. The renderer should operate on backend-neutral PTLAS structs and services.

Dependency owner:

- Backend-private `RHI` code owns native PTLAS details.
- `Renderer` owns backend-neutral ray tracing feature orchestration.

How to fix a violation:

- Replace native PTLAS identifiers with backend-neutral RHI PTLAS structures or service calls.
- Move backend-native handling into `Engine/RHI/Private/D3D12` or `Engine/RHI/Private/Vulkan` as appropriate.

### `D3D12_NO_VULKAN_BACKEND`

What it prevents:

- D3D12 backend code including or using Vulkan backend/native identifiers.

Why it exists:

- Backend implementations must remain separable and mentally local. Cross-backend leakage makes parity, debugging, and future backend work much harder.

Dependency owner:

- `Engine/RHI/Private/D3D12` owns D3D12 backend behavior only.

How to fix a violation:

- Remove Vulkan-native includes, symbols, or assumptions from D3D12 backend code.
- Move truly shared logic into backend-neutral RHI common code if it does not require native API identifiers.

### `VULKAN_NO_D3D12_BACKEND`

What it prevents:

- Vulkan backend code including or using D3D12 backend/native identifiers.

Why it exists:

- Same reason as `D3D12_NO_VULKAN_BACKEND`: backend implementations must stay cleanly separated.

Dependency owner:

- `Engine/RHI/Private/Vulkan` owns Vulkan backend behavior only.

How to fix a violation:

- Remove D3D12-native includes, symbols, or assumptions from Vulkan backend code.
- Move shared logic into backend-neutral RHI common code only if it remains native-API-free.

### `APPLICATION_VALIDATION_NO_BACKEND_NATIVE`

What it prevents:

- `Engine/Application/Private/Validation` growing direct D3D12 or Vulkan backend dependencies.

Why it exists:

- Application validation should validate runtime/editor behavior and smoke paths without turning into a second backend implementation layer.

Dependency owner:

- `Application` owns validation orchestration.
- `RHI` owns backend-native API details.

How to fix a violation:

- Route validation through backend-neutral RHI and renderer contracts.
- If a backend-specific capture or inspection path is needed, define a backend-neutral request surface first instead of pulling native headers into application validation.

### `NO_PARALLEL_ASSET_CONVERTER_PIPELINE`

What it prevents:

- Reintroduction of the retired `Tools/Conversion/AssetConverter` production path.

Why it exists:

- The engine is standardizing on focused `AssetCooker` inspection/debug/cook flows instead of reviving the older broad conversion path as production architecture.

Dependency owner:

- `AssetCooker` and focused cookers own the supported content pipeline path.

How to fix a violation:

- Remove new `AssetConverter` production files from the repository path.
- Use `AssetCooker` inspect/debug/cook commands or focused cooker tools instead.

## Counted Exceptions

Some exceptions are intentionally narrow and frozen by count rather than granted as open-ended permission.

Current counted exceptions:

- `RENDERER_NO_BACKEND_NATIVE: NVIDIA DLSS provider Vulkan::Vulkan link`
- `RENDERER_NO_BACKEND_NATIVE: Streamline DLSS Vulkan bridge`

What counted exceptions mean:

- The exception is only valid for a specific file path plus a specific allowed pattern.
- The check counts how many allowed matches exist.
- If the count grows above the frozen baseline, the architecture check fails.
- If the count drops below the frozen baseline, the check emits a reminder to remove or tighten the exception.

Why this matters:

- The exception is a bounded migration seam, not a policy change.
- Counted exceptions make intentional debt visible and measurable.

Current owner model:

- These exceptions are provider-scoped and tied to NVIDIA DLSS/Streamline integration.
- They are not permission for general renderer-native Vulkan or D3D12 code.

See:

- [0001-renderer-native-api-provider-exceptions.md](/C:/Users/stole/Documents/GitHub/SparkleEngine/Docs/Architecture/01-Boundaries/ADR/0001-renderer-native-api-provider-exceptions.md:1)

## How To Add A New Exception

New exceptions must not be added casually.

Required process:

1. Confirm the dependency cannot be expressed through an existing backend-neutral contract.
2. Scope the exception to the narrowest file path and pattern possible.
3. Freeze the exception with a counted baseline where practical.
4. Add or update an ADR in `Docs/Architecture/01-Boundaries/ADR/`.
5. Explain the owning module, risk, and retirement path in the ADR.
6. Keep the exception provider-scoped or backend-scoped, never renderer-wide by default.

What not to do:

- Do not treat a single provider exception as a precedent for broad renderer-native access.
- Do not add unbounded path-wide exceptions.
- Do not hide the exception in comments without updating the ADR set.

## Rule Summary Table

| Rule name | Prevents | Dependency owner |
| --- | --- | --- |
| `RHI_NO_RENDERER_PRIVATE` | `RHI` including renderer-private headers | `Renderer` private implementation remains above `RHI` |
| `RENDERER_NO_BACKEND_NATIVE` | General renderer-native D3D12/Vulkan usage | `RHI`, with narrow provider exceptions only |
| `RENDERER_NO_NATIVE_PTLAS` | Renderer-native PTLAS identifiers | Backend-private `RHI` |
| `D3D12_NO_VULKAN_BACKEND` | Vulkan identifiers in D3D12 backend code | D3D12 backend |
| `VULKAN_NO_D3D12_BACKEND` | D3D12 identifiers in Vulkan backend code | Vulkan backend |
| `APPLICATION_VALIDATION_NO_BACKEND_NATIVE` | Native backend use in application validation | `Application` validates through RHI/Renderer contracts |
| `NO_PARALLEL_ASSET_CONVERTER_PIPELINE` | Revival of retired AssetConverter production path | `AssetCooker` and focused cookers |

## Ambiguities And Planned Clarifications

- The boundary check currently documents two provider-counted exceptions, both Vulkan-oriented. If future D3D12-side provider bridging needs counted exceptions, the same ADR process should be used rather than broadening current rules implicitly.
- The check enforces file/path ownership well, but it does not yet replace the need for deeper contract docs such as `RHIContract.md` and `RendererProviderContract.md`.

