# ShaderCompiler — Implementation Plan

> Working plan for Sparkle's offline shader compiler. This is the contract
> the implementation follows: what we want, why, and the phased path to
> get there.
>
> **Scope today:** D3D12 only, global shaders only, single-threaded
> executor, offline-only compilation. **Designed for:** Vulkan / SPIR-V
> as a peer target via the same backend, Slang as a future second
> backend behind the same interface. No design choice may close the
> door on those.

## 1. Vision

The ShaderCompiler is an **offline asset cooker**, not a runtime compiler.
Runtime never invokes a shader compiler — it only reads cooked binary
packages.

What we want, in one diagram:

```text
                 ┌──────────────────────────────────────────────────────────┐
                 │                       OFFLINE                            │
                 │  Tools/ShaderCompiler.exe                                │
                 │                                                          │
   .hlsl    ───►│  Registrations ──► Plan ──► DependencyGraph ──► Executor│
   .hlsli   ───►│         │            │              │              │     │
   C++      ───►│         ▼            ▼              ▼              ▼     │
                │   PermutationExpand   CacheLookup        CompileStage    │
                │                          │                   │           │
                │                          ├── hit ──► reuse artifact      │
                │                          └── miss ─► IShaderBackend ──► DXC (D3D12+SPIRV)
                │                                            │             │
                │                                            ▼             │
                │                                   ReflectionExtract      │
                │                                            │             │
                │                                            ▼             │
                │                            ParameterStructVerify         │
                │                                            │             │
                │                                            ▼             │
                │                                  CookedPackageWriter     │
                │                                            │             │
                │                          ┌─────────────────▼──────────┐  │
                │                          │  *.spkg  +  registry       │  │
                │                          │  + DebugArtifactBundle/    │  │
                │                          └────────────────────────────┘  │
                └──────────────────────────────────────────────────────────┘
                                              │
                              (file system + ShaderRecookSignal)
                                              │
                 ┌──────────────────────────────────────────────────────────┐
                 │                       RUNTIME                            │
                 │  Engine/RHI + Engine/Renderer + Engine/Editor            │
                 │                                                          │
                 │   CookedPackageReader (in RHI) ─► ShaderResource         │
                 │              │                          │                │
                 │              ▼                          ▼                │
                 │     ReflectionView           PSO / RootSignature build   │
                 │                                                          │
                 │   Editor: ShaderRecookWatcher ─► reopen + bump version   │
                 └──────────────────────────────────────────────────────────┘
```

The seam between the two halves is exactly one thing: a **versioned binary
file format** whose schema lives in
`Engine/RHI/Public/Shaders/CookedShaderPackage.h`. Nothing else crosses.

## 2. Design Pillars (locked decisions)

These are committed for the implementation. Anything that contradicts
them is a design bug.

1. **Offline-only compilation.** Runtime/editor consumes cooked
   artifacts. The editor links no shader compiler. Editor iteration is
   out-of-process recook, not in-process compile.
2. **Backend abstraction from day one.** `IShaderBackend` + `ShaderTarget`
   enum. Today the only adapter is `DxcShaderBackend`, which produces
   both **DXIL** (D3D12) and **SPIR-V** (Vulkan-ready) through DXC's
   `-spirv` mode. A future `SlangShaderBackend` and a separate
   `GlslangShaderBackend` are documented seams; neither is implemented
   in the initial plan.
3. **Typed UE-style authoring for global shaders.** Shaders are declared
   as C++ classes deriving from `TGlobalShader<T>`, with typed
   parameter structs (`BEGIN_SHADER_PARAMETER_STRUCT`), typed permutation
   domains (`TShaderPermutationDomain<...>`), and an
   `IMPLEMENT_GLOBAL_SHADER` macro at the cpp side. No manifest file in
   the initial scope.
4. **First-class inspection.** Every compile can drop a
   `DebugArtifactBundle/`: preprocessed source, reflection JSON,
   disassembly, parameter-struct match report, compile args. CLI verbs
   `list-shaders`, `list-permutations`, `inspect-shader`,
   `inspect-package` are part of the tool, not an add-on.

## 3. Rationale

Why these four pillars, briefly:

- **Offline-only** keeps shipping size small (no DXC at runtime),
  preserves determinism (the build farm and end users see the same
  bytecode), and lets one CMake validator
  (`CMake/Validation/ValidateShaderCompilerBoundary.cmake`) prove the
  architecture invariant.
- **Backend abstraction** is what makes Vulkan/SPIR-V cheap later. The
  cooked package format already carries a `CookedShaderBinaryFormat`
  enum (`Dxil`, `SpirV`); we just need the producer side to fan out.
  Slang slots in as another `IShaderBackend` if we ever want its IR or
  multi-target codegen.
- **Typed authoring** removes the entire class of bugs where a shader
  expects a binding the C++ side doesn't provide (or vice versa). The
  cooker performs a **mandatory** parameter-struct verification step
  that fails the cook on any mismatch.
- **First-class inspection** is the portfolio differentiator and the
  debugging story. We never want to be in a position where the only way
  to know what the backend produced is to attach a debugger to the
  cook.

Non-goals (explicitly out of scope):

- Multithreaded executor (single-threaded today; the dependency graph
  is the parallelism contract for tomorrow).
- Distributed compile workers.
- Shared/remote DDC implementation.
- Shader language invention.
- Material/uasset shader workflows. Global shaders only.
- Manifest-driven authoring as the primary surface. (Documented as a
  future extension; not implemented.)
- In-editor **in-process** recompilation. The editor never links a shader
  compiler. Editor-driven iteration is delivered by Phase 4 as
  out-of-process recook (spawn `ShaderCompiler.exe`, watch signal,
  reopen packages). "In-editor recompile" is a supported workflow;
  "in-editor compiler" is not.

> **Implementation note (pay special attention).** The user goal is
> "iterate on shaders with the editor open." That goal is satisfied by
> Phase 4 + Phase 3 together, with a deliberate split:
> - **Pure HLSL edits** (math, constants, sampling, control flow) hot-
>   reload via out-of-process recook + `ShaderResource::Version++`.
>   No editor restart, no C++ rebuild.
> - **Binding / resource-layout edits** (changing the parameter struct,
>   root signature, descriptor layout) intentionally require a C++
>   rebuild, because the typed parameter struct lives in C++ and the
>   Phase 3 `ShaderParameterStructVerifier` will (correctly) fail the
>   cook on any mismatch. This is a feature, not a limitation — it is
>   what prevents silent GPU crashes.
>
> Any future PR that proposes "just link DXC into the editor to make
> hot reload faster" is rejecting this design pillar and must be
> bounced back to this section.

## 4. Module Boundaries

### Layout

```text
Tools/ShaderCompiler/             orchestration, planning, executor, cache, CLI
Tools/ShaderCompiler/Public/      IShaderBackend, ShaderTarget, ShaderCompileEnvironment
Tools/ShaderCompiler/Backends/    IShaderBackend adapters
  └─ Dxc/                         DxcShaderBackend (DXIL + SPIR-V via DXC)
  └─ Slang/                       (future, not in initial plan)
  └─ Glslang/                     (future, not in initial plan)
Tools/ShaderCompiler/Analysis/    optional analysis passes (PsoStatsPass, RGA wrap, ...)

Engine/RHI/Public/Shaders/        cooked package SCHEMA + reader + ShaderReflection
                                  + typed authoring macros (TGlobalShader, etc.)
Engine/Core/                      generic file/path/string/hash/log utilities
Engine/Renderer/                  CONSUMER of cooked packages (no compile, ever)
Engine/Editor/                    editor UI + ShaderRecookWatcher (no compile, ever)
```

### Allowed edges

```text
Tools/ShaderCompiler           ──► Engine/Core           (utilities)
Tools/ShaderCompiler           ──► Engine/RHI public     (cooked schema, reflection, authoring macros)
Tools/ShaderCompiler/Backends  ──► dxcompiler            (only the DXC adapter)
Engine/RHI                     ──► Engine/Core
Engine/Renderer                ──► Engine/RHI            (reads cooked packages)
Engine/Editor                  ──► Engine/RHI            (reads cooked packages)
Engine/Editor                  ──► OS file watcher       (observes recook signal)
```

