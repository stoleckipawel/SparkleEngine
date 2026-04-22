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
- **Manifest** — a declarative package-definition file authored by a human
  that tells the cooker *which packages exist* and *which stages each
  contains*. The concept is common across engines, but the exact word is not
  universal; other systems may call the same thing a descriptor, shader
  database, registry input, or definition file. It is the authoring input
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
- Initial scope is **global shaders only**: renderer/engine-owned shaders
  declared in C++ and backed by `.hlsl` / `.hlsli` source files.
- Incremental, content-addressed cook — unchanged inputs reuse previous output.
- Backend-agnostic orchestration — DXIL today, SPIR-V designed for, no churn
  in the orchestration layer when a new backend lands.
- Stable, versioned cooked artifact format owned by RHI.
- Structured diagnostics — machine-readable errors with source maps.
- First-class visibility into shader compilation products — preprocessed
  source, reflection output, backend disassembly, and intermediate
  representations when the backend can expose them.
- Editor hot reload via out-of-process recook + signal, not in-process compile.
- Single-threaded executor today; the dependency graph is the parallelism
  contract for tomorrow.

### UE-Inspired Design Commitments We Are Adopting

These are not just references or recommendations anymore. They are part of the
target Sparkle design.

- **Clear runtime vs offline ownership.** Runtime and editor consume cooked
  shader artifacts; the offline tool owns compilation work.
- **Out-of-process compile orchestration.** Shader compilation runs through
  `ShaderCompiler.exe`, including editor recook workflows.
- **Explicit permutation domain model.** Permutations are represented through
  `ShaderPermutationDomainDesc` and `ShaderPermutationVector`, not loose define
  bags.
- **Content-addressed shader caching.** Cache lookup is part of the normal cook
  path, not a future bolt-on.
- **One cook path for editor and automation.** The editor should trigger the
  same offline cook path CI and scripts use.
- **Cooked-artifact-only runtime contract.** Runtime loads only cooked shader
  packages through RHI-owned schema and readers.
- **Backend abstraction from the start.** DXC is the first adapter, not the
  permanent shape of the orchestration layer.
- **Debug artifact capture is intentional.** Shader compilation should produce
  optional inspection artifacts for debugging and learning, not hide all
  intermediate forms behind the backend boundary.

**Non-goals (explicitly out of scope)**

- Multithreaded executor implementation.
- Distributed compile workers (Unreal-style farm).
- Shared/remote DDC implementation (interface seam only).
- A new shader language or IR.
- In-editor in-process recompilation.
- Vendor-specific analysis (RGA-style ISA disassembly) as a built-in — left as
  an `IAnalysisPass` seam.
- Unreal-scale material graph driven shader generation.
- Unreal-scale shader type taxonomy and vertex-factory matrix.
- Copying Unreal's class, macro, or naming surface literally.
- Material/uasset shader workflows in the initial shipping version.
- Project-authored shader package manifests in the initial shipping version.

## 4. End-User Mental Model

Three personas interact with the system. Each has a different surface.

**Engine programmer.** Edits `.hlsl` source, edits binding layout C++ code,
runs the cook tool (or the editor, which triggers a recook). Cares about:
diagnostics quality, cook speed, deterministic output, never having to think
about runtime fallback.

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

This is an adopted design rule, not just a possible implementation option:

```text
Editor/CI request work.
ShaderCompiler.exe owns compile orchestration.
RHI/Renderer consume cooked results only.
```

## 7. Core Abstractions / Object Catalog

Each entry: **role**, **owns**, **depends on**, **lifetime**.

### Front-end (authoring intent → in-memory description)

- **`ShaderRegistrationDesc`** — C++-declared shader registration record for
  Unreal-style engine/global shader workflows. Captures source path, entry
  point, stage, shader family id, and optional permutation domain metadata.
- **`IShaderRegistrationSource`** — pluggable registration source interface.
  Initial implementation is C++ registration compiled into the engine/tool.
  Future implementations may load generated registrations or declarative files.
- **`ShaderPackageDesc`** — normalized internal package/family description
  produced from registrations. Even in a global-shader-only system, the cooker
  still benefits from a normalized internal model.
- **`ShaderStageDesc`** — stage enum + source path + entry point. Plain data.

### Plan (intent → unit-of-work)

- **`PermutationExpander`** — turns one `ShaderPackageDesc` into N
  `ShaderCompileRequest`s by walking a declared permutation domain and
  materializing concrete permutation vectors. Pure function.
- **`ShaderPermutationDomainDesc`** — typed declaration of the legal
  permutation dimensions for a package or shader family.
- **`ShaderPermutationVector`** — one concrete choice of values inside a
  permutation domain. Used to derive compile definitions and form cache keys.
- **`ShaderPermutationKey`** — stable hash of a `ShaderPermutationVector`
  under a specific domain schema version. Used as part of `ShaderCacheKey`.
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
  Returns bytecode + raw reflection blob + diagnostics + optional debug
  artifacts/intermediate outputs.
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
- **`ShaderDebugArtifactSet`** — optional bundle of offline inspection
  artifacts: preprocessed source, reflection dump, include graph snapshot,
  compile-request replay data, backend disassembly, and backend-specific
  intermediate representation files.

### Editor / hot reload (design-only seams)

- **`ShaderRecookSignal`** — a small marker file or named event the cook tool
  emits on successful completion. The runtime watcher consumes it.
- **`ShaderRecookWatcher`** — editor-side file watcher that observes the
  signal, asks RHI to re-open changed packages, and bumps shader-resource
  version numbers so PSO caches invalidate.

## 8. Authoring Front-End: Global Shader Registration First

The initial Sparkle design should not be manifest-first. It should be
**global-shader-registration-first**.

That means the primary authoring workflow is:

- write `.hlsl` / `.hlsli` files
- declare shaders in C++
- point registration at source path + entry point + stage
- attach optional permutation-domain metadata in C++
- let the offline cooker discover those registrations and compile them

This is much closer to how Unreal global shaders feel in practice, and it is a
better fit for the current Sparkle scope because there are no material/uasset
shader workflows yet.

### Why this is the right simplification now

If Sparkle supports only global shaders initially, a manifest file adds more
surface area than value.

For global shaders, the natural source of truth is usually the renderer or
engine feature that owns the shader. That makes C++ registration a better fit:

