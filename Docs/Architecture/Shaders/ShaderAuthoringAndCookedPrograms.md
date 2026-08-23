# Shader Authoring and Cooked Program Architecture

Status: target architecture and implementation plan; current-state findings are source-backed, while target completion requires the final evidence phase
Current audit: 2026-08-23 at `0e85e99d3afeb671bcf9d4f6b494dcf7c8a2c1d6`
Scope: shader source/import, types and permutations, compilation and validation, DDC/cooking/publication, code libraries and residency/streaming, runtime objects, graphics/compute/RT pipelines, PSO caching/prewarming, SBTs, RDG use, recooking/hot reload, debugging/analysis, and evidence

## Purpose

This document decides how SparkleEngine should identify render passes and shaders without requiring authors to maintain parallel strings such as `DirectLighting`, `RendererShaderPackages::DirectLighting`, binding-layout names, pipeline names, source basenames, and cooked-package names.

It also maps Epic's global-shader lifecycle onto Sparkle from source import through runtime pipeline creation, explains which Unreal patterns are worth adopting, traces the design to Sparkle's engineering and portfolio requirements, and defines atomic implementation phases for the migration. It refines the shader-specific conclusions from [External Renderer Repository Comparison](../ExternalReferences/ExternalRendererComparison.md). That comparison remains the source-linked broad research document. This document owns the target shader-authoring architecture, current shader-lifecycle audit, implementation sequence, clean-break boundaries, and final acceptance gates. Code, tests, executable build configuration, and captured evidence remain the authority for what is implemented and proven today.

The implementation phases apply directly to the unstaged `master` worktree. They do not authorize creating or switching branches, staging, committing, pushing, or submitting; the user owns review and every source-control action. Phases 0 through 7 are source/document checkpoints and do not configure, build, compile shaders, cook, launch, capture, or claim executable validation. Phase 8 validates the complete candidate once, after all obsolete authoring, package, parameter, cook, runtime, and frontend paths are gone.

The [Ray-Tracing Pipeline and Dual-Execution Delivery Plan](RayTracingPipelineImplementationPlan.md) owns the staged implementation, effect-level inline/pipeline selection contract, native pipeline and shader-table gates, and paired execution evidence for full ray-tracing shader support.

## Adversarial Review Verdict

The 2026-08-15 review assumed every proposed Unreal/vendor analogy was wrong until a primary source, current Sparkle code, and a local requirement established its exact scope. The result keeps the end-to-end direction but narrows several earlier overclaims:

| Challenged claim | Verdict and proof boundary |
| --- | --- |
| The shader filename can replace pass identity. | Rejected. Epic global shaders, Donut/NVRHI-style shader creation, current multi-entry/multi-stage cases, and Sparkle's `GBuffer` prove source location, entry, stage, program, and graph operation are different identities. |
| Every pass needs a handwritten `PassName`. | Rejected. The semantic event label is necessary, but its default can be generated from one typed pass declaration; only instance-specific text remains authored at scheduling. |
| Authors need named shader packages. | Rejected. Authors need typed shader/program declarations and cooked runtime artifacts; physical package/library membership is generated delivery policy. |
| One shader parameter struct must be every pass struct. | Overconstrained and corrected. Epic RDG intentionally reuses shader parameters for common one-to-one passes but also supports pass parameters without shader semantics. Sparkle selects one owner per shader-visible field plus explicit pass-envelope composition. |
| Unreal's `FShaderCompileJobKey` is the full content/DDC hash. | False and corrected. It is a narrower logical type/platform/permutation key; the complete job-input hash is separate. Sparkle names its content identity `ShaderCompileInputHash`. |
| A deduplicated compressed/streamed physical code library is always required. | Not proven for the current catalog. Exact hashes, maps, and code-record schemas are required; physical merging, compression, chunks, and streaming require measured byte/I/O value. |
| D3D12 pipeline libraries and a long-lived shader/module cache are universal baseline best practice. | Rejected as universal. Complete canonical PSO descriptors, async preparation, and telemetry are the portable baseline. Native cache and shader/module object lifetime are backend/capability-specific measured optimizations. |

“Epic-aligned” therefore means matching responsibility boundaries, invariants, failure behavior, and authoring ergonomics—not copying class names, macro volume, material-system scale, or every optional cache.

## Decision Summary

1. Sparkle should adopt Unreal's global-shader architecture as its core mental model: immutable typed shader metadata, explicit virtual source path/entry/stage, typed permutation rules, complete compile inputs, asynchronous jobs, shader maps, generated cooked code records, typed runtime references, and separate PSO caching.
2. A semantic render-pass label is necessary for frame-graph diagnostics, GPU markers, errors, captures, and profiling.
3. `static constexpr const char* PassName = "DirectLighting";` is not necessary as a separately authored field on every pass. The default label should be generated from one typed pass declaration, with an explicit per-instance label only when the same pass type is scheduled for different work.
4. A shader filename must not be the render-pass identity. It is one compile input, not the meaning of a frame-graph operation.
5. Renderer authors should register typed shaders using virtual source path, entry point, stage, parameter structure, and permutation/environment rules. They should compose those shader types into a typed shader program. They should not manually invent a cooked-package string.
6. One typed shader-visible parameter schema should be authoritative for reflection validation and binding. A one-shader pass should reuse it directly; a pass may instead compose it into a graph-only envelope when it owns render targets, copy parameters, or several shader invocations. Sparkle should remove duplicated shader-visible fields, not ban pass-only RDG metadata or shaderless passes.
7. Sparkle should keep cooked shader data but separate three concepts that the current "package" partly conflates: a logical shader map, generated code records, and program/pipeline metadata. Content hashes are required; physically merging, compressing, or streaming records is selected only when current size and I/O evidence justify it. Physical files are generated delivery details, not renderer authoring concepts.
8. The cooker should generate program membership, stage masks, layout identity, compile keys, artifact keys, and the runtime manifest. Every ambiguity or collision must be a deterministic error.
9. Shader bytecode caching and graphics/compute pipeline caching are different systems. The latter must include complete fixed-function and render-target state and should support asynchronous precaching plus hit/miss/too-late evidence.
10. Sparkle should copy Unreal's separations and invariants, not its full material, vertex-factory, distributed-build, or plugin machinery until a real Sparkle workload requires those systems.
11. Shader Tools should center `Apply Changed`, semantic search/selection, one operation state, source-located errors, and contextual next actions. Package/layout IDs, hashes, raw artifacts, full rebuild/reload, backend flags, and cache mechanics remain searchable expert details.
12. Loading, streaming, and residency are policies over generated maps/code libraries and live RHI resources. For the current small catalog, eager loading may be best; the runtime should expose preload/readiness/budgets and select policy from measurements rather than assume a complex streamer.
13. Inline ray queries and full ray-tracing pipelines are different systems. Sparkle currently has the former. RT library metadata is compiler-only until native state objects/pipelines, identifiers, SBT construction, trace commands, RDG/lifetime/cache integration, fallback selection, and paired execution evidence arrive together.

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
  | program + Parameters   |                   | diagnostics only        |
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
  | stage + Parameters     |                   +------------+------------+
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

The RHI owns generic shader descriptors, neutral cooked-record validation primitives, and backend object creation. The Renderer owns its shader types, typed program compositions, pass types, shader-map use, runtime generation, and pass-to-program use. The shader compiler consumes the renderer's immutable registration catalog and produces generated maps/libraries without taking ownership of renderer runtime policy.

## Unreal-Aligned System Model

Unreal does not have one object called a "shader package" that owns every concern. Its useful architectural pattern is a pipeline of distinct authorities. Sparkle should preserve that separation even when its implementation is much smaller.

| Unreal concept | Responsibility | Sparkle target | Current Sparkle approximation |
| --- | --- | --- | --- |
| virtual shader source paths | stable source namespace independent of checkout location | `ShaderSourceMountTable` with `/Engine`, `/Project`, and `/Plugin/<Name>` roots | project-first then engine physical path search |
| `FShaderType` / `FGlobalShaderType` | immutable shader metadata and compile hooks | `ShaderTypeDesc` emitted by typed registration | `ShaderRegistrationDesc` |
| `TShaderPermutationDomain` | typed, bounded permutation dimensions and stable IDs | `TShaderPermutationDomain<...>` | free-form compile defines and package variants |
| `FShaderCompilerInput` | complete read-only input for one compilation | `ShaderCompileRequest` | `ShaderCompileOptions` plus cook-node metadata |
| `FShaderCompileJob` / `FShaderCompileJobKey` / input hash | scheduled unit and logical type/platform/permutation key, plus a separate hash over all job inputs for the job cache | `ShaderCompileJob` plus `ShaderCompileInputHash`; a small logical job ID may exist only for scheduling | `CookNode` plus `TaskExecutor` task |
| `FShaderCompilingManager` and Shader Compile Workers | asynchronous coordination and compiler-process isolation | compile-job coordination on the existing cooker `TaskExecutor`; optional worker processes only when justified | one out-of-process cooker with bounded in-process stage tasks |
| shader job cache / DDC | disposable content-addressed derived compilation results | cooker-owned `ShaderDerivedDataCache` with one local implementation | local per-stage artifact store |
| Global Shader Map / `TShaderMapRef<T>` | typed permutation lookup and shader lifetime | `GlobalShaderMap` / `TShaderRef<T>` | pass runtime cache loading by package ID |
| `FShaderMapResourceCode` | code hashes and map resource content | generated map resource record | code embedded in each `.sparkshader` program package |
| `FShaderCodeLibrary` | cook-time collection of unique code and runtime loading by hash | `CookedShaderLibrary` | no cross-package code library |
| `FShaderPipelineType` | a declared set of shader stages | `TShaderProgram<Stages...>` | stages grouped by repeated package string |
| RDG shader/pass parameter structs | shader parameters can directly serve a one-to-one pass; pass envelopes and shaderless pass parameters are also valid | one shader-visible `Parameters` schema, reused or composed into a pass envelope without duplicating shader fields | separate pass parameters and registered shader parameters |
| RDG event name | diagnostic/profiler identity of one graph operation | generated default `RenderPassLabel`, optional instance override | repeated `PassName` literal |
| PSO precache / shader pipeline cache | full pipeline-state preparation and hitch tracking | extend existing `RenderPassRuntimeCache` generation ownership with explicit preparation and readiness evidence | pipeline creation coupled to pass runtime creation |

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
        +--> CookedShaderCode: exact code hash -> indexed bytecode record
        +--> provenance / layout signatures / feature requirements
                         |
                         v
              deterministic transactional publication

RUNTIME

  open library -> load shader map -> TShaderRef<TShader>
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
- code records provide exact hashed physical delivery; a library may merge duplicate blobs when measured useful;
- pipeline descriptors add fixed-function state;
- render-pass labels provide diagnostics only.

This is why filenames cannot safely replace shader types, programs, passes, or PSOs. Automation should derive everything that is derivable, while authors still state facts the tool cannot infer: entry point, stage, supported permutations, feature requirements, and semantic composition.

## Proposed Authoring Experience

The exact API spelling is an implementation choice. The intended amount of authored information is illustrated below:

```cpp
class DirectLightingCS final : public TGlobalShader<DirectLightingCS>
{
public:
    using Parameters = DirectLightingParameters;
    using PermutationDomain = TShaderPermutationDomain<UseRayQuery>;

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
    "/Engine/Passes/Lighting/Direct/DirectLighting.hlsl",
    "main",
    Compute);

using DirectLightingProgram = TShaderProgram<DirectLightingCS>;

SPARKLE_RENDER_PASS(DirectLightingPass, DirectLightingProgram, DirectLightingParameters);
```

The final form should have these properties:

- `SPARKLE_RENDER_PASS` or equivalent registers one pass type and generates its default diagnostic label from the type token. There is no hand-written `"DirectLighting"` beside `DirectLightingPass`.
- `TShaderProgram<...>` declares the complete stage set. A compute program has one compute shader; a raster program can have vertex and pixel shader types; a ray-tracing program owns its library exports and hit groups.
- The shader registration still states virtual source path, entry point, and stage because those are independent compile inputs.
- The shader type owns compile eligibility, environment mutation, compiled-output validation, precache policy, and a typed permutation domain. The empty permutation domain remains the zero-cost default.
- `DirectLightingParameters` is the one shader-visible schema. This one-to-one pass can use it directly for graph dependencies, binding, reflection verification, and its persisted layout signature. More complex passes may compose the schema into a pass-only RDG envelope without repeating those shader fields.
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

### Intent-First Shader Frontend Contract

The normal shader workflow is one `Shader Tools` surface plus the existing console/shortcut route, not a panel for every compiler, cache, package, pipeline, or artifact layer. The user supplies intent; the system resolves dependency closure, supported targets, cache policy, job scheduling, validation, publication, reload, and lifetime safety.

| User intent | Primary frontend action/result | Automatically derived and validated | Contextual expert access |
| --- | --- | --- | --- |
| Apply saved source edits | `Apply Changed` with one shortcut and nonblocking status | changed virtual paths, reverse dependencies, affected shaders/programs, active development targets, cache hits, bounded jobs, complete replacement generation, safe activation | selected/compiled/cache-hit counts; exact job list under Details |
| Understand a compile failure | source-located root cause, affected shader/program, `Open Source`, `Retry`, and reassurance that the previous generation remains active | duplicate diagnostics collapsed by root cause, dependent jobs classified as skipped, portable virtual paths, failure bundle creation | compiler output, command, preprocessed source, dependency hashes, reflection/layout comparison, one-job replay |
| Inspect what the running frame uses | select/search a semantic pass, program, or shader and see `Ready`, `Stale`, `Compiling`, `Failed`, or `Unsupported` | join from pass/program to active generation, code record, PSO readiness, source, symbols, and captured marker identity | permutation, target/backend, hashes, reflection, disassembly, manifest and pipeline cache provenance |
| Validate a development or release build | `Validate Shaders` through the normal build/cook preset | complete registered catalog, compile-policy filtering, D3D12/Vulkan targets required by that preset, deterministic publication and conformance checks | support matrix, excluded permutations with reasons, cold/warm/cache statistics |
| Investigate a shader or PSO hitch | `Open Shader` from the selected Performance/GPU marker or pipeline event | preserve frame, marker, program, code/pipeline hashes, backend, generation, and symbol package | PIX/RenderDoc/Nsight/RGP guidance and copied replay/capture identity |
| Reproduce one expert failure | `Replay Failed Job` from Diagnostics details | exact compiler/tool version, virtual source closure, target, environment, and bounded output location | editable command copy and machine-readable request |

The common Editor layout is deliberately small:

```text
+ Shader Tools ---------------------------------------------------------------+
| 3 changed files ready       [Apply Changed]                    [Search ...] |
| Status: Ready | active generation 42 | last apply 18 shaders, 16 cache hits |
+ Programs / shaders ----------------------+ Selection -----------------------+
| DirectLighting        Ready              | DirectLighting / Compute          |
| GBuffer               Ready              | source + entry                    |
| Exposure              Changed            | used by 1 pass                    |
| ...                                        | [Open Source] [Apply Selection]  |
+ Diagnostics ----------------------------------------------------------------+
| No active errors. [Show last operation] [Advanced...]                       |
+------------------------------------------------------------------------------+
```

The primary list should normally expose semantic program/shader name, stage where useful, source, active status, and pass consumers. Package ID, binding-layout ID, generation number, backend/profile, artifact directory, code/input hashes, compiler command, raw reflection, and disassembly are contextual details—not ten default columns. `Reload Cooked`, `Recook All`, package-targeted recook, compiler backend/target listing, cache paths, and worker controls do not belong beside the common `Apply Changed` action. Validated publication activates automatically; `Rebuild All` and manual reload remain searchable expert recovery actions with an explanation of cost and risk.

```text
Idle -> ChangesReady -> Compiling -> Validating -> Active
                         |              |
                         +-> Failed <---+
                               |
                     previous Active generation remains
```

- One operation card owns progress, cancellation, queued/coalesced follow-up state, result, elapsed time, and the active-generation guarantee. Do not emit a dialog or toast per shader job.
- Source watching may offer coalesced `Auto Apply Changed` as a development preference after debounce/cost measurement. The default interaction must always reveal what will compile; a widely included source or release-target expansion shows the estimated affected scope before expensive work starts.
- The frontend chooses the named development/release preset and capabilities. An advanced override shows its difference from that preset, validates it before launch, persists only at the declared project/platform scope, and offers `Reset To Preset`.
- A successful apply needs a short result, not raw cooker stdout. A failure leads with one actionable source diagnostic. All raw artifacts remain available without dominating the workflow.
- Opening Shader Tools from a GPU marker, error, or pipeline event preserves that selection. The user never retypes a package, shader ID, hash, source path, or backend to continue the investigation.
- The panel reads one immutable shader-operation/catalog model and submits typed requests through `EditorOperationService`; it never scans artifact directories, invokes the compiler, mutates the active shader map, or creates RHI objects itself.

Epic's current shader-development workflow similarly centers a saved edit plus `recompileshaders changed`, asynchronous compilation, direct source diagnostics, retry, and DDC invalidation from full compile inputs. Sparkle adopts the intent and safe iteration loop while generating more of the target/dependency/publication detail and retaining the previous valid generation. [Epic Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)

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

The target model uses four identities with different scopes. The names deliberately distinguish Unreal's logical `FShaderCompileJobKey` from its full input hash:

1. `ShaderTypeId`: a stable ID emitted from the typed shader declaration. Together with permutation and target it addresses a shader-map entry.
2. `ShaderCompileInputHash`: a content hash of every input that can change one stage compile, including virtual source path, transitive source contents, entry point, stage, permutation, environment, target, compiler backend, and compiler version. Parameter metadata enters this hash only when it changes generated declarations, resource-table/environment input, or compiler output; a validation-only signature belongs in the result/manifest contract instead. Program/package display identity is excluded unless it changes compilation.
3. `ShaderCodeHash`: a hash of the exact validated backend bytecode. Stage, entry, reflection, feature, layout, and provenance metadata remain adjacent current-contract map/record fields and are covered by artifact integrity; they are not silently conflated with raw code identity. This preserves exact byte deduplication and symbol correlation without treating equal bytes with incompatible metadata as interchangeable records.
4. `ShaderProgramId`: a canonical declared program identifier emitted by the generated catalog and collision-checked in the manifest. Typed composition determines and validates its ordered stages, but compiler RTTI and refactor-sensitive type spellings are never its persisted representation. An owned rename updates every producer/consumer and regenerates the disposable map/library rather than introducing schema migration.

This allows source edits to invalidate compilation, source moves to be diagnosed correctly, identical compile jobs and identical output code to be deduplicated at their proper layers, and logical program lookup to remain independent of a file basename.

The current one-`.sparkshader`-per-program layout supplies migration inputs only; it is not a target checkpoint or compatibility container. Phase 5 atomically replaces it with independently modeled map records, program records, and code records, deletes the old readers/writers, and leaves physical grouping as a generated policy. The initial library may use one or several generated files, but that choice must not expose authored package identity. Later regrouping requires current startup I/O, file-open, compression, patching, and preload evidence and regenerates the one current representation in place.

