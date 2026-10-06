# Core Capability Inventory

**Status:** capability snapshot; current, but not release approval or runtime evidence

**Snapshot:** 2026-09-06 at committed `master` revision `8414b5dc`; `Engine/Core` public/private source and CMake membership inspected in the live working tree; evidence `S` only

**Scope:** dependency-free engine foundations: diagnostics, console/configuration, events, files, paths, process execution, serialization helpers, input vocabulary, math, pixels, time, and thread ownership

**Owner:** `Engine/Core` / `SparkleCore`

**Evidence and disposition:** [Capability Evidence Plan](../../CapabilityEvidencePlan.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

**Current readiness:** **50/100** — broad integrated source foundations exist; verification and delivery/adoption credit remain zero. See [Current Feature Readiness](../../../../Acceptance/CurrentReadiness.md#foundation-world-content-shaders-and-tools).

Source ownership reconciled on 2026-10-06: `CommandLine::ReadToken` owns the borrowed wide-token view and advancing offset used by Application CVar parsing and RHI backend selection. Core also owns the immutable default texture product catalog shared by Renderer and AssetCooker; cooking and runtime policy remain with those consumers.

## At A Glance

| Foundation | Current role | Deliberate boundary |
| --- | --- | --- |
| diagnostics and console | logging/fatal/verify, typed CVars, commands, bounded interactive session | product hosts decide reachability; Core does not persist settings or define feature policy |
| files, paths, processes, serialization | validated paths, transactional publication helpers, bounded byte decoding, lightweight JSON, child execution | no general asset database, schema migration system, or package manager |
| events, input vocabulary, time, ownership | synchronous typed events, platform-neutral input state, timer, thread-role assertions | not an async bus, fixed-step simulation scheduler, or automatic thread safety |
| math, pixels, hashing/string tables | shared transforms, sampling helpers, linear/sRGB conversion, deterministic identifiers | consuming schemas own units, collision policy, formats, and numerical oracles |

Core keeps dependencies pointed downward by owning only reusable mechanisms. That reduces duplication, but it also means a higher-level feature must own every semantic policy, lifetime, capacity, and user-visible failure built from those mechanisms.

## Module Boundary

`SparkleCore` is the base engine library. It has no other engine dependency; it publicly carries the header-only `spdlog` contract and links Windows `bcrypt` privately. It can be static or shared. Logging compile level is Trace in Debug, Info in Development, and Warning in Shipping profiles.

## Capability Surface

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `CORE-001` | Named diagnostics | Implemented path | Process-wide `spdlog` bootstrap, lightweight declared `LogCategory` frontend, Core-private named logger creation/lifetime, active log-file discovery, runtime level control, fatal/verify helpers, and debugger break-if-attached. Fatal paths are process-terminating policy, not recoverable errors. | `S` |
| `CORE-002` | Console-variable registry | Implemented path | Statically registered typed CVars support bool, integral, floating, enum-as-number, and string values. Lookup and string mutation report parse errors. There is no persistence layer in Core. | `S` |
| `CORE-003` | Console command/session | Implemented path | Runtime/editor command scopes, registry, argument parsing, autocomplete, bounded history, bounded 512-record session output, and built-ins for help/list/get/set. Product hosts decide which commands are reachable. | `S` |
| `CORE-004` | Event dispatch | Implemented path | Fixed-capacity typed synchronous events return handles; `ScopedEventHandle` removes a subscription on destruction. This is same-thread callback dispatch, not an asynchronous message bus. | `S` |
| `CORE-005` | Whole-file I/O | Implemented path | Binary/text reads and writes, explicit error strings, temporary paths, close/finalize helpers, atomic text write, and multi-file publication with cleanup. Atomicity/durability across crash or hostile filesystems is not runtime-proven. | `S` |
| `CORE-006` | Bounded binary decode/encode | Implemented path | Span reader checks remaining bytes and multiplication overflow; buffer/stream writers cover trivially-copyable values/arrays, raw bytes, and UInt32-length strings. These helpers do not provide schema evolution by themselves. | `S` |
| `CORE-007` | Lightweight JSON helpers | Partial | Escaped JSON writing and targeted property/string/unsigned/hex reads exist. This is not a general DOM, schema validator, or complete JSON parser. | `S` |
| `CORE-008` | Workspace/project path model | Implemented path | Resolves executable, workspace, build output, logs, configured project, Engine/Project assets, cooked asset families, shader products/symbols, and recook signal paths. Discovery uses ancestor markers and environment/workspace state; packaged independence is not proven. | `S` |
| `CORE-009` | Typed asset path resolution | Implemented path | Asset families and Engine/Project/Any roots drive normalized/validated lookup for meshes, textures, shaders, scene manifests, materials, skeletons, and animations. It is path resolution, not an asset database. | `S` |
| `CORE-010` | Child-process execution | Implemented path | Synchronous Win32 child launch with argv, working directory, environment overrides, output capture/callback, log path, cancellation token, exit code, and explicit launch/failure state. Native start failures classify missing executable/access denial/application-policy rejection without localized-text matching and expose Windows messages as UTF-8. Process-tree termination and package execution still need evidence. | `S` |
| `CORE-011` | Environment and command-line utilities | Implemented path | String/bool/UInt environment access plus command-line quoting/formatting and UTF-8/wide conversion support tool and host boundaries. | `S` |
| `CORE-012` | Project level catalog | Implemented path | Shared parser/model reads level and asset-pack metadata used by GameFramework, cooking, and Launcher: selection, family/variant, source/archive/hash/version/license, download/runtime support, and blockers. It does not download or activate content. | `S` |
| `CORE-013` | Input vocabulary/state | Implemented path | Key, mouse button, modifier, device, dispatch-layer/mode, typed events, and per-frame pressed/released/held, pointer delta, wheel, capture, and cursor state. OS acquisition/routing belongs to Platform. | `S` |
| `CORE-014` | Math and coordinate helpers | Implemented path | DirectXMath-based transforms/normalization/frustum utilities, declared world-coordinate convention, and Bessel/sinc/Kaiser functions used by filtering. This is not a general numerics library. | `S` |
| `CORE-015` | Pixel conversions | Implemented path | Float conversion plus linear/sRGB byte encode/decode helpers. RHI format support and texture cooking own storage-format decisions. | `S` |
| `CORE-016` | Timer | Implemented path | Per-frame raw/scaled delta and total time, frame count, time scale, pause/resume, and seconds/milliseconds conversion. No fixed-step simulation scheduler exists here. | `S` |
| `CORE-017` | Thread ownership assertions | Implemented path | Threads can receive diagnostic roles; `OwnerThread` captures an owner and asserts access. This detects contract violations in instrumented paths; it does not make objects thread-safe. | `S` |
| `CORE-018` | Stable hashing/string tables | Implemented path | Hash helpers, string escaping/parsing, and deterministic string-table construction support cooked identifiers and manifests. Collision policy remains the consuming schema's responsibility. | `S` |

## Vertical Traces

### Transactional file publication

Producer builds temporary files -> `Files::FilePublication` groups temporary/final paths -> `TryPublishFileSet` finalizes the set -> failures return an error and temporary cleanup helpers remove staging products. Shader and asset cookers consume this route; release proof must show that an interrupted generation never leaves a mixed readable generation.

### Configuration entry

Static `ConsoleVariable<T>` construction registers with the singleton registry -> command-line or console host resolves the name -> string parsing mutates the typed value -> Application/Renderer/RHI reads it at its chosen boundary. Core provides no persistence, range schema, restart policy, or UI ownership; those remain with the consumer.

### Logging category ownership

A receiver declares a stable category with `SPARKLE_DEFINE_LOG_CATEGORY_STATIC`
in its implementation file, or pairs `SPARKLE_DECLARE_LOG_CATEGORY` in a
shared header with one `SPARKLE_DEFINE_LOG_CATEGORY` definition. Receivers log
through that category and do not cache `shared_ptr<spdlog::logger>` instances or
create registry entries. `SparkleCore` alone owns lazy category resolution,
registry synchronization, sinks, levels, and logger lifetime. This keeps the
frontend declarative while preserving one backend policy and one creation path.
Logger bootstrap follows `Logger -> LogPathPolicy -> stateless user-state and
filesystem discovery`. The policy must not call logging or aggregate path
getters; the public `Paths::LogFile` function delegates to the same policy. This
allows aggregate path initialization to report diagnostics without re-entering
itself. `architecture_boundary_check` rejects dependency reversals.

### Workspace-derived paths

Executable/current directory and markers -> workspace/project discovery -> `ConfigureProjectRoot` publishes the active project -> typed getters derive Engine/Project/cooked paths -> runtime/tools open those products. This is a vital package risk because a successful workspace path does not prove operation outside the repository.

## Explicit Non-Capabilities And Risks

- No networking, sockets, plugin loader, reflection/type system, general archive/container format, database, allocator framework, or cross-platform process implementation was found in this module.
- `Windows.h`, DirectXMath, Win32 process code, and private `bcrypt` make the current Core surface Windows-oriented despite otherwise portable helpers.
- CVar assignment is mutable global state and not synchronized or persisted by Core.
- `Files::BinarySpanReader` owns cursor advancement, bounds, multiplication overflow, and borrowed raw array bytes. GameFramework's cooked reader translates failures into domain exceptions around that same cursor; it does not implement another byte decoder. The owning cooked schema still validates magic, counts, cross-references, and semantic invariants. This source boundary was rechecked on 2026-10-06.
- Source presence does not establish crash-safe publication, Unicode/path edge behavior, privilege behavior, or packaged path correctness.
