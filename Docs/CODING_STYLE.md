# Sparkle Coding Style

This document captures the coding conventions used across the engine. The repository-level tooling (`.clang-format`, `.clang-tidy`) is authoritative — editors, CI, and local tooling should follow those files.

## Tooling

- **Authoritative configs**: The canonical configs live at the repo root:
  - **.clang-format** — formatting rules for C++ and HLSL
  - **.clang-tidy** — static-analysis checks and options

- **Use the wrapper**: Prefer the provided `RunClangFormat.bat` to apply formatting consistently across environments.

- **Quick commands (Windows PowerShell)**:

```powershell
.\Scripts\RunClangFormat.bat
# optionally inspect the log at logs/LogClangFormat.txt
``` 

## Formatting (authoritative: `.clang-format`)

The following settings are the exact, authoritative formatting options from the repository `.clang-format`. Keep these in sync with the file at the repo root.

- **Base style**: BasedOnStyle: LLVM
- **Language**: Cpp
- **Indentation**: tabs for indentation (UseTab: ForIndentation)
  - `IndentWidth`: 4
  - `TabWidth`: 4
  - `ContinuationIndentWidth`: 4
- **Line length**: `ColumnLimit: 140`
- **Braces**: `BreakBeforeBraces: Allman` (Allman style)
- **Constructor initializer lists**:
  - `ConstructorInitializerAllOnOneLineOrOnePerLine: true`
  - `BreakConstructorInitializers: AfterColon`
  - `ConstructorInitializerIndentWidth: 4`
- **Bin-packing**: `BinPackParameters: false`, `BinPackArguments: false` (prefer one item per line when wrapped)
- **Pointer/reference alignment**: `PointerAlignment: Left`, `ReferenceAlignment: Left`
- **Include handling**: `SortIncludes: false`, `IncludeBlocks: Preserve`
- **Lambdas**: `LambdaBodyIndentation: Signature` and `AllowShortLambdasOnASingleLine: None`
- **Comments**: `ReflowComments: false`

If you need a quick reference, open `.clang-format` at the repo root — that file is the source of truth.

## Practical editor rules

- Configure your editor/IDE to: use the repository `.clang-format` and run formatting on save where possible.
- Disable editor-specific format overrides that conflict with the repo config.

## File layout

### Headers

- Keep headers self-contained and `#include` what they use.
- Prefer forward declarations over includes where practical.

### Source files

- Translation units should include the PCH first when available (e.g. `#include "PCH.h"`), then the matching header, then engine headers, then system/third-party headers.

## Includes

- Group includes with a blank line between groups (PCH / engine / third-party / system).
- Prefer engine headers with quotes and system/third-party headers with angle brackets.

## Naming

- **Types** (classes/structs/enums): `PascalCase`
- **Functions and methods**: `PascalCase`
- **Local variables**: `camelCase`
- **Member variables**: `m_` prefix, `camelCase` afterwards (e.g. `m_device`, `m_frameIndex`)
- **Boolean naming**:
  - Parameters: `bSomething`
  - Members: `m_bSomething`
- **Global singletons**: `GName`

## Error handling

- Use `CHECK(hr)` / `LOG_FATAL(...)` for unrecoverable initialization failures.
- Prefer returning status only at module boundaries; internally keep code simple and fail fast.

## Performance and memory

- Avoid per-frame heap allocations in hot paths.
- Prefer value types and stack allocations for transient data.
- Use RAII for resource lifetime; avoid shared ownership by default.

---

## Comments

High-quality comments explain **why** and **what** — not just **how**. They enable future maintainers (and your future self) to understand intent, constraints, and non-obvious decisions without reverse-engineering the code.

### Comment Philosophy

1. **Code should be self-documenting first** — use clear naming, small functions, and logical structure
2. **Comments fill the gaps** — explain intent, constraints, and "why" that code cannot express
3. **Avoid noise** — don't comment obvious code; don't paraphrase what the code already says
4. **Keep comments accurate** — outdated comments are worse than no comments

### File Headers

Use decorated block comments only when a file needs a short purpose statement at the top. Keep them brief and factual:

```cpp
// =============================================================================
// ComponentName.h — Brief One-Line Description
// =============================================================================
//
// Short summary of the file's purpose or the constraint a reader needs to know.
// Stop once the intent is clear.
// =============================================================================
```

Do not add template sections such as `USAGE`, `DESIGN`, `CONTROLS`, `RESPONSIBILITIES`, or `NOTES` at the start of headers. If a header does not benefit from a short summary, skip the file header entirely.

**When to use:** Public headers where a compact purpose statement reduces ambiguity.
**When to skip:** Internal implementation headers, trivial utility classes, or headers whose names already make the intent obvious.

### Section Dividers

Group related code within a file using section comments:

```cpp
// =============================================================================
// Lifecycle
// =============================================================================

void Initialize();
void Shutdown();

// =============================================================================
// Public API
// =============================================================================

void DoWork();

// -----------------------------------------------------------------------------
// Internal Helpers (private section divider - lighter weight)
// -----------------------------------------------------------------------------
```

Use `=` for major public sections, `-` for internal/private groupings.