### Forbidden edges (enforced by `ValidateShaderCompilerBoundary.cmake`)

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
Tools/ShaderCompiler        ──/► Engine/RHI/Private/    FORBIDDEN (only RHI public)
```

**Shader declaration ownership.** Translation units that contain
`IMPLEMENT_GLOBAL_SHADER` must live in a low-level shared target that
both `ShaderCompiler.exe` and runtime consumers can link without
violating the forbidden edges above. In practice that means
`Engine/RHI/Public/Shaders/...` plus a dedicated shared registration
target or equivalent bootstrap layer, **not** `Engine/Renderer`,
`Engine/Editor`, or `Engine/Application`. The tool discovers shaders by
linking that shared registration surface, never by depending on
Renderer.

The forbidden-edge list is *the* architectural invariant. Everything else
can change. If a future PR needs a forbidden edge, the design is wrong,
not the validator.

## 5. Core Abstractions

Each entry: role + lifetime owner.

### Authoring (in `Engine/RHI/Public/Shaders/Authoring/`)

- **`TGlobalShader<T>`** — base class for typed global shader
  declarations. CRTP; no virtuals.
- **`BEGIN_SHADER_PARAMETER_STRUCT(Name, Prefix)` / `END_SHADER_PARAMETER_STRUCT()`**
  — declares a typed parameter struct. Members declared with
  `SHADER_PARAMETER`, `SHADER_PARAMETER_TEXTURE`,
  `SHADER_PARAMETER_SAMPLER`, `SHADER_PARAMETER_RDG_BUFFER_SRV`,
  `SHADER_PARAMETER_RDG_BUFFER_UAV`, `SHADER_PARAMETER_RDG_TEXTURE_SRV`,
  `SHADER_PARAMETER_RDG_TEXTURE_UAV`. Expands to a POD plus a
  compile-time `ShaderParameterStructDescriptor`.
- **`IMPLEMENT_GLOBAL_SHADER(Class, Path, Entry, Stage)`** — registers
  the shader in a static `IShaderRegistrationSource`.
- **`TShaderPermutationDomain<...>`**, **`ShaderPermutationBool`**,
  **`ShaderPermutationEnum<E, Count>`** — typed permutation dimensions.
- **`ShouldCompilePermutation(domain) -> bool`** /
  **`ModifyCompilationEnvironment(domain, env)`** — static methods on
  the shader class. Pruning + per-permutation env tweaks.
- **`TShaderRef<T>`** — runtime handle obtained from the cooked package
  by `(shader id, permutation key, target)`.

### Plan (in `Tools/ShaderCompiler/`)

- **`ShaderRegistrationDesc`** — what the macros produce: source path,
  entry point, stage, family id, permutation domain descriptor, target
  list.
- **`IShaderRegistrationSource`** — pluggable source of registrations.
  Initial implementation reads the static registry the macros populate.
- **`ShaderPackageDesc`** / **`ShaderStageDesc`** — normalized internal
  view used by the cooker.
- **`ShaderPermutationDomainDesc`** / **`ShaderPermutationVector`** /
  **`ShaderPermutationKey`** — domain expansion + stable hash.
- **`PermutationExpander`** — domain → list of `ShaderCompileRequest`.
  Pure function. Applies `ShouldCompilePermutation` pruning.
- **`ShaderCompileRequest`** — immutable, fully resolved unit of work.
- **`ShaderTarget`** — `enum class { DxilSm60..67, SpirV14..16, Future... }`.

### Source layer

- **`ShaderIncludeResolver`** — resolves `#include` against engine /
  project source roots.
- **`ShaderSourceDatabase`** — indexes sources, computes content hashes,
  builds the include graph for invalidation.

### Cook graph + execution

- **`CookNode`** — wraps one `ShaderCompileRequest` plus resolved input
  hashes.
- **`DependencyGraph`** — DAG of cook nodes; topological order is the
  cook order. Today walked serially; the graph is the parallelism
  contract.
- **`ICookExecutor`** — `SerialCookExecutor` today;
  `ParallelCookExecutor` is a future drop-in.

### Cache

- **`ShaderCacheKey`** — content-addressed: source hash + include
  closure hash + options hash + backend version + schema version +
  permutation key + target.
- **`IShaderArtifactStore`** — `LocalDiskShaderArtifactStore` under
  `bin/Cache/Shaders/` today; remote store is a future seam.

### Backend

- **`IShaderBackend`** —
  `Compile(ShaderCompileRequest, CookDiagnosticSink) → CompiledArtifact`.
- **`ShaderBackendCapabilities`** — `SupportsTarget(...)`,
  `SupportsFeature(...)` (mesh, raytracing), debug-artifact
  capabilities.
- **`ShaderCompileEnvironment`** — defines, include roots, debug flags,
  target, profile. Mutated by `ModifyCompilationEnvironment`. Consumed
  by backends.
- **`DxcShaderBackend`** — wraps `IDxcCompiler3`. Produces both DXIL and
  SPIR-V depending on `ShaderTarget`. The only adapter we ship in the
  initial plan.
- **`SlangShaderBackend`**, **`GlslangShaderBackend`** — designed seams,
  not implemented. Slot in via `IShaderBackend` only.

### Reflection

- **`IReflectionExtractor`** — backend-specific extractor. We ship
  `DxilReflectionExtractor` (DXC reflection API) and
  `SpirVReflectionExtractor` (SPIRV-Reflect).
- **`ShaderReflection`** — normalized, backend-agnostic resource
  layout. Defined in `Engine/RHI/Public/Shaders/ShaderReflection.h`.
  Fields: bindings (descriptor `Set`/`Space` + `Slot`/`Register` +
  `Count`), constant buffers (full member layout: name, offset, size,
  type, array stride), thread group size, IO signature, push/root
  constant ranges, specialization constants, resource access,
  entry-point metadata (wave size, derivatives).
- **`ShaderParameterStructDescriptor`** — C++-side layout, generated by
  `BEGIN_SHADER_PARAMETER_STRUCT`.
- **`ShaderParameterStructVerifier`** — cook-time check
  `Descriptor × Reflection`. Mismatch → `SC2xxx` diagnostic, cook
  fails. Mandatory.

### Output

- **`CookedPackageBuilder`** — assembles per-stage artifacts +
  reflection + binding records + string table.
- **`CookedShaderPackageWriter`** — serializes via the schema in
  `Engine/RHI/Public/Shaders/CookedShaderPackage.h`. Atomic write.
- **`CookedRegistryWriter`** — human-readable registry index.

### Diagnostics + inspection

- **`CookDiagnosticSink`** — receives structured diagnostics. Two
  renderers: console (humans) and JSON Lines (`--json`, for CI/editor).
- **`ShaderDebugArtifactSet`** / **`DebugArtifactBundle/`** — per
  `(shader id, permutation, target)` directory containing
  `compile-request.json`, `defines.json`, `permutation-vector.json`,
  `preprocessed-source.hlsl`, `reflection.json`,
  `parameter-struct-match.json`, `disassembly.txt`, `compiler-stderr.txt`.

### Editor / hot reload

- **`ShaderRecookSignal`** — marker file written atomically by the tool
  on successful cook.
- **`ShaderRecookWatcher`** — editor-side, observes the signal, asks
  RHI to reopen changed packages, bumps `ShaderResource::Version`.

## 6. Authoring Model

### What the user writes

A shader is one C++ class plus an `.hlsl` file.

```cpp
// FullscreenBlit.h
class FFullscreenBlitPS : public TGlobalShader<FFullscreenBlitPS>
{
public:
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_TEXTURE(Texture2D, SourceTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, SourceSampler)
        SHADER_PARAMETER(FVector4, TintColor)
    END_SHADER_PARAMETER_STRUCT()

    class FUseGammaCorrection : public ShaderPermutationBool {};
    class FQuality            : public ShaderPermutationEnum<EQuality, EQuality::Count> {};
    using FPermutationDomain  = TShaderPermutationDomain<FUseGammaCorrection, FQuality>;

    static bool ShouldCompilePermutation(const FPermutationDomain& d)
    {
        // Prune illegal combinations.
        if (!d.Get<FUseGammaCorrection>() && d.Get<FQuality>() == EQuality::High) return false;
        return true;
    }

    static void ModifyCompilationEnvironment(const FPermutationDomain& d,
                                             ShaderCompileEnvironment& env)
    {
        env.SetDefine("USE_GAMMA", d.Get<FUseGammaCorrection>() ? 1 : 0);
        env.SetDefine("QUALITY",   static_cast<int>(d.Get<FQuality>()));
    }
};

// FullscreenBlit.cpp
IMPLEMENT_GLOBAL_SHADER(FFullscreenBlitPS,
    "/Engine/Shaders/PostProcess/FullscreenBlit.hlsl",
    "MainPS",
    Pixel);
```

### What the user binds at runtime

```cpp
auto shader = TShaderRef<FFullscreenBlitPS>::Get(
    /*permutation*/ { .UseGammaCorrection = true, .Quality = EQuality::High },
    /*target*/      ShaderTarget::DxilSm66);

FFullscreenBlitPS::FParameters params{};
params.SourceTexture = sourceSrv;
params.SourceSampler = linearSampler;
params.TintColor     = { 1, 1, 1, 1 };

// Generated SetParameters walks the parameter struct, writes to the right
// slot using reflection — no per-shader hand-bound code.
shader.SetParameters(cmdList, params);
```

### Macros are sugar

The macros expand into static `ShaderRegistrationDesc` records held by an
`IShaderRegistrationSource`. The cooker reads that registry at startup;
no manifest, no code generator. Every macro effect is C++-discoverable.

## 7. Permutation System

Permutations are typed, not ad-hoc define bags. Each permutation
dimension is a class; the domain is a `TShaderPermutationDomain<...>`
of those classes.

```text
Domain:
  FUseGammaCorrection : bool
  FQuality            : enum { Low, High }

Candidate vectors:
  { UseGammaCorrection=0, Quality=Low }
  { UseGammaCorrection=0, Quality=High }   ── pruned by ShouldCompilePermutation
  { UseGammaCorrection=1, Quality=Low }
  { UseGammaCorrection=1, Quality=High }

Cooked vectors after pruning:
  { UseGammaCorrection=0, Quality=Low }
  { UseGammaCorrection=1, Quality=Low }
  { UseGammaCorrection=1, Quality=High }
```

