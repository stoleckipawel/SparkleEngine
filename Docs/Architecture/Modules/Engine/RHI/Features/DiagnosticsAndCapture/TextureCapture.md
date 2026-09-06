# RHI Texture Capture

Status: current feature dossier; source-backed, not captured-pixel, format, color, latency, or release evidence

Verified: 2026-09-06 at committed `master` revision `8414b5dc`

Scope: `RHI-DIAG-06`; asynchronous texture readback, staging lifetime, supported-format conversion, bitmap writing, polling, result delivery, failure, and cleanup

## Feature Promise

A request for a supported neutral texture/subresource becomes one asynchronous readback result with explicit dimensions, row layout, format, identity, and terminal status. An empty buffer, stale prior image, unsupported conversion, or merely submitted copy is never capture success.

## Ownership And Lifetime

- Renderer selects the semantic render product and owns its color/encoding provenance; RHI capture owns native copy/readback, staging storage, polling, format mapping, and byte/image result.
- The active backend transitions/copies the exact source, retains it and staging memory through queue completion, then maps and converts only after completion.
- Common capture-format and bitmap code defines shared byte/layout behavior. Backend code retains only native resource/copy details.
- Cancellation, backend failure, map/encode failure, and shutdown have one terminal result and completion-safe cleanup.

## Acceptance Criteria

- `AC-RHI-CAP-01` — capture preserves requested resource/subresource, extent, row pitch, format, frame/generation identity, and asynchronous state progression on both backends.
- `AC-RHI-CAP-02` — canonical pixel patterns decode to the documented channel values and orientation for every supported capture format.
- `AC-RHI-CAP-03` — source and staging resources remain alive until copy completion and are reclaimed after result delivery, failure, cancellation, or shutdown.
- `AC-RHI-CAP-04` — unsupported format/state, allocation/copy/map/encode failure, queue failure, and stale identity produce one explicit Failed result without stale or empty success.
- `AC-RHI-CAP-05` — queue depth, latency, memory, and observer cost are bounded and reported for the evidence configuration.

## Controlled Failures And Checks

| Failure | Safe result | Check |
| --- | --- | --- |
| `FM-RHI-CAP-01` unsupported format/state or stale request | reject before copy or complete Failed with exact reason | `CHK-RHI-CAP-01` format/state/identity matrix |
| `FM-RHI-CAP-02` readback allocation/copy/map/encode failure | terminal Failed; all owned resources retire | `CHK-RHI-CAP-02` injected stage failures |
| `FM-RHI-CAP-03` cancellation/shutdown with capture in flight | no callback/use after owner teardown; cleanup waits for completion | `CHK-RHI-CAP-03` in-flight lifecycle stress |

Check coverage: `CHK-RHI-CAP-01` covers `AC-RHI-CAP-01`, `AC-RHI-CAP-02`, `AC-RHI-CAP-04`, and `FM-RHI-CAP-01`; `CHK-RHI-CAP-02` covers `AC-RHI-CAP-03`, `AC-RHI-CAP-04`, and `FM-RHI-CAP-02`; `CHK-RHI-CAP-03` covers `AC-RHI-CAP-03`, `AC-RHI-CAP-05`, and `FM-RHI-CAP-03`.

Definition of done: pattern decoding, all supported formats, fault injection, lifecycle/queue bounds, memory/cost, resize/shutdown, native validation, and both-backend evidence pass.

## Primary Source Routes

- `Engine/RHI/Public/Capture/RhiCaptureService.h`
- `Engine/RHI/Private/Capture` and backend `Capture` implementations
- [Renderer Diagnostics, Products, and Capture](../../../Renderer/Features/ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md)
