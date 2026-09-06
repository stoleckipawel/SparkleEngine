# Shared Tool Support Capability Inventory

Status: capability snapshot; current, but not tool-output compatibility or integration evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; `Tools/Support/ToolConsoleSupport`, its CMake membership, and current AssetCooker, TextureCooker, and ShaderCompiler consumers inspected; evidence `S` only

Scope: the shared host-tool console formatting boundary used by current command-line content and shader tools

Owner: `Tools/Support/ToolConsoleSupport` / `ToolConsoleSupport`

Evidence and disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

## Module Boundary

`ToolConsoleSupport` is a C++20 static host-tool library. CMake includes it only when the content pipeline or ShaderCompiler is enabled and excludes it from default game-profile builds. AssetCooker, TextureCooker, and ShaderCompiler link it; runtime and Editor logging remain separate Core/Application facilities.

## Capability Surface

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `TOOL-001` | Severity-prefixed messages | Implemented path | Writes `[LOG]`, `[WARN]`, or `[ERROR]` plus a message to a caller-selected stream; convenience Info/Warning use stdout and Error uses stderr. Severity is presentation text, not a machine-readable event contract. | `S` |
| `TOOL-002` | Named fields | Implemented path | Appends ordered `name=value` fields with raw or single-quoted values. `PathField` uses the filesystem path string and quoted presentation. No escaping of embedded quotes or line breaks is performed. | `S` |
| `TOOL-003` | Progress records | Implemented path | Prints action, asset type, one-based caller-supplied index/total, name, and optional fields as one line. It has no terminal cursor control, update-in-place protocol, rate estimate, or cancellation channel. | `S` |
| `TOOL-004` | Summaries and lists | Implemented path | Prints titled multi-line summaries plus indexed list headers/items. Layout is intended for readable CLI diagnostics; no schema/version guarantees parser compatibility. | `S` |
| `TOOL-005` | Path display helpers | Implemented path | Produces a filename for compact display when present and falls back to the whole path. It does not normalize, validate, redact, relativize, or resolve paths. | `S` |
| `TOOL-006` | Current consumers | Implemented path | AssetCooker reports diagnostics, stage progress, child-process failure, and product counts; TextureCooker reports request/publish/item results; ShaderCompiler reports cook validation, counts, targets, and analysis artifacts. Other tools do not automatically receive this formatting. | `S` |
| `TOOL-007` | Game-profile isolation | Implemented path | Host-tool target configuration excludes the library from default game configurations, and no Engine/product target links it in the inspected build graph. Final binary/package absence remains unproven. | `S` |

## Vertical Tool-Diagnostic Trace

Cooker/compiler operation selects an output stream and severity -> the producer constructs raw, quoted, or path fields -> `ToolConsole` emits the stable human-oriented prefix/layout -> Launcher or a terminal captures the child process streams -> operation-specific code owns exit status and success/failure semantics. `ToolConsoleSupport` does not own process launch, log retention, cancellation, or result classification.

## Explicit Non-Capabilities And Risks

- No JSON/event stream, diagnostic code taxonomy, localization, terminal color, timestamp, thread/process identity, telemetry, or structured progress protocol was found.
- Quoted values are not escaped; a value containing a quote or newline can make output ambiguous to humans or ad hoc parsers.
- The format has no version contract. Launcher must not infer operation success solely from console text; process state, exit code, and expected artifacts remain authoritative.
- Source inspection does not prove stdout/stderr ordering, Unicode/path rendering, pipe behavior, or the final absence of this library from game products.