The vector is the source of truth for compile defines; the backend never
receives a hand-assembled define bag from random call sites.

`ShaderPermutationKey` is a stable hash of the vector under the domain
schema version. It is part of `ShaderCacheKey` and embedded in the
cooked package so the runtime can fetch the right variant.

## 8. Backend Abstraction

Single interface, multiple targets:

```cpp
class IShaderBackend
{
public:
    virtual ~IShaderBackend() = default;

    virtual ShaderBackendCapabilities GetCapabilities() const = 0;

    virtual CompiledArtifact Compile(
        const ShaderCompileRequest& request,
        CookDiagnosticSink&         diagnostics) = 0;
};
```

```cpp
enum class ShaderTarget : uint16_t {
    DxilSm60, DxilSm61, DxilSm62, DxilSm63,
    DxilSm64, DxilSm65, DxilSm66, DxilSm67,
    SpirV14,  SpirV15,  SpirV16,
    // future: Slang IR target families, MetalIR, WGSL, ...
};
```

### Today's adapter

- **`DxcShaderBackend`** wraps `IDxcCompiler3`.
- It supports both `Dxil*` and `SpirV*` targets by toggling DXC's
  `-spirv` mode based on `ShaderTarget`.
- DXIL is what we use for D3D12 (the only renderer backend we have).
- SPIR-V is built and validated end-to-end through the cooker so that
  when a Vulkan RHI lands, the producer side is already in place.

### Per-shader target selection

`IMPLEMENT_GLOBAL_SHADER` may name a target list (or take the project
default). The cook expands per-target × per-permutation. Capability
selection is by `ShaderBackendCapabilities::SupportsTarget(...)`; the
orchestrator picks the first registered backend that claims the target.

### Future seams (designed for, not implemented)

- **`SlangShaderBackend`** — when we want Slang's IR, modules, or
  multi-target codegen. Slots in via `IShaderBackend` like DXC.
- **`GlslangShaderBackend`** — only if DXC's SPIR-V mode misses
  something concrete (specific extensions, GLSL ingest).

The orchestrator must never name DXC, Slang, or glslang. It holds an
`IShaderBackend*` chosen by capability lookup. **This is the single
biggest architectural lever for Vulkan readiness.**

The cooked artifact already accommodates plurality:
`CookedShaderBinaryFormat::{Dxil, SpirV}` is in the schema today. The
producer side is what we are catching up.

## 9. Reflection & Parameter-Struct Verification

Bytecode is useless without knowing what resources it expects.
Reflection is that knowledge, normalized across backends.

Per-backend extraction, common output:

```cpp
struct ShaderReflection
{
    std::vector<ShaderResourceBinding>   Bindings;          // descriptor Set/Space + Slot/Register + Count
    std::vector<ShaderConstantBuffer>    ConstantBuffers;   // name, members[name, offset, size, type, array stride]
    std::array<uint32_t, 3>              ThreadGroupSize;   // compute only
    ShaderIOSignature                    InputSignature;    // VS only
    std::vector<PushConstantRange>       PushConstants;     // root/push constants
    std::vector<SpecializationConstant>  SpecConstants;     // id, type, default
    EntryPointMetadata                   Entry;             // wave size, derivatives, ...
};
```

`IReflectionExtractor` is per-backend. The output is backend-agnostic.
The renderer's PSO/root-signature builder consumes the normalized form
and never sees DXIL or SPIR-V specifics.

### Parameter-struct verification (mandatory)

At cook time, every compile result is verified against the
`ShaderParameterStructDescriptor` declared by the C++ class:

```text
compile  ──►  reflect  ──►  verify(descriptor, reflection)
                                  │
                                  ├── match    ──► proceed to package writer
                                  └── mismatch ──► SC2xxx diagnostic, cook fails
```

Mismatch checks:

- missing binding (declared in C++, not present in shader)
- extra binding (present in shader, not declared in C++)
- type mismatch (`Texture2D` vs `Texture3D`, etc.)
- size mismatch (CB member size differs from declared)
- register/space mismatch (binding is at a different slot)
- array count mismatch

A transitional flag `--allow-parameter-mismatch` downgrades these to
warnings. Not for production cooks.

## 10. Cooked Artifact Format & Versioning

Schema in `Engine/RHI/Public/Shaders/CookedShaderPackage.h`. Owned by
RHI, *not* by the tool. The tool writes; the runtime reads.

Current shape:

```text
CookedShaderPackageHeader
├─ Magic            ('S','S','H','D')
├─ Version          kCookedShaderPackageVersion
├─ DeclaredStages   ShaderStageMask
├─ ShaderModelMajor/Minor
├─ counts (binary records, binding records, specialization inputs)
├─ section sizes
├─ ShaderPackageKey
└─ hashes (SourceIdentity, BindingLayout, Variant)

CookedShaderBinaryRecord[]                  per stage: stage, format (Dxil/SpirV), entry, blob ref, hash
CookedShaderBindingRecord[]                 per binding: name, semantic, access, slot, space/set, count
CookedShaderSpecializationInputRecord[]
StringTable bytes
BinaryBlob bytes                            concatenated bytecode chunks, referenced by offset/size
```

Versioning rules:

- `Magic` rejects entirely-foreign files at the door.
- `Version` rejects same-magic-different-layout files cleanly.
- Bumping `Version` requires a migration entry: a documented diff and a
  loader path that either reads-old or refuses-with-a-clear-error.
- Records are `trivially_copyable` (already enforced by `static_assert`).
  The loader can mmap and reinterpret.
- New fields are appended; old loaders refuse-by-version, never
  mis-read.

A separate **registry** (also versioned) indexes the `*.spkg` files
produced by the cook: package id, variant, output path, hashes. Two
consumers: humans (debugging "what got cooked") and runtime discovery.

## 11. Diagnostics, Inspection, CLI

### Structured diagnostics

```cpp
struct CookDiagnostic
{
    Severity                              Severity;   // Error / Warning / Info
    std::string                           Code;       // "SC1001", stable for grep/CI
    SourceLocation                        Location;   // file + line + col, post source-map
    std::string                           Message;
    std::vector<DiagnosticContextFrame>   Context;    // include stack, permutation, package
};
```

Two renderers: console (humans, colored, with caret-and-source-line) and
JSON Lines (`--json`, for CI and editor). Source maps map post-preprocess
positions back to the original `.hlsl` line.

### DebugArtifactBundle

Per `(shader id, permutation, target)`:

```text
DebugArtifactBundle/<ShaderId>__<PermutationHash>__<Target>/
  compile-request.json
  defines.json
  permutation-vector.json
  preprocessed-source.hlsl
  reflection.json
  parameter-struct-match.json
  disassembly.txt
  compiler-stderr.txt
  compile-args.json
```

These exist for offline inspection only. They let you see what the
backend actually compiled, how a permutation translated into concrete
defines, and whether the C++ parameter struct matched reflection.

### CLI surface

```text
ShaderCompiler.exe <verb> [options]

verbs:
  cook                          cook all registered shaders
  cook --shader <id>            cook one shader (all surviving permutations × targets)
  cook --target=<list>          comma-separated ShaderTarget values
  cook --no-cache               force full recook
  cook --debug-artifacts <dir>  emit DebugArtifactBundle/ per compile
  cook --analysis <pass>        run optional analysis pass(es)
  cook --json                   structured diagnostics on stderr

  list-shaders                  enumerate registered shaders + permutation domains
  list-permutations <ShaderId>  enumerate legal vectors after pruning
  inspect-shader  <ShaderId>    [--permutation k=v,...] [--target ...] dump bundle + summary
  inspect-package <PackageId>   summarize cooked package contents
  list-targets                  print supported ShaderTarget values

  --cache-dir <path>            override default cache location
  --allow-parameter-mismatch    transitional: parameter mismatch becomes a warning
```

Exit codes (subset):

- `0` success
- `1` usage error
- `5` registration / planning failure
- `6` cook failure (backend, reflection, or parameter-struct verification)

## 12. Runtime ↔ Offline Boundary + Editor Hot Reload

### The seam

Exactly one thing crosses the boundary: the cooked binary file format
plus the registry. No shared headers, no in-process compile, no DLL the
runtime loads from the tool.

`Engine/RHI` owns:

- The `CookedShaderPackage*` schema and the reader.
- `ShaderResource` handles + a version counter so consumers can
  invalidate when a package is reopened.

`Engine/Renderer` owns:

- PSO and root-signature construction from `ShaderReflection` + cooked
  bytecode.
- PSO cache keyed on `ShaderResource::Version` + render-state hash.

`Engine/Editor` owns:

- Editor UI surfaces (recook button, status panel).
- `ShaderRecookWatcher`.

### Editor hot reload — out-of-process recook

