# ShaderCompiler Architecture Review — Production-Grade Target Design

> Companion document to the Sparkle engine. The goal is not to document what the
> tool *currently* does in detail (that lives in code). The goal is to describe
> **how a production-grade offline shader compiler should be designed**, what
> module boundaries exist, how data and control flow between editor, runtime,
> and the offline tool, and how the present Sparkle implementation maps onto
> that target.
>
> The design is deliberately **single-threaded today**, but the abstractions
> are chosen so that parallelism, additional backends (Vulkan/SPIR-V), caching,
> reflection, and editor hot-reload can be added later without re-architecting.

## TL;DR For A Reviewer

1. The tool is an **offline asset cooker**, not a runtime compiler. Runtime
   never invokes a shader compiler — it only reads cooked binary packages.
2. The system is split into clear layers: **CLI → Manifest → Plan → Compile →
   Cook → Write**, with a **runtime-owned binary contract** as the only thing
   that crosses the offline/runtime seam.
3. Backends are abstracted behind `IShaderBackend`. DXC is one adapter; SPIR-V
   would be another. The orchestration layer never names DXC.
4. Editor "hot reload" is implemented as **out-of-process recook + reload
   notification**, not as an in-process compiler. That preserves the rule that
   runtime never links the compiler.
5. The cooked package format is **versioned** (`Magic`, `Version`, content
   hashes) so the runtime reader can refuse mismatches cleanly and the cooker
   can evolve the schema with a migration playbook.

---

## 1. Purpose & Audience

This document targets two readers:

- **An engineer joining the Sparkle codebase** who needs to understand how
  shaders move from `.hlsl` source on disk to GPU bytecode at runtime, and
  where they are allowed (and not allowed) to change things.
- **A portfolio reviewer** evaluating whether the author understands how
  production engines structure their shader pipelines.

It is intentionally a *design* document, not an implementation log. Concrete
class names from the present implementation appear only when grounding a
decision in real code.

## 2. What Is A Shader Compiler (In The Engine Sense)

The phrase "shader compiler" means two very different things depending on
scope. They get conflated constantly. Distinguishing them is half the design.

**Backend compiler** — turns a single `.hlsl` source file plus options into a
bytecode blob. Examples: `dxc`, `fxc`, `glslang`, `slangc`, `clang`. These are
language toolchains.

**Shader pipeline / cooker** — the engine-side system that decides *what* needs
to be compiled, with *which* options, packages results into runtime-ready
artifacts, tracks dependencies, caches results, and feeds the runtime. Examples
of this category: Unreal's `ShaderCompileWorker` + DDC, Frostbite's "ShaderDB"
pipeline, Unity's `ShaderLab` cooker, Sparkle's `Tools/ShaderCompiler`.

A production engine almost always **wraps** a backend compiler (DXC/Slang)
inside its own pipeline. The pipeline is where engine-specific concepts live:
permutations, packages, binding layouts, target platforms, caching,
incremental cook. This document is about the **pipeline**.

Vocabulary used throughout (full glossary in Appendix B):

- **Stage** — one programmable pipeline step (vertex, pixel, compute, mesh, …).
- **Package** — a named bundle of stages that ship together because they are
  used together (e.g. a vertex+pixel pair forming a material program).
- **Variant / Permutation** — one specific compilation of a package or stage
  parameterised by `#define`s, target, quality, feature flags.
- **Manifest** — a declarative file authored by a human that tells the cooker
  *which packages exist* and *which stages each contains*. It is the input
  surface; humans edit manifests, the cooker reads them.
- **Cook** — the act of taking source + manifest and producing the runtime
  binary artifact.
- **Cooked package** — the binary file the runtime mmap's / loads. Owned by a
  schema in the engine's RHI module, not by the tool.
- **DDC (Derived Data Cache)** — content-addressed cache of cook outputs so
  unchanged inputs don't recompile. Local on-disk today; shared/remote later.
- **Reflection** — extracted layout metadata (resources, constant buffers,
  binding slots) describing what the bytecode expects from the runtime.

## 3. Design Goals & Non-Goals

**Goals**

- Single source of truth for shader compilation — runtime never compiles.
- Incremental, content-addressed cook — unchanged inputs reuse previous output.
- Backend-agnostic orchestration — DXIL today, SPIR-V designed for, no churn
  in the orchestration layer when a new backend lands.
- Stable, versioned cooked artifact format owned by RHI.
- Structured diagnostics — machine-readable errors with source maps.
- Editor hot reload via out-of-process recook + signal, not in-process compile.
- Single-threaded executor today; the dependency graph is the parallelism
  contract for tomorrow.

**Non-goals (explicitly out of scope)**

- Multithreaded executor implementation.
- Distributed compile workers (Unreal-style farm).
- Shared/remote DDC implementation (interface seam only).
- A new shader language or IR.
- In-editor in-process recompilation.
- Vendor-specific analysis (RGA-style ISA disassembly) as a built-in — left as
  an `IAnalysisPass` seam.

## 4. End-User Mental Model

Three personas interact with the system. Each has a different surface.

**Engine programmer.** Edits `.hlsl` source, edits binding layout C++ code,
runs the cook tool (or the editor, which triggers a recook). Cares about:
diagnostics quality, cook speed, deterministic output, never having to think
about runtime fallback.

**Shader author / TA.** Edits manifests and `.hlsl` source. Adds new packages.
Cares about: a clear authoring format, fast iteration, useful error messages
that point to source, reasonable permutation behaviour.

**Build automation.** Invokes the tool from CI / batch scripts. Cares about:
exit codes, machine-readable output, reproducibility, idempotency.

## 5. High-Level Architecture (Target)

The big picture, including the offline/runtime seam:

```text
                 ┌──────────────────────────────────────────────────────────┐
                 │                       OFFLINE                             │
                 │  (Tools/ShaderCompiler executable)                        │
                 │                                                           │
   manifest ───►│  Manifest ──► Plan ──► DependencyGraph ──► Executor       │
   .hlsl    ───►│      │           │              │              │           │
   layout   ───►│      ▼           ▼              ▼              ▼           │
                │  PermutationExpand   CacheLookup        CompileStage       │
                │                          │                   │             │
                │                          ├── hit ──► reuse artifact        │
                │                          └── miss ─► IShaderBackend ──► DXC/SPIRV
                │                                            │             │
                │                                            ▼             │
                │                                      ReflectionExtract   │
                │                                            │             │
                │                                            ▼             │
                │                                     CookedPackageWriter  │
                │                                            │             │
                │                                            ▼             │
                │                          ┌─────────────────────────────┐ │
                │                          │  *.spkg  +  registry        │ │
                │                          │  (cooked artifacts on disk) │ │
                │                          └─────────────────────────────┘ │
                └──────────────────────────────────────────────────────────┘
                                              │
                              (file system + ShaderRecookSignal)
                                              │
                 ┌──────────────────────────────────────────────────────────┐
                 │                       RUNTIME                             │
                 │  (Engine/RHI + Engine/Renderer + Engine/Editor)           │
                 │                                                           │
                 │   CookedPackageReader (in RHI) ─► ShaderResource          │
                 │              │                          │                 │
                 │              ▼                          ▼                 │
                 │     ReflectionView           PSO / RootSignature build    │
                 │                                                           │
                 │   Editor: ShaderRecookWatcher ─► (re-read + bump version) │
                 └──────────────────────────────────────────────────────────┘
```

The seam is exactly one layer thick: **a versioned binary file format** whose
schema lives in `Engine/RHI/Public/Shaders/CookedShaderPackage.h`. Nothing
else crosses.

## 6. Module Boundaries & Communication

Module ownership in the target design:

```text
Tools/ShaderCompiler/             orchestration, planning, executor, cache, CLI
Tools/ShaderCompiler/Backends/    IShaderBackend + adapters (DXC today, SPIRV later)
Tools/ShaderCompiler/Analysis/    optional analysis passes (e.g. RGA wrapper)
Tools/ShaderCompiler/Cli/         command verbs, exit codes, machine-readable output

Engine/RHI/Public/Shaders/        cooked package SCHEMA + reader + reflection types
Engine/Core/                      generic file/path/string/hash/log utilities
Engine/Renderer/                  CONSUMER of cooked packages (no compile, ever)
Engine/Editor/                    editor UI + ShaderRecookWatcher (no compile, ever)
```

### Allowed edges

```text
Tools/ShaderCompiler           ──► Engine/Core           (utilities)
Tools/ShaderCompiler           ──► Engine/RHI public     (cooked schema, enums)
Tools/ShaderCompiler/Backends  ──► dxcompiler            (only the DXC adapter)
Engine/RHI                     ──► Engine/Core           (utilities)
Engine/Renderer                ──► Engine/RHI            (reads cooked packages)
Engine/Editor                  ──► Engine/RHI            (reads cooked packages)
Engine/Editor                  ──► (file watcher OS APIs)(observes recook signal)
```

### Forbidden edges (enforced by `CMake/Validation/ValidateShaderCompilerBoundary.cmake`)

```text
Engine/RHI                  ──/► dxcompiler             FORBIDDEN
Engine/RHI                  ──/► DxcShaderCompiler      FORBIDDEN
Engine/RHI                  ──/► Tools/ShaderCompiler   FORBIDDEN
Engine/Renderer             ──/► Tools/ShaderCompiler   FORBIDDEN
Engine/Editor               ──/► Tools/ShaderCompiler   FORBIDDEN
Engine/Application          ──/► Tools/ShaderCompiler   FORBIDDEN
Engine/GameFramework        ──/► Tools/ShaderCompiler   FORBIDDEN
Tools/ShaderCompiler        ──/► Engine/Renderer        FORBIDDEN
Tools/ShaderCompiler        ──/► Engine/Editor          FORBIDDEN
Tools/ShaderCompiler        ──/► Engine/Application     FORBIDDEN
Tools/ShaderCompiler        ──/► Engine/GameFramework   FORBIDDEN
Tools/ShaderCompiler        ──/► Engine/RHI/Private/    FORBIDDEN (only RHI public)
```

The forbidden-edge list is *the* architectural invariant. Everything else can
change. If a future PR needs a forbidden edge, the design is wrong, not the
validator.

### Communication patterns

- **Tool ↔ runtime**: only via cooked binary files + the registry index. No
  shared memory, no IPC, no in-process linkage.
- **Tool ↔ tool internals**: in-process function calls, plain values, no
  globals beyond the `DxcContext` adapter singleton (which lives behind
  `IShaderBackend`).
- **Editor ↔ tool**: editor *spawns* the tool process, waits for exit code,
  then re-reads cooked artifacts. The editor never links the tool.
- **Editor ↔ runtime**: the editor calls into RHI to bump shader resource
  versions after a successful recook.

## 7. Core Abstractions / Object Catalog

Each entry: **role**, **owns**, **depends on**, **lifetime**.

### Front-end (authoring intent → in-memory description)

- **`ShaderManifest`** — the parsed, validated, merged authoring document.
  Owns: a list of `ShaderPackageDesc`. Depends on: `IManifestSource`. Lifetime:
  one cook invocation.
- **`IManifestSource`** — pluggable parser interface (`ini`, `json`, code).
  Owns: nothing. Depends on: `Engine/Core` file utilities. Lifetime: stateless.
- **`ShaderPackageDesc`** — declarative package: id, binding-layout id, variant
  id, list of `ShaderStageDesc`. Plain data.
- **`ShaderStageDesc`** — stage enum + source path + entry point. Plain data.

