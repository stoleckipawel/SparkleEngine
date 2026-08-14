# Shader Authoring and Cooked Program Architecture

Status: target proposal; not implementation evidence
Date: 2026-08-13
Scope: shader source import, shader-type registration, permutation policy, compilation, derived-data caching, cooking, runtime lookup, render-pass integration, PSO handling, recooking, hot reload, and diagnostics

## Purpose

This document decides how SparkleEngine should identify render passes and shaders without requiring authors to maintain parallel strings such as `DirectLighting`, `RendererShaderPackages::DirectLighting`, binding-layout names, pipeline names, source basenames, and cooked-package names.

It also maps Epic's global-shader lifecycle onto Sparkle from source import through runtime pipeline creation, explains which Unreal patterns are worth adopting, and gives an ordered migration that can be used to learn those patterns through implementation. It refines the shader-specific conclusions from [External Renderer Repository Comparison](ExternalRendererComparison.md). That comparison remains the source-linked broad research document. This document owns the proposed local shader-authoring direction. Code and executable build configuration remain the authority for what is implemented today.

## Decision Summary

1. Sparkle should adopt Unreal's global-shader architecture as its core mental model: immutable typed shader metadata, explicit virtual source path/entry/stage, typed permutation rules, complete compile inputs, asynchronous jobs, shader maps, a deduplicated cooked code library, typed runtime references, and separate PSO caching.
2. A semantic render-pass label is necessary for frame-graph diagnostics, GPU markers, errors, captures, and profiling.
3. `static constexpr const char* PassName = "DirectLighting";` is not necessary as a separately authored field on every pass. The default label should be generated from one typed pass declaration, with an explicit per-instance label only when the same pass type is scheduled for different work.
4. A shader filename must not be the render-pass identity. It is one compile input, not the meaning of a frame-graph operation.
5. Renderer authors should register typed shaders using virtual source path, entry point, stage, parameter structure, and permutation/environment rules. They should compose those shader types into a typed shader program. They should not manually invent a cooked-package string.
6. One typed parameter structure should be the authority for shader reflection validation, frame-graph resource declaration, and pass binding. Sparkle should remove the current parallel pass-parameter and shader-parameter declarations.
7. Sparkle should keep cooked shader data but separate three concepts that the current "package" partly conflates: a logical shader map, a content-addressed code library, and program/pipeline metadata. Physical files are generated delivery details, not renderer authoring concepts.
8. The cooker should generate program membership, stage masks, layout identity, compile keys, artifact keys, and the runtime manifest. Every ambiguity or collision must be a deterministic error.
9. Shader bytecode caching and graphics/compute pipeline caching are different systems. The latter must include complete fixed-function and render-target state and should support asynchronous precaching plus hit/miss/too-late evidence.
10. Sparkle should copy Unreal's separations and invariants, not its full material, vertex-factory, distributed-build, or plugin machinery until a real Sparkle workload requires those systems.

The short answer is therefore:

> Keep the concept of a pass label, remove the repeated `PassName` literal, do not replace it with the shader filename, remove manual renderer package names, and make typed shader metadata drive compilation, cooking, runtime lookup, parameter binding, and PSO preparation.

## Why the Shader Filename Is Not the Pass Name

A filename answers "where is source text?" A pass name answers "what GPU operation is this?" Coupling them creates incorrect behavior in ordinary cases:

- One source file can contain several entry points or stages.
- One graphics program can use stages from several files. Sparkle's current `GBuffer` program already uses `GBufferVS.hlsl` and `GBufferPS.hlsl`.
- The same compiled shader can be reused by multiple semantic passes.
- A pass type can be scheduled several times with instance-specific labels such as a mip number, eye, cascade, phase, or view.
- Renaming or moving a source file should invalidate compilation, but it should not silently rename profiler history, GPU markers, frame-graph nodes, or a logical runtime program.
- Two directories can contain the same source basename. Sparkle's current basename-derived fallback cannot distinguish them.
- Include files and shader libraries are source dependencies but are not independently executable passes.

Using a filename as a default shader debug name is reasonable. Using it as the durable pass, program, layout, and artifact identity is not.

## Required Identity Model

Each identity has one responsibility and one owner.

| Identity | Meaning | Proposed authority | Must not be derived solely from |
| --- | --- | --- | --- |
| Render-pass type | C++ operation and parameter contract | typed pass declaration | shader filename |
| Render-pass label | frame-graph, GPU marker, diagnostic, and profiler text | generated default from pass declaration; optional instance override | cooked artifact name |
| Shader type | one compilable shader specialization | typed shader registration | pass label |
| Source identity | normalized virtual path plus transitive include closure | shader compiler dependency graph | basename |
| Entry identity | source path, entry point, and stage | typed shader registration | source path alone |
| Permutation identity | defines, specialization inputs, platform, feature level, and compiler environment | deterministic compiler key | display/debug name |
| Shader program | ordered stage set or ray-tracing library/export set | typed program composition | manually repeated package string |
| Binding layout | full structural binding contract | generated parameter/reflection contract hash | parameter count |
| Cooked artifact | backend binaries, reflection, layouts, features, and provenance | cooker-generated content key | pass label or filename |
| Pipeline | shader program plus fixed-function and target state | pipeline cache key | shader program alone |

Pass labels and debug names may be human-readable. Lookup and cache identities must be typed or hash-backed and collision checked.

## Target Shape

```text
  Renderer-owned authoring                     Generated compile/cook data
  ========================                     ===========================

  +------------------------+                   +-------------------------+
  | typed render pass      |-- default label ->| frame graph / GPU marker|
  | program + FParameters  |                   | diagnostics only        |
  +-----------+------------+                   +-------------------------+
              |
              | uses typed program
              v
  +------------------------+     registers     +-------------------------+
  | shader program         |------------------>| frozen shader catalog   |
  | ordered shader types   |                   | types + permutations    |
  +-----------+------------+                   +------------+------------+
              |                                             |
              v                                             v
  +------------------------+                   +-------------------------+
  | shader type            |                   | compile jobs + DDC      |
  | virtual source + entry |                   | input hash + validation |
  | stage + FParameters    |                   +------------+------------+
  | permutation + policy   |                                |
  +------------------------+                                v
                                               +-------------------------+
                                               | GlobalShaderMap         |
                                               | ProgramManifest         |
                                               | CookedShaderLibrary     |
                                               +------------+------------+
                                                            |
                                                            v
                                               +-------------------------+
                                               | typed runtime lookup    |
                                               | RHI shaders + PSO cache |
                                               +-------------------------+
```