- the shader is declared next to the engine feature that uses it
- source path, stage, and entry point live in one obvious place
- permutation-domain setup can live next to registration
- there is no need to invent a project/content authoring format before content
  shaders exist

So the design choice should be:

```text
Initial Sparkle scope:
  global shaders only
  C++ registration is the primary authoring surface

Future extension:
  declarative shader package files only if content/uasset shader workflows
  appear and genuinely need them
```

### What Unreal-style C++ registration looks like in Sparkle

The C++ side should feel familiar if you are used to Unreal:

```cpp
struct ShaderRegistrationDesc {
    std::string_view ShaderId;
    std::string_view ShaderFamilyId;
    ShaderStage      Stage;
    std::string_view SourcePath;
    std::string_view EntryPoint;
    std::string_view BindingLayoutId;
    ShaderTarget     Target;
    ShaderPermutationDomainDesc PermutationDomain;
};

RegisterShader({
    .ShaderId = "FullscreenBlitPS",
  .ShaderFamilyId = "FullscreenBlit",
    .Stage = ShaderStage::Pixel,
    .SourcePath = "/Engine/Shaders/PostProcess/FullscreenBlit.hlsl",
    .EntryPoint = "MainPS",
    .BindingLayoutId = "FullscreenPass",
    .Target = ShaderTarget::DxilSm66,
    .PermutationDomain = MakeDomain<UseGammaCorrection>()
});
```

You could register a vertex shader and pixel shader into the same shader family
in C++, and the cooker would normalize those registrations into one internal
description for compilation and packaging.

That gives you the Unreal-like workflow you are asking for:

- write `.hlsl` / `.hlsli` shader files
- declare VS / PS / CS entry points in C++
- attach permutation-domain metadata in C++
- let the offline compiler discover those registrations and cook them

Under this initial scope, there is no need for a manifest at all.

### Where manifests still fit later

The earlier manifest discussion is still useful as a **future extension point**,
not as an initial requirement.

If Sparkle later grows into project-authored shader packages, content shaders,
or asset-driven shader groups, then a declarative file format may become useful.
But that should happen only when the engine actually has that category of work.

For now, the recommendation is simple:

```text
Do what Unreal global shaders do:
  register shaders in C++
  compile them offline
  keep runtime limited to cooked artifacts
```

### Why you need it in practice

If Sparkle only ever compiled one shader file by hand, a manifest would be
overkill. You could just run something like:

```text
dxc BasicLit.hlsl -E PSMain -T ps_6_6
```

That stops working the moment the engine needs more than "compile this one
file once":

- one package contains multiple stages that must stay compatible
- the package has a binding layout id the runtime needs to agree with
- the same shader has multiple variants or quality levels
- the tool needs to know what output package name/path to produce
- the editor or CI needs one stable input file that declares the whole set

The manifest exists so the cooker can answer questions like:

- Which packages are part of this project?
- Which stages belong to each package?
- Which entry points should be used?
- Which binding layout does this package expect?
- Which variants should be cooked?

So the short version is:

```text
source file (.hlsl) = shader code
manifest            = build declaration for shader packages
cooker              = tool that reads both and emits cooked runtime artifacts
```

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

Read that example as:

- define one package named `BasicLit`
- use the runtime binding layout `StandardMaterial`
- cook the `Default` variant
- compile `VSMain` as the vertex stage
- compile `PSMain` as the pixel stage

Slightly richer example with two packages:

```ini
[ShaderCookManifest]
Version = 1

[Package BasicLit]
BindingLayout = StandardMaterial
Variant       = Default
Stage.Vertex  = Materials/BasicLit.hlsl | VSMain
Stage.Pixel   = Materials/BasicLit.hlsl | PSMain

[Package ShadowDepth]
BindingLayout = ShadowPass
Variant       = Default
Stage.Vertex  = Shadow/ShadowDepth.hlsl | VSMain
```

This tells the cooker:

- build a `BasicLit` package for the main material pass
- build a separate `ShadowDepth` package for shadow rendering
- each package has its own binding layout and stage set

Without a manifest, that information has to live somewhere else anyway:

- hardcoded in C++
- buried in scripts
- duplicated in the editor
- duplicated in CI/build files

That is why production systems usually want some declarative input file, even
if they do not literally call it a manifest.

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

The right mental model here is the one Unreal uses:

- a **permutation domain** declares which dimensions exist
- a **permutation vector** chooses one concrete value for each dimension
- the compile environment is derived from that vector
- `ShouldCompilePermutation`-style logic prunes illegal or wasteful cases

That is better than treating permutations as a loose bag of defines. It gives
the system structure, validation rules, and a stable identity.

Target design:

- Each package or shader family declares a **`ShaderPermutationDomainDesc`**.
  That domain defines the legal dimensions, their value sets, and the schema
  version. Example dimensions:

  - `USE_SHADOWS = {0,1}`
  - `USE_NORMAL_MAP = {0,1}`
  - `QUALITY = {Low, High}`

- The expander walks the domain and produces concrete
  **`ShaderPermutationVector`** instances. A vector is one specific choice,
  for example:

  - `USE_SHADOWS=1`
  - `USE_NORMAL_MAP=0`
  - `QUALITY=High`

- The vector is the source of truth for compile definitions. The backend does
  not receive a hand-assembled define bag from random call sites. It receives
  a normalized vector, and the compile environment is derived from it.
- Pruning rules are first-class and live next to the domain, not buried in
  ad-hoc scripts. This is the equivalent of Unreal's
  `ShouldCompilePermutation`: if a combination is invalid, unsupported, or not
  worth the cook cost, it never becomes a compile request.
- A **`ShaderPermutationKey`** is a stable hash of the
  `ShaderPermutationVector` plus the domain schema version. It is part of
  `ShaderCacheKey` and embedded in the cooked package so the runtime can look
  up the right variant.
- The runtime never *computes* permutations. It looks them up by key.

Small example:

```text
Domain:
  USE_SHADOWS   = {0,1}
  QUALITY       = {Low,High}

Candidate vectors:
  { USE_SHADOWS=0, QUALITY=Low }
  { USE_SHADOWS=0, QUALITY=High }
  { USE_SHADOWS=1, QUALITY=Low }
  { USE_SHADOWS=1, QUALITY=High }

Prune rule:
  if USE_SHADOWS=0, force QUALITY=Low

Cooked vectors:
  { USE_SHADOWS=0, QUALITY=Low }
  { USE_SHADOWS=1, QUALITY=Low }
  { USE_SHADOWS=1, QUALITY=High }
```

