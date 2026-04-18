# Logging Contract Inventory

This document supports Milestone 1 / Phase 1E after the direction change to remove `Log.h` and `Log.cpp` entirely instead of preserving them as a permanent facade.

The inventory below captures the legacy Sparkle logging contract that repo code was written against. Prompt 2 has already swapped the active backend implementation to `spdlog`, but Prompt 1 still needs a precise record of the contract being migrated and deleted.

## Legacy contract inventory

- Public logging surface used by the repo:
  - `LogLevel`
  - `Logger::SetLevel(...)`
  - `Logger::GetLevel()`
  - `Logger::IsEnabled(...)`
  - `LOG_TRACE(...)`, `LOG_DEBUG(...)`, `LOG_INFO(...)`, `LOG_WARNING(...)`, `LOG_ERROR(...)`, `LOG_FATAL(...)`
  - `CHECK(...)`
- Compile-time filtering:
  - `LE_COMPILE_LOG_LEVEL` removes lower-severity macro expansions at compile time.
  - Debug builds default to `LE_LOG_LEVEL_TRACE`.
  - Release builds default to `LE_LOG_LEVEL_INFO`.
- Runtime filtering:
  - `Logger::SetLevel(...)` controls the active minimum runtime level.
  - `Logger::IsEnabled(...)` returns whether a level survives runtime filtering.
  - repo code does not currently call `Logger::SetLevel(...)`, `Logger::GetLevel()`, or `Logger::IsEnabled(...)` directly outside `Log.h` and `Log.cpp`; the practical external surface is the macro layer plus `CHECK(...)`.
- Message envelope behavior:
  - the macro layer injects `__FILE__` and `__LINE__`
  - output prefixes the basename of the source file and line number
  - output prefixes a fixed level tag such as `[INFO]` or `[ERROR]`
  - each emitted record ends with a newline
- Legacy sink behavior before the backend swap:
  - all messages are written to `stderr`
  - Windows builds mirror the same text to `OutputDebugStringA`
  - sink writes are serialized by a global mutex
- Fatal-path behavior:
  - `LOG_FATAL(...)` always writes the message, flushes `stderr`, breaks into an attached debugger in debug Windows builds, then aborts
  - `CHECK(...)` logs a fatal HRESULT failure with the platform error text when available, flushes `stderr`, breaks into an attached debugger in debug Windows builds, then aborts
- Formatting behavior observed in callsites:
  - most callsites pass a final string, usually built with `std::format(...)` or string concatenation
  - the current macro surface does not accept typed format arguments directly

## Representative callsites

- `Engine/Renderer/Private/Textures/TextureManager.cpp`
  - uses `std::format(...)` before `LOG_INFO`, `LOG_DEBUG`, `LOG_ERROR`, and `LOG_WARNING`
- `Engine/RHI/Private/D3D12/D3D12Rhi.cpp`
  - uses `LOG_FATAL(...)` for invariant failures and `CHECK(...)` for HRESULT failures
- `Tools/ShaderCompiler/Private/Compiler/DxcShaderCompiler.cpp`
  - uses string concatenation for compiler warnings and errors
- `Tools/AssetConverter/Private/Assets/Importers/Gltf/GltfSceneReader.cpp`
  - uses `std::format(...)` for importer diagnostics

## Callsite distribution snapshot

- `LOG_*` usage is still broad across `Engine/Core`, `Engine/Renderer`, `Engine/RHI`, `Engine/Editor`, `Engine/Platform`, and the asset or shader tools, so Prompt 3 is a real repo-wide migration rather than a local Core-only cleanup.
- `CHECK(...)` usage is concentrated in D3D12 and COM-facing code paths where HRESULT failures need fatal handling.
- Raw low-level entrypoints such as `Logger::Detail::Write(...)` and `Logger::Detail::CheckHR(...)` are not consumed directly outside the logging header or implementation, which confirms that migrating callsites should focus on macro replacement and verify-helper relocation rather than preserving those low-level functions.

## Usage Model Options

- Option A: direct `spdlog` calls everywhere.
  - Shape: include `spdlog/spdlog.h` or logger headers in normal engine code and call `spdlog::info(...)`, `spdlog::warn(...)`, and friends directly.
  - Benefits: smallest migration surface, very common in small engines and tools, no custom abstraction to maintain.
  - Costs: third-party ownership spreads through the repo, future logger replacement becomes expensive, and logger bootstrap discipline can erode if every subsystem configures its own state.