```text
┌────────────────────────────────────────────────────────────────┐
│ Editor process (links Engine/RHI, NOT Tools/ShaderCompiler)    │
│                                                                │
│  User saves Materials/BasicLit.hlsl                            │
│         │                                                      │
│         ▼                                                      │
│  ShaderRecookWatcher detects change (or "Recompile shaders"    │
│  menu action)                                                  │
│         │                                                      │
│         ▼                                                      │
│  Editor SPAWNS ShaderCompiler.exe cook --json                  │
│         │                                                      │
│         ▼                                                      │
│  Editor waits on exit + reads stderr (JSON Lines)              │
│         │                                                      │
│         ├─ exit ≠ 0 → show diagnostics, do not reload          │
│         └─ exit = 0 + ShaderRecookSignal updated:              │
│                  │                                             │
│                  ▼                                             │
│            For each package whose hash changed:                │
│              RHI reopens *.spkg                                │
│              ShaderResource::Version++                         │
│                  │                                             │
│                  ▼                                             │
│            Renderer's PSO cache notices, evicts dependent      │
│            PSOs, rebuilds lazily on next draw                  │
└────────────────────────────────────────────────────────────────┘
```

Properties: process isolation (compiler crash cannot take the editor
down), same code path as CI (no editor-only compile path to drift),
cache reuse (DDC means recook is fast), boundary preserved (validator
still passes).

Edge cases: writers write temp + rename atomically (readers see old or
new, never partial); recook failure leaves prior artifacts in place;
schema version mismatch refuses with a clear error.

## 13. Implementation Playbook

This is the working contract for *how* the design above gets built. Each
phase is a checkable increment.

### How to read a phase

1. **Goal.** One sentence describing the user-visible increment.
2. **Why.** What pain this phase removes, with a concrete example so the
   goal is unambiguous.
3. **Prerequisites.** What must be green before starting.
4. **Work Items.** Numbered, PR-sized tasks. Cleanup of replaced legacy
   files is an explicit work item, not an afterthought.
5. **Implementation Prompts.** Copy-paste-ready prompts naming the files
   they may touch.
6. **Validation Gates.** Binary pass/fail checks.
7. **Increment Demo.** Tangible artifact you can show.

### Global invariants

All phases must keep these green:

- `cmake --build build --target ValidateShaderCompilerBoundary` passes.
- Renderer/RHI/Editor link no shader compiler.
- `ShaderCompiler.exe cook` succeeds on the Showcase project.
- The cooked package format only changes on a deliberate `Version` bump
  documented in §10.
- The shader compiler executor remains single-threaded.

### Guardrails (apply to every phase and every prompt)

These positive/negative rules apply to every phase, every prompt, and
every PR. A phase is not done if any of these is violated.

**Positive guardrails — every change must:**

- Keep `ValidateShaderCompilerBoundary.cmake` green, including any new
  forbidden-token rules added by the phase.
- Keep `ShaderCompiler.exe cook` exit-code-`0` on Showcase before and
  after the change (cold and warm where relevant).
- Land replacement and removal of legacy code in the **same PR**
  (cleanup discipline; no parallel old/new paths).
- Cross the runtime↔offline boundary only via the cooked package format
  + registry described in §10/§12.
- Scope diagnostics with stable `SC####` codes when introducing new
  failure modes; non-zero exit codes when the cook is involved.
- Add or extend tests under `Tools/ShaderCompiler/Tests/` for any new
  pure-logic component (graph, cache, expander, verifier, coordinator).

**Negative guardrails — no change in any phase may:**

- Link a shader compiler (DXC, Slang, glslang, …) into `Engine/RHI`,
  `Engine/Renderer`, `Engine/Editor`, `Engine/Application`, or
  `Engine/GameFramework`.
- Introduce a runtime shader-compile fallback path or "dev mode" that
  bypasses cooked artifacts.
- Reference DXC types, headers, or symbols outside
  `Tools/ShaderCompiler/Backends/Dxc/`.
- Place `IMPLEMENT_GLOBAL_SHADER` translation units in
  `Engine/Renderer`, `Engine/Editor`, `Engine/Application`, or any
  module the standalone tool does not link.
- Add a parallel "legacy" code path, compatibility shim, or re-export
  header to keep retired code alive past the phase that retired it.
- Multithread the cook executor (the dependency graph is the
  parallelism contract for tomorrow; today it stays serial).
- Bump the cooked package `Version` without a documented `LoadV<N-1>`
  migration entry in §10.
- Change behavior silently — every new failure mode emits a structured
  diagnostic and propagates a non-zero exit code where applicable.

### Cleanup discipline (applies to every phase)

When a phase introduces a replacement, the legacy path is **deleted in
the same PR** — no parallel old/new code paths, no compatibility shims,
no dead headers left behind. We finish each phase with a smaller tree,
not a larger one. The current legacy surface that will be retired across
Phases 2 and 3 is enumerated in the relevant phase's Cleanup work item.

### Legacy surface map (what gets retired, and when)

| Legacy file / construct | Role today | Retired in | Replacement |
| --- | --- | --- | --- |
| `Tools/ShaderCompiler/Private/Compiler/DxcShaderCompiler.{h,cpp}` | DXC invocation, hard-coded | Phase 2 | `Backends/Dxc/DxcShaderBackend` behind `IShaderBackend` |
| `Tools/ShaderCompiler/Private/Compiler/DxcContext.{h,cpp}` | Singleton DXC com objects | Phase 2 | Owned privately by `DxcShaderBackend` |
| `Tools/ShaderCompiler/Private/Compiler/ShaderCompileOptionsBuilder.{h,cpp}` | Builds `ShaderCompileOptions` from manifest stage desc | Phase 3 | Typed registration → `ShaderCompileEnvironment` |
| `Engine/RHI/Public/Shaders/ShaderCompileOptions.h` | Compile-time options leaked into RHI public | Phase 2 | Move under `Tools/ShaderCompiler/Public/` |
| `Engine/RHI/Public/Shaders/ShaderCompileResult.h` | Compile result leaked into RHI public | Phase 2 | Move under `Tools/ShaderCompiler/Public/` |
| `Tools/ShaderCompiler/Private/Manifest/*` (ShaderCookManifest, Parser, Validator, Types, ShaderStageNames) | INI parser for `ShaderPackages.ini` | Phase 3 | Typed `IMPLEMENT_GLOBAL_SHADER` registrations |
| `Tools/ShaderCompiler/Private/Cli/InspectManifestCommand.{h,cpp}` | `inspect-manifest` verb | Phase 3 | `list-shaders` + `inspect-shader` |
| `Engine/RHI/Public/Shaders/ShaderPackageLayoutCatalog.h` + `Engine/RHI/Private/Shaders/ShaderPackageLayoutCatalog.cpp` | Hard-coded `ForwardOpaque`/`ShadowOpaque`/`ComputeClear` layouts | Phase 3 | `BEGIN_SHADER_PARAMETER_STRUCT` per shader |
| `Tools/ShaderCompiler/Private/Cooking/BindingRecordBuilder.{h,cpp}` | Walks `PassParameterLayout` → binding records | Phase 3 | Bindings come from reflection + param-struct verifier |
| `Tools/ShaderCompiler/Private/Cooking/StageMaskUtils.{h,cpp}` | Visibility → mask helper for the catalog path | Phase 3 | No replacement (visibility comes from typed shader class) |
| `Constants/ShaderCompilerConstants.h` `kManifest*`, `kCommand*ManifestLegacy`, `kCommandCookLegacy` | Manifest constants and legacy verb aliases | Phase 3 | Removed; only `cook`, `list-shaders`, `inspect-shader`, `list-permutations` remain |
| `ShaderPackages.ini` files under engine and project shader roots | Manifest source of truth | Phase 3 | Deleted; typed registrations are the source of truth |
| `Scripts/Cook/CookShaders.bat` `inspect-manifest` step | Pre-cook validation of manifest | Phase 3 | Replaced by `list-shaders --validate` (typed registry self-check) |

### Phase 0 — Stabilize current state

- **Goal.** Clean baseline: build, cook, boundary validator all green
  before any refactor.
- **Why.** Every later phase is a surgical change. If the baseline is
  already red, we cannot tell whether a regression came from our work or
  from pre-existing drift. Example: today `ShaderCompiler.exe cook`
  succeeds locally but is not gated in CI — a teammate could land a
  broken `.hlsl` on `main` and nobody would notice until the next
  manual cook. Phase 0 closes that gap so every PR proves the cook is
  green.
- **Prerequisites.** None.
- **Work Items.**
  1. Confirm `Tools/ShaderCompiler/` builds in Debug + Release.
  2. Add a CI job that runs `ShaderCompiler.exe cook --no-cache` against
     the Showcase project on every PR. Fail on non-zero exit.
  3. Audit and remove any include of Renderer/Editor private headers
     from the tool.
- **Implementation Prompts.**
  - *"Add a CI job that builds `Tools/ShaderCompiler` and runs
    `ShaderCompiler.exe cook --no-cache` on the Showcase project. Fail on
    non-zero exit. Touch only CI config and `Scripts/`."*
  - *"Audit `Tools/ShaderCompiler/` for any include of Renderer/Editor
    private headers; remove and re-run
    `ValidateShaderCompilerBoundary.cmake`."*