That gives Sparkle the same core benefit Unreal gets from permutation domains:
definitions are no longer just strings; they are typed configuration choices
with stable identity, explicit pruning, and deterministic cache keys.

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

For Sparkle, backend capability should also cover a tooling question:

*which intermediate forms and debug artifacts can this backend expose?*

Examples:

- DXC path may expose preprocessed HLSL, reflection blobs, DXIL disassembly,
  compiler replay arguments, and debug-info sidecar outputs.
- Future SPIR-V path may expose SPIR-V binary, SPIR-V text disassembly, and
  reflection dumps.

Sparkle should treat those outputs as intentional offline products. They are
useful for debugging, learning, and validating the pipeline, even when they do
not participate in runtime loading.

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

### Intermediate representations and debug artifact bundles

For this engine, access to intermediate representations is a feature, not a
luxury. The system should be able to emit a structured debug bundle per shader
compile or package when requested.

```text
DebugArtifactBundle/
  compile-request.json
  defines.json
  include-graph.json
  preprocessed-source.hlsl
  reflection.json
  backend-ir.bin / backend-ir.txt
  disassembly.txt
  compiler-stderr.txt
```

These artifacts exist for offline inspection only. They let you:

- see what the backend actually compiled after preprocessing
- understand how a permutation vector translated into concrete defines
- inspect reflection output against the runtime binding model
- study backend-generated disassembly or IR for learning/debugging
- replay or reproduce a compile outside the engine when necessary

That is especially useful for Sparkle because part of the value of the system is
that it should teach you what the compiler and backend are doing, not just hide
the result behind a final bytecode blob.

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

Recommended ordering for Sparkle:

- first build the debug artifact capture path
- then expose backend disassembly and intermediate forms where available
- only after that consider heavyweight vendor analyzers like RGA

That gives immediate learning/debug value without making the initial system
depend on any vendor-specific tool.

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
package-as-unit-of-shipping, and the idea that permutations should be modeled
as an explicit domain with code-level pruning rather than as unstructured
define lists.

Sparkle skips: distributed farm, materials-as-shader-graphs, multi-cooker
DDC sharding.

#### Unreal runtime vs offline mental model

The easiest way to understand Unreal is to stop looking for one manifest file
and instead look at the **split of responsibilities**.

```text
AUTHORING / DECLARATION
  .usf / .ush shader files
  C++ shader type registration
  material system
  vertex factory types
        │
        ▼
RUNTIME / EDITOR PROCESS
  UnrealEditor.exe or the game process
  owns materials, shader maps, render resources, PSOs
  decides a shader map is missing / stale
        │
        ▼
OFFLINE COMPILE PATH
  build compile jobs
  send them to ShaderCompileWorker processes
  compile for target platform
  return bytecode + parameter/reflection info
        │
        ▼
CACHE / COOKED OUTPUT
  store results in DDC
  use cooked shader data during packaging/cooking
  material shaders end up in cooked asset data
  global shaders are stored separately for startup/runtime use
```

That leads to one important conclusion:

- Unreal's **runtime/editor process** is the owner of shader usage,
  materials, shader maps, and render resources.
- Unreal's **offline/worker side** is the owner of heavy compilation work,
  platform compiler invocation, and derived shader artifacts.

It is not a perfectly pure separation in the abstract, because Unreal can be
configured to compile more directly for debugging, but the production mental
model is still: **runtime requests, worker compiles, DDC caches, cooked builds
consume results**.

More concretely:

- Runtime/editor decides *what is needed*.
  Example: a material needs a shader map for a given platform and vertex
  factory combination.
- Shader type registration decides *what may exist*.
  Example: `FShaderType`, `FMaterialShaderType`, and
  `FMeshMaterialShaderType` define the legal shader families.
- Permutation domain logic decides *which combinations are legal*.
  Example: `ShouldCompilePermutation` and compile-environment setup prune the
  matrix.
- ShaderCompileWorker decides *how compilation is executed*.
  Example: helper processes call the platform compiler rather than doing all
  work inline in the editor process.
- DDC decides *whether work can be skipped*.
  Example: if the shader inputs hash to an existing cached result, UE reuses
  it instead of recompiling.
- Cooking/package output decides *how runtime receives the result*.
  Example: cooked builds ship shader data as derived/cooked content, not as
  source that end users compile on their machines.

This is the part Sparkle should copy conceptually.

Sparkle does **not** need to copy Unreal's full shader-class hierarchy,
material graph system, or distributed worker farm. But it **should** copy the
decision split:

- authoring/declaration layer says what may be built
- runtime/editor layer decides what it needs
- offline compiler layer performs compilation work
- cache layer avoids recompiling unchanged inputs
- cooked output layer is the only thing runtime consumes

That is the real architectural lesson from Unreal, not the exact class names.

#### Sparkle vs Unreal side-by-side

The point of this comparison is not to say Sparkle should become Unreal.
It is to make the architectural choices legible.

##### Side-by-side flow

```text
SPARKLE TARGET DESIGN                             UNREAL PRODUCTION MODEL
──────────────────────────────────────────        ──────────────────────────────────────────
Author edits:                                    Author edits:
  - .hlsl / .hlsli files                           - .usf / .ush files
  - C++ shader registration                        - material graphs / settings
  - permutation-domain metadata                    - C++ shader type registration
                                                   - vertex factory / platform rules

        │                                                      │
        ▼                                                      ▼
Planning layer:                                     Runtime/editor decides need:
  - discover C++ registrations                        - material needs shader map
  - normalize shader families                         - pass / VF / platform combination
  - expand permutation domain                         - stale or missing shader job

        │                                                      │
        ▼                                                      ▼
Offline compile layer:                               Offline compile layer:
  - ShaderCompiler.exe                                 - ShaderCompileWorker
  - IShaderBackend                                     - platform compiler invocation
  - serial executor today                              - async / multi-worker scheduling

        │                                                      │
        ▼                                                      ▼
Cache layer:                                         Cache layer:
  - local shader artifact store                        - DDC local/shared/cloud
  - content-addressed keys                             - content-addressed derived data

        │                                                      │
        ▼                                                      ▼
Cooked output:                                      Cooked output:
  - *.spkg + registry                                 - shader maps / cooked shader data
  - RHI-owned schema                                  - cooked packages + global shader data

        │                                                      │
        ▼                                                      ▼
Runtime consumes only cooked data                    Runtime consumes only cooked data
  - RHI reader                                         - shader maps / global shaders
  - renderer builds PSOs                               - renderer builds PSOs/resources
```

