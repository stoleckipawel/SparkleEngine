# Shared Tool Support Capability Inventory

**Status:** capability snapshot; current, but not tool-output compatibility or integration evidence

**Snapshot:** 2026-09-28 at committed `master` revision `452c1f32`; `Tools/Support/ToolConsoleSupport`, its CMake membership, and current Launcher, AssetCooker, TextureCooker, and ShaderCompiler consumers inspected; evidence `S` only

**Scope:** the shared host-tool console formatting boundary used by current command-line content and shader tools

**Owner:** `Tools/Support/ToolConsoleSupport` / `ToolConsoleSupport`

**Evidence and disposition:** [Capability Evidence Plan](../../CapabilityEvidencePlan.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

**Current readiness:** **50/100** — shared console products have real tool consumers; script, Unicode/path, progress, failure, and compatibility evidence remains open. See [Current Feature Readiness](../../../../Acceptance/CurrentReadiness.md#foundation-world-content-shaders-and-tools).

## At A Glance

| Facility | Current result | Deliberate non-claim |
| --- | --- | --- |
| severity lines | readable `[LOG]`, `[WARN]`, and `[ERROR]` prefixes to chosen streams | not a machine-readable diagnostic event schema |
| named/path fields | ordered `name=value` display with optional quoting | does not escape every quote/newline, normalize paths, or redact secrets |
| progress | flushed phase records with optional authoritative completed/total work, plus human item records | no rate/remaining-time estimate, cancellation, or inferred percentage |
| summaries/lists | consistent human-oriented multi-line output | no versioned parser-compatibility contract |
| consumer integration | current cookers and ShaderCompiler share formatting | exit status and expected artifacts—not text—remain success authority |

The library standardizes presentation and one deliberately narrow cross-process progress record. Keeping process control and result semantics in each tool avoids a hidden orchestration layer. Only work-progress records are parser-facing; severity, field, list, summary, and item-progress text remain human output and must not be parsed as an API.

## Module Boundary

`ToolConsoleSupport` is a C++20 static host-tool library. CMake includes it only when host tools are enabled and excludes it from default game-profile builds. AssetCooker, TextureCooker, ShaderCompiler, and Launcher host-tool code link it; runtime and Editor logging remain separate Core/Application facilities.

## Capability Surface

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `TOOL-001` | Severity-prefixed messages | Implemented path | Writes `[LOG]`, `[WARN]`, or `[ERROR]` plus a message to a caller-selected stream; convenience Info/Warning use stdout and Error uses stderr. Severity is presentation text, not a machine-readable event contract. | `S` |
| `TOOL-002` | Named fields | Implemented path | Appends ordered `name=value` fields with raw or single-quoted values. `PathField` uses the filesystem path string and quoted presentation. No escaping of embedded quotes or line breaks is performed. | `S` |
| `TOOL-003` | Progress records | Implemented path | `ToolWorkProgressProtocol` emits and parses one flushed `[PROGRESS]` record containing a phase plus either exact completed/total work or an explicit unknown total. `ToolWorkProgressWriter` serializes concurrent producers and bounds repeated output to phase or integer-percentage changes. The older named-item formatter remains human-only. There is no rate estimate or cancellation channel. | `S` |
| `TOOL-004` | Summaries and lists | Implemented path | Prints titled multi-line summaries plus indexed list headers/items. Layout is intended for readable CLI diagnostics; no schema/version guarantees parser compatibility. | `S` |
| `TOOL-005` | Path display helpers | Implemented path | Produces a filename for compact display when present and falls back to the whole path. It does not normalize, validate, redact, relativize, or resolve paths. | `S` |
| `TOOL-006` | Current consumers | Implemented path | AssetCooker reports stage and scene progress; TextureCooker reports request and publication progress; ShaderCompiler reports planning, compile, verification, and publication progress. Launcher-owned build, dependency, level, cook, and run executors report exact plan-step progress and preserve nested tool progress. | `S` |
| `TOOL-007` | Game-profile isolation | Implemented path | Host-tool target configuration excludes the library from default game configurations, and no Engine/product target links it in the inspected build graph. Final binary/package absence remains unproven. | `S` |

## Vertical Tool-Diagnostic Trace

Cooker/compiler or Launcher plan owns a phase and its real work count -> `ToolWorkProgressWriter` suppresses redundant percentage updates and emits a flushed progress record -> Launcher captures the stream, parses only that shared record, and updates the activity phase/bar -> ordinary output remains visible in the run log -> operation-specific code owns exit status and success/failure semantics. `ToolConsoleSupport` does not own process launch, log retention, cancellation, or result classification.

## Progress Contract

- The producer that owns the work owns the phase name and count. A receiving feature does not estimate work from elapsed time, output volume, filenames, or child CPU activity.
- `total=0` means indeterminate. For determinate progress, `total` is immutable and `completed` is monotonic within one phase, with `0 <= completed <= total`. A new phase may establish a new denominator, so percentages are phase-local rather than a fabricated end-to-end estimate.
- `ToolWorkProgressWriter` is the producer-side serialization and throttling boundary. It is safe for concurrent workers, flushes every accepted record, and emits at most one update per integer percentage unless the phase changes.
- Launcher decodes raw byte chunks at the process-output boundary before converting ordinary output for Qt presentation. The decoder reconstructs split lines, bounds incomplete input, rejects malformed or regressing records, and publishes a typed progress signal. Widgets never parse console text.
- Progress never determines success. Completion, failure, cancellation, exit status, and expected products remain owned by the operation result contract. Raw records remain in the operation log for diagnosis.

## Explicit Non-Capabilities And Risks

- No general JSON/event stream, diagnostic code taxonomy, localization, terminal color, timestamp, thread/process identity, telemetry, rate, or remaining-time protocol exists. The progress record is intentionally limited to phase and optional completed/total work.
- Quoted values are not escaped; a value containing a quote or newline can make output ambiguous to humans or ad hoc parsers.
- Progress is observational only. Launcher must not infer operation success from it; process state, exit code, and expected artifacts remain authoritative.
- Source inspection does not prove stdout/stderr ordering, Unicode/path rendering, pipe behavior, or the final absence of this library from game products.