- **Guardrails.**
  - *Must:* land the CI cook job in a PR that demonstrates a green run
    on `main`'s current state before any other phase begins.
  - *Must:* keep changes scoped to CI configuration, `Scripts/`, and
    include hygiene fixes only.
  - *Must not:* refactor cook orchestration, change cook output,
    introduce new flags, or modify the package schema in this phase.
  - *Must not:* paper over a pre-existing red baseline by relaxing the
    boundary validator or marking the CI cook job non-blocking.
- **Validation Gates.**
  - `cmake --build build --target Sparkle ShaderCompiler` succeeds.
  - `ShaderCompiler.exe cook` exit code is `0` on Showcase.
  - `cmake --build build --target ValidateShaderCompilerBoundary`
    succeeds.
  - CI cook job runs on PRs.
- **Increment Demo.** Green CI on a no-op PR; cook log printed at the
  end of the build.

### Phase 1 — Dependency graph + content-addressed cache

- **Goal.** Re-running `cook` with no changes performs zero backend
  invocations and finishes in milliseconds.
- **Why.** Today `ShaderPackageCooker::CookAll` walks the manifest and
  unconditionally re-invokes DXC for every stage of every package. On
  Showcase that is fast enough to tolerate; on a real shader library it
  is the difference between a 200 ms iteration loop and a 30-second one
  — and a 30-second loop kills hot reload (Phase 4) before it ships.
  Example: you tweak one line of `Lighting.hlsli` that is included by
  three shaders. Today every shader recompiles. After this phase, only
  the three dependents recompile, and the 50 shaders that don't include
  that header are served from cache in milliseconds. The cache key is
  content-addressed (source + include closure + options + backend
  version + permutation key + target), so two machines with identical
  inputs get identical artifacts byte-for-byte.
- **Prerequisites.** Phase 0 gates green.
- **Work Items.**
  1. Introduce `CookNode` and `DependencyGraph` under
     `Tools/ShaderCompiler/Private/Cook/`.
  2. Introduce `ICookExecutor` and `SerialCookExecutor`.
  3. Introduce `ShaderCacheKey` (content-addressed: source + include
     closure + options + backend version + schema version + permutation
     key + target).
  4. Introduce `IShaderArtifactStore` + `LocalDiskShaderArtifactStore`
     under `bin/Cache/Shaders/`. Temp-file + atomic rename.
  5. Refactor `ShaderPackageCooker::Cook` to drive the graph instead of
     looping.
  6. Add `--no-cache` and `--cache-dir` CLI flags.
- **Implementation Prompts.**
  - *"Create `CookNode` and `DependencyGraph` types under
    `Tools/ShaderCompiler/Private/Cook/`. A `CookNode` wraps one
    `ShaderCompileRequest` plus its resolved input hashes. The graph is
    a DAG with topological-order traversal. No threading. Add unit tests
    under `Tools/ShaderCompiler/Tests/`."*
  - *"Implement `LocalDiskShaderArtifactStore` under
    `Tools/ShaderCompiler/Private/Cook/Cache/`. Keys map to files at
    `<cache-dir>/<first-2-hex>/<full-hex>.bin`. Writes go through a
    temp-file + `std::filesystem::rename`. Reads return
    `std::optional`. Cover miss → put → hit in tests."*
  - *"Refactor `ShaderPackageCooker::Cook` to (a) expand registrations
    into `CookNode`s, (b) compute `ShaderCacheKey` per node, (c) ask
    the store for a hit, (d) on miss invoke the backend, (e) put result
    back. Keep the executor serial. Do not change the cooked package
    format yet."*
- **Guardrails.**
  - *Must:* include backend version, schema version, target, and
    permutation key in `ShaderCacheKey` so bumping any of them
    invalidates the cache automatically.
  - *Must:* write cache entries via temp-file + `std::filesystem::rename`
    so concurrent reads never observe partial files.
  - *Must:* produce byte-identical cooked output between cached and
    uncached runs (golden test).
  - *Must not:* change the cooked package schema, add new CLI verbs
    beyond `--no-cache` / `--cache-dir`, or move DXC code in this phase
    (those belong to Phase 2).
  - *Must not:* parallelize the executor; the graph is the parallelism
    contract for later, but the runtime stays serial.
  - *Must not:* leak cache files outside the configured cache directory
    or write outside the project tree by default.
- **Validation Gates.**
  - First cook on a clean cache: full backend invocations, exit `0`.
  - Second cook with no source changes: backend invocation count is
    `0`, wall time `< 500 ms` on Showcase.
  - Touching one `.hlsl` file: only nodes whose include closure
    contains it are recompiled.
  - `--no-cache` forces full recompile.
  - Cooked output is byte-identical between cached and uncached runs.
- **Increment Demo.** `time` comparison cold vs warm cook in CI logs.

### Phase 2 — Backend abstraction + rich reflection + DebugArtifactBundle

- **Goal.** DXC lives behind one adapter that produces both DXIL and
  SPIR-V. `ShaderReflection` is PSO-grade. Every compile can drop a
  `DebugArtifactBundle/`.
- **Why.** Three concrete pains today:
  1. **DXC is hard-wired into the cooker.** `DxcShaderCompiler` and
     `DxcContext` are called directly from `StageCompiler`. There is no
     seam to ever add Vulkan-native (`glslang`) or Slang. Today's
     SPIR-V story is "we have an enum value but no path that emits it."
     After this phase, asking for SPIR-V is `--target SpirV16` and the
     same DXC backend produces both via its `-spirv` mode.
  2. **`ShaderCompileOptions` and `ShaderCompileResult` live under
     `Engine/RHI/Public/Shaders/`** even though only the offline tool
     uses them. That's a leak — RHI public headers should describe
     runtime resources, not compiler I/O. They move to
     `Tools/ShaderCompiler/Public/`.
  3. **Reflection is anemic.** Today the cooked package carries
     `CookedShaderBindingRecord` built by hand from a `PassParameterLayout`
     catalog. The renderer cannot ask "what root parameters does this
     PSO need?" — it has to know out of band. After this phase,
     `ShaderReflection` carries every field listed in §9 (resources,
     CBuffer layout, push constants, vertex input, thread group, etc.),
     extracted from DXIL and SPIR-V into one normalized shape, and is
     written into the cooked package for the renderer to consume.

  Example: a Vulkan port becomes a one-line CLI change
  (`--target SpirV16`) plus a renderer that reads `ShaderReflection`
  to build descriptor set layouts — no new offline tool, no second
  manifest format, no recook of the DXIL artifacts.
- **Prerequisites.** Phase 1 gates green.
- **Work Items.**
  1. Define `IShaderBackend`, `ShaderBackendCapabilities`,
     `ShaderCompileEnvironment`, `ShaderTarget`, `CompiledArtifact` in
     `Tools/ShaderCompiler/Public/Backend/`.
  2. Move all DXC code under `Tools/ShaderCompiler/Backends/Dxc/` as
     `DxcShaderBackend`. Backend reports support for both `Dxil*` and
     `SpirV*` targets.
  3. Move `Engine/RHI/Public/Shaders/ShaderCompileOptions.h` and
     `ShaderCompileResult.h` to `Tools/ShaderCompiler/Public/`. Update
     all includes. RHI public headers must no longer mention compile
     options or compile results.
  4. Extend `ValidateShaderCompilerBoundary.cmake` with a forbidden-
     token check so `dxc`/`IDxcCompiler`/`dxcompiler` only appear under
     `Backends/Dxc/`, and `ShaderCompileOptions`/`ShaderCompileResult`
     never appear under `Engine/RHI/Public/`.
  5. Define `ShaderReflection` in
     `Engine/RHI/Public/Shaders/ShaderReflection.h` covering every
     field listed in §9.
  6. Implement `DxilReflectionExtractor` (DXC reflection API) and
     `SpirVReflectionExtractor` (SPIRV-Reflect). Same normalized output.
  7. Wire reflection into the cooked package. Bump
     `kCookedShaderPackageVersion` and add a `LoadV<N-1>` migration
     entry (§10).
  8. Implement `ShaderDebugArtifactSet` writer + `--debug-artifacts
     <dir>` flag.
  9. **Cleanup (same PR).** Delete the legacy DXC files in their old
     location after the move:
     `Tools/ShaderCompiler/Private/Compiler/DxcShaderCompiler.{h,cpp}`,
     `Tools/ShaderCompiler/Private/Compiler/DxcContext.{h,cpp}`. The
     `Compiler/` folder keeps only `ShaderCompileOptionsBuilder` (which
     Phase 3 retires). Remove the moved-from headers
     `Engine/RHI/Public/Shaders/ShaderCompileOptions.h` and
     `ShaderCompileResult.h`. No re-export shims.