##### Responsibility split

| Concern | Sparkle target design | Unreal production model | Takeaway |
|---|---|---|---|
| Authoring surface | `.hlsl` / `.hlsli` files + C++ registration | Distributed across shader files, C++ shader types, materials, vertex factories | Sparkle is simpler and more explicit; Unreal is more implicit and scalable |
| What declares legal shaders | C++ registrations + permutation domain descriptors | `FShaderType`, material shader types, vertex factory types, platform rules | Both are code-registration driven, but Sparkle stays much smaller in scope |
| What decides what must be compiled | Cook command + discovered registrations + permutation expansion | Runtime/editor discovers missing or stale shader maps | Sparkle is batch-first; Unreal is demand-driven in the editor |
| Permutation modeling | `ShaderPermutationDomainDesc` + `ShaderPermutationVector` + pruning | permutation domain parameters + `ShouldCompilePermutation` | This is the main Unreal idea Sparkle should borrow directly |
| Compile execution | `ShaderCompiler.exe` with backend adapters | `ShaderCompileWorker` helper processes | Same principle: heavy compile work should live out of process |
| Cache | Local artifact store first, shared later if needed | DDC local/shared/cloud | Same principle, different scale |
| Runtime contract | `*.spkg` schema owned by RHI | shader maps / cooked shader blobs owned by runtime systems | Runtime must consume cooked artifacts, not source |
| Editor hot reload | spawn tool, recook, reload package, bump version | editor triggers recompilation and reloads shader maps | Same workflow shape, Unreal just has more infrastructure around it |
| Platform scope | D3D12 first, Vulkan-ready interfaces | many platforms, many shader families | Sparkle should copy separation, not scale |

##### Decision-making split

| Question | Sparkle target owner | Unreal owner |
|---|---|---|
| What shaders exist? | C++ shader registration | shader type registration + material system |
| Which permutations are legal? | permutation domain + prune rules | permutation domain + `ShouldCompilePermutation` |
| Which permutations are needed right now? | cook command / package selection / future editor recook logic | editor/runtime shader-map demand |
| How are they compiled? | `IShaderBackend` adapter | worker process + platform compiler path |
| Can compilation be skipped? | `IShaderArtifactStore` key lookup | DDC key lookup |
| What does runtime load? | `*.spkg` through RHI reader | cooked shader maps / global shader data |

##### What Sparkle should copy vs not copy

| Copy | Do not copy blindly |
|---|---|
| Clear runtime vs offline ownership | Full Unreal shader class hierarchy |
| Out-of-process compile path | Material graph complexity |
| Explicit permutation domain + vector model | Distributed worker farm right now |
| Content-addressed cache keys | Full DDC product surface |
| Runtime consumes cooked data only | Every Unreal-specific naming convention |

The practical conclusion is:

```text
Sparkle should be Unreal-like in separation of concerns,
not Unreal-like in sheer amount of machinery.
```

##### UE-inspired workflow for Sparkle: adopt now vs defer

If the goal is a UE-inspired workflow without importing Unreal's full scale,
the right move is to separate ideas into three buckets.

| Bucket | Unreal-inspired element | Why it is worth it for Sparkle now |
|---|---|---|
| Adopt now | Clear runtime vs offline boundary | This is the most important architectural rule and gives immediate design clarity |
| Adopt now | Out-of-process shader compiler executable | Keeps DXC and compiler failures out of runtime/editor code |
| Adopt now | Explicit permutation domain + permutation vector model | Gives structured definitions, pruning, and stable cache identity |
| Adopt now | Content-addressed shader cache | Prevents pointless recompiles and aligns with production workflows |
| Adopt now | Editor-driven recook that spawns the offline tool | Matches the Unreal iteration loop without needing Unreal's full worker farm |
| Adopt now | Runtime only loads cooked shader artifacts | Keeps shipping/runtime behavior deterministic |
| Adopt now | Backend abstraction layer | Needed for Vulkan readiness without polluting the rest of the tool |

| Bucket | Unreal-inspired element | Why it should wait |
|---|---|---|
| Defer | Multiple worker processes / compile farm | Great for scale, but operationally heavy and unnecessary while Sparkle is single-threaded |
| Defer | Shared/cloud DDC product surface | Valuable later, but local cache is enough to prove the architecture first |
| Defer | Full material graph driven shader generation | This explodes scope and ties the shader system to a much bigger content pipeline |
| Defer | Huge shader type hierarchy like Unreal's | Sparkle needs the ownership split, not the entire taxonomy |
| Defer | Massive platform matrix from day one | D3D12 first plus a clean Vulkan-ready seam is the right scope |
| Defer | Deep editor tooling around shader maps, viewmodes, and diagnostics | Good later, but not required to prove a strong portfolio-grade pipeline |

| Bucket | Unreal-inspired element | Why it is too big for now |
|---|---|---|
| Avoid for now | Reproducing Unreal's material system architecture | It is a rendering/content framework problem, not just a shader compiler problem |
| Avoid for now | Reproducing all engine shader families and vertex factory combinations | Sparkle does not yet need Unreal's breadth of rendering abstractions |
| Avoid for now | Treating runtime/editor as the owner of compile orchestration complexity | Sparkle should keep compile orchestration concentrated in the offline tool |
| Avoid for now | Copying Unreal naming/macros/class shapes literally | The concepts matter; the exact surface does not |

Recommended Sparkle workflow, inspired by UE but scaled down:

```text
1. Author edits .hlsl / .hlsli files and updates C++ registration if needed
2. Editor or CI asks ShaderCompiler.exe to cook affected shader registrations
3. ShaderCompiler expands permutation domains into vectors
4. Cache decides which vectors actually need compilation
5. Backend adapter compiles only cache misses
6. Tool writes cooked packages + registry + reload signal
7. Runtime/editor reopens cooked artifacts and rebuilds dependent PSOs lazily
```