### Plan (intent → unit-of-work)

- **`PermutationExpander`** — turns one `ShaderPackageDesc` into N
  `ShaderCompileRequest`s by combining variant axes. Pure function.
- **`ShaderPermutationKey`** — stable hash of all axis values. Used as part of
  `ShaderCacheKey`.
- **`ShaderCompileRequest`** — immutable, fully-resolved unit of work: source
  path, entry point, stage, target, define set, include roots, debug flags.
- **`ShaderTarget`** — enum of (api, profile, shader model). E.g. `DxilSm66`,
  `SpirV15`. Maps onto `CookedShaderBinaryFormat` for emission.

### Source layer

- **`ShaderIncludeResolver`** — resolves `#include` paths against the engine
  + project source roots. Used by both compile and dependency tracking.
- **`ShaderSourceDatabase`** — indexes all known shader source files, computes
  content hashes, builds the include graph for dependency invalidation.

### Cook graph + execution

- **`CookNode`** — graph node wrapping one `ShaderCompileRequest` plus its
  resolved inputs (source hashes, include hashes, options hash).
- **`DependencyGraph`** — DAG of `CookNode`s; topological order is the cook
  order. The graph is the parallelism contract; today the executor walks it
  serially.
- **`ICookExecutor`** — interface; `SerialCookExecutor` is today's
  implementation; `ParallelCookExecutor` is a future drop-in.

### Cache

- **`ShaderCacheKey`** — content-addressed key composed of: source content
  hash, include closure hash, target, permutation key, options hash, backend
  version, schema version.
- **`IShaderArtifactStore`** — get/put for `ShaderCacheKey → CompiledArtifact`.
  Today: local on-disk store under `bin/Cache/Shaders/`. Future seam:
  shared/remote store. Same interface.

### Backend

- **`IShaderBackend`** — `Compile(ShaderCompileRequest) → CompiledArtifact`.
  Returns bytecode + raw reflection blob + diagnostics.
- **`ShaderBackendCapabilities`** — query: which targets, which features (mesh
  shaders, raytracing), which reflection format.
- **`DxcShaderBackend`** — adapter over `IDxcCompiler3`. Current concrete
  backend. Lives in `Tools/ShaderCompiler/Backends/`.
- **`SpirVShaderBackend`** — designed-for, not implemented. Would adapt
  `glslang` or DXC's SPIR-V codegen.

### Reflection

- **`IReflectionExtractor`** — backend-specific; turns the raw reflection
  blob into the normalized `ShaderReflection`.
- **`ShaderReflection`** — normalized resource layout: bindings, constant
  buffer members, push-constant ranges (if applicable), thread group size,
  IO signature. Backend-agnostic at this layer.

### Output

- **`CookedPackageBuilder`** — assembles all per-stage artifacts + reflection
  + binding records + string table into the in-memory package layout.
- **`CookedShaderPackageWriter`** — serializes the in-memory layout into the
  on-disk binary using the schema in `Engine/RHI/Public/Shaders/CookedShaderPackage.h`.
- **`CookedRegistryWriter`** — writes a human-readable registry index of all
  packages produced this cook.

### Diagnostics

- **`CookDiagnosticSink`** — receives structured diagnostics
  (`severity, code, source location, message, context`). Renders to console
  for humans, to JSON Lines for automation.
- **`SourceMap`** — maps post-preprocess line numbers back to original source
  + line. Attached to errors so a paste-into-IDE jump works.

### Editor / hot reload (design-only seams)

- **`ShaderRecookSignal`** — a small marker file or named event the cook tool
  emits on successful completion. The runtime watcher consumes it.
- **`ShaderRecookWatcher`** — editor-side file watcher that observes the
  signal, asks RHI to re-open changed packages, and bumps shader-resource
  version numbers so PSO caches invalidate.

## 8. Authoring Front-End: What A Manifest Is And Why

A **manifest** is a declarative file that tells the cooker *what to cook*.
It is the human-authored input. It does **not** contain shader code; it
references `.hlsl` files. A manifest is to a shader pipeline what
`CMakeLists.txt` is to a build system.

A minimal manifest in today's INI-flavored shape:

```ini
[ShaderCookManifest]
Version = 1

[Package BasicLit]
BindingLayout = StandardMaterial
Variant       = Default
Stage.Vertex  = Materials/BasicLit.hlsl | VSMain
Stage.Pixel   = Materials/BasicLit.hlsl | PSMain
```

Why packages, not loose stage files? Because the **runtime unit of binding** is
a pipeline (vertex+pixel, or compute, or mesh+pixel). Cooking at the package
granularity means the runtime never has to assemble compatible stages itself —
the package *is* the contract. This matches Unreal's `FShaderType` grouping
and Frostbite's "shader" being a package of permutations.

### Recommended evolution

- Treat manifest format as **pluggable** behind `IManifestSource`. The current
  INI parser becomes one implementation. JSON or TOML can be added without
  changing any downstream layer.
- Make the manifest **versioned**. A `Version` field lets the parser refuse
  unknown formats cleanly and lets the format evolve.
- Keep parsing **separate from validation**. Parse produces a syntax tree;
  validation enforces semantic invariants (no duplicate stages, references
  resolve, binding layout exists).

### Anti-patterns to avoid

- Inlining shader code into the manifest. Keeps the tool from being a code
  editor.
- Letting the manifest reference runtime code paths. It is offline-only.
- Letting the manifest depend on the build configuration. Reproducibility
  dies.

## 9. Permutation / Variant System

A **permutation** is one concrete compile of a stage with a specific set of
`#define`s. Real engines have hundreds to millions of permutations per
material. Even a toy engine grows them quickly the moment quality levels,
feature flags, or platform variants appear.

Target design:

- Each package declares **variant axes** (e.g. `SHADOWS={On,Off}`,
  `QUALITY={Low,High}`). The expander produces the cartesian product, optionally
  pruned.
- Pruning rules are first-class: `if SHADOWS=Off then QUALITY ignored`. This
  is what Frostbite calls "permutation pruning" and UE handles via
  `ShouldCompilePermutation`.
- A **`ShaderPermutationKey`** is a stable hash of all axis values plus the
  axis schema version. It is part of `ShaderCacheKey` and embedded in the
  cooked package so the runtime can look up the right variant.
- The runtime never *computes* permutations. It looks them up by key.

Out of scope for now: dynamic permutation generation from material graphs.
That is a content-pipeline problem above this layer.

## 10. Dependency Graph & Incremental Cook

The cook is a DAG, not a script:

```text
ShaderSourceFile ──┐
ShaderSourceFile ──┼──► CookNode (one ShaderCompileRequest)
IncludeFile      ──┤        │
BindingLayout    ──┘        ▼
                       CompiledArtifact ──► CookedPackageWriter
```

Inputs to a node:

- the resolved source file content hash
- the **include closure** content hash (transitive `#include`s)
- the binding layout content hash
- the resolved options hash (target + defines + flags)
- the backend identity + version

`ShaderCacheKey = hash(all of the above)`.

A cook proceeds:

1. Build the manifest → expand permutations → produce `CookNode`s.
2. For each node: compute `ShaderCacheKey`. Ask `IShaderArtifactStore`.
3. **Hit** → reuse stored artifact + reflection.
4. **Miss** → invoke `IShaderBackend`, store result.
5. After all nodes: assemble cooked packages, write registry.

Why a graph and not just a flat list? Because future features (linked
modules à la Slang, shared common bytecode chunks, or parallel execution)
need to reason about *order* and *dependency*. Building the graph now even
when the executor walks it serially keeps that door open.

### Invalidation rules

- Source file changes → all nodes whose include closure contains it
  invalidate.
- Binding layout changes → all nodes referencing that layout invalidate.
- Backend version bump → everything invalidates (rare, deliberate).
- Schema version bump → everything invalidates and the on-disk artifact
  format migrates.

### Why not just timestamps

Timestamps lie. They change without content changing (git checkouts, file
syncs, IDE saves of unchanged buffers). Content hashes are the truth. They
also enable a future shared cache: two machines with the same inputs hit
the same key.

## 11. Backend Abstraction

Single interface, multiple adapters:

```cpp
class IShaderBackend {
public:
    virtual ~IShaderBackend() = default;

    virtual ShaderBackendCapabilities GetCapabilities() const = 0;

    virtual CompiledArtifact Compile(
        const ShaderCompileRequest& request,
        CookDiagnosticSink&         diagnostics) = 0;
};
```

`ShaderTarget` enum (initial shape):

```cpp
enum class ShaderTarget : std::uint16_t {
    DxilSm60, DxilSm61, DxilSm62, DxilSm63, DxilSm64, DxilSm65, DxilSm66, DxilSm67,
    SpirV14, SpirV15, SpirV16,
    // future: MetalIR, WGSL, ...
};
```

`ShaderBackendCapabilities` answers: *can you compile this target?* *do you
support mesh shaders?* *what reflection format do you emit?*

Concrete adapters:

- **`DxcShaderBackend`** — wraps `IDxcCompiler3`. Maps Sparkle options to DXC
  arguments. Today's only implementation. Lives in
  `Tools/ShaderCompiler/Backends/Dxc/`.
- **`SpirVShaderBackend`** — design only. Would either (a) call DXC's SPIR-V
  codegen path, or (b) wrap `glslang`. Decision deferred until Vulkan work
  starts.

Crucial rule: **the orchestrator never names DXC.** It holds an
`IShaderBackend*` chosen at startup based on `ShaderTarget`. This is the
single biggest architectural lever for Vulkan readiness.

The cooked artifact already accommodates plurality:
`CookedShaderBinaryFormat::{Dxil, SpirV}` is in
`Engine/RHI/Public/Shaders/CookedShaderPackage.h` today. The schema is
already neutral; only the producer side is still DXC-only.

## 12. Reflection & Pipeline-Layout Extraction

Bytecode is useless without knowing what resources it expects. Reflection is
that knowledge.

Per-backend extraction, common output:

```cpp
struct ShaderReflection {
    std::vector<ShaderResourceBinding>  Bindings;       // SRVs, UAVs, CBVs, Samplers
    std::vector<ShaderConstantBuffer>   ConstantBuffers;
    std::array<std::uint32_t, 3>        ThreadGroupSize; // compute only
    ShaderIOSignature                   InputSignature;  // VS only
    // ... small, append-only as backends require
};
```

`IReflectionExtractor` is per-backend. The output `ShaderReflection` is
backend-agnostic. The renderer's PSO/root-signature builder consumes the
normalized form and never sees DXIL or SPIR-V specifics.

Why normalize? Because the renderer needs one code path. A backend swap must
be invisible above this layer. UE solves this with `FShaderParameterMap` +
`FShaderParameterBindings`. Sparkle's equivalent is the
`CookedShaderBindingRecord` array embedded in the cooked package.

## 13. Cooked Artifact Format & Versioning

The on-disk format is owned by **RHI**, not by the tool. Schema in
`Engine/RHI/Public/Shaders/CookedShaderPackage.h`.

Current shape (already in code):

```text
CookedShaderPackageHeader
├─ Magic            ('S','S','H','D')
├─ Version          (kCookedShaderPackageVersion)
├─ DeclaredStages   (ShaderStageMask)
├─ ShaderModelMajor/Minor
├─ counts (binary records, binding records, specialization inputs)
├─ section sizes (string table, binary blob)
├─ ShaderPackageKey
└─ hashes (SourceIdentity, BindingLayout, Variant)

CookedShaderBinaryRecord[]          (one per stage: stage, format, entry, blob ref, hash)
CookedShaderBindingRecord[]         (one per binding: name, semantic, access, slot)
CookedShaderSpecializationInputRecord[]   (specialization constants metadata)
StringTable bytes
BinaryBlob bytes                    (concatenated bytecode chunks, referenced by offset/size)
```