The RHI owns generic shader descriptors, cooked schemas, validation primitives, and backend object creation. The Renderer owns its shader types, typed program compositions, pass types, and pass-to-program use. The shader compiler consumes the renderer's registration catalog without taking ownership of renderer runtime policy.

## Unreal-Aligned System Model

Unreal does not have one object called a "shader package" that owns every concern. Its useful architectural pattern is a pipeline of distinct authorities. Sparkle should preserve that separation even when its implementation is much smaller.

| Unreal concept | Responsibility | Sparkle target | Current Sparkle approximation |
| --- | --- | --- | --- |
| virtual shader source paths | stable source namespace independent of checkout location | `ShaderSourceMountTable` with `/Engine`, `/Project`, and `/Plugin/<Name>` roots | project-first then engine physical path search |
| `FShaderType` / `FGlobalShaderType` | immutable shader metadata and compile hooks | `ShaderTypeDescriptor` emitted by typed registration | `ShaderRegistrationDesc` |
| `TShaderPermutationDomain` | typed, bounded permutation dimensions and stable IDs | `TShaderPermutationDomain<...>` | free-form compile defines and package variants |
| `FShaderCompilerInput` | complete read-only input for one compilation | `ShaderCompileRequest` | `ShaderCompileOptions` plus cook-node metadata |
| `FShaderCompileJob` and job key | scheduled unit, priority, deduplication, result, diagnostics | `ShaderCompileJob` and `ShaderCompileJobKey` | `CookNode` plus `TaskExecutor` task |
| `FShaderCompilingManager` and Shader Compile Workers | asynchronous coordination and compiler-process isolation | cooker-owned `ShaderCompileCoordinator`; optional worker-process pool when justified | one out-of-process cooker with bounded in-process stage tasks |
| shader job cache / DDC | disposable content-addressed derived compilation results | layered `IShaderArtifactStore` | local per-stage artifact store |
| Global Shader Map / `TShaderMapRef<T>` | typed permutation lookup and shader lifetime | `GlobalShaderMap` / `TShaderRef<T>` | pass runtime cache loading by package ID |
| `FShaderMapResourceCode` | code hashes and map resource content | generated map resource record | code embedded in each `.sparkshader` program package |
| `FShaderCodeLibrary` | cook-time collection of unique code and runtime loading by hash | `CookedShaderLibrary` | no cross-package code library |
| `FShaderPipelineType` | a declared set of shader stages | `TShaderProgram<Stages...>` | stages grouped by repeated package string |
| RDG shader parameter structs | one typed resource/binding contract for shader and graph pass | one pass/shader `FParameters` authority | separate pass parameters and registered shader parameters |
| RDG event name | diagnostic/profiler identity of one graph operation | generated default `RenderPassLabel`, optional instance override | repeated `PassName` literal |
| PSO precache / shader pipeline cache | full pipeline-state preparation and hitch tracking | `RenderPipelineCache` with async precache evidence | pipeline creation coupled to pass runtime creation |

The mapping is architectural, not a request to copy Unreal class names or source code. Sparkle should keep names that fit its own standards while retaining the responsibility boundaries.

### Full Lifecycle

```text
AUTHORING / MODULE STARTUP

  physical roots                 typed declarations
  Engine/Shaders --------+       DirectLightingCS
  Project/Shaders -------+-----> source mounts + shader-type registry
  Plugin/Shaders --------+       source / entry / stage / parameters / policy
                                 |
                                 +--> validate, sort, freeze before first query

COMPILE / DERIVED DATA

  frozen shader types + requested target/platform/features
                         |
                         v
              enumerate supported permutations
                         |
                         v
              immutable compile requests
                         |
             preprocess + include dependency graph
                         |
                         v
              complete input/content hash
                    +----+----+
             cache hit   |    cache miss
                    |    |       |
                    |    |       v
                    |    |  bounded async compile jobs
                    |    |       |
                    +----+-------+
                         v
              reflection + parameter validation

COOK / PUBLICATION

  validated outputs
        |
        +--> GlobalShaderMap: (type, permutation, target) -> code hash
        +--> ProgramManifest: program -> ordered stage/map references
        +--> CookedShaderLibrary: unique code hash -> compressed bytecode
        +--> provenance / layout signatures / feature requirements
                         |
                         v
              deterministic transactional publication

RUNTIME

  open library -> load shader map -> TShaderRef<T, Permutation>
                                      |
                                      v
                           lazy/preloaded RHI shader
                                      |
                         typed program + complete PSO state
                                      |
                                      v
                         frame-graph pass + diagnostic label

DEVELOPMENT RELOAD

  changed virtual path -> reverse include dependencies -> affected shader jobs
       -> validated replacement map/library generation -> atomic swap
       -> old generation retained until all recorded GPU submissions complete
```

### The Separation That Prevents Bloat

The lean design is not "compile every file automatically." It is automatic generation from small, explicit declarations:

- source files provide implementation text and includes;
- shader types provide executable entry points and compile policy;
- permutation domains provide the finite variant space;
- programs provide legal stage composition;
- parameter metadata provides the resource contract;
- shader maps provide logical typed lookup;
- code libraries provide deduplicated physical delivery;
- pipeline descriptors add fixed-function state;
- render-pass labels provide diagnostics only.

This is why filenames cannot safely replace shader types, programs, passes, or PSOs. Automation should derive everything that is derivable, while authors still state facts the tool cannot infer: entry point, stage, supported permutations, feature requirements, and semantic composition.

## Proposed Authoring Experience

The exact API spelling is an implementation choice. The intended amount of authored information is illustrated below:

```cpp
class DirectLightingCS final : public TGlobalShader<DirectLightingCS>
{
public:
    using FParameters = DirectLightingParameters;
    using FPermutationDomain = TShaderPermutationDomain<FUseRayQuery>;

    static bool ShouldCompilePermutation(const ShaderPermutationParameters& parameters);
    static void ModifyCompilationEnvironment(
        const ShaderPermutationParameters& parameters,
        ShaderCompilerEnvironment& environment);
    static bool ValidateCompiledResult(
        const ShaderPermutationParameters& parameters,
        const ShaderCompilerOutput& output,
        ShaderDiagnosticSink& diagnostics);
    static ShaderPrecacheRequest ShouldPrecachePermutation(
        const ShaderPermutationParameters& parameters);
};

IMPLEMENT_GLOBAL_SHADER(
    DirectLightingCS,
    "/Engine/Passes/Deferred/DirectLighting.hlsl",
    "main",
    Compute);

using DirectLightingProgram = TComputeShaderProgram<DirectLightingCS>;

SPARKLE_RENDER_PASS(DirectLightingPass, DirectLightingProgram, DirectLightingParameters);
```