That workflow is strongly Unreal-inspired in the right places:

- offline compilation is separate from runtime use
- permutations are first-class and pruned before compile
- caching is part of the design, not an afterthought
- editor iteration goes through the same cook path as automation

But it stays small enough for Sparkle because:

- one executable owns orchestration
- one local cache is enough initially
- one renderer/backend pair can prove the design
- no distributed farm is needed
- no giant material graph system is required

If Sparkle wants a concise rule for decision-making, it should be this:

```text
Take Unreal's ownership boundaries, cache philosophy, and permutation model.
Do not take Unreal's scale, taxonomy, or infrastructure footprint until
Sparkle's real workloads force it.
```

For this design document, that rule is now treated as adopted architecture, not
just advisory guidance.

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

Sparkle also adopts a smaller adjacent idea: preserve backend intermediate
representations and debug artifacts so developers can inspect what the compile
pipeline produced even before any heavyweight vendor analysis tool is added.

Sparkle skips: shipping any vendor analyzer in the box initially and making
vendor ISA reporting mandatory for the first useful version of the system.

## 19. Sparkle Today vs Target (Gap Matrix)

| Concern | Today | Target | Example file (today) |
|---|---|---|---|
| CLI dispatch | `CommandRegistry`, two verbs | same shape, more verbs, `--json` | `Tools/ShaderCompiler/Private/Cli/CommandRegistry.cpp` |
| Manifest | hand-rolled INI parser | `IManifestSource` + versioned schema | `Tools/ShaderCompiler/Private/Manifest/ShaderCookManifestParser.cpp` |
| Validation | rule list in one validator | same, more rules, structured errors | `Tools/ShaderCompiler/Private/Manifest/ShaderCookManifestValidator.cpp` |
| Permutations | none (single variant per package) | `ShaderPermutationDomainDesc` + `ShaderPermutationVector` + `PermutationExpander` + `ShaderPermutationKey` | (new) |
| Dependency graph | implicit (loop) | explicit `DependencyGraph` of `CookNode`s | (new) |
| Cache | none (always recompiles) | `IShaderArtifactStore` content-addressed | (new) |
| Backend abstraction | `DxcShaderCompiler` directly called | `IShaderBackend` + `DxcShaderBackend` adapter | `Tools/ShaderCompiler/Private/Compiler/DxcShaderCompiler.cpp` |
| Reflection | not extracted | `IReflectionExtractor` + `ShaderReflection` | (new) |
| Cooked package | binary with header + records | same, schema already neutral | `Engine/RHI/Public/Shaders/CookedShaderPackage.h` |
| Schema versioning | `Magic` + `Version` already present | add migration playbook + loader policy | `Engine/RHI/Public/Shaders/CookedShaderPackage.h` |
| Diagnostics | log strings | `CookDiagnosticSink` + JSON Lines + source maps | (new) |
| Intermediate visibility | minimal | `ShaderDebugArtifactSet` + optional IR/disassembly dump path | (new) |
| Editor hot reload | not implemented | spawn-tool + `ShaderRecookSignal` + watcher | (new) |
| Boundary enforcement | CMake script in place | keep, extend with new forbidden tokens | `CMake/Validation/ValidateShaderCompilerBoundary.cmake` |
| Threading | serial loop | serial executor + DAG-ready interface | `Tools/ShaderCompiler/Private/Cooking/ShaderPackageCooker.cpp` |

## 20. Implementation Playbook

This chapter is the working contract for *how* the design in chapters 1–19
gets built. Every phase is shaped the same way so progress is checkable and
each increment is visible end-to-end.

### How to read a phase

Each phase has six sections:

1. **Goal.** One sentence describing the user-visible increment.
2. **Prerequisites.** What must be true before starting.
3. **Work Items.** Numbered, granular tasks. Each is small enough to land in
   one PR and reviewable in isolation.
4. **Implementation Prompts.** Copy-paste-ready prompts you can hand to an
   agent (or use as your own checklist). Each prompt is self-contained and
   names the files it is allowed to touch.
5. **Validation Gates.** Binary pass/fail checks. A gate is the equivalent
   of a green light: if any gate fails, the phase is not done. Gates are
   listed as either a CLI invocation, a CMake check, or a one-line manual
   verification a reviewer can perform.
6. **Increment Demo.** What you can *show* at the end of the phase — the
   tangible artifact a reviewer or portfolio viewer can see.

Global invariants that all phases must keep green:

- `cmake --build build --target ValidateShaderCompilerBoundary` passes.
- Renderer/RHI/Editor link no shader compiler.
- `ShaderCompiler.exe cook` succeeds on the Showcase project.
- The cooked package format only changes on a deliberate `Version` bump
  documented in the migration playbook (Ch.13).
- The shader compiler executor remains single-threaded.

### Phase 0 — Stabilize current state

- **Goal.** A clean baseline: the existing tool builds, cooks, and is
  guarded by CI before any refactor begins.
- **Prerequisites.** None.
- **Work Items.**
  1. Confirm `Tools/ShaderCompiler/` builds in Debug + Release.
  2. Add a CI job that runs `ShaderCompiler.exe cook` + `inspect-manifest`
     on the Showcase project on every PR.
  3. Snapshot the current manifest format in `docs/plans/` as the frozen
     v0 reference.
  4. Run `ValidateShaderCompilerBoundary.cmake` in CI; fix any current
     boundary violations before the refactor begins.
- **Implementation Prompts.**
  - *"Add a CI job that builds `Tools/ShaderCompiler` and runs
    `ShaderCompiler.exe cook --no-cache` against the Showcase project.
    Fail the job on non-zero exit. Touch only CI config and
    `Scripts/`."*
  - *"Audit the current `Tools/ShaderCompiler` for any include of
    Renderer/Editor private headers; remove them and re-run
    `ValidateShaderCompilerBoundary.cmake`."*
- **Validation Gates.**
  - `cmake --build build --target Sparkle ShaderCompiler` succeeds.
  - `ShaderCompiler.exe cook` exit code is `0` on Showcase.
  - `cmake --build build --target ValidateShaderCompilerBoundary` succeeds.
  - CI job is wired and runs on PRs.