- Option B: central logger registry plus direct `spdlog` API at callsites.
  - Shape: SparkleCore owns startup, sinks, and named logger creation; engine code retrieves a named logger and then uses normal `spdlog` methods on that logger.
  - Benefits: matches common engine practice, keeps bootstrap and sink policy centralized, avoids rebuilding a custom logging facade, and still lets code use native `spdlog` features directly.
  - Costs: requires a small access layer for logger discovery and category ownership, and callsites still depend on `spdlog` types.
- Option C: thin wrapper or access macros around `spdlog`.
  - Shape: create a small engine-owned header or helper layer that forwards into `spdlog` without preserving the entire old API.
  - Benefits: some insulation from third-party includes and a smaller callsite diff if macro-style usage is preferred.
  - Costs: easiest path to recreating the old facade by accident; if it grows, the repo ends up maintaining a custom logging API again.

## Selected Direction

- Selected option: Option B.
- Why this is the chosen fit here:
  - it follows the new direction to remove `Log.h` and `Log.cpp` instead of keeping a long-term compatibility facade
  - it still keeps SparkleCore in charge of logger bootstrap, sink setup, file logging, and named logger lifetime
  - it lets engine and tool code use `spdlog` idioms directly after acquiring a logger, which is closer to how many modern engines and tools use the library in practice
  - it avoids turning the migration into a permanent custom wrapper maintenance problem

## Non-Logging Diagnostics Split

- `CHECK(...)` and HRESULT-specific fatal helpers should not stay bundled with a logging header that is scheduled for deletion.
- If the project still wants those semantics, move them into a separate verify or assert utility under `Diagnostics` rather than treating them as part of the logging API.

## Prompt 2 implications

- The spdlog migration should replace the active logging path and then delete `Log.h`, `Log.cpp`, and the old custom sink or buffer helpers instead of preserving them behind a shim.
- Existing repo callsites should be migrated to the chosen `spdlog` usage model explicitly rather than relying on a permanent alias layer.
- Sink policy, file logging, and any optional rotating-file support should remain SparkleCore-owned bootstrap decisions even if callsites use native `spdlog` APIs.

## Prompt 2 implementation snapshot

- `spdlog` is fetched through `CMake/Dependencies/FetchDependencies.cmake` and linked to `SparkleCore` via `spdlog::spdlog_header_only`.
- `SparkleCore` now exposes a named logger registry through `Engine/Core/Public/Diagnostics/Logger.h`, and `spdlog` is a public dependency of `SparkleCore` because callsites will retrieve engine-owned loggers and then use native `spdlog` APIs directly.
- Default runtime sinks are `stderr` plus the Windows debugger output sink.
- An optional private file sink is enabled when `SPARKLE_LOG_FILE` points to a writable log path.
- `CHECK(...)` and HRESULT failure handling now live under a separate verify seam in `Engine/Core/Public/Diagnostics/Verify.h` and `Engine/Core/Private/Diagnostics/Verify.cpp` instead of remaining part of `Log.h`.
- Prompt 3 has now completed: repo callsites use the chosen native `spdlog` model after logger lookup, and the legacy `Log.h`, `Log.cpp`, `LogBuffer`, `LogSink`, and `LogFormatting` files have been deleted.
- Prompt 1 should therefore be read as the recorded legacy contract and selected access decision, not as a claim that the active sink plumbing is still the pre-`spdlog` implementation.

## Prompt 3 completion snapshot

- Engine and tool callsites now log through `SPDLOG_LOGGER_*` macros after acquiring engine-owned named loggers from `Engine::Logging`.
- Fatal and HRESULT paths now route through `Engine/Core/Public/Diagnostics/Verify.h` instead of the deleted logging facade.
- Repo-wide searches over `Engine/**` and `Tools/**` no longer find legacy `LOG_*`, `Log.h`, `LogLevel`, or `LE_LOG` usage in active source.
- The remaining work for Phase 1E is Prompt 4 guardrails, not more bridge cleanup.

## Prompt 4 guardrail snapshot

- `CMake/Validation/ValidateLoggingBoundary.cmake` now rejects legacy logging macros or headers, direct default-logger `spdlog` usage, and ad hoc sink or bootstrap code outside `Engine/Core/Private/Diagnostics/Logger.cpp`.
- The top-level `logging_boundary_check` target is wired into engine, tool, and discovered project targets so the guardrail runs in the normal build path.
- `Engine/Core/CMakeLists.txt` now fails configure immediately if the deleted legacy logging facade files are reintroduced.