- **Implementation Prompts.**
  - *"Define `IShaderBackend`, `ShaderBackendCapabilities`,
    `ShaderCompileEnvironment`, `ShaderTarget` in
    `Tools/ShaderCompiler/Public/Backend/`. Implementations under
    `Tools/ShaderCompiler/Backends/`. Do not name DXC outside
    `Backends/Dxc/`."*
  - *"Implement `DxcShaderBackend` so it compiles both DXIL and SPIR-V by
    selecting the target via `ShaderTarget` and toggling DXC's `-spirv`
    mode. Both code paths share compile-environment marshalling and
    diagnostic translation. Add a smoke test that cooks one PS to both
    targets and checks the resulting `*.spkg` contains both binaries."*
  - *"Define the full `ShaderReflection` struct in
    `Engine/RHI/Public/Shaders/ShaderReflection.h`. Implement
    `DxilReflectionExtractor` and `SpirVReflectionExtractor`. Round-trip
    reflection through the cooked package. Bump the version and add a
    `LoadV<N-1>` migration entry."*
  - *"Add `--debug-artifacts <dir>` to `ShaderCompiler.exe`. For every
    successful compile, emit a folder
    `<shaderId>__<permutationHash>__<target>/` with the bundle layout
    from §11."*
- **Guardrails.**
  - *Must:* contain every DXC type, header, and symbol to
    `Tools/ShaderCompiler/Backends/Dxc/`; the orchestrator names
    `IShaderBackend*` only.
  - *Must:* normalize reflection so DXIL and SPIR-V produce the same
    `ShaderReflection` shape consumed by the renderer.
  - *Must:* bump `kCookedShaderPackageVersion` and add a `LoadV<N-1>`
    migration entry in §10 in the same PR.
  - *Must:* delete `DxcShaderCompiler` / `DxcContext` and the moved
    `ShaderCompileOptions.h` / `ShaderCompileResult.h` from their old
    locations in the same PR — no re-export shims.
  - *Must not:* introduce a second backend (Slang, glslang, …) in this
    phase; only DXC ships, exposing both DXIL and SPIR-V via `-spirv`.
  - *Must not:* put compile options, compile results, or any
    backend-specific type under `Engine/RHI/Public/` after this phase.
  - *Must not:* break the cache from Phase 1: cached artifacts produced
    by the new backend must remain content-addressed and byte-stable
    for identical inputs.
- **Validation Gates.**
  - `grep -r "dxc\|IDxcCompiler\|dxcompiler" Tools/ShaderCompiler/`
    matches *only* under `Backends/Dxc/`.
  - `cmake --build build --target ValidateShaderCompilerBoundary`
    passes with the new forbidden-token check.
  - `ShaderCompiler.exe cook --target=DxilSm66,SpirV16` produces a
    cooked package containing both binaries; `inspect-package` lists
    both.
  - Reflection JSON for a known shader contains every field listed in
    §9.
  - Renderer/RHI consume `ShaderReflection` only; no renderer file
    includes a DXC header.
  - `--debug-artifacts <tmp>` produces the bundle layout from §11.
- **Increment Demo.** Side-by-side `inspect-package` showing the same
  shader compiled to DXIL and SPIR-V; `DebugArtifactBundle/` directory
  tree printed.

### Phase 3 — Typed shader classes + permutations + parameter-struct verification + inspect CLI

- **Goal.** Shaders are authored as typed C++ classes with typed
  parameter structs and typed permutation domains. The cooker rejects
  parameter-struct mismatches. CLI lets you list and inspect any shader.
- **Why.** This is the phase that retires the largest chunk of legacy
  code and gives the engine its UE-style authoring contract. Today:
  - **Shaders are registered in `ShaderPackages.ini`.** Adding a new
    shader means editing INI, picking a string `BindingLayout` from
    `ShaderPackageLayoutCatalog.cpp` (where someone hand-wrote
    `ForwardOpaque`, `ShadowOpaque`, `ComputeClear`), and praying the
    HLSL register layout matches what the catalog declared. There is no
    type system. A typo in `BindingLayout = ForwardOpqaue` is caught by
    a string compare, not the compiler.
  - **The runtime trusts the catalog.** Bindings are emitted from
    `BindingRecordBuilder` walking a `PassParameterLayout`. If the HLSL
    actually binds `t6` but the layout only declares 5 textures,
    nothing complains until a GPU crash.
  - **There is no permutation system.** Each variant needs a separate
    INI section.

  After this phase, authoring a shader looks like the `FullscreenBlit`
  example in §6: one C++ class declares its stage, parameter struct
  (`SHADER_PARAMETER_TEXTURE(Texture2D, InputColor)`), permutation
  domain (`ShaderPermutationBool<"USE_GAMMA_CORRECTION">`), and
  source path. The macros register it. Cook expands permutations,
  compiles, and runs `ShaderParameterStructVerifier` against
  reflection — if the HLSL declares `Texture2D Foo` but the C++ struct
  declares `InputColor`, the cook fails with `SC2001` and a message
  pointing at both files. Renaming a binding in HLSL without renaming
  the C++ field is now a compile-time-of-cook error, not a GPU crash.
  Adding a permutation is one line: extend the `TShaderPermutationDomain`.
- **Prerequisites.** Phase 2 gates green.
- **Work Items.**
  1. Implement the typed authoring surface (§5, §6) under
     `Engine/RHI/Public/Shaders/Authoring/`: `TGlobalShader<T>`,
     `BEGIN_SHADER_PARAMETER_STRUCT` / `SHADER_PARAMETER*`,
     `TShaderPermutationDomain`, `ShaderPermutationBool`,
     `ShaderPermutationEnum`, `IMPLEMENT_GLOBAL_SHADER`.
  2. Macros expand into static `ShaderRegistrationDesc` records held by
     `IShaderRegistrationSource`.
  3. Introduce the shared shader-registration bootstrap/target that is
    linked by both `ShaderCompiler.exe` and runtime consumers. All
    translation units containing `IMPLEMENT_GLOBAL_SHADER` must live in
    that shared surface, not in `Engine/Renderer` or editor-only code.
  4. Implement `PermutationExpander` (domain → vectors → requests, with
     `ShouldCompilePermutation` pruning).
  5. Implement `ShaderParameterStructDescriptor` (generated by the
     macros) and `ShaderParameterStructVerifier`. Mismatch → `SC2xxx`,
     cook exits with code `6`.
  6. Implement runtime `TShaderRef<T>` lookup by
    `(shader id, permutation key, target)` plus generated
    `SetParameters(cmdList, params)` binding that walks the typed
    parameter struct against cooked `ShaderReflection`, with no
    per-shader hand-written binding code.
  7. Add CLI verbs: `list-shaders`, `list-permutations <id>`,
     `inspect-shader <id> [--permutation k=v,...] [--target ...]`,
     and `list-shaders --validate` (used by `CookShaders.bat` in place
     of the old `inspect-manifest` step).
  8. Convert `FullscreenBlit` end-to-end as the canonical reference;
     delete the legacy registration.
  9. **Cleanup (same PR or immediate follow-up).** Retire the manifest
     world entirely:
     - Delete `Tools/ShaderCompiler/Private/Manifest/` (all 8 files:
       `ShaderCookManifest.{h,cpp}`, `ShaderCookManifestParser.{h,cpp}`,
       `ShaderCookManifestValidator.{h,cpp}`,
       `ShaderCookManifestTypes.h`, `ShaderStageNames.{h,cpp}`).
     - Delete `Tools/ShaderCompiler/Private/Cli/InspectManifestCommand.{h,cpp}`
       and remove its registration from `CommandRegistry.cpp`.
     - Delete `Tools/ShaderCompiler/Private/Cooking/BindingRecordBuilder.{h,cpp}`
       and `Tools/ShaderCompiler/Private/Cooking/StageMaskUtils.{h,cpp}`.
     - Delete `Tools/ShaderCompiler/Private/Compiler/ShaderCompileOptionsBuilder.{h,cpp}`
       (typed registration builds the env directly).
     - Delete `Engine/RHI/Public/Shaders/ShaderPackageLayoutCatalog.h`
       and `Engine/RHI/Private/Shaders/ShaderPackageLayoutCatalog.cpp`.
       Verify no other module includes them.
     - Strip from `Constants/ShaderCompilerConstants.h`: every
       `kManifest*` constant and the legacy `kCommandInspectManifestLegacy`
       / `kCommandCookLegacy` aliases.
     - Delete `ShaderPackages.ini` files under engine and project shader
       roots.
     - Update `Scripts/Cook/CookShaders.bat`: replace the
       `inspect-manifest` invocation with `list-shaders --validate`.
     - Update `CMake/Validation/ValidateShaderCompilerBoundary.cmake`
       forbidden-token list: remove tokens that referenced the deleted
       layout catalog if any; add `ShaderPackageLayoutCatalog` and
       `ShaderCookManifest` as forbidden tokens to prevent regressions.