### Versioning rules

- `Magic` rejects entirely-foreign files at the door.
- `Version` rejects same-magic-different-layout files cleanly.
- Bumping `Version` requires a migration entry: a documented diff and a
  loader path that either reads-old or refuses-with-a-clear-error.
- All records are `trivially_copyable` (already enforced via `static_assert`
  in the header). The loader can mmap and reinterpret without per-record
  parsing.
- Strings live in a single string table referenced by `(offset, size)`. New
  fields are appended; old loaders refuse-by-version rather than mis-read.

### Migration playbook

1. Bump `kCookedShaderPackageVersion`.
2. Add new record fields at the end (or new record types).
3. Add a `LoadV<N>` function in RHI; keep `LoadV<N-1>` for one release if
   shipped; otherwise refuse with a recook hint.
4. Bumping the version invalidates every cache entry — that is correct.

### Registry

Alongside `*.spkg` files, a human-readable registry indexes them: package id,
variant, output path, hashes. Two consumers:

- humans / CI logs (debugging "what got cooked")
- runtime discovery (RHI loads the registry, then mmaps packages on demand)

The registry format is also versioned (`kRegistryFormatVersion`).

## 14. Diagnostics & Analysis Hooks

Diagnostics are first-class. Bad diagnostics ruin shader workflows.

`CookDiagnosticSink` receives structured records:

```cpp
struct CookDiagnostic {
    Severity     Severity;   // Error / Warning / Info
    std::string  Code;       // "SC1001" — stable codes for grep/CI rules
    SourceLocation Location; // file + line + column, post source-map
    std::string  Message;
    std::vector<DiagnosticContextFrame> Context; // include stack, permutation, package
};
```

Two renderers:

- **Console renderer** for humans: colored, with caret-and-source-line, with
  a "compiled while expanding permutation X of package Y" frame trail.
- **JSON Lines renderer** for CI: one JSON object per line, deterministic
  field order, exit code reflects highest severity seen.

### Source maps

DXC accepts `-Zi` for debug info. The cooker captures the preprocessed
source + line directives so reported errors point back to the original
`.hlsl` line, not the post-preprocess line. Same idea as JS source maps.

### Analysis pass seam

```cpp
class IAnalysisPass {
public:
    virtual ~IAnalysisPass() = default;
    virtual void Analyze(const CompiledArtifact&, CookDiagnosticSink&) = 0;
};
```

Examples (all optional, all out-of-scope to implement now):

- **`RgaAnalysisPass`** — feed bytecode to AMD RGA, attach ISA + register
  pressure summaries to diagnostics.
- **`PsoStatsPass`** — record bytecode size, resource counts, into a CSV.
- **`ValidationPass`** — re-run DXC validator with stricter rules.

These plug in at the cook orchestrator level. Their failure can be either
warning or error per project policy.

## 15. Runtime ↔ Offline Boundary (and Editor Hot Reload)

This is the chapter the user explicitly asked for.

### The seam

**Exactly one thing crosses the offline/runtime boundary: the cooked binary
file format.** Schema in `Engine/RHI/Public/Shaders/CookedShaderPackage.h`.

That's it. No shared headers between the tool and runtime that aren't part of
the format contract. No "common shader options struct" shared by both. No
in-process compile fallback. No DLL the runtime loads from the tool.

### Why the rule is this strict

If runtime ever links DXC (or any compiler), three things happen:

1. **Shipping size explodes.** DXC + its dependencies are tens of MB.
2. **Determinism dies.** End-user machines now have a different toolchain
   than the build farm. "Works on my machine" becomes "works on my GPU
   driver's compiler".
3. **The validator breaks down.** The whole point of the boundary is that
   you can audit "no shader compilation at runtime" with one CMake check.
   `ValidateShaderCompilerBoundary.cmake` enforces it today.

### Runtime side responsibilities

`Engine/RHI` owns:

- The `CookedShaderPackage*` schema.
- The reader (`CookedShaderPackageReader`) — opens the file, validates
  magic+version, exposes typed views over records and the binary blob.
- A small `ShaderResource` handle the renderer holds.
- A version counter on each `ShaderResource` so consumers (PSO cache, material
  cache) can invalidate when a package is reloaded.

`Engine/Renderer` owns:

- PSO and root-signature construction from `ShaderReflection` + cooked
  bytecode.
- Caching keyed on `ShaderResource` version + render-state hash.

`Engine/Editor` owns:

- The editor UI surfaces (recook button, status panel).
- The `ShaderRecookWatcher` (see below).

### Editor hot reload — how it works

The rule "runtime never compiles" still has to support editor iteration. The
target design solves it with **out-of-process recook**:

```text
┌────────────────────────────────────────────────────────────────┐
│ Editor process (links Engine/RHI, NOT Tools/ShaderCompiler)    │
│                                                                │
│  User saves Materials/BasicLit.hlsl                            │
│         │                                                      │
│         ▼                                                      │
│  ShaderRecookWatcher detects file change                       │
│  (or user clicks "Recompile shaders")                          │
│         │                                                      │
│         ▼                                                      │
│  Editor SPAWNS ShaderCompiler.exe with cook verb               │
│         │                                                      │
│         ▼                                                      │
│  Editor waits on process exit + reads stderr (JSON Lines)      │
│         │                                                      │
│         ├─ exit ≠ 0 → show diagnostics in UI, do not reload    │
│         └─ exit = 0 + ShaderRecookSignal updated:              │
│                  │                                             │
│                  ▼                                             │
│            For each package whose hash changed:                │
│              RHI re-opens *.spkg                               │
│              ShaderResource.Version++                          │
│                  │                                             │
│                  ▼                                             │
│            Renderer's PSO cache notices version bump,          │
│            evicts dependent PSOs, recreates lazily             │
└────────────────────────────────────────────────────────────────┘
```

