# Shader Authoring and Cooked Shader Architecture

Status: target architecture and implementation plan; current-state findings are source-backed, while target completion requires the final evidence phase
Current audit: 2026-08-23 against the unstaged `master` worktree based on `0e85e99d3afeb671bcf9d4f6b494dcf7c8a2c1d6`
Scope: base implementation covers virtual sources, lean shader classes and parameters, compilation/validation without persistent compile-result caching, global shader map/code library publication, typed runtime lookup, graph draw/dispatch, reload, diagnostics, and evidence; permutations, precaching/prewarming, preload/streaming, native driver caches, and full RT execution are research context or deferred follow-ups, not base deliverables

## Purpose

This document decides how SparkleEngine should identify render passes and shaders without requiring authors to maintain parallel strings such as `DirectLighting`, `RendererShaderPackages::DirectLighting`, binding-layout names, pipeline names, source basenames, and cooked-package names.

It also maps Epic's global-shader lifecycle onto Sparkle from source import through runtime pipeline creation, explains which Unreal patterns are worth adopting, traces the design to Sparkle's engineering and portfolio requirements, and defines atomic implementation phases for the migration. It refines the shader-specific conclusions from [External Renderer Repository Comparison](../ExternalReferences/ExternalRendererComparison.md). That comparison remains the source-linked broad research document. This document owns the target shader-authoring architecture, current shader-lifecycle audit, implementation sequence, clean-break boundaries, and final acceptance gates. Code, tests, executable build configuration, and captured evidence remain the authority for what is implemented and proven today.

The implementation phases apply directly to the unstaged `master` worktree. They do not authorize creating or switching branches, staging, committing, pushing, or submitting; the user owns review and every source-control action. Phases 0 through 7 are source/document checkpoints and do not configure, build, compile shaders, cook, launch, capture, or claim executable validation. Phase 8 validates the complete candidate once, after all obsolete authoring, package, parameter, cook, runtime, and frontend paths are gone.

The [Ray-Tracing Pipeline and Dual-Execution Delivery Plan](RayTracingPipelineImplementationPlan.md) owns the staged implementation, effect-level inline/pipeline selection contract, native pipeline and shader-table gates, and paired execution evidence for full ray-tracing shader support.

## Adversarial Review Verdict

The 2026-08-15 review assumed every proposed Unreal/vendor analogy was wrong until a primary source, current Sparkle code, and a local requirement established its exact scope. The result keeps the end-to-end direction but narrows several earlier overclaims:

| Challenged claim | Verdict and proof boundary |
| --- | --- |
| The shader filename can replace pass identity. | Rejected. Epic global shaders, Donut/NVRHI-style shader creation, current multi-entry/multi-stage cases, and Sparkle's `GBuffer` prove source location, entry, shader stage, pipeline composition, and graph operation are different identities. |
| Every pass needs a handwritten `PassName`. | Rejected. The semantic event label is necessary, but the one-to-one default can derive from the concrete shader class; only instance-specific text remains authored at scheduling. |
| Authors need named shader packages. | Rejected. Authors need concrete shader classes and cooked runtime artifacts; physical map/library membership is generated delivery policy. |
| One shader parameter struct must be every pass struct. | Overconstrained and corrected. Epic RDG intentionally reuses shader parameters for common one-to-one passes but also supports pass parameters without shader semantics. Sparkle selects one owner per shader-visible field plus explicit pass-envelope composition. |
| A compile-input hash requires a persistent compile-result cache. | Rejected. Sparkle needs deterministic input identity for diagnostics, deduplication within one operation, and provenance, but every selected job still invokes the compiler. |
| A deduplicated compressed/streamed physical code library is always required. | Not proven for the current catalog. Exact hashes, maps, and code-record schemas are required; physical merging, compression, chunks, and streaming require measured byte/I/O value. |
| D3D12 pipeline libraries and a long-lived shader/module cache are universal baseline best practice. | Rejected as universal. Complete pipeline descriptions, correct lazy graph-time materialization, and first-use measurement are the base contract. Async preparation, native caches, and shader/module object retention are backend/capability-specific measured follow-ups. |

“Epic-aligned” therefore means matching responsibility boundaries, invariants, failure behavior, and authoring ergonomics—not copying class names, macro volume, material-system scale, or every optional cache.

## Decision Summary

1. Sparkle should adopt Unreal's global-shader flow as its core mental model: one concrete shader class owns its parameter contract and compile hooks; one implementation declaration supplies virtual source, entry point, and stage; cooking produces a global shader map and code records; runtime resolves a typed shader reference from the active map.
2. A one-shader compute pass does not need an authored program alias, pass-registration macro, duplicate pass-parameter struct, or forwarding pass class. The frame graph should accept the shader type, its nested `Parameters`, a diagnostic label only when the generated default is insufficient, and the dispatch dimensions.
3. Graphics work names the concrete vertex/pixel shader types at the draw site with the actual pipeline description. Full ray-tracing stage grouping remains owned by the separate RT pipeline plan. Sparkle does not need a universal authored `ShaderProgram` abstraction for the current renderer.
4. Shader inputs and outputs live in the shader class's nested `Parameters` schema. The frame graph consumes the same schema to declare resource access; runtime binding and reflection validation derive from it. Multi-shader or shaderless graph work may own a small envelope only for composition or graph-only fields.
5. A semantic render-pass label remains necessary for frame-graph diagnostics, GPU markers, errors, captures, and profiling, but it does not select code. Generate the one-to-one default from the shader type and accept an instance override for mip, cascade, phase, view, or repeated use.
6. A shader filename is a virtual source input, not the pass, shader-map, pipeline, or diagnostic identity.
7. Sparkle should keep cooked shader data but replace handwritten packages with two generated authorities: `GlobalShaderMap` for typed logical lookup and `CookedShaderLibrary` for validated code records addressed by hash. `RenderPassRuntimeCache` continues to own generation-safe materialized layouts/pipelines derived from the active map.
8. Permutation infrastructure is deliberately postponed until after the non-permuted shader-map path is complete and accepted. The base map key is `(ShaderTypeId, Target)`; a later measured follow-up may extend it with a typed `PermutationId` without changing the lean one-variant authoring path.
9. PSO precaching/prewarming, preload controls, and driver-cache integration are outside this implementation plan. Preserve correct lazy materialization before command recording; add earlier preparation only after measured first-use hitch evidence justifies it.
10. Shader Tools should center `Apply Changed`, semantic shader/source selection, one operation state, source-located errors, and contextual next actions. Package/layout IDs, hashes, raw artifacts, full rebuild/reload, backend flags, and runtime materialization mechanics remain expert details.
11. Inline ray queries and full ray-tracing pipelines are different systems. Sparkle currently has the former. RT shader declarations may share the same class/parameter/map pattern, while native pipeline grouping, SBT construction, trace commands, and execution evidence stay in the separate RT plan.

The short answer is therefore:

> Make the shader class the lean authoring unit, keep its parameters inside the class, remove manual package/program/pass duplication, resolve it through a generated global shader map, and let the frame graph dispatch that shader directly with the same typed parameters.

## Why the Shader Filename Is Not the Pass Name

A filename answers "where is source text?" A pass name answers "what GPU operation is this?" Coupling them creates incorrect behavior in ordinary cases:

- One source file can contain several entry points or stages.
- One graphics pipeline can use shader stages from several files. Sparkle's current `GBuffer` package already uses `GBufferVS.hlsl` and `GBufferPS.hlsl`.
- The same compiled shader can be reused by multiple semantic passes.
- A pass type can be scheduled several times with instance-specific labels such as a mip number, eye, cascade, phase, or view.
- Renaming or moving a source file should invalidate compilation, but it should not silently rename profiler history, GPU markers, frame-graph nodes, or the shader type that references it.
- Two directories can contain the same source basename. Sparkle's current basename-derived fallback cannot distinguish them.
- Include files and shader libraries are source dependencies but are not independently executable passes.

Using a filename as a default shader debug name is reasonable. Using it as the durable shader-type, pass, pipeline, layout, and artifact identity is not.

## Required Identity Model

Each identity has one responsibility and one owner.

| Identity | Meaning | Proposed authority | Must not be derived solely from |
| --- | --- | --- | --- |
| Render-pass label | frame-graph, GPU marker, diagnostic, and profiler text | generated from the one-to-one shader type or supplied as an instance override | cooked artifact name |
| Shader type | one compilable source/entry/stage plus its nested parameter contract | concrete shader class and implementation declaration | pass label |
| Source identity | normalized virtual path plus transitive include closure | shader compiler dependency graph | basename |
| Entry identity | source path, entry point, and stage | typed shader registration | source path alone |
| Binding layout | full structural binding contract | generated parameter/reflection contract hash | parameter count |
| Cooked artifact | backend binaries, reflection, layouts, features, and provenance | cooker-generated content key | pass label or filename |
| Shader-map entry | target-specific typed lookup from shader class to validated code record | generated `GlobalShaderMap` | source basename or authored package |
| Pipeline | concrete shader references plus fixed-function and target state | complete pipeline description/cache key at graph use | one shader or diagnostic label alone |

Pass labels and debug names may be human-readable. Lookup and reuse identities must be typed or hash-backed and collision checked.

## Target Shape

```text
  AUTHORING                         COOKED/RUNTIME                    FRAME GRAPH
  =========                         ==============                    ===========

  DirectLightingCS                  GlobalShaderCatalog              AllocParameters<DirectLightingCS>()
  + virtual source/entry/stage ---> ShaderCompileJob                 fill typed inputs/outputs
  + nested Parameters               bounded compile jobs             Dispatch<DirectLightingCS>()
  + optional compile hooks          GlobalShaderMap                  optional instance label
            |                       CookedShaderLibrary                        |
            |                                  |                              |
            +---------------------------------> ShaderRef<DirectLightingCS> <---+
                                               |
                                               v
                                      RenderPassRuntimeCache
                                      binding layout / pipeline
                                               |
                                               v
                                      bind declared resources
                                      record draw/dispatch
```

There is one lookup chain, not a catalog plus handwritten program registry plus package cache. `GlobalShaderCatalog` describes what can be compiled. `GlobalShaderMap` is the active target-specific typed lookup. `CookedShaderLibrary` supplies validated bytes by hash. `RenderPassRuntimeCache` derives generation-bound layouts and pipelines from shader references plus the actual draw/dispatch description. The RHI owns neutral descriptors, record validation primitives, and backend objects; Renderer owns concrete shader classes, graph use, and runtime generation policy; ShaderCompiler owns source dependency, bounded compilation, map/library generation, and transactional publication.

## Unreal-Aligned System Model

Unreal does not have one object called a "shader package" that owns every concern. Its useful architectural pattern is a pipeline of distinct authorities. Sparkle should preserve that separation even when its implementation is much smaller.

| Unreal concept | Responsibility | Sparkle target | Current Sparkle approximation |
| --- | --- | --- | --- |
| virtual shader source paths | stable source namespace independent of checkout location | `ShaderSourceMountTable` with `/Engine`, `/Project`, and `/Plugin/<Name>` roots | project-first then engine physical path search |
| `FShaderType` / `FGlobalShaderType` | immutable shader metadata and compile hooks | `ShaderTypeDesc` emitted by typed registration | `ShaderRegistrationDesc` |
| `TShaderPermutationDomain` | typed, bounded permutation dimensions and stable IDs | deferred follow-up after the one-variant shader-map path is accepted | free-form compile defines and package variants |
| `FShaderCompilerInput` | complete read-only input for one compilation | `ShaderCompileRequest` | `ShaderCompileOptions` plus cook-node metadata |
| `FShaderCompileJob` / `FShaderCompileJobKey` / input hash | scheduled unit and logical shader/target key, plus a separate hash over all compiler-affecting inputs | `ShaderCompileJob` plus `ShaderCompileInputHash`; a small logical job ID may exist only for scheduling | `CookNode` plus `TaskExecutor` task |
| `FShaderCompilingManager` and Shader Compile Workers | asynchronous coordination and compiler-process isolation | compile-job coordination on the existing cooker `TaskExecutor`; optional worker processes only when justified | one out-of-process cooker with bounded in-process stage tasks |
| Global Shader Map / `TShaderMapRef<T>` | typed target-specific lookup and shader lifetime | `GlobalShaderMap` / `ShaderRef<Shader>` | pass runtime cache loading by package ID |
| `FShaderMapResourceCode` | code hashes and map resource content | generated map resource record | code embedded in each `.sparkshader` program package |
| `FShaderCodeLibrary` | cook-time collection of unique code and runtime loading by hash | `CookedShaderLibrary` | no cross-package code library |
| `FShaderPipelineType` | optional declared stage grouping | no base authoring abstraction; graphics names stage shader types at the draw site and full RT grouping belongs to the RT plan | stages grouped by repeated package string |
| RDG shader/pass parameter structs | shader parameters can directly serve a one-to-one pass; pass envelopes and shaderless pass parameters are also valid | one shader-visible `Parameters` schema, reused or composed into a pass envelope without duplicating shader fields | separate pass parameters and registered shader parameters |
| RDG event name | diagnostic/profiler identity of one graph operation | generated default `RenderPassLabel`, optional instance override | repeated `PassName` literal |
| PSO precache / shader pipeline cache | earlier pipeline preparation and hitch tracking | deferred until current lazy materialization shows a measured product hitch | pipeline creation coupled to pass runtime creation |

The mapping is architectural, not a request to copy Unreal class names or source code. Sparkle should keep names that fit its own standards while retaining the responsibility boundaries.

### Full Lifecycle

```text
AUTHORING
  shader class
    + nested Parameters
    + optional compile eligibility/environment/validation hooks
  IMPLEMENT_GLOBAL_SHADER(class, virtual source, entry, stage)
                         |
                         v
CATALOG / COMPILE
  validate and freeze ShaderTypeDesc records
  create one ShaderCompileJob per (ShaderTypeId, Target)
  preprocess virtual include closure -> input hash -> bounded compile
  verify reflection against Shader::Parameters
                         |
                         v
COOK / PUBLICATION
  GlobalShaderMap: (ShaderTypeId, Target) -> ShaderCodeHash + ABI metadata
  CookedShaderLibrary: ShaderCodeHash -> validated backend code record
  development provenance: source/dependencies/compiler/symbols
  publish the complete generation transactionally
                         |
                         v
RUNTIME / FRAME GRAPH
  open map + library -> ShaderRef<Shader>
  AllocParameters<Shader>() -> fill declared graph inputs/outputs
  Dispatch<Shader>(parameters, groupCount) or Draw<VS, PS>(parameters, pipeline, draws)
  RenderPassRuntimeCache lazily materializes generation-bound layout/pipeline before recording
  Execute binds only declared resources and records commands
                         |
                         v
DEVELOPMENT RELOAD
  changed virtual paths -> reverse dependencies -> affected shader jobs
  complete replacement map/library -> validate -> atomic swap
  old generation retires after all recorded GPU submissions complete
```

### The Separation That Prevents Bloat

The lean design is not "compile every file automatically." It is automatic generation from small, explicit declarations:

- source files provide implementation text and includes;
- shader classes provide executable entry points, nested parameter metadata, and only the compile hooks they actually need;
- graph dispatch names the concrete shader type(s) and supplies execution dimensions or draw work;
- parameter metadata provides the shared graph-resource and shader-binding contract;
- shader maps provide logical typed lookup;
- code records provide exact hashed physical delivery; a library may merge duplicate blobs when measured useful;
- pipeline descriptors add fixed-function state;
- render-pass labels provide diagnostics only.

This is why filenames cannot safely replace shader types, graph operations, or pipelines. Automation derives map identity, package/layout names, binding metadata, and runtime lookup. Authors state only facts the system cannot infer: class, virtual source, entry, stage, parameter fields, optional capability policy, and the actual dispatch/draw request.

## Proposed Authoring Experience

Unreal's production pattern is intentionally direct: the shader class declares `FParameters`; `IMPLEMENT_GLOBAL_SHADER` binds the class to source, entry, and stage; RDG allocates that same parameter type; `FComputeShaderUtils::AddPass` receives the typed shader reference, parameters, and group count. Sparkle should preserve that experience while using Sparkle naming and its existing graph/runtime owners. It should not copy Unreal-only `F` prefixes, constructor/type-layout macros, or systems that Sparkle's CRTP registration already derives.

The exact lean Sparkle target for a one-to-one compute shader is:

```cpp
class DirectLightingCS final : public GlobalShader<DirectLightingCS>
{
public:
    BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
        SHADER_PARAMETER_UAV(RWTexture2D, DirectDiffuse)
        SHADER_PARAMETER_TEXTURE(Texture2D, ShadowVisibility)
        SHADER_PARAMETER_CBUFFER(View, ViewUniformData)
    END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(
    DirectLightingCS,
    "/Engine/Passes/Lighting/Direct/DirectLighting.hlsl",
    "main",
    Compute);
```

Graph construction uses that class directly:

```cpp
auto& parameters = builder.AllocParameters<DirectLightingCS>();
parameters.DirectDiffuse = builder.CreateUAV(directDiffuse);
parameters.ShadowVisibility = builder.Read(shadowVisibility);
parameters.View = viewUniforms;

builder.Dispatch<DirectLightingCS>(parameters, groupCount);
```

`AllocParameters<Shader>()` means `Shader::Parameters`; it does not allocate a second schema. `Dispatch<Shader>()` resolves `ShaderRef<Shader>` from the active `GlobalShaderMap`, derives the default diagnostic label from the shader type, declares resource usages from the same parameter metadata, materializes the generation-bound layout/pipeline through the backend owner, and records the dispatch later. The caller sees none of the package, map, code-library, binding-layout, or pipeline-cache mechanics.

An explicit label is only for a genuinely distinct graph instance:

```cpp
builder.Dispatch<ExposureDownsampleCS>(
    RenderPassLabel::Format("ExposureDownsample mip {}", mip),
    parameters,
    groupCount);
```

That label describes this graph node. It does not select shader bytecode.

Graphics follows the same rule without inventing a program alias: the draw path requests typed `ShaderRef<GBufferVS>` and `ShaderRef<GBufferPS>` from the map and combines them with the actual vertex layout, render-target formats, depth/stencil, blend, raster, topology, and draw data. A focused mesh-pass collaborator may remain when it owns real draw-list/cache behavior; one-method pass wrappers that only forward parameters to a shader do not.

Ray-tracing shader declarations should use the same class shape—nested root `Parameters`, virtual source, entry/export, stage, compile eligibility, and payload/attribute metadata only where required. Ray-generation, miss, hit, intersection, and callable shaders resolve through the same `GlobalShaderMap`. Hit-group composition, native RT pipeline creation, shader identifiers, SBT local records, and trace dispatch remain a single responsibility in the separate RT pipeline plan; this shader-authoring migration must not create a parallel `TShaderProgram` abstraction for them.

The final form has these invariants:

- the shader class is the sole author-facing owner of shader-visible parameters;
- the implementation declaration states only class, virtual source, entry, and stage/export facts;
- optional compile hooks appear only on shader classes that use them; the common class has none;
- no precache hook or permutation-domain boilerplate exists in the base migration;
- no `DirectLightingProgram`, `SPARKLE_RENDER_PASS`, duplicated `DirectLightingPassParameters`, or forwarding `DirectLightingPass` is required for the one-to-one compute case;
- raw shader directory scanning does not replace registration because include/library files are not executable entries;
- binding-layout and pipeline debug names are generated and never lookup authorities;
- compiler RTTI strings and implementation-specific pretty-function text are not serialized; generated catalog records carry deterministic IDs and declaration locations.

### Intent-First Shader Frontend Contract

The normal shader workflow is one `Shader Tools` surface plus the existing console/shortcut route, not a panel for every compiler, package, pipeline, or artifact layer. The user supplies intent; the system resolves dependency closure, supported targets, job scheduling, validation, publication, reload, and lifetime safety.

| User intent | Primary frontend action/result | Automatically derived and validated | Contextual expert access |
| --- | --- | --- | --- |
| Apply saved source edits | `Apply Changed` with one shortcut and nonblocking status | changed virtual paths, reverse dependencies, affected shader types, active development targets, bounded jobs, complete replacement generation, safe activation | selected/compiled counts; exact job list under Details |
| Understand a compile failure | source-located root cause, affected shader type, `Open Source`, `Retry`, and reassurance that the previous generation remains active | duplicate diagnostics collapsed by root cause, dependent jobs classified as skipped, portable virtual paths, failure bundle creation | compiler output, command, preprocessed source, dependency hashes, reflection/layout comparison, one-job replay |
| Inspect what the running frame uses | select/search a semantic graph operation or shader and see active, changed, compiling, failed, or unsupported state | join from graph label/shader type to active shader-map entry, code record, source, symbols, pipeline, and captured marker identity | target/backend, hashes, reflection, disassembly, map record, and pipeline provenance |
| Validate a development or release build | `Validate Shaders` through the normal build/cook preset | complete registered catalog, compile-policy filtering, D3D12/Vulkan targets required by that preset, deterministic publication and conformance checks | support matrix, compiled-job counts, compiler time, and memory statistics |
| Investigate a shader or pipeline hitch | `Open Shader` from the selected Performance/GPU marker or pipeline event | preserve frame, marker, shader/code/pipeline hashes, backend, generation, and symbol identity | PIX/RenderDoc/Nsight/RGP guidance and copied replay/capture identity |
| Reproduce one expert failure | `Replay Failed Job` from Diagnostics details | exact compiler/tool version, virtual source closure, target, environment, and bounded output location | editable command copy and machine-readable request |

The common Editor layout is deliberately small:

```text
+ Shader Tools ---------------------------------------------------------------+
| 3 changed files ready       [Apply Changed]                    [Search ...] |
| Status: Ready | active generation 42 | last apply compiled 18 shaders |
+ Shaders ---------------------------------+ Selection -----------------------+
| DirectLighting        Ready              | DirectLighting / Compute          |
| GBuffer               Ready              | source + entry                    |
| Exposure              Changed            | used by 1 pass                    |
| ...                                        | [Open Source] [Apply Selection]  |
+ Diagnostics ----------------------------------------------------------------+
| No active errors. [Show last operation] [Advanced...]                       |
+------------------------------------------------------------------------------+
```

The primary list should normally expose shader name, stage, virtual source, active status, and graph consumers. Package ID, binding-layout ID, generation number, backend/profile, artifact directory, code/input hashes, compiler command, raw reflection, and disassembly are contextual details—not ten default columns. `Reload Cooked`, `Recook All`, package-targeted recook, compiler backend/target listing, and worker controls do not belong beside the common `Apply Changed` action. Validated publication activates automatically; `Rebuild All` and manual reload remain searchable expert recovery actions with an explanation of cost and risk.

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

Epic's current shader-development workflow similarly centers a saved edit plus `recompileshaders changed`, asynchronous compilation, direct source diagnostics, and retry. Sparkle adopts that intent and safe iteration loop while generating more of the target/dependency/publication detail, compiling every selected job, and retaining the previous valid generation. [Epic Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)

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

The base target uses three identities with different scopes. The names deliberately distinguish Unreal's logical job identity from its full compiler-input hash:

1. `ShaderTypeId`: a stable ID emitted from the concrete shader declaration. Together with target it addresses a base shader-map entry.
2. `ShaderCompileInputHash`: a content hash of every input that can change one stage compile, including virtual source path, transitive source contents, entry point, stage, environment, target, compiler backend, and compiler version. Parameter metadata enters this hash only when it changes generated declarations, resource-table/environment input, or compiler output; a validation-only signature belongs in the map record instead. Package/pass/display identity is excluded.
3. `ShaderCodeHash`: a hash of the exact validated backend bytecode. Stage, entry, reflection, feature, layout, and provenance metadata remain adjacent current-contract map/record fields and are covered by artifact integrity; they are not silently conflated with raw code identity. This preserves exact byte deduplication and symbol correlation without treating equal bytes with incompatible metadata as interchangeable records.

This allows source edits to invalidate compilation, source moves to be diagnosed correctly, identical compile jobs and identical output code to be deduplicated at their proper layers, and typed shader lookup to remain independent of a file basename.

The current one-`.sparkshader`-per-package layout supplies migration inputs only; it is not a target checkpoint or compatibility container. Phase 5 atomically replaces it with shader-map entries and code records, deletes the old readers/writers, and leaves physical grouping as generated policy. The initial library may use one or several generated files, but that choice must not expose authored package identity. Later regrouping requires current startup I/O, file-open, compression, and patching evidence and regenerates the one current representation in place.