- **Increment Demo.** A green CI badge on a no-op PR; cook log printed at
  the end of the build.

### Phase 1 — Dependency graph + content-addressed cache

- **Goal.** Re-running `cook` with no changes performs zero backend
  invocations and finishes in milliseconds.
- **Prerequisites.** Phase 0 gates green.
- **Work Items.**
  1. Introduce `Tools/ShaderCompiler/Private/Cook/CookNode.h/cpp` and
     `DependencyGraph.h/cpp`.
  2. Introduce `ICookExecutor` and a `SerialCookExecutor` implementation.
  3. Introduce `ShaderCacheKey` (content-addressed: source hash + include
     closure hash + options hash + backend version + schema version).
  4. Introduce `IShaderArtifactStore` with a local on-disk implementation
     under `bin/Cache/Shaders/`. Use temp-file + atomic rename for writes.
  5. Refactor `ShaderPackageCooker` to build a graph and drive the executor
     instead of looping.
  6. Add a `--no-cache` CLI flag (already documented in Ch.16) and a
     `--cache-dir <path>` flag.
- **Implementation Prompts.**
  - *"Create `CookNode` and `DependencyGraph` types under
    `Tools/ShaderCompiler/Private/Cook/`. A `CookNode` wraps one
    `ShaderCompileRequest` plus its resolved input hashes. The graph is a
    DAG with topological-order traversal. No threading. Add unit tests
    under `Tools/ShaderCompiler/Tests/`."*
  - *"Implement `IShaderArtifactStore` and `LocalDiskShaderArtifactStore`
    under `Tools/ShaderCompiler/Private/Cook/Cache/`. Keys map to files at
    `<cache-dir>/<first-2-hex>/<full-hex>.bin`. Writes go through a
    temp-file + `std::filesystem::rename`. Reads return `std::optional`.
    Add unit tests covering miss → put → hit."*
  - *"Refactor `ShaderPackageCooker::Cook` to (a) expand the manifest into
    `CookNode`s, (b) compute `ShaderCacheKey` per node, (c) ask the store
    for a hit, (d) on miss invoke the backend, (e) put result back. Keep
    the executor serial. Do not change the cooked package format."*
- **Validation Gates.**
  - First cook on a clean cache: full backend invocations, exit `0`.
  - Second cook with no source changes: backend invocation count is `0`,
    wall time `< 500 ms` on Showcase, exit `0`.
  - Touching one `.hlsl` file: only nodes whose include closure contains
    that file are recompiled.
  - `--no-cache` forces full recompile.
  - Cooked output is byte-identical between cached and uncached runs.
- **Increment Demo.** A `time` comparison in CI logs: cold cook vs warm
  cook. Cache directory layout visible under `bin/Cache/Shaders/`.

### Phase 2 — Backend abstraction + rich reflection + DebugArtifactBundle

- **Goal.** DXC lives behind one adapter. The cooker can already emit both
  DXIL and SPIR-V targets through the same backend. `ShaderReflection` is
  PSO-grade. Every compile can optionally drop a `DebugArtifactBundle/`.