- **Implementation Prompts.**
  - *"Implement `TGlobalShader<T>`, `BEGIN_SHADER_PARAMETER_STRUCT`,
    `SHADER_PARAMETER`, `SHADER_PARAMETER_TEXTURE`,
    `SHADER_PARAMETER_SAMPLER`, `SHADER_PARAMETER_RDG_BUFFER_SRV`, and
    `IMPLEMENT_GLOBAL_SHADER` in
    `Engine/RHI/Public/Shaders/Authoring/`. Macros expand into POD
    descriptors plus a `static const ShaderRegistrationDesc&` registered
    at static init."*
  - *"Introduce the shared shader-registration bootstrap that both
    `ShaderCompiler.exe` and runtime link. Move every translation unit
    containing `IMPLEMENT_GLOBAL_SHADER` into that shared surface so the
    standalone tool can enumerate shaders without depending on
    `Engine/Renderer`. Touch CMake targets as needed, but keep
    forbidden-edge validation green."*
  - *"Implement `TShaderPermutationDomain<...>`, `ShaderPermutationBool`,
    `ShaderPermutationEnum<E, Count>`. Provide `ToVector()` and
    `FromKey(ShaderPermutationKey)`. Unit-test domain enumeration,
    pruning, and stable key hashing."*
  - *"Implement `ShaderParameterStructVerifier::Verify(descriptor,
    reflection)`. Mismatch checks: missing/extra binding, type, size,
    register/space, array count. Wire into the cook so a mismatch
    becomes `SC2001` and exit code `6`."*
  - *"Implement runtime typed binding for `TShaderRef<T>` so
    `shader.SetParameters(cmdList, params)` walks the generated
    `ShaderParameterStructDescriptor`, matches it to cooked
    `ShaderReflection`, caches the binding map, and writes descriptors/
    constants with no per-shader hand-written binding code. Validate it
    with `FullscreenBlit`."*
  - *"Add CLI verbs `list-shaders`, `list-permutations <id>`,
    `inspect-shader <id> [--permutation k=v,...] [--target ...]` to
    `Tools/ShaderCompiler/Private/Cli/`. `inspect-shader` writes the
    same `DebugArtifactBundle/` layout for one shader on demand and
    prints a one-page summary to stdout."*
  - *"Convert `FullscreenBlit` to the typed authoring surface. Delete
    the legacy registration. Cook still produces the same cooked
    bytecode (modulo header version) and the renderer still draws."*
- **Guardrails.**
  - *Must:* enforce the parameter-struct verifier as **mandatory** in
    the cook path; mismatch → `SC2xxx` + exit code `6`.
  - *Must:* keep `--allow-parameter-mismatch` strictly transitional and
    never the default; CI cooks must not pass this flag.
  - *Must:* place every TU containing `IMPLEMENT_GLOBAL_SHADER` in the
    shared registration surface so the standalone tool can enumerate
    them without linking `Engine/Renderer`.
  - *Must:* delete the entire manifest world (`Manifest/` folder,
    `ShaderPackageLayoutCatalog`, `BindingRecordBuilder`,
    `StageMaskUtils`, `ShaderCompileOptionsBuilder`,
    `ShaderPackages.ini`, legacy verbs/constants) in the same PR or
    immediate follow-up.
  - *Must:* extend `ValidateShaderCompilerBoundary.cmake` with
    `ShaderPackageLayoutCatalog` and `ShaderCookManifest` as forbidden
    tokens to prevent regressions.
  - *Must not:* generate code from a manifest, an INI file, or any
    sidecar; macros + static registration are the only authoring path.
  - *Must not:* hand-write per-shader binding code; runtime binding
    flows exclusively through generated `SetParameters(...)` over the
    typed parameter struct + cooked `ShaderReflection`.
  - *Must not:* leave any `ShaderPackages.ini` or
    `ShaderPackageLayoutCatalog` reference in the tree after this
    phase.
- **Validation Gates.**
  - `ShaderCompiler.exe list-shaders` enumerates ≥1 shader with its
    permutation domain and target list, and does so without the tool
    linking `Engine/Renderer`.
  - `list-permutations FullscreenBlitPS` enumerates only legal vectors
    after `ShouldCompilePermutation` pruning.
  - `inspect-shader FullscreenBlitPS --permutation
    UseGammaCorrection=1` writes a complete `DebugArtifactBundle/`.
  - Introducing a deliberate mismatch (rename a `SHADER_PARAMETER`
    field) causes the cook to fail with `SC2xxx` and a non-zero exit
    code.
  - `--allow-parameter-mismatch` downgrades the same mismatch to a
    warning.
  - The converted `FullscreenBlit` runtime path binds exclusively via
    `shader.SetParameters(cmdList, params)`; no bespoke binding helper
    survives for that shader.
  - Cooked package round-trips through the RHI reader; renderer renders
    the converted `FullscreenBlit` shader correctly.
  - Permutation key hashing is stable across runs.
- **Increment Demo.** Terminal recording: `list-shaders` →
  `list-permutations` → `inspect-shader` → deliberate-typo → cook fails
  → fix → cook passes. `FullscreenBlit` C++ class shown side by side
  with its `reflection.json`.

### Phase 4 — Editor hot reload via out-of-process recook

- **Goal.** Saving an `.hlsl` file with the editor open triggers a
  recook; the viewport reflects the change without restart; the editor
  still links no compiler.
- **Scope of hot reload (pay special attention).** Pure HLSL edits
  (math, constants, sampling, control flow) hot-reload while the editor
  stays open. Edits that change resource bindings (parameter struct,
  root signature, descriptor layout) are **out of scope for hot reload**
  by design — they require a C++ rebuild because the typed parameter
  struct lives in C++ and the Phase 3 cook-time verifier will reject a
  mismatched cook. This split is the contract; do not try to work
  around it.
- **Why.** This is the iteration loop that makes the engine pleasant to
  work in. Today: edit `.hlsl` → close editor → run cook batch →
  relaunch editor → navigate back to the scene you were on. That kills
  the kind of debugging where you want to multiply `diffuse` by
  `float3(1, 0, 0)` to see which surfaces are actually getting hit by a
  light path. After this phase: edit `.hlsl` → save → the viewport tints
  red within one frame. The strict architectural rule is **the editor
  still links no compiler** — recook is done by spawning
  `ShaderCompiler.exe` as a subprocess and watching for an atomic signal
  file. That keeps the boundary validator green and means the same code
  path used in CI is the one used for hot reload.
- **Prerequisites.** Phase 3 gates green.
- **Work Items.**
  1. Implement `ShaderRecookSignal` (marker file written atomically by
     the tool on successful cook).
  2. Implement `ShaderRecookWatcher` in
     `Engine/Editor/ShaderRecook/` using `ReadDirectoryChangesW` + an
     explicit "Recompile shaders" menu action.
  3. Add a recook coordinator in the editor that resolves the
    `ShaderCompiler.exe` path explicitly, allows at most one active
    subprocess plus one queued rerun, and discards stale completion
    events so an older cook can never overwrite a newer save.
  4. On signal, the editor diffs the registry, asks RHI to reopen
     changed `*.spkg` files, bumps `ShaderResource::Version`.
  5. Renderer's PSO cache key includes `ShaderResource::Version` so
     a bump invalidates dependent PSOs; rebuild lazily.
  6. Editor surfaces structured diagnostics from the spawned tool's
     stderr (`--json`) in a status panel.
  7. Atomic-rename writes for both `*.spkg` and the registry.
- **Implementation Prompts.**
  - *"Implement `ShaderRecookSignal` as
    `bin/Cache/Shaders/recook.signal` written via temp-file + atomic
    rename at the end of every successful cook. Contents are the
    registry file hash."*
  - *"Implement `ShaderRecookWatcher` in
    `Engine/Editor/ShaderRecook/`. On `recook.signal` change, parse the
    new registry, diff against the previous one, call
    `RHI::ReopenCookedPackage(packageId)` for each changed entry, and
    increment `ShaderResource::Version`. Add a 'Recompile Shaders'
    menu action that spawns `ShaderCompiler.exe cook --json` and streams
    stderr into a status panel."*
  - *"Introduce an editor-side recook coordinator that resolves the
   absolute `ShaderCompiler.exe` path from the current build output,
   spawns at most one active cook process plus one queued rerun, and
   ignores stale process completions/results if a newer save arrived in
   the meantime. Failed path resolution must surface a clear editor
   diagnostic instead of silently doing nothing."*
  - *"Make the renderer's PSO cache key include
    `ShaderResource::Version` so a version bump invalidates dependent
    PSOs. Recreate them lazily on next draw."*
- **Guardrails.**
  - *Must:* spawn `ShaderCompiler.exe` as a subprocess; the editor
    process must not link any shader compiler symbol.
  - *Must:* go through one recook coordinator that resolves the tool
    path, serializes runs (one active + one queued), and ignores stale
    completions.
  - *Must:* leave prior `*.spkg` artifacts in place on cook failure and
    surface diagnostics in the editor status panel.
  - *Must:* write `*.spkg`, registry, and recook signal via temp-file +
    atomic rename; readers see old or new, never partial.
  - *Must not:* allow binding/parameter-struct edits to "hot reload";
    those require a C++ rebuild by design (see §3 callout).
  - *Must not:* add an in-process compile fallback "for speed",
    "for debug builds only", or under any other guise.
  - *Must not:* poll on tight loops; the OS file watcher and explicit
    menu action coexist (Open Question 5) — neither is a busy-wait.
- **Validation Gates.**
  - `ValidateShaderCompilerBoundary` still passes; the editor links no
    compiler symbol.
  - With editor running, edit `FullscreenBlit.hlsl` → save → viewport
    reflects the change within one frame after recook completes, no
    restart required.
  - Saving the same shader repeatedly during an in-flight cook produces
    one final up-to-date reload; stale subprocess completion cannot roll
    the viewport back to an older artifact.
  - Forcing a cook failure (introduce a syntax error) leaves the
    previous artifacts in place; editor surfaces the diagnostic without
    crashing.
  - Concurrent reads during a recook never observe a partial file.
- **Increment Demo.** Screen recording: editor open, edit shader, save,
  viewport updates; then introduce error, see diagnostic in panel, fix,
  see green.

### Phase 5 — Editor shader inspector + analysis seam + secondary backend seam

- **Goal.** Visualize compilation products inside the editor. Document
  the optional analysis and secondary-backend seams without making them
  mandatory.
- **Why.** Phases 0-4 give us a correct, fast, hot-reloadable pipeline
  but the artifacts still live as files on disk that you have to open
  in a text editor to read. This phase brings them into the editor:
  pick a shader, pick a permutation, pick a target, and see the
  preprocessed source, the reflection table, the disassembly, and the
  parameter-struct match status side by side. Example use case: a PSO
  is mysteriously big — open the inspector, switch to the Disassembly
  tab, see that DXC unrolled a loop you didn't expect, fix the source.
  The analysis seam (`IAnalysisPass`) lets us bolt on optional passes
  like `PsoStatsPass` (CSV of bytecode size + resource counts) or a
  future `RgaAnalysisPass` that calls AMD RGA for ISA / register
  pressure — without polluting the core cook path. And the
  secondary-backend seam (`SlangShaderBackend`, `GlslangShaderBackend`)
  is documented so when a real workload forces it, the integration is
  a known shape rather than a green-field design.
- **Prerequisites.** Phase 4 gates green.
- **Work Items.**
  1. Editor "Shader Inspector" panel that browses on-disk
     `DebugArtifactBundle/` directories: select shader → permutation →
     target → view preprocessed source, reflection, disassembly,
     parameter-struct match report. Editor links no compiler.
  2. Per-PSO live overlay: selected PSO in the renderer debug HUD
     shows its shader class, permutation vector, last cook timestamp,
     reflection summary.
  3. `IAnalysisPass` seam in the cooker: optional pass list invoked
     after each successful compile. Ship `PsoStatsPass` (writes
     bytecode size + resource counts to CSV) as the example.
  4. Document `SlangShaderBackend` and `GlslangShaderBackend` as
     `IShaderBackend` seams in §8. Implement only when a real workload
     forces it.
  5. Optional: `RgaAnalysisPass` skeleton that shells out to AMD RGA
     when installed and attaches ISA / register-pressure to
     diagnostics.
- **Implementation Prompts.**
  - *"Add a Shader Inspector panel to the editor. It reads only on-disk
    artifacts under `bin/Cache/Shaders/Debug/`. Tree view: shader →
    permutation → target. Detail tabs: Source (preprocessed), Reflection
    (table), Disassembly (text), Param Match (status). The editor must
    not link the compiler."*
  - *"Implement `IAnalysisPass` and `PsoStatsPass` in
    `Tools/ShaderCompiler/Private/Analysis/`. Wire `--analysis
    pso-stats` to run the pass list. Output goes to
    `bin/Cache/Shaders/Analysis/<shaderId>.csv`."*
- **Guardrails.**
  - *Must:* read on-disk `DebugArtifactBundle/` artifacts only; the
    editor still links no compiler.
  - *Must:* keep analysis passes optional and out of the default cook
    path so cook latency does not regress.
  - *Must:* document `SlangShaderBackend` / `GlslangShaderBackend` as
    `IShaderBackend` seams without implementing them until a real
    workload (Open Question 7) demands it.
  - *Must not:* let the inspector or any analysis pass call into a
    shader compiler from the editor process.
  - *Must not:* couple the inspector UI to a specific backend; it
    consumes normalized `ShaderReflection` and the bundle layout from
    §11 only.
  - *Must not:* gate Phases 0–4 on Phase 5 deliverables; Phase 5 is
    additive polish, not on the critical path.
- **Validation Gates.**
  - Inspector panel opens, lists every shader the cooker registered, and
    renders the four detail tabs without error for a known shader.
  - `ShaderCompiler.exe cook --analysis pso-stats` produces a CSV whose
    row count equals the number of cooked permutations.
  - Boundary validator still passes — the inspector reads files only.
- **Increment Demo.** Screenshot of the Shader Inspector showing
  preprocessed source + reflection + disassembly side by side; CSV
  excerpt from `pso-stats`.

### Phase tracker (snapshot)

```text
[ ] Phase 0 — Stabilize current state
    [ ] Build green   [ ] CI cook job   [ ] Boundary validator green