Properties of this design:

- **Process isolation.** A compiler crash or DXC misbehavior cannot take down
  the editor.
- **Same code path as CI.** The editor uses *the exact same tool* CI uses.
  No "editor-only compile path" to drift out of sync.
- **Cache reuse.** The DDC means the recook is fast: only changed shaders
  miss the cache.
- **Boundary preserved.** `ValidateShaderCompilerBoundary.cmake` still passes
  — the editor links no compiler.

### What goes across the seam at recook time

```text
Editor process                       ShaderCompiler.exe process
──────────────                       ──────────────────────────
spawn(args) ──────────────────────► main(argv)
                                     [cook]
                                     read manifest
                                     run dependency graph
                                     write *.spkg files
                                     write registry
                                     write ShaderRecookSignal
                                     emit JSON diagnostics on stderr
                                     exit(code)
on exit ◄─────────────────────────── exit(code)

read stderr (JSON Lines)
read ShaderRecookSignal
diff registry vs previous registry
for each changed package:
    reader.Reopen(package_path)
    shaderResource.Version++
```

### What does *not* go across the seam

- No in-process function calls.
- No shared mutable state.
- No DXC objects, ever.
- No partial in-memory bytecode handed off — always a complete `*.spkg` file
  reaches the runtime through the file system.

### Hot reload edge cases (design notes)

- **Reflection changed.** If new shader has different bindings than old, the
  PSO using it is no longer valid. The version bump invalidates it; the next
  draw rebuilds it from the new reflection.
- **Recook in progress while runtime reads.** Writers write to a temp file
  and rename atomically; readers either see the old or the new, never a
  partial file.
- **Recook fails.** Old artifacts remain on disk. Runtime keeps using them.
  Editor surfaces the diagnostic.
- **Schema version mismatch.** Reader refuses with a clear error message
  pointing to the recook command. No best-effort partial load.

## 16. CLI & Automation Surface

Verbs (today: `cook`, `inspect-manifest`; targets cleanly extensible):

```text
ShaderCompiler.exe <verb> [options]

verbs:
  cook                 cook all packages declared in manifests
  cook --package <id>  cook one package
  inspect-manifest     parse + validate + print merged manifest
  inspect-package <id> dump cooked package contents
  list-targets         print supported ShaderTarget values
  --json               emit machine-readable diagnostics on stderr
  --no-cache           force full recook (CI smoke)
  --cache-dir <path>   override default cache location
```

Exit codes (subset of `ShaderCompilerConstants.h`):

- `0` success
- `1` usage error
- `5` manifest failure
- `6` cook failure (one or more packages failed to compile / write)

JSON Lines diagnostic schema (one object per line on stderr when `--json`):

```json
{"sev":"error","code":"SC1001","file":"...","line":12,"col":7,
 "msg":"undeclared identifier 'foo'",
 "ctx":[{"package":"BasicLit","permutation":"SHADOWS=On","stage":"Pixel"}]}
```

This is the contract editor + CI both consume.

## 17. Threading Posture (Single-Threaded Today, Parallel-Ready)

The system is **single-threaded today**. Deliberately. Reasons:

- Easier to debug when the engine is small.
- DXC has its own thread-safety story; pinning it to one thread sidesteps
  questions until they matter.
- A serial executor produces deterministic logs trivially.

But the *architecture* is parallelism-ready:

- The cook is a **DAG of independent `CookNode`s**. The executor is an
  interface (`ICookExecutor`). Today's `SerialCookExecutor` walks topological
  order. A future `ParallelCookExecutor` runs ready nodes concurrently.
- The cache (`IShaderArtifactStore`) is designed to be safe against
  concurrent get/put; the on-disk store uses temp-file + rename.
- Diagnostics are routed through `CookDiagnosticSink`; today the sink is
  trivially thread-unsafe. Tomorrow it grows a mutex or a per-thread queue.
- `IShaderBackend` instances are kept per-thread when parallelism arrives;
  the orchestrator never assumes a single backend.

The lesson from Unreal's `ShaderCompileWorker` farm: parallelism is a
*scheduler* problem on top of a clean unit-of-work, not a property to bolt
on later. Get the unit-of-work right now; add the scheduler when the cook
time hurts.

## 18. What Production Engines Do (Reference Distillation)

Five reference points. Each: how they do it, what Sparkle borrows, what
Sparkle deliberately skips.

### Unreal Engine

How they do it:

- `FShaderType` declares a shader (vertex/pixel/compute/etc.) plus its
  permutation domain (`FShaderPermutationParameters`,
  `ShouldCompilePermutation`).
- `ShaderCompileWorker` is a separate executable that compiles. The editor
  spawns instances of it (a farm) — *not* in-process compile.
- The DDC (Derived Data Cache) is content-addressed and shared across team
  members.
- Reflection is normalized into `FShaderParameterMap` consumed by the renderer.

Sparkle borrows: out-of-process worker model, content-addressed cache,
package-as-unit-of-shipping.

Sparkle skips: distributed farm, materials-as-shader-graphs, multi-cooker
DDC sharding.

### Frostbite

How they do it (per public SIGGRAPH talks):

- A "ShaderDB" describes all shaders + permutation axes declaratively.
- Aggressive permutation **pruning** keeps the matrix tractable.
- Offline-only compilation; runtime is a reader.
- Heavy investment in iteration time: DDC + recook-on-change.

Sparkle borrows: declarative manifest, pruning as a first-class concern,
offline-only.