- **Prerequisites.** Phase 1 gates green.
- **Work Items.**
  1. Define `IShaderBackend`, `ShaderBackendCapabilities`,
     `ShaderCompileEnvironment`, `ShaderTarget`, and `CompiledArtifact`
     under `Tools/ShaderCompiler/Public/Backend/`.
  2. Move all DXC code under `Tools/ShaderCompiler/Backends/Dxc/` as
     `DxcShaderBackend` implementing `IShaderBackend`. The backend reports
     support for both `Dxil*` and `SpirV*` targets (DXC's `-spirv` mode).
  3. Rip every reference to `dxcompiler` outside
     `Tools/ShaderCompiler/Backends/Dxc/`. Extend
     `ValidateShaderCompilerBoundary.cmake` with a new forbidden token
     check.
  4. Define `ShaderReflection` in
     `Engine/RHI/Public/Shaders/ShaderReflection.h` covering: bindings
     (descriptor `Set`/`Space` + `Slot`/`Register` + `Count`), constant
     buffer member layout (name, offset, size, type, array stride), thread
     group size, IO signature, push/root-constant ranges, specialization
     constants, resource access, entry-point metadata.
  5. Implement `IReflectionExtractor` + `DxilReflectionExtractor` +
     `SpirVReflectionExtractor`. Both produce the same normalized
     `ShaderReflection`.
  6. Wire reflection into the cooked package. Bump
     `kCookedShaderPackageVersion` and follow the migration playbook in
     Ch.13.
  7. Implement `ShaderDebugArtifactSet` writer. Add CLI flag
     `--debug-artifacts <dir>` that writes one bundle per
     (shader id, permutation, target).
- **Implementation Prompts.**
  - *"Define `IShaderBackend`, `ShaderBackendCapabilities`,
    `ShaderCompileEnvironment`, and `ShaderTarget` exactly as Ch.11
    describes. Place public headers in
    `Tools/ShaderCompiler/Public/Backend/`. Implementations under
    `Tools/ShaderCompiler/Backends/`. Do not name DXC anywhere outside
    `Tools/ShaderCompiler/Backends/Dxc/`."*
  - *"Implement `DxcShaderBackend` so it can compile both DXIL and SPIR-V
    by selecting the target via `ShaderTarget` and toggling DXC's `-spirv`
    mode. Both code paths share the same compile environment marshalling
    and the same diagnostic translation. Add a smoke test that cooks one
    PS to both targets and checks the resulting `*.spkg` contains both
    binaries."*
  - *"Define the full `ShaderReflection` struct in
    `Engine/RHI/Public/Shaders/ShaderReflection.h`. Implement
    `DxilReflectionExtractor` (DXC) and `SpirVReflectionExtractor`
    (SPIRV-Reflect). Round-trip the reflection through the cooked package.
    Bump the package version and add a `LoadV<N-1>` migration entry."*
  - *"Add a `--debug-artifacts <dir>` flag to `ShaderCompiler.exe`. For
    every successful compile, emit a folder named
    `<shaderId>__<permutationHash>__<target>/` containing
    `compile-request.json`, `defines.json`, `preprocessed-source.hlsl`,
    `reflection.json`, `disassembly.txt`, and `compiler-stderr.txt`."*
- **Validation Gates.**
  - `grep -r "dxc\|IDxcCompiler\|dxcompiler" Tools/ShaderCompiler/`
    returns matches *only* under `Tools/ShaderCompiler/Backends/Dxc/`.
  - `cmake --build build --target ValidateShaderCompilerBoundary`
    succeeds with the new forbidden-token check enabled.
  - `ShaderCompiler.exe cook --target=DxilSm66,SpirV16` produces a cooked
    package containing both binaries; `inspect-package` lists both.
  - Reflection JSON for a known shader contains every field listed in
    Ch.12 (descriptor space/set, CB member offsets, thread group, etc.).
  - Renderer/RHI consume `ShaderReflection` only; no renderer file
    includes a DXC header.
  - `--debug-artifacts <tmp>` produces the documented bundle layout.
- **Increment Demo.** Side-by-side `inspect-package` showing the same
  shader compiled to DXIL and SPIR-V, plus a `DebugArtifactBundle/`
  directory tree printed by `tree`.

### Phase 3 — Typed shader classes, permutations, parameter-struct verification, inspect CLI

- **Goal.** Shaders are authored as UE-style typed C++ classes with typed
  parameter structs and typed permutation domains. The cooker validates
  every compile against the declared parameter struct and refuses on
  mismatch. The CLI lets you list and inspect any shader.
- **Prerequisites.** Phase 2 gates green.
- **Work Items.**
  1. Implement the typed authoring surface from Ch.8 / Ch.8.5 / Ch.9:
     `TGlobalShader<T>`, `BEGIN_SHADER_PARAMETER_STRUCT`,
     `SHADER_PARAMETER*` macros, `TShaderPermutationDomain`,
     `ShaderPermutationBool`, `ShaderPermutationEnum`,
     `IMPLEMENT_GLOBAL_SHADER`, `ShouldCompilePermutation`,
     `ModifyCompilationEnvironment`.
  2. Macros expand into static `ShaderRegistrationDesc` records held by an
     `IShaderRegistrationSource` registry the cooker reads at startup.
  3. Implement `PermutationExpander`: walk each registered shader's
     domain, prune via `ShouldCompilePermutation`, emit one
     `ShaderCompileRequest` per surviving vector × target.
  4. Implement `ShaderParameterStructDescriptor` (generated by
     `BEGIN_SHADER_PARAMETER_STRUCT`) and
     `ShaderParameterStructVerifier` (cook-time check against
     `ShaderReflection`). Mismatch → cook failure with diagnostic code
     `SC2xxx`.
  5. Implement runtime `TShaderRef<T>` lookup that finds the cooked entry
     by `(shader id, permutation key, target)`.
  6. Add CLI verbs: `list-shaders`, `list-permutations <ShaderId>`,
     `inspect-shader <ShaderId> [--permutation k=v,...] [--target ...]`.
     `inspect-shader` emits the same `DebugArtifactBundle/` content for
     one shader on demand.
  7. Convert one existing global shader (e.g. `FullscreenBlit`) end-to-end
     to the new authoring surface as the canonical reference.
- **Implementation Prompts.**
  - *"Implement `TGlobalShader<T>`, `BEGIN_SHADER_PARAMETER_STRUCT`,
    `SHADER_PARAMETER`, `SHADER_PARAMETER_TEXTURE`,
    `SHADER_PARAMETER_RDG_BUFFER_SRV`, and `IMPLEMENT_GLOBAL_SHADER` in
    `Engine/RHI/Public/Shaders/Authoring/`. Macros expand into POD
    descriptors plus a `static const ShaderRegistrationDesc&` registered
    at static init. No runtime cost beyond a vector push at init."*
  - *"Implement `TShaderPermutationDomain<...>`, `ShaderPermutationBool`,
    and `ShaderPermutationEnum<E, Count>`. Provide `ToVector()` and
    `FromKey(ShaderPermutationKey)`. Add unit tests covering domain
    enumeration, pruning, and stable key hashing."*
  - *"Implement `ShaderParameterStructVerifier::Verify(descriptor,
    reflection) -> std::optional<MismatchReport>`. Mismatch checks: missing
    binding, type mismatch, size mismatch, register/space mismatch, array
    count mismatch. Wire into the cook so a mismatch becomes a `SC2001`
    diagnostic and the cook exits with code `6`."*
  - *"Add CLI verbs `list-shaders`, `list-permutations <id>`,
    `inspect-shader <id> [--permutation k=v,...] [--target ...]` to
    `Tools/ShaderCompiler/Private/Cli/`. `inspect-shader` writes the
    same `DebugArtifactBundle/` layout for one shader on demand and
    prints a one-page summary to stdout."*
  - *"Convert `FullscreenBlit` to the typed authoring surface. Delete the
    legacy registration. Ensure the cook still produces the same cooked
    bytecode (modulo header version) and the renderer still draws."*
- **Validation Gates.**
  - `ShaderCompiler.exe list-shaders` enumerates ≥1 shader with its
    permutation domain and target list.
  - `ShaderCompiler.exe list-permutations FullscreenBlitPS` enumerates
    only legal vectors after `ShouldCompilePermutation` pruning.
  - `ShaderCompiler.exe inspect-shader FullscreenBlitPS --permutation
    UseGammaCorrection=1` writes a complete `DebugArtifactBundle/`.
  - Introducing a deliberate mismatch (rename a `SHADER_PARAMETER` field)
    causes the cook to fail with `SC2xxx` and a non-zero exit code.
  - `--allow-parameter-mismatch` downgrades the same mismatch to a
    warning (transitional flag).
  - Cooked package round-trips through the RHI reader; renderer renders
    the converted `FullscreenBlit` shader correctly.
  - Permutation key hashing is stable across runs (re-running the cook
    produces the same keys).
- **Increment Demo.** Terminal recording of `list-shaders` →
  `list-permutations` → `inspect-shader` → deliberate-typo-causes-cook-
  fail → fix → cook-passes. The `FullscreenBlit` C++ class shown side by
  side with the corresponding `reflection.json`.

### Phase 4 — Editor hot reload via out-of-process recook

- **Goal.** Saving an `.hlsl` file with the editor open triggers a recook
  and the viewport reflects the new shader without restart, with the
  editor still linking no compiler.
- **Prerequisites.** Phase 3 gates green.
- **Work Items.**
  1. Implement `ShaderRecookSignal` (marker file written atomically by
     the tool on successful cook).
  2. Implement `ShaderRecookWatcher` in `Engine/Editor/ShaderRecook/`
     using `ReadDirectoryChangesW` + an explicit "Recompile shaders"
     menu action.
  3. On signal, the editor diffs the registry, asks RHI to reopen changed
     `*.spkg` files, and bumps `ShaderResource::Version`.
  4. Renderer's PSO cache observes the version bump and lazily rebuilds
     dependent PSOs.
  5. Editor surfaces structured diagnostics from the spawned tool's
     stderr (`--json`) in a status panel.
  6. Atomic-rename writes for both `*.spkg` and the registry.
- **Implementation Prompts.**
  - *"Implement `ShaderRecookSignal` as `bin/Cache/Shaders/recook.signal`
    written via temp-file + atomic rename at the end of every successful
    cook. The file contents are the registry file hash."*
  - *"Implement `ShaderRecookWatcher` in
    `Engine/Editor/ShaderRecook/`. On `recook.signal` change, parse the
    new registry, diff against the previously loaded one, call
    `RHI::ReopenCookedPackage(packageId)` for each changed entry, and
    increment `ShaderResource::Version`. Add a 'Recompile Shaders'
    editor menu action that spawns `ShaderCompiler.exe cook --json` and
    streams stderr into a status panel."*
  - *"Make the renderer's PSO cache key include
    `ShaderResource::Version` so a version bump invalidates dependent
    PSOs. Recreate them lazily on next draw."*
- **Validation Gates.**
  - `cmake --build build --target ValidateShaderCompilerBoundary`
    succeeds; the editor links no compiler symbol.
  - With editor running, edit `FullscreenBlit.hlsl` → save → viewport
    reflects the change within one frame after recook completes, no
    restart required.
  - Forcing a cook failure (introduce a syntax error) leaves the
    previous artifacts in place; editor surfaces the diagnostic without
    crashing.
  - Concurrent reads during a recook never observe a partial file
    (atomic rename test).
- **Increment Demo.** Screen recording: editor open, edit shader, save,
  see viewport update; then introduce error, see diagnostic in panel,
  fix, see green.

### Phase 5 — Editor shader inspector + analysis seam + glslang seam

- **Goal.** Visualize shader compilation products inside the editor and
  document the optional analysis/secondary-backend seams without making
  them mandatory.
- **Prerequisites.** Phase 4 gates green.
- **Work Items.**
  1. Editor "Shader Inspector" panel that browses `DebugArtifactBundle/`
     directories: select shader → permutation → target → view
     preprocessed source, reflection, disassembly, parameter-struct match
     report.
  2. Per-PSO live overlay: selected PSO in the renderer debug HUD shows
     its shader class, permutation vector, last cook timestamp,
     reflection summary.
  3. `IAnalysisPass` seam in the cooker: optional pass list invoked after
     each successful compile. Ship one example wrapper
     (`PsoStatsPass` writing bytecode size + resource counts to CSV).
  4. Document `GlslangShaderBackend` as a future seam in Ch.11; do not
     implement unless DXC's SPIR-V mode misses something concrete.
  5. Optional: `RgaAnalysisPass` skeleton that shells out to AMD RGA if
     installed and attaches ISA/register-pressure to diagnostics.
- **Implementation Prompts.**
  - *"Add a Shader Inspector panel to the editor. It reads only on-disk
    artifacts under `bin/Cache/Shaders/Debug/`. Tree view: shader →
    permutation → target. Detail tabs: Source (preprocessed),
    Reflection (table), Disassembly (text), Param Match (status). The
    editor must not link the compiler."*
  - *"Implement `IAnalysisPass` and `PsoStatsPass` in
    `Tools/ShaderCompiler/Private/Analysis/`. Wire a `--analysis pso-stats`
    CLI flag that runs the pass list. Output goes to
    `bin/Cache/Shaders/Analysis/<shaderId>.csv`."*
- **Validation Gates.**
  - Inspector panel opens, lists every shader the cooker registered, and
    renders the four detail tabs without error for a known shader.
  - Running `ShaderCompiler.exe cook --analysis pso-stats` produces a
    CSV whose row count equals the number of cooked permutations.
  - Boundary validator still passes — the inspector reads files only.
- **Increment Demo.** Screenshot of the Shader Inspector showing
  preprocessed source + reflection + disassembly side by side; CSV
  excerpt from `pso-stats`.

### Phase tracker (snapshot)

Use this checklist as the live progress view; check off gates, not
intents.

```text
[ ] Phase 0 — Stabilize current state
    [ ] Build green   [ ] CI cook job   [ ] Boundary validator green