```text
logical identity                         physical delivery
================                         =================

ShaderTypeId + Permutation + Target ---> GlobalShaderMap entry -----+
                                                                  |
ShaderProgramId -----------------------> ordered stage references  |
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

Epic's `FShaderCompilerInput` gathers the read-only inputs for one compile, including the virtual source path, entry point, target, platform/format, environment, parameter metadata, debug information, and its job-cache hash. `FShaderCompileJob` pairs input, a logical job key, preprocess output, compiler output, and diagnostics; the logical `FShaderCompileJobKey` is not the full content hash. `FShaderCompilingManager` coordinates priorities, pending jobs, result application, cancellation, worker processes, and optional distributed execution. Epic explains that Shader Compile Workers provide process-level parallelism around compiler implementations that may otherwise serialize internally.

Unreal stores disposable compile outputs in its Derived Data Cache, whose hierarchy can include local, shared, and packaged read-only layers. Cooking then collects unique shader code into shader libraries. At runtime `FShaderCodeLibrary` can open project/plugin libraries, test for code by hash, and preload or release code. `FShaderMapResource` separately owns the render-resource lifetime and can create/preload individual RHI shaders. This is the reason Sparkle should separate derived compilation cache, logical shader map, cooked code library, and live RHI objects.

Unreal's Render Dependency Graph intentionally lets a shader parameter structure also describe the common one-to-one pass, while `FRDGBuilder::AddPass` still receives a separate event name for debugging and profiling. Epic also documents pass parameter structures without shader semantics, such as copy passes. Sparkle should therefore reuse or compose one authoritative shader-visible schema, not force every graph pass into one shader struct. Epic's parameter metadata exposes both a layout hash and a strong persistable layout signature; that is stronger than Sparkle's current independent Direct Lighting declarations plus count-only runtime fallback.

Finally, Unreal's PSO precaching operates above shader compilation. It assembles full graphics/compute pipeline descriptions, compiles asynchronously, and records hits, misses, untracked uses, and requests that completed too late. Shader bytecode availability alone is not proof that the driver-level pipeline was prepared without a hitch.

Adopt:

- typed shader registration
- explicit virtual source path, entry point, and stage
- typed permutation domains and compile-policy hooks
- complete immutable compile inputs, deterministic input hashes and logical job IDs, in-flight deduplication, priorities, and cancellation
- typed shader-map lookup and independently owned RHI shader lifetime
- compile-input hashes derived from the full compiler-affecting closure and compiler provenance
- a disposable derived-data cache outside runtime packages
- cook-time collection and deduplication of global shader code
- one owner for each shader-visible parameter contract with a persistable structural signature, reused when a pass composes those contracts into its own envelope
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
- [Epic: `FShaderCommonCompileJob` and full `InputHash`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCommonCompileJob)
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

### Compiler, Capture, and Native Pipeline Precedent

Epic's development workflow treats a reproducible compile as a product feature: a debug dump contains the contributing sources and includes, preprocessed source, and a command file that replays the compiler invocation. Epic's cooked-shader debugging workflow can generate symbols separately from runtime shader data, so making a shader inspectable does not require permanently bloating the shipped bytecode.

The external GPU tools reinforce that provenance requirement. PIX resolves separate DXIL debug data by a compiler-suggested hash name and can show source and compile arguments from slim PDBs. NVIDIA Nsight Graphics needs source-level debug information and line mappings to correlate DXIL or SPIR-V hotspots and crashes back to HLSL. AMD Radeon GPU Analyzer inspects target ISA, register pressure, LDS, and scratch use. Sparkle therefore needs a bytecode-hash-to-source-symbol record and an opt-in analysis build, not only a text disassembly produced during a successful cook.

The native APIs also reinforce the separation between shaders and pipelines. Microsoft recommends creating D3D12 pipeline state objects outside render-time use and provides cached blobs and pipeline libraries. Vulkan pipeline caches can preserve expensive pipeline-creation work between runs. Epic's PSO precache adds the renderer-level policy Sparkle needs on top: enumerate complete descriptors, compile asynchronously, and classify runtime uses as hit, missed, untracked, or too late. A compiled shader package is not a prepared pipeline.

Adopt:

- write a self-contained replay bundle for failed as well as successful compile jobs
- store source mappings and separate debug symbols by shader/code hash so PIX, RenderDoc, and Nsight can resolve the exact cooked binary
- keep symbols and analysis products outside the lean runtime artifact unless an explicit development mode embeds them
- make representative DXIL and SPIR-V inspection first-class verification outputs
- feed exact shipped compile requests to RGA, Nsight, PIX, or equivalent analysis rather than recompiling an approximate shader by hand
- create complete D3D12/Vulkan pipeline keys, persistent native caches where supported, asynchronous precache requests, and hitch telemetry

Sources:

- [Epic: Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [Epic: Shader Debugging Workflows](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-debugging-workflows-unreal-engine)
- [PIX: automatic shader PDB resolution](https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/)
- [RenderDoc: shader debug-symbol handling](https://github.com/baldurk/renderdoc/releases)
- [NVIDIA: using shader debug information with Nsight Graphics](https://developer.nvidia.com/blog/harness-powerful-shader-insights-using-shader-debug-info-with-nvidia-nsight-graphics/)
- [NVIDIA: Nsight Graphics shader compilation guidance](https://developer.nvidia.com/docs/drive/drive-os/7.0.3/public/nsight/nsight-graphics/UserGuide/index.html)
- [AMD: Radeon GPU Analyzer](https://gpuopen.com/rga/)
- [Microsoft: managing D3D12 pipeline state](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)
- [Microsoft: D3D12 pipeline-state cache sample](https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-pipeline-state-cache-sample-win32/)
- [Khronos: Vulkan pipeline cache](https://docs.vulkan.org/guide/latest/pipeline_cache.html)

## Current Sparkle Findings

The 2026-08-23 source review at `0e85e99d3afeb671bcf9d4f6b494dcf7c8a2c1d6` traced this production path:

```text
Authoring                     Host tooling                              Runtime and GPU
=========                     ============                              ===============

HLSL / HLSLI ----------+      ShaderCompiler CLI                        FrameGraphBuilder::Draw/Dispatch
                       |              |                                             |
C++ shader type -------+----> static GlobalShaderRegistry                           |
and FParameters        |              |                                             |
                       |              v                                             |
C++ pass Parameters ---+----> contract catalog ----> cook plan                      |
       |                              |                 |                            |
       |                              |                 v                            |
       +--> RDG resource use          |          bounded SparkleTasks jobs          |
                                      |                 |                            |
                         physical source/include closure                             |
                                                        |                            |
                                              +---------+---------+                  |
                                              |                   |                  |
                                             DXC                Slang                |
                                              |                   |                  |
                                              +--> bytecode + reflection             |
                                                        |                            |
                                              local stage artifact cache             |
                                                        |                            |
                                              transactional .sparkshader             |
                                              + registry + recook signal             |
                                                        |                            |
                                                        +--------------------------->+
                                                                                     |
                                                                     package load + ABI validation
                                                                                     |
                                                                     binding-layout creation
                                                                                     |
                                                                     lazy pipeline creation
                                                                                     |
                                                                     pass Execute: bind + draw/dispatch