Sparkle skips: their scale of permutation infrastructure; their
material/visual-graph layer.

### Slang

How they do it:

- A shader *language* (superset of HLSL) with modules, generics, interfaces.
- Multi-target codegen (DXIL/SPIR-V/Metal/CUDA/...) from a shared IR.
- Separate compilation + linking of shader modules.

Sparkle borrows: the *idea* of `ShaderTarget` enum + neutral IR-shaped
artifact, the discipline of letting the orchestrator be backend-agnostic.

Sparkle skips: inventing a language; building an IR; separate compilation —
Sparkle compiles whole packages.

### DirectXShaderCompiler (DXC)

How it's structured:

- LLVM/Clang fork. `dxc.exe` is a thin driver around `dxcompiler.dll`.
- The library is the contract; the executable is convenience.
- SPIR-V codegen is a backend selection inside the same compiler.

Sparkle borrows: the driver/library split mirrored at one level up — the
Sparkle CLI is a thin driver over orchestration code that could equally be
called from another host.

Sparkle skips: building a compiler. DXC *is* the backend; Sparkle adapts it.

### AMD Radeon GPU Analyzer

How it works:

- Offline compile + analysis: ISA disassembly, register usage, control flow
  graphs, stats per AMD architecture.

Sparkle borrows: the *seam* — `IAnalysisPass` lets RGA-style passes plug in
later without changing the core pipeline.

Sparkle skips: shipping any vendor analyzer in the box; vendor ISA reporting.

## 19. Sparkle Today vs Target (Gap Matrix)

| Concern | Today | Target | Example file (today) |
|---|---|---|---|
| CLI dispatch | `CommandRegistry`, two verbs | same shape, more verbs, `--json` | `Tools/ShaderCompiler/Private/Cli/CommandRegistry.cpp` |
| Manifest | hand-rolled INI parser | `IManifestSource` + versioned schema | `Tools/ShaderCompiler/Private/Manifest/ShaderCookManifestParser.cpp` |
| Validation | rule list in one validator | same, more rules, structured errors | `Tools/ShaderCompiler/Private/Manifest/ShaderCookManifestValidator.cpp` |
| Permutations | none (single variant per package) | `PermutationExpander` + `ShaderPermutationKey` | (new) |
| Dependency graph | implicit (loop) | explicit `DependencyGraph` of `CookNode`s | (new) |
| Cache | none (always recompiles) | `IShaderArtifactStore` content-addressed | (new) |
| Backend abstraction | `DxcShaderCompiler` directly called | `IShaderBackend` + `DxcShaderBackend` adapter | `Tools/ShaderCompiler/Private/Compiler/DxcShaderCompiler.cpp` |
| Reflection | not extracted | `IReflectionExtractor` + `ShaderReflection` | (new) |
| Cooked package | binary with header + records | same, schema already neutral | `Engine/RHI/Public/Shaders/CookedShaderPackage.h` |
| Schema versioning | `Magic` + `Version` already present | add migration playbook + loader policy | `Engine/RHI/Public/Shaders/CookedShaderPackage.h` |
| Diagnostics | log strings | `CookDiagnosticSink` + JSON Lines + source maps | (new) |
| Editor hot reload | not implemented | spawn-tool + `ShaderRecookSignal` + watcher | (new) |
| Boundary enforcement | CMake script in place | keep, extend with new forbidden tokens | `CMake/Validation/ValidateShaderCompilerBoundary.cmake` |
| Threading | serial loop | serial executor + DAG-ready interface | `Tools/ShaderCompiler/Private/Cooking/ShaderPackageCooker.cpp` |

## 20. Phased Evolution Roadmap

Each phase: scope, files touched (illustrative), exit criteria.

### Phase 0 — Stabilize current state

- **Scope.** Restore full build; add a smoke cook in CI; document current
  manifest format precisely.
- **Files.** existing `Tools/ShaderCompiler/*`.
- **Exit criteria.** `ShaderCompiler cook` succeeds end-to-end on the
  Showcase project; CI runs `cook` + `inspect-manifest` on every PR.

### Phase 1 — Dependency graph + content-addressed cache

- **Scope.** Introduce `CookNode`, `DependencyGraph`, `ICookExecutor`
  (serial), `ShaderCacheKey`, `IShaderArtifactStore` (local on-disk).
- **Files.** New `Tools/ShaderCompiler/Private/Cook/` directory; refactor
  `ShaderPackageCooker` to drive the graph instead of looping.
- **Exit criteria.** Re-running `cook` with no changes does zero compile
  invocations and finishes in milliseconds.

### Phase 2 — Backend abstraction + reflection

- **Scope.** Extract `IShaderBackend`. Move DXC behind `DxcShaderBackend`.
  Introduce `ShaderTarget`. Extract `IReflectionExtractor` + `ShaderReflection`.
  Wire reflection through to the cooked package.
- **Files.** New `Tools/ShaderCompiler/Backends/`. Cooked package writer
  consumes `ShaderReflection` instead of layout directly.
- **Exit criteria.** No file outside `Tools/ShaderCompiler/Backends/Dxc/`
  mentions DXC. Renderer consumes `ShaderReflection` only.

### Phase 3 — Permutations

- **Scope.** Introduce `ShaderPermutationKey`, axis declarations in manifest,
  `PermutationExpander`, pruning rules.
- **Files.** `Tools/ShaderCompiler/Private/Permutation/`. Cooked package
  gains permutation key in record metadata.
- **Exit criteria.** A package can declare ≥2 axes and the runtime can pick
  the right variant by key.

### Phase 4 — SPIR-V adapter (Vulkan readiness)

