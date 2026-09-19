# Renderer Settings State and Persistence

**Status:** current feature dossier; source-backed, not round-trip, package, threaded-equivalence, or release evidence

**Verified:** source route re-audited 2026-09-19 against revision `d1108d44de49d90313abb43a66f924f89d6c6bb2` plus the current ownership-cleanup working tree; executable build/runtime evidence remains unrun

**Scope:** `REN-SET-01` through `REN-SET-05`; owns the lifecycle of the aggregate rendering-settings state, editor commit, workspace persistence, startup restore, render-thread handoff, live versus restart-required application, and requested-state limitations. [Feature Selector Catalog](FeatureSelectorCatalog.md) remains the exact per-selector ledger.

**Current readiness:** **40/100** — settings edit/load/save/handoff is reachable; durable error reporting, malformed/concurrent/package storage, queue pressure, and structured requested-versus-active state remain incomplete. See [Current Feature Readiness](../../../../../../Acceptance/CurrentReadiness.md#renderer).

## At A Glance

| Question | Current answer |
| --- | --- |
| what is edited? | one aggregate `EngineRenderingSettingsState`; 26 fields describe requested rendering configuration |
| what is persisted? | 26 allowlisted `r.*` values in the workspace `Config/DefaultEngine.ini`; view mode is owned separately by each viewport session |
| how does it reach rendering? | startup applies the section before command-line overrides; editor commits go through the Renderer facade and serial or render-thread control path |
| what becomes active immediately? | only settings whose feature owner can apply them without recreation; adapter and back-buffer format changes require restart |
| what is missing? | atomic/user-scoped packaged persistence, surfaced parse/write errors, and a structured requested-versus-active/fallback status model |

The aggregate exists to make an editor commit coherent, not to centralize feature policy. Each feature still resolves capability, fallback, topology, and active state at its own owner.

## Feature Contract

The settings route coordinates three owners without merging them. Renderer publicly owns only `EngineRenderingSettingsState`, captures/applies that rendering state, and accepts it through its sequenced control boundary. Editor privately owns `EngineRenderingSettingsSection`, the mutable interaction model and restart message. Application owns `EngineRenderingSettingsPersistence`, startup restore order, filesystem location, and save-before-submit policy. Serial execution applies CVars directly, while threaded execution queues a `RenderSettingsChangedCommand` for the render execution context.

```text
application startup
  -> load owned INI section -> set registered CVars

editor setter
  -> mutate Editor-owned aggregate interaction state
  -> Application callback -> rewrite owned INI section
  -> Renderer facade -> coordinator
  -> serial apply or render-thread control command -> set changed CVars
  -> feature owners resolve active per-frame/topology behavior
```

The motivation is one coherent editor transaction and deterministic render-thread ownership. It does not make every console selector an editor setting, and a requested CVar value is not automatically the active capability result.

## State, Persistence, And Reachability

| Surface | Current coverage | Boundary |
| --- | --- | --- |
| aggregate public state | 26 fields: presentation/device, tone/output, exposure, upscale/RR, GBuffer/RT, batching, and TLAS/PTLAS | value snapshot; it does not contain per-viewport UI state or active provider/capability/fallback reasons |
| persisted allowlist | 26 exact `r.*` names in `/Script/SparkleRenderer.EngineRenderingSettings` | View mode is deliberately not Renderer settings; `RenderViewMode` is one non-persisted per-view request value, while Editor owns its menu presentation and interaction |
| persistence file | workspace `Config/DefaultEngine.ini`; writer replaces its one section and retains other loaded lines/sections | not an atomic temp-and-replace write; error/status is not returned |
| startup | `Application` applies persisted settings before command-line CVar overrides | malformed values are currently attempted and their error text is discarded |
| editor commit | each changed setter invokes the Application-owned callback; Application persists and submits the state | Editor has no filesystem access; whole snapshot is resent and unchanged fields are skipped by Renderer CVar comparison |
| threaded Renderer | control queue transfers the snapshot to render execution context | ordering/backpressure and exit behavior need executable evidence |
| restart-required fields | adapter preference and back-buffer format are compared with session-start values and produce a restart message | values are still persisted/submitted; the message, not an active-state object, expresses pending restart |
| routed controls outside state | direct-shadow controls plus RHI back-buffer count/maximum frames in flight and other developer CVars | console/catalog-owned; absence from this state must not be presented as UI support |

## Persistence Semantics And Known Gaps

The loader ignores missing files, comments, blank lines, other sections, unknown names, and registered-name lookup failures. It trims key/value strings. Valid allowlisted values are parsed by the owning CVar. Invalid parse diagnostics are presently discarded. The writer creates the parent directory best-effort, reads the existing file, removes the first matching owned section, inserts a freshly generated 26-value section, and truncates/re-writes the file.

Consequences that must remain explicit:

- no status reaches the caller for directory creation, open, write, flush, or truncation failure;
- concurrent edits can be overwritten between read and truncate;
- malformed/unknown/duplicate section/value behavior has no user-facing diagnostic contract;
- workspace-root persistence does not establish a writable packaged-user configuration route;
- no schema/version/migration compatibility path exists or is intended under the current clean-break policy;
- requested state, CVar state, resolved graph/provider state, and restart-active state are not one thing and must be reported separately where a feature can reject, clamp, defer, or fall back.

## Ownership Rules

- This dossier owns the cross-module aggregate state route. Each feature dossier owns value meaning, clamps, graph/history impact, capability fallback, and output evidence.
- [Feature Selector Catalog](FeatureSelectorCatalog.md) owns exact names, defaults, domains, persistence membership, consumers, and absent/ineffective controls.
- Application owns persistence, filesystem location, startup ordering, and save-before-submit policy; Editor owns the private interaction model and restart presentation; Renderer owns the value contract, capture/apply operation, cross-thread delivery, and rendering consumers. CVar/feature owners remain the active runtime authority.
- Adding a setting requires the state field, setter/UI, CVar/parser, persistence decision, runtime consumer, requested/active report, topology/history/restart behavior, and feature dossier to change together.

## Horizontal Coverage

| Axis | Required cells | Invariant |
| --- | --- | --- |
| execution | direct/no callback, serial Renderer callback, threaded Renderer callback, queue pressure, shutdown | committed state reaches the owning CVar once in order or fails visibly |
| persistence | missing/existing file, unrelated sections/comments, first/last owned section, malformed/unknown/duplicate values, read-only/full/concurrently edited file | unrelated data is preserved and failure never masquerades as saved state |
| lifecycle | startup, command-line override, live edit, topology/history change, pending restart, restart, package run | requested, CVar, resolved active, and session-active state remain distinguishable |
| surface | editor/public state, console-only Renderer, RHI-only controls, absent features | no hidden or orphan selector is inferred as supported UI capability |
| configuration | Debug/Development/Shipping, editor/runtime, workspace/package, D3D12/Vulkan | reachability and writable location are explicitly classified |

## Acceptance Criteria

- `AC-SET-01` — public state, editor controls, setters, CVar capture/apply, persistence allowlist, selector catalog, and feature consumers agree field-for-field with intentional exclusions named.
- `AC-SET-02` — a valid 26-name section round-trips exactly while comments and unrelated sections remain unchanged according to the declared formatting policy; per-viewport view mode remains outside Application rendering-settings persistence.
- `AC-SET-03` — malformed, unknown, duplicate, unreadable, unwritable, partial-write, and concurrent-edit cases return an actionable result and preserve a valid prior file/state rather than silently succeeding.
- `AC-SET-04` — startup persisted values apply before command-line overrides; serial and threaded commits produce equivalent ordered CVar and next-frame resolved states.
- `AC-SET-05` — live, topology/history-affecting, capability-gated, and restart-required changes expose requested/CVar/resolved/session-active state and reason without false activation.
- `AC-SET-06` — editor/runtime and workspace/package reachability plus writable configuration location are classified for every supported build/backend cell.
- `AC-SET-07` — adding/removing a selector leaves no orphan state field, setter, persisted name, UI control, CVar, consumer, or stale compatibility alias.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-SET-01` | malformed/unknown/duplicate key or value | actionable diagnostic; unrelated and last valid state follow declared policy | `CHK-SET-02` |
| `FM-SET-02` | directory/open/write/flush failure, partial disk, or concurrent edit | commit reports failure and valid prior file is preserved atomically | `CHK-SET-02` |
| `FM-SET-03` | render control queue pressure/reorder or shutdown with pending settings | last admitted state applies in declared order or reports rejection; no post-shutdown mutation | `CHK-SET-03` |
| `FM-SET-04` | unsupported/clamped/fallback/restart-required request | active state and reason differ visibly from request until genuinely active | `CHK-SET-03` |
| `FM-SET-05` | field/name/UI/consumer added or removed on only one surface | mechanical inventory detects mismatch and scope freeze fails | `CHK-SET-01` |

| Check | Cheapest claim-falsifying exercise | Covers |
| --- | --- | --- |
| `CHK-SET-01` | mechanically enumerate public fields/setters, editor controls, persisted names, Renderer/RHI CVars, consumers, and catalog rows; compare one-to-one decisions | `AC-SET-01`, `AC-SET-07`; `FM-SET-05` |
| `CHK-SET-02` | isolated-config round trip with valid/boundary/malformed/unknown/duplicate values, unrelated content, read-only/full/partial/concurrent-write faults, and package path | `AC-SET-02`, `AC-SET-03`, `AC-SET-06`; `FM-SET-01`, `FM-SET-02` |
| `CHK-SET-03` | replay identical edit sequence through direct, serial, and threaded routes including pressure/shutdown, topology/history/provider fallback, and restart; compare ordered requested/CVar/resolved/session state | `AC-SET-04`, `AC-SET-05`; `FM-SET-03`, `FM-SET-04` |

This contract is **defined but unproved**. The current implementation does not yet satisfy the documented durable-write/error-reporting criteria; those are explicit gaps, not claimed behavior.

## Primary Source Routes

- [`EngineRenderingSettings.h`](../../../../../../../Engine/Renderer/Public/Settings/EngineRenderingSettings.h)
- [`EngineRenderingSettingsSection.cpp`](../../../../../../../Engine/Editor/Private/Settings/EngineRenderingSettingsSection.cpp)
- [`EngineRenderingSettingsPersistence.cpp`](../../../../../../../Engine/Application/Private/RenderingSettings/EngineRenderingSettingsPersistence.cpp)
- [`EngineRenderingSettingsRuntime.cpp`](../../../../../../../Engine/Renderer/Private/Settings/EngineRenderingSettingsRuntime.cpp)
- [`RenderCoordinator.cpp`](../../../../../../../Engine/Renderer/Private/Concurrency/Coordinator/RenderCoordinator.cpp) and [`RendererExecutionContext.cpp`](../../../../../../../Engine/Renderer/Private/Concurrency/Coordinator/RendererExecutionContext.cpp)
- [`Application.cpp`](../../../../../../../Engine/Application/Private/Application.cpp) and [`UI.cpp`](../../../../../../../Engine/Editor/Private/UI.cpp)