The final form should have these properties:

- `SPARKLE_RENDER_PASS` or equivalent registers one pass type and generates its default diagnostic label from the type token. There is no hand-written `"DirectLighting"` beside `DirectLightingPass`.
- `TShaderProgram<...>` declares the complete stage set. A compute program has one compute shader; a raster program can have vertex and pixel shader types; a ray-tracing program owns its library exports and hit groups.
- The shader registration still states virtual source path, entry point, and stage because those are independent compile inputs.
- The shader type owns compile eligibility, environment mutation, compiled-output validation, precache policy, and a typed permutation domain. The empty permutation domain remains the zero-cost default.
- `DirectLightingParameters` is declared once. It drives frame-graph dependency declaration, runtime binding, reflection verification, and the persisted layout signature.
- The build exposes shader registration objects to both the cooker and runtime as it does today. Raw shader directory scanning does not replace registration: not every `.hlsl` or `.hlsli` file is an entry shader.
- Frame-graph APIs obtain the default label through `RenderPassTraits<TPass>` or an equivalent generated trait. An overload accepts an instance label when one pass type is dispatched several times.
- Binding-layout and pipeline debug names are generated from the pass/program label. They are never lookup authorities.
- Compiler RTTI strings, `typeid(T).name()`, and implementation-specific pretty-function text are not serialized. The registration macro or generated catalog emits deterministic text, IDs, and source declaration locations.

For example, a multi-instance pass should remain able to say:

```cpp
builder.Dispatch<ExposureDownsamplePass>(
    RenderPassLabel::Format("ExposureDownsample mip {}", mip),
    parameters,
    width,
    height);
```

That label describes this graph node. It does not select shader bytecode.

## Cooked Artifact Model

Sparkle does need cooked shader data; it does not need authors to name each physical package manually.

The current package already carries information that a loose compiled shader file cannot represent safely:

- declared stages and package kind
- DXIL and SPIR-V records
- entry points and ray-tracing exports/hit groups
- reflection and merged resource bindings
- backend-specific pipeline layouts
- specialization inputs
- required feature flags
- compiler backend and target provenance
- source, bytecode, binding-layout, and package hashes

The target model uses four keys with different scopes:

1. `ShaderTypeId`: a stable ID emitted from the typed shader declaration. Together with permutation and target it addresses a shader-map entry.
2. `CompileJobKey`: a content hash of all inputs to one stage compile, including virtual source path, transitive source contents, entry point, stage, permutation, environment, target, parameter-layout signature, compiler backend, and compiler version. It must not contain the program/package name, because identical compile work should deduplicate across consumers.
3. `ShaderCodeHash`: a content hash of validated backend bytecode plus required runtime metadata. It addresses unique code inside the cooked library.
4. `ProgramId`: a stable, typed logical ID generated from program composition. The generated manifest maps it to ordered shader-map entries/code hashes and a compatible layout signature.

This allows source edits to invalidate compilation, source moves to be diagnosed correctly, identical compile jobs and identical output code to be deduplicated at their proper layers, and logical program lookup to remain independent of a file basename.

The current one-`.sparkshader`-per-program layout is an acceptable migration container, but its schema should first separate map records, program records, and code records. A later writer can emit one global/project/plugin library or several chunks without changing renderer authoring. The physical grouping decision should use startup I/O, file-open count, compression, patching, and preload evidence; the logical separation should not wait for that measurement.

```text
logical identity                         physical delivery
================                         =================

ShaderTypeId + Permutation + Target ---> GlobalShaderMap entry -----+
                                                                  |
ProgramId -----------------------------> ordered stage references  |
                                                                  v
ShaderCodeHash ------------------------------------------------> code library
                                                                  |
PSO key = shader hashes + complete pipeline state                  |
                                                                  v
                                                        RHI shader / pipeline
```

## External Precedent and What Sparkle Adopts

The sources below are precedents, not local implementation authority. Repository links are pinned to the reviewed revisions where possible.

### Unreal Engine as the Core Model

Epic's global-shader documentation registers a C++ shader type against a source file, entry point, and shader stage. Its example deliberately registers vertex and pixel shader types from the same file with different entry points, then retrieves instances by type from the Global Shader Map. `FShaderType` metadata also carries the source filename, function, frequency, permutation count, parameter metadata, and hooks for compile eligibility, environment modification, output validation, and precaching. A source file is therefore an input to a shader type, never the universal runtime identity.

Epic's `FShaderCompilerInput` gathers the read-only inputs for one compile, including the virtual source path, entry point, target, platform/format, environment, parameter metadata, debug information, and its job-cache hash. `FShaderCompileJob` pairs input, key, preprocess output, compiler output, and diagnostics. `FShaderCompilingManager` coordinates priorities, pending jobs, result application, cancellation, worker processes, and optional distributed execution. Epic explains that Shader Compile Workers provide process-level parallelism around compiler implementations that may otherwise serialize internally.

Unreal stores disposable compile outputs in its Derived Data Cache, whose hierarchy can include local, shared, and packaged read-only layers. Cooking then collects unique shader code into shader libraries. At runtime `FShaderCodeLibrary` can open project/plugin libraries, test for code by hash, and preload or release code. `FShaderMapResource` separately owns the render-resource lifetime and can create/preload individual RHI shaders. This is the reason Sparkle should separate derived compilation cache, logical shader map, cooked code library, and live RHI objects.

Unreal's Render Dependency Graph uses shader parameter structures for shader bindings and graph-visible resources, while `FRDGBuilder::AddPass` receives a separate event name for debugging and profiling. Its parameter metadata exposes both a layout hash and a strong persistable layout signature. This is stronger and less repetitive than Sparkle's current two independent Direct Lighting parameter declarations plus count-only runtime compatibility fallback.

Finally, Unreal's PSO precaching operates above shader compilation. It assembles full graphics/compute pipeline descriptions, compiles asynchronously, and records hits, misses, untracked uses, and requests that completed too late. Shader bytecode availability alone is not proof that the driver-level pipeline was prepared without a hitch.

Adopt:

- typed shader registration
- explicit virtual source path, entry point, and stage
- typed permutation domains and compile-policy hooks
- complete immutable compile inputs, deterministic job keys, in-flight deduplication, priorities, and cancellation
- typed shader-map lookup and independently owned RHI shader lifetime
- compile keys derived from the full input closure and compiler provenance
- a disposable derived-data cache outside runtime packages
- cook-time collection and deduplication of global shader code
- one shader/graph parameter metadata authority with a persistable structural signature
- render-graph event labels kept separate from shader identity
- separate PSO precaching, cache keys, and validation evidence