- **Scope.** Implement `SpirVShaderBackend` (likely via DXC's SPIR-V codegen).
  Introduce a `SpirVReflectionExtractor`. Verify the cooked package emits
  `CookedShaderBinaryFormat::SpirV` correctly.
- **Files.** `Tools/ShaderCompiler/Backends/SpirV/`.
- **Exit criteria.** A trivial compute shader cooks to SPIR-V and the cooked
  package round-trips through the RHI reader. (Renderer Vulkan path is a
  separate effort; this phase only proves the cooker.)

### Phase 5 — Editor hot reload + analysis hooks

- **Scope.** `ShaderRecookSignal` emission; editor-side `ShaderRecookWatcher`;
  ShaderResource version bumping. Optional `IAnalysisPass` seam.
- **Files.** `Engine/Editor/ShaderRecook/`, `Engine/RHI/Shaders/Reload.h`.
- **Exit criteria.** Saving an `.hlsl` file with the editor open triggers a
  recook and the editor viewport reflects the new shader without restart.

## 21. Open Questions

These are decisions the design intentionally defers:

1. **Manifest format.** Stay INI-but-formalized, or move to JSON/TOML? The
   `IManifestSource` seam means the choice can be deferred without blocking
   other phases.
2. **Permutation key strategy.** Bitfield vs sorted (axis,value) string hash.
   Bitfield is faster but brittle when axes are added; string hash is
   resilient but bigger.
3. **Cache storage location.** Per-user `%LOCALAPPDATA%` vs in-tree
   `bin/Cache/`. Per-user is friendlier; in-tree is easier to wipe.
4. **Hot reload trigger source.** OS file watcher (`ReadDirectoryChangesW`)
   vs explicit "Recompile" button vs both. Recommend *both*: button is
   reliable, watcher is convenient.
5. **Reflection coverage at first cut.** Bindings only, or also constant
   buffer member layout? Bindings are enough for PSO building; member layout
   matters when materials are added.
6. **Schema migration policy.** Single-version-only (recook everything) vs
   read-old-versions for one release. Recommend single-version while the
   engine is small.

## 22. Appendix A — Object Catalog Quick Reference

```text
Front-end:
  ShaderManifest          parsed + validated authoring document
  IManifestSource         pluggable parser (ini today)
  ShaderPackageDesc       declared package
  ShaderStageDesc         declared stage

Plan:
  PermutationExpander     desc → list of compile requests
  ShaderPermutationKey    stable hash of axis values
  ShaderCompileRequest    immutable unit of work
  ShaderTarget            target API + profile enum

Source layer:
  ShaderIncludeResolver   resolves #include against roots
  ShaderSourceDatabase    indexes sources, computes hashes, builds include graph

Cook graph + execution:
  CookNode                graph node wrapping a request
  DependencyGraph         DAG of cook nodes
  ICookExecutor           serial today, parallel-ready

Cache:
  ShaderCacheKey          content-addressed key
  IShaderArtifactStore    get/put for cache key → artifact

Backend:
  IShaderBackend          compile interface
  ShaderBackendCapabilities  query target/feature support
  DxcShaderBackend        adapter over IDxcCompiler3 (today)
  SpirVShaderBackend      designed-for, not implemented

Reflection:
  IReflectionExtractor    backend-specific reflection extraction
  ShaderReflection        normalized resource layout

Output:
  CookedPackageBuilder    assembles in-memory package layout
  CookedShaderPackageWriter  serializes to *.spkg using RHI schema
  CookedRegistryWriter    writes human-readable index

Diagnostics:
  CookDiagnosticSink      structured diagnostic receiver
  SourceMap               post-preprocess line → original line

Editor / hot reload:
  ShaderRecookSignal      marker file emitted on successful cook
  ShaderRecookWatcher     editor-side watcher, bumps ShaderResource versions
```

## 23. Appendix B — Glossary

- **Backend compiler** — language toolchain that turns source into bytecode
  (DXC, glslang, slangc).
- **Cook** — process of producing runtime-ready artifacts from source +
  manifest.
- **Cooked package** — on-disk binary file the runtime consumes; schema in
  `Engine/RHI/Public/Shaders/CookedShaderPackage.h`.
- **DDC (Derived Data Cache)** — content-addressed cache of derived
  artifacts. Sparkle's `IShaderArtifactStore` is its small DDC.
- **Dependency graph** — DAG of cook nodes capturing what each unit of work
  depends on; the parallelism contract.
- **Manifest** — declarative file telling the cooker what packages exist and
  what stages they contain. Authoring input. Not shader code.
- **Package** — a named bundle of stages cooked + shipped + bound together.
  Unit of runtime binding.
- **Permutation / variant** — one specific compile of a package or stage
  parameterised by `#define`s, target, quality flags.
- **Pipeline (engine sense)** — the orchestration system around a backend
  compiler: planning, caching, packaging, registry. What this document is
  about.
- **Reflection** — extracted layout metadata describing what bytecode expects
  from the runtime (bindings, CBs, signatures).
- **Source map** — mapping from post-preprocess source positions to original
  source positions, attached to diagnostics.
- **Stage** — a programmable pipeline step (vertex, pixel, compute, mesh).
- **Target** — the (api, profile, model) tuple the backend compiles for.

## 24. Appendix C — Reading List

- DXC (Microsoft) — `https://github.com/microsoft/DirectXShaderCompiler`
- Slang (NVIDIA-led) — `https://github.com/shader-slang/slang`
- AMD Radeon GPU Analyzer — `https://github.com/GPUOpen-Tools/radeon_gpu_analyzer`
- Unreal Engine documentation portal — `https://dev.epicgames.com/documentation/en-us/unreal-engine`
- Unreal `ShaderCompileWorker` (in-engine source) — search the UE source
  tree for `ShaderCompileWorker.cpp` and `FShaderCompilingManager`.
- Frostbite "Rapid Iteration with Shaders" / "Moving Frostbite to PBR" —
  public SIGGRAPH talks describing their shader pipeline shape.