[ ] Phase 1 — Dependency graph + cache
    [ ] Cold cook    [ ] Warm cook = 0 backend invocations
    [ ] Targeted invalidation works   [ ] --no-cache forces recompile
[ ] Phase 2 — Backend abstraction + reflection + DebugArtifactBundle
    [ ] DXC contained to Backends/Dxc/   [ ] DXIL + SPIR-V both cooked
    [ ] PSO-grade reflection lands   [ ] DebugArtifactBundle/ writes
[ ] Phase 3 — Typed shaders + permutations + verification + inspect CLI
    [ ] list-shaders / list-permutations / inspect-shader work
    [ ] Parameter-struct verifier rejects mismatches
    [ ] FullscreenBlit converted as reference
[ ] Phase 4 — Editor hot reload
    [ ] Save .hlsl → viewport updates without restart
    [ ] Failed cook surfaces diagnostic, keeps old artifacts
[ ] Phase 5 — Inspector + analysis seam + glslang seam
    [ ] Editor Shader Inspector panel
    [ ] PsoStatsPass CSV   [ ] GlslangShaderBackend documented
```

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
  ShaderPermutationDomainDesc declared legal permutation dimensions
  ShaderPermutationVector one concrete value selection within a domain
  ShaderPermutationKey    stable hash of a permutation vector
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
  ShaderDebugArtifactSet  optional bundle of IR/disassembly/debug outputs

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
- **Permutation domain** — the typed declaration of which permutation
  dimensions exist and what values they may take.
- **Permutation vector** — one concrete selection of values inside a
  permutation domain. This is the real input to compile-environment setup.
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