[ ] Phase 1 — Dependency graph + cache
    [ ] Cold cook    [ ] Warm cook = 0 backend invocations
    [ ] Targeted invalidation works   [ ] --no-cache forces recompile
[ ] Phase 2 — Backend abstraction + reflection + DebugArtifactBundle
    [ ] DXC contained to Backends/Dxc/   [ ] DXIL + SPIR-V both cooked
    [ ] PSO-grade reflection lands       [ ] DebugArtifactBundle/ writes
    [ ] Cleanup: legacy DxcShaderCompiler / DxcContext deleted
    [ ] Cleanup: ShaderCompileOptions.h / ShaderCompileResult.h moved out of RHI public
[ ] Phase 3 — Typed shaders + permutations + verification + inspect CLI
    [ ] list-shaders / list-permutations / inspect-shader work
    [ ] Parameter-struct verifier rejects mismatches
    [ ] FullscreenBlit converted as reference
    [ ] Cleanup: Manifest/ folder deleted (8 files)
    [ ] Cleanup: ShaderPackageLayoutCatalog deleted (RHI public + private)
    [ ] Cleanup: BindingRecordBuilder + StageMaskUtils + ShaderCompileOptionsBuilder deleted
    [ ] Cleanup: InspectManifestCommand + manifest constants + legacy verbs removed
    [ ] Cleanup: ShaderPackages.ini files deleted; CookShaders.bat updated
[ ] Phase 4 — Editor hot reload
    [ ] Save .hlsl → viewport updates without restart
    [ ] Failed cook surfaces diagnostic, keeps old artifacts
[ ] Phase 5 — Inspector + analysis seam + secondary backend seam
    [ ] Editor Shader Inspector panel
    [ ] PsoStatsPass CSV   [ ] Slang/Glslang documented
```

### Definition of done (whole effort)

The shader compiler effort is "done" when *all* of the following are
true at the same time:

- The legacy surface table above is fully retired — no file in that
  table still exists in the tree, no `kManifest*` constant remains in
  `Constants/ShaderCompilerConstants.h`, no `ShaderPackages.ini` exists
  under engine or project shader roots.
- Every shader in the engine is authored with `IMPLEMENT_GLOBAL_SHADER`
  and a typed parameter struct. Grep for the old patterns returns zero
  hits.
- Every translation unit containing `IMPLEMENT_GLOBAL_SHADER` lives in
  the shared shader-registration surface consumed by both the tool and
  runtime, never under `Engine/Renderer`, `Engine/Editor`, or
  `Engine/Application`.
- `ValidateShaderCompilerBoundary.cmake` enforces the new forbidden
  tokens (`ShaderPackageLayoutCatalog`, `ShaderCookManifest`, DXC outside
  `Backends/Dxc/`, compile options outside `Tools/`) and passes.
- `ShaderCompiler.exe --help` lists exactly: `cook`, `list-shaders`,
  `list-permutations`, `inspect-shader`. No legacy aliases.
- The cooked package version reflects the Phase 2 bump, with one
  documented `LoadV<N-1>` migration entry.

## 14. Open Questions

Decisions intentionally deferred until a phase needs them:

1. **Sparkle naming prefix.** No prefix for plain classes
   (`GlobalShader`, `ShaderClassDescriptor`) and `T` for templates
   (`TGlobalShader<T>`, `TShaderPermutationDomain<...>`)? Or `F`-prefix
   UE-style? Or `Spk`-prefix? *Recommend no-prefix + T.*
2. **Macro spelling.** Single `IMPLEMENT_GLOBAL_SHADER(Class, Path,
   Entry, Stage)` in cpp, or split `DECLARE_GLOBAL_SHADER` (header) +
   `IMPLEMENT_GLOBAL_SHADER` (cpp)? *Recommend single macro; class
   declaration already uses `TGlobalShader<T>`.*
3. **Permutation key strategy.** Bitfield vs sorted (axis,value) string
   hash. *Recommend bitfield with `static_assert` on axis count.*
4. **Cache storage location.** Per-user `%LOCALAPPDATA%` vs in-tree
   `bin/Cache/`. *Recommend in-tree; easier to wipe and inspect.*
5. **Hot reload trigger source.** OS file watcher
   (`ReadDirectoryChangesW`) vs explicit "Recompile" button vs both.
   *Recommend both: button is reliable, watcher is convenient.*
6. **Schema migration policy.** Single-version-only (recook everything)
   vs read-old-versions for one release. *Recommend single-version
   while the engine is small.*
7. **Slang adoption trigger.** What concrete capability would push us
   from DXC-SPIRV to a `SlangShaderBackend`? (Likely: modules /
   generics, multi-target IR sharing, or a target DXC cannot reach.)