```text
logical identity                         physical delivery
================                         =================

ShaderTypeId + Target ----------------> GlobalShaderMap entry -----+
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

Epic's `FShaderCompilerInput` gathers the read-only inputs for one compile, including the virtual source path, entry point, target, platform/format, environment, parameter metadata, debug information, and deterministic input identity. `FShaderCompileJob` pairs input, a logical job key, preprocess output, compiler output, and diagnostics; the logical `FShaderCompileJobKey` is not the full content hash. `FShaderCompilingManager` coordinates priorities, pending jobs, result application, cancellation, worker processes, and optional distributed execution. Epic explains that Shader Compile Workers provide process-level parallelism around compiler implementations that may otherwise serialize internally.

Unreal cooking collects unique shader code into shader libraries. At runtime `FShaderCodeLibrary` can open project/plugin libraries and test for code by hash. `FShaderMapResource` separately owns the render-resource lifetime. Sparkle adopts the logical shader map, cooked code library, and live RHI-object separation while deliberately omitting persistent storage of compiler results.

Unreal's Render Dependency Graph intentionally lets a shader parameter structure also describe the common one-to-one pass, while `FRDGBuilder::AddPass` still receives a separate event name for debugging and profiling. Epic also documents pass parameter structures without shader semantics, such as copy passes. Sparkle should therefore reuse or compose one authoritative shader-visible schema, not force every graph pass into one shader struct. Epic's parameter metadata exposes both a layout hash and a strong persistable layout signature; that is stronger than Sparkle's current independent Direct Lighting declarations plus count-only runtime fallback.

Unreal's shader frontend is the more important precedent for this migration: a concrete global-shader class declares its `FParameters`; the implementation macro binds source, entry, and stage; RDG allocates the same parameter type; and `FComputeShaderUtils::AddPass` consumes a typed shader reference, those parameters, and group count. Unreal also has optional shader-pipeline and PSO-precaching systems, but Sparkle does not adopt them in this plan because the current workload has not justified their authoring or runtime cost.

Adopt now:

- typed shader registration
- explicit virtual source path, entry point, and stage
- shader-class-local compile eligibility/environment/result-validation hooks only where currently needed
- complete immutable compile inputs, deterministic input hashes and logical job IDs, in-flight deduplication, priorities, and cancellation
- typed shader-map lookup and independently owned RHI shader lifetime
- compile-input hashes derived from the full compiler-affecting closure and compiler provenance
- cook-time collection and deduplication of global shader code
- a nested shader parameter contract reused directly by one-to-one graph dispatch, with a pass envelope only for real multi-shader or graph-only composition
- render-graph event labels kept separate from shader identity
- direct typed compute dispatch utilities and typed shader-map lookup

Do not copy yet:

- Unreal's full material, vertex-factory, and permutation universe
- authored shader-pipeline types, PSO precaching/prewarming, driver-cache integration, and preload/residency controls before measurements justify them
- distributed compilation and worker-farm infrastructure before local scale justifies it
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

FidelityFX uses an effect-specific pass enum and permutation flags to select generated shader blobs, then gives the created pipeline a separate human-readable name. Its useful precedent for Sparkle is only the separation of executable identity from the human-readable pipeline label. Sparkle's base migration does not adopt FidelityFX's effect/pass enum or generated-variant selector; the concrete shader class and typed draw/dispatch already provide the required identity.

Adopt:

- include-aware and define-aware compile identity
- typed shader selection at draw/dispatch
- separate pipeline/debug presentation names

Study generated permutation lookup only in the separately approved future permutation proposal.

Sources:

- [AMD Cauldron `ShaderCompiler.h` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/common/base/ShaderCompiler.h)
- [AMD Cauldron `ShadowResolvePass.cpp` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/PostProc/ShadowResolvePass.cpp)
- [AMD Cauldron `GltfPbrPass.cpp` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/GLTF/GltfPbrPass.cpp)
- [AMD FidelityFX optical-flow program selection at `60f4ea8`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/FidelityFX/framegeneration/fsr3/internal/ffx_opticalflow.cpp)

### Compiler, Capture, and Native Pipeline Precedent

Epic's development workflow treats a reproducible compile as a product feature: a debug dump contains the contributing sources and includes, preprocessed source, and a command file that replays the compiler invocation. Epic's cooked-shader debugging workflow can generate symbols separately from runtime shader data, so making a shader inspectable does not require permanently bloating the shipped bytecode.

The external GPU tools reinforce that provenance requirement. PIX resolves separate DXIL debug data by a compiler-suggested hash name and can show source and compile arguments from slim PDBs. NVIDIA Nsight Graphics needs source-level debug information and line mappings to correlate DXIL or SPIR-V hotspots and crashes back to HLSL. AMD Radeon GPU Analyzer inspects target ISA, register pressure, LDS, and scratch use. Sparkle therefore needs a bytecode-hash-to-source-symbol record and an opt-in analysis build, not only a text disassembly produced during a successful cook.

The native APIs also reinforce the separation between shaders and pipelines. Microsoft exposes cached D3D12 pipeline state and Vulkan exposes pipeline caches, while Epic layers a renderer-level precache policy above complete descriptors. Those are useful future precedents, not base-migration requirements. The current migration preserves correct lazy materialization before command recording and measures it; it does not add caches or precache orchestration without a demonstrated product hitch.

Adopt:

- write a self-contained replay bundle for failed as well as successful compile jobs
- store source mappings and separate debug symbols by shader/code hash so PIX, RenderDoc, and Nsight can resolve the exact cooked binary
- keep symbols and analysis products outside the lean runtime artifact unless an explicit development mode embeds them
- make representative DXIL and SPIR-V inspection first-class verification outputs
- feed exact shipped compile requests to RGA, Nsight, PIX, or equivalent analysis rather than recompiling an approximate shader by hand
- preserve complete D3D12/Vulkan pipeline descriptions and measure current lazy materialization so a later cache/precache proposal has evidence

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
                                              compile every selected stage           |
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
- The preprocessor already expands nested includes, detects recursion, handles `#pragma once`, and emits line directives. `IncludeClosureHasher` and compile-option hashing feed a deterministic `compileInputHash` used only for job identity, diagnostics, and provenance.
- `ShaderCookPlanExecutor` performs bounded parallel stage jobs through `TaskExecutor`, and every selected job invokes the compiler and validates parameter metadata. There is no persisted compile-result lookup, local artifact store, cache key/status, cache-directory option, or Launcher cache control.
- Cancellation is checked before a cook-node task begins, but an executing compiler session or child operation is not cooperatively cancelled. Jobs have no priority, no in-flight duplicate fan-out, no time-sliced result integration, and no measured worker-memory ceiling.
- Include-closure hashes serialize normalized physical paths, and compile-option hashes include physical include directories. Compile identity is therefore tied to a checkout/machine layout even when source bytes and virtual meaning are identical.
- Changed-source monitoring detects that some shader file changed, then invokes a full catalog `cook`; because selected jobs always compile, the missing reverse-dependency selection directly recompiles every registered shader.
- The cooker publishes selected `.sparkshader` files, a registry, and a recook signal as one transactional file set. The registry is not currently the runtime lookup authority; runtime computes a path directly from the manually repeated package ID.
- Bytecode is embedded per current package, so Sparkle has neither in-operation compile-job deduplication across identical shader inputs nor a cooked unique-code library addressed by code hash.
- DXC can emit disassembly, preprocessed source, compiler arguments, diagnostics, and separate debug data for a successful opt-in analysis cook. Slang's current analysis output is narrower and does not emit equivalent disassembly.
- The DXC backend applies requested debug information, optimization, warnings-as-errors, and debug stripping. The Slang backend currently does not apply equivalent cook-policy switches, and its stage mapping covers vertex, pixel/fragment, and compute rather than the full stage enum. Slang is therefore an architectural backend seam, not a proved release-equivalent compiler path.
- Debug-artifact publication begins only after compilation succeeds. A syntax error or backend failure throws before `ShaderDebugArtifactWriter` receives an artifact set, so the failure most in need of reproduction has no complete replay bundle, dependency manifest, or one-job command.
- `CookedShaderStats.csv` reports package/stage/backend/target/entry, bytecode bytes, reflection counts, and layout-record counts. It does not report queue/wall/CPU time, peak memory, source/code hash correlation, native pipeline creation, or runtime hitch percentiles.
- `inspect-shader` inspects registration/catalog metadata; `inspect-package` inspects the cooked container, reflection, layouts, and ray-tracing records. There is no single command that follows a typed shader from declaration through compile-input hash, code hash, active map/library record, runtime shader reference, pipeline, and captured GPU event.
- `RenderPassRuntimeCache` already builds and validates a complete replacement generation, atomically activates it, and retires the old generation only after recorded RHI submission tokens complete. This is a strong lifetime pattern and must be preserved.
- Runtime package validation is materially stronger than the authoring layer: it verifies schema/version, source and layout identities, bounds, complete logical binding records, required stages, runtime backend format, bytecode hashes, reflection, feature flags, and ray-tracing metadata before use.
- The editor already launches the shader cooker out of process, coalesces one follow-up request, rejects stale publications, and leaves the active generation unchanged after cook or runtime-validation failure.
- The current `Shader Tools` window presents Refresh, Reload Cooked, Recook All, and Recook Selected as equal toolbar actions above a ten-column table. Package ID, binding layout, parameter count, backend/target text, generation, and artifact availability are visible before the user has asked an expert question. This is an implementation-oriented inventory, not the selected intent-first frontend.
- The current selection area opens raw Source, Reflection, Disassembly, Param Match, and Compile Request artifacts and discovers their directory from shader/package identity. These are valuable expert details, but the panel lacks one operation/status model, source-located failure summary, shader consumer trace, and a guided next action. Raw artifacts must move behind contextual Diagnostics/Advanced disclosure rather than be removed.
- Current source tracking automatically schedules a changed recook, while the toolbar and console also expose manual global/package/shader actions. The target must converge these into one coalesced `Apply Changed` workflow with one visible state; full rebuild, manual reload, and identity-targeted recovery remain expert actions rather than parallel normal paths.
- A pass runtime is materialized lazily from `FrameGraphBuilder::Draw`, `Dispatch`, or `DispatchAsync`. Its package, binding layout, and pipeline are created during graph construction, not command recording. This preserves the frame-graph Execute boundary but can place first-use pipeline creation on a frame-critical construction path.
- Renderer frame orchestration now uses `BuildRenderFrameGraph`, explicit pass parameter records, and the infrastructure-only `PassCommandContext`. Shader work must preserve that boundary: graph setup may resolve a prepared immutable shader/pipeline runtime, while pass recording may only bind declared resources and record commands.
- `RenderFrameIdentity` captures the active shader generation from `RenderPassRuntimeCache`; `RenderViewState` invalidates history from the generation transition. The shader migration must keep one renderer-owned generation source and must not publish package/cache/compiler mechanics through GameFramework, `RenderFrameSubmission`, `RenderView`, or `PreparedRenderScene`.
- `FrameGraph::HasBeenProduced` is now the authority for whether a declared graph resource has prior contents. Shader and pipeline work must not reintroduce one-field history-validity records, mirrored production flags, or broad frame/pass contexts.
- Renderer C++ passes now live under semantic `Passes/GBuffer`, `Passes/Lighting/<Direct|Reference|Restir|Shadows|Sky>`, `Passes/PostProcessing`, `Passes/Presentation`, `Passes/RayTracing`, and `Passes/Debug` owners. Shader sources still collect 18 files under `Engine/Assets/Shaders/Passes/Deferred`; the source-namespace phase must move those files to matching semantic owners and delete the broad directory rather than treating `Deferred` as renderer-wide architecture.
- D3D12 creates every graphics/compute pipeline with an empty `CachedPSO`; Vulkan calls `vkCreateGraphicsPipelines` and `vkCreateComputePipelines` with `VK_NULL_HANDLE` for the pipeline cache. Vulkan computes a local cache-key-shaped struct and then discards it. There is no renderer pipeline cache, native persistent cache, asynchronous precache coordinator, or hit/miss/too-late telemetry.
- Pass labels, D3D12 PIX events, Vulkan object names, binding-layout names, and pipeline names are readable. They do not carry stable shader-type/code identity that can join a capture event to the cooker artifacts and external shader symbols.
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
- [`IncludeClosureHasher.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/Identity/IncludeClosureHasher.cpp)
- [`ShaderCompileOptionsHasher.cpp`](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/Identity/ShaderCompileOptionsHasher.cpp)
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

source mounts                catalog selection                   open shader map + code index
    |                             |                                      |
source + includes ----------> preprocess + dependency graph              |
    |                             |                                      |
shader type + entry --------> immutable request + scheduled job          |
    |                             |                                      |
parameter ABI --------------> front end -> IR -> optimize/codegen        |
    |                             |                                      |
shader class + Parameters ---> reflection + ABI validation              |
    |                             |                                      |
RDG pass                         bounded compile/publication               |
                                  |                                      |
                                  +-> shader map + code records ----------+
                                  +-> symbols/provenance                  |
                                  +-> map/library metadata                v
                                                                  resolve typed shader ref
                                                                          |
                                                               backend-specific native input
                                                                          |
                                                             complete pipeline description
                                                                          |
                                                          lazy native pipeline materialization
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

### Shader Definition and Graph Use

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 1. Platform and capability contract | lowest common feature set; per-platform shader platforms; runtime feature branching | Define explicit shader-platform capabilities and compile eligibility; reject unsupported combinations before jobs are scheduled. | More platform records, but failures become deterministic and the base request set stays bounded. | DXIL/SM and SPIR-V targets plus package feature flags exist; some feature validation and consumption branches are incomplete. |
| 2. Source namespace/import | physical paths; basename lookup; virtual mounted paths | Unreal-style `/Engine`, `/Project`, and `/Plugin/<Name>` virtual paths mapped to physical roots by tooling. | A mount table and collision rules are required; checkout-independent identity and portable diagnostics are gained. | Physical project-first/engine-second lookup, absolute includes, and basename package fallback. |
| 3. Language and modules | HLSL only; Slang only; HLSL plus opt-in Slang; generated kernels | Keep HLSL/DXC as the production reference path. Keep Slang as an explicit backend/language experiment until policy and output parity are proved. Permit generated source only through one provenance-recorded generator with source maps and deterministic regeneration. | One reference path is less flexible; two unqualified paths multiply conformance work. | `.hlsl` auto-selects DXC; `.slang` auto-selects Slang. Explicit backend selection can override that convention. |
| 4. Includes and reusable code | textual includes; language modules; generated amalgamation | Preserve small textual includes now, record the complete dependency graph, and evaluate Slang modules only where generics/composition remove measured duplication. | Textual preprocessing is simple but recompiles shared text; modules improve composition at toolchain and cache-complexity cost. | Recursive includes, `#pragma once`, recursion detection, and line directives exist. |
| 5. Executable shader type | infer every file; string registration; generated catalog; typed C++ registration | Lean immutable shader class with nested parameters, explicit virtual source/entry/stage, optional used compile hooks, and declaration location. | Authors still state irreducible facts; invalid files and entry points are not guessed. | Mutable `GlobalShaderRegistry` entries with explicit source/entry/stage and repeated package string. |
| 6. Parameter and GPU ABI | hand-authored root signatures/layouts; reflection-only binding; typed parameter metadata | One typed shader-visible declaration drives reflection validation, backend layouts, and binding; reuse it directly or compose it into an RDG pass envelope with graph-only fields. | Code generation/metadata is needed; composition must preserve one owner for every shader-visible field. | Shader `FParameters` and pass parameters are separate; runtime has strong record validation but `PassBinder` retains a count-only compatibility path. |
| 7. Variants | preprocessor permutations; specialization constants; dynamic branches; separate source files | Keep one variant per shader/target in the base migration; design typed permutations only as a later measured extension. | Avoids unused authoring/cache/map complexity now; later variants require one atomic extension. | Free-form defines and specialization records exist; no typed permutation domain. |
| 8. Stage composition | filename grouping; manual package grouping; typed stage types at use | Compute names one shader type; graphics names concrete vertex/pixel types with the real pipeline description; full RT grouping stays in the RT plan. | No universal program layer; genuine multi-stage call sites remain explicit. | Package ID groups stages; `GBuffer` is the only current VS+PS grouping. |
| 9. Pass/RDG declaration | pass strings select bytecode; pass owns shader references; implicit global lookup | Direct compute dispatch consumes `Shader::Parameters`; graphics/shaderless work uses a narrow real-owner envelope; default label derives from shader type with instance override. | Deletes forwarding wrappers while preserving real feature owners. | Pass name, package ID, binding-layout name, pipeline name, and duplicate parameters are repeated. |

### Compilation, Validation, Cooking, and Publication

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 10. Selection | compile all; package/shader selection; changed dependency closure; on-demand runtime compile | Cook all registered shader types for release; use shader-type selection for tools and reverse-dependency selection for development. Do not compile at shipping runtime. | Dependency metadata must be durable, integrity-checked, and regenerated with the current tool contract; development iteration becomes proportional to the edit. | CLI supports all, package, or shader ID. Editor `Changed` and `Global` both launch an unfiltered `cook`. |
| 11. Eligibility | compile every registered target; capability filter before scheduling; runtime compile | Apply target/stage/feature eligibility before one-variant jobs; do not compile at shipping runtime. | Simple bounded selection now; permutation enumeration is deferred. | Target capability skips exist. |
| 12. Preprocessing/dependencies | compiler-owned preprocessing; engine-owned preprocessing; both | Use one canonical Sparkle preprocessing/dependency pass for identity and diagnostics, then invoke the backend with controlled input. Validate that backend include behavior cannot introduce hidden inputs. | Engine preprocessing gives portability and replayability but must track compiler semantics accurately. | Sparkle preprocesses before both DXC and Slang and hashes the include closure. |
| 13. Compile request/input hash | path/timestamp key; package-scoped key; full content-addressed input hash | Immutable request plus `ShaderCompileInputHash` over virtual source closure, entry/stage, environment, compiler-affecting parameter metadata, target, backend, and compiler version. Exclude validation-only and package/pass presentation identity. | Larger hash construction cost; safe in-operation deduplication and deterministic invalidation. | `compileInputHash` combines source, include-closure, option, backend, and compiler-version identity for diagnostics; it is not a persisted-result lookup key. |
| 14. Scheduling/isolation | serial; in-process task parallelism; persistent worker processes; distributed farm | Keep bounded `SparkleTasks` jobs now. Add priority, in-flight dedupe, cancellation, and memory budgets. Add worker processes only when compiler isolation or scale is measured. | Processes isolate crashes/leaks and bypass compiler locks but add IPC, startup, deployment, and debugging cost. | One out-of-process cooker, 1-8 in-process compiler sessions, shallow cancellation, no priority/dedupe/memory ceiling. |
| 15. Front end and intermediate form | DXC HLSL to DXIL/SPIR-V; Slang to DXIL/SPIR-V; source-specific compilers | DXC is the release oracle for HLSL. Slang output is accepted only after the same options, reflection, diagnostics, symbols, and paired-backend tests pass. | Compiler diversity finds issues and enables language features but doubles versioning and reproducibility obligations. | DXC is feature-complete for current HLSL. Slang is narrower and does not honor all cook policy switches. |
| 16. Optimization/code generation | debug/O0; optimized/O3; separate analysis variants; profile-guided/vendor compilation | Cook optimized runtime bytecode; produce replayable debug/symbol artifacts by policy; compare disassembly and resource use for selected acceptance shaders. | Debuggable code may differ from optimized execution; analysis must preserve provenance to avoid comparing the wrong binary. | DXC applies optimization/debug/strip policy; Slang policy parity is incomplete. |
| 17. Reflection and ABI validation | trust hand-authored bindings; runtime reflection; offline reflection plus runtime signature | Extract reflection for every compilation, normalize it, compare against typed parameters, persist a strong signature in cooked output, and revalidate integrity at load. | More cooker work and schema data; binding failures move out of rendering. | Strong cooker/runtime record validation exists; the authoring side still has duplicate parameter authorities. |
| 18. Static shader analysis | compiler warnings only; lint/validation; DXIL/SPIR-V validation; vendor ISA analysis | Warnings-as-errors in owned release shaders, DXIL/SPIR-V validators, bounded statistics, and opt-in RGA/Nsight/PIX analysis for representative hot shaders. | Vendor analysis is hardware/tool-version specific and cannot be a hermetic gate. | Warnings policy and cooked statistics exist; no complete validation/ISA regression lane. |
| 19. Compile diagnostics | console errors; dump every job; dump failures; reproducible job bundle | Structured source diagnostics plus a failure-first replay bundle containing preprocessed source, dependencies, arguments, compiler identity, reflection, and a one-job command. | Bundles consume storage; failure-only default controls bloat. | Successful opt-in DXC bundles exist; failed compiles and Slang do not have equivalent replay artifacts. |
| 20. Compile execution | compile every selected job; deduplicate identical jobs within one cook | Compile every selected shader input and deduplicate identical in-flight jobs only within the active cook. Persist no compiler-result data and expose no storage/configuration surface. | Repeated cooks spend compiler time again; the implementation has fewer owners, formats, invalidation rules, failure modes, and cleanup paths. | The local artifact store, key/status fields, cache directory, CLI flags, and Launcher controls are deleted in the current worktree. |
| 21. Cooked logical map | per-pass file; global shader map; material/asset-local shader maps | Generated `GlobalShaderMap` from shader type/target to code hash and ABI metadata; no base program manifest. | Map indirection gives typed lookup and stable identity. | Per-package `.sparkshader`; generated registry is published but runtime does not use it as lookup authority. |
| 22. Code records and physical library | code embedded per package; indexed records; one library; project/plugin/chunk libraries | Emit exact code hashes and map references. Merge/compress/chunk only after measured byte, I/O, or patch benefit; never expose membership to passes. | A simple index is proportionate; richer libraries add formats and failure modes. | Bytecode is duplicated inside package containers. |
| 23. Compression/chunking | loose uncompressed records; block compression; whole-library compression; platform containers | Keep the first indexed migration format simple. Add independently compressed blocks/chunks only when startup, size, patch, or preload measurements justify them and retain integrity coverage. | Small blocks stream well but compress less; large blocks compress well but amplify reads and patch deltas; uncompressed records may win at current scale. | One container per program; no cross-program compression/chunk policy. |
| 24. Publication/security | overwrite in place; temporary files plus rename; generation manifest; signing | Deterministic transactional generation publication with hashes, schema/toolchain provenance, rollback, and optional platform signing at release packaging. | Keeping generations costs disk; it prevents partial activation and makes failures auditable. | Transactional packages, registry, and recook signal with stale-generation rejection already exist. |

### Runtime Loading, Residency, Pipelines, and Execution

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 25. Open and index | scan loose files; direct computed paths; open map/library indexes | Open and validate active generation map/library indexes once. | Index memory and startup validation buy deterministic lookup. | Runtime computes a path from handwritten package ID and caches loaded packages. |
| 26. Code loading | eager all; synchronous lazy; async preload; memory mapping; streaming | Preserve lazy materialization during graph construction and measure it; do not add preload/streaming state in the base migration. | Minimal new machinery; evidence can justify a later policy. | Per-package code is loaded on first graph materialization. |
| 27. Code lifetime/compression | uncompressed records; transient decompression; retained code; budgeted LRU | Tie map/library/live objects to one runtime generation and GPU retirement. Add compression/eviction only from measured need. | Simple retention is proportionate; extra states are deferred. | Live package/runtime objects follow materialized pass generation. |
| 28. RHI shader/module creation | backend consumes bytecode during pipeline creation; transient module; reusable shader object | Preserve graph-time lazy materialization outside Execute. Treat native shader/module lifetime as backend-specific: D3D12 consumes bytecode for pipelines; Vulkan modules may be transient after pipeline creation. | Retaining native objects may reduce repeated creation but consumes memory and is not portable. | D3D12 consumes bytecode in pipeline creation; Vulkan creates shader modules during runtime materialization. |
| 29. Binding layout/root signature | per-pass hand-authored; reflection-generated; typed ABI-generated and reflection-verified | Generate backend-neutral layout intent from typed parameters, validate cooked reflection, then create/cache native root signatures or pipeline layouts by strong layout signature. | Fully generated layouts constrain bespoke root-layout tuning; allow explicit, reviewed policy hooks rather than parallel declarations. | Cooked layouts and runtime creation exist, but debug names/package lookup and parameter declarations repeat. |
| 30. Logical pipeline descriptor/key | pointer identity; ad hoc hash; complete canonical descriptor | Canonical `RenderPipelineKey` over shader code hashes, layout signature, specialization, and every API-visible fixed-function field. | Canonicalization and completeness are exacting; incomplete keys cause incorrect reuse. | A Vulkan key-shaped value is computed then discarded; no shared renderer pipeline-key authority. |
| 31. Pipeline-description reuse | no cache; per-generation owner; in-memory dedupe; recorded usage database | Keep `RenderPassRuntimeCache` as the sole per-generation owner and preserve its current lazy reuse. Do not add a second descriptor cache or recorded database in the base migration. | This avoids a new authority; measured duplicate creation can justify a later focused cache proposal. | Each pass runtime owns pipelines; there is no cross-pass descriptor cache or recorded database. |
| 32. Native driver cache/binary | rely on opaque driver cache; D3D12 cached blob/library; Vulkan pipeline cache/binaries | Defer all explicit native-cache paths until measured lazy pipeline creation justifies a separate proposal. | Native data is adapter/driver/API-version sensitive. | D3D12 passes an empty `CachedPSO`; Vulkan passes `VK_NULL_HANDLE`. |
| 33. Pipeline compilation/linking | synchronous first use; async creation; partial libraries; shader objects | Preserve synchronous graph-time creation of complete conventional pipelines for the base migration. Defer async creation, graphics-pipeline libraries, partial programs, and shader objects until measured first-use evidence justifies them. | Monolithic pipelines are simple and optimizable but can be expensive to create; the base change avoids adding scheduling and state before that cost is demonstrated. | Graphics/compute pipelines are created synchronously during first runtime materialization. |
| 34. Prewarming policy | compile every PSO; explicit precache; recorded cache; hybrid | Explicitly deferred; add no prewarm state or authoring now. | Avoids unproved startup/runtime complexity. | No precache coordinator. |
| 35. First-use behavior | block; skip; fallback; lazy graph materialization | Preserve current graph-construction materialization, keep it out of Execute, and measure its cost. | A hitch remains possible but is evidence for a later proposal, not a reason to prebuild machinery now. | First use can synchronously create during graph construction. |
| 36. RDG declaration and scheduling | immediate calls; untyped graph node; typed graph pass | `Shader::Parameters` declares resources/queues; graph compilation owns hazards; typed shader lookup resolves before Execute. | Provides lifetime, synchronization, async compute, and diagnostics with less frontend code. | Frame graph already supports typed draw/dispatch but duplicates pass schemas. |
| 37. Binding and dispatch | bind by slot; reflection name; typed parameters | Bind validated shader/layout/pipeline and `Shader::Parameters`, then draw/dispatch. Execute performs no I/O, compile, lookup, layout, or pipeline creation. | Execution stays deterministic. | Current Execute binds and dispatches only. |

### Diagnostics, Iteration, Analysis, and Retirement

| Stage | Options on the table | Recommended choice | Main tradeoff | Current Sparkle |
| --- | --- | --- | --- | --- |
| 38. Semantic labels and provenance | human names only; hashes only; both | Carry readable event/shader names plus stable shader-type, code, pipeline, and generation identities into bounded markers, object names, captures, and crash records. | Extra metadata has storage/runtime cost; bounded development metadata makes evidence joinable without cluttering normal control flow. | Readable names exist, stable cooker-to-capture hash correlation does not. |
| 39. Source debugging | visual output; shader printf/asserts; source debugger; replay capture | Support all four: cheap visualization first, bounded debug instrumentation, external PIX/RenderDoc/Nsight debugging, and reproducible compiler/capture artifacts. | Debug builds and instrumentation alter timing and code generation; conclusions must identify the binary used. | Visualize Buffers and capture labels exist; symbol/provenance workflow is incomplete. |
| 40. Performance analysis | compiler statistics; IR/disassembly; vendor ISA; GPU counters/timelines | Preserve DXIL/SPIR-V inspection, add selected RGA/Nsight analysis, and correlate static register/LDS data with runtime occupancy, cache, bandwidth, and latency evidence. | Static estimates do not prove runtime bottlenecks; hardware results are device/driver specific. | Cooked byte/reflection stats exist; no joined static/runtime shader evidence. |
| 41. Hot reload invalidation | reload all; timestamp scan; content/dependency invalidation; runtime patch-in-place | Changed virtual paths select reverse dependencies; produce a complete validated generation; atomically swap; never patch individual live entries. | Whole-generation publication duplicates some data briefly but keeps rollback and lifetime coherent. | Timestamp polling notices `.hlsl`, `.hlsli`, and `.slang`; the all-catalog plan recompiles every job; generation swap is safe. |
| 42. Cancellation/failure | abort process; cooperative job cancellation; keep partial output; transactional rollback | Cooperative cancellation at job/backend/process boundaries; discard incomplete generation; keep previous active generation; initial startup failure is explicit. | Some compiler calls are not interruptible; process isolation may be needed to bound cancellation. | Child process can be cancelled, but running compiler sessions are not cooperatively stopped. Rollback is strong. |
| 43. Lifetime/retirement | device idle; immediate destroy; queue-fence/token retirement | Reference resources through the active generation and retire old shader/pipeline/library state only after every recorded queue submission token completes. | More bookkeeping and transient overlap; no global device-idle reload hitch. | Implemented by `RenderPassRuntimeCache`; preserve it. |
| 44. Release evidence | successful compile; one smoke frame; paired deterministic evidence | Prove clean and repeated cooks, repeated-operation recompilation, map/library lookup, lazy materialization cost, D3D12/Vulkan captures, hash-to-source lookup, representative disassembly, fallback behavior, and latency/memory. | Evidence takes hardware time and storage. | Catalog validation and one representative CLI artifact exist; paired hardware evidence is incomplete. |

The recommended base path is intentionally conventional at the API boundary: offline high-level compilation, typed maps/libraries, conventional complete pipelines, and lazy graph-time materialization outside Execute. Asynchronous preparation, Vulkan shader objects, graphics pipeline libraries, Vulkan pipeline binaries, D3D12 partial programs, work graphs, and distributed compilation remain options, not assumed improvements. Each adds a capability branch and requires its own proposal only after current pipeline counts, first-use timings, and a representative workload show that the simpler path is insufficient.

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
| compile execution | selected jobs; worker bound; cancellation | every selected job compiles; 1-8 bounded compiler sessions; cancellation is checked before job execution | Preserve compile-every-time behavior and add only in-operation identical-job fan-out. |
| compile policy | debug info; optimization; warnings-as-errors; strip debug | DXC applies these controls; Slang currently does not apply equivalent policy | One canonical request must either be honored or rejected as unsupported by every backend. Never silently ignore release policy. |
| analysis | none; debug-artifact directory; `cooked-shader-stats` | DXC success can emit source/arguments/disassembly/debug data; Slang is narrower; failures have no full bundle | Failure-first portable replay bundles plus optional successful analysis and backend-specific extensions. |
| task execution | serial through 8 sessions | bounded tasks, one backend instance per node, cancellation before job start | Add job priority, dedupe/fan-out, cancellation boundary, timings, and memory budget before adding another worker pool. |
| publication | selected package files + registry + signal | staged files publish transactionally; runtime still resolves manual paths | Publish a complete generation manifest and make it the lookup authority. |
| runtime backend | D3D12/DXIL; Vulkan/SPIR-V | one runtime-format binary/layout is selected and strongly validated | Preserve backend-neutral shader-type/map identity and paired-backend validation. |
| runtime creation | first materialization; replacement generation | package/layout/pipeline creation is lazy during graph construction; generation replacement is eager-validation plus atomic swap | Preserve lazy graph-time materialization and safe generation swap; measure first use without adding readiness or preload state. |
| render feature | raster/ray-query GBuffer; ReSTIR/reference lighting; exposure method; debug/presentation/upscaler branches | frame CVars/settings choose producers; package catalog itself does not prove a branch is consumed or a fallback works | Generate a shader-use inventory and test each supported branch/fallback on both backends. |

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

## Runtime Residency and Deferred Delivery Taxonomy

Compile inputs, cooked runtime data, live backend objects, and native driver acceleration have different owners and lifetimes. Sparkle persists no compiler result. The first four rows are base-migration responsibilities; streaming, explicit native pipeline persistence, and prewarming remain deferred research.

| Layer | Prevents or reduces | Correct key/invalidation owner | Recommended policy | Current Sparkle |
| --- | --- | --- | --- | --- |
| dependency/preprocess record | rescanning and re-deriving include closure | virtual source contents, preprocessor provenance, defines; ShaderCompiler | persist a reverse-dependency index; regenerate when the current tool contract or integrity check rejects it | closure is rebuilt and hashed; reverse graph is not a durable selection authority |
| in-flight job coalescing | duplicate concurrent compilation | full `ShaderCompileInputHash`; compile coordinator | one producer with result fan-out, priority escalation, and cancellation reference counts; retain nothing after the operation | absent |
| global shader map | repeated discovery and unstable lookup | shader type/target; ShaderCompiler/Renderer generation | immutable per-generation entries with strong references to code hashes | package registry published but not runtime authority |
| cooked code records/library | duplicate bytecode and scattered I/O | exact code hash; ShaderCompiler/Renderer generation | independently indexed and integrity checked; merge/compress only after measured benefit | bytecode embedded per package |
| OS/file/decompression cache | disk reads and decompression | physical records/chunks; platform I/O/runtime service | measured reads; add block compression and decompressed residency only if the selected format needs them | ordinary per-file reads; no explicit streaming/decompression layer |
| code and transient backend-object reuse | repeated code reads or backend object creation | code hash + backend creation identity; runtime/RHI | code/object lifetime follows map/runtime generations; keep the current lazy owner and add no second reuse authority without evidence | live objects are owned by each materialized pass runtime |
| canonical pipeline descriptor reuse | duplicate logical pipeline requests | complete `RenderPipelineKey`; Renderer | deferred; first measure current generation-bound lazy materialization | absent |
| recorded/stable PSO database | unknown content-driven PSO enumeration | stable high-level descriptor plus build/platform version; Renderer/cooker | add only when content creates combinations that bounded declarations cannot enumerate | absent |
| native pipeline cache/library/binary | repeated driver compilation/linking | native API/device/driver/cache UUID identity; RHI backend | validated, transactionally persisted, recoverable miss; never source authority | D3D12 empty cached state and Vulkan null cache |
| opaque driver internal cache | vendor-specific repeated work | driver-owned | treat as helpful but unobservable/non-portable; do not use as readiness proof | implicitly relied upon |

The accepted base runtime model is deliberately smaller:

1. At generation activation, open and validate maps/library indexes without creating every pipeline.
2. Resolve code and lazily materialize binding layouts/pipelines during graph construction, never Execute.
3. Measure map/library open, code read, layout creation, and pipeline creation time with existing evidence surfaces.
4. Refuse release while an active/reloading generation or recorded GPU submission can still reference required state.

Do not add a residency state machine, preload queue, or streamer before file count, startup I/O, code size, memory, or a measured first-use hitch demonstrates a problem.

### Deferred PSO Prewarming Research

The strategies below are retained only as future decision context. None is selected or implemented by the base phases.

| Strategy | Strength | Cost/failure mode | Recommendation |
| --- | --- | --- | --- |
| compile synchronously on first use | minimal startup and enumeration work | visible frame hitch and nondeterministic driver work | Reject for required or common pipelines. Retain only as classified late fallback. |
| enumerate every theoretical PSO | strongest nominal coverage | combinatorial cook/startup/memory bloat and unused work | Reject. A future proposal may consider only measured, reachable pipeline descriptions. |
| explicit precache declarations | deterministic and reviewable for bounded shaders | authors/collectors can omit a legal state | Defer until measured first-use evidence requires it. |
| recorded/bundled usage cache | captures content combinations actually exercised | misses unplayed branches and can go stale across shader/content changes | Add with material/content PSOs; use stable descriptions, never raw transient hashes alone. |
| native driver cache only | little engine work | opaque coverage, device/driver invalidation, first-run hitches | Use as an acceleration layer, never as the only plan. |
| hybrid explicit + recorded + native | covers known global state, observed content, and repeated driver work | most coordination and telemetry | Research option only; far beyond current need. |

A future prewarming proposal must define its own minimal state/evidence contract. This migration adds none of those states or diagnostics.

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
| library/pipeline granularity | one large library/RTPSO; collections/libraries plus link; one pipeline per effect | In the separate RT plan, begin with one small complete effect pipeline whose descriptor names concrete RT shader-class references; do not add a universal `TRayTracingProgram` authoring type. Measure compile/link before introducing collections. | Large pipelines maximize shared optimization but are slow and hard to replace; many small pipelines duplicate work and switches. |
| shader exports and hit groups | string lists; generated typed IDs; reflection discovery | Typed declared exports/hit groups validated against compiler output; persist stable logical IDs but query native identifiers per native pipeline. | Native shader identifiers/group handles are pipeline-specific and cannot be serialized as universal shader identity. |
| global/local parameters | all global descriptors; local root/record data; bindless indices/device addresses | Keep most resources in global/bindless tables; keep SBT record data to small stable indices/constants only. | Fat local records are simple per hit but multiply SBT memory, upload bandwidth, and update complexity. |
| SBT organization | per instance; per geometry/material/ray type; deduplicated records with indirection | Explicit formula and one ray type in the first vertical slice; use TLAS instance contribution plus geometry/ray offsets; deduplicate material/geometry data outside SBT. | Indirection reduces memory and churn but adds shader loads and indexing complexity. |
| SBT update | rebuild every frame; patch dirty ranges; persistent GPU-generated table | CPU-build an immutable/persistent table per validated scene generation first; add dirty-range or GPU generation only from measured update cost. | Full rebuild is simple but scales poorly; incremental/GPU updates complicate synchronization and validation. |
| alignment/layout | backend-specific code paths; one conservative cross-API layout; normalized builder with backend rules | One backend-neutral record builder that applies D3D12/Vulkan handle size, base alignment, record alignment/stride, region, and bounds rules explicitly. | A conservative maximum wastes memory; backend-specific packing needs paired tests. |
| pipeline/SBT lifetime | SBT independent; rebuild on pipeline change; cache native handles | Tie SBT records to the exact native RT pipeline generation whose identifiers/group handles they contain; retire both after GPU completion. | Rebuild/upload on pipeline reload is mandatory, but stale identifiers cannot execute. |
| recursion/stack | maximum device limits; fixed conservative values; shader-derived measured policy | Start with recursion depth 1 and explicit payload/attribute contracts; query/report stack information and increase only for a demonstrated algorithm. | Higher recursion/payload/stack improves expressiveness while reducing occupancy and raising memory/driver cost. |
| dispatch integration | direct command calls; neutral RHI `TraceRays`; RDG typed RT pass | Add backend-neutral RT pipeline/SBT descriptors and a typed frame-graph ray-dispatch pass in one vertical slice. | Partial schema support without command/execution ownership creates misleading dead infrastructure. |

The current cooked schema already models RT exports, hit groups, local parameter records, payload/attribute sizes, and recursion metadata. That is useful compiler-only scaffolding, but it is not a runtime feature. Before changing the runtime rejection, Sparkle needs all of these together in the separate RT plan: Renderer RT shader classes and typed stage references; DXIL and SPIR-V compiler capability parity or an explicitly platform-limited contract; native D3D12 state-object and Vulkan RT-pipeline creation; group identifier retrieval; SBT construction/alignment/indexing; RHI command recording; graph resources and synchronization; pipeline/SBT generation lifetime; a non-RT accepted fallback; and paired execution/capture tests. This base shader-authoring migration neither adds RT pipeline composition nor requires precache telemetry.

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
| [Repository ownership](../../Engineering/Standards/RepositoryStructureAndOwnership.md) and [Renderer/RHI boundary](../RendererRhiBoundary.md) | **Partial.** Renderer owns concrete registrations and passes; Tools owns cooking; RHI owns neutral pipeline creation and cooked validation. The RHI-facing `GlobalShader` authoring layer still carries source/package policy that should belong to the Renderer/tooling contract. | Keep neutral bytecode, reflection, layout, shader-object, and pipeline contracts in RHI. Keep concrete shader classes, nested parameters, graph use, and runtime generation in Renderer. Keep virtual-source resolution, compiler execution, map/library publication, and cooker policy in Tools. Application/editor only orchestrates recook and activation. |
| [Graphics engineering](../../Engineering/Standards/GraphicsEngineering.md) | **Partial.** DXIL/SPIR-V compilation, reflection, package validation, backend capabilities, readable labels, and runtime-format selection exist. Paired inspection, disassembly/counters, fallback captures, and exact hardware/driver evidence are not automated. | Make a paired-backend vertical slice the first proof; inspect layouts and IL on both targets; preserve a named fallback; record exact compiler, backend, hardware, driver, workload, and capture. |
| [Editor and tools](../../Engineering/Standards/EditorAndTools.md) | **Partial to strong.** Cooking is out of process; publication is transactional; stale generations are rejected; previous accepted artifacts survive failure. Cancellation is shallow, compiler memory is unbudgeted, and compile failures lack replay artifacts. | Preserve transactional replacement. Add job cancellation boundaries, bounded compiler-session memory/parallelism, progress/results integration, deterministic failure bundles, and a stable command/API usable outside the editor. |
| [Concurrency](../../Engineering/Standards/Concurrency.md) | **Partial.** Bounded work uses `SparkleTasks`, with no second general pool. It lacks priority, in-flight dedupe, compiler-session cancellation, serial-vs-N evidence, and memory-ceiling proof. | Evolve cook nodes into explicit jobs coordinated through `SparkleTasks`; never add a shader-only general worker pool. Prove serial, 1/2/N, cancellation, stale generation, failure, and memory behavior. Add worker processes only from measured compiler isolation/throughput need. |
| [Data-oriented design](../../Engineering/Standards/DataOrientedDesign.md) | **Partial.** Cook plans, reflection arrays, and compact cooked records are batch-friendly, but runtime lookup is string/path-led and physical packages duplicate bytecode. | Use immutable sorted manifest/map records and content-addressed code tables; keep human-readable strings in diagnostics, not hot lookup or cache identity. Measure layout/memory changes rather than asserting them. |
| [Naming and vocabulary](../../Engineering/Standards/NamingAndVocabulary.md) | **Partial.** Neutral RHI uses `RenderPipeline`, `GraphicsPipelineDesc`, and `ComputePipelineDesc`, while semantic pass labels are useful. Authored package IDs and generic-looking `PassName` fields conflate identities. | Derive the default event label from the concrete shader class or call site, keep an explicit label overload for real pass instances, distinguish shader type, compile input, code, map entry, and pipeline identities, and reserve stable hashes for content/cache identity. Do not leak `PSO` into neutral public RHI names. |
| [Validation, performance, and evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md) | **Missing as a complete gate.** One representative CLI target exists, but no paired all-shader validation, injected-defect proof, compile-time report, first-materialization evidence, or external capture pack was found. | Extend existing product/tool validation surfaces for every replaced contract, use temporary removed-before-handoff harnesses only where necessary, then run the paired DXIL/SPIR-V cook/load slice, negative ABI/compile/reload cases, serial/N matrices, first-materialization measurements, and capture-backed evidence. A document or successful build is not acceptance evidence. |
| Frame-graph execution ownership | **Partial to strong.** RDG setup declares resources and materializes runtime state before Execute; Execute only binds and records work. Duplicate parameter metadata and count-only compatibility weaken the dependency/binding proof. | Make one schema own each shader-visible field, reuse/compose it in the pass envelope, and reject structural mismatch; never permit Execute to discover resources, load shaders, or create pipelines. |
| Reload and GPU lifetime | **Meets the reviewed invariant.** A replacement generation is fully built before activation, a failure preserves the active generation, and retirement waits on `RhiSubmissionToken` state for all queues. | Preserve this path unchanged while replacing lookup and physical storage. Test delayed GPU completion, reload churn, invalid replacements, and device-loss/error paths. |

### Portfolio Requirement Contribution

The shader architecture directly owns `PGE-09` and contributes evidence to several broader requirements. It cannot alone complete requirements that also need scene, workload, GPU-capture, productization, or adoption evidence.

| Requirement | Current contribution | Missing proof or design gate |
| --- | --- | --- |
| `PGE-01` Partner adoption and collaboration | **Indirect partial.** CLI/editor workflows and architecture notes can become an adoption surface; no second-engineer shader integration record is proved here. | Capture adopter constraints, review history, setup/cook/debug/fallback steps, measured outcome, issue/reproducer, and second-person reproduction without requiring hidden repository knowledge. |
| `PGE-02` Real-time ray tracing, GI, and path tracing | **Partial.** Sparkle has BLAS/TLAS and several path/ReSTIR compute programs using inline ray query, but no full RT pipeline/SBT and no verified no-ray/device-address shadow selection. | Prove raster, ray-query, and accepted fallback behavior under Bistro/San Miguel quality, temporal, latency, memory, and paired-capture gates. Add a full RT pipeline only through the complete conditional vertical slice. |
| `PGE-03` Neural graphics product feature | **No shader-lifecycle feature proof.** The architecture can carry future generated or fixed inference shaders, but it does not provide a trained model, runtime inference, or classical fallback. | Reuse this exact shader-class/map/library/ABI/pipeline/provenance path for a real model artifact and shader kernels; do not create a neural-only compiler/runtime authority. Evidence remains owned by the neural workload. |
| `PGE-04` Model-to-kernel translation | **Future contribution only.** Slang is a backend seam, but no model-to-shader generator, operator contract, or generated-kernel runtime was found. | Use provenance-recorded generated virtual sources/source maps, deterministic regeneration, the normal compile request and ABI validation, numerical reference checks, latency/memory, disassembly/counters, precision/layout/fusion decisions, and classical fallback. |
| `PGE-05` Whole-system performance | **Partial.** Package sizes and runtime load microseconds exist; compile, map/library loading, native pipeline, and hitch distributions do not. | Record compile queue/wall/CPU time for every selected job, map/library open time, lazy pipeline creation, frame pacing, memory high-water, and p50/p95/p99 under a pinned workload. Native-cache, streaming, and preloading experiments belong to later measured proposals. |
| `PGE-06` Workload analysis and hard debugging | **Partial.** The catalog targets DXIL/SPIR-V and both backends expose markers/debug names, but captured shaders are not joined to compile provenance. | Capture the same workload on both APIs; inspect queues, barriers, descriptors, memory, pipelines, shaders, symbols, and one hard incident with hypotheses, experiments, root cause, and minimal reproducer. |
| `PGE-07` C++ and Python software engineering | **Partial.** A C++ CLI, out-of-process orchestration, transactional cook, and runtime validation exist; no useful Python shader-analysis automation is required or proved by this design alone. | Keep the C++ ownership narrow and tested; add Python only for a concrete report/conformance/analysis workflow; provide clean-clone commands, deterministic artifacts, and documentation matching executable behavior. |
| `PGE-08` Applied mathematics and modeling | **Indirect.** Shader infrastructure cannot prove estimator, signal-processing, stability, or cost mathematics. | Let shader-type metadata link to the owning feature's math/reference tests and preserve exact compile-input/code/capture identity so predicted cost/quality can be compared with measurement. |
| `PGE-09` Explicit APIs, shaders, compilers, and GPU ABI | **Partial.** Explicit D3D12/Vulkan, HLSL to DXIL/SPIR-V, reflection, cooked ABI validation, and diagnostics exist. | Produce a paired shader-source-to-runtime trace; inspect both compiled forms; prove layout/resource states and complete support matrix; inject defects; verify real fallbacks; join code/pipeline hashes to GPU events. |
| `PGE-10` CPU/GPU architecture and concurrency | **Partial.** Bounded cooker tasks and async-compute scheduling exist. | Compare serial/1/2/N compile execution with time/memory/cancellation; correlate IR/ISA register/LDS/scratch findings with runtime occupancy, divergence, cache/bandwidth, and synchronized queue evidence. |
| `PGE-11` Machine-learning fundamentals | **Out of shader-lifecycle scope.** No compile/package design demonstrates training, objectives, splits, optimization, quantization, or generalization. | Do not claim coverage. A future generated shader path consumes an independently validated frozen model artifact and records provenance; training evidence stays in its owning workflow. |
| `PGE-12` Training and inference workload engineering | **Partial infrastructure only.** Cooked packages are versioned, validated, atomically published, and lazy-loaded, which can support deterministic inference deployment; no real inference workload exists. | Measure export, cook, map/library open, lazy materialization, and inference latency/memory separately from training; preserve an explicit classical fallback under one normal runtime path. Variant and preload policy remain outside this base shader migration. |
| `PGE-13` Productization, tools, and communication | **Partial.** CLI discovery/inspection, editor recook, and this source-linked design are credible beginnings. | Deliver edit-to-failure/replay/reload/trace workflows, stable navigation, clean cook/run, troubleshooting, bounded reports, adoption feedback, and deletion evidence for replaced concepts. |
| `PGE-14` Platform and ecosystem breadth | **Partial.** Windows D3D12/Vulkan code paths and compiler/tool references exist; native Linux/Vulkan behavior is not proved by this document. | Record OS, SDK, compiler, driver, capture/profiler, and build setup. Add native Linux/Vulkan cook-run-capture only before claiming it; keep platform limitations in the support matrix. |
| `PGE-15` Principal judgment and sustained influence | **Design target, not proof.** The proposal removes repeated authority, rejects premature streaming/RT/compiler complexity, and selects measured gates. | Demonstrate completed vertical slices, deleted old paths, fewer authored concepts, preserved capability/error quality, causal evidence, review/adoption, and a repository that became easier to explain and maintain. |

### End-to-End Lifecycle Verdict

| Lifecycle stage | Verdict from reviewed code | Recommended end state | Acceptance evidence |
| --- | --- | --- | --- |
| Write shader source | **Partial.** HLSL/HLSLI and recursive includes work, but physical search roots, absolute includes, and project-first shadowing define identity. | Canonical `/Engine`, `/Project`, and `/Plugin/<Name>` mounts; deterministic include ownership; no authored absolute path. | Mount collision, traversal, case policy, same-basename, source-move, and cross-checkout key tests. |
| Declare shader type | **Partial.** Explicit source, entry, stage, feature flags, and parameter descriptor exist. Static registration silently drops duplicates and freezes implicitly on first snapshot. | Lean immutable shader class with nested `Parameters`, declaration location, explicit catalog freeze, collision errors, and only compile hooks consumed by the base implementation. | Duplicate/late registration negative tests and a readable catalog dump. |
| Declare parameters and RDG resources | **Unsafe partial.** Typed pass resources drive graph declarations; a separate shader `FParameters` drives reflection; count-only binding compatibility can accept different layouts. | One schema owns every shader-visible field and is reused directly or composed into a pass envelope with graph-only fields; binding and structural signatures derive from that schema. | Direct one-shader, graph-only/copy, and composed-pass tests; a field reorder/kind/name/visibility/array/size defect fails before execution on both backends. |
| Name executable stages | **Partial.** Shared package strings group stages and allow the valid multi-file `GBuffer` case. | Compute dispatch names one compute shader class; graphics draw names the concrete vertex/pixel classes and complete pipeline state. Full RT grouping remains in the RT plan. | Direct compute and VS+PS graphics tests; no authored package string, program alias, or pass-registration macro. |
| Select permutations | **Explicitly deferred.** Variants are represented through free-form defines and separate registrations/packages. | Keep one registered variant per shader/target in the base migration and add no permutation frontend, enumeration, callback, or cache dimension. | Final base searches prove no new permutation API or policy; a future proposal must provide its own workload, owner map, and acceptance evidence. |
| Build compile input and dependencies | **Partial.** Options and transitive include contents are hashed, but physical directories and path bytes leak into identity. | One immutable compile request with virtual source identity, normalized dependency graph, compiler provenance, platform/features, parameter signature, and debug policy. | The same source tree in different checkout roots produces the same input hash; every meaningful input change invalidates it. |
| Schedule compilation | **Partial.** Bounded `SparkleTasks` execution works. | Explicit logical job identity, `ShaderCompileInputHash`, result, priority, in-flight/completed dedupe, cooperative cancellation, bounded result application, and measured worker/session budget. | Serial/1/2/N, duplicate fan-out, cancellation, backend crash/failure, stale result, and memory-ceiling tests. |
| Compile DXIL/SPIR-V | **Strong foundation, incomplete parity proof.** DXC and Slang expose target capabilities; DXC produces DXIL/SPIR-V and rich successful analysis artifacts; reflection is extracted for both formats. | A backend-neutral result contract with equivalent diagnostics/provenance and declared analysis capability differences. | Representative optimized DXIL and SPIR-V builds for all supported stage kinds; reflection/layout comparison and compiler-version record. |
| Enforce compiler policy/capabilities | **Unsafe partial.** Target/package capability filters exist, but DXC and Slang do not honor the same policy controls and schema-known stages exceed executable runtime support. | Generated matrix for language, backend, target, stage, package kind, feature, and debug/optimization/warning/symbol policy; unsupported requests fail before jobs. | Every matrix cell is produced by a capability probe and executable positive/negative test; no ignored policy or unreported target skip. |
| Compile selected shader inputs | **Simplified in the current worktree.** Every selected job invokes the compiler; the local artifact store and its key/status/configuration surface are deleted. | `ShaderCompileInputHash` is independent from package/pass presentation identity and exists only for in-operation deduplication, diagnostics, and provenance. | Repeated identical cooks both compile; identical jobs within one operation fan out one result; cancellation and deterministic publication remain proven. |
| Cook and publish runtime data | **Strong publication, coarse storage.** Per-program packages, registry, and recook signal publish transactionally; code is duplicated and registry is not runtime authority. | One generated `GlobalShaderMap` plus unique code-hash library; physical file policy remains cooker-owned and invisible to Renderer callers. | Reproducible map/library hashes, one copy per unique bytecode, transactional rollback, and map/library validation. |
| Inspect and reproduce compilation | **Partial.** Catalog/package inspection and successful opt-in artifacts exist. | One trace command plus always-available failure bundle containing virtual dependencies, preprocess output where available, exact arguments/defines, compiler/version, parameter comparison, and replay command. | Injected syntax/include/ABI/backend failures replay outside the editor and navigate to portable virtual paths. |
| Load and validate runtime shader | **Strong.** Cooked schema, target format, hashes, records, stages, features, reflection, and logical layouts are validated before use. | `ShaderRef<Shader>` resolves through the active `GlobalShaderMap` and code hash, independent of physical library filenames. | Cold/warm map/library open time and memory, corrupt/truncated/wrong-backend/wrong-layout records, and typed lookup across generation replacement. |
| Prepare code and native objects | **No additional owner required in the base migration.** Current materialization is lazy and pass-runtime-owned. | Keep `RenderPassRuntimeCache` as the sole generation/lifetime owner, materialize during graph construction, and keep Execute free of loading or creation. Add preload/readiness only in a future measured proposal. | Map/library/native-object/pipeline timing and high-water evidence; backend-specific release tests; proof that Execute performs no loading or creation. |
| Integrate with RDG | **Partial to strong.** Resource uses are declared during setup; runtime is materialized before Execute; Execute only binds and records. | `Shader::Parameters` is the one shader-visible schema consumed directly by typed draw/dispatch; only real graph-only data uses a narrow owner envelope; optional per-instance event labels remain presentation. | Direct compute, graphics, composed, and shaderless pass tests; graph resource-state explanation, async-queue legality, no hidden creation in Execute, and meaningful capture markers. |
| Create/use graphics and compute pipelines | **Correct lazy path, unmeasured first-use cost.** Complete descriptors reach RHI and first graph use synchronously creates native pipelines. | Preserve complete descriptors and graph-time lazy materialization under `RenderPassRuntimeCache`. Do not add descriptor caches, precache policy, or native cache integration in this migration. | Cold/warm D3D12 and Vulkan creation timings, first-use frame impact, descriptor completeness, generation reuse, and feature preservation. |
| Debug/profile on GPU | **Partial.** Semantic events and native object names exist; compiler artifacts are not joined to captured shader hashes or external symbols. | Stable capture correlation record from event label and pipeline identity to shader type, code hash, virtual source, compile request, and debug symbol. | PIX and RenderDoc paired captures plus Nsight or RGA analysis on the exact cooked shader; record counters, IL/ISA, hardware, driver, API, and workload. |
| Recook and hot reload | **Strong foundation, coarse invalidation.** Out-of-process cook, transactional signal, stale rejection, rollback, generation swap, and GPU-safe retirement exist; any change plans the whole catalog. | Persist reverse dependencies, select affected shader types, publish a complete new generation, and retain current rollback/lifetime behavior. | Root/include change selection, unrelated-shader exclusion, rapid edit coalescing, invalid replacement, delayed completion, and reload-churn tests. |
| Inline ray-query capability/fallback | **Live primary path, unproved alternatives.** Compute shaders use TLAS/inline queries, while the no-query and device-address shadow passes have no frame selection consumer. | One capability-selection owner chooses descriptor, supported device-address, or accepted no-ray behavior; each branch has complete feature/ABI validation. | Forced-capability paired tests/captures prove selection, resources, output, and failure reason for every advertised branch. |
| Full RT pipeline and SBT | **Explicitly deferred at runtime.** Schema/cooker inspection exists; no renderer RT registrations, native state-object/pipeline, group identifier, SBT, or trace-rays command path exists. | Add only as one paired vertical slice with typed exports/hit groups, layouts, payload/attribute/recursion/stack policy, native pipeline, SBT builder/index formula, RDG command/lifetime/cache integration, and selected fallback. | Raygen/miss/hit execution on D3D12/Vulkan; identifier-generation, alignment/index/bounds, reload/retirement, corruption, fallback, cold/warm pipeline, SBT memory/update, and capture evidence. |
| Automated conformance | **Missing as a complete gate.** The CLI validation custom target covers catalog structure and one shader artifact only. | Extend existing tool/product validation and CI evidence gates for identity, dependency, compilation, layout, schema, and paired end-to-end cook/load/reload/pipeline behavior; use temporary local harnesses only when an existing surface cannot falsify the invariant. | Named executable checks, injected failures, all-shader catalog cook, paired backends, `architecture_boundary_check`, clean diff/build evidence, and no submitted test-only scaffold unless separately authorized. |

### Required Evidence Pack

Implementation is not accepted merely when the new API compiles. The completed shader lifecycle must produce one navigable evidence pack for a pinned engine commit and workload:

1. an authoring trace from the concrete shader class and nested `Parameters` to virtual source, entry, parameter signature, compile-input hash, code hash, global-shader-map record, runtime shader reference, pipeline identity, and GPU event;
2. optimized DXIL and SPIR-V artifacts for representative graphics stages and compute shaders, with reflection/layout comparison and readable compiler diagnostics;
3. one deliberately broken binding contract and one deliberately broken compile/include, both rejected with portable source locations and replayable failure bundles;
4. repeated cook results for serial, 1, 2, and N workers, including selected/compiled job counts, wall/CPU time, peak memory, cancellation, in-operation deduplication, and identical-output checks;
5. cold and warm D3D12/Vulkan pipeline results, including graph-time creation, generation reuse, first-use frame impact, and proof that Execute performs no loading or creation;
6. paired PIX and RenderDoc captures for the same representative scene and settings, plus Nsight or RGA shader analysis where the finding requires source/IL/ISA counters;
7. exact engine commit, global-shader-map/library hash, compiler backend and version, target, optimization/debug policy, GPU, driver, API, scene, camera, resolution, warm-up, capture frame, and run count;
8. editor adoption evidence: edit, changed-dependency selection, successful reload, failed replacement rollback, direct error navigation, one-job replay, and GPU-safe old-generation retirement;
9. a clean build/cook/run from documented commands and a recorded issue or minimal reproducer created by a second adopter;
10. before/after authored-string, registration, parameter-declaration, forwarding-pass, compile-job, unique-code, cooked-code/library-byte, cook-time, map/library-open, and first-materialization counts so the simplification claim is measurable;
11. a generated support matrix proving backend/target/stage/shader-type/feature/policy status and a consumer report distinguishing registered, cooked, runtime-valid, selected, and captured shaders/fallbacks;
12. runtime-generation and lazy-materialization lifetime evidence, including only compression/eviction metrics that exist. If full RT is later in scope, its separate plan adds state-object/pipeline, SBT layout/index/update/memory, trace dispatch, fallback, reload, and paired capture evidence.

The [performance diagnostics architecture](../Performance/Diagnostics/PerformanceDiagnosticsArchitecture.md) owns the shared measurement and capture infrastructure. This document owns the shader-specific identities and joins that make those captures traceable. Evidence records belong under the repository's evidence path selected by the acceptance workload; they must not be embedded here as claims that age with hardware, drivers, or compiler versions.

## Target Capability Requirements

These requirements describe the accepted base migration. They intentionally exclude permutations, PSO precaching/prewarming, preload/streaming controls, native driver caches, and full RT execution.

### 1. Canonical Virtual Shader Sources

Create one immutable `ShaderSourceMountTable` with `/Engine`, `/Project`, and `/Plugin/<Name>` roots. Registrations and root includes use canonical virtual paths. Relative includes resolve from the including virtual file. Physical paths are read locations only and never portable identity. Unknown/overlapping mounts, absolute authored paths, mount escape, duplicate virtual paths, case-policy collisions, and late mounts fail deterministically.

### 2. One Lean Shader Class Contract

Replace the current mixed `F*`/`T*` Unreal-prefix vocabulary with Sparkle names such as `GlobalShader<Shader>` and `ShaderRef<Shader>`. A concrete shader class owns:

- nested `Parameters`, declared inline for the normal case or aliased to one shared schema only when several shader stages truly share it;
- optional compile eligibility, environment mutation, and compiled-result validation hooks only when that shader uses them;
- ray-tracing payload/attribute/export metadata only for a ray-tracing shader that requires it.

`IMPLEMENT_GLOBAL_SHADER(Class, VirtualSource, Entry, Stage)` remains the one implementation declaration. Sparkle does not need separate `DECLARE_GLOBAL_SHADER` or `SHADER_USE_PARAMETER_STRUCT` macros because its CRTP base and implementation declaration already know the concrete type and parameter owner. Raw source scanning never substitutes for typed registration.

### 3. Frozen Shader Catalog

`ShaderTypeDesc` records deterministic `ShaderTypeId`, readable type/stage, declaration location, virtual source/entry, `Parameters` metadata/signature, capability requirements, and optional compile hooks. Collect declarations, validate collisions, sort, freeze, then query. Duplicate or late declarations report both source locations and fail; static initialization order is not catalog order.

### 4. One Parameter Schema from Graph Setup through Binding

For a one-shader pass, `Shader::Parameters` is simultaneously the graph dependency declaration, parameter storage, reflection contract, structural signature, and runtime binding input. `FrameGraphBuilder::AllocParameters<Shader>()` allocates that exact nested type. Inputs/outputs use typed graph resource wrappers, so access and lifetime derive from the fields the author fills.

A real multi-stage graphics operation may compose `VertexShader::Parameters`, `PixelShader::Parameters`, and graph-only attachments in one small envelope. A shaderless copy/clear pass owns graph-only parameters. No envelope restates a shader-visible field. Delete count-only compatibility and report the first structural mismatch before pipeline creation.

### 5. Thin Typed Graph Dispatch

Provide Unreal-like focused entry points over existing owners:

- `Dispatch<Shader>(parameters, groupCount)` and async equivalent for compute;
- an optional leading `RenderPassLabel` only for a distinct graph instance;
- typed graphics draw helpers that take the actual vertex/pixel shader types, complete pipeline description, parameters, and draw collaborator/data;
- no package/program/layout/pipeline strings at the call site.

The helper resolves `ShaderRef<Shader>` from the active `GlobalShaderMap`, derives the default label from the shader type, declares resources from `Shader::Parameters`, asks `RenderPassRuntimeCache` for the generation-bound layout/pipeline, and records binding/dispatch later through `PassCommandContext`. Delete one-method pass classes, duplicate `*PassParameters`, `GetDefinition`, `GetParameterMetadata`, and forwarding Execute bodies where the shader type and graph helper express the entire operation. Keep focused pass collaborators only when they own real mesh/draw/feature behavior.

### 6. Reproducible Compile Jobs without Persistent Compiler Results

Create one immutable `ShaderCompileRequest` and `ShaderCompileJob` per `(ShaderTypeId, Target)`. `ShaderCompileInputHash` covers the virtual source closure, entry, stage, compiler-affecting parameters/environment, target, backend/tool provenance, and compile policy. It excludes pass labels, packages, pipelines, and presentation strings.

Replace package-shaped cook nodes with jobs on the existing `TaskExecutor`. Keep one out-of-process cooker, bounded compiler sessions/memory, deterministic ordering/result merge, cancellation settlement, and in-flight duplicate fan-out. Every selected input invokes the compiler; `ShaderCompileInputHash` is identity and provenance, never a persisted-result lookup key. There is no compiler-result store, cache directory, enable/disable flag, storage schema, cache browser, or cleanup operation.

### 7. Global Shader Map and Cooked Code Library

Cooking publishes one deterministic generation:

1. `GlobalShaderMap`: sorted `(ShaderTypeId, Target) -> ShaderCodeHash + parameter signature + required runtime metadata`.
2. `CookedShaderLibrary`: validated code records addressed by exact `ShaderCodeHash`.
3. development-only provenance joining code/input hashes to virtual dependencies, compiler invocation, symbols, reflection, and diagnostics.

There is no authored or generated `ShaderProgramDesc`, `ShaderProgramId`, `TShaderProgram`, or mandatory program manifest in the base architecture. Graphics pipeline stage composition is the typed draw request. Full RT pipeline/hit-group composition is owned by the RT plan. Phase 4 atomically deletes `.sparkshader` package readers/writers/cache/path derivation and publishes only the map/library representation; no compatibility container remains.

### 8. Typed Runtime Lookup and One Generation Lifetime

`ShaderRef<Shader>` resolves through the active `GlobalShaderMap`; it never derives a path or package from a source basename. `RenderPassRuntimeCache` remains the sole active/replacement/retired generation owner and stores the map/library plus materialized binding layouts/pipelines for that generation. Lazy materialization may occur during graph construction, never pass recording. Reload validates a complete replacement before activation, preserves the current generation on failure, and retires old state only after all captured submission tokens complete.

### 9. Dependency-Directed Apply Changed

Persist forward and reverse virtual-source dependencies. Editor submits `Apply Changed`; Application snapshots changed virtual paths and routes one typed request; ShaderCompiler selects affected shader types, cooks, and publishes; Renderer validates and activates one replacement. Missing/corrupt dependency metadata fails with explicit `Rebuild All` guidance rather than silently broadening the normal operation. The Editor never scans artifact directories or mutates runtime state.

### 10. Bounded Diagnostics and Traceability

Failures identify shader type, virtual source, entry, stage, target, backend/tool, compile input hash, and declaration location. One bounded replay bundle contains the canonical request, dependency closure, preprocessed source where available, arguments, compiler diagnostics, and reflection mismatch. Successful debug artifacts remain opt-in.

One read-only trace joins shader type or graph/capture label to catalog declaration, virtual dependencies, compile job/input hash, compiler result, shader-map entry, code hash/record, generation, binding layout, pipeline key, graph consumers, and external symbol/capture paths. It reads authoritative metadata and creates no registry or permanent log stream.

### 11. Honest Backend and Ray-Tracing Boundaries

Report language/backend/target/stage/feature support as supported, unsupported, compiler-only, or runtime-validated. No backend silently ignores compile policy. Inline ray-query compute shaders use the normal compute class/map/dispatch route. Ray-generation/miss/hit/intersection/callable declarations may enter the catalog/map as compiler-only records, but native RT pipeline grouping, identifiers, SBT, trace dispatch, fallbacks, and paired execution remain blocked on the separate RT plan.

### 12. Explicitly Deferred Follow-Ups

After this base migration is accepted, a separate measured proposal may add typed shader permutations. It must preserve a zero-boilerplate one-variant default, extend the shader-map key to `(ShaderTypeId, PermutationId, Target)`, keep permutation selection typed, and update compile-input identity, map publication, and diagnostics together. It does not reopen packages or program aliases.

PSO precaching/prewarming, preload/residency/streaming controls, Vulkan pipeline cache, D3D12 cached/library paths, distributed compilation, worker farms, material shaders, and vertex factories remain outside this plan. Persistent compiler-result storage is rejected rather than deferred. A future proposal for another listed capability requires current workload evidence and its own owner/AC; none is scaffolded here.

## Ownership and Dependency Boundaries

| Owner | Owns | Must not own |
| --- | --- | --- |
| Core | generic paths, hashing, files, processes, tasks, diagnostics | shader source policy, renderer shader types, RHI compilation |
| RHI public contract | shader stages/formats, backend-neutral cooked-record validation primitives, parameter metadata primitives, shader/pipeline descriptors, RHI handles | renderer pass catalog, project source discovery, compile orchestration, runtime map policy |
| RHI backend | create/destroy backend shader and pipeline objects from validated data | source preprocessing, renderer shader/pass policy, package naming |
| Renderer | concrete global shader classes and nested parameters, graph dispatch/draw use, shader-map references, runtime generation, lazy layout/pipeline materialization | filesystem source resolution, compiler backend selection, editor process UI, precache/preload policy in this migration |
| ShaderCompiler tool | virtual source mounts, preprocessing/dependencies, compile requests/jobs, backend invocation, reflection validation, shader map/library cook and atomic publication | persisted compiler-result storage, renderer scheduling, live RHI ownership, editor workflow state |
| Application | changed-path watching, typed operation dispatch, publication notification, and activation request routing | dependency resolution, compilation algorithms, catalog construction, shader-map mutation, editor presentation, direct RHI shader construction |
| Editor | immutable shader catalog/operation presentation, semantic selection, source navigation, cancellation intent, and contextual diagnostics | artifact-directory scanning, dependency calculation, compiler invocation, publication, shader-map mutation, cache policy, direct RHI shader construction |

The generic shader authoring primitives may remain in RHI public code if they stay renderer-agnostic. Concrete shader classes and graph use stay Renderer-owned. The runtime shader map may be implemented in Renderer over generic cooked/RHI primitives; it must not cause RHI to depend on Renderer. The standalone compiler may link the registration object catalog, but no runtime module may depend on the tool.

## Preserve, Improve, Delete, and Defer

| Disposition | Item |
| --- | --- |
| Preserve | explicit source/entry/stage registration; DXIL and SPIR-V targets; DXC/Slang backend boundary; recursive preprocessing; include-closure/options hashing; reflection extraction; parameter verification for every compilation; bounded task execution; out-of-process cook; transactional cooked-output publication; validated generation swap; GPU-token retirement; strict runtime rejection of RT libraries until execution is complete |
| Improve | physical paths into virtual paths; mutable registry into validated frozen metadata; cook nodes into input-hash-deduplicated jobs; mixed Unreal prefixes into Sparkle vocabulary; package registry into the runtime `GlobalShaderMap`; change polling into reverse-dependency selection; layout count check into a strong structural signature; diagnostics into replayable per-job bundles; package-led load into typed map lookup; silent backend-policy differences into a generated conformance matrix |
| Delete now and never replace | `ShaderCacheKey`; `IShaderArtifactStore`; `LocalDiskShaderArtifactStore`; cache key/status fields; compile-result serialization; `--no-cache`; `--cache-dir`; Launcher shader-cache settings; `Build/Cache/Shaders`; the standalone shader-cache cleanup scope |
| Delete after migration | `RendererShaderPackages.h`; `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`; basename-derived package fallback; authored program/pass aliases; repeated pass/package/layout/pipeline strings; one-method shader pass wrappers; duplicated shader-visible fields across pass and shader declarations; silent duplicate suppression; count-only layout acceptance; direct runtime path construction from author package IDs |
| Defer until measured | typed permutations; shader pipeline authoring types; PSO precaching/prewarming; preload/residency/streaming controls; native driver caches; material shaders; vertex factories; distributed compilation; persistent worker-process pool; complex plugin loading phases; library chunk/patch system; Vulkan shader objects/pipeline binaries; D3D12 partial programs; work graphs; mesh/task shaders |
| Require one complete future slice | RT state objects/pipelines, native identifiers/group handles, SBT build/indexing, trace-rays commands, graph integration, pipeline/generation lifetime, selected fallback, and paired execution evidence |

## Error Policy

No architecture can promise "error free." This design should make errors difficult to express and deterministic to detect.

The cooker or startup validation must fail on:

- invalid, unknown, overlapping, or late source mounts
- absolute authored source/include paths or virtual paths escaping their mount
- duplicate shader type IDs
- late shader-type registration after registry freeze
- missing source, include, entry point, parameter metadata, or backend output
- source-path canonicalization collisions
- inconsistent reflected layouts across stages/backends or a parameter layout/signature mismatch
- compiler output that fails a shader type's `ValidateCompiledResult`
- corrupt or incompatible cooked map/library records
- duplicate cooked records with the same identity but different content
- a shader-map entry whose code hash, source hash, layout signature, feature set, or declared stage does not match
- a shader-map reference whose code hash is absent from the opened library
- a graph dispatch/draw whose shader parameter contract is structurally incompatible
- a requested shader or pipeline that cannot be created for the active RHI target
- a compiler backend that silently ignores a requested release/debug/optimization/warning/symbol policy
- a registered shader or frame branch whose stage/feature combination is not executable on the selected runtime backend
- an active shader-map record whose descriptor-indexing or acceleration-structure device-address feature is not explicitly supported by the active capability contract
- an RT pipeline descriptor whose concrete shader references, export/hit-group associations, payload/attribute contracts, local/global layouts, recursion/stack policy, or native metadata are inconsistent
- an SBT whose native identifier belongs to another pipeline generation or whose region, stride, alignment, record, instance/geometry/ray-type index, or referenced data lifetime is invalid

Errors must name the shader type, virtual source, entry point, stage, target, backend/version, logical job identity and compile input hash where relevant, plus both conflicting declaration locations. Development recook errors retain the previous active generation; initial startup has no safe previous generation and fails explicitly.

## Rejected Alternatives

### Use the shader filename as `PassName`

Rejected because it couples source organization to graph semantics and fails multi-entry, multi-file, reuse, instance-label, and rename cases.

### Keep all current strings because they are explicit

Rejected because the same fact is authored in several places and can drift. Explicit source/entry/stage is valuable; repeated package, pass, layout, pipeline, or program aliases for that same shader selection are not.

### Remove cooked packages and compile everything at runtime

Rejected because it weakens deterministic releases, startup behavior, backend validation, reflection/layout validation, and failure containment.

### Treat every shader source file as an executable shader

Rejected because includes and libraries are not entry shaders, and one file can contain several entry points.

### Build one global physical package immediately

Rejected as a default because no current measurement shows that 28 files are a material problem. Logical authoring cleanup does not require a physical mega-package.

### Use compiler-generated C++ type names as serialized IDs

Rejected because they are toolchain-specific and unstable. Macro stringization or generated catalog IDs are deterministic; RTTI spellings are not.

### Copy Unreal's complete shader subsystem

Rejected because Unreal's materials, vertex factories, cook workers, distributed compilers, plugin loading phases, platform count, and content scale solve workloads Sparkle does not currently have. Sparkle should copy responsibility boundaries, invariants, typed authoring, lifetime rules, and failure behavior while keeping the implementation proportional.

### Launch one compiler process per shader immediately

Rejected as a default because Sparkle already runs the cooker out of process and executes bounded stage tasks. A serializable job boundary is required now; a persistent worker pool is added only when compiler critical sections, crashes, leaks, or throughput measurements justify its process cost.

## Implementation Contract

The base migration is an ordered clean break, not a menu. Each phase is one manually reviewed changelist-sized checkpoint on `master`; none may leave two authoring, parameter, lookup, cook, or runtime authorities active together. A difficult consumer blocks its owning phase rather than justifying an alias, adapter, wrapper, or cleanup ticket.

### Common rules for every phase

- Work directly in the unstaged `master` worktree. Do not create or switch branches and do not stage, commit, push, or submit. The user owns every source-control action.
- Apply the [Integration Style Guide](../../Engineering/Standards/IntegrationStyleGuide.md), [Change Process](../../Engineering/Standards/ChangeProcess.md), and applicable subject standards. This document controls shader-specific vocabulary and ordering.
- Phases 0 through 5 do not configure, build, compile shaders, cook, launch, capture, or run tests. They use exact searches, bounded owner/consumer reads, source/build/document reconciliation, local-link validation, no-write formatting when available, and `git diff --check`. Phase 6 validates the complete candidate once.
- Preserve one `SparkleTasks` runtime, one out-of-process cooker, one transactional publication route, one active renderer shader generation, and all-queue `RhiSubmissionToken` retirement.
- Preserve `PassCommandContext` as command/declared-resource/diagnostic infrastructure only. Pass recording performs no file I/O, compilation, shader-map/library lookup, layout creation, pipeline creation, or hidden resource discovery.
- Do not add permutations, `ShouldPrecachePermutation`, pipeline precaching/prewarming, preload/readiness/streaming controls, native driver caches, an authored shader-program layer, or full RT execution in this migration.
- Do not add one-field carriers, broad context/service/resource bags, a second catalog/map/runtime-generation owner, permanent migration diagnostics, per-job logging, a compiler-result browser, report generators, feature flags, compatibility formats, or submitted test scaffolding.
- Update definitions, consumers, filenames, includes, CMake/source groups, CLI/help/autocomplete, editor models, diagnostics, current documentation, and disposable generated/cooked outputs in the phase that owns their replacement.
- Preserve unrelated dirty work. Phase 0 records the path-level exclusion list and every later phase rechecks it.

### Frozen base vocabulary and navigation

| Responsibility | Target vocabulary | Canonical owner |
| --- | --- | --- |
| virtual source identity | `ShaderSourceMountTable` and canonical `/Engine`, `/Project`, `/Plugin/<Name>` paths | ShaderCompiler source/dependency capability |
| shader authoring type | `GlobalShader<Shader>` with nested `Parameters` | generic primitive in RHI public; concrete class in semantic Renderer pass/feature ownership |
| implementation registration | `IMPLEMENT_GLOBAL_SHADER(Class, VirtualSource, Entry, Stage)` | concrete shader implementation |
| immutable metadata | `ShaderTypeDesc`, `ShaderTypeId`, `GlobalShaderCatalog` | catalog built from concrete Renderer declarations and frozen before query |
| compile work | `ShaderCompileRequest`, `ShaderCompileJob`, `ShaderCompileInputHash`, `ShaderCompileResult` | ShaderCompiler compilation capability |
| cooked logical lookup | `GlobalShaderMap` | generated by ShaderCompiler; opened read-only by Renderer runtime generation |
| cooked code | `ShaderCodeRecord`, `ShaderCodeHash`, `CookedShaderLibrary` | generated cook output; neutral validation records in RHI public |
| typed runtime lookup | `ShaderRef<Shader>` | Renderer resolves through the active `GlobalShaderMap` |
| graph use | `AllocParameters<Shader>`, `Dispatch<Shader>`, typed graphics draw helpers, `RenderPassLabel` override | `FrameGraphBuilder` focused helpers over existing graph/runtime owners |
| materialized layout/pipeline and generation | existing `RenderPassRuntimeCache` | Renderer `Private/Pipeline` |
| frontend intent | `Apply Changed` and expert `Rebuild All` | Application routing and Editor Shader Tools presentation |

Do not introduce `ShaderProgramDesc`, `ShaderProgramId`, `TShaderProgram`, `SPARKLE_RENDER_PASS`, `ShaderSystem`, `ShaderManager`, `ShaderServices`, `ShaderContext`, `ShaderData2`, `NewShader*`, or Unreal `F*`/`T*` prefixes in new target names. Do not keep `PackageId` as a shader identity synonym.

### Phase 0 - Freeze the lean shader/map contract and inventory

#### Implementation prompt

> Implement Phase 0 as one documentation and inventory CL directly in the unstaged `master` worktree. Re-run exact shader class/parameter, pass-wrapper, package, source/include, registration, compile/cache/cook, publication, runtime-generation, graph-dispatch, editor, build-membership, generated-artifact, diagnostic, and documentation searches. Freeze the owner map and assign every old field/type/file/consumer to one later phase. Reconcile stale documentation and baseline provenance. Do not edit runtime/tool source or run executable checks.

#### Phase-specific references

- [Documentation authority](../../README.md)
- [Integration Style Guide clean-break policy](../../Engineering/Standards/IntegrationStyleGuide.md#current-clean-break-policy)
- [Coding Style one-field types](../../Engineering/Standards/CodingStyle.md#one-field-types)
- [Renderer/RHI boundary](../RendererRhiBoundary.md)
- [Epic RDG shader/pass parameters](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)

#### Required work

- Inventory every shader class, nested `FParameters`, duplicate `*PassParameters` field, pass class, `RenderPassDefinition`, `GetDefinition`, `GetParameterMetadata`, Execute body, graph dispatch/draw consumer, and focused collaborator dependency.
- Classify each pass as direct one-shader compute, multi-stage graphics, shaderless graph work, or real feature collaborator. Assign forwarding wrappers to Phase 2 deletion and justify every retained class with behavior it owns.
- Inventory package identity/generation/cache/readers/writers and assign their complete deletion to Phase 4.
- Record the exact current counts for registrations, handwritten labels/package constants, duplicate parameter fields, wrapper files, HLSL files under `Passes/Deferred`, and generated `.sparkshader` artifacts.
- Record exact baseline provenance or mark final runtime/performance claims blocked. Record unrelated dirty exclusions.

#### Positive guardrails

- Use `rg`/`rg --files` and bounded owner/consumer reads.
- Keep inventory in this document or CL description, not a runtime reporting system.
- Every rejected definition/path has exactly one deletion phase.

#### Negative guardrails

- No runtime edits, target scaffolding, renames, adapters, branches, builds, cooks, or tests.
- No permutation or precache design hidden in the inventory.

#### Acceptance criteria

- Every current shader/pass/package field and material consumer has one target owner or deletion.
- Every forwarding pass and duplicate parameter schema has one Phase 2 disposition.
- Every package reader/writer/cache/identity/generation spelling has one Phase 4 disposition.
- Local links, scoped documentation diff, and `git diff --check` pass; no executable claim is made.

#### CL boundary

Suggested title: `Shaders: freeze lean shader-map migration contract`.

### Phase 1 - Establish virtual sources and semantic shader navigation

#### Implementation prompt

> Implement Phase 1 as one source-identity and physical-layout CL directly in the unstaged `master` worktree. Introduce one canonical virtual source namespace, convert every registration/include/dependency/cache diagnostic input to virtual identity, move the broad `Passes/Deferred` shader bucket into semantic owners matching Renderer navigation, and delete old physical search/fallback paths and the old directory. Do not add compatibility mounts or run executable checks.

#### Phase-specific references

- [Repository Structure and Ownership](../../Engineering/Standards/RepositoryStructureAndOwnership.md)
- [Data-Oriented Design identity rules](../../Engineering/Standards/DataOrientedDesign.md#identity-and-references)
- [Epic shader source-path precedent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/GetShaderSourceFilePath)

#### Required work

- Add immutable `ShaderSourceMountTable` with `/Engine`, `/Project`, and `/Plugin/<Name>` ownership, canonicalization, collision, traversal, case, and late-registration rules.
- Convert registered source paths and root includes to virtual paths; persist virtual dependency identities and use physical paths only for bounded reads.
- Remove project-first shadowing, absolute authored includes, basename identity/fallback, and checkout paths from portable hashes/diagnostics.
- Move shader sources from `Engine/Assets/Shaders/Passes/Deferred` to `Passes/GBuffer`, `Passes/Lighting/...`, `Passes/PostProcessing`, `Passes/Presentation`, `Passes/RayTracing`, and `Passes/Debug` owners as applicable.
- Update every C++ registration, HLSL include, ShaderCompiler resolver/hash/dependency consumer, CMake/source group, documentation link, and generated metadata spelling.

#### Positive guardrails

- One virtual path names one source regardless of machine.
- Relative includes remain relative to the including virtual source.
- Technique names remain only where a shader specifically implements that technique.

#### Negative guardrails

- No old/new search order, alias mount, absolute fallback, duplicate source tree, raw directory registry, or renderer-wide `Deferred` owner.

#### Acceptance criteria

- Exact searches find zero authored old physical registration paths, zero `Passes/Deferred/` paths/files, zero basename fallback, and zero portable hashes containing checkout roots.
- Same-basename files in distinct virtual directories remain distinct; project/engine ownership cannot silently shadow.
- Includes/CMake/source groups/docs reconcile and `git diff --check` passes without compilation claims.

#### CL boundary

Suggested title: `Shaders: establish virtual sources and semantic navigation`.

### Phase 2 - Make the shader class the complete lean frontend

#### Implementation prompt

> Implement Phase 2 as one shader-authoring, parameter-authority, graph-dispatch, and pass-wrapper deletion CL directly in the unstaged `master` worktree. Rename the generic shader frontend to Sparkle vocabulary, make nested `Parameters` the single shader/graph ABI, add direct typed compute and graphics graph entry points, update every shader/pass consumer, and delete duplicate pass schemas, package/debug strings at graph call sites, `RenderPassDefinition` bags, and one-method forwarding pass classes. Retain only collaborators with real feature/draw behavior. Do not introduce programs, permutations, precaching, compatibility overloads, or executable checks.

#### Phase-specific references

- [Coding Style](../../Engineering/Standards/CodingStyle.md)
- [Naming and Vocabulary](../../Engineering/Standards/NamingAndVocabulary.md)
- [Data-Oriented Design single truth](../../Engineering/Standards/DataOrientedDesign.md#single-truth-and-copy-budget)
- [Epic RDG shader parameters and utility passes](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Epic `FComputeShaderUtils::AddPass`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FComputeShaderUtils__AddPass)

#### Required work

- Clean-break rename `TGlobalShader` to `GlobalShader`, `TShaderRef` to `ShaderRef`, and nested `FParameters` to `Parameters` across macros, traits, registrations, tools, and consumers. Delete or rename unused `FGlobalShader`/`FComputeShader`/`FRay*Shader` prefix types; no aliases remain.
- Keep shader-visible inputs/outputs inside each concrete shader class. Move class declarations beside their semantic feature owner where graph setup must name the type.
- Make `AllocParameters<Shader>()` allocate `Shader::Parameters`. Make `Dispatch<Shader>()` and async dispatch accept that same instance and group count, resolve the current shader runtime internally, and derive the default diagnostic label from the shader type.
- Provide an explicit label overload for repeated graph instances. It changes diagnostics only.
- Make graphics graph setup name concrete vertex/pixel shader types plus complete pipeline state and real draw collaborator/data. Do not require a program alias.
- Delete duplicate `*PassParameters` fields/schemas and count-only layout acceptance. Compose a small envelope only for real multi-stage or graph-only fields.
- Delete `RenderPassDefinition`, `RenderPassDefinitionRuntime`, `ComputePassOperations`/equivalent forwarding paths, `GetDefinition`, `GetParameterMetadata`, repeated `PassName`/layout/pipeline strings, and pass classes whose body only constructs/binds/dispatches one shader.
- Retain GBuffer/mesh or feature collaborators only where they own real draw-list/cache/feature behavior; strip shader lookup/binding boilerplate from them.
- Keep the current package-backed runtime representation internally until its atomic Phase 4 replacement, but remove package details from graph/feature call sites and do not create a second representation.

#### Positive guardrails

- The common author writes one shader class, nested parameters, one implementation declaration, and one graph dispatch.
- Graph resource usages and shader binding derive from the same metadata instance/signature.
- `FrameGraphBuilder` automates map/runtime/layout/pipeline mechanics; `PassCommandContext` remains semantic-free.

#### Negative guardrails

- No `DirectLightingProgram`, `SPARKLE_RENDER_PASS`, pass traits duplicating shader metadata, generic program abstraction, separate shader/pass schemas, owner pointer, service bag, runtime reflection-name discovery, or copied bindings.
- No deletion of a class that owns real mesh iteration, draw cache access, feature policy, or several meaningful operations; narrow it instead.
- No permutation/precache callbacks or readiness frontend.

#### Acceptance criteria

- A representative direct compute shader reads as class+nested `Parameters`+implementation declaration+`Dispatch<Shader>` with no authored package/program/pass/layout/pipeline string.
- Exact searches return zero `TGlobalShader`, `TShaderRef`, nested `FParameters`, `RenderPassDefinition`, count-only parameter acceptance, and phase-owned forwarding pass definitions/uses.
- Every shader-visible field exists once; graph setup and reflection/binding consume that authority.
- Graphics names concrete stage shader types and complete pipeline state without a `TShaderProgram`.
- `PassCommandContext` and recording remain infrastructure-only; includes/CMake/docs reconcile and `git diff --check` passes.

#### CL boundary

Suggested title: `Renderer: make shader classes drive typed graph dispatch`.

### Phase 3 - Establish reproducible compile jobs and changed dependencies

#### Implementation prompt

> Implement Phase 3 as one compiler-job, dependency-selection, and replay-diagnostics CL directly in the unstaged `master` worktree. Replace package-shaped cook nodes with immutable one-variant shader compile requests/jobs/results keyed by compiler-affecting input, persist virtual dependencies, make Changed select the exact reverse closure, and delete old node identities and full-catalog Changed behavior. Preserve one cooker, `SparkleTasks`, and the compile-every-selected-input rule. Do not add permutations, persistent compiler-result storage, workers, or executable checks.

#### Phase-specific references

- [Concurrency](../../Engineering/Standards/Concurrency.md)
- [Editor and Tools](../../Engineering/Standards/EditorAndTools.md)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Epic shader compile job](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCompileJob)

#### Required work

- Introduce `ShaderCompileRequest`, `ShaderCompileJob`, `ShaderCompileInputHash`, and `ShaderCompileResult` for `(ShaderTypeId, Target)`.
- Hash virtual source identity/content closure, entry, stage, compiler-affecting environment/ABI, target, backend/tool provenance, and policy. Exclude package, program, pass label, filename basename, and presentation text.
- Replace `CookNode`, node builders/executors, and package-led compile options in one path while retaining the already established `ShaderCompileInputHash` identity.
- Compile every selected input. Deduplicate only identical in-flight jobs inside the active cook and fan out their one result; persist no compiler output between operations.
- Persist forward/reverse dependencies. Change source tracking from a boolean to changed virtual paths and select only affected shader types.
- Use one `TaskExecutor` graph with deterministic ordering, duplicate fan-out, bounded compiler sessions/memory, cancellation settlement, and no worker waits.
- Emit one bounded failure replay bundle; successful analysis artifacts remain opt-in.

#### Positive guardrails

- Identical compiler-affecting requests compile once regardless of graph consumers.
- Cancellation/failure publishes nothing partial.
- Application supplies intent/changed paths; compiler owners select and schedule work.

#### Negative guardrails

- No shader thread pool, `std::async`, persistent worker, compiler-result store, cache configuration/service/browser, retry loop, per-job log stream, permutation dimension, or valid-dependency full-catalog fallback.

#### Acceptance criteria

- Exact searches return zero old cook-node/cache-key definitions and consumers.
- Checkout moves preserve the input hash; any compiler-affecting change invalidates it.
- Duplicate fan-out compiles once per operation, a repeated cook recompiles its selected jobs, cancellation settles all jobs, and changed dependency selection reaches all and only affected shader types.
- Includes/CMake/help/docs reconcile and `git diff --check` passes.

#### CL boundary

Suggested title: `Shaders: establish compile jobs and dependency-directed cooking`.

### Phase 4 - Replace cooked packages with the global shader map

#### Implementation prompt

> Implement Phase 4 as one cooked-map, code-library, typed-lookup, and generation-lifetime CL directly in the unstaged `master` worktree. Replace per-package cooked authority with `GlobalShaderMap` and content-addressed `CookedShaderLibrary` records, make `ShaderRef<Shader>` resolve through the active map, preserve `RenderPassRuntimeCache` as the sole generation/materialization owner, and delete every old package identity/file/reader/writer/cache/path/schema dispatch and package-generation spelling. Do not keep an adapter or run executable checks.

#### Phase-specific references

- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md)
- [Renderer/RHI boundary](../RendererRhiBoundary.md)
- [Epic `FGlobalShaderMap`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FGlobalShaderMap)
- [Epic `FShaderMapResource`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderMapResource)
- [Epic `FShaderCodeLibrary`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FShaderCodeLibrary)

#### Required work

- Generate deterministic `GlobalShaderMap` entries for `(ShaderTypeId, Target)` containing `ShaderCodeHash`, parameter signature, stage/feature/runtime metadata, and provenance joins.
- Generate `CookedShaderLibrary` records keyed by exact `ShaderCodeHash`; deduplicate validated identical code bytes without conflating incompatible metadata.
- Open/validate map and library once per replacement generation. `ShaderRef<Shader>` resolves the active target entry; graph/runtime never computes a file path or package ID.
- Refactor `RenderPassRuntimeCache::ShaderRuntimeGeneration` to own the opened map/library and generation-bound materialized layouts/pipelines. Keep lazy graph-construction materialization and all-queue retirement.
- Delete `RendererShaderPackages.h`, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, `BuildShaderPackageIdFromSourcePath`, `CookedShaderPackage*`, `LoadedShaderPackage`, package cache/identity/path/schema-version dispatch, `.sparkshader` readers/writers, package generations, and obsolete generated outputs.
- Keep neutral code/map validation records in RHI public, generation policy in Renderer, generation output in ShaderCompiler, and backend object creation in backend-private RHI.
- Preserve complete replacement validation, rollback on failure, renderer shader-generation capture in `RenderFrameIdentity`, view-history invalidation, and submission-token retirement.

#### Positive guardrails

- Catalog says what exists; map says what the active target resolves; library owns validated bytes; runtime cache owns derived live objects. No owner duplicates another.
- Physical file grouping is generated policy and remains simple for the current catalog.
- Lazy materialization stays before recording and is not called precaching.

#### Negative guardrails

- No package-to-map converter at runtime, dual emission, old reader, upgrade path, alias ID, directory scan, live map patch, second generation counter, program manifest, streaming/preload framework, or driver cache.

#### Acceptance criteria

- Exact runtime/tool/build searches return zero phase-owned package types/names/paths/readers/writers, `.sparkshader` I/O, and old schema dispatch.
- Every catalog shader resolves through one map entry and every referenced code hash exists exactly once in the library index.
- Invalid replacement preserves the current generation; retired map/library/layout/pipeline state remains until all recorded submissions complete.
- RHI remains Renderer-independent; map/library/runtime owners have no duplicated identity or lifetime state.
- Includes/CMake/generated policy/docs reconcile and `git diff --check` passes.

#### CL boundary

Suggested title: `Shaders: replace cooked packages with the global shader map`.

### Phase 5 - Deliver Apply Changed and one provenance trace

#### Implementation prompt

> Implement Phase 5 as one Application/Editor shader-workflow CL directly in the unstaged `master` worktree. Replace package-oriented recook/reload controls, artifact-directory scans, and the implementation-record table with one semantic `Apply Changed` workflow, immutable operation/catalog read models, automatic activation after renderer validation, contextual expert inspection, and one provenance trace over authoritative catalog/job/map/library/runtime metadata. Delete package targets, manual normal-path reload, duplicate status formatting, and old model fields. Do not add another panel, cache browser, log stream, permutation UI, or executable checks.

#### Phase-specific references

- [Editor and Tools intent-first workflows](../../Engineering/Standards/EditorAndTools.md#intent-first-frontend-workflows)
- [Validation logging and instrumentation](../../Engineering/Standards/ValidationPerformanceAndEvidence.md#logging)
- [Epic Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [PIX shader PDB resolution](https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/)

#### Required work

- Editor submits `Apply Changed`; Application snapshots changed virtual paths and routes one request; ShaderCompiler selects/cooks/publishes; Renderer validates/activates; the operation settles once.
- Keep expert `Rebuild All` and typed shader targeting in Advanced/CLI only. Remove package targeting and manual normal-path reload.
- Replace package/editor rows and artifact scans with shader type, stage, virtual source, active status, and graph consumers from immutable owner read models.
- Present one concise result. Failure leads with source root cause, next action, and confirmation that the previous generation remains active.
- Add one trace from shader type, graph/capture label, code hash, or pipeline key through declaration, dependencies, compile job/input hash/result, map entry, code record, runtime generation/materialization, consumers, and symbols/capture.
- Delete duplicated coordinator/console/panel lifecycle logs/status formatting; keep one bounded operation result through `EditorOperationService`.

#### Positive guardrails

- Application routes without reproducing compiler/runtime policy; ShaderCompiler owns dependency/cook/publication; Renderer owns validation/activation/generation/retirement; Editor owns presentation.
- Primary UI remains shader/source/task oriented; raw hashes/reflection/disassembly/requests remain contextual.

#### Negative guardrails

- No UI compiler sessions, cache directories, publication files, mutable renderer caches, RHI objects, task executor, artifact scans, per-job dialogs/toasts, readiness/precache controls, or second operation runtime.

#### Acceptance criteria

- Normal workflow exposes one dominant `Apply Changed` action and no package/layout/hash/backend/cache mechanics.
- Exact searches return zero package-target request/UI/help/autocomplete/model fields and editor artifact-directory scans.
- Success activates one validated generation; failure/cancellation settles once and preserves the previous generation.
- Trace reads authoritative state and creates no duplicate registry/cache/log.
- Includes/CMake/help/docs reconcile and `git diff --check` passes.

#### CL boundary

Suggested title: `Shader Tools: deliver Apply Changed and shader provenance`.

### Phase 6 - Regenerate, validate, and hand off the complete candidate

#### Implementation prompt

> Implement Phase 6 directly in the unstaged `master` worktree against the complete Phase 0-5 candidate. Run the final legacy-eradication gate first, delete obsolete disposable shader and cooked output, regenerate the one current catalog/map/library once, then perform claim-driven formatting, architecture, compiler, cook, runtime, reload, lifetime, capture, and performance validation. Fix failures at their owning responsibility without compatibility. Remove temporary harnesses, fault injectors, verbose logging, reports, and excessive diagnostics before handoff. Do not stage, commit, push, or submit.

#### Phase-specific references

- [Change Process review and acceptance](../../Engineering/Standards/ChangeProcess.md#review-and-acceptance)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md)
- [Renderer/RHI boundary enforcement](../RendererRhiBoundary.md#enforcement)
- [Bistro and San Miguel workloads](../../Engineering/BistroAndSanMiguelWorkloads.md)

#### Required work

- Prove zero definitions/uses of every phase-owned package/program/pass-wrapper/duplicate-parameter/old-prefix/old-path/compatibility symbol and no permutation/precache/preload scaffold.
- Regenerate the complete catalog, dependency records, global shader map, code library, provenance, and publication metadata once from final source.
- Run pinned no-write formatting where available, `git diff --check`, local-link validation, file/CMake/include inventory, and `architecture_boundary_check`.
- Build the smallest owning ShaderCompiler/Renderer targets, then the exact D3D12/Vulkan DevelopmentEditor product target required by the contract.
- Cook and inspect every supported current shader for DXIL and SPIR-V; compare reflection/layout/type/code identities and capability policy without silently skipped cells.
- Validate checkout-independent hashes, in-operation duplicate fan-out, repeated-operation recompilation, changed dependency closure, cancellation, failed-job replay, transactional publication, stale rejection, invalid replacement rollback, delayed GPU completion, and generation retirement.
- Run D3D12/Vulkan Showcase smokes/captures for supported raster, compute/inline-ray-query, exposure, presentation, debug, and advertised fallback branches. Full RT execution remains explicitly unavailable under its separate plan.
- Measure compile queue/wall/CPU time, compiler-session memory, selected/compiled job counts, generated/cooked bytes, map/library open time, lazy materialization time, runtime overlap during reload, and frame impact. Do not add precache/readiness metrics for a system not implemented.
- Perform a final diagnostic/code-structure audit: no migration log stream, per-job spam, default report files, cache browser, submitted test scaffold, god orchestrator/folder/function, forwarding wrapper, or duplicated validation/policy remains.
- Recheck `master`, empty staged diff, unrelated dirty exclusions, generated/cooked source-control policy, and exact diff boundaries. Leave all work unstaged.

#### Positive guardrails

- Use the cheapest claim-falsifying check first and report exact commands/configurations/results/unavailable evidence.
- Temporary local harnesses are removed before handoff; no submitted test-only code without separate authorization.
- Preserve concise owner-local failures and external capture/profiler integration while deleting migration diagnostics.

#### Negative guardrails

- No speculative broad build before focused owners, simulated backend/capture result, performance claim without baseline provenance, retry loop, compatibility reader, old/new cook, fallback catalog, device-idle reload, or miscellaneous final-fix bucket.

#### Acceptance criteria

- Every final acceptance criterion below has exact evidence or is explicitly blocked; no unrun/static-only check is called passed.
- Shader class/catalog/job/map/library/runtime/graph/frontend has one authority at each stage and no legacy/compatibility path.
- Required generated artifacts match final source; no obsolete output, report, debug artifact, capture, log, or temporary proof file is unintentionally included.
- Diagnostics are bounded, orchestration reads as named stages, and no owner/folder/function mixes unrelated responsibilities.
- Branch is `master`, staged diff is empty, scoped checks pass where available, and the user receives the unstaged changelist for manual review.

#### CL boundary

Suggested title: `Shaders: validate lean global-shader map architecture`.

Full ray-tracing execution is not a hidden later phase. The [Ray-Tracing Pipeline and Dual-Execution Delivery Plan](RayTracingPipelineImplementationPlan.md) owns native RT pipeline/SBT/trace/fallback work. Typed permutations and PSO precaching are separate future proposals after this base migration is accepted.
## Final Acceptance Criteria

The base migration is accepted only when:

- a one-to-one compute author writes one `GlobalShader<Shader>` class with nested `Parameters`, one `IMPLEMENT_GLOBAL_SHADER` declaration, parameter assignments, and `Dispatch<Shader>`; there is no package, program alias, pass-registration macro, duplicate pass schema, forwarding pass class, layout string, or pipeline string;
- `AllocParameters<Shader>()` and shader reflection/binding consume the same `Shader::Parameters` metadata and every shader-visible field has one declaration;
- graph input/output access derives from typed parameter fields and pass recording sees only declared resources;
- graphics names concrete stage shader types and complete pipeline state at the real draw owner without a universal shader-program abstraction;
- shaderless and true multi-stage/graph-only operations use narrow envelopes without copying shader-visible fields;
- every registered source/include has a canonical virtual path and portable diagnostic identity; same-basename paths cannot collide or shadow silently;
- catalog freeze rejects duplicate/late declarations with both source locations;
- the base catalog/map contains exactly one variant per `(ShaderTypeId, Target)` and no permutation/precache/preload scaffolding;
- `ShaderCompileInputHash` changes for every compiler-affecting input, survives checkout relocation, and excludes package/pass/presentation text;
- identical compile requests deduplicate only within one active operation, repeated cooks compile again, cancellation settles, and no partial publication appears;
- `GlobalShaderMap` is the sole typed logical lookup and every map entry references a validated `ShaderCodeHash` in `CookedShaderLibrary`;
- runtime lookup never derives source basenames, package IDs, or cooked paths and no `.sparkshader` reader/writer remains;
- `RenderPassRuntimeCache` is the sole active/replacement/retired generation and materialized layout/pipeline owner; lazy creation occurs before recording, not in Execute;
- changed includes select every dependent shader type and no unrelated shader when dependency data is valid;
- Shader Tools presents one `Apply Changed` intent, one operation state, automatic validated activation, source navigation, and contextual expert details without artifact scans or package mechanics;
- compile/validation failure reports one source-located root cause and preserves the previous accepted generation;
- every supported shader cooks and validates for the required DXIL/SPIR-V targets; unsupported/compiler-only stages remain honestly classified;
- D3D12/Vulkan runtime/capture evidence covers the selected current graph branches and a captured shader/code/pipeline identity resolves to the exact class, source closure, compile request, map entry, code record, and symbols;
- delayed GPU completion proves old map/library/layout/pipeline generations retire only after all queue submissions complete;
- full ray-tracing shader execution remains reported unsupported until the separate RT pipeline plan lands its complete paired slice;
- no migration logging, report generator, cache browser, submitted test scaffold, god owner/folder/function, one-method forwarding wrapper, duplicated policy, or excessive diagnostics remains;
- the [required evidence pack](#required-evidence-pack) is complete or each unavailable claim is explicitly blocked with provenance.

## Final Position

Sparkle should follow Unreal's lean global-shader center end to end: one shader class owns its nested parameters and optional compile hooks; one implementation declaration owns virtual source, entry, and stage; one frozen catalog drives reproducible compile-every-time jobs; one generated global shader map resolves typed shader references to code-library records; and the frame graph dispatches those shader types directly with the same parameter schema. Render-graph labels remain diagnostic presentation, while package, map, layout, and pipeline mechanics stay behind their owners.

The clean target is neither "the filename is everything" nor "copy every Unreal subsystem." It is "the author states only the shader class, parameters, source/entry/stage, and the actual draw/dispatch; the engine derives and validates everything else." Permutations, authored program types, precaching, preload/streaming, and full RT execution stay out until a measured workload earns them.