Do not copy yet:

- Unreal's full material, vertex-factory, and permutation universe
- distributed compilation, cloud cache, and worker-farm infrastructure before local scale justifies it
- engine-scale macro, plugin, chunking, and patching complexity where Sparkle has no corresponding workload

Sources:

- [Epic: Adding Global Shaders](https://dev.epicgames.com/documentation/en-us/unreal-engine/adding-global-shaders-to-unreal-engine)
- [Epic: `GetShaderSourceFilePath`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/GetShaderSourceFilePath)
- [Epic: `FShaderType`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderType)
- [Epic: `TShaderPermutationDomain`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/TShaderPermutationDomain)
- [Epic: `FShaderCompilerInput`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompilerInput)
- [Epic: `FShaderCompileJob`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompileJob)
- [Epic: `FShaderCompileJobKey`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompileJobKey)
- [Epic: `FShaderCompilingManager`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FShaderCompilingManager)
- [Epic: Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [Epic: Derived Data Cache](https://dev.epicgames.com/documentation/en-us/unreal-engine/derived-data-cache?application_version=5.3)
- [Epic: `FShaderCodeLibrary`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCodeLibrary)
- [Epic: `FShaderLibraryCooker`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderLibraryCooker)
- [Epic: `FShaderMapResource`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderMapResource)
- [Epic: `FShaderPipelineType`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderPipelineType)
- [Epic: `FShaderParametersMetadata`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderParametersMetadata)
- [Epic: Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Epic: `FRDGBuilder::AddPass`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRDGBuilder/AddPass)
- [Epic: PSO Precaching](https://dev.epicgames.com/documentation/en-us/unreal-engine/pso-precaching-for-unreal-engine)
- [Epic: Debugging the Shader Compile Process](https://dev.epicgames.com/documentation/en-us/unreal-engine/debugging-the-shader-compile-process-in-unreal-engine)

### NVIDIA Donut and NVRHI

Donut's `ShaderFactory` resolves bytecode using a filename plus entry point and selects an embedded DXBC, DXIL, or SPIR-V variant when available. It may use the filename as the shader object's default debug name. Its deferred-lighting implementation nevertheless creates the shader from `deferred_lighting_cs.hlsl` and separately emits the semantic GPU marker `DeferredLighting`.

NVRHI keeps `shaderType`, `debugName`, and `entryName` as distinct fields in `ShaderDesc`; a shader library resolves a shader by entry name and shader type. This is useful evidence that the low-level RHI description should not be responsible for Sparkle's source-package authoring policy.

Adopt:

- source plus entry point as load/compile inputs
- automatic backend-bytecode selection
- separate semantic GPU markers and debug names
- a narrow RHI descriptor that consumes bytecode instead of inventing renderer package ownership

Sources:

- [NVIDIA Donut `ShaderFactory.cpp` at `bfdebdd`](https://github.com/NVIDIA-RTX/Donut/blob/bfdebdd7dd5455c503b2737a1967a4ef651c145b/src/engine/ShaderFactory.cpp)
- [NVIDIA Donut `DeferredLightingPass.cpp` at `bfdebdd`](https://github.com/NVIDIA-RTX/Donut/blob/bfdebdd7dd5455c503b2737a1967a4ef651c145b/src/render/DeferredLightingPass.cpp)
- [NVIDIA NVRHI `nvrhi.h` at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/include/nvrhi/nvrhi.h)

### AMD Cauldron and FidelityFX

Cauldron's reviewed D3D12 paths compile shaders with an explicit file, entry point, target profile, and define list. It hashes shader source recursively through includes and hashes defines. Its rendering code uses separate user-marker strings such as `GltfPbrPass::DrawBatchList`.

FidelityFX uses an effect-specific pass enum and permutation flags to select generated shader blobs, then gives the created pipeline a separate human-readable name. That is a stronger fit for Sparkle's target than filename-derived package identity: a typed logical pass/program chooses a generated variant, while the debug name remains presentation.

Adopt:

- include-aware and define-aware compile identity
- typed pass/program selection
- generated backend blobs and permutation lookup
- separate pipeline/debug presentation names

Sources:

- [AMD Cauldron `ShaderCompiler.h` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/common/base/ShaderCompiler.h)
- [AMD Cauldron `ShadowResolvePass.cpp` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/PostProc/ShadowResolvePass.cpp)
- [AMD Cauldron `GltfPbrPass.cpp` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/GLTF/GltfPbrPass.cpp)
- [AMD FidelityFX optical-flow program selection at `60f4ea8`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/FidelityFX/framegeneration/fsr3/internal/ffx_opticalflow.cpp)

## Current Sparkle Findings

The 2026-08-13 source review found:

- 28 pass classes declare `static constexpr const char* PassName`.
- `RendererShaderPackages.h` declares 28 package strings.
- Renderer shader-registration sources make 29 calls to `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE` and no calls to the package-inferred `IMPLEMENT_GLOBAL_SHADER` path.
- The three names normally repeat: pass name, package constant, and generated binding/pipeline debug names.
- Most current programs contain one compute stage. `GBuffer` demonstrates the legitimate multi-file, multi-stage case with a vertex shader and a pixel shader sharing one cooked program.
- `BuildShaderPackageIdFromSourcePath` derives only the filename stem, so same-basename sources in different directories collide and a source rename changes the fallback package ID.
- `GlobalShaderRegistry::Register` silently ignores a repeated shader name instead of reporting a conflicting declaration.
- The contract validator catches empty IDs, duplicate shader names, package layout/kind mismatch, and duplicate stages, but the registry's early duplicate suppression can prevent the validator from seeing the conflict.
- Direct Lighting and comparable passes declare resources twice: `DirectLightingPassParameters` drives frame-graph/pass binding, while `DirectLightingCS::FParameters` independently drives shader reflection verification. Both lists can compile while drifting semantically.
- `PassBinder` accepts either the same parameter-layout instance or merely the same parameter count. Equal counts do not prove equal binding order, kinds, names, visibility, or sizes.
- Source resolution is based on physical paths and searches project shaders before engine shaders. Includes search the includer's directory, a primary root, then additional roots, and absolute include paths are accepted. This is functional locally but lacks an explicit virtual namespace, permits machine-specific source identity, and can silently shadow an engine shader with a project file of the same relative path.
- The preprocessor already expands nested includes, detects recursion, handles `#pragma once`, and emits line directives. `IncludeClosureHasher` and compile-option hashing feed a versioned per-stage cache key.
- That cache key currently includes the package identity and binding-layout identity. This prevents otherwise identical compile work from being shared across programs and mixes compile identity with program composition.
- `ShaderCookPlanExecutor` already performs bounded parallel stage jobs through `TaskExecutor`, and each job validates parameter metadata on both cache hits and compiler misses. This should evolve into the Unreal-style job model rather than be replaced.
- The local `IShaderArtifactStore` seam and atomic cache publication are good foundations for a layered disposable derived-data cache. Runtime cooked data must remain separate from that cache.
- changed-source monitoring detects that some shader file changed, then invokes a full catalog `cook`. Include-aware stage cache hits avoid recompiling unchanged jobs, but the planner does not yet select only shader types and programs affected through reverse dependencies.
- The cooker publishes selected `.sparkshader` files, a registry, and a recook signal as one transactional file set. The registry is not currently the runtime lookup authority; runtime computes a path directly from the manually repeated package ID.
- Bytecode is embedded per program package and `ShaderCacheKey` contains package identity, so Sparkle has neither compile-job deduplication across program consumers nor a cooked unique-code library addressed by code hash.
- `RenderPassRuntimeCache` already builds and validates a complete replacement generation, atomically activates it, and retires the old generation only after recorded RHI submission tokens complete. This is a strong lifetime pattern and must be preserved.
- The editor already launches the shader cooker out of process, coalesces one follow-up request, rejects stale publications, and leaves the active generation unchanged after cook or runtime-validation failure.
- CLI validation structurally validates the catalog but cooks and inspects only the representative `ComputeClear` artifact.
- the existing build already discovers and links the shader-registration object library into compiler/runtime consumers, so authoring automation does not require scanning all shader source files.
- 28 generated `.sparkshader` artifacts were present in the reviewed development artifacts, totaling 1,994,681 bytes. This is not evidence that 28 physical files are a performance problem.

Relevant implementation entry points:

- [`GlobalShader.h`](../../Engine/RHI/Public/Shaders/Authoring/GlobalShader.h)
- [`ShaderAuthoring.cpp`](../../Engine/RHI/Private/Shaders/ShaderAuthoring.cpp)
- [`RendererShaderPackages.h`](../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h)
- [`DirectLightingPass.h`](../../Engine/Renderer/Private/Passes/Deferred/DirectLightingPass.h)
- [`DirectLightingPass.cpp`](../../Engine/Renderer/Private/Passes/Deferred/DirectLightingPass.cpp)
- [`DirectLightingShaders.cpp`](../../Engine/Renderer/ShaderRegistrations/DirectLightingShaders.cpp)
- [`FrameGraphBuilder.h`](../../Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h)
- [`ShaderContractCatalogBuilder.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractCatalogBuilder.cpp)
- [`ShaderContractValidator.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractValidator.cpp)
- [`ShaderCookPlanExecutor.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderCookPlanExecutor.cpp)
- [`ShaderCookNodeExecutor.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderCookNodeExecutor.cpp)
- [`IncludeClosureHasher.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/Cache/IncludeClosureHasher.cpp)
- [`CookedShaderPackage.h`](../../Engine/RHI/Public/Shaders/CookedShaderPackage.h)
- [`CookedPackageWriter.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/CookedPackageWriter.cpp)
- [`CookedShaderPackageCache.cpp`](../../Engine/RHI/Private/Shaders/CookedShaderPackageCache.cpp)
- [`PassBinder.cpp`](../../Engine/Renderer/Private/Pipeline/PassBinder.cpp)
- [`RenderPassRuntimeCache.cpp`](../../Engine/Renderer/Private/Pipeline/RenderPassRuntimeCache.cpp)
- [`ShaderSourceChangeTracker.cpp`](../../Engine/Application/Private/ShaderRecook/ShaderSourceChangeTracker.cpp)
- [`ShaderRecookCoordinator.cpp`](../../Engine/Application/Private/ShaderRecook/ShaderRecookCoordinator.cpp)
- [`ShaderCompilerProcess.cpp`](../../Engine/Application/Private/ShaderRecook/ShaderCompilerProcess.cpp)
- [`ValidateShaderCompilerCli.cmake`](../../Tools/Shaders/ShaderCompiler/ValidateShaderCompilerCli.cmake)

## Changes to Make

### 1. Introduce a Virtual Shader Source Namespace

Create one immutable mount table before shader registration is frozen:

```text
/Engine/...        -> <engine root>/Shaders/...
/Project/...       -> <active project>/Shaders/...
/Plugin/<Name>/... -> <plugin root>/Shaders/...
```

Required behavior:

- shader registrations and authored root includes use canonical virtual paths, never checkout-specific absolute paths;
- relative includes resolve against the including virtual file, while root includes resolve through the mount table;
- path separators, `.` segments, case policy, and Unicode policy normalize exactly once;
- `..` traversal outside a mount, absolute authored paths, unknown mounts, overlapping mounts, duplicate virtual paths, and case-only collisions are errors;
- a project shader does not silently override an engine shader through search order; ownership is explicit in `/Project` or `/Engine`;
- the resolver returns both virtual identity and physical read path, but only virtual identity and source content enter diagnostics, dependency records, and portable compile keys;
- mounts are registered before any shader type whose source depends on them. Late mount registration is an error.

This follows Unreal's virtual-source-path model without importing its plugin/module loader. Sparkle's current project and engine roots can supply the first two mounts directly.

### 2. Make the Shader-Type Registry Immutable and Diagnostic

Replace the mutable string catalog with frozen metadata emitted by typed declarations. A `ShaderTypeDescriptor` should contain:

- stable type ID and human-readable type name;
- declaration file and line;
- owning renderer module/provider;
- virtual source path, entry point, and stage/frequency;
- parameter metadata and strong layout signature;
- permutation count/domain metadata;
- `ShouldCompilePermutation`, `ModifyCompilationEnvironment`, `ValidateCompiledResult`, and `ShouldPrecachePermutation` callbacks;
- feature/capability requirements and ray-tracing export metadata where applicable.

Registration should have an explicit lifecycle:

```text
collect declarations -> validate all -> deterministic sort -> freeze -> query
```

Duplicate IDs, conflicting source/entry/stage declarations, missing parameter metadata, invalid stages, and late registration must report both declarations and fail. Silently ignoring a duplicate is forbidden. Registry order must never depend on static initialization order.

### 3. Use Typed, Bounded Permutation Domains

Add a small Unreal-like `TShaderPermutationDomain<Dimensions...>` with an empty-domain default. A dimension declares its valid values and deterministic encoding; a domain converts a typed vector to and from one stable permutation ID.

- `ShouldCompilePermutation` removes unsupported combinations before jobs are created.
- `ModifyCompilationEnvironment` turns the typed vector into defines, specialization constants, or backend options.
- `ShouldPrecachePermutation` must never request a permutation rejected by `ShouldCompilePermutation`.
- every compiled permutation records a readable dimension/value expansion in diagnostics and manifests.
- free-form defines remain possible only as compiler-environment inputs owned by a shader type or platform, not as ad hoc renderer call-site strings.
- a value should use specialization constants only when the target backend and runtime use genuinely benefit; otherwise it remains a compile-time permutation.

Sparkle should not add material or vertex-factory permutations. Current global passes need only the dimensions demonstrated by actual variants, such as ray-query availability or device-address mode.

### 4. Unify Shader Parameters and Frame-Graph Pass Parameters

Use one C++ parameter structure as the authority for all of the following:

```text
typed FParameters
      |
      +--> shader registration and root-parameter metadata
      +--> compile/reflection verification
      +--> frame-graph read/write dependency declaration
      +--> runtime parameter instance and binding
      +--> persisted layout signature
```

The field description must preserve shader name, C++ field identity, resource kind/dimension, array count, access, stage visibility, semantic/resource domain, value size/alignment, and source declaration location. The frame graph may add graph-only metadata, but it must decorate the same field rather than repeat the resource list in a second struct.

Remove parameter-count-only compatibility. Runtime accepts the same frozen metadata instance or the same strong structural layout signature; in validation builds it should also report the first differing field. Reflection validation remains mandatory on cache hits and compiler misses for every selected backend.

### 5. Generate Semantic Pass Labels and Typed Programs

- Add one pass trait/declaration mapping a pass type to its single parameter type, pipeline kind, and typed shader program.
- Generate the default diagnostic label from the declared pass type token and preserve an explicit per-instance label for mip, cascade, eye, phase, or view-specific scheduling.
- Introduce `TComputeShaderProgram<CS>`, `TGraphicsShaderProgram<VS, PS, ...>`, and a ray-tracing library/program descriptor with explicit exports and hit groups.
- Derive stage membership, package kind, feature union, layout compatibility, default binding-layout debug text, and program ID from typed composition.
- Remove the 28 per-class `PassName` literals, `RendererShaderPackages.h`, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, and repeated runtime `PackageId` fields in the same ownership migration.
- Keep source path, entry point, and stage explicit on each shader type; these are not derivable from the pass.

### 6. Formalize Compile Requests, Jobs, and Results

The cooker should create one immutable `ShaderCompileRequest` per `(ShaderTypeId, PermutationId, Target)` after compile-policy filtering. It should contain every read-only input needed to reproduce compilation:

- virtual source path, entry point, stage, and preprocessed/include dependency information;
- target platform, shader model/profile, backend, backend version, and output format;
- ordered canonical defines, specialization values, binding remaps, optimization/debug/strip/warning policy;
- parameter-layout signature, required symbols/exports, feature requirements, and shader schema versions;
- stable debug group/type/permutation names that do not participate in semantic lookup.

`ShaderCompileJobKey` hashes the canonical request and complete source content closure. `ShaderCompileJob` adds priority, cancellation state, preprocess output, compiler output, timings, diagnostics, dependencies, and cache status. Jobs with the same key share one in-flight or completed result even when several programs consume it.

The current `ShaderCookPlanExecutor` and `TaskExecutor` are the starting coordinator. Add deterministic job deduplication, explicit priorities, bounded memory, cancellation propagation, and time-sliced result integration. Keep one out-of-process cooker. Introduce a persistent pool of `ShaderCompileWorker` child processes only when measurements show compiler serialization, crash isolation, or memory recovery requires it; the job serialization boundary should make that addition possible without changing authoring.

### 7. Turn the Artifact Store into a Derived-Data Cache

Keep `IShaderArtifactStore`, but define it as a chain of disposable content-addressed stores:

```text
request -> local writable -> optional shared/read-only -> compile -> publish outward
```

- local disk is the required first layer;
- a shared team cache and read-only build cache are optional configuration, not runtime requirements;
- cache entries are safe to delete and must always be reproducible from source plus toolchain;
- cache format, compiler/backend version, target, full compile input, and parameter signature are versioned in the key;
- corrupted or incompatible entries are quarantined/missed and regenerated, never trusted;
- cache lookup/publish statistics expose hit, miss, duplicate-in-flight, read/write bytes, and time saved;
- cooked runtime libraries never depend on the DDC being installed or writable.

Most importantly, remove program/package identity from the stage compile key unless it actually changes compilation. Program composition and display names belong to manifest keys, not the reusable compiler job key.

### 8. Build a Global Shader Map and a Deduplicated Code Library

At cook completion, generate these independent outputs:

1. `GlobalShaderMap`: sorted mapping from `(ShaderTypeId, PermutationId, Target)` to `ShaderCodeHash`, parameter-layout signature, and required runtime metadata.
2. `ShaderProgramManifest`: sorted mapping from `ProgramId` to the ordered shader-map entries/exports that form the program.
3. `CookedShaderLibrary`: unique validated bytecode records addressed by `ShaderCodeHash`, with compression/index metadata and optional preload/chunk group.
4. `ShaderProvenance`: editor/development-only mapping from hashes to source, entry, permutation, compiler, dependencies, symbols, and debug artifacts.

The existing `.sparkshader` writer can initially serialize all four record classes per program, but runtime must resolve through the manifest rather than recomputing a file path from a handwritten package string. Once this logical split is proven, a library writer may merge identical code and group files by Global, Project, or Plugin ownership without renderer changes.

The cook must sort all records deterministically, compute content hashes after validation, stage the complete file set, and publish one generation atomically. A partial map/library generation is never visible.

### 9. Use Typed Runtime Lookup and Preserve Generation Safety

Runtime startup should:

```text
open compatible library -> validate manifest/map headers -> create active generation
```

`TShaderRef<TShader>` should resolve the requested permutation through the active `GlobalShaderMap`; it should not derive a package ID from the source filename. RHI shader creation can remain lazy by code hash, while startup-critical shaders and selected programs can be preloaded. Missing required global shaders, incompatible targets, corrupt code, missing features, and parameter-signature mismatches are fatal at initial startup.

Development reload should keep the current strong behavior: materialize and validate a complete replacement generation, swap only after success, retain the previous generation on failure, and release the retired generation only when all captured submission tokens are complete. Runtime cache keys must include the active generation so an object from an old library cannot leak into the new one.

### 10. Make Changed-Shader Recompilation Dependency-Directed

Preprocessing already discovers the include closure. Persist it as virtual paths and build both directions:

```text
shader type -> source/include dependencies
changed virtual source -> affected shader types -> affected programs
```

`RecompileShaders Changed` should send the changed virtual paths to the cooker, invalidate only matching job/map entries, compile missing results, rebuild the complete published generation, and report selected/cache-hit/compiled counts. If dependency metadata is absent or its schema/toolchain version is incompatible, fall back to the full catalog and state the reason.

The editor should continue to coalesce requests, run compilation outside the renderer process, reject stale publications, and keep the active generation after any failure. Do not mutate a live shader map one entry at a time.

### 11. Add Reproducible Compile Diagnostics

Every diagnostic must identify shader type, virtual source, entry point, stage, permutation values/ID, target, backend/version, job key, and declaration location. On failure—or on explicit opt-in—the tool should write a bounded debug bundle containing:

- canonical compile request and command arguments;
- final preprocessed source with virtual `#line` paths;
- ordered defines and specialization values;
- dependency list and hashes;
- compiler stdout/stderr and structured diagnostics;
- bytecode/output hash when available;
- a single-job replay command/file usable without recooking the catalog.

Default policy should dump failures only; dumping every compile is opt-in because it creates many small files. Shader symbols and debug metadata should be configurable independently from runtime bytecode so shipping-symbol workflows do not force different runtime shader content where a backend permits separation.

### 12. Separate PSO Precaching from Shader Cooking

Define a `RenderPipelineKey` from shader code hashes plus the complete API-visible state:

- graphics: vertex input/layout, topology, rasterization, depth/stencil, blend, multisampling, render-target/depth formats, and dynamic-state policy;
- compute: compute shader hash plus layout/specialization state required by the backend;
- ray tracing: libraries/exports, hit groups, recursion/payload/attribute limits, and backend-specific state.

Typed programs may enumerate known global PSO descriptors during startup or cook. Queue those descriptors for asynchronous creation before first use and allow loading screens to wait for required/high-priority requests. Record `Hit`, `Missed`, `Untracked`, `TooLate`, `Used`, `Precached`, creation duration, and hitch threshold evidence. A shader-only hit and a complete-PSO hit should be distinguishable.

Do not put render-target or blend/raster/depth state into shader package identity. Do not claim hitch-free rendering merely because bytecode was cooked.

### 13. Keep Physical Packaging an Internal Optimization

- Preserve one logical authoring model regardless of whether release data is written as per-program files, one global library, project/plugin libraries, or chunks.
- Measure startup I/O, file-open count, compression ratio, artifact size, patch delta, cache reuse, preload latency, and generation replacement cost.
- Choose physical grouping from that evidence and supported platform packaging constraints.
- Never expose physical library or chunk membership as a repetitive render-pass constant.

## Ownership and Dependency Boundaries

| Owner | Owns | Must not own |
| --- | --- | --- |
| Core | generic paths, hashing, files, processes, tasks, diagnostics | shader source policy, renderer shader types, RHI compilation |
| RHI public contract | shader stages/formats, cooked schema, parameter metadata primitives, backend-neutral shader/pipeline descriptors, RHI handles | renderer pass catalog, project source discovery, compile orchestration |
| RHI backend | create/destroy backend shader and pipeline objects from validated data | source preprocessing, renderer program policy, package naming |
| Renderer | concrete global shader types, typed programs, parameter structs, pass traits, shader-map use, preload/PSO declarations | filesystem source resolution, compiler backend selection, editor process UI |
| ShaderCompiler tool | virtual source mounts, preprocessing/dependencies, compile requests/jobs, backend invocation, DDC, reflection validation, shader map/library cook and atomic publication | renderer scheduling, live RHI ownership, editor workflow state |
| Application/editor | file watching, user commands, async operation status, cancellation, publication/reload request | compilation algorithms, shader-map mutation, direct RHI shader construction |

The generic shader authoring primitives may remain in RHI public code if they stay renderer-agnostic. Concrete registrations and program composition stay Renderer-owned. The runtime shader map may be implemented in Renderer over generic cooked/RHI primitives; it must not cause RHI to depend on Renderer. The standalone compiler may link the registration object catalog, but no runtime module may depend on the tool.

## Preserve, Improve, Delete, and Defer

| Disposition | Item |
| --- | --- |
| Preserve | explicit source/entry/stage registration; DXIL and SPIR-V targets; DXC/Slang backend boundary; recursive preprocessing; include-closure/options hashing; reflection extraction; parameter verification on hit and miss; bounded task execution; local artifact store; out-of-process cook; transactional publication; validated generation swap; GPU-token retirement |
| Improve | physical paths into virtual paths; mutable registry into validated frozen metadata; free-form variants into typed permutations; cook nodes into deduplicated prioritized jobs; local store into a layered DDC; package registry into the actual runtime manifest; change polling into reverse-dependency selection; layout count check into a strong structural signature; diagnostics into replayable per-job bundles |
| Delete after migration | `RendererShaderPackages.h`; `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`; basename-derived package fallback; repeated pass/package/layout/pipeline strings; duplicate pass and shader parameter structures; silent duplicate suppression; count-only layout acceptance; direct runtime path construction from author package IDs |
| Defer until measured | material shaders; vertex factories; remote/distributed compilation; persistent worker-process pool; cloud DDC; complex plugin loading phases; library chunk/patch system; automatic PSO discovery for arbitrary content |

## Error Policy

No architecture can promise "error free." This design should make errors difficult to express and deterministic to detect.

The cooker or startup validation must fail on:

- invalid, unknown, overlapping, or late source mounts
- absolute authored source/include paths or virtual paths escaping their mount
- duplicate shader type IDs
- duplicate program IDs
- late shader-type registration after registry freeze
- two stages of the same kind in a non-library program
- an invalid stage set for the program kind
- invalid or unsupported permutation requests
- missing source, include, entry point, parameter metadata, or backend output
- source-path canonicalization collisions
- inconsistent reflected layouts across stages/backends or a parameter layout/signature mismatch
- compiler output that fails a shader type's `ValidateCompiledResult`
- corrupt or incompatible cache entries that cannot be regenerated
- duplicate cooked records with the same identity but different content
- a manifest entry whose artifact hash, source hash, layout hash, feature set, or declared stages do not match
- a shader-map reference whose code hash is absent from the opened library
- a pass requesting a program whose parameter contract is structurally incompatible
- a required startup shader or PSO that cannot be created for the active RHI target

Errors must name the logical program, shader type, virtual source, entry point, stage, readable permutation, target, backend/version, compile job key, and both conflicting declaration locations where relevant. Development recook errors retain the previous active generation; initial startup has no safe previous generation and fails explicitly.

## Rejected Alternatives

### Use the shader filename as `PassName`

Rejected because it couples source organization to graph semantics and fails multi-entry, multi-file, reuse, instance-label, and rename cases.

### Keep all current strings because they are explicit

Rejected because the same fact is authored in several places and can drift. Explicit source/entry/stage is valuable; repeated aliases for the same logical program are not.

### Remove cooked packages and compile everything at runtime

Rejected because it weakens deterministic releases, startup behavior, backend validation, reflection/layout validation, and failure containment.

### Treat every shader source file as an executable shader

Rejected because includes and libraries are not entry shaders, and one file can contain several entry points.

### Build one global physical package immediately

Rejected as a default because no current measurement shows that 28 files are a material problem. Logical authoring cleanup does not require a physical mega-package.

### Use compiler-generated C++ type names as serialized IDs

Rejected because they are toolchain-specific and unstable. Macro stringization or generated catalog IDs are deterministic; RTTI spellings are not.

### Copy Unreal's complete shader subsystem

Rejected because Unreal's materials, vertex factories, cook workers, distributed compilers, plugin loading phases, platform count, and content scale solve workloads Sparkle does not currently have. Sparkle should copy responsibility boundaries, invariants, cache semantics, typed authoring, and failure behavior while keeping the implementation proportional.

### Launch one compiler process per shader immediately

Rejected as a default because Sparkle already runs the cooker out of process and executes bounded stage tasks. A serializable job boundary is required now; a persistent worker pool is added only when compiler critical sections, crashes, leaks, or throughput measurements justify its process cost.

## Migration and Acceptance Gates

Implement this through ordered vertical slices. Each slice must replace an existing authority and delete its old path before the next broadens the system; no permanent compatibility subsystem is accepted.

| Slice | Implementation outcome | What it demonstrates about Unreal's design |
| --- | --- | --- |
| 0. Characterize | freeze current counts, hashes, cook outputs, cache behavior, reload behavior, and representative timings in tests/evidence | mature shader changes start with measurable contracts, not macro resemblance |
| 1. Source namespace | add `/Engine` and `/Project` mounts, virtual include resolution, collision/traversal tests, and portable diagnostics; remove silent project-first shadowing and absolute authored includes | Unreal separates virtual shader identity from physical checkout paths |
| 2. Shader types | add declaration locations, registry validation/freeze, typed empty permutation domain, and compile-policy hooks; migrate `ComputeClear` | `FShaderType` is metadata and policy for an entry point, not a filename wrapper |
| 3. Parameters and pass traits | make one `FParameters` authority drive shader verification and frame-graph dependencies; generate the pass label; remove count-only binding acceptance for the proof | RDG intentionally connects parameter metadata, resource lifetime, binding, and diagnostics without equating pass name to shader identity |
| 4. Programs | add typed compute/graphics/ray-tracing composition; migrate `ComputeClear`, `GBuffer`, and one ray-tracing library; derive program IDs/stage sets/layouts | shader types, shader pipelines/programs, and graph passes are separate typed concepts |
| 5. Jobs and DDC | split reusable compile job keys from program manifest keys; add typed permutations, dedupe, priority/cancellation, dependency persistence, replay bundles, and layered-store tests | `FShaderCompilerInput`, compile jobs, worker coordination, and DDC are distinct layers |
| 6. Map and library | generate and consume `GlobalShaderMap`, `ShaderProgramManifest`, and code-hash records; deduplicate code; preserve transactional generation publication | shader maps provide logical lookup while shader code libraries provide physical delivery |
| 7. Renderer migration | mechanically migrate remaining registrations/passes, then delete package constants, explicit-package macro, basename fallback, repeated debug strings, and duplicate parameter declarations | automation follows from one authoritative typed graph rather than from scanning files |
| 8. Incremental development | pass changed virtual paths to the cooker, select through reverse dependencies, publish complete generations, and retain failure rollback/GPU-safe retirement | changed-shader iteration updates a map generation; it does not patch unsafe live objects |
| 9. PSO precache | enumerate supported global program pipeline descriptors, compile asynchronously, and report hit/miss/too-late evidence | cooked shaders and prepared PSOs solve different hitch sources |

For every compiling slice, run the smallest relevant shader/compiler/runtime tests and both supported backend cooks. Any Renderer/RHI boundary change also runs `architecture_boundary_check`; every handoff runs `git diff --check`.

The migration is accepted only when:

- a pass author writes no package string, binding-layout debug string, pipeline debug string, or default `PassName` string
- every registered source and include has a canonical virtual path and portable diagnostic identity
- registry freeze rejects duplicate or late declarations with source locations
- typed permutation IDs round-trip, unsupported variants are never scheduled, and manifest diagnostics show readable values
- one parameter declaration drives graph dependency, reflection verification, binding, and structural layout identity
- `GBuffer` still cooks vertex and pixel stages into one logical program
- one file with several entry points and one program with stages from several files are both tested
- moving a shader source invalidates its compile key without silently renaming the render-pass label or logical program
- same-basename source paths cannot collide
- identical compile requests deduplicate even when used by different programs; presentation/package text does not alter their job key
- identical validated bytecode appears once in the cooked code library and is referenced by hash
- runtime typed lookup resolves through the generated shader map/manifest, not a derived source basename or handwritten package string
- changed includes select every dependent program and no unrelated program when dependency data is valid
- all registered programs validate and cook for the supported targets
- an invalid replacement leaves the previous runtime generation active
- profiler and GPU-capture labels remain semantic and readable
- PSO validation distinguishes shader-only readiness from full-pipeline hits, misses, and late requests

## Final Position

Sparkle should follow Unreal's semantic center end to end: virtual shader source paths; immutable C++ shader types registered with explicit source, entry point, stage, parameters, permutation domain, and compile policy; reproducible asynchronous compile jobs backed by disposable derived data; generated global shader maps and deduplicated cooked code libraries; typed runtime references with render-resource lifetime; RDG-integrated parameter metadata; and separate PSO precaching. Render-graph names remain diagnostic presentation. NVIDIA and AMD reinforce the same separation with filename-plus-entry shader loading, typed stage/pass selection, generated platform blobs, permutation keys, and independent GPU/pipeline labels.

The clean target is not "the filename is everything" and not "copy every Unreal subsystem." It is "each identity has one job, authors state only irreducible facts, tooling generates the rest, and every generated boundary is deterministic, validated, cacheable, inspectable, and safe to replace."