### Function Comments (Declaration)

Document public functions at declaration site in headers:

```cpp
// Compiles a shader with the given options.
// Returns a result containing bytecode on success, or error message on failure.
static ShaderCompileResult Compile(const ShaderCompileOptions& options);

// Convenience overload: resolves the shader path and builds options automatically.
// sourcePath: relative path from shader root (e.g., "Passes/Forward/ForwardLitVS.hlsl")
static ShaderCompileResult CompileFromAsset(
    const std::filesystem::path& sourcePath,
    ShaderStage stage,
    const std::string& entryPoint = "main");
```

**Guidelines:**
- First line: **what** the function does (verb phrase)
- Additional lines: parameters, return value, preconditions, side effects
- Document non-obvious behavior, not obvious signatures

### Function Comments (Definition)

At implementation site, add a brief one-liner only when it explains intent that is not already obvious from the function name or surrounding code:

```cpp
// Selects the best available adapter (GPU) that supports Direct3D 12
void D3D12Rhi::SelectAdapter() noexcept
{
    // ...
}

// Initializes the window and registers its class
void Window::Initialize()
{
    // ...
}
```

### Inline Comments

Use sparingly for non-obvious logic. Focus on **why**, not **what**. Delete comments that merely narrate the next line, restate the function name, or label a block whose intent is already obvious from the code:

```cpp
// Try adapter-by-preference first. Use a local temporary adapter to avoid
// repeatedly replacing the member until a suitable one is found.
for (UINT i = 0;; ++i)
{
    // ...
}

// Lightweight feature probe: does this adapter support D3D12 at the
// desired feature level? We don't create a device here, just test.
if (SUCCEEDED(D3D12CreateDevice(candidate.Get(), m_DesiredD3DFeatureLevel, ...)))
```

**Bad (noise):**
```cpp
// Increment the counter
++counter;

// Check if null
if (ptr == nullptr)

// Creates render target views for all swap chain buffers
void CreateRenderTargetViews();
```

**Good (explains why):**
```cpp
// Guard against pathological allocations from malformed image headers
if (slicePitch64 > std::numeric_limits<size_t>::max())

// Fixed stack capacity chosen to comfortably hold typical log lines
// (file:line + level tag + message). Keeping this on the stack makes
// logging cheap and avoids heap churn during bursts.
static constexpr std::size_t kCapacity = 2048;
```

### Enum and Constant Documentation

Document enum values inline when meaning isn't obvious from the name:

```cpp
enum class LogLevel : std::uint8_t
{
    Trace = 0,    // Frame-by-frame diagnostics: hot-path traces. High volume; disabled in release.
    Debug = 1,    // Developer-focused flow/state info for debugging.
    Info = 2,     // High-level runtime events (startup, shutdown). Non-noisy normal ops.
    Warning = 3,  // Unexpected but recoverable conditions (fallbacks, missing optionals).
    Error = 4,    // Failures preventing operation completion; process may continue degraded.
    Fatal = 5     // Unrecoverable: log, flush, break if debugger attached, terminate.
};
```

### Class Member Documentation

Group members logically and document non-obvious ones:

```cpp
private:
    // Fixed stack capacity chosen to comfortably hold typical log lines
    static constexpr std::size_t kCapacity = 2048;  // tuned for typical messages
    
    char m_data[kCapacity]{};  // zero-initialized for clarity when printed
    std::size_t m_pos = 0;     // current write position
```

### TODO/FIXME Comments

Use standardized tags for actionable items:

```cpp
// TODO: Add support for cubemap arrays
// FIXME: Memory leak when reloading textures
// PERF: Consider caching this computation per-frame
// HACK: Workaround for driver bug on AMD RX 6000 series
```

### What NOT to Comment

| Anti-Pattern | Example |
|--------------|---------|
| Paraphrasing code | `// Set x to 5` above `x = 5;` |
| Trivial getters/setters | `// Returns the width` above `GetWidth()` |
| Boilerplate header sections | `USAGE`, `DESIGN`, `CONTROLS`, `RESPONSIBILITIES`, `NOTES` blocks at the top of headers |
| Commented-out code | Delete it; version control has history |
| Journal entries | `// Modified by John on 2024-01-15` — use git |
| ASCII art dividers | `///////////////////////////` |

### Comment Formatting

- Use `//` for single-line and short multi-line comments
- Use `/* */` sparingly, primarily for disabling code blocks temporarily
- Maintain consistent capitalization (sentence case, start with capital)
- End complete sentences with periods; fragments can omit punctuation
- Align trailing comments when they form a logical group:

```cpp
m_data[kCapacity]{};   // zero-initialized for clarity
std::size_t m_pos = 0; // current write position
```

### Relationship to Logging

Comments document **static intent** in code; logging captures **runtime behavior**.

| Use Comments For | Use Logging For |
|------------------|-----------------|
| Algorithm explanation | Runtime diagnostics |
| API contracts | State transitions |
| Design decisions | Error conditions |
| Performance notes | Debugging flow |

See [LOGGING_STYLE.md](LOGGING_STYLE.md) for runtime logging conventions.

---