```

The separation at the right is important: graph construction may materialize a pass runtime, while graph execution does not load a package or create a pipeline. The following findings classify what that path actually provides today:

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
- Cancellation is checked before a cook-node task begins, but an executing compiler session or child operation is not cooperatively cancelled. Jobs have no priority, no in-flight duplicate fan-out, no time-sliced result integration, and no measured worker-memory ceiling.
- The local artifact-store behavior and atomic cache publication are useful foundations for one cooker-owned `ShaderDerivedDataCache`. The implementation phase should replace the generic store vocabulary with that concrete authority rather than retain a second cache abstraction. Runtime cooked data remains separate from the disposable cache.
- Include-closure keys serialize normalized physical paths, and compile-option keys include physical include directories. Cache identity is therefore tied to a checkout/machine layout even when source bytes and virtual meaning are identical.
- Corrupt or incompatible local cache data fails the cook instead of being quarantined and treated as a cache miss. The cached payload has no independent integrity record comparable to the runtime package's bytecode hash validation.
- Changed-source monitoring detects that some shader file changed, then invokes a full catalog `cook`. Include-aware stage cache hits avoid recompiling unchanged jobs, but the planner does not yet select only shader types and programs affected through reverse dependencies.
- The cooker publishes selected `.sparkshader` files, a registry, and a recook signal as one transactional file set. The registry is not currently the runtime lookup authority; runtime computes a path directly from the manually repeated package ID.
- Bytecode is embedded per program package and `ShaderCacheKey` contains package identity, so Sparkle has neither compile-job deduplication across program consumers nor a cooked unique-code library addressed by code hash.
- DXC can emit disassembly, preprocessed source, compiler arguments, diagnostics, and separate debug data for a successful opt-in analysis cook. Slang's current analysis output is narrower and does not emit equivalent disassembly.
- The DXC backend applies requested debug information, optimization, warnings-as-errors, and debug stripping. The Slang backend currently does not apply equivalent cook-policy switches, and its stage mapping covers vertex, pixel/fragment, and compute rather than the full stage enum. Slang is therefore an architectural backend seam, not a proved release-equivalent compiler path.
- Debug-artifact publication begins only after compilation succeeds. A syntax error or backend failure throws before `ShaderDebugArtifactWriter` receives an artifact set, so the failure most in need of reproduction has no complete replay bundle, dependency manifest, or one-job command.
- `CookedShaderStats.csv` reports package/stage/backend/target/entry, bytecode bytes, reflection counts, and layout-record counts. It does not report queue/wall/CPU time, peak memory, cache aggregates, source/code hash correlation, native pipeline creation, or runtime hitch percentiles.
- `inspect-shader` inspects registration/catalog metadata; `inspect-package` inspects the cooked container, reflection, layouts, and ray-tracing records. There is no single command that follows a typed shader from declaration through compile key, code hash, program manifest, runtime package, pipeline, and captured GPU event.
- `RenderPassRuntimeCache` already builds and validates a complete replacement generation, atomically activates it, and retires the old generation only after recorded RHI submission tokens complete. This is a strong lifetime pattern and must be preserved.
- Runtime package validation is materially stronger than the authoring layer: it verifies schema/version, source and layout identities, bounds, complete logical binding records, required stages, runtime backend format, bytecode hashes, reflection, feature flags, and ray-tracing metadata before use.
- The editor already launches the shader cooker out of process, coalesces one follow-up request, rejects stale publications, and leaves the active generation unchanged after cook or runtime-validation failure.
- The current `Shader Tools` window presents Refresh, Reload Cooked, Recook All, and Recook Selected as equal toolbar actions above a ten-column table. Package ID, binding layout, parameter count, backend/target text, generation, and artifact availability are visible before the user has asked an expert question. This is an implementation-oriented inventory, not the selected intent-first frontend.
- The current selection area opens raw Source, Reflection, Disassembly, Param Match, and Compile Request artifacts and discovers their directory from shader/package identity. These are valuable expert details, but the panel lacks one operation/status model, source-located failure summary, pass/program usage, readiness, and a guided next action. Raw artifacts must move behind contextual Diagnostics/Advanced disclosure rather than be removed.
- Current source tracking automatically schedules a changed recook, while the toolbar and console also expose manual global/package/shader actions. The target must converge these into one coalesced `Apply Changed` workflow with one visible state; full rebuild, manual reload, and identity-targeted recovery remain expert actions rather than parallel normal paths.
- A pass runtime is materialized lazily from `FrameGraphBuilder::Draw`, `Dispatch`, or `DispatchAsync`. Its package, binding layout, and pipeline are created during graph construction, not command recording. This preserves the frame-graph Execute boundary but can place first-use pipeline creation on a frame-critical construction path.
- Renderer frame orchestration now uses `BuildRenderFrameGraph`, explicit pass parameter records, and the infrastructure-only `PassCommandContext`. Shader work must preserve that boundary: graph setup may resolve a prepared immutable shader/pipeline runtime, while pass recording may only bind declared resources and record commands.
- `RenderFrameIdentity` captures the active shader generation from `RenderPassRuntimeCache`; `RenderViewState` invalidates history from the generation transition. The shader migration must keep one renderer-owned generation source and must not publish package/cache/compiler mechanics through GameFramework, `RenderFrameSubmission`, `RenderView`, or `PreparedRenderScene`.
- `FrameGraph::HasBeenProduced` is now the authority for whether a declared graph resource has prior contents. Shader and pipeline work must not reintroduce one-field history-validity records, mirrored production flags, or broad frame/pass contexts.
- Renderer C++ passes now live under semantic `Passes/GBuffer`, `Passes/Lighting/<Direct|Reference|Restir|Shadows|Sky>`, `Passes/PostProcessing`, `Passes/Presentation`, `Passes/RayTracing`, and `Passes/Debug` owners. Shader sources still collect 18 files under `Engine/Assets/Shaders/Passes/Deferred`; the source-namespace phase must move those files to matching semantic owners and delete the broad directory rather than treating `Deferred` as renderer-wide architecture.
- D3D12 creates every graphics/compute pipeline with an empty `CachedPSO`; Vulkan calls `vkCreateGraphicsPipelines` and `vkCreateComputePipelines` with `VK_NULL_HANDLE` for the pipeline cache. Vulkan computes a local cache-key-shaped struct and then discards it. There is no renderer pipeline cache, native persistent cache, asynchronous precache coordinator, or hit/miss/too-late telemetry.
- Pass labels, D3D12 PIX events, Vulkan object names, binding-layout names, and pipeline names are readable. They do not carry a stable program/code hash that can join a capture event to the cooker artifacts and external shader symbols.
- Ray-tracing library registration, cooking, inspection, and metadata validation exist in the generic schema/tooling, but there are no renderer `IMPLEMENT_RAY_TRACING_SHADER` registrations. Runtime validation explicitly rejects a ray-tracing library because RT state-object/pipeline execution is not enabled. Current ray-query lighting passes are compute shaders and must not be presented as proof of a ray-tracing shader pipeline.
- `DirectShadowSignalNoRayQuery` and `DirectShadowSignalDeviceAddress` are registered and have pass implementations, but the production frame path always dispatches `DirectShadowSignal`. `CanUseInlineRayQueryShadows` has no selection consumer, and top-level provider selection falls back to classic descriptor access rather than choosing the device-address shader. These alternatives are not verified fallbacks.
- Geometry, hull, and domain stages appear in shader enums, package validation, and Vulkan shader-module mapping, but current graphics pipeline descriptions/backends wire only vertex plus optional pixel stages. Mesh/task stages are absent and both RHI capability reports mark them unsupported. Schema awareness is not executable stage support.
- Runtime capability checks directly reject missing acceleration-structure and inline-ray-query support, but do not explicitly check every declared package feature such as acceleration-structure device-address access and descriptor indexing in the same capability gate.
- CLI validation structurally validates the catalog but cooks and inspects only the representative `ComputeClear` artifact. `ShaderCompilerCliValidation` is a custom build target rather than a registered CTest, and no shader-specific unit/integration test suite was found.
- The existing build already discovers and links the shader-registration object library into compiler/runtime consumers, so authoring automation does not require scanning all shader source files.
- 28 generated `.sparkshader` artifacts were present in the reviewed development artifacts, totaling 1,994,681 bytes. This is not evidence that 28 physical files are a performance problem.

Relevant implementation entry points:

- [`GlobalShader.h`](../../../Engine/RHI/Public/Shaders/Authoring/GlobalShader.h)
- [`ShaderAuthoring.cpp`](../../../Engine/RHI/Private/Shaders/ShaderAuthoring.cpp)
- [`RendererShaderPackages.h`](../../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h)
- [`DirectLightingPass.h`](../../../Engine/Renderer/Private/Passes/Lighting/Direct/DirectLightingPass.h)
- [`DirectLightingPass.cpp`](../../../Engine/Renderer/Private/Passes/Lighting/Direct/DirectLightingPass.cpp)
- [`DirectLightingShaders.cpp`](../../../Engine/Renderer/ShaderRegistrations/DirectLightingShaders.cpp)
- [`FrameGraphBuilder.h`](../../../Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h)
- [`ShaderContractCatalogBuilder.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractCatalogBuilder.cpp)
- [`ShaderContractValidator.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractValidator.cpp)
- [`ShaderCookPlanExecutor.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderCookPlanExecutor.cpp)
- [`ShaderCookNodeExecutor.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderCookNodeExecutor.cpp)
- [`ShaderDebugArtifactWriter.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderDebugArtifactWriter.cpp)
- [`IncludeClosureHasher.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/Cache/IncludeClosureHasher.cpp)
- [`ShaderCompileOptionsHasher.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/Cache/ShaderCompileOptionsHasher.cpp)
- [`CookedShaderPackage.h`](../../../Engine/RHI/Public/Shaders/CookedShaderPackage.h)
- [`CookedPackageWriter.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/CookedPackageWriter.cpp)
- [`CookedShaderPackageCache.cpp`](../../../Engine/RHI/Private/Shaders/CookedShaderPackageCache.cpp)
- [`CookedShaderPackageValidation.cpp`](../../../Engine/RHI/Private/Shaders/CookedShaderPackageValidation.cpp)
- [`PassBinder.cpp`](../../../Engine/Renderer/Private/Pipeline/PassBinder.cpp)
- [`RenderPassShaderRuntime.h`](../../../Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h)
- [`PipelineRuntimeLibrary.cpp`](../../../Engine/Renderer/Private/PipelineRuntime/PipelineRuntimeLibrary.cpp)
- [`RenderPassRuntimeCache.cpp`](../../../Engine/Renderer/Private/Pipeline/RenderPassRuntimeCache.cpp)
- [`D3D12Pipeline.cpp`](../../../Engine/RHI/Private/D3D12/Pipeline/D3D12Pipeline.cpp)
- [`VulkanPipeline.cpp`](../../../Engine/RHI/Private/Vulkan/Pipeline/VulkanPipeline.cpp)
- [`ShaderSourceChangeTracker.cpp`](../../../Engine/Application/Private/ShaderRecook/ShaderSourceChangeTracker.cpp)
- [`ShaderRecookCoordinator.cpp`](../../../Engine/Application/Private/ShaderRecook/ShaderRecookCoordinator.cpp)
- [`ShaderCompilerProcess.cpp`](../../../Engine/Application/Private/ShaderRecook/ShaderCompilerProcess.cpp)
- [`ShaderCompiler` build definition](../../../Tools/Shaders/ShaderCompiler/CMakeLists.txt)
- [`ValidateShaderCompilerCli.cmake`](../../../Tools/Shaders/ShaderCompiler/ValidateShaderCompilerCli.cmake)

## End-to-End Shader Pipeline Atlas

This section is the complete learning map requested for shader work. It deliberately includes stages that Sparkle does not implement yet so that source compilation, runtime loading, pipeline compilation, and GPU execution cannot be collapsed into one vague activity called "loading a shader."

Status language in the tables is normative:

- **Current** is proved by the reviewed Sparkle source.
- **Recommended** is the selected target design.
- **Optional** is a valid alternative that is justified only by a measured workload or platform need.
- **Deferred** is intentionally outside the current migration, not silently implemented.
- **Unsupported** means a declaration or API shape must not be advertised as an executable feature.

```text
AUTHORING                    COMPILATION AND COOK                 RUNTIME AND EXECUTION
=========                    ====================                 =====================

source mounts                selection / permutations            open manifest + code index
    |                             |                                      |
source + includes ----------> preprocess + dependency graph              |
    |                             |                                      |
shader type + entry --------> immutable request + scheduled job          |
    |                             |                                      |
parameter ABI --------------> front end -> IR -> optimize/codegen        |
    |                             |                                      |
typed program ----------------> reflection + ABI validation              |
    |                             |                                      |
RDG pass                         DDC lookup/publication                    |
                                  |                                      |
                                  +-> shader map + code records ----------+
                                  +-> symbols/provenance                  |
                                  +-> program/PSO manifests               v
                                                                  make required code ready
                                                                          |
                                                               backend-specific native input
                                                                          |
                                                             logical PSO key/cache lookup
                                                                          |
                                                          native PSO compile/link/prewarm
                                                                          |
                                               +--------------------------+--------------------+
                                               |                          |                    |
                                         graphics draw              compute dispatch      RT trace rays
                                                                                              |
                                                                       RT pipeline + identifiers + SBT

OPERATIONS ACROSS EVERY STAGE

diagnostics -> replay -> inspection/disassembly -> captures/counters -> hot reload
            -> generation swap -> GPU-safe retirement -> regression evidence
```

### Authoring and Program Definition

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 1. Platform and capability contract | lowest common feature set; per-platform shader platforms; runtime feature branching | Define explicit shader-platform capabilities and compile eligibility; reject unsupported combinations before jobs are scheduled. | More platform records, but failures become deterministic and permutations stay bounded. | DXIL/SM and SPIR-V targets plus package feature flags exist; some feature validation and consumption branches are incomplete. |
| 2. Source namespace/import | physical paths; basename lookup; virtual mounted paths | Unreal-style `/Engine`, `/Project`, and `/Plugin/<Name>` virtual paths mapped to physical roots by tooling. | A mount table and collision rules are required; checkout-independent identity and portable diagnostics are gained. | Physical project-first/engine-second lookup, absolute includes, and basename package fallback. |
| 3. Language and modules | HLSL only; Slang only; HLSL plus opt-in Slang; generated kernels | Keep HLSL/DXC as the production reference path. Keep Slang as an explicit backend/language experiment until policy and output parity are proved. Permit generated source only through one provenance-recorded generator with source maps and deterministic regeneration. | One reference path is less flexible; two unqualified paths multiply conformance work. | `.hlsl` auto-selects DXC; `.slang` auto-selects Slang. Explicit backend selection can override that convention. |
| 4. Includes and reusable code | textual includes; language modules; generated amalgamation | Preserve small textual includes now, record the complete dependency graph, and evaluate Slang modules only where generics/composition remove measured duplication. | Textual preprocessing is simple but recompiles shared text; modules improve composition at toolchain and cache-complexity cost. | Recursive includes, `#pragma once`, recursion detection, and line directives exist. |
| 5. Executable shader type | infer every file; string registration; generated catalog; typed C++ registration | Typed immutable shader types with explicit virtual source, entry point, stage, parameter metadata, permutation domain, compile hooks, and declaration location. | Authors still state irreducible facts; invalid files and entry points are not guessed. | Mutable `GlobalShaderRegistry` entries with explicit source/entry/stage and repeated package string. |
| 6. Parameter and GPU ABI | hand-authored root signatures/layouts; reflection-only binding; typed parameter metadata | One typed shader-visible declaration drives reflection validation, backend layouts, and binding; reuse it directly or compose it into an RDG pass envelope with graph-only fields. | Code generation/metadata is needed; composition must preserve one owner for every shader-visible field. | Shader `FParameters` and pass parameters are separate; runtime has strong record validation but `PassBinder` retains a count-only compatibility path. |
| 7. Variants | preprocessor permutations; specialization constants; dynamic branches; separate source files | Typed bounded permutations for code/ABI-changing choices, specialization constants only where backend behavior and PSO identity are tested, and dynamic branches for cheap/high-coherence runtime choices. | Static variants improve specialization but multiply compile/code/PSO cost; dynamic branches reduce variants but may waste GPU work. | Free-form defines and specialization records exist; no typed permutation domain or evidence-based variant budget. |
| 8. Program composition | filename equals program; manual package grouping; typed stage composition | Typed compute, graphics, and ray-tracing programs generated from registered shader types. A program owns legal stage composition, not physical package membership. | A separate program concept adds one logical layer while deleting many repeated strings. | Package ID groups stages; `GBuffer` is the only current VS+PS program. |
| 9. Pass/RDG declaration | pass strings select bytecode; pass owns a program reference; implicit global lookup | A pass references typed program(s); its shader-visible schemas are reused/composed into one pass envelope, its default event label is generated, and an instance label is optional. Shaderless copy/clear passes remain legal. | Typed dependencies require registration/catalog generation; multi-shader/pass-only composition must stay explicit. | Pass name, package ID, binding-layout name, and pipeline name are repeated. |

### Compilation, Validation, Cooking, and Publication

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 10. Selection | compile all; package/shader selection; changed dependency closure; on-demand runtime compile | Cook all for release; use type/program selection for tools; use reverse-dependency selection for development. Do not compile at shipping runtime. | Dependency metadata must be durable, integrity-checked, and regenerated with the current tool contract; development iteration becomes proportional to the edit. | CLI supports all, package, or shader ID. Editor `Changed` and `Global` both launch an unfiltered `cook`. |
| 11. Eligibility/permutation enumeration | enumerate everything; filter before scheduling; compile on first request | Run `ShouldCompilePermutation` and platform/feature filters before producing jobs; separately declare the precache subset. | Aggressive filtering saves work but needs tests proving every runtime request was cooked. | Target capability skips exist; typed eligibility and precache policy do not. |
| 12. Preprocessing/dependencies | compiler-owned preprocessing; engine-owned preprocessing; both | Use one canonical Sparkle preprocessing/dependency pass for identity and diagnostics, then invoke the backend with controlled input. Validate that backend include behavior cannot introduce hidden inputs. | Engine preprocessing gives portability and replayability but must track compiler semantics accurately. | Sparkle preprocesses before both DXC and Slang and hashes the include closure. |
| 13. Compile request/input hash | path/timestamp key; package-scoped key; full content-addressed input hash | Immutable request plus `ShaderCompileInputHash` over virtual source closure, entry/stage, permutation/environment, compiler-affecting parameter metadata, target, backend, and compiler version. Exclude validation-only and program/package presentation identity. | Larger hash construction cost; safe cross-consumer deduplication and deterministic invalidation. | Versioned cache key includes physical paths, package ID, and binding-layout identity. |
| 14. Scheduling/isolation | serial; in-process task parallelism; persistent worker processes; distributed farm | Keep bounded `SparkleTasks` jobs now. Add priority, in-flight dedupe, cancellation, and memory budgets. Add worker processes only when compiler isolation or scale is measured. | Processes isolate crashes/leaks and bypass compiler locks but add IPC, startup, deployment, and debugging cost. | One out-of-process cooker, 1-8 in-process compiler sessions, shallow cancellation, no priority/dedupe/memory ceiling. |
| 15. Front end and intermediate form | DXC HLSL to DXIL/SPIR-V; Slang to DXIL/SPIR-V; source-specific compilers | DXC is the release oracle for HLSL. Slang output is accepted only after the same options, reflection, diagnostics, symbols, and paired-backend tests pass. | Compiler diversity finds issues and enables language features but doubles versioning and reproducibility obligations. | DXC is feature-complete for current HLSL. Slang is narrower and does not honor all cook policy switches. |
| 16. Optimization/code generation | debug/O0; optimized/O3; separate analysis variants; profile-guided/vendor compilation | Cook optimized runtime bytecode; produce replayable debug/symbol artifacts by policy; compare disassembly and resource use for selected acceptance shaders. | Debuggable code may differ from optimized execution; analysis must preserve provenance to avoid comparing the wrong binary. | DXC applies optimization/debug/strip policy; Slang policy parity is incomplete. |
| 17. Reflection and ABI validation | trust hand-authored bindings; runtime reflection; offline reflection plus runtime signature | Extract reflection offline, normalize it, compare against typed parameters on cache hit and miss, persist a strong signature, and revalidate package integrity at load. | More cooker work and schema data; binding failures move out of rendering. | Strong cooker/runtime record validation exists; the authoring side still has duplicate parameter authorities. |
| 18. Static shader analysis | compiler warnings only; lint/validation; DXIL/SPIR-V validation; vendor ISA analysis | Warnings-as-errors in owned release shaders, DXIL/SPIR-V validators, bounded statistics, and opt-in RGA/Nsight/PIX analysis for representative hot shaders. | Vendor analysis is hardware/tool-version specific and cannot be a hermetic gate. | Warnings policy and cooked statistics exist; no complete validation/ISA regression lane. |
| 19. Compile diagnostics | console errors; dump every job; dump failures; reproducible job bundle | Structured source diagnostics plus a failure-first replay bundle containing preprocessed source, dependencies, arguments, compiler identity, reflection, and a one-job command. | Bundles consume storage; failure-only default controls bloat. | Successful opt-in DXC bundles exist; failed compiles and Slang do not have equivalent replay artifacts. |
| 20. Derived-data cache | no cache; local path cache; layered content-addressed DDC; remote farm cache | Layered disposable content-addressed DDC with local writable and optional shared/read-only layers, integrity checks, quarantine, and hit provenance. | Shared caches need permissions, eviction, and poisoning defense; they must never become source authority. | Atomic local stage cache; debug-artifact mode bypasses reads; corrupt data fails instead of becoming a recoverable miss. |
| 21. Cooked logical maps | per-pass file; global shader map; material/asset-local shader maps | Generated global shader map and program manifest for current global shaders; add asset-local maps only when material/content shaders exist. | Maps add indirection but give typed lookup and stable logical identity. | Per-program `.sparkshader`; generated registry is published but runtime does not use it as lookup authority. |
| 22. Code records and physical library | code embedded per program; indexed per-program records; one monolithic library; project/plugin/chunk libraries | Always emit exact code hashes and manifest references. Physically merge duplicate blobs or add compression/chunks only after measured byte, I/O, patch, or preload benefit; never expose membership to passes. | A simple index is proportionate for the current catalog; richer libraries improve scale but add formats, I/O policy, and failure modes. | Bytecode is duplicated inside package containers and compile cache keys are package-scoped. |
| 23. Compression/chunking | loose uncompressed records; block compression; whole-library compression; platform containers | Keep the first indexed migration format simple. Add independently compressed blocks/chunks only when startup, size, patch, or preload measurements justify them and retain integrity coverage. | Small blocks stream well but compress less; large blocks compress well but amplify reads and patch deltas; uncompressed records may win at current scale. | One container per program; no cross-program compression/chunk policy. |
| 24. Publication/security | overwrite in place; temporary files plus rename; generation manifest; signing | Deterministic transactional generation publication with hashes, schema/toolchain provenance, rollback, and optional platform signing at release packaging. | Keeping generations costs disk; it prevents partial activation and makes failures auditable. | Transactional packages, registry, and recook signal with stale-generation rejection already exist. |

### Runtime Loading, Residency, Pipelines, and Execution

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 25. Open and index | scan loose files; direct computed paths; open a manifest/library index | Open and validate the active generation's shader map, program manifest, and code-library indexes once. | Index memory and startup validation cost buy deterministic lookup and better I/O planning. | Runtime computes a path from handwritten package ID and caches loaded packages. |
| 26. Code loading | eager all; synchronous lazy; async preload; memory mapping; chunk streaming | Validate indexes eagerly and make required programs ready before interaction. Compare eager code against bounded predicted preload; keep any lazy fallback outside pass Execute and report late use. | Eager load raises startup/memory; lazy load risks first-use stalls; prediction and streaming can waste I/O/complexity. | Per-program package is loaded on first runtime materialization during graph construction. |
| 27. Code lifetime/compression | uncompressed eager records; transient decompression; retained decompressed code; budgeted LRU/refcounts | Start with exact code-record bytes and explicit generation/program/PSO lifetime. Add compressed/decompressed budgets or eviction only when the selected format and measured catalog need them. | Simple retention may be cheapest at current scale; compression/eviction add states and failure paths. | No separate code-record lifetime owner or readiness policy. |
| 28. RHI shader/module creation | backend consumes bytecode during PSO creation; transient module; reusable shader object | Prepare required PSOs before interaction. Treat native shader/module creation and caching as backend-specific: D3D12 consumes bytecode for PSOs; Vulkan modules may be transient after pipeline creation. | Retaining native objects may reduce repeated creation but consumes memory and is not portable. | D3D12 consumes bytecode in PSO creation; Vulkan creates shader modules during runtime materialization. |
| 29. Binding layout/root signature | per-pass hand-authored; reflection-generated; typed ABI-generated and reflection-verified | Generate backend-neutral layout intent from typed parameters, validate cooked reflection, then create/cache native root signatures or pipeline layouts by strong layout signature. | Fully generated layouts constrain bespoke root-layout tuning; allow explicit, reviewed policy hooks rather than parallel declarations. | Cooked layouts and runtime creation exist, but debug names/package lookup and parameter declarations repeat. |
| 30. Logical pipeline descriptor/key | pointer identity; ad hoc hash; complete canonical descriptor | Canonical `RenderPipelineKey` over shader code hashes, layout signature, specialization, and every API-visible fixed-function field. | Canonicalization and completeness are exacting; incomplete keys cause incorrect reuse. | A Vulkan key-shaped value is computed then discarded; no shared renderer pipeline-key authority. |
| 31. Pipeline-description cache | no cache; in-memory map; serialized stable descriptions; recorded usage database | In-memory dedupe plus explicit global-program descriptors; later add stable recorded descriptions for content-dependent PSOs. | Explicit enumeration can miss dynamic content; recording covers reality but only exercised workloads. | Each pass runtime owns pipelines; there is no cross-pass descriptor cache or recorded database. |
| 32. Native driver cache/binary | rely on opaque driver cache; D3D12 cached blob/library; Vulkan pipeline cache; Vulkan pipeline binaries; D3D12 advanced shader delivery | First make complete canonical renderer PSO descriptors, async enumeration/prewarm, and telemetry correct. Then evaluate a validated Vulkan `VkPipelineCache` and feature-gated D3D12 cached/library path independently; admit newer binary/delivery paths only with capability and measured benefit. | Native data is adapter/driver/API-version sensitive; backend paths differ and remain recoverable optimizations, not source authority. | D3D12 passes an empty `CachedPSO`; Vulkan passes `VK_NULL_HANDLE`. |
| 33. Pipeline compilation/linking | synchronous first use; async creation; partial libraries; shader objects | Asynchronously create complete conventional pipelines from known descriptors. Defer graphics-pipeline libraries, partial programs, and shader objects until pipeline counts and driver evidence justify them. | Monolithic pipelines are simple and optimizable but expensive to create; partial linking reduces duplication with more state combinations and capability paths. | Graphics/compute pipelines are created synchronously during first runtime materialization. |
| 34. Prewarming policy | compile every possible PSO; explicit precache; recorded/bundled cache; hybrid | Hybrid: explicitly enumerate bounded global compute/graphics PSOs, record content-driven misses when content shaders arrive, prioritize requests, and wait only at accepted loading boundaries. | Over-precaching increases startup, CPU, memory, and driver-cache size; under-precaching hitches. | No precache coordinator, queue, wait/fallback policy, or readiness telemetry. |
| 35. First-use behavior | block; skip draw; fallback shader/PSO; lower-quality feature path | Required startup pipelines block before interaction; optional work uses an explicitly accepted fallback or omission policy; any synchronous late creation records `Missed`/`TooLate`. | Fallbacks cost quality and maintenance; blocking costs latency but preserves correctness. | First use can synchronously create on graph construction; no event classifies the stall. |
| 36. RDG declaration and scheduling | immediate API calls; untyped graph node; typed RDG pass | Typed pass parameters declare resources/queues; graph compilation owns hazards and scheduling; shader/program lookup is resolved before Execute. | RDG adds a compilation layer but provides lifetime, synchronization, aliasing, async-compute, and diagnostics. | Frame graph already declares resources and supports draw, dispatch, and async dispatch. |
| 37. Binding and dispatch | bind by slot convention; bind by reflection name; bind typed prepared parameters | Bind a validated program/layout/pipeline and prepared parameter block, then issue draw/dispatch. Execute must not perform file I/O, compilation, layout creation, or pipeline creation. | Strict preparation requires visible preload/materialization failures; execution becomes deterministic. | Current Execute binds and dispatches only; creation occurs earlier during graph construction. |

### Diagnostics, Iteration, Analysis, and Retirement

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 38. Semantic labels and provenance | human names only; hashes only; both | Carry readable pass/program/shader names plus stable program, code, pipeline, and generation hashes into logs, markers, object names, captures, and crash records. | Extra metadata has storage/runtime cost; bounded development metadata makes evidence joinable. | Readable names exist, stable cooker-to-capture hash correlation does not. |
| 39. Source debugging | visual output; shader printf/asserts; source debugger; replay capture | Support all four: cheap visualization first, bounded debug instrumentation, external PIX/RenderDoc/Nsight debugging, and reproducible compiler/capture artifacts. | Debug builds and instrumentation alter timing and code generation; conclusions must identify the binary used. | Visualize Buffers and capture labels exist; symbol/provenance workflow is incomplete. |
| 40. Performance analysis | compiler statistics; IR/disassembly; vendor ISA; GPU counters/timelines | Preserve DXIL/SPIR-V inspection, add selected RGA/Nsight analysis, and correlate static register/LDS data with runtime occupancy, cache, bandwidth, and latency evidence. | Static estimates do not prove runtime bottlenecks; hardware results are device/driver specific. | Cooked byte/reflection stats exist; no joined static/runtime shader evidence. |
| 41. Hot reload invalidation | reload all; timestamp scan; content/dependency invalidation; runtime patch-in-place | Changed virtual paths select reverse dependencies; produce a complete validated generation; atomically swap; never patch individual live entries. | Whole-generation publication duplicates some data briefly but keeps rollback and lifetime coherent. | Timestamp polling notices `.hlsl`, `.hlsli`, and `.slang`; all-catalog plan plus cache hits; safe generation swap. |
| 42. Cancellation/failure | abort process; cooperative job cancellation; keep partial output; transactional rollback | Cooperative cancellation at job/backend/process boundaries; discard incomplete generation; keep previous active generation; initial startup failure is explicit. | Some compiler calls are not interruptible; process isolation may be needed to bound cancellation. | Child process can be cancelled, but running compiler sessions are not cooperatively stopped. Rollback is strong. |
| 43. Lifetime/retirement | device idle; immediate destroy; queue-fence/token retirement | Reference resources through the active generation and retire old shader/pipeline/library state only after every recorded queue submission token completes. | More bookkeeping and transient overlap; no global device-idle reload hitch. | Implemented by `RenderPassRuntimeCache`; preserve it. |
| 44. Release evidence | successful compile; one smoke frame; paired deterministic evidence | Prove clean and warm cooks, cache corruption recovery, startup/prewarm, D3D12/Vulkan captures, hash-to-source lookup, representative disassembly, fallback behavior, and p50/p95/p99 latency/memory. | Evidence takes hardware time and storage; it is the difference between architecture intent and demonstrated engineering. | Catalog validation and one representative CLI artifact exist; shader-specific CTest/paired hardware evidence is incomplete. |

The recommended path is intentionally conventional at the API boundary: offline high-level compilation, typed maps/libraries, conventional complete pipelines, and measured asynchronous precaching. Vulkan shader objects, graphics pipeline libraries, Vulkan pipeline binaries, D3D12 partial programs, work graphs, and distributed compilation remain options, not assumed improvements. Each adds a capability branch and is adopted only after current pipeline counts, first-use timings, and a representative workload show that the simpler path is insufficient.

## Current Sparkle Inventory

### Registered Programs and Actual Frame Consumption

This table covers every current renderer shader package/program. "Live" means a frame producer can dispatch it; it does not mean every configuration executes it every frame.

| Current program/package | Stage and required feature | Producer or selection branch | Code-backed status |
| --- | --- | --- | --- |
| `ComputeClear` | compute | utility helper; used for reservoir and other clears with per-instance labels | **Live and reused.** One shader program legitimately serves several graph operations. |
| `DirectShadowSignalNoRayQuery` | compute; no RT feature | intended no-ray fallback | **Registered/cooked but unconsumed.** No frame producer dispatches this pass. |
| `DirectShadowSignal` | compute; descriptor indexing + AS + inline ray query | ReSTIR direct-light shadow signal | **Live.** The producer always selects this descriptor path. |
| `DirectShadowSignalDeviceAddress` | compute; descriptor features + AS device address + inline ray query | intended partitioned-TLAS/device-address path | **Registered/cooked but unconsumed.** Provider selection and the frame producer do not choose it. |
| `DirectLightReservoirTemporal` | compute | ReSTIR direct temporal reservoir | **Live in `RestirPathTraced`.** |
| `DirectLightReservoirSpatial` | compute | ReSTIR direct spatial reservoir | **Live in `RestirPathTraced`.** |
| `DirectLighting` | compute | ReSTIR direct resolve/lighting | **Live in `RestirPathTraced`.** |
| `ExposureReduceScene` | compute | first level of parallel-reduction metering | **Live when metering is `ParallelReduction`.** |
| `ExposureReduceTexture` | compute | remaining parallel-reduction levels | **Live when the reduction requires more levels.** |
| `ExposureDownsampleScene` | compute | first level of downsample-pyramid metering | **Live when metering is `DownsamplePyramid`.** |
| `ExposureDownsampleTexture` | compute | remaining downsample-pyramid levels | **Live when the pyramid requires more levels.** |
| `Exposure` | compute | exposure history/resolve after either metering branch | **Live.** Dispatched asynchronously in the frame graph. |
| `GBuffer` | vertex + pixel | `GBufferMode::Rasterized` | **Live conditional graphics program.** It proves stages need not share a file. |
| `RaytracedGBuffer` | compute; descriptor indexing + AS + inline ray query | `GBufferMode::Raytraced` | **Live conditional ray-query compute program; not an RT pipeline.** |
| `SkyMotionVector` | compute | after either GBuffer producer | **Live.** |
| `SceneDepth` | compute | depth linearization after either GBuffer producer | **Live.** |
| `PathTracedDirectLighting` | compute; descriptor indexing + AS + inline ray query | reference path-traced lighting | **Live in `ReferencePathTraced`.** |
| `PathTracedIndirectLighting` | compute; descriptor indexing + AS + inline ray query | reference path-traced lighting | **Live in `ReferencePathTraced`.** |
| `ReferenceLightingAccumulation` | compute | accumulation/finalization of reference sample | **Live in `ReferencePathTraced`.** |
| `RestirIndirectTemporal` | compute; descriptor indexing + AS + inline ray query | ReSTIR indirect temporal reservoir | **Live in `RestirPathTraced`.** |
| `RestirIndirectSpatial` | compute; descriptor indexing + AS + inline ray query | ReSTIR indirect spatial reservoir | **Live in `RestirPathTraced`.** |
| `RestirIndirectResolve` | compute; descriptor indexing + AS + inline ray query | ReSTIR indirect resolve | **Live in `RestirPathTraced`.** |
| `LightingComposite` | compute | after either lighting-mode producer | **Live.** |
| `Sky` | compute | after lighting composite | **Live.** |
| `LinearUpscale` | compute | baseline upscaling and current pre-external-upscaler path | **Live.** The current graph schedules it before an enabled external upscaler too, so selection/cost should be verified separately from shader management. |
| `VisualizeBuffers` | compute | debug view-mode branch | **Live conditional diagnostic shader.** |
| `ToneMapping` | compute | presentation when a back buffer is requested | **Live conditional on presentation.** |
| `OutputEncoding` | compute | presentation after tone mapping | **Live conditional on presentation.** |

The current catalog therefore has 28 logical program/package IDs and 29 stage registrations. Twenty-six programs have at least one frame producer. The two no-ray/device-address shadow alternatives are catalog-only. All current ray/path names still execute compute shaders with inline ray queries. There is no registered ray-generation, miss, closest-hit, any-hit, intersection, or callable shader in Renderer.

### Compile, Cook, and Runtime Decision Branches

| Branch point | Current choices | What actually happens | Required target decision |
| --- | --- | --- | --- |
| cook selection | all; `--package`; `--shader-id` | default and editor Changed/Global cook the catalog; targeted CLI paths select a package or entry | Keep release-all and targeted modes; add changed-path dependency closure. |
| source language/backend | DXC and Slang; `auto` or explicit | auto uses DXC for `.hlsl` and Slang for `.slang`; all current registrations are HLSL, so production auto-cooks use DXC | DXC remains oracle; reject or label backend-policy mismatches until Slang parity tests pass. |
| target | DXIL SM 6.0-6.7; SPIR-V 1.4-1.6 | default cook requests DXIL SM 6.6 and SPIR-V 1.6; runtime contract consumes the backend-matching one | Keep one declared release target per RHI/platform and use older/newer targets only in explicit compatibility/analysis matrices. |
| stage | VS, PS, GS, HS, DS, CS; RT library metadata | cooker/schema know six raster/compute stages; Slang maps only VS/PS/CS; runtime graphics descriptors create VS plus optional PS only | Treat GS/HS/DS as schema-only until RHI descriptors and tests exist; mesh/task are unsupported; RT library is compiler-only. |
| package kind | graphics; compute; RT library | graphics/compute can reach runtime; valid RT library packages are deliberately rejected at runtime | Keep the rejection explicit until paired state-object/pipeline, SBT, command, lifetime, and tests land. |
| compiler capability filter | supported; skipped target; no targets left | RT libraries are skipped per target if backend capability is absent; DXC advertises DXIL RT library but not SPIR-V RT library; Slang advertises neither | Capabilities must be target- and policy-probed, reported in manifests, and tested rather than inferred from backend name. |
| feature flags | inline ray query; AS; AS device address; descriptor indexing | planning can filter some compiler capabilities; runtime library directly checks AS and inline ray query only | Validate every declared feature, including device-address and descriptor-indexing requirements, before materialization. |
| local compile cache | disabled; hit; miss; corrupt/incompatible | debug-artifact mode forces a read miss; normal corrupt/incompatible data terminates the cook | Treat corrupt/incompatible disposable cache entries as quarantined misses and report recovery. |
| compile policy | debug info; optimization; warnings-as-errors; strip debug | DXC applies these controls; Slang currently does not apply equivalent policy | One canonical request must either be honored or rejected as unsupported by every backend. Never silently ignore release policy. |
| analysis | none; debug-artifact directory; `cooked-shader-stats` | DXC success can emit source/arguments/disassembly/debug data; Slang is narrower; failures have no full bundle | Failure-first portable replay bundles plus optional successful analysis and backend-specific extensions. |
| task execution | serial through 8 sessions | bounded tasks, one backend instance per node, cancellation before job start | Add job priority, dedupe/fan-out, cancellation boundary, timings, and memory budget before adding another worker pool. |
| publication | selected package files + registry + signal | staged files publish transactionally; runtime still resolves manual paths | Publish a complete generation manifest and make it the lookup authority. |
| runtime backend | D3D12/DXIL; Vulkan/SPIR-V | one runtime-format binary/layout is selected and strongly validated | Preserve backend-neutral program identity and paired-backend validation. |
| runtime creation | cache hit; first materialization; replacement generation | package/layout/pipeline creation is lazy during graph construction; generation replacement is eager-validation plus atomic swap | Add preload/materialization scheduling and pipeline readiness; preserve safe generation swap. |
| render feature | raster/ray-query GBuffer; ReSTIR/reference lighting; exposure method; debug/presentation/upscaler branches | frame CVars/settings choose producers; package catalog itself does not prove a branch is consumed or a fallback works | Generate a program-use inventory and test each supported branch/fallback on both backends. |

### Current Renderer Flow by Configuration

```text
Frame
 |
 +-- TLAS/BLAS preparation -----------------------------------------+
 |                                                                 |
 +-- GBufferMode                                                    |
 |    +-- Rasterized ----> GBuffer VS+PS                            |
 |    `-- Raytraced -----> RaytracedGBuffer CS + inline ray query <-+
 |             |
 |             +--> SkyMotionVector CS --> SceneDepth CS
 |
 +-- LightingMode
 |    +-- RestirPathTraced
 |    |    +--> Direct reservoirs --> DirectShadowSignal CS -------> inline ray query
 |    |    +--> DirectLighting CS
 |    |    `--> ReSTIR indirect temporal/spatial/resolve ----------> inline ray query
 |    |
 |    `-- ReferencePathTraced
 |         +--> PathTracedDirectLighting CS -----------------------> inline ray query
 |         +--> PathTracedIndirectLighting CS ---------------------> inline ray query
 |         `--> ReferenceLightingAccumulation CS
 |
 +--> LightingComposite CS --> Sky CS
 |
 +-- ExposureMeteringMethod
 |    +-- ParallelReduction --> ReduceScene CS --> ReduceTexture CS x N
 |    `-- DownsamplePyramid --> DownsampleScene CS --> DownsampleTexture CS x N
 |                    `----------------------------------------------> Exposure CS
 |
 +--> LinearUpscale CS --> optional external upscaler
 +--> optional VisualizeBuffers CS
 `--> ToneMapping CS --> OutputEncoding CS --> copy/present

Catalog-only shadow alternatives:
  DirectShadowSignalNoRayQuery CS       (no producer selects it)
  DirectShadowSignalDeviceAddress CS    (no producer/provider selects it)
```

This is why capability declaration, successful cooking, runtime creation, and frame consumption need separate evidence states. A fallback is real only when a selection owner chooses it, its resources and parameter ABI are valid, and an exercised test or capture proves its output.

## Cache, Streaming, and Prewarming Taxonomy

Calling all of the following a "shader cache" hides ownership and invalidation bugs. They solve different costs and use different keys.

| Layer | Prevents or reduces | Correct key/invalidation owner | Recommended policy | Current Sparkle |
| --- | --- | --- | --- | --- |
| dependency/preprocess record | rescanning and re-deriving include closure | virtual source contents, preprocessor provenance, defines; ShaderCompiler | persist reverse dependencies with compile records; regenerate when the current tool contract or integrity check rejects them | closure is rebuilt and hashed; reverse graph is not a durable selection authority |
| in-flight job cache | duplicate concurrent compilation | full `ShaderCompileInputHash`; compile coordinator | one producer with result fan-out, priority escalation, and cancellation reference counts | absent |
| completed compile DDC | repeated front-end/codegen/reflection work | full `ShaderCompileInputHash` and compiler provenance; ShaderCompiler | disposable layered local/shared store with integrity/quarantine | local package-scoped stage cache |
| logical shader map/program manifest | repeated discovery and unstable lookup | shader type/permutation/target and program IDs; cooker/Renderer | immutable per-generation maps with strong references to code hashes | package registry published but not runtime authority |
| cooked code records/library | duplicate bytecode and scattered I/O | exact code hash; cooker/runtime service | independently indexed and integrity checked; merge/compress only after measured benefit | bytecode embedded per program package |
| OS/file/decompression cache | disk reads and decompression | physical records/chunks; platform I/O/runtime service | measured reads; add block compression and decompressed residency only if the selected format needs them | ordinary per-file reads; no explicit streaming/decompression layer |
| code and transient backend-object cache | repeated code reads or backend object creation | code hash + backend creation identity; runtime/RHI | code lifetime follows maps/programs/PSOs/generations; cache native shader/module objects only where that backend has such a reusable object and measurement justifies it | live objects are owned by each materialized pass runtime |
| canonical pipeline descriptor cache | duplicate logical pipeline requests | complete `RenderPipelineKey`; Renderer | in-memory dedupe and readiness/usage telemetry | absent |
| recorded/stable PSO database | unknown content-driven PSO enumeration | stable high-level descriptor plus build/platform version; Renderer/cooker | add only when content creates combinations that bounded declarations cannot enumerate | absent |
| native pipeline cache/library/binary | repeated driver compilation/linking | native API/device/driver/cache UUID identity; RHI backend | validated, transactionally persisted, recoverable miss; never source authority | D3D12 empty cached state and Vulkan null cache |
| opaque driver internal cache | vendor-specific repeated work | driver-owned | treat as helpful but unobservable/non-portable; do not use as readiness proof | implicitly relied upon |

The candidate residency model is a small hybrid, not a mandatory streaming subsystem:

1. At generation activation, open and validate maps/library indexes without creating every pipeline.
2. Make code and pipelines required for the first interactive frame ready before entry.
3. Queue predicted next-feature programs and PSOs at bounded priority during loading or spare task budget.
4. Keep a lazy path for truly optional programs, but materialize before pass Execute and record it as late if it enters a frame-critical path.
5. Track file/code bytes, native objects where the backend exposes them, pipeline counts/memory proxies, requests, hits, misses, and read/create time. Add compressed/decompressed/eviction fields only when those mechanisms exist.
6. Refuse release while a program, native pipeline, active/reloading generation, or recorded GPU submission can still reference an object whose backend lifetime requires it.

For Sparkle's current 28-program catalog, eager code loading may prove cheaper and cleaner than elaborate streaming. The architecture should make that a measured policy choice. It must not require a streaming implementation before file count, startup I/O, code size, or memory demonstrates a problem.

### PSO Prewarming Choices

| Strategy | Strength | Cost/failure mode | Recommendation |
| --- | --- | --- | --- |
| compile synchronously on first use | minimal startup and enumeration work | visible frame hitch and nondeterministic driver work | Reject for required or common pipelines. Retain only as classified late fallback. |
| enumerate every theoretical PSO | strongest nominal coverage | combinatorial cook/startup/memory bloat and unused work | Reject. Filter permutations and descriptors to reachable configurations. |
| explicit precache declarations | deterministic, reviewable, ideal for bounded global shaders | authors/collectors can omit a legal state | **Use now** for Sparkle global compute and graphics programs with catalog validation. |
| recorded/bundled usage cache | captures content combinations actually exercised | misses unplayed branches and can go stale across shader/content changes | Add with material/content PSOs; use stable descriptions, never raw transient hashes alone. |
| native driver cache only | little engine work | opaque coverage, device/driver invalidation, first-run hitches | Use as an acceleration layer, never as the only plan. |
| hybrid explicit + recorded + native | covers known global state, observed content, and repeated driver work | most coordination and telemetry | **Long-term recommendation**, matching Unreal's separation of automatic precaching and bundled caches. |

Prewarming must expose at least `Requested`, `AlreadyReady`, `Compiling`, `Precached`, `Used`, `Missed`, `TooLate`, `Failed`, and `Evicted`, with queue time, creation time, first-use time, pipeline key, program/code hashes, generation, backend, adapter, driver, and cache source. A shader-map/code-library hit is never reported as a PSO hit.

## Ray Query Versus Ray-Tracing Pipeline and SBT

Sparkle must teach and report these as two different execution architectures.

```text
INLINE RAY QUERY (CURRENT)

compute shader + compute PSO
        |
        +--> bind TLAS/AS descriptor (or a supported address representation)
        +--> RayQuery / Proceed inside normal compute invocation
        `--> Dispatch

Required: BLAS/TLAS, AS resource/binding, inline-ray-query compiler/API feature
Not required: RT library exports, RT state object/pipeline, shader identifiers, SBT, DispatchRays

FULL RAY-TRACING PIPELINE (DEFERRED)

RT shader types: raygen / miss / closest-hit / any-hit / intersection / callable
        |
        +--> libraries + exports + hit groups
        +--> global/local layout associations
        +--> payload / attribute / recursion / stack policy
        v
native RT state object or VkPipeline
        |
        +--> query shader identifiers / shader-group handles
        v
SBT buffer: raygen | miss[] | hit[] | callable[]
        |
        +--> aligned records: identifier/handle + bounded local record data
        +--> instance/geometry/ray-type indexing contract
        v
bind RT pipeline + SBT regions + DispatchRays / vkCmdTraceRaysKHR
```

### Full RT Pipeline Decisions

| Concern | Options | Recommended initial implementation | Tradeoff |
| --- | --- | --- | --- |
| library/pipeline granularity | one large library/RTPSO; collections/libraries plus link; one pipeline per effect | One small complete effect pipeline first, expressed by a typed `TRayTracingProgram`; measure compile/link before introducing collections. | Large pipelines maximize shared optimization but are slow and hard to replace; many small pipelines duplicate work and switches. |
| shader exports and hit groups | string lists; generated typed IDs; reflection discovery | Typed declared exports/hit groups validated against compiler output; persist stable logical IDs but query native identifiers per native pipeline. | Native shader identifiers/group handles are pipeline-specific and cannot be serialized as universal shader identity. |
| global/local parameters | all global descriptors; local root/record data; bindless indices/device addresses | Keep most resources in global/bindless tables; keep SBT record data to small stable indices/constants only. | Fat local records are simple per hit but multiply SBT memory, upload bandwidth, and update complexity. |
| SBT organization | per instance; per geometry/material/ray type; deduplicated records with indirection | Explicit formula and one ray type in the first vertical slice; use TLAS instance contribution plus geometry/ray offsets; deduplicate material/geometry data outside SBT. | Indirection reduces memory and churn but adds shader loads and indexing complexity. |
| SBT update | rebuild every frame; patch dirty ranges; persistent GPU-generated table | CPU-build an immutable/persistent table per validated scene generation first; add dirty-range or GPU generation only from measured update cost. | Full rebuild is simple but scales poorly; incremental/GPU updates complicate synchronization and validation. |
| alignment/layout | backend-specific code paths; one conservative cross-API layout; normalized builder with backend rules | One backend-neutral record builder that applies D3D12/Vulkan handle size, base alignment, record alignment/stride, region, and bounds rules explicitly. | A conservative maximum wastes memory; backend-specific packing needs paired tests. |
| pipeline/SBT lifetime | SBT independent; rebuild on pipeline change; cache native handles | Tie SBT records to the exact native RT pipeline generation whose identifiers/group handles they contain; retire both after GPU completion. | Rebuild/upload on pipeline reload is mandatory, but stale identifiers cannot execute. |
| recursion/stack | maximum device limits; fixed conservative values; shader-derived measured policy | Start with recursion depth 1 and explicit payload/attribute contracts; query/report stack information and increase only for a demonstrated algorithm. | Higher recursion/payload/stack improves expressiveness while reducing occupancy and raising memory/driver cost. |
| dispatch integration | direct command calls; neutral RHI `TraceRays`; RDG typed RT pass | Add backend-neutral RT pipeline/SBT descriptors and a typed frame-graph ray-dispatch pass in one vertical slice. | Partial schema support without command/execution ownership creates misleading dead infrastructure. |

The current cooked schema already models RT exports, hit groups, local parameter records, payload/attribute sizes, and recursion metadata. That is useful compiler-only scaffolding, but it is not a runtime feature. Before changing the runtime rejection, Sparkle needs all of these together: Renderer registrations and typed RT program composition; DXIL and SPIR-V compiler capability parity or an explicitly platform-limited contract; native D3D12 state-object and Vulkan RT-pipeline creation; group identifier retrieval; SBT construction/alignment/indexing; RHI command recording; RDG resources and synchronization; pipeline/SBT generation lifetime; precache/cache telemetry; a non-RT accepted fallback; and paired execution/capture tests.

## Feature Horizon and Adoption Gates

| Feature | Value | Why it is not the current default | Adoption gate |
| --- | --- | --- | --- |
| geometry and tessellation stages | legacy programmable raster expansion/tessellation | schema/compiler awareness is not runtime pipeline support; no registered workload uses them | complete neutral/native descriptors, paired shader/program tests, and a workload that beats simpler mesh/compute/raster alternatives |
| mesh/task shaders | modern GPU-driven geometry pipeline | absent from shader-stage schema and both RHI capability paths report unsupported | paired API support, fallback path, GPU-driven workload, pipeline/cache/debug evidence |
| specialization constants | reduce source-level variants and defer values toward pipeline creation | behavior and pipeline-cache identity differ by backend; current variant budget is tiny | cross-backend semantics, canonical PSO key, cold/warm timing, and code-size evidence |
| Slang modules/generics | reusable interfaces, generics, and specialization across targets | current backend lacks release-policy/diagnostic/stage parity and all current sources are HLSL | representative shaders match ABI, output validation, symbols/replay, performance, and paired-backend results |
| Vulkan graphics pipeline library | partial precompile/link for high pipeline/material counts | another capability/cache/link path with no measured current need | monolithic creation/link is a demonstrated bottleneck and driver coverage is acceptable |
| Vulkan shader objects | more dynamic shader/state binding | moves responsibility from pipeline objects to many dynamic states and changes cache/debug assumptions | a workload benefits after correctness, state completeness, and driver evidence across target devices |
| Vulkan pipeline binaries | explicit reusable native pipeline artifacts | newer capability with strict device/driver lifecycle and distribution concerns | target platform support plus measured improvement over robust `VkPipelineCache` |
| D3D12 partial programs/advanced shader delivery | reusable/precompiled state-object components | evolving platform capability and unnecessary for 28 bounded programs | target OS/driver availability and measured pipeline-delivery bottleneck |
| work graphs/execution graphs | GPU-driven scheduling of dependent work | distinct program, memory, synchronization, debugging, and capability model | accepted workload, conventional-compute comparison, fallback, and end-to-end evidence |
| distributed shader compilation/shared remote DDC | scale large teams/platform/permutation spaces | operational cost and poisoning/security/diagnostic complexity exceed current scale | local critical path and cache data show material benefit with reproducible administration |
| runtime source compilation | modding or developer experimentation | nondeterministic shipping startup, toolchain deployment, security, and failure containment | development-only sandbox or explicit product requirement; never the normal cooked runtime |

### Primary References for the Atlas

These sources supplement the references in [External Precedent and What Sparkle Adopts](#external-precedent-and-what-sparkle-adopts):

- [Epic: Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [Epic: Shader Debugging Workflows](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-debugging-workflows-unreal-engine)
- [Epic: PSO Precaching](https://dev.epicgames.com/documentation/unreal-engine/pso-precaching-for-unreal-engine)
- [Epic: Manually Creating Bundled PSO Caches](https://dev.epicgames.com/documentation/unreal-engine/manually-creating-bundled-pso-caches-in-unreal-engine)
- [Epic: `FShaderPipelineCache`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FShaderPipelineCache)
- [Microsoft: D3D12 graphics pipeline state](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)
- [Microsoft: `ID3D12PipelineLibrary::LoadGraphicsPipeline`](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12pipelinelibrary-loadgraphicspipeline)
- [Microsoft: DXR functional specification](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [Microsoft: PIX automatic shader PDB resolution](https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/)
- [Khronos: Vulkan pipelines and pipeline caches](https://docs.vulkan.org/spec/latest/chapters/pipelines.html)
- [Khronos: Vulkan ray tracing and shader binding tables](https://docs.vulkan.org/spec/latest/chapters/raytracing.html)
- [Khronos: `VK_EXT_graphics_pipeline_library`](https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_graphics_pipeline_library.html)
- [Khronos: `VK_EXT_shader_object`](https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_shader_object.html)
- [Khronos: `VK_KHR_pipeline_binary`](https://docs.vulkan.org/features/latest/features/proposals/VK_KHR_pipeline_binary.html)
- [NVIDIA: Nsight Graphics shader debugger](https://docs.nvidia.com/nsight-graphics/UserGuide/shader-debugger-overview.html)
- [NVIDIA: configuring shader debug information for Nsight](https://docs.nvidia.com/nsight-graphics/UserGuide/configure-application.html)
- [NVIDIA: DXR shader binding table tutorial](https://developer.nvidia.com/rtx/raytracing/dxr/DX12-Raytracing-tutorial-Part-2)
- [NVIDIA: SBT data-layout optimization](https://developer.nvidia.com/blog/efficient-ray-tracing-with-nvidia-optix-shader-binding-table-optimization/)
- [AMD: Radeon GPU Analyzer](https://gpuopen.com/rga/)
- [AMD: occupancy and shader resource limits](https://gpuopen.com/learn/occupancy-explained/)
- [AMD: RGA and RGP pipeline-binary interop](https://gpuopen.com/manuals/rgp_manual/rga_and_rgp_interop/)
- [He et al.: *Slang: Language Mechanisms for Extensible Real-Time Shading Systems*](https://graphics.cs.cmu.edu/projects/slang/he18_slang.pdf)

## Repository Requirements Traceability

This audit routes to the owning repository authorities instead of duplicating their rules:

- [Portfolio requirements](../../Strategy/Requirements.md) define what the finished work must prove.
- [Engineer persona](../../Strategy/EngineerPersona.md) defines the end-to-end graphics-engineering behaviors the implementation and evidence should teach.
- [Engineering standards map](../../Engineering/Standards/README.md) selects the binding implementation, graphics, tools, concurrency, naming, and validation rules.
- [Bistro and San Miguel workloads](../../Engineering/BistroAndSanMiguelWorkloads.md) define the paired-backend stress cases and evidence shape.
- [Renderer/RHI boundary](../RendererRhiBoundary.md) owns the architectural split among Renderer policy, frame-graph scheduling, and neutral backend creation.

`Meets` below means the reviewed code implements the shader-specific invariant. It does not mean the corresponding portfolio requirement is fully proven. `Partial` means a useful production path exists but misses a required invariant or evidence gate. `Missing` means no production path or executable proof was found. `Explicitly deferred` means the repository correctly exposes the feature as unavailable rather than silently pretending it works.

### Engineering-Standard Compliance

| Authority | Current shader-lifecycle status | Required architectural response |
| --- | --- | --- |
| [Integration style](../../Engineering/Standards/IntegrationStyleGuide.md) | **Partial.** There is one real cooker and one runtime-package path, but manually repeated package/pass/debug names and two parameter declarations create parallel authorities inside that path. | Replace each duplicated authority in a bounded vertical slice and delete its old macro, fallback, or declaration in the same slice. Do not add a second shader subsystem beside the current one. |
| [Repository ownership](../../Engineering/Standards/RepositoryStructureAndOwnership.md) and [Renderer/RHI boundary](../RendererRhiBoundary.md) | **Partial.** Renderer owns concrete registrations and passes; Tools owns cooking; RHI owns neutral pipeline creation and cooked validation. The RHI-facing `GlobalShader` authoring layer still carries source/package policy that should belong to the Renderer/tooling contract. | Keep neutral bytecode, reflection, layout, shader-object, and pipeline contracts in RHI. Keep shader types, programs, permutations, and pass use in Renderer. Keep import, compilers, cache, and cook publication in Tools. Application/editor only orchestrates recook and activation. |
| [Graphics engineering](../../Engineering/Standards/GraphicsEngineering.md) | **Partial.** DXIL/SPIR-V compilation, reflection, package validation, backend capabilities, readable labels, and runtime-format selection exist. Paired inspection, disassembly/counters, fallback captures, and exact hardware/driver evidence are not automated. | Make a paired-backend vertical slice the first proof; inspect layouts and IL on both targets; preserve a named fallback; record exact compiler, backend, hardware, driver, workload, and capture. |
| [Editor and tools](../../Engineering/Standards/EditorAndTools.md) | **Partial to strong.** Cooking is out of process; publication is transactional; stale generations are rejected; previous accepted artifacts survive failure. Cancellation is shallow, compiler memory is unbudgeted, and compile failures lack replay artifacts. | Preserve transactional replacement. Add job cancellation boundaries, bounded compiler-session memory/parallelism, progress/results integration, deterministic failure bundles, and a stable command/API usable outside the editor. |
| [Concurrency](../../Engineering/Standards/Concurrency.md) | **Partial.** Bounded work uses `SparkleTasks`, with no second general pool. It lacks priority, in-flight dedupe, compiler-session cancellation, serial-vs-N evidence, and memory-ceiling proof. | Evolve cook nodes into explicit jobs coordinated through `SparkleTasks`; never add a shader-only general worker pool. Prove serial, 1/2/N, cancellation, stale generation, failure, and memory behavior. Add worker processes only from measured compiler isolation/throughput need. |
| [Data-oriented design](../../Engineering/Standards/DataOrientedDesign.md) | **Partial.** Cook plans, reflection arrays, and compact cooked records are batch-friendly, but runtime lookup is string/path-led and physical packages duplicate bytecode. | Use immutable sorted manifest/map records and content-addressed code tables; keep human-readable strings in diagnostics, not hot lookup or cache identity. Measure layout/memory changes rather than asserting them. |
| [Naming and vocabulary](../../Engineering/Standards/NamingAndVocabulary.md) | **Partial.** Neutral RHI uses `RenderPipeline`, `GraphicsPipelineDesc`, and `ComputePipelineDesc`, while semantic pass labels are useful. Authored package IDs and generic-looking `PassName` fields conflate identities. | Generate default labels from typed pass traits, keep optional instance labels, use program/shader/artifact/code terms for their distinct roles, and reserve stable hashes for content/cache identity. Do not leak `PSO` into neutral public RHI names. |
| [Validation, performance, and evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md) | **Missing as a complete gate.** One representative CLI target exists, but no paired all-program validation, injected-defect proof, cold/warm report, pipeline-hitch evidence, or external capture pack was found. | Extend existing product/tool validation surfaces for every replaced contract, use temporary removed-before-handoff harnesses only where necessary, then run the paired DXIL/SPIR-V cook/load slice, negative ABI/cache/reload cases, serial/N and cold/warm matrices, and capture-backed evidence. A document or successful build is not acceptance evidence. |
| Frame-graph execution ownership | **Partial to strong.** RDG setup declares resources and materializes runtime state before Execute; Execute only binds and records work. Duplicate parameter metadata and count-only compatibility weaken the dependency/binding proof. | Make one schema own each shader-visible field, reuse/compose it in the pass envelope, and reject structural mismatch; never permit Execute to discover resources, load shaders, or create pipelines. |
| Reload and GPU lifetime | **Meets the reviewed invariant.** A replacement generation is fully built before activation, a failure preserves the active generation, and retirement waits on `RhiSubmissionToken` state for all queues. | Preserve this path unchanged while replacing lookup and physical storage. Test delayed GPU completion, reload churn, invalid replacements, and device-loss/error paths. |

### Portfolio Requirement Contribution

The shader architecture directly owns `PGE-09` and contributes evidence to several broader requirements. It cannot alone complete requirements that also need scene, workload, GPU-capture, productization, or adoption evidence.

| Requirement | Current contribution | Missing proof or design gate |
| --- | --- | --- |
| `PGE-01` Partner adoption and collaboration | **Indirect partial.** CLI/editor workflows and architecture notes can become an adoption surface; no second-engineer shader integration record is proved here. | Capture adopter constraints, review history, setup/cook/debug/fallback steps, measured outcome, issue/reproducer, and second-person reproduction without requiring hidden repository knowledge. |
| `PGE-02` Real-time ray tracing, GI, and path tracing | **Partial.** Sparkle has BLAS/TLAS and several path/ReSTIR compute programs using inline ray query, but no full RT pipeline/SBT and no verified no-ray/device-address shadow selection. | Prove raster, ray-query, and accepted fallback behavior under Bistro/San Miguel quality, temporal, latency, memory, and paired-capture gates. Add a full RT pipeline only through the complete conditional vertical slice. |
| `PGE-03` Neural graphics product feature | **No shader-lifecycle feature proof.** The architecture can carry a future generated/fixed inference program, but it does not provide a trained model, runtime inference, or classical fallback. | Reuse this exact map/library/ABI/preload/pipeline/provenance path for a real model artifact and shader kernels; do not create a neural-only compiler/runtime authority. Evidence remains owned by the neural workload. |
| `PGE-04` Model-to-kernel translation | **Future contribution only.** Slang is a backend seam, but no model-to-shader generator, operator contract, or generated-kernel runtime was found. | Use provenance-recorded generated virtual sources/source maps, deterministic regeneration, the normal compile request and ABI validation, numerical reference checks, latency/memory, disassembly/counters, precision/layout/fusion decisions, and classical fallback. |
| `PGE-05` Whole-system performance | **Partial.** Package sizes and runtime load microseconds exist; compile, residency, streaming, native pipeline, and hitch distributions do not. | Record compile queue/wall/CPU time, DDC and native-cache cold/warm behavior, I/O/decompression/residency, PSO creation, frame pacing, memory high-water, and p50/p95/p99 under a pinned workload. |
| `PGE-06` Workload analysis and hard debugging | **Partial.** The catalog targets DXIL/SPIR-V and both backends expose markers/debug names, but captured shaders are not joined to compile provenance. | Capture the same workload on both APIs; inspect queues, barriers, descriptors, memory, pipelines, shaders, symbols, and one hard incident with hypotheses, experiments, root cause, and minimal reproducer. |
| `PGE-07` C++ and Python software engineering | **Partial.** A C++ CLI, out-of-process orchestration, transactional cook, and runtime validation exist; no useful Python shader-analysis automation is required or proved by this design alone. | Keep the C++ ownership narrow and tested; add Python only for a concrete report/conformance/analysis workflow; provide clean-clone commands, deterministic artifacts, and documentation matching executable behavior. |
| `PGE-08` Applied mathematics and modeling | **Indirect.** Shader infrastructure cannot prove estimator, signal-processing, stability, or cost mathematics. | Let shader/program metadata link to the owning feature's math/reference tests and preserve exact permutation/compiler/capture identity so predicted cost/quality can be compared with measurement. |
| `PGE-09` Explicit APIs, shaders, compilers, and GPU ABI | **Partial.** Explicit D3D12/Vulkan, HLSL to DXIL/SPIR-V, reflection, cooked ABI validation, and diagnostics exist. | Produce a paired shader-source-to-runtime trace; inspect both compiled forms; prove layout/resource states and complete support matrix; inject defects; verify real fallbacks; join code/pipeline hashes to GPU events. |
| `PGE-10` CPU/GPU architecture and concurrency | **Partial.** Bounded cooker tasks and async-compute scheduling exist. | Compare serial/1/2/N compile execution with time/memory/cancellation; correlate IR/ISA register/LDS/scratch findings with runtime occupancy, divergence, cache/bandwidth, and synchronized queue evidence. |
| `PGE-11` Machine-learning fundamentals | **Out of shader-lifecycle scope.** No compile/package design demonstrates training, objectives, splits, optimization, quantization, or generalization. | Do not claim coverage. A future generated shader path consumes an independently validated frozen model artifact and records provenance; training evidence stays in its owning workflow. |
| `PGE-12` Training and inference workload engineering | **Partial infrastructure only.** Cooked packages are versioned, validated, atomically published, and lazy-loaded, which can support deterministic inference deployment; no real inference workload exists. | Measure export/cook/startup/preload/residency and inference latency/memory separately from training; preserve precision/layout variants and an explicit classical fallback under one normal runtime path. |
| `PGE-13` Productization, tools, and communication | **Partial.** CLI discovery/inspection, editor recook, and this source-linked design are credible beginnings. | Deliver edit-to-failure/replay/reload/trace workflows, stable navigation, clean cook/run, troubleshooting, bounded reports, adoption feedback, and deletion evidence for replaced concepts. |
| `PGE-14` Platform and ecosystem breadth | **Partial.** Windows D3D12/Vulkan code paths and compiler/tool references exist; native Linux/Vulkan behavior is not proved by this document. | Record OS, SDK, compiler, driver, capture/profiler, and build setup. Add native Linux/Vulkan cook-run-capture only before claiming it; keep platform limitations in the support matrix. |
| `PGE-15` Principal judgment and sustained influence | **Design target, not proof.** The proposal removes repeated authority, rejects premature streaming/RT/compiler complexity, and selects measured gates. | Demonstrate completed vertical slices, deleted old paths, fewer authored concepts, preserved capability/error quality, causal evidence, review/adoption, and a repository that became easier to explain and maintain. |

### End-to-End Lifecycle Verdict

| Lifecycle stage | Verdict from reviewed code | Recommended end state | Acceptance evidence |
| --- | --- | --- | --- |
| Write shader source | **Partial.** HLSL/HLSLI and recursive includes work, but physical search roots, absolute includes, and project-first shadowing define identity. | Canonical `/Engine`, `/Project`, and `/Plugin/<Name>` mounts; deterministic include ownership; no authored absolute path. | Mount collision, traversal, case policy, same-basename, source-move, and cross-checkout key tests. |
| Declare shader type | **Partial.** Explicit source, entry, stage, feature flags, and parameter descriptor exist. Static registration silently drops duplicates and freezes implicitly on first snapshot. | Immutable typed descriptor with declaration location, explicit registry freeze, collision errors, policy hooks, and a typed permutation domain. | Duplicate/late registration negative tests and a readable catalog dump. |
| Declare parameters and RDG resources | **Unsafe partial.** Typed pass resources drive graph declarations; a separate shader `FParameters` drives reflection; count-only binding compatibility can accept different layouts. | One schema owns every shader-visible field and is reused directly or composed into a pass envelope with graph-only fields; binding and structural signatures derive from that schema. | Direct one-shader, graph-only/copy, and composed-pass tests; a field reorder/kind/name/visibility/array/size defect fails before execution on both backends. |
| Compose a program | **Partial.** Shared package strings group stages and allow the valid multi-file `GBuffer` case. | Typed compute/graphics/ray-tracing program composition derives stage set, layout, stable logical ID, and manifest membership. | One file/multiple entries and multiple files/one program tests; no authored package string. |
| Select permutations | **Missing.** Variants are represented through free-form defines and separate registrations/packages. | Typed bounded dimensions with stable readable IDs plus compile, environment, validation, and precache policy. | Round-trip and boundary tests; invalid dimensions cannot compile; unsupported variants never enter a cook or precache request. |
| Build compile input and dependencies | **Partial.** Options and transitive include contents are hashed, but physical directories and path bytes leak into identity. | One immutable compile request with virtual source identity, normalized dependency graph, compiler provenance, platform/features, parameter signature, and debug policy. | The same source tree in different checkout roots produces the same input hash; every meaningful input change invalidates it. |
| Schedule compilation | **Partial.** Bounded `SparkleTasks` execution works. | Explicit logical job identity, `ShaderCompileInputHash`, result, priority, in-flight/completed dedupe, cooperative cancellation, bounded result application, and measured worker/session budget. | Serial/1/2/N, duplicate fan-out, cancellation, backend crash/failure, stale result, and memory-ceiling tests. |
| Compile DXIL/SPIR-V | **Strong foundation, incomplete parity proof.** DXC and Slang expose target capabilities; DXC produces DXIL/SPIR-V and rich successful analysis artifacts; reflection is extracted for both formats. | A backend-neutral result contract with equivalent diagnostics/provenance and declared analysis capability differences. | Representative optimized DXIL and SPIR-V builds for all supported stage kinds; reflection/layout comparison and compiler-version record. |
| Enforce compiler policy/capabilities | **Unsafe partial.** Target/package capability filters exist, but DXC and Slang do not honor the same policy controls and schema-known stages exceed executable runtime support. | Generated matrix for language, backend, target, stage, package kind, feature, and debug/optimization/warning/symbol policy; unsupported requests fail before jobs. | Every matrix cell is produced by a capability probe and executable positive/negative test; no ignored policy or unreported target skip. |
| Cache derived compile output | **Partial.** Atomic local store and cache-hit revalidation are good; keys mix program/package identity with reusable compilation and cache corruption is fatal. | Compile-input hash independent from program composition; one focused local derived-data owner; integrity validation; corrupt-entry quarantine; bounded stats and size policy. Add another store only for a proved current consumer. | Cold/warm, corrupt entry, compiler/tool provenance invalidation, two-program dedupe, cancellation, and deterministic publication evidence. |
| Cook and publish runtime data | **Strong publication, coarse storage.** Per-program packages, registry, and recook signal publish transactionally; code is duplicated and registry is not runtime authority. | Global shader map/program manifest plus unique code-hash library; physical chunk/file policy generated by cooker. | Reproducible manifest/library hashes, one copy per unique bytecode, transactional rollback, and package/chunk validation. |
| Inspect and reproduce compilation | **Partial.** Catalog/package inspection and successful opt-in artifacts exist. | One trace command plus always-available failure bundle containing virtual dependencies, preprocess output where available, exact arguments/defines, compiler/version, parameter comparison, and replay command. | Injected syntax/include/ABI/backend failures replay outside the editor and navigate to portable virtual paths. |
| Load and validate runtime shader | **Strong.** Cooked schema, target format, hashes, records, stages, features, reflection, and logical layouts are validated before use. | Typed shader/program lookup through the generated manifest and code hash, with lazy/preload policy independent of package filenames. | Cold/warm load time and memory, corrupt/truncated/wrong-backend/wrong-layout packages, and typed lookup across generation replacement. |
| Prepare code and native objects | **Missing as a separate owner.** Per-program loading/materialization is lazy and pass-runtime-owned; no explicit readiness/lifetime policy exists. | Measure eager code against startup-required/predicted preload; expose only the readiness/release operations the selected policy needs; prepare required PSOs and keep backend-native object lifetime explicit. | File/code/native-object/pipeline timing and high-water evidence; backend-specific release tests; proof that Execute performs no loading or creation. |
| Integrate with RDG | **Partial to strong.** Resource uses are declared during setup; runtime is materialized before Execute; Execute only binds and records. | Typed program/pass traits plus one owner per shader-visible field, reused or composed in a pass envelope; optional per-instance event labels remain presentation. | Direct/composed/shaderless pass tests, graph resource-state explanation, async-queue legality, no hidden creation in Execute, and meaningful capture markers. |
| Create/use graphics and compute pipelines | **Missing cache/precache behavior.** Complete descriptors reach RHI, but first graph use synchronously creates native pipelines without cached state. | Renderer-owned pipeline descriptor cache and precache policy; RHI-owned native cache/library integration; complete keys include shader code, binding layout, fixed function, vertex layout, formats, and relevant device identity. | Cold/warm D3D12 and Vulkan pipeline timings, hit/miss/untracked/too-late counts, first-use hitch threshold, invalid-cache recovery, and feature preservation. |
| Debug/profile on GPU | **Partial.** Semantic events and native object names exist; compiler artifacts are not joined to captured shader hashes or external symbols. | Stable capture correlation record from pass label and pipeline key to program, code hash, virtual source, compile request, and debug symbol. | PIX and RenderDoc paired captures plus Nsight or RGA analysis on the exact cooked shader; record counters, IL/ISA, hardware, driver, API, and workload. |
| Recook and hot reload | **Strong foundation, coarse invalidation.** Out-of-process cook, transactional signal, stale rejection, rollback, generation swap, and GPU-safe retirement exist; any change plans the whole catalog. | Persist reverse dependencies, select affected shader types/programs, publish a complete new generation, and retain current rollback/lifetime behavior. | Root/include change selection, unrelated-program exclusion, rapid edit coalescing, invalid replacement, delayed completion, and reload-churn tests. |
| Inline ray-query capability/fallback | **Live primary path, unproved alternatives.** Compute shaders use TLAS/inline queries, while the no-query and device-address shadow passes have no frame selection consumer. | One capability-selection owner chooses descriptor, supported device-address, or accepted no-ray behavior; each branch has complete feature/ABI validation. | Forced-capability paired tests/captures prove selection, resources, output, and failure reason for every advertised branch. |
| Full RT pipeline and SBT | **Explicitly deferred at runtime.** Schema/cooker inspection exists; no renderer RT registrations, native state-object/pipeline, group identifier, SBT, or trace-rays command path exists. | Add only as one paired vertical slice with typed exports/hit groups, layouts, payload/attribute/recursion/stack policy, native pipeline, SBT builder/index formula, RDG command/lifetime/cache integration, and selected fallback. | Raygen/miss/hit execution on D3D12/Vulkan; identifier-generation, alignment/index/bounds, reload/retirement, corruption, fallback, cold/warm pipeline, SBT memory/update, and capture evidence. |
| Automated conformance | **Missing as a complete gate.** The CLI validation custom target covers catalog structure and one program only. | Extend existing tool/product validation and CI evidence gates for identity, dependency, cache, layout, schema, and paired end-to-end cook/load/reload/pipeline behavior; use temporary local harnesses only when an existing surface cannot falsify the invariant. | Named executable checks, injected failures, all-program catalog cook, paired backends, `architecture_boundary_check`, clean diff/build evidence, and no submitted test-only scaffold unless separately authorized. |

### Required Evidence Pack

Implementation is not accepted merely when the new API compiles. The completed shader lifecycle must produce one navigable evidence pack for a pinned engine commit and workload:

1. an authoring trace from typed pass/program/shader declarations to virtual source, entry, permutation, parameter signature, compile input hash, code hash, manifest record, runtime lookup, pipeline key, and GPU event;
2. optimized DXIL and SPIR-V artifacts for the representative graphics and compute programs, with reflection/layout comparison and readable compiler diagnostics;
3. one deliberately broken binding contract and one deliberately broken compile/include, both rejected with portable source locations and replayable failure bundles;
4. cold and warm cook/cache results for serial, 1, 2, and N workers, including compile count, cache hits/misses, wall/CPU time, peak memory, cancellation, and identical-output checks;
5. cold and warm D3D12/Vulkan pipeline results, including creation time, hit/miss/untracked/too-late counts, first-use hitch evidence, and invalid-native-cache recovery;
6. paired PIX and RenderDoc captures for the same representative scene and settings, plus Nsight or RGA shader analysis where the finding requires source/IL/ISA counters;
7. exact engine commit, shader manifest/library hash, compiler backend and version, target, optimization/debug policy, GPU, driver, API, scene, camera, resolution, warm-up, capture frame, and run count;
8. editor adoption evidence: edit, changed-dependency selection, successful reload, failed replacement rollback, direct error navigation, one-job replay, and GPU-safe old-generation retirement;
9. a clean build/cook/run from documented commands and a recorded issue or minimal reproducer created by a second adopter;
10. before/after authored-string, registration, parameter-declaration, compile-job, unique-code, cooked-code/library-byte, cook-time, runtime-load, and pipeline-hitch counts so the simplification claim is measurable.
11. a generated support matrix proving backend/target/stage/program-kind/feature/policy status and a consumer report distinguishing registered, cooked, runtime-valid, selected, and captured programs/fallbacks;
12. shader-code readiness/lifetime evidence comparing eager and preload candidates, including only compression/eviction metrics that exist; if full RT is in scope, add state-object/pipeline, SBT layout/index/update/memory, trace dispatch, fallback, reload, and paired capture evidence.

The [performance diagnostics architecture](../Performance/Diagnostics/PerformanceDiagnosticsArchitecture.md) owns the shared measurement and capture infrastructure. This document owns the shader-specific identities and joins that make those captures traceable. Evidence records belong under the repository's evidence path selected by the acceptance workload; they must not be embedded here as claims that age with hardware, drivers, or compiler versions.

## Target Capability Requirements

The requirements below describe the complete end state and the rationale behind it. The [implementation contract](#implementation-contract) owns sequencing, exact clean-break boundaries, validation timing, and source-control rules; these numbered requirements are not independent migration steps.

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

Replace the mutable string catalog with frozen metadata emitted by typed declarations. A `ShaderTypeDesc` contains:

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

### 4. Unify Shader-Visible Parameters; Compose Pass Envelopes

Use one C++ schema as the authority for every shader-visible field:

```text
typed shader Parameters
      |
      +--> shader registration and root-parameter metadata
      +--> compile/reflection verification
      +--> runtime parameter instance and binding
      +--> persisted layout signature
```

For the common one-shader pass, this schema is also the pass parameter structure and drives frame-graph dependency declaration. A pass that has render-target attachments, copy/clear parameters, graph-only resources, or multiple shader invocations instead owns a pass envelope that embeds/composes the relevant shader schemas and declares its pass-only fields once. A shaderless pass owns only its graph parameters. No envelope may restate a shader-visible field independently.

The shader field description must preserve shader name, C++ field identity, resource kind/dimension, array count, access, stage visibility, semantic/resource domain, value size/alignment, and source declaration location. Composition must retain declaration provenance and produce one deterministic graph/binding view.

Remove parameter-count-only compatibility. Runtime accepts the same frozen metadata instance or the same strong structural layout signature; in validation builds it should also report the first differing field. Reflection validation remains mandatory on cache hits and compiler misses for every selected backend.

### 5. Generate Semantic Pass Labels and Typed Programs

- Add one pass trait/declaration mapping a pass type to its pass envelope (or direct shader parameter type), pipeline kind, and one or more typed shader programs as the operation requires.
- Generate the default diagnostic label from the declared pass type token and preserve an explicit per-instance label for mip, cascade, eye, phase, or view-specific scheduling.
- Introduce `TShaderProgram` composition for legal compute and graphics stage sets; ray-tracing program composition remains compiler-only until the separate RT delivery plan lands.
- Derive stage membership, program kind, feature union, layout compatibility, default binding-layout debug text, and the program composition record from typed composition; use a canonical catalog-emitted `ShaderProgramId`.
- Remove the 28 per-class `PassName` literals, `RendererShaderPackages.h`, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, and repeated runtime `PackageId` fields in the same ownership migration.
- Keep source path, entry point, and stage explicit on each shader type; these are not derivable from the pass.

### 6. Formalize Compile Requests, Jobs, and Results

The cooker should create one immutable `ShaderCompileRequest` per `(ShaderTypeId, PermutationId, Target)` after compile-policy filtering. It should contain every read-only input needed to reproduce compilation:

- virtual source path, entry point, stage, and preprocessed/include dependency information;
- target platform, shader model/profile, backend, backend version, and output format;
- ordered canonical defines, specialization values, binding remaps, optimization/debug/strip/warning policy;
- parameter metadata that affects generated compiler input, required symbols/exports, feature requirements, and generated shader ABI/tool provenance; validation-only layout signatures remain result/manifest contracts rather than poisoning cross-consumer compile reuse;
- stable debug group/type/permutation names that do not participate in semantic lookup.

`ShaderCompileInputHash` hashes the canonical compiler-affecting request and complete source content closure. `ShaderCompileJob` adds its logical scheduling identity, priority, cancellation state, preprocess output, compiler output, timings, diagnostics, dependencies, and cache status. Jobs with the same input hash share one in-flight or completed result even when several programs consume it. Do not call the full content hash `ShaderCompileJobKey`: in Unreal that name denotes the narrower logical type/platform/permutation key.

The current `ShaderCookPlanExecutor` and `TaskExecutor` are the starting coordinator. Replace the node executor with deterministic job deduplication, explicit priorities, bounded memory, cancellation propagation, and deterministic result merge. Keep one out-of-process cooker. Introduce persistent `ShaderCompileWorker` child processes only after a separate measured design proves compiler serialization, crash isolation, or memory recovery requires them.

### 7. Turn the Artifact Store into a Derived-Data Cache

Replace the interface-plus-single-implementation artifact seam with one concrete, disposable, content-addressed `ShaderDerivedDataCache` owner:

```text
request -> local derived-data lookup -> compile on miss -> atomic local publication
```

- local disk is the only required current store;
- a shared/read-only store is a later capability only after a real second consumer and publication policy are approved;
- cache entries are safe to delete and must always be reproducible from source plus toolchain;
- compiler/backend provenance, target, full compile input, parameter signature, and current cache contract affect the key; old disposable entries are misses, not inputs to version dispatch;
- corrupted or incompatible entries are quarantined/missed and regenerated, never trusted;
- cache lookup/publish statistics expose hit, miss, duplicate-in-flight, read/write bytes, and time saved;
- cooked runtime libraries never depend on the DDC being installed or writable.

Most importantly, remove program/package identity from `ShaderCompileInputHash` unless it actually changes compilation. Program composition and display names belong to manifest identity, not the reusable compiler-input hash.

### 8. Build a Global Shader Map and Generated Code Records

At cook completion, generate these independent outputs:

1. `GlobalShaderMap`: sorted mapping from `(ShaderTypeId, PermutationId, Target)` to `ShaderCodeHash`, parameter-layout signature, and required runtime metadata.
2. `ShaderProgramManifest`: sorted mapping from `ShaderProgramId` to the ordered shader-map entries/exports that form the program.
3. `CookedShaderLibrary`: validated bytecode records addressed by exact `ShaderCodeHash`, with adjacent current-contract runtime metadata and integrity coverage. Physical compression, chunking, and preload groups remain optional measured optimizations.
4. `ShaderProvenance`: editor/development-only mapping from hashes to source, entry, permutation, compiler, dependencies, symbols, and debug artifacts.

The clean break emits the shader map, program manifest, code library, and optional development provenance as the only current representation. It does not retain a per-program `.sparkshader` bridge. A later writer may change physical grouping without changing renderer authoring only when current catalog I/O/size evidence justifies the policy and the old owned representation is regenerated in place.

The cook must sort all records deterministically, compute content hashes after validation, stage the complete file set, and publish one generation atomically. A partial map/library generation is never visible.

### 9. Use Typed Runtime Lookup and Preserve Generation Safety

Runtime startup should:

```text
open current library -> validate manifest/map integrity -> create active generation
```

`TShaderRef<TShader>` should resolve the requested permutation through the active `GlobalShaderMap`; it should not derive a package ID from the source filename. RHI shader creation can remain lazy by code hash, while startup-critical shaders and selected programs can be preloaded. Missing required global shaders, incompatible targets, corrupt code, missing features, and parameter-signature mismatches are fatal at initial startup.

Development reload should keep the current strong behavior: materialize and validate a complete replacement generation, swap only after success, retain the previous generation on failure, and release the retired generation only when all captured submission tokens are complete. Runtime cache keys must include the active generation so an object from an old library cannot leak into the new one.

### 10. Make Changed-Shader Recompilation Dependency-Directed

Preprocessing already discovers the include closure. Persist it as virtual paths and build both directions:

```text
shader type -> source/include dependencies
changed virtual source -> affected shader types -> affected programs
```

`Apply Changed` should send the changed virtual paths to the cooker, invalidate only matching job/map entries, compile missing results, rebuild the complete published generation, and report selected/cache-hit/compiled counts. If dependency metadata is absent, corrupt, or rejected by the current tool contract, the operation fails with `Rebuild All` as the explicit next action; it does not silently broaden into a full cook.

The editor should continue to coalesce requests, run compilation outside the renderer process, reject stale publications, and keep the active generation after any failure. Do not mutate a live shader map one entry at a time.

### 11. Add Reproducible Compile Diagnostics

Every diagnostic must identify shader type, virtual source, entry point, stage, permutation values/ID, target, backend/version, logical job identity, compile input hash, and declaration location. On failure, or on explicit opt-in, the tool should write a bounded debug bundle containing:

- canonical compile request and command arguments;
- final preprocessed source with virtual `#line` paths;
- ordered defines and specialization values;
- dependency list and hashes;
- compiler stdout/stderr and structured diagnostics;
- bytecode/output hash when available;
- reflection and parameter-layout comparison when available;
- a single-job replay command/file usable without recooking the catalog.

Default policy should dump failures only; dumping every compile is opt-in because it creates many small files. Shader symbols and debug metadata should be configurable independently from runtime bytecode so shipping-symbol workflows do not force different runtime shader content where a backend permits separation. The provenance index must map runtime bytecode hash to the exact PDB, SPIR-V debug data, virtual source closure, and compile request used for PIX, RenderDoc, Nsight, or crash-dump inspection. An analysis cook should be able to emit paired DXIL/SPIR-V inspection artifacts without changing the release manifest identity except where the compiler necessarily changes the bytecode.

### 12. Separate PSO Precaching from Shader Cooking

Define a `RenderPipelineKey` from shader code hashes plus the complete API-visible state:

- graphics: vertex input/layout, topology, rasterization, depth/stencil, blend, multisampling, render-target/depth formats, and dynamic-state policy;
- compute: compute shader hash plus layout/specialization state required by the backend;
- ray tracing: libraries/exports, hit groups, recursion/payload/attribute limits, and backend-specific state.

Typed programs may enumerate known global PSO descriptors during startup or cook. Queue those descriptors for asynchronous creation before first use and allow loading screens to wait for required/high-priority requests. Record `Hit`, `Missed`, `Untracked`, `TooLate`, `Used`, `Precached`, creation duration, and hitch threshold evidence. A shader-only hit and a complete-PSO hit should be distinguishable.

Renderer owns descriptor enumeration, priority, wait/fallback policy, and telemetry because those are workload decisions. RHI owns native creation and backend cache integration. The portable baseline is complete canonical descriptors plus async prewarm and hit/miss/late evidence. Evaluate Vulkan `VkPipelineCache` and D3D12 cached-blob/pipeline-library support as separate capability paths and retain each only when cold/warm measurements justify its complexity. Newer Vulkan pipeline binaries and D3D12 advanced shader delivery remain later options. Any native cache rejection is a recoverable miss followed by fresh creation, not permission to accept incompatible data or terminate normal startup.

First use must have an explicit policy: wait outside interactive rendering, use an accepted fallback, or record a `TooLate`/`Missed` event before synchronous creation. Pipeline compilation must never be silently introduced inside pass Execute. The task/memory budget and any backend serialization must be measured before selecting the concurrency level.

Do not put render-target or blend/raster/depth state into shader package identity. Do not claim hitch-free rendering merely because bytecode was cooked.

### 13. Keep Physical Packaging an Internal Optimization

- Preserve one logical authoring model regardless of whether release data is written as per-program files, one global library, project/plugin libraries, or chunks.
- Measure startup I/O, file-open count, compression ratio, artifact size, patch delta, cache reuse, preload latency, and generation replacement cost.
- Choose physical grouping from that evidence and supported platform packaging constraints.
- Never expose physical library or chunk membership as a repetitive render-pass constant.

### 14. Make Conformance and Evidence Executable

Use the repository's existing validation surfaces plus temporary local harnesses where no current owner can falsify the invariant. The implementation plan does not authorize submitted test classes, fixtures, executables, source files, or CTest registration.

- Exercise virtual paths, include graphs, identity/key stability, permutation encoding, catalog freeze, parameter signatures, map/library integrity, and cache corruption through the narrowest existing tool command or a temporary removed-before-handoff harness.
- Enumerate every registered program and reject duplicate stages, incompatible layouts, unsupported targets/features, or missing sources before compilation.
- Cook and inspect representative and all-program DXIL/SPIR-V outputs through the existing ShaderCompiler/product validation route.
- Validate typed lookup, code corruption, wrong backend/layout, readiness, pipeline-cache recovery, reload rollback, stale generation, and delayed GPU completion through focused runtime smokes or temporary harnesses.
- Inject compile and ABI failures only long enough to prove portable diagnostics and replay bundles, then remove the injectors.
- Use PIX/RenderDoc/Nsight/RGA and cold/warm pipeline measurements only when the claim requires hardware evidence.

Tool unavailability and unsupported hardware remain explicit unavailable evidence. The all-program gate stays bounded by compile-policy-filtered supported catalog entries and never compiles speculative permutation space.

### 15. Define Code Loading, Residency, and Preload Policy

Extend the existing `RenderPassRuntimeCache` generation owner over the generated maps and records. It opens and validates generation indexes, resolves code by hash, and schedules only the bounded reads/decompression/native creation selected by the physical format. Do not introduce a second runtime shader-code service. Backend object policy is not universal: D3D12 pipeline creation consumes shader bytecode, while Vulkan shader modules may be destroyed after pipeline creation. Cache a reusable native shader/module object only when the backend exposes one and measurements prove value. Renderer supplies required-startup and predicted-program requests; RHI supplies native creation and observable memory where available.

Start with evidence, not an elaborate streamer. Measure eager indexed loading against startup-required plus predicted preload for the current 28 programs and record the actual cooked bytes for each target. The winning simple policy defines which of `Preload`, `IsReady`, and `Release` are meaningful; do not manufacture eviction or decompression machinery merely because larger engines have it. Lazy materialization remains outside Execute and records frame-critical use as late. Accounting covers the bytes, code records, native objects, pipelines, I/O/decompression/create latency, and high-water marks that actually exist. Active programs, pipelines, generations, and GPU submissions pin only resources whose backend lifetime requires it.

### 16. Make Backend, Target, Stage, and Feature Support Honest

Generate one conformance matrix from backend capability probes and executable evidence. For every language/backend/target/stage/program-kind/feature and compile-policy combination, report `Supported`, `Unsupported`, `CompilerOnly`, or `RuntimeValidated`. A backend must not silently ignore debug, optimization, warning, strip, specialization, or symbol policy. Unsupported explicit requests fail before scheduling; automatic selection can skip a target only with a reported reason and must fail if no required release target remains.

Treat geometry/hull/domain as schema/compiler-only until neutral pipeline descriptors, D3D12/Vulkan creation, registrations, and paired tests exist. Treat mesh/task as unsupported. Validate descriptor indexing and acceleration-structure device-address feature flags explicitly at runtime. Generate a consumer audit so registered/cooked alternatives are not described as fallbacks until a frame selection owner and test exercise them.

### 17. Add Full Ray-Tracing Pipelines Only as a Complete Vertical Slice

Keep current inline-ray-query compute programs separate from future RT programs. Preserve the cook schema and compiler-only RT fixture, but keep runtime rejection until one complete effect provides:

- typed raygen/miss/hit/intersection/callable registration and legal hit-group composition;
- explicit global/local parameter, payload, attribute, recursion, stack, and feature contracts;
- D3D12 state-object and Vulkan ray-pipeline creation with shader/group identifier retrieval;
- a backend-neutral SBT builder with aligned raygen, miss, hit, and callable regions;
- a documented instance/geometry/ray-type indexing formula and bounded local record data;
- RHI trace-rays command recording plus RDG resources, states, synchronization, and labels;
- pipeline/SBT preload, cache, generation, reload, and GPU-safe retirement ownership;
- a selected non-RT fallback and paired-backend execution, capture, corruption, reload, and performance evidence.

Do not enable one backend silently. If Vulkan compiler support or another required platform is unavailable, define an explicit platform-limited product contract and accepted fallback before implementation.

### 18. Provide One Trace Command from Declaration to GPU Evidence

Extend inspection around stable IDs rather than adding another package-centric tool. Given a shader type, program, code hash, pipeline key, map/library artifact, or captured marker identity, the tool should print the declaration location, virtual source closure, permutation, logical job identity, compile input hash, cache provenance, backend/version, reflection/layout signature, code record, program manifest, readiness/lifetime state, logical/native pipeline cache result, generation, pass consumers, and symbol/capture paths.

This trace is a diagnostic join, not another registry. It reads the same immutable metadata used by cooking and runtime. Machine-readable JSON and concise human text should be available so CI, editor diagnostics, PIX/RenderDoc/Nsight/RGP workflows, and incident reports share one provenance chain.

## Ownership and Dependency Boundaries

| Owner | Owns | Must not own |
| --- | --- | --- |
| Core | generic paths, hashing, files, processes, tasks, diagnostics | shader source policy, renderer shader types, RHI compilation |
| RHI public contract | shader stages/formats, backend-neutral cooked-record validation primitives, parameter metadata primitives, shader/pipeline descriptors, RHI handles | renderer pass catalog, project source discovery, compile orchestration, runtime map policy |
| RHI backend | create/destroy backend shader and pipeline objects from validated data | source preprocessing, renderer program policy, package naming |
| Renderer | concrete global shader types, typed programs, parameter structs, pass traits, shader-map use, preload/PSO declarations | filesystem source resolution, compiler backend selection, editor process UI |
| ShaderCompiler tool | virtual source mounts, preprocessing/dependencies, compile requests/jobs, backend invocation, DDC, reflection validation, shader map/library cook and atomic publication | renderer scheduling, live RHI ownership, editor workflow state |
| Application | changed-path watching, typed operation dispatch, publication notification, and activation request routing | dependency resolution, compilation algorithms, catalog construction, shader-map mutation, editor presentation, direct RHI shader construction |
| Editor | immutable shader catalog/operation presentation, semantic selection, source navigation, cancellation intent, and contextual diagnostics | artifact-directory scanning, dependency calculation, compiler invocation, publication, shader-map mutation, cache policy, direct RHI shader construction |

The generic shader authoring primitives may remain in RHI public code if they stay renderer-agnostic. Concrete registrations and program composition stay Renderer-owned. The runtime shader map may be implemented in Renderer over generic cooked/RHI primitives; it must not cause RHI to depend on Renderer. The standalone compiler may link the registration object catalog, but no runtime module may depend on the tool.

## Preserve, Improve, Delete, and Defer

| Disposition | Item |
| --- | --- |
| Preserve | explicit source/entry/stage registration; DXIL and SPIR-V targets; DXC/Slang backend boundary; recursive preprocessing; include-closure/options hashing; reflection extraction; parameter verification on hit and miss; bounded task execution; atomic local cache publication behavior; out-of-process cook; transactional publication; validated generation swap; GPU-token retirement; strict runtime rejection of RT libraries until execution is complete |
| Improve | physical paths into virtual paths; mutable registry into validated frozen metadata; free-form variants into typed permutations; cook nodes into input-hash-deduplicated prioritized jobs; the current local artifact behavior into one focused `ShaderDerivedDataCache`; package registry into the actual runtime manifest; change polling into reverse-dependency selection; layout count check into a strong structural signature; diagnostics into replayable per-job bundles; per-pass package load into explicit code readiness/lifetime policy; silent backend-policy differences into a generated conformance matrix; program feature checks into complete capability validation |
| Delete after migration | `RendererShaderPackages.h`; `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`; basename-derived package fallback; repeated pass/package/layout/pipeline strings; duplicated shader-visible fields across pass and shader declarations; silent duplicate suppression; count-only layout acceptance; direct runtime path construction from author package IDs |
| Defer until measured | material shaders; vertex factories; remote/distributed compilation; persistent worker-process pool; cloud DDC; complex plugin loading phases; library chunk/patch system; automatic PSO discovery for arbitrary content; graphics pipeline libraries; Vulkan shader objects/pipeline binaries; D3D12 partial programs; work graphs; mesh/task shaders |
| Require one complete future slice | RT state objects/pipelines, native identifiers/group handles, SBT build/indexing, trace-rays commands, RDG integration, preload/cache/lifetime, selected fallback, and paired execution evidence |

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
- a compiler backend that silently ignores a requested release/debug/optimization/warning/symbol policy
- a registered program or frame branch whose stage/program-kind/feature combination is not executable on the selected runtime backend
- an active shader-map/program record whose descriptor-indexing or acceleration-structure device-address feature is not explicitly supported by the active capability contract
- an RT program whose export/hit-group associations, payload/attribute contracts, local/global layouts, recursion/stack policy, or native pipeline metadata is inconsistent
- an SBT whose native identifier belongs to another pipeline generation or whose region, stride, alignment, record, instance/geometry/ray-type index, or referenced data lifetime is invalid

Errors must name the logical program, shader type, virtual source, entry point, stage, readable permutation, target, backend/version, logical job identity and compile input hash where relevant, plus both conflicting declaration locations. Development recook errors retain the previous active generation; initial startup has no safe previous generation and fails explicitly.

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

## Implementation Contract

The migration is an ordered clean break, not a menu. Each phase below is one manually reviewed changelist-sized checkpoint on `master`; none may leave old and new authorities active together. The typed catalog, compile products, cooked generation, and runtime owner evolve in place. A difficult remaining consumer blocks that phase rather than justifying an adapter, alias, fallback, or cleanup ticket.

### Common rules for every phase

- Work directly in the unstaged `master` worktree. Do not create or switch branches and do not stage, commit, push, or submit. The user owns every source-control action.
- Apply [Integration Style Guide](../../Engineering/Standards/IntegrationStyleGuide.md), [Change Process](../../Engineering/Standards/ChangeProcess.md), and every applicable subject standard from the [standards map](../../Engineering/Standards/README.md#standards-map). This document controls shader-specific target vocabulary and phase ordering; binding standards control general implementation behavior.
- Phases 0 through 7 do not configure, build, compile shaders, cook, launch, capture, or run tests. They use exact searches, bounded owner/consumer reads, source/build/document reconciliation, no-write formatting when available, local-link validation, and `git diff --check`. Phase 8 runs executable validation once against the complete candidate.
- Preserve one `SparkleTasks` runtime, one out-of-process shader cooker, one transactional publication route, one active renderer shader generation, and `RhiSubmissionToken` retirement. Do not add a shader thread pool, live-object patch path, second registry, second cache authority, or device-idle reload.
- Preserve the current renderer boundary: `BuildRenderFrameGraph` and `FrameGraphBuilder` may borrow already prepared immutable shader/pipeline state; `PassCommandContext` remains semantic-free and pass Execute performs no file I/O, compilation, package/library lookup, layout creation, pipeline creation, or hidden resource discovery.
- Use owner/resource queries instead of mirrored validity booleans. Do not add one-field carrier structs, broad `*Context`/`*Services`/`*Resources` bags, renderer reach-through, or copied frame/view/scene policy.
- No permanent migration diagnostics, per-job log stream, cache browser, report generator, test scaffold, feature flag, or dual UI. Keep one concise owner-local error and bounded operation status; temporary proof machinery remains outside the submitted diff and is removed before handoff.
- Update definitions, consumers, filenames, includes, CMake/source groups, CLI/help/autocomplete, editor models, diagnostics vocabulary, current documentation, generated metadata, and disposable cooked/cache artifacts in the phase that owns their replacement.
- Preserve unrelated dirty work. Phase 0 records the exact path-level exclusion list; later phases recheck it before editing.

### Frozen target vocabulary and navigation

| Responsibility | Target vocabulary | Canonical owner/path |
| --- | --- | --- |
| virtual source identity | `ShaderSourceMountTable`, canonical `/Engine`, `/Project`, `/Plugin/<Name>` paths | ShaderCompiler `Private/Source`; renderer declarations store only virtual paths |
| one entry-point specialization | `ShaderTypeDesc`, `ShaderTypeId`, typed permutation domain | generic authoring primitive in RHI public; concrete declarations in `Engine/Renderer/ShaderRegistrations` |
| legal stage composition | `ShaderProgramDesc`, `ShaderProgramId`, `TShaderProgram` | Renderer shader registrations/catalog |
| one shader-visible ABI | nested `Parameters` alias naming one authoritative typed schema | concrete Renderer shader declaration; pass envelopes compose graph-only fields without copying shader-visible fields |
| immutable authoring catalog | `GlobalShaderCatalog` | built once from concrete registrations, validated, sorted, and frozen before the first cook/runtime/editor query |
| compile work and reusable identity | `ShaderCompileRequest`, `ShaderCompileJob`, `ShaderCompileInputHash`, `ShaderCompileResult` | ShaderCompiler `Private/Compilation` |
| disposable compiled-derived data | `ShaderDerivedDataCache` | ShaderCompiler `Private/Cache`; local writable store first, no cloud/worker-farm framework |
| logical cooked lookup | `GlobalShaderMap` | generated by ShaderCompiler and opened read-only by runtime |
| typed runtime lookup | `TShaderRef<TShader>` | Renderer resolves through the active `GlobalShaderMap`; RHI receives validated code/descriptor input only |
| program composition and code delivery | `ShaderProgramManifest`, `ShaderCodeRecord`, `ShaderCodeHash`, `CookedShaderLibrary` | ShaderCompiler cook output; neutral read/validation records in RHI public |
| renderer generation and pipeline lifetime | existing `RenderPassRuntimeCache`, with package vocabulary removed | Renderer `Private/Pipeline`; active/replacement/retired generations remain one owner |
| graph diagnostic identity | `RenderPassLabel` generated from the typed pass declaration, with an explicit instance label only at scheduling | Renderer pass/frame-graph setup |
| frontend intent | `Apply Changed`, expert `Rebuild All`, typed program/shader inspection | Application `ShaderRecook` orchestration and Editor Shader Tools presentation |

Do not introduce `ShaderSystem`, `ShaderManager`, `ShaderServices`, `ShaderContext`, `ShaderData2`, `NewShader*`, or Unreal prefixes. Do not keep `PackageId` as a synonym for `ShaderProgramId`. Physical files and hashes are generated delivery details; a program remains a logical typed stage composition.

### Phase 0 - Freeze the current contract, inventory, and evidence floor

#### Implementation prompt

> Implement Phase 0 as one documentation and inventory CL directly in the unstaged `master` worktree. Do not create or switch branches and do not stage, commit, push, or submit. Re-run the complete shader authoring, source/include, registration, program/package, parameter, compile, cache, cook, publication, runtime, frame-graph, frontend, build-membership, generated-artifact, diagnostic-label, and documentation searches. Freeze the exact target vocabulary above, assign every current definition and consumer to one later phase, record baseline provenance and unrelated dirty exclusions, and reconcile stale documentation. Do not edit runtime/tool source and do not configure, build, compile shaders, cook, launch, capture, or run tests.

#### Phase-specific references

- [Documentation authority map](../../README.md)
- [Whole Repository Architecture Map](../WholeRepositoryMap.md)
- [SparkleEngine Code Review](../../Engineering/CodeReview.md)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)

#### Required work

- Record exact `rg` counts and owner/consumer paths for the five identity layers: pass label, shader type, shader program, compile input, and cooked/runtime product.
- Inventory every field and consumer of `ShaderRegistrationDesc`, `ShaderPackageCookSettings`, `ShaderCookPackageDesc`, `CookNode`, `ShaderCacheKey`, cooked package records, `RenderPassDefinition`, `RenderPassRuntimeCache`, `ShaderRecookRequest`, registered-shader editor rows, and generated/cooked publication files.
- Classify every retained field as authoring metadata, compile input, derived-cache identity, cooked manifest/code metadata, renderer generation state, pipeline description, diagnostic presentation, or deletion.
- Record the mutable/lifetime owner, producer, consumers, publication/retirement behavior, copy justification, target name/path, and deletion phase for every retained value.
- Record the exact revision and provenance of cold/warm cook, cache, reload, D3D12/Vulkan runtime, capture, and pipeline-readiness evidence. Mark the corresponding Phase 8 claim blocked where no valid baseline exists.
- Record all unrelated dirty paths as exclusions. Do not silently absorb or restore them.

#### Positive guardrails

- Use exact `rg`, `rg --files`, CMake membership, shader registration, generated metadata, cooked artifact, include closure, CLI, and editor-consumer evidence.
- Keep the inventory in this implementation document or the CL description, not a new runtime reporting system.
- Assign every rejected symbol/path to exactly one deletion phase.

#### Negative guardrails

- No runtime/tool edits, scaffolding, target types, file moves, generated code, or migration utilities.
- Do not claim the target is implemented or manufacture a baseline after source changes begin.
- Do not defer any owned old item to generic cleanup later.

#### Acceptance criteria

- Every current definition, material field, producer, consumer, generated artifact, and UI/CLI spelling has one target or deletion decision and one phase.
- Baseline evidence has exact revision/environment provenance or its Phase 8 comparative claim is explicitly blocked.
- Local links resolve, the scoped diff contains documentation only, and `git diff --check` passes.

#### CL boundary

Suggested title: `Shaders: freeze atomic authoring and cooked-program migration contract`.

### Phase 1 - Establish canonical virtual sources and semantic shader navigation

#### Implementation prompt

> Implement Phase 1 as one source-identity and shader-layout CL directly in the unstaged `master` worktree. Introduce canonical virtual source mounts, convert every registered source and include to virtual identity, move the broad `Passes/Deferred` shader bucket into semantic owners matching Renderer pass navigation, update every consumer, and delete the old physical-path resolution and old directory. Do not add mount compatibility, do not retain old include spellings, and do not configure, build, compile shaders, cook, launch, or run tests.

#### Phase-specific references

- [Repository Structure and Ownership](../../Engineering/Standards/RepositoryStructureAndOwnership.md)
- [Coding Style](../../Engineering/Standards/CodingStyle.md)
- [Editor and Tools](../../Engineering/Standards/EditorAndTools.md)
- [Epic virtual shader source precedent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/GetShaderSourceFilePath)

#### Required work

- Add `ShaderSourceMountTable` beneath ShaderCompiler source ownership with deterministic `/Engine`, `/Project`, and real `/Plugin/<Name>` mounts; reject unknown, overlapping, escaping, absolute-authored, and case-colliding paths.
- Store virtual paths in shader registrations, include graphs, diagnostics, dependency records, replay bundles, compile hashes, and manifests. Resolve to physical files once inside the source owner.
- Move `Engine/Assets/Shaders/Passes/Deferred` files into `Passes/GBuffer`, `Passes/Lighting/Direct`, `Passes/Lighting/Shadows`, `Passes/Lighting/Sky`, and `Passes/Debug` according to their actual semantic owner; update all C++ registrations and HLSL/HLSLI includes and remove the empty directory.
- Remove project-first silent shadowing, physical checkout paths from persistent identities, and authored absolute include support.
- Reconcile ShaderCompiler file placement so source mounting, include resolution, preprocessing, and dependency discovery live under one predictable `Private/Source` owner without creating a catch-all helper folder.

#### Positive guardrails

- One virtual identity resolves to one physical file and diagnostics retain both virtual identity and an actionable local location.
- Source moves alter compile identity but never pass labels or program identity.
- Keep compiler backend selection and renderer feature policy outside the source resolver.

#### Negative guardrails

- No old/new mount search order, fallback to basenames, absolute-path escape hatch, alias mount, or duplicate source tree.
- Do not encode `Deferred` in renderer-wide source ownership; technique terms may remain only where the shader itself specifically implements that technique.

#### Acceptance criteria

- Exact searches return zero authored `Passes/Deferred/` paths, zero files in the old directory, and zero persistent physical checkout paths in compile/cache/manifests.
- Same-basename sources under different virtual directories remain distinct; collisions, traversal, invalid mounts, and source shadowing are deterministic errors.
- Includes, registrations, compiler resolution, diagnostics, documentation, and build membership use only canonical virtual paths.

#### CL boundary

Suggested title: `Shaders: establish virtual source identity and semantic layout`.

### Phase 2 - Replace package authoring with one typed shader and program catalog

#### Implementation prompt

> Implement Phase 2 as one authoring-catalog and program-identity CL directly in the unstaged `master` worktree. Replace mutable package-led shader registration with one validated frozen `GlobalShaderCatalog`, typed shader/program declarations, typed program references in render-pass definitions, and generated default pass labels. Update every Renderer, RHI, ShaderCompiler, Application, Editor, CLI, CMake, diagnostic, and documentation consumer, then delete every manual package constant, explicit-package registration macro, basename fallback, repeated default label/debug string, and package-generation spelling owned by this phase. Do not add conversion or dual catalogs and do not configure, build, compile shaders, cook, launch, or run tests.

#### Phase-specific references

- [Naming and Vocabulary](../../Engineering/Standards/NamingAndVocabulary.md)
- [Data-Oriented Design](../../Engineering/Standards/DataOrientedDesign.md)
- [Epic global shader type precedent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderType)
- [Epic shader pipeline type precedent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderPipelineType)

#### Required work

- Replace `ShaderRegistrationDesc`/mutable `GlobalShaderRegistry` behavior with collision-checked `ShaderTypeDesc` records and an explicitly frozen, sorted `GlobalShaderCatalog` that records declaration location and rejects duplicate/late declarations.
- Add typed `ShaderProgramDesc`, `ShaderProgramId`, and `TShaderProgram` composition for compute and graphics programs. Preserve compiler-only ray-tracing metadata without advertising runtime execution.
- Derive stable serialized IDs from explicit declaration tokens/catalog data, never RTTI/compiler spellings, source basename, pass label, or physical package filename.
- Make `RenderPassDefinition` reference its typed shader program and fixed-function state. Generate `RenderPassLabel` and binding/pipeline debug names from the typed pass declaration; retain only explicit per-instance scheduling labels such as mip/cascade/phase.
- Rename package-led generation vocabulary to shader generation across `RenderPassRuntimeCache`, `RenderFrameIdentity`, `RenderViewState`, Renderer/Application/Editor APIs, captures, diagnostics, and UI.
- Delete `RendererShaderPackages.h`, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, `BuildShaderPackageIdFromSourcePath`, package/binding-layout fallback derivation, authored `PackageId`/`PackageName` fields, and the 28 per-pass default `PassName` literals.

#### Positive guardrails

- Shader type, source/entry/stage, program, pass, compile identity, and cooked identity remain distinct.
- One catalog is read by cooker, runtime, CLI, and editor; consumers do not copy it into private registries.
- `GBuffer` remains one graphics program composed from its vertex and pixel shader types.

#### Negative guardrails

- No package-to-program adapter, alias macro, legacy registry, string lookup facade, compatibility overload, parallel generated catalog, or mixed package/program CLI.
- Do not use the shader filename as pass/program identity or make graph infrastructure own renderer program policy.
- Do not add an empty permutation framework beyond the typed bounded domain required by current programs.

#### Acceptance criteria

- Exact searches return zero `RendererShaderPackages`, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, `BuildShaderPackageIdFromSourcePath`, authored package IDs/names, old package-generation APIs, and default per-pass `PassName` fields.
- There is one catalog freeze route; duplicate, conflicting, invalid, and late declarations have one owner-local failure path with both declaration locations.
- All 29 current stage declarations belong to one typed program and every live pass references that program without repeated identity strings.
- CLI/help/autocomplete, editor rows, captures, diagnostics, CMake, and current documentation use shader/program/generation vocabulary only.

#### CL boundary

Suggested title: `Shaders: replace package authoring with typed programs`.

### Phase 3 - Make shader-visible parameters one authoritative ABI

#### Implementation prompt

> Implement Phase 3 as one parameter-authority and binding-validation CL directly in the unstaged `master` worktree. Make each shader-visible field exist once under the Sparkle-native nested `Parameters` contract, derive reflection verification, graph usage, binding layout, structural signature, and runtime binding from that authority, and compose graph-only pass envelopes without copying shader fields. Update every shader registration and pass together and delete the Unreal-prefixed `FParameters` spelling, duplicate pass declarations, count-only compatibility, generic accessors, and forwarding metadata. Do not retain old/new parameter paths and do not configure, build, compile shaders, cook, launch, or run tests.

#### Phase-specific references

- [Coding Style one-field rule](../../Engineering/Standards/CodingStyle.md#one-field-types)
- [Data-Oriented Design single-truth and copy budget](../../Engineering/Standards/DataOrientedDesign.md#single-truth-and-copy-budget)
- [Renderer/RHI boundary](../RendererRhiBoundary.md)
- [Epic shader parameter metadata precedent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderParametersMetadata)
- [Epic RDG pass parameters](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)

#### Required work

- Freeze the representation for a direct one-shader pass, a graphics multi-stage pass (`GBuffer`), a pass with graph-only attachments, and a shaderless copy/transfer pass before mechanical migration.
- Rename the nested shader contract from `FParameters` to `Parameters` across the authoring macro/API and every concrete registration; do not introduce an alias retaining the old spelling.
- Extend the existing typed parameter owner so one declaration supplies shader name/type/array/visibility, graph read/write semantics where applicable, reflection contract, structural signature, and binding offsets.
- Reuse the shader parameter schema directly for one-to-one passes; compose it by reference/typed embedding into a pass envelope only when graph-only targets, copy parameters, or several shader invocations require it.
- Make stage/program layout compatibility compare the full canonical structural signature: names, kinds, dimensions, arrays, visibility, sizes/alignments, uniforms, samplers, and stage composition.
- Update all passes and shader declarations in the same CL, including `GBuffer` draw parameters and binding overrides, without adding a generic context or service bag.
- Delete duplicated shader-visible fields, independent macro parameter declarations, count-only acceptance, nullable fallback getters, and comments that endorse parallel schemas.

#### Positive guardrails

- Graph dependencies remain declared from the pass envelope and execution sees only the typed bound values it consumes.
- Validation occurs once at catalog/cook/runtime owners and fails before recording.
- ABI fields preserve identical C++/HLSL names unless one explicit toolchain mapping owns the exception.

#### Negative guardrails

- No reflection-name runtime discovery, slot convention, copied parameter mirror, `ParameterContext`, broad owner pointer, or one-field wrapper kept for naming symmetry.
- No compatibility overload accepting the old parameter type and no per-pass duplicate validation/logging.

#### Acceptance criteria

- Every shader-visible field has exactly one owned declaration and one generated structural identity.
- Exact searches find zero runtime/build uses of `FParameters`, no count-only layout acceptance, and no pass/shader duplicate declaration pairs.
- Field reorder/name/kind/visibility/array/size mismatches are rejected by the narrow structural validator before pipeline creation.
- `PassCommandContext` remains command/resource/diagnostic infrastructure only and every pass records from explicit declared inputs.

#### CL boundary

Suggested title: `Shaders: unify pass parameters and shader ABI authority`.

### Phase 4 - Replace cook nodes with reusable compile jobs, DDC, and dependency-directed recook

#### Implementation prompt

> Implement Phase 4 as one compiler-job, derived-data, dependency, and replay-diagnostics CL directly in the unstaged `master` worktree. Replace package-shaped cook nodes and cache keys with immutable compile requests/jobs/results keyed only by compiler-affecting input; persist the virtual include dependency graph; make Changed recook select the exact reverse dependency closure; and produce bounded replay artifacts for failures. Preserve the single out-of-process cooker and `SparkleTasks`. Delete all old node/cache identities and full-catalog Changed behavior. Do not add a worker pool or configure, build, compile shaders, cook, launch, or run tests.

#### Phase-specific references

- [Concurrency](../../Engineering/Standards/Concurrency.md)
- [Editor and Tools](../../Engineering/Standards/EditorAndTools.md)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Epic shader compile job precedent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompileJob)
- [Epic Derived Data Cache](https://dev.epicgames.com/documentation/en-us/unreal-engine/derived-data-cache)

#### Required work

- Introduce immutable `ShaderCompileRequest`, `ShaderCompileJob`, `ShaderCompileInputHash`, and `ShaderCompileResult`; keep scheduling identity distinct from the full content hash and from program/artifact identity.
- Hash virtual source identity and the complete transitive content closure, entry, stage, typed permutation/environment, target, backend/version, policy, and compiler-affecting generated ABI. Exclude program/pass/display/package text that cannot alter compiler output.
- Replace `CookNode`, `ShaderCookNodeBuilder`, `ShaderCookNodeExecutor`, `ShaderCacheKey`, and package-led compile options with the new job path in one change.
- Replace the interface-plus-single-local-implementation artifact seam with one focused `ShaderDerivedDataCache` owner unless a second current store is proved. Validate integrity, quarantine corrupt disposable entries as misses, and publish writes atomically.
- Persist forward and reverse virtual-source dependencies. Change `ShaderSourceChangeTracker` from a boolean scan result to a bounded set of changed virtual paths and make `Changed` cook only the affected shader types/programs.
- Use one `TaskExecutor` graph with deterministic job ordering, in-flight duplicate fan-out, bounded parallel compiler sessions/memory, cancellation settlement, and no worker waits. Do not add persistent worker processes without measured need.
- Emit one bounded replay bundle for failed jobs containing virtual dependency closure, preprocessed source where available, request, backend/version, arguments, and one actionable root diagnostic; successful analysis artifacts remain opt-in.

#### Positive guardrails

- Identical compiler-affecting requests share one job/result regardless of program consumers.
- Cancellation/failure publishes no partial generation and leaves the previous accepted output intact.
- Application submits intent/changed paths; compiler owners select jobs and cache policy.

#### Negative guardrails

- No shader-only thread pool, `std::async`, detached process per shader, cache service locator, retry loop, permanent per-job logging, or full-catalog fallback for valid dependency data.
- No cache schema/version compatibility reader; disposable old entries are deleted or treated as misses.

#### Acceptance criteria

- Exact searches return zero old cook-node/cache-key definitions and uses.
- Moving the checkout without changing virtual paths or bytes preserves `ShaderCompileInputHash`; changing any compiler-affecting input invalidates it.
- Duplicate fan-out compiles once; corrupt cache recovery is a miss; cancellation settles every job and publishes nothing partial.
- Changed include/source selection reaches every dependent program and no unrelated program under a valid dependency graph.

#### CL boundary

Suggested title: `Shaders: establish compile jobs and dependency-directed derived data`.

### Phase 5 - Publish generated shader maps and code libraries through one runtime generation

#### Implementation prompt

> Implement Phase 5 as one cooked-map, code-library, lookup, and generation-lifetime CL directly in the unstaged `master` worktree. Replace per-program cooked package authority with generated `GlobalShaderMap`, `ShaderProgramManifest`, and content-addressed `CookedShaderLibrary` records; make runtime lookup manifest-driven and typed; preserve `RenderPassRuntimeCache` as the sole active/replacement/retired generation owner; and delete every old package file, reader/cache/type/path computation, schema version, and package-centric CLI/editor consumer. Do not keep a package adapter and do not configure, build, compile shaders, cook, launch, or run tests.

#### Phase-specific references

- [Integration Style Guide clean-break policy](../../Engineering/Standards/IntegrationStyleGuide.md#current-clean-break-policy)
- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md)
- [Renderer/RHI boundary](../RendererRhiBoundary.md)
- [Epic shader map resource precedent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderMapResource)
- [Epic shader code library precedent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCodeLibrary)

#### Required work

- Generate one deterministic current representation containing `GlobalShaderMap` logical entries, `ShaderProgramManifest` ordered stage references, exact `ShaderCodeRecord` entries keyed by `ShaderCodeHash`, reflection/layout signatures, feature requirements, compiler provenance, and publication integrity.
- Deduplicate exact validated code bytes without conflating incompatible metadata. Keep readable names adjacent for diagnostics but out of content identity.
- Open and validate generation indexes once. Resolve typed shader/program references through the map/manifest, never through source basename, authored strings, directory scans, or computed artifact paths.
- Replace old cooked package readers/writers/cache/identity/contracts and `.sparkshader` per-program output atomically. Delete obsolete cooked/cache/generated output and regenerate only in Phase 8.
- Keep generic cooked records and validation in RHI public, tool-side generation in ShaderCompiler, renderer program policy in Renderer, and native object creation in backend-private RHI.
- Preserve replacement validation before activation, previous-generation rollback on failure, shader generation capture in `RenderFrameIdentity`, view-history invalidation, and all-queue token retirement.
- Remove internal content/schema version dispatch. Old owned artifacts fail or are deleted and regenerated; compiler/tool/backend provenance versions remain explicit evidence.

#### Positive guardrails

- One immutable generation owns map, manifest, library, prepared layouts/pipelines, and retirement reachability.
- Physical grouping remains generated policy; do not force compression/chunking/streaming without measured need.
- Eager index validation and simple code retention are acceptable for the current catalog.

#### Negative guardrails

- No package-to-program converter, dual file emission, old reader, upgrade path, alias ID, runtime directory scan, live record patch, or second generation counter.
- No cloud DDC, chunk patcher, streaming framework, or LRU unless current measured catalog size/I/O requires it.

#### Acceptance criteria

- Exact runtime/tool/build searches return zero `CookedShaderPackage*`, `LoadedShaderPackage`, package cache/identity/path computation, `.sparkshader` writer/reader, and old schema-version dispatch owned by the replaced representation.
- Every catalog shader/program resolves through one generated map/manifest and every referenced code hash exists exactly once in the opened library.
- Invalid replacement generation leaves the current generation active; retired generations remain until all recorded submission tokens complete.
- RHI remains Renderer-independent and `architecture_boundary_check` has no new source-level violations.

#### CL boundary

Suggested title: `Shaders: replace cooked packages with shader maps and code libraries`.

### Phase 6 - Make pipeline preparation explicit and remove first-use creation from graph building

#### Implementation prompt

> Implement Phase 6 as one renderer pipeline-readiness CL directly in the unstaged `master` worktree. Extend the existing `RenderPassRuntimeCache` generation owner so render-graph construction declares the exact required typed programs and complete pipeline descriptions, preparation occurs before graph publication/interaction, and graph setup only borrows ready immutable runtimes. Add canonical complete pipeline keys and bounded readiness state without a second cache or frontend policy leak. Delete lazy first-use materialization from `FrameGraphBuilder`. Do not enable native driver caches without evidence and do not configure, build, compile shaders, cook, launch, or run tests.

#### Phase-specific references

- [Repository ownership orchestrator/capability boundary](../../Engineering/Standards/RepositoryStructureAndOwnership.md#mandatory-orchestratorimplementor-boundary)
- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md)
- [Epic PSO precaching](https://dev.epicgames.com/documentation/en-us/unreal-engine/pso-precaching-for-unreal-engine)
- [Microsoft D3D12 pipeline state](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)
- [Khronos Vulkan pipeline cache](https://docs.vulkan.org/guide/latest/pipeline_cache.html)

#### Required work

- Have `BuildRenderFrameGraph`/feature assembly declare the typed pass-program and fixed-function pipeline descriptions required by the selected topology.
- Build a canonical renderer pipeline key from program/code hashes, layout signature, specialization, vertex layout, raster/depth/stencil/blend state, target formats, sample state, topology, and every API-visible field.
- Prepare required startup/global pipelines before interaction and publish the graph only after required runtimes are ready. Optional pipelines need one explicit omission/fallback policy owned by the feature.
- Make `FrameGraphBuilder::Draw`, `Dispatch`, and `DispatchAsync` borrow prepared pass runtimes; delete their lazy package/layout/pipeline creation calls.
- Keep pipeline creation out of pass Execute and `PassCommandContext`; backend owners create native objects from the complete neutral descriptions.
- Record bounded existing-profiler/capture evidence for prepared hit, missed/untracked use, and too-late preparation without permanent per-frame logs or a new dashboard.
- Admit Vulkan pipeline cache or D3D12 cached/library paths only as independently capability-gated optimizations after compatible-key validation and measured cold/warm benefit.

#### Positive guardrails

- `RenderPassRuntimeCache` remains the one persistent generation-keyed cache named by the binding naming standard.
- High-level graph construction exposes required passes; pipeline mechanics remain in the focused runtime capability.
- Pipeline readiness is distinct from graph resource production and shader-code availability.

#### Negative guardrails

- No `PipelineManager`, second `RenderPipelineCache`, pass-owned mutable cache, backend branch in renderer policy, synchronous creation in Execute, or hidden graph-builder fallback.
- No driver-cache binary treated as source authority and no unconditional native cache path.

#### Acceptance criteria

- Exact searches find no pass-runtime materialization or pipeline creation in `FrameGraphBuilder` or pass Execute.
- Every supported graph topology declares its required typed runtimes and cannot publish with a missing required pipeline.
- Canonical keys distinguish every materially different complete pipeline description and reuse identical descriptions within one generation.
- Startup/graph-build timing, pipeline creation counts, and late-use evidence have an explicit Phase 8 measurement plan.

#### CL boundary

Suggested title: `Renderer: prepare shader pipelines before graph publication`.

### Phase 7 - Deliver one intent-first shader workflow and provenance trace

#### Implementation prompt

> Implement Phase 7 as one Application/Editor shader-workflow CL directly in the unstaged `master` worktree. Replace package-oriented recook/reload controls, artifact-directory scanning, and the ten-column internal-record UI with one semantic `Apply Changed` workflow, one immutable operation/catalog model, automatic activation after backend validation, contextual expert inspection, and a provenance trace over the same authoritative metadata. Update console/CLI help and completion, then delete package targets, manual normal-path reload, duplicate status formatting, and old UI/model fields. Keep dependency selection, cooking, publication validation, runtime replacement, and retirement behind their existing tool/renderer owners. Do not add a second panel, cache browser, log stream, or configure, build, compile shaders, cook, launch, or run tests.

#### Phase-specific references

- [Editor and Tools intent-first workflows](../../Engineering/Standards/EditorAndTools.md#intent-first-frontend-workflows)
- [Validation logging and instrumentation](../../Engineering/Standards/ValidationPerformanceAndEvidence.md#logging)
- [Epic Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [PIX shader PDB resolution](https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/)

#### Required work

- Make the normal command/UI action `Apply Changed`: Editor submits intent; Application snapshots changed virtual paths and routes one typed request; ShaderCompiler derives affected programs/targets/capabilities/cache use and performs cook/publication; Renderer validates and activates one replacement generation; the operation settles once.
- Keep expert `Rebuild All` plus typed shader/program targeting in Advanced/CLI only. Remove package targeting and manual reload from the normal workflow because successful publication activates automatically.
- Replace `ShaderRecookRequestType::PackageId`, `PackageId` editor rows, package autocomplete/help, and artifact-directory scans with typed shader/program identities and immutable operation/catalog results supplied by their owner.
- Present one operation state and one concise result. Failure leads with one source-located root cause, next action, and confirmation that the previous generation remains active; raw command, hashes, manifest, reflection, disassembly, replay artifacts, backend policy, and cache details remain contextual.
- Add one trace operation that joins shader/program identity, virtual dependency closure, compile request/hash, cache provenance, code hash, manifest entry, shader generation, prepared pipeline key/readiness, pass consumers, and external symbol/capture locations. It reads existing immutable metadata and owns no registry/cache.
- Preserve selection when opening from a compile error, program/pass row, GPU marker, or pipeline event.
- Remove duplicated coordinator/console/panel status formatting and repeated lifecycle logs; keep one bounded operation result through `EditorOperationService`.

#### Positive guardrails

- Application routes request and accepted-publication notifications without reproducing dependency or runtime policy; ShaderCompiler owns dependency selection, compile/cook, and transactional publication; Renderer owns validation, activation, runtime generation, and retirement; Editor owns presentation only.
- Primary UI columns remain semantic and bounded: program/shader, stage where useful, virtual source, active/readiness state, and pass consumers.
- Advanced overrides are typed deviations from a named preset, validated before launch, resettable, and recorded in evidence.

#### Negative guardrails

- No UI access to compiler sessions, cache directories, mutable renderer caches, RHI objects, task executor, publication files, or artifact filesystem scans.
- No toast/dialog per job, per-frame polling logs, second shader operation runtime, or package compatibility command.

#### Acceptance criteria

- The normal workflow exposes one dominant `Apply Changed` action and never asks for package/layout/hash/backend/cache mechanics.
- Exact searches return zero package-target request/UI/help/autocomplete/model fields and zero editor artifact-directory scanning.
- Success activates exactly one validated generation; failure/cancellation settles once and preserves the previous active generation.
- Trace output can start from shader type, program, code hash, pipeline key, or pass/capture identity and reaches the same authoritative provenance chain without creating duplicate state.

#### CL boundary

Suggested title: `Shader Tools: deliver intent-first apply and provenance workflow`.

### Phase 8 - Regenerate, validate, reconcile, and hand off the complete candidate

#### Implementation prompt

> Implement Phase 8 directly in the unstaged `master` worktree against the complete Phase 0-7 candidate. Do not create or switch branches and do not stage, commit, push, or submit. Run the final legacy-eradication gate first, delete obsolete disposable shader/cache/cooked output, regenerate the one current representation once, then perform claim-driven formatting, architecture, compiler, cook, runtime, reload, lifetime, capture, and performance validation. Fix failures at their owning phase responsibility without compatibility. Remove temporary harnesses, fault injectors, verbose logging, report builders, and excessive diagnostics before handoff. Update current documentation only after its exact claims pass.

#### Phase-specific references

- [Change Process review and acceptance](../../Engineering/Standards/ChangeProcess.md#review-and-acceptance)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md)
- [Renderer/RHI boundary enforcement](../RendererRhiBoundary.md#enforcement)
- [Bistro and San Miguel workloads](../../Engineering/BistroAndSanMiguelWorkloads.md)

#### Required work

- Prove zero definitions/uses of every phase-owned legacy symbol/path, old source directory, compatibility representation, obsolete generated record, package-oriented UI spelling, broad context, and one-field migration carrier.
- Regenerate the complete shader catalog/map/program manifest/code library, dependency data, publication metadata, and canonical cooked outputs once from the final source.
- Run no-write formatting with the pinned formatter when available, `git diff --check`, local-link validation, file/CMake/include inventory, and `architecture_boundary_check`.
- Build the smallest owning ShaderCompiler/Renderer targets first, then the exact DevelopmentEditor D3D12+Vulkan product target required by the cross-backend contract.
- Cook and inspect every supported current program for DXIL and SPIR-V; compare reflection/layout/program/code identities and verify compiler policy/capability matrices without silently skipped cells.
- Validate clean/cold and warm DDC behavior, same-checkout-independent hashes, duplicate fan-out, changed dependency closure, corrupt-entry recovery, cancellation, failed-job replay, transactional publication, stale publication rejection, invalid replacement rollback, delayed GPU completion, and generation retirement.
- Run D3D12 and Vulkan Showcase smokes/captures for every supported raster, ray-query, exposure, presentation, debug, and advertised fallback branch. Full RT pipeline behavior remains owned by the separate RT delivery plan and must remain explicitly unavailable here.
- Measure cook queue/wall/CPU time, compiler-session memory high-water, cache hit/miss/corrupt recovery, generated/cooked bytes, startup/index/preload time, pipeline creation/readiness/late use, runtime memory overlap during reload, and frame impact on an exact pinned workload/environment. Classify comparisons as improves, preserves, or blocked from provenance.
- Perform a final diagnostic/code-structure audit: no permanent migration log stream, per-job spam, default report files, cache browser, submitted test scaffold, god orchestrator, god folder, forwarding wrapper, or duplicated validation/policy remains.
- Recheck `master`, staged state, unrelated dirty exclusions, generated/cooked source-control policy, and exact diff boundaries. Leave all work unstaged for user review.

#### Positive guardrails

- Use the cheapest claim-falsifying check first and record exact commands, configurations, backends, results, and unavailable evidence.
- Temporary local harnesses may prove state-machine/ABI defects but are removed before handoff; do not submit new test-only code without explicit user authorization.
- Preserve concise actionable failure diagnostics and external capture/profiler integration while deleting migration-only instrumentation.

#### Negative guardrails

- No broad speculative workspace build before focused owners, no simulated backend/capture result, no performance claim without baseline provenance, and no documentation claim ahead of evidence.
- No compatibility reader, old/new cook, fallback catalog, device-idle reload, retry loop, permanent validator, or miscellaneous final fix bucket.

#### Acceptance criteria

- Every final acceptance criterion below has exact evidence or is explicitly `BLOCKED`; no unrun or static-only check is described as passed.
- The source/catalog/ABI/cook/map/runtime/pipeline/frontend production path has one authority at every stage and zero legacy/compatibility paths.
- Required generated artifacts match the final source and no cache, report, debug bundle, capture, log, or temporary proof file is unintentionally included.
- Diagnostics are bounded and actionable, shader/runtime orchestration reads as named stages, and no implementation owner or folder mixes unrelated responsibilities.
- Branch is `master`, the staged diff is empty, scoped formatting/link/diff checks pass where available, and the user receives the unstaged changelist for manual review.

#### CL boundary

Suggested title: `Shaders: validate typed authoring and cooked runtime generation`.

Full ray-tracing pipeline execution is not a hidden Phase 9 of this migration. The [Ray-Tracing Pipeline and Dual-Execution Delivery Plan](RayTracingPipelineImplementationPlan.md) owns that separate complete vertical slice. This migration carries honest compiler-only RT metadata and preserves runtime rejection until that plan's native pipeline, SBT, trace, lifetime, fallback, and paired evidence gates land together.

## Final Acceptance Criteria

The migration is accepted only when:

- a pass author writes no package string, binding-layout debug string, pipeline debug string, or default `PassName` string
- every registered source and include has a canonical virtual path and portable diagnostic identity
- registry freeze rejects duplicate or late declarations with source locations
- typed permutation IDs round-trip, unsupported variants are never scheduled, and manifest diagnostics show readable values
- every shader-visible field has one declaration that drives reflection verification, binding, and structural identity; a direct pass reuses it, while a pass envelope composes it with graph-only fields without duplication
- `GBuffer` still cooks vertex and pixel stages into one logical program
- one file with several entry points and one program with stages from several files are both tested
- moving a shader source invalidates its compile input hash without silently renaming the render-pass label or logical program
- same-basename source paths cannot collide
- identical compiler-affecting requests deduplicate by `ShaderCompileInputHash` even when used by different programs; presentation/package text does not alter that hash
- identical validated bytecode has one exact `ShaderCodeHash`; physical duplication is measured and removed when an indexed merged library is actually selected
- runtime typed lookup resolves through the generated shader map/manifest, not a derived source basename or handwritten package string
- changed includes select every dependent program and no unrelated program when dependency data is valid
- the normal Shader Tools path is one `Apply Changed` intent with semantic search, one operation state, automatic validated activation, and no required package/layout/hash/backend/cache configuration
- the primary shader list has a bounded task-oriented column set; raw reflection, disassembly, compile requests, hashes, manifests, backend policy, rebuild-all, and manual reload remain reachable through contextual Diagnostics/Advanced actions
- a compile or validation failure shows one source-located root cause and next action, settles progress/cancellation once, and visibly confirms that the previous accepted generation remains active
- opening Shader Tools from a compile error, GPU marker, or pipeline event preserves the selected source/program/pass/frame/configuration identity without retyping it
- all registered programs validate and cook for the supported targets
- the generated support matrix distinguishes compiler-only, runtime-validated, unsupported, and fully exercised combinations; no backend silently ignores requested compile policy
- every advertised fallback has a selection owner and an exercised paired-backend test; registered-only alternatives are reported as such
- an invalid replacement leaves the previous runtime generation active
- required startup programs and PSOs are ready before interaction; any lazy creation is classified and measured; code and pipeline lifetimes are explicit, while native shader/module caching is backend-specific rather than assumed
- profiler and GPU-capture labels remain semantic and readable
- PSO validation distinguishes shader-only readiness from full-pipeline hits, misses, and late requests
- every enabled D3D12/Vulkan native cache path rejects incompatible/corrupt entries as a recoverable miss; a backend cache is not required until its measured benefit exceeds its capability and maintenance cost
- a captured shader/code hash resolves to the exact program, compile request, virtual source closure, and external debug symbols
- compiler failures produce a bounded one-job replay bundle even when no bytecode exists
- the all-program paired-backend validation runs as an explicitly reported product/CI evidence gate without adding submitted test-only code unless the user separately authorizes it
- ray-tracing library runtime execution remains a reported unsupported capability until a paired state-object/pipeline and shader-table path is implemented
- if full RT execution is later enabled, SBT identifiers/group handles are tied to the exact native pipeline generation, every region/index/alignment rule is tested, and a selected non-RT fallback is captured on both backends
- the [required evidence pack](#required-evidence-pack) is complete for the selected acceptance workload and pinned environment

## Final Position

Sparkle should follow Unreal's semantic center end to end: virtual shader source paths; immutable C++ shader types registered with explicit source, entry point, stage, parameters, permutation domain, and compile policy; reproducible asynchronous compile jobs backed by disposable derived data; generated global shader maps and code records; typed runtime references with render-resource lifetime; shader-visible parameter metadata reused or composed into RDG pass envelopes; and separate PSO precaching. Render-graph names remain diagnostic presentation. NVIDIA and AMD reinforce the same separation with filename-plus-entry shader loading, typed stage/pass selection, generated platform blobs, permutation keys, and independent GPU/pipeline labels.

The clean target is not "the filename is everything" and not "copy every Unreal subsystem." It is "each identity has one job, authors state only irreducible facts, tooling generates the rest, and every generated boundary is deterministic, validated, cacheable, inspectable, and safe to replace."
