# Shader System Architecture

Status: target architecture; source-reconciled but not executable proof

Responsibility: shader authoring identity, compilation inputs, cooked map/library shape, runtime materialization, typed graph use, ownership, failure policy, and capability boundaries

Delivery sequence: [Shader System Delivery Plan](../../Plans/CrossModule/ShaderSystem.md)

Migration provenance: [Shader System Migration Baseline](../../Research/ShaderSystemMigrationBaseline.md)

Current source inventory: [Shader Compilation Capability Inventory](../Modules/Tools/ShaderCompiler/README.md)

## Purpose And Authority

This document owns the enduring shader-system design. It does not own phase order, historical pre-migration counts, implementation status, or executable evidence. Code and build configuration prove current implementation; the delivery plan owns remaining work and validation order; the migration baseline preserves dated audit evidence.

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
| A graphics graph caller should author one complete pipeline-state aggregate. | Rejected. Epic keeps a complete RHI initializer internally, but mesh-pass code supplies narrow render-state overrides, mesh/material processing supplies vertex/raster facts, and RDG attachments supply target compatibility. Sparkle's current `GraphicsShaderPipelineState` duplicates graph-owned formats and exposes backend-shaped mechanics at the feature call. |

“Epic-aligned” therefore means matching responsibility boundaries, invariants, failure behavior, and authoring ergonomics—not copying class names, macro volume, material-system scale, or every optional cache.

## Decision Summary

1. Sparkle should adopt Unreal's global-shader flow as its core mental model: one concrete shader class owns its parameter contract and compile hooks; one implementation declaration supplies virtual source, entry point, and stage; cooking produces a global shader map and code records; runtime resolves a typed shader reference from the active map.
2. A one-shader compute pass does not need an authored program alias, pass-registration macro, duplicate pass-parameter struct, or forwarding pass class. The frame graph should accept the shader type, its nested `Parameters`, a diagnostic label only when the generated default is insufficient, and the dispatch dimensions.
3. Graphics work names the concrete vertex/pixel shader types at the draw site and supplies a real draw collaborator. Pass code sets only narrow blend/depth/stencil intent; graph attachments, mesh geometry, material policy, shader references, and the runtime pipeline owner contribute the rest. Ray tracing uses one narrowly scoped `RayTracingPipelineComposition` only because exports and hit groups form a real multi-stage execution unit. Sparkle does not add a universal authored `ShaderProgram` or frontend pipeline-description abstraction.
4. A shader class declares nested `Parameters` only when that shader is a direct graph-dispatch binding owner. Compute and ray-generation shaders normally own them; miss, hit, intersection, and callable shaders do not repeat the ray-generation root schema. The frame graph consumes the selected dispatch shader's schema to declare resource access, and runtime binding/reflection validation derive from it. A multi-shader, shaderless, or local-record owner may add one small envelope only for fields that no shader root schema already owns.
5. A semantic render-pass label remains necessary for frame-graph diagnostics, GPU markers, errors, captures, and profiling, but it does not select code. Generate the one-to-one default from the shader type and accept an instance override for mip, cascade, phase, view, or repeated use.
6. A shader filename is a virtual source input, not the pass, shader-map, pipeline, or diagnostic identity.
7. Sparkle should keep cooked shader data but replace handwritten packages with two generated authorities: `GlobalShaderMap` for typed logical lookup and `CookedShaderLibrary` for validated code records addressed by hash. The runtime pipeline owner holds generation-safe binding layouts and lazily materialized compute/graphics pipelines derived from active-map shader references plus complete internal state keys.
8. Permutation infrastructure is deliberately postponed until after the non-permuted shader-map path is complete and accepted. The base map key is `(ShaderTypeId, Target)`; a later measured follow-up may extend it with a typed `PermutationId` without changing the lean one-variant authoring path.
9. PSO precaching/prewarming, preload controls, and driver-cache integration remain outside this implementation plan. Phase 5 replaces eager speculative variants with exact lazy materialization before command recording; add earlier preparation only after measured first-use hitch evidence justifies it.
10. Shader Tools should center `Apply Changed`, semantic shader/source selection, one operation state, source-located errors, and contextual next actions. Package/layout IDs, hashes, raw artifacts, full rebuild/reload, backend flags, and runtime materialization mechanics remain expert details.
11. Inline ray queries and full ray-tracing pipelines remain different execution systems that share shader identity, parameters, maps, scene/TLAS/material data, semantic HLSL kernels, runtime generation, and retirement. Their native invocation and SBT mechanisms remain distinct, while this one plan delivers both without a parallel shader architecture.
12. Classic TLAS versus partitioned TLAS and descriptor encoding versus device-address storage are RHI binding mechanics, not shader identities or effect choices. One shader class declares one semantic acceleration-structure parameter and one HLSL entry. The selected backend/provider lowers that parameter to its exact native descriptor representation. Sparkle deletes the duplicate device-address shader, raw-address shader uniform fields, access-mode frontend, and no-query shadow shader; it does not replace them with a hidden compiler variant or author-facing permutation.
13. Shader resources use explicit view vocabulary: `CreateSRV` for read-only texture/buffer views and `CreateUAV` for writable texture/buffer views. Acceleration structures use one semantic `CreateAccelerationStructureBinding`; raster/depth outputs use neutral render-target/depth-target attachment bindings. Generic `Read` aliases and neutral `CreateRTV` / `CreateDSV` spellings do not survive.
14. Raster attachments carry load/store/clear and depth/stencil access. Their graph resource descriptions derive target formats/count/sample compatibility automatically. `GpuMesh` supplies vertex input and topology; the mesh-pass owner supplies material-dependent fill/cull and pass render state; the pipeline owner assembles and keys the complete neutral descriptor. No caller repeats these facts.

The short answer is therefore:

> Make the shader class the lean authoring unit, keep its parameters inside the class, remove manual package/program/pass duplication, resolve it through a generated global shader map, and let the frame graph dispatch that shader directly with the same typed parameters.

## Why the Shader Filename Is Not the Pass Name

A filename answers "where is source text?" A pass name answers "what GPU operation is this?" Coupling them creates incorrect behavior in ordinary cases:

- One source file can contain several entry points or stages.
- One graphics pipeline can use shader stages from several files. Sparkle's GBuffer pipeline uses `GBufferVS.hlsl` and `GBufferPS.hlsl`.
- The same compiled shader can be reused by multiple semantic passes.
- A pass type can be scheduled several times with instance-specific labels such as a mip number, eye, cascade, phase, or view.
- Renaming or moving a source file should invalidate compilation, but it should not silently rename profiler history, GPU markers, frame-graph nodes, or the shader type that references it.
- Two directories can contain the same source basename. Sparkle's Phase 0 basename-derived fallback could not distinguish them.
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
| Pipeline | concrete shader references plus fixed-function and target state | complete pipeline description/cache key assembled by the runtime materialization owner | one shader, diagnostic label, or caller-authored partial aggregate alone |

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
| virtual shader source paths | stable source namespace independent of checkout location | `ShaderSourceMountTable` with `/Engine`, `/Project`, and `/Plugin/<Name>` roots | canonical virtual registrations, includes, dependency identity, and diagnostics backed by bounded physical mounts |
| `FShaderType` / `FGlobalShaderType` | immutable shader metadata and compile hooks | `ShaderTypeDesc` emitted by typed registration | `ShaderRegistrationDesc` |
| `TShaderPermutationDomain` | typed, bounded permutation dimensions and stable IDs | deferred follow-up after the one-variant shader-map path is accepted | one registered shader type/target variant; no authored permutation domain |
| `FShaderCompilerInput` | complete read-only input for one compilation | `ShaderCompileRequest` | implemented as the package-free compiler input for one shader type and target |
| `FShaderCompileJob` / `FShaderCompileJobKey` / input hash | scheduled unit and logical shader/target key, plus a separate hash over all compiler-affecting inputs | `ShaderCompileJob` plus `ShaderCompileInputHash`; a small logical job ID may exist only for scheduling | implemented compile job and result records with in-operation identical-input fan-out |
| `FShaderCompilingManager` and Shader Compile Workers | asynchronous coordination and compiler-process isolation | compile-job coordination on the existing cooker `TaskExecutor`; optional worker processes only when justified | one out-of-process cooker and one bounded `TaskExecutor` compile batch; no second pool or persistent compiler worker |
| Global Shader Map / `TShaderMapRef<T>` | typed target-specific lookup and shader lifetime | `GlobalShaderMap` / `ShaderRef<Shader>` | implemented typed lookup into one validated map/library generation |
| `FShaderMapResourceCode` | code hashes and map resource content | generated map resource record | map entries reference content-addressed library records by exact code hash |
| `FShaderCodeLibrary` | cook-time collection of unique code and runtime loading by hash | `CookedShaderLibrary` | implemented as `CookedShaderLibrary.slib` with exact-hash lookup and publication validation |
| `FShaderPipelineType` | optional declared stage grouping | no universal authoring abstraction; graphics names stage types at the draw site and ray tracing alone uses a focused typed pipeline composition | graphics draw names typed VS/PS stages; RT products use focused typed compositions and hit groups |
| RDG shader/pass parameter structs | shader parameters can directly serve a one-to-one pass; pass envelopes and shaderless pass parameters are also valid | one shader-visible `Parameters` schema, reused or composed into a pass envelope without duplicating shader fields | nested typed `Parameters` drive shader binding; graph-only attachments and feature collaborators remain separate owners |
| RDG event name | diagnostic/profiler identity of one graph operation | generated default label with an optional instance override | frame-graph pass names are formatted into diagnostic and event-scope labels; instance labels remain at scheduling sites |
| `FMeshPassProcessorRenderState` | narrow pass-wide blend, depth/stencil, access, stencil-reference, and uniform-buffer overrides | smaller `RasterPassRenderState` containing only semantic blend/depth-stencil and dynamic stencil-reference choices; Sparkle attachment access stays graph-owned | `RasterPassRenderState` owns narrow pass intent; graph attachments and prepared mesh work own their facts |
| `FGraphicsMinimalPipelineStateInitializer` | mesh-draw fixed-function and shader state without render-target state | internal `GraphicsPipelineKey` inputs contributed by shader references, mesh/material facts, and pass render state | `GraphicsPipelineRequest` and `GraphicsPipelineKey` combine exact shader, binding, pass, mesh, and attachment facts |
| `FGraphicsPipelineRenderTargetsInfo` / `ExtractRenderTargetsInfo` | render-target information extracted from RDG attachment bindings | compatibility signature derived from graph resource descriptions; load/store/clear/access remain graph execution and validation facts | `GraphicsAttachmentSignature` is derived by the frame graph from bound attachments |
| `FGraphicsPipelineStateInitializer` / `SetGraphicsPipelineState` | complete RHI-facing state and final materialization/binding | private complete `GraphicsPipelineDesc` assembled and lowered by Renderer/RHI owners | Renderer builds a complete `GraphicsPipelineDesc`; backend validation and lowering consume it without inventing feature policy |
| PSO precache / shader pipeline cache | earlier pipeline preparation and hitch tracking | deferred until current lazy materialization shows a measured product hitch | generation-bound lazy materialization exists; no precache coordinator or persistent native cache |

The mapping is architectural, not a request to copy Unreal class names or source code. Sparkle should keep names that fit its own standards while retaining the responsibility boundaries.

### Full Lifecycle

```text
AUTHORING
  shader class
    + nested Parameters when it owns direct graph bindings
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
  Dispatch<Shader>(parameters, groupCount) or Draw<VS, PS>(parameters, draws)
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
- shader classes provide executable entry points, direct-binding parameter metadata only where used, and only the compile hooks they actually need;
- graph dispatch names the concrete shader type(s) and supplies execution dimensions or draw work;
- parameter metadata provides the shared graph-resource and shader-binding contract;
- shader maps provide logical typed lookup;
- code records provide exact hashed physical delivery; a library may merge duplicate blobs when measured useful;
- pipeline descriptors add fixed-function state;
- render-pass labels provide diagnostics only.

This is why filenames cannot safely replace shader types, graph operations, or pipelines. Automation derives map identity, code-library records, binding metadata, and runtime lookup. Authors state only facts the system cannot infer: class, virtual source, entry, stage, parameter fields, optional capability policy, and the actual dispatch/draw request.

## Proposed Authoring Experience

Unreal's production pattern is intentionally direct: the shader class declares `FParameters`; `IMPLEMENT_GLOBAL_SHADER` binds the class to source, entry, and stage; RDG allocates that same parameter type; `FComputeShaderUtils::AddPass` receives the typed shader reference, parameters, and group count. Sparkle should preserve that experience while using Sparkle naming and its existing graph/runtime owners. It should not copy Unreal-only `F` prefixes, constructor/type-layout macros, or systems that Sparkle's CRTP registration already derives.

The exact lean Sparkle target for a one-to-one compute shader is:

```cpp
class DirectLightingCS final : public GlobalShader<DirectLightingCS>
{
public:
    BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
        SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, DirectDiffuse)
        SHADER_PARAMETER_TEXTURE_SRV(Texture2D, ShadowVisibility)
        SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
    END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(
    DirectLightingCS,
    "/Engine/Passes/Lighting/Direct/DirectLighting.hlsl",
    "main",
    Compute);
```

`DirectDiffuse`, `ShadowVisibility`, and `View` are each the one C++ member, graph/layout key, and reflected HLSL binding name. The type token is type information, not a second binding name; there is no author-facing alias or `_NAMED` escape hatch.

Graph construction uses that class directly:

```cpp
auto& parameters = builder.AllocParameters<DirectLightingCS>();
parameters.DirectDiffuse = builder.CreateUAV(directDiffuse);
parameters.ShadowVisibility = builder.CreateSRV(shadowVisibility);
parameters.View = viewUniforms;

builder.Dispatch<DirectLightingCS>(parameters, groupCount);
```

### Resource-View and Raster-Attachment Vocabulary

Unreal exposes two legitimate read-only RDG forms: a direct `SHADER_PARAMETER_RDG_TEXTURE` resource reference and an explicit `SHADER_PARAMETER_RDG_TEXTURE_SRV` created with `FRDGBuilder::CreateSRV`. It exposes writable shader views through `CreateUAV`. It does not expose `FRDGBuilder::Read`, and it does not create raster attachments through a symmetric `CreateRTV`; graphics parameters use `FRenderTargetBinding` / `RENDER_TARGET_BINDING_SLOTS`. NVRHI makes the same semantic split: binding layouts distinguish `Texture_SRV`, buffer SRVs, texture/buffer UAVs, and `AccelStruct`, while render targets and depth targets belong to framebuffer attachments rather than shader binding sets.

Sparkle chooses the explicit-view form because its shader parameter metadata already describes a concrete shader binding and must drive graph access, descriptor materialization, and reflection from one field. The author-facing vocabulary is frozen as follows:

| Use | Shader parameter declaration | Graph assignment |
| --- | --- | --- |
| read-only texture | `SHADER_PARAMETER_TEXTURE_SRV(Type, Name)` | `builder.CreateSRV(texture, optionalViewDesc)` |
| read-only buffer | `SHADER_PARAMETER_BUFFER_SRV(Type, Name)` | `builder.CreateSRV(buffer, optionalViewDesc)` |
| collaborator-owned read-only buffer | `SHADER_PARAMETER_EXTERNAL_BUFFER_SRV(Type, Name)` | focused draw collaborator supplies the already-materialized SRV as a binding override |
| read/write texture | `SHADER_PARAMETER_TEXTURE_UAV(Type, Name)` | `builder.CreateUAV(texture, optionalViewDesc)` |
| read/write buffer | `SHADER_PARAMETER_BUFFER_UAV(Type, Name)` | `builder.CreateUAV(buffer, optionalViewDesc)` |
| scene acceleration structure | `SHADER_PARAMETER_ACCELERATION_STRUCTURE(Name)` | `builder.CreateAccelerationStructureBinding(sceneTlas)` |
| raster color attachment | `SHADER_PARAMETER_RENDER_TARGET(Name)` in the narrow graphics envelope | `builder.CreateRenderTarget(texture)` |
| raster depth attachment | `SHADER_PARAMETER_DEPTH_TARGET(Name)` in the narrow graphics envelope | `builder.CreateDepthTarget(texture)` |

`CreateSRV` and `CreateUAV` create graph-tracked shader views; their typed return values carry the parent resource, subresource/format selection, and access declared by the parameter metadata. `SHADER_PARAMETER_EXTERNAL_BUFFER_SRV` is the narrow exception for a buffer whose lifetime and descriptor are intentionally owned by a real draw collaborator, such as the GBuffer mesh cache's skinning and morph buffers. It remains part of the shader's one parameter schema, is explicitly excluded from graph-resource declaration, and must be supplied by that collaborator at draw binding. It is not a bypass for graph-owned resources. `CreateRenderTarget` and `CreateDepthTarget` create graph attachment bindings, not HLSL parameters or shader-visible descriptors. Load/store, mip, slice, resolve, and clear policy belong to those attachment bindings or the focused graphics envelope.

There is deliberately no author-facing `Read(texture)` / `Read(buffer)` alias: it hides whether the shader receives an SRV, a copy source, an attachment load, or another access kind. There is deliberately no neutral `CreateRTV` / `CreateDSV`: those acronyms name backend-native D3D views and conflict with Sparkle's [neutral render-target vocabulary](../../Engineering/Foundations/Naming.md#canonical-concurrency-and-rendering-terms). An acceleration structure is also not generalized into `CreateSRV`; NVRHI models it as `AccelStruct` and Vulkan gives it a distinct descriptor kind, so Sparkle retains one semantic acceleration-structure binding and lowers it privately per backend.

Primary references:

- [Epic `FRDGBuilder::CreateSRV`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRDGBuilder/CreateSRV)
- [Epic `FRDGBuilder::CreateUAV`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRDGBuilder/CreateUAV)
- [Epic RDG shader and render-target parameter examples](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Epic `FRenderTargetBinding`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRenderTargetBinding)
- [NVIDIA NVRHI binding sets and framebuffers at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)

`AllocParameters<Shader>()` means `Shader::Parameters`; it does not allocate a second schema. `Dispatch<Shader>()` resolves `ShaderRef<Shader>` from the active `GlobalShaderMap`, derives the default diagnostic label from the shader type, declares resource usages from the same parameter metadata, materializes the generation-bound layout/pipeline through the backend owner, and records the dispatch later. The caller sees none of the package, map, code-library, binding-layout, or pipeline-cache mechanics.

An explicit label is only for a genuinely distinct graph instance:

```cpp
builder.Dispatch<ExposureDownsampleCS>(
    RenderPassLabel::Format("ExposureDownsample mip {}", mip),
    parameters,
    groupCount);
```

That label describes this graph node. It does not select shader bytecode.

Graphics follows the same rule without inventing a program alias or making the pass author restate a complete PSO. The draw path requests typed `ShaderRef<GBufferVS>` and `ShaderRef<GBufferPS>` from the map. The pass sets only the blend/depth/stencil intent it owns; graph attachments derive target formats, count, samples, load/store, and depth/stencil access; the mesh supplies vertex input and topology; material/pass policy supplies fill and cull; the runtime owner assembles and keys the complete neutral descriptor before recording. A focused mesh-pass collaborator may remain when it owns real draw-list/cache behavior; one-method pass wrappers that only forward parameters to a shader do not.

The target authoring shape is granular and semantic:

```cpp
auto& parameters = builder.AllocGraphParameters<GBufferGraphParameters>("GBuffer");
parameters->Shader.Pixel.View = viewUniforms;
parameters->BaseColor = builder.CreateRenderTarget(baseColor, RenderTargetLoadAction::Clear);
parameters->DeviceZ = builder.CreateDepthTarget(
	deviceZ,
	DepthStencilAccess::DepthWrite,
	RenderTargetLoadAction::Clear);

RasterPassRenderState renderState;
renderState.SetDepthStencil(DepthStencilState::WriteNearOrEqual);
renderState.SetBlend(BlendState::Opaque);

builder.Draw<GBufferVS, GBufferPS>(parameters, renderState, meshDraws);
```

This is deliberately not a second aggregate with the old fields under a new name. `RasterPassRenderState` contains only pass-selected semantic overrides and has granular setters. It cannot name vertex layouts, topology, render-target formats/count, depth format, sample count, shader stages, or backend objects. Attachment bindings are authoritative for their own compatibility and load/store behavior. The complete immutable pipeline description still exists where D3D12 and Vulkan require it, but only inside the runtime/RHI materialization boundary.

Ray-tracing stages use the same concrete `GlobalShader` registration and `ShaderRef` lookup as raster and compute, but they do not pretend to have identical binding roles. The selected ray-generation shader owns the dispatch-global nested `Parameters` and the dispatch-wide payload, attribute, and recursion compile contract, following Unreal's shader-type ownership. Miss, closest-hit, any-hit, intersection, and callable classes declare only stage identity, genuinely local-record metadata, and optional compile hooks; they do not repeat the ray-generation contract or introduce empty placeholder structs. A focused `RayTracingPipelineComposition` names typed stage membership and hit groups and derives the shared ABI from its selected ray-generation type. It is not a universal `TShaderProgram`, package, second registration framework, or copied metadata record. The target semantics remain in the [ray-tracing target architecture](../Modules/Engine/Renderer/RayTracingExecution.md), while the phases below deliver the entire path.

The intended authoring surface mirrors Unreal's useful production split without copying its prefixes or legacy binding adapters. This abridged example shows the declaration shape; the product implementation supplies the complete GBuffer output schema required by Phase 7.

```cpp
class RayTracingGBufferRGS final : public GlobalShader<RayTracingGBufferRGS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
		SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
		SHADER_PARAMETER_ACCELERATION_STRUCTURE(RayTracingScene)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, BaseColor)
	END_SHADER_PARAMETER_STRUCT()
};

class RayTracingGBufferMiss final : public GlobalShader<RayTracingGBufferMiss> {};

class RayTracingGBufferClosestHit final : public GlobalShader<RayTracingGBufferClosestHit> {};

IMPLEMENT_GLOBAL_SHADER(
	RayTracingGBufferRGS,
	"/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl",
	"RayTracingGBufferRayGeneration",
	RayGeneration);

IMPLEMENT_GLOBAL_SHADER(
	RayTracingGBufferMiss,
	"/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl",
	"RayTracingGBufferMiss",
	Miss);

IMPLEMENT_GLOBAL_SHADER(
	RayTracingGBufferClosestHit,
	"/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl",
	"RayTracingGBufferClosestHit",
	ClosestHit);
```

The effect owner declares one `RayTracingPipelineComposition` relating those typed stages and its hit group. The selected ray-generation class is the single author-facing owner of `GBufferRayPayload`, `BuiltInTriangleAttributes`, and recursion; the composition derives that contract and never copies its names, stage, or ABI metadata. A hit group owns only genuinely consumed local-record data. Graph construction then remains as lean as compute dispatch:

```cpp
auto& parameters = builder.AllocParameters<RayTracingGBufferRGS>();
parameters.View = viewUniforms;
parameters.RayTracingScene = builder.CreateAccelerationStructureBinding(rayTracingScene);
parameters.BaseColor = builder.CreateUAV(baseColor);

builder.TraceRays<RayTracingGBufferRGS>("RayTracingGBuffer.Pipeline", composition, parameters, renderExtent);
```

`TraceRays<RayGenerationShader>` is the Renderer/frame-graph frontend. It resolves the typed ray-generation shader and focused composition, derives the global layout from `RayGenerationShader::Parameters`, asks the existing runtime owner for the exact pipeline/table generation, declares all graph resources, and records later. The author does not pass a native pipeline, SBT, global binding writer, code hash, export string, table address, stride, or backend command. An explicit `RenderPassLabel` overload changes diagnostics only.

The final form has these invariants:

- the direct dispatch shader is the sole author-facing owner of its global shader-visible parameters; optional local-record fields have one separate group/stage owner and are never mirrored;
- the implementation declaration states only class, virtual source, entry, and stage/export facts;
- optional compile hooks appear only on shader classes that use them; dispatch-wide payload/attribute/recursion facts appear once on the ray-generation shader and bounded local records appear once on their owning hit group or stage;
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

The current global shader map and code library carry information that a loose compiled shader file cannot represent safely:

- declared stage, compile-unit kind, and ray-tracing stage metadata
- target-specific DXIL and SPIR-V code records
- entry points, exports, hit-group roles, and local-record contracts where applicable
- normalized reflection, merged resource bindings, and parameter signatures
- specialization inputs
- required feature flags
- compiler backend and target provenance
- compile-input, exact code, binding-layout, and publication hashes

The base target uses three identities with different scopes. The names deliberately distinguish Unreal's logical job identity from its full compiler-input hash:

1. `ShaderTypeId`: a stable ID emitted from the concrete shader declaration. Together with target it addresses a base shader-map entry.
2. `ShaderCompileInputHash`: a content hash of every input that can change one stage compile, including virtual source path, transitive source contents, entry point, stage, environment, target, compiler backend, and compiler version. Parameter metadata enters this hash only when it changes generated declarations, resource-table/environment input, or compiler output; a validation-only signature belongs in the map record instead. Package/pass/display identity is excluded.
3. `ShaderCodeHash`: a hash of the exact validated backend bytecode. Stage, entry, reflection, feature, layout, and provenance metadata remain adjacent current-contract map/record fields and are covered by artifact integrity; they are not silently conflated with raw code identity. This preserves exact byte deduplication and symbol correlation without treating equal bytes with incompatible metadata as interchangeable records.

This allows source edits to invalidate compilation, source moves to be diagnosed correctly, identical compile jobs and identical output code to be deduplicated at their proper layers, and typed shader lookup to remain independent of a file basename.

Committed source uses one transactionally published `GlobalShaderMap.smap` plus one content-addressed `CookedShaderLibrary.slib`; the Phase 4 clean break deleted the former one-`.sparkshader`-per-package readers, writers, registry, and authored identity. Physical grouping remains generated cooker policy and is invisible to Renderer callers. Any later regrouping requires startup I/O, file-open, compression, or patching evidence and must regenerate the one current representation in place.

```text
ShaderTypeId + Target --------------------------> GlobalShaderMap entry
ShaderCodeHash --------------------------------> CookedShaderLibrary record
                                                            |
typed shader refs + pass state + mesh/material facts + attachment signature
                                                            |
                                                            v
                                                 GraphicsPipelineKey
                                                            |
                                                            v
                                      complete descriptor -> RHI pipeline
```

## External Precedent and What Sparkle Adopts

The sources below are precedents, not local implementation authority. Repository links are pinned to the reviewed revisions where possible.

### Unreal Engine as the Core Model

Epic's global-shader documentation registers a C++ shader type against a source file, entry point, and shader stage. Its example deliberately registers vertex and pixel shader types from the same file with different entry points, then retrieves instances by type from the Global Shader Map. `FShaderType` metadata also carries the source filename, function, frequency, permutation count, parameter metadata, and hooks for compile eligibility, environment modification, output validation, and precaching. A source file is therefore an input to a shader type, never the universal runtime identity.

Epic's `FShaderCompilerInput` gathers the read-only inputs for one compile, including the virtual source path, entry point, target, platform/format, environment, parameter metadata, debug information, and deterministic input identity. `FShaderCompileJob` pairs input, a logical job key, preprocess output, compiler output, and diagnostics; the logical `FShaderCompileJobKey` is not the full content hash. `FShaderCompilingManager` coordinates priorities, pending jobs, result application, cancellation, and worker processes. Epic explains that Shader Compile Workers provide process-level parallelism around compiler implementations that may otherwise serialize internally.

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

#### Graphics Pipeline and Mesh-Draw State Study

The review followed Epic's documented entry point at `FDeferredShadingSceneRenderer::Render`, then followed the source-derived API responsibilities into mesh-pass processing, RDG attachment extraction, pipeline-state assembly, and RHI binding. No licensed Unreal source checkout is present in the reviewed workspace, so this plan does not claim a line-by-line local reading of private implementation bodies. It relies on Epic's official architecture documentation and generated API pages, whose source locations identify `DeferredShadingRenderer.cpp`, `MeshPassProcessor.h/.cpp`, `RenderGraphResources.h/.inl`, `PipelineStateCache.h/.cpp`, and RHI headers. NVIDIA NVRHI provides the independent primary-source cross-check.

| Read path | Production responsibility observed | Sparkle conclusion |
| --- | --- | --- |
| `FDeferredShadingSceneRenderer::Render` and documented base-pass flow | sequences scene-rendering phases and calls focused draw systems | frame/feature orchestration does not author a backend-complete PSO |
| `FMeshBatch` -> `FMeshPassProcessor` -> `BuildMeshDrawCommands` | filters mesh work, selects shaders and pass state, and builds stateless draw commands | keep a focused GBuffer mesh collaborator; do not make graph construction enumerate native pipeline fields |
| `FMeshPassProcessorRenderState` | carries only pass-wide blend/depth-stencil/access/stencil and uniform-buffer overrides, then applies them to PSO state | adopt the narrow-state idea as an even smaller `RasterPassRenderState`; attachment access remains graph-owned and the complete aggregate is deleted |
| `FGraphicsMinimalPipelineStateInitializer` | holds mesh-draw shader/fixed-function state without render-target state | key shader, raster, depth/stencil, blend, and primitive facts separately from attachments |
| RDG `RENDER_TARGET_BINDING_SLOTS`, `FGraphicsPipelineRenderTargetsInfo`, and `ExtractRenderTargetsInfo` | make attachments authoritative for formats, count, depth/stencil access, load/store, and samples | derive only formats/count/depth/sample compatibility from resource descriptions; keep load/store/clear/access in graph execution and validate them against pass depth/stencil intent |
| `FGraphicsPipelineStateInitializer`, `SetGraphicsPipelineState`, and pipeline-state cache | assemble/cache/bind the complete state required by the RHI | retain a complete internal descriptor and key; never require feature authors to fill it |
| `FMeshDrawCommand` submission | binds shader parameters, vertex streams, cached/minimal pipeline identity, topology, stencil reference, and draw arguments | prepared draw work contributes mesh facts; dynamic recording data is not stored in a frontend PSO bag |
| NVRHI `GraphicsPipelineDesc`, `FramebufferInfo`, and command `GraphicsState` | separate reusable pipeline state, attachment compatibility, and per-command binding state | Sparkle's split is not Unreal-specific and maps cleanly onto both backend APIs |

The important Unreal lesson is where completeness occurs. The RHI initializer is intentionally large because a native graphics pipeline is a compound object. The authored mesh-pass surface is intentionally smaller because no single feature caller owns all compound facts. Sparkle should reproduce that ownership split, not copy Unreal prefixes, its full material/vertex-factory/permutation system, cached mesh-draw-command framework, or PSO-precache machinery.

The current Sparkle Phase 2 checkpoint exposes the following incomplete ownership:

| Current surface | Problem | Owning disposition |
| --- | --- | --- |
| `GraphicsShaderPipelineState` at `RasterizedGBuffer` | repeats six color formats, count, depth format, vertex layout, and fixed state next to graph declarations | Phase 5 deletes it; the graph, mesh/material, and pass state become the separate authorities |
| `FrameGraphBuilder::Draw(..., pipelineState, ...)` | forces the graph caller to construct a backend-shaped aggregate | Phase 5 accepts only typed shaders, narrow render state, and real draw work |
| `RenderPassRuntimeCache` keyed only by shader pair | rejects a second legitimate state for the same vertex/pixel shaders | Phase 5 introduces a complete immutable key assembled from all contributing authorities |
| `RasterPassPipelineRuntime` base/wireframe/two-sided bundle | eagerly creates speculative variants for every graphics runtime | Phase 5 lazily materializes only exact requested variants before recording; it does not add precaching |
| generic `ShaderPassOperations` view-mode selection | leaks semantic wireframe policy into generic shader binding | Phase 5 moves fill/cull choice to the mesh/material/pass owner and leaves binding semantic-free |
| `GBufferMeshPass::PrepareTargets` plus graph attachments | manually binds and clears targets already declared by the graph | Phase 5 makes attachment actions authoritative and removes the duplicate target preparation path |
| `GBufferMeshPass::ConfigurePipeline` plus `GpuMesh::Bind` | sets triangle topology in two owners | Phase 5 leaves topology with prepared mesh draw work exactly once |
| public RHI fields plus backend defaults | omits authored blend/sample authority while D3D12/Vulkan hard-code opaque, sample-count-one, triangle, and raster defaults | Phase 5 completes the neutral descriptor and makes paired backends consume every supported field or reject it loudly |

The target state contribution is therefore:

```text
typed shader references ----------- shader code + binding-layout identity --+
RasterPassRenderState ------------- blend + depth/stencil -------------------+
prepared mesh/material draw ------- vertex input + topology + fill/cull -----+--> GraphicsPipelineKey
frame-graph attachments ------------ target/depth formats + samples ----------+        |
                                                                                      v
                                                                     complete GraphicsPipelineDesc
                                                                                      |
                                                                        D3D12 / Vulkan object

dynamic command state: viewport, scissor, blend constants, stencil reference,
vertex/index streams, parameter bindings, and draw arguments
```

Pipeline materialization remains lazy but must finish before parallel command recording begins. The graph/runtime owner collects the finite exact keys requested by prepared draw work, resolves them once per active shader-map generation, then recording performs lookup and binding only. This is not precaching, prewarming, or a readiness system: no unrequested wireframe/two-sided combination is created, no driver cache is introduced, and no speculative future variant becomes an authored obligation.

Primary graphics sources:

- [Epic: Graphics Programming Overview and `FDeferredShadingSceneRenderer::Render`](https://dev.epicgames.com/documentation/en-us/unreal-engine/graphics-programming-overview-for-unreal-engine)
- [Epic: Mesh Drawing Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/mesh-drawing-pipeline-in-unreal-engine)
- [Epic: `FMeshPassProcessor`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FMeshPassProcessor)
- [Epic: `FMeshPassProcessorRenderState`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FMeshPassProcessorRenderState)
- [Epic: `FMeshDrawCommand`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Renderer/FMeshDrawCommand)
- [Epic: `FGraphicsMinimalPipelineStateInitializer`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FGraphicsMinimalPipelineStateIni-)
- [Epic: `FGraphicsMinimalPipelineStateId`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Renderer/FGraphicsMinimalPipelineStateId)
- [Epic: `TStaticDepthStencilState`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/TStaticDepthStencilState)
- [Epic: `FGraphicsPipelineRenderTargetsInfo`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FGraphicsPipelineRenderTargetsIn-)
- [Epic: `ExtractRenderTargetsInfo`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/ExtractRenderTargetsInfo)
- [Epic: `FGraphicsPipelineStateInitializer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FGraphicsPipelineStateInitialize-)
- [Epic: `SetGraphicsPipelineState`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/SetGraphicsPipelineState/2)
- [NVIDIA NVRHI programming guide: pipelines, framebuffers, and graphics state](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [NVIDIA NVRHI tutorial: pipeline and framebuffer separation](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/Tutorial.md)

For ray tracing specifically, current Unreal 5.8 keeps the same shader-type center but separates three responsibilities that Sparkle must also keep separate:

1. `FShaderType`/`FShader` carry root-parameter metadata, compile hooks, and a ray-tracing payload hook.
2. `FRayTracingPipelineStateInitializer` relates ray-generation, miss, hit-group, and callable shader tables plus the shared binding layout. It is pipeline composition, not another shader declaration system.
3. `RayTraceDispatch` receives the materialized pipeline, selected ray-generation shader, shader binding table, global parameters, and dimensions. Unreal 5.5 began replacing the older ray-tracing binding writer with the normal batched shader-parameter path; Sparkle should start with its one typed `Parameters` path and copy no deprecated writer layer.

Sparkle retains those responsibilities but removes repetition that its smaller frontend does not need: `GlobalShader` stage classes, ray-generation-owned root `Parameters` and shared payload/attribute/recursion compile contract, one focused typed `RayTracingPipelineComposition` containing only membership and hit-group policy, and `TraceRays<RayGenerationShader>`. The compiler validates each registered stage contract, while materialization derives the composition ABI from the authoritative ray-generation registration and map entry. The frame graph/runtime retains the materialized pipeline, table, and binding state that Unreal's lower-level RHI call receives explicitly. This preserves Unreal's authoring experience while respecting Sparkle's graph and RHI ownership.

Primary sources:

- [Epic: `FShader` root parameters and ray-tracing payload hook](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FShader)
- [Epic: `FRHIRayTracingShader` stage hierarchy](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRHIRayTracingShader)
- [Epic: `FRayTracingPipelineStateInitializer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FRayTracingPipelineStateInitiali-)
- [Epic: `RayTraceDispatch`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRHIComputeCommandList/RayTraceDispatch)
- [Epic 5.5 release note for batched ray-tracing shader parameters](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-5-release-notes)

### NVIDIA Donut and NVRHI

Donut's `ShaderFactory` resolves bytecode using a filename plus entry point and selects an embedded DXBC, DXIL, or SPIR-V variant when available. It may use the filename as the shader object's default debug name. Its deferred-lighting implementation nevertheless creates the shader from `deferred_lighting_cs.hlsl` and separately emits the semantic GPU marker `DeferredLighting`.

NVRHI keeps `shaderType`, `debugName`, and `entryName` as distinct fields in `ShaderDesc`; a shader library resolves a shader by entry name and shader type. This is useful evidence that the low-level RHI description should not be responsible for Sparkle's source-package authoring policy.

NVRHI's RT API also confirms that pipeline description, shader table, command state, and ray dispatch are separate mechanism objects. Sparkle adopts that separation and paired D3D12/Vulkan contract, but not NVRHI's name-addressed mutable/versioned shader table. Sparkle's Renderer owns typed logical records, and backend-private RHI materializes an immutable table tied to one exact pipeline generation.

Adopt:

- source plus entry point as load/compile inputs
- automatic backend-bytecode selection
- separate semantic GPU markers and debug names
- a narrow RHI descriptor that consumes bytecode instead of inventing renderer package ownership

Sources:

- [NVIDIA Donut `ShaderFactory.cpp` at `bfdebdd`](https://github.com/NVIDIA-RTX/Donut/blob/bfdebdd7dd5455c503b2737a1967a4ef651c145b/src/engine/ShaderFactory.cpp)
- [NVIDIA Donut `DeferredLightingPass.cpp` at `bfdebdd`](https://github.com/NVIDIA-RTX/Donut/blob/bfdebdd7dd5455c503b2737a1967a4ef651c145b/src/render/DeferredLightingPass.cpp)
- [NVIDIA NVRHI `nvrhi.h` at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/include/nvrhi/nvrhi.h)
- [NVIDIA NVRHI programming guide at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [NVIDIA NVRHI ray-tracing tutorial at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/Tutorial.md)

### AMD Cauldron and FidelityFX

Cauldron's reviewed D3D12 paths compile shaders with an explicit file, entry point, target profile, and define list. It hashes shader source recursively through includes and hashes defines. Its rendering code uses separate user-marker strings such as `GltfPbrPass::DrawBatchList`.

FidelityFX uses an effect-specific pass enum and permutation flags to select generated shader blobs, then gives the created pipeline a separate human-readable name. Its useful precedent for Sparkle is only the separation of executable identity from the human-readable pipeline label. Sparkle's unified migration does not adopt FidelityFX's effect/pass enum or generated-variant selector; the concrete shader class and typed draw/dispatch/trace already provide the required identity.

AMD's reviewed Vulkan capability path keeps ray-tracing-pipeline and ray-query feature structures distinct, and FidelityFX's denoiser sample keeps reusable ray construction/traversal/material helpers in shared HLSL. Sparkle adopts those two boundaries: independent capability truth and shared semantic kernels with thin execution frontends. Cauldron/FidelityFX do not provide a typed engine shader-class frontend comparable to Unreal's, so they are not evidence for adding another authoring macro, pass enum, program alias, or string-addressed pipeline layer.

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
- [AMD Cauldron ray-pipeline/ray-query capability separation at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/VK/base/ExtRayTracing.cpp)
- [AMD FidelityFX shared ray-tracing HLSL at `60f4ea8`](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Samples/Denoisers/FidelityFX_Denoiser/dx12/shaders/raytracing_common.hlsl)

### Acceleration-Structure Binding Decision

The Phase 0 baseline shadow duplication was not a legitimate shader-authoring variant. `DirectShadowSignalCS` and `DirectShadowSignalDeviceAddressCS` executed the same entry and shared algorithm; the latter existed only to define `SPARKLE_RAY_TRACING_SCENE_TLAS_DEVICE_ADDRESS`, omit the semantic `SceneTlas` parameter, and reconstruct an opaque acceleration structure from two address words stored in an effect uniform. The separate class, source root, pass wrapper, package, feature flags, parameter schema, address fields, and selection enum were one backend representation leaking through every frontend layer.

Production API precedent removes the reason for that leak:

- DirectX declares one HLSL `RaytracingAccelerationStructure` resource. D3D12 may bind it through an acceleration-structure SRV or a root SRV whose native value is a GPU virtual address; the HLSL shader identity does not change.
- NVRHI exposes one semantic `ResourceType::AccelStruct` binding and lets its D3D12/Vulkan implementations materialize API-specific layouts and bindings.
- Vulkan defines both `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` and `VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV`. `VkWriteDescriptorSetPartitionedAccelerationStructureNV` carries the PTLAS device address inside the native descriptor write. Therefore a PTLAS address is backend descriptor data, not an HLSL constant or a reason for another shader class.
- Sparkle already resolves an acceleration-structure binding to a classic or partitioned native descriptor in Vulkan command recording. The missing contract is to select and validate that completed descriptor route consistently instead of advertising only the raw-address alternative.

The frozen target is one `SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)` field on `DirectShadowSignalCS` and every other shader that traces the scene. The graph binds one `FrameGraphAccelerationStructureHandle`. The selected TLAS provider is fixed before shader layout/pipeline materialization, and private RHI chooses the exact native descriptor type and write structure. Renderer shader/effect code never receives a TLAS GPU address or access-mode enum. If a provider cannot complete that semantic binding on the active device, that provider is unavailable and selection retains the supported classic provider; Sparkle does not compile a second shader or silently branch on a raw address.

The Phase 0 Vulkan mutable-descriptor feature/bootstrap/layout scaffold had no independent consumer outside this attempted classic/partitioned binding switch. Because provider selection is fixed before layout creation, Phase 1 deletes that scaffold and creates the exact descriptor layout for the selected provider. Sparkle does not retain a device feature, `pNext` chain, or generalized layout mechanism for hypothetical run-time switching.

This is not the postponed permutation system. The catalog and map retain one code record per `(ShaderTypeId, Target)`, the shader author writes no define or mode, and the shader bytecode does not fork for the binding representation. A future backend that truly cannot implement the semantic binding must extend the RHI capability and binding contract with evidence; it may not create `*DeviceAddressShader`, `*DescriptorShader`, or an internal pseudo-permutation by convention.

The no-query shadow program is deleted rather than generalized. Shadow visibility consumed by direct lighting is mandatory, so a shaderless clear is not a valid producer and the user cannot disable the only real frontend. Phase 1 requires inline-query capability before graph construction and schedules the one `DirectShadowSignalCS` producer unconditionally. Phase 8 replaces that inline-only precondition with selection between inline query and a complete pipeline/RGS implementation; absence of both still fails before scheduling. A compute shader that recompiles the same lighting code only to skip traversal is equally invalid.

Primary sources:

- [Microsoft DXR acceleration-structure resource binding](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [NVIDIA NVRHI acceleration-structure binding model at `8e8c36e`](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [Khronos `VK_NV_partitioned_acceleration_structure`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_NV_partitioned_acceleration_structure.html)
- [Khronos partitioned-AS descriptor write](https://docs.vulkan.org/refpages/latest/refpages/source/VkWriteDescriptorSetPartitionedAccelerationStructureNV.html)

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

The fifth column in the next four design-space tables is a migration ledger: most entries record the Phase 0 baseline, while cells that explicitly name a later checkpoint record that checkpoint. It is not a uniformly current inventory. Use the [Shader Compilation Capability Inventory](../Modules/Tools/ShaderCompiler/README.md) for the dated current-source shape.

### Shader Definition and Graph Use

| Stage | Options on the table | Recommended choice | Main tradeoff | Baseline or named checkpoint |
| --- | --- | --- | --- | --- |
| 1. Platform and capability contract | lowest common feature set; per-platform shader platforms; runtime feature branching | Define explicit shader-platform capabilities and compile eligibility; reject unsupported combinations before jobs are scheduled. | More platform records, but failures become deterministic and the base request set stays bounded. | DXIL/SM and SPIR-V targets plus package feature flags exist; some feature validation and consumption branches are incomplete. |
| 2. Source namespace/import | physical paths; basename lookup; virtual mounted paths | Unreal-style `/Engine`, `/Project`, and `/Plugin/<Name>` virtual paths mapped to physical roots by tooling. | A mount table and collision rules are required; checkout-independent identity and portable diagnostics are gained. | Physical project-first/engine-second lookup, absolute includes, and basename package fallback. |
| 3. Language and modules | HLSL only; Slang only; HLSL plus opt-in Slang; generated kernels | Keep HLSL/DXC as the production reference path. Keep Slang as an explicit backend/language experiment until policy and output parity are proved. Permit generated source only through one provenance-recorded generator with source maps and deterministic regeneration. | One reference path is less flexible; two unqualified paths multiply conformance work. | `.hlsl` auto-selects DXC; `.slang` auto-selects Slang. Explicit backend selection can override that convention. |
| 4. Includes and reusable code | textual includes; language modules; generated amalgamation | Preserve small textual includes now, record the complete dependency graph, and evaluate Slang modules only where generics/composition remove measured duplication. | Textual preprocessing is simple but recompiles shared text; modules improve composition at toolchain and cache-complexity cost. | Recursive includes, `#pragma once`, recursion detection, and line directives exist. |
| 5. Executable shader type | infer every file; string registration; generated catalog; typed C++ registration | Lean immutable shader class with direct-binding parameters only when used, explicit virtual source/entry/stage, optional compile/RT ABI hooks, and declaration location. | Authors still state irreducible facts; invalid files and entry points are not guessed. | Mutable `GlobalShaderRegistry` entries with explicit source/entry/stage and repeated package string. |
| 6. Parameter and GPU ABI | hand-authored root signatures/layouts; reflection-only binding; typed parameter metadata | One typed shader-visible declaration drives reflection validation, backend layouts, and binding; reuse it directly or compose it into an RDG pass envelope with graph-only fields. | Code generation/metadata is needed; composition must preserve one owner for every shader-visible field. | Shader `FParameters` and pass parameters are separate; runtime has strong record validation but `PassBinder` retains a count-only compatibility path. |
| 7. Variants | preprocessor permutations; specialization constants; dynamic branches; separate source files | Keep one variant per shader/target in the unified migration; design typed permutations only as a later measured extension. | Avoids unused authoring/cache/map complexity now; later variants require one atomic extension. | Free-form defines and specialization records exist; no typed permutation domain. |
| 8. Stage composition | filename grouping; manual package grouping; typed stage types at use | Compute names one shader type; graphics names concrete vertex/pixel types with the real pipeline description; ray tracing uses a focused typed pipeline composition for exports and hit groups. | No universal program layer; genuine multi-stage call sites remain explicit. | Package ID groups stages; `GBuffer` is the only current VS+PS grouping. |
| 9. Pass/RDG declaration | pass strings select bytecode; pass owns shader references; implicit global lookup | Direct compute dispatch consumes `Shader::Parameters`; graphics/shaderless work uses a narrow real-owner envelope; default label derives from shader type with instance override. | Deletes forwarding wrappers while preserving real feature owners. | Pass name, package ID, binding-layout name, pipeline name, and duplicate parameters are repeated. |

### Compilation, Validation, Cooking, and Publication

| Stage | Options on the table | Recommended choice | Main tradeoff | Baseline or named checkpoint |
| --- | --- | --- | --- | --- |
| 10. Selection | compile all; package/shader selection; changed dependency closure; on-demand runtime compile | Cook all registered shader types for release; use shader-type selection for tools and reverse-dependency selection for development. Do not compile at shipping runtime. | Dependency metadata must be durable, integrity-checked, and regenerated with the current tool contract; development iteration becomes proportional to the edit. | CLI supports all, package, or shader ID. Editor `Changed` and `Global` both launch an unfiltered `cook`. |
| 11. Eligibility | compile every registered target; capability filter before scheduling; runtime compile | Apply target/stage/feature eligibility before one-variant jobs; do not compile at shipping runtime. | Simple bounded selection now; permutation enumeration is deferred. | Target capability skips exist. |
| 12. Preprocessing/dependencies | compiler-owned preprocessing; engine-owned preprocessing; both | Use one canonical Sparkle preprocessing/dependency pass for identity and diagnostics, then invoke the backend with controlled input. Validate that backend include behavior cannot introduce hidden inputs. | Engine preprocessing gives portability and replayability but must track compiler semantics accurately. | Sparkle preprocesses before both DXC and Slang and hashes the include closure. |
| 13. Compile request/input hash | path/timestamp key; package-scoped key; full content-addressed input hash | Immutable request plus `ShaderCompileInputHash` over virtual source closure, entry/stage, environment, compiler-affecting parameter metadata, target, backend, and compiler version. Exclude validation-only and package/pass presentation identity. | Larger hash construction cost; safe in-operation deduplication and deterministic invalidation. | `compileInputHash` combines source, include-closure, option, backend, and compiler-version identity for diagnostics; it is not a persisted-result lookup key. |
| 14. Scheduling/isolation | serial; in-process task parallelism; persistent local worker processes | Keep bounded `SparkleTasks` jobs, operation-local identical-input fan-out, and cooperative cancellation. Add priority or local worker processes only when measured scheduling or compiler-isolation need justifies them. | Processes isolate crashes/leaks and bypass compiler locks but add IPC, startup, deployment, and debugging cost. | One out-of-process cooker, 1-8 in-process compiler sessions, operation-local identical-input fan-out, and cooperative cancellation before emission. Priority and measured memory ceilings remain absent. |
| 15. Front end and intermediate form | DXC HLSL to DXIL/SPIR-V; Slang to DXIL/SPIR-V; source-specific compilers | DXC is the release oracle for HLSL. Slang output is accepted only after the same options, reflection, diagnostics, symbols, and paired-backend tests pass. | Compiler diversity finds issues and enables language features but doubles versioning and reproducibility obligations. | DXC is feature-complete for current HLSL. Slang is narrower and does not honor all cook policy switches. |
| 16. Optimization/code generation | debug/O0; optimized/O3; separate analysis variants; profile-guided/vendor compilation | Cook optimized runtime bytecode; produce replayable debug/symbol artifacts by policy; compare disassembly and resource use for selected acceptance shaders. | Debuggable code may differ from optimized execution; analysis must preserve provenance to avoid comparing the wrong binary. | DXC applies optimization/debug/strip policy; Slang policy parity is incomplete. |
| 17. Reflection and ABI validation | trust hand-authored bindings; runtime reflection; offline reflection plus runtime signature | Extract reflection for every compilation, normalize it, compare against typed parameters, persist a strong signature in cooked output, and revalidate integrity at load. | More cooker work and schema data; binding failures move out of rendering. | Strong cooker/runtime record validation exists; the authoring side still has duplicate parameter authorities. |
| 18. Static shader analysis | compiler warnings only; lint/validation; DXIL/SPIR-V validation; vendor ISA analysis | Warnings-as-errors in owned release shaders, DXIL/SPIR-V validators, bounded statistics, and opt-in RGA/Nsight/PIX analysis for representative hot shaders. | Vendor analysis is hardware/tool-version specific and cannot be a hermetic gate. | Warnings policy and cooked statistics exist; no complete validation/ISA regression lane. |
| 19. Compile diagnostics | console errors; dump every job; dump failures; reproducible job bundle | Structured source diagnostics plus a failure-first replay bundle containing preprocessed source, dependencies, arguments, compiler identity, reflection, and a one-job command. | Bundles consume storage; failure-only default controls bloat. | Successful opt-in DXC bundles exist; failed compiles and Slang do not have equivalent replay artifacts. |
| 20. Compile execution | compile every selected job; deduplicate identical jobs within one cook | Compile every selected shader input and deduplicate identical in-flight jobs only within the active cook. Persist no compiler-result data and expose no storage/configuration surface. | Repeated cooks spend compiler time again; the implementation has fewer owners, formats, invalidation rules, failure modes, and cleanup paths. | The local artifact store, key/status fields, cache directory, CLI flags, and Launcher controls are deleted in current committed source. |
| 21. Cooked logical map | per-pass file; global shader map; material/asset-local shader maps | Generated `GlobalShaderMap` from shader type/target to code hash and ABI metadata; no base program manifest. | Map indirection gives typed lookup and stable identity. | Per-package `.sparkshader`; generated registry is published but runtime does not use it as lookup authority. |
| 22. Code records and physical library | code embedded per package; indexed records; one library; project/plugin/chunk libraries | Emit exact code hashes and map references. Merge/compress/chunk only after measured byte, I/O, or patch benefit; never expose membership to passes. | A simple index is proportionate; richer libraries add formats and failure modes. | Bytecode is duplicated inside package containers. |
| 23. Compression/chunking | loose uncompressed records; block compression; whole-library compression; platform containers | Keep the first indexed migration format simple. Add independently compressed blocks/chunks only when startup, size, patch, or preload measurements justify them and retain integrity coverage. | Small blocks stream well but compress less; large blocks compress well but amplify reads and patch deltas; uncompressed records may win at current scale. | One container per program; no cross-program compression/chunk policy. |
| 24. Publication/security | overwrite in place; temporary files plus rename; generation manifest; signing | Deterministic transactional generation publication with hashes, schema/toolchain provenance, rollback, and optional platform signing at release packaging. | Keeping generations costs disk; it prevents partial activation and makes failures auditable. | Transactional packages, registry, and recook signal with stale-generation rejection already exist. |

### Runtime Loading, Residency, Pipelines, and Execution

| Stage | Options on the table | Recommended choice | Main tradeoff | Baseline or named checkpoint |
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

| Stage | Options on the table | Recommended choice | Main tradeoff | Baseline or named checkpoint |
| --- | --- | --- | --- | --- |
| 38. Semantic labels and provenance | human names only; hashes only; both | Carry readable event/shader names plus stable shader-type, code, pipeline, and generation identities into bounded markers, object names, captures, and crash records. | Extra metadata has storage/runtime cost; bounded development metadata makes evidence joinable without cluttering normal control flow. | Readable names exist, stable cooker-to-capture hash correlation does not. |
| 39. Source debugging | visual output; shader printf/asserts; source debugger; replay capture | Support all four: cheap visualization first, bounded debug instrumentation, external PIX/RenderDoc/Nsight debugging, and reproducible compiler/capture artifacts. | Debug builds and instrumentation alter timing and code generation; conclusions must identify the binary used. | Visualize Buffers and capture labels exist; symbol/provenance workflow is incomplete. |
| 40. Performance analysis | compiler statistics; IR/disassembly; vendor ISA; GPU counters/timelines | Preserve DXIL/SPIR-V inspection, add selected RGA/Nsight analysis, and correlate static register/LDS data with runtime occupancy, cache, bandwidth, and latency evidence. | Static estimates do not prove runtime bottlenecks; hardware results are device/driver specific. | Cooked byte/reflection stats exist; no joined static/runtime shader evidence. |
| 41. Hot reload invalidation | reload all; timestamp scan; content/dependency invalidation; runtime patch-in-place | Changed virtual paths select reverse dependencies; produce a complete validated generation; atomically swap; never patch individual live entries. | Whole-generation publication duplicates some data briefly but keeps rollback and lifetime coherent. | Timestamp polling notices `.hlsl`, `.hlsli`, and `.slang`; the all-catalog plan recompiles every job; generation swap is safe. |
| 42. Cancellation/failure | abort process; cooperative job cancellation; keep partial output; transactional rollback | Cooperative cancellation at job/backend/process boundaries; discard incomplete generation; keep previous active generation; initial startup failure is explicit. | Some compiler calls are not interruptible; process isolation may be needed to bound cancellation. | A private Application-to-cooker signal cancels pending work and prevents emission; active compiler calls finish before settlement. The parent no longer kills the process during publication, and a cancellation arriving after commit begins loses to the coherent transactional file set. |
| 43. Lifetime/retirement | device idle; immediate destroy; queue-fence/token retirement | Reference resources through the active generation and retire old shader/pipeline/library state only after every recorded queue submission token completes. | More bookkeeping and transient overlap; no global device-idle reload hitch. | Implemented by `RenderPassRuntimeCache`; preserve it. |
| 44. Release evidence | successful compile; one smoke frame; paired deterministic evidence | Prove clean and repeated cooks, repeated-operation recompilation, map/library lookup, lazy materialization cost, D3D12/Vulkan captures, hash-to-source lookup, representative disassembly, fallback behavior, and latency/memory. | Evidence takes hardware time and storage. | Catalog validation and one representative CLI artifact exist; paired hardware evidence is incomplete. |

The recommended base path is intentionally conventional at the API boundary: offline high-level compilation, typed maps/libraries, conventional complete pipelines, and lazy graph-time materialization outside Execute. Asynchronous preparation, Vulkan shader objects, graphics pipeline libraries, Vulkan pipeline binaries, D3D12 partial programs, and work graphs remain options, not assumed improvements. Each adds a capability branch and requires its own proposal only after current pipeline counts, first-use timings, and a representative workload show that the simpler path is insufficient.

## Current Implementation Snapshot

The dated registration, cook, publication, runtime, and frame-consumption inventory lives in the [Shader Compilation Capability Inventory](../Modules/Tools/ShaderCompiler/README.md). Keep implementation counts and coverage there; this architecture owns only the enduring distinctions among declaration, cooking, materialization, selection, and exercised behavior.
## Runtime Residency and Deferred Delivery Taxonomy

Compile inputs, cooked runtime data, live backend objects, and native driver acceleration have different owners and lifetimes. Sparkle persists no compiler result. The first four rows are unified-migration responsibilities; streaming, explicit native pipeline persistence, and prewarming remain deferred research.

| Layer | Prevents or reduces | Correct key/invalidation owner | Recommended policy | Current Sparkle |
| --- | --- | --- | --- | --- |
| dependency/preprocess record | rescanning and re-deriving include closure | virtual source contents, preprocessor provenance, defines; ShaderCompiler | persist a reverse-dependency index; regenerate when the current tool contract or integrity check rejects it | implemented as the transactionally published shader dependency manifest used by `--changed` selection |
| in-flight job coalescing | duplicate concurrent compilation | full `ShaderCompileInputHash`; compile coordinator | one producer with result fan-out; retain nothing after the operation | implemented within one cook; every later cook recompiles its selected inputs |
| global shader map | repeated discovery and unstable lookup | shader type/target; ShaderCompiler/Renderer generation | immutable per-generation entries with strong references to code hashes | implemented as `GlobalShaderMap.smap` and used by typed runtime lookup |
| cooked code records/library | duplicate bytecode and scattered I/O | exact code hash; ShaderCompiler/Renderer generation | independently indexed and integrity checked; merge/compress only after measured benefit | implemented as the content-addressed `CookedShaderLibrary.slib`; no package reader remains |
| OS/file/decompression cache | disk reads and decompression | physical records/chunks; platform I/O/runtime service | measured reads; add block compression and decompressed residency only if the selected format needs them | ordinary per-file reads; no explicit streaming/decompression layer |
| code and transient backend-object reuse | repeated code reads or backend object creation | code hash + backend creation identity; runtime/RHI | code/object lifetime follows map/runtime generations; keep the current lazy owner and add no second reuse authority without evidence | `RenderPassRuntimeCache` owns lazy compute/graphics/RT materialization and submitted-state retirement for each active/replaced generation |
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
| compile synchronously on first use | minimal startup and enumeration work | visible frame hitch and nondeterministic driver work | Reject for required or common pipelines. Retain only as a classified late path. |
| enumerate every theoretical PSO | strongest nominal coverage | combinatorial cook/startup/memory bloat and unused work | Reject. A future proposal may consider only measured, reachable pipeline descriptions. |
| explicit precache declarations | deterministic and reviewable for bounded shaders | authors/collectors can omit a legal state | Defer until measured first-use evidence requires it. |
| recorded/bundled usage cache | captures content combinations actually exercised | misses unplayed branches and can go stale across shader/content changes | Add with material/content PSOs; use stable descriptions, never raw transient hashes alone. |
| native driver cache only | little engine work | opaque coverage, device/driver invalidation, first-run hitches | Use as an acceleration layer, never as the only plan. |
| hybrid explicit + recorded + native | covers known global state, observed content, and repeated driver work | most coordination and telemetry | Research option only; far beyond current need. |

A future prewarming proposal must define its own minimal state/evidence contract. This migration adds none of those states or diagnostics.

## Ray Query Versus Ray-Tracing Pipeline and SBT

Sparkle must teach and report these as two different execution architectures.

```text
INLINE RAY QUERY (IMPLEMENTED SOURCE AND ESTABLISHED PRODUCT ROUTES)

compute shader + compute PSO
        |
        +--> bind TLAS/AS descriptor (or a supported address representation)
        +--> RayQuery / Proceed inside normal compute invocation
        `--> Dispatch

Required: BLAS/TLAS, AS resource/binding, inline-ray-query compiler/API feature
Not required: RT library exports, RT state object/pipeline, shader identifiers, SBT, DispatchRays

FULL RAY-TRACING PIPELINE (SOURCE ROUTE PRESENT THROUGH PHASE 8; EXECUTABLE PROOF DEFERRED)

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
| library/pipeline granularity | one large library/RTPSO; collections/libraries plus link; one pipeline per effect | Begin with one small complete effect pipeline whose `RayTracingPipelineComposition` names concrete RT shader-class references; do not add a universal `TRayTracingProgram` authoring type. Measure compile/link before introducing collections. | Large pipelines maximize shared optimization but are slow and hard to replace; many small pipelines duplicate work and switches. |
| shader exports and hit groups | string lists; generated typed IDs; reflection discovery | Typed declared exports/hit groups validated against compiler output; persist stable logical IDs but query native identifiers per native pipeline. | Native shader identifiers/group handles are pipeline-specific and cannot be serialized as universal shader identity. |
| global/local parameters | all global descriptors; local root/record data; bindless indices/device addresses | Derive the global layout from the selected ray-generation shader's `Parameters`; keep most resources global/bindless and SBT record data to small stable indices/constants only. Do not repeat the root schema on miss/hit/callable stages. | Fat or duplicated local records multiply authoring drift, SBT memory, upload bandwidth, and update complexity. |
| SBT organization | per instance; per geometry/material/ray type; deduplicated records with indirection | Explicit formula and one ray type in the first vertical slice; use TLAS instance contribution plus geometry/ray offsets; deduplicate material/geometry data outside SBT. | Indirection reduces memory and churn but adds shader loads and indexing complexity. |
| SBT update | rebuild every frame; patch dirty ranges; persistent GPU-generated table | CPU-build an immutable/persistent table per validated scene generation first; add dirty-range or GPU generation only from measured update cost. | Full rebuild is simple but scales poorly; incremental/GPU updates complicate synchronization and validation. |
| alignment/layout | backend-specific code paths; one conservative cross-API layout; normalized builder with backend rules | One backend-neutral record builder that applies D3D12/Vulkan handle size, base alignment, record alignment/stride, region, and bounds rules explicitly. | A conservative maximum wastes memory; backend-specific packing needs paired tests. |
| pipeline/SBT lifetime | SBT independent; rebuild on pipeline change; cache native handles | Tie SBT records to the exact native RT pipeline generation whose identifiers/group handles they contain; retire both after GPU completion. | Rebuild/upload on pipeline reload is mandatory, but stale identifiers cannot execute. |
| recursion/stack | maximum device limits; fixed conservative values; shader-derived measured policy | Start with recursion depth 1 and explicit payload/attribute contracts; query/report stack information and increase only for a demonstrated algorithm. | Higher recursion/payload/stack improves expressiveness while reducing occupancy and raising memory/driver cost. |
| dispatch integration | direct command calls; neutral RHI `TraceRays`; RDG typed RT pass | Add backend-neutral RT pipeline/SBT descriptors and a typed frame-graph ray-dispatch pass in one vertical slice. | Partial schema support without command/execution ownership creates misleading dead infrastructure. |

Current committed source carries RT exports, hit groups, payload/attribute/recursion metadata, and local-record contracts in the final global-map/library representation. It also contains typed Renderer RT shader classes and compositions, D3D12 state-object and Vulkan RT-pipeline creation, identifier/group-handle retrieval, neutral checked shader-table packing, typed frame-graph trace dispatch, generation lifetime, dual GBuffer/shadow frontends, alpha any-hit, and the shared two-ray-type scene table plan. That is source reachability, not paired executable conformance. Portable shader-visible nonzero local records, intersection/callable execution, native correctness, parity, reload/retirement, and performance remain Phase 12 obligations. No phase adds precache telemetry.

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
- [Engineering task map](../../Engineering/README.md#choose-by-task) selects the binding foundation, module, and verification rules.
- [Bistro and San Miguel workloads](../../Acceptance/GraphicsWorkloads.md) define the paired-backend stress cases and evidence shape.
- [Renderer/RHI boundary](../Decisions/RendererRhiBoundary.md) owns the architectural split among Renderer policy, frame-graph scheduling, and neutral backend creation.

`Meets` below means the reviewed code implements the shader-specific invariant. It does not mean the corresponding portfolio requirement is fully proven. `Partial` means a useful production path exists but misses a required invariant or evidence gate. `Missing` means no production path or executable proof was found. `Explicitly deferred` means the repository correctly exposes the feature as unavailable rather than silently pretending it works.

### Engineering-Standard Compliance

| Authority | Current shader-lifecycle status | Required architectural response |
| --- | --- | --- |
| [Integration style](../../Engineering/Workflow/ChangeIntegration.md) | **Strong source structure, delivery incomplete.** The package/pass-wrapper and duplicate graphics-state authorities are gone; one typed catalog, map/library publication route, runtime generation, and graph materialization path remain. Later effect-plan and final evidence phases are still open. | Preserve the clean break, complete only the remaining phase-owned slices, and reject aliases, compatibility readers, or a second shader/runtime authority. |
| [Repository ownership](../../Engineering/Foundations/ModuleOwnership.md) and [Renderer/RHI boundary](../Decisions/RendererRhiBoundary.md) | **Strong source structure.** Renderer owns concrete shader classes, graph use, generation policy, RT compositions, and scene-table planning; Tools owns virtual-source resolution, compilation, dependencies, and publication; RHI owns neutral cooked/runtime contracts and backend lowering; Application/editor orchestrate recook and activation. | Preserve the split and run the boundary check whenever implementation moves it. Native handles, table packing, and backend capability details must not migrate into feature policy. |
| [Renderer](../../Engineering/Modules/Renderer.md) and [RHI engineering](../../Engineering/Modules/RHI.md) | **Partial.** DXIL/SPIR-V compilation, reflection, map/library validation, complete graphics state, explicit RT source routes, backend capabilities, and readable labels exist. Paired execution, inspection, disassembly/counters, alternate-path/failure captures, and exact hardware/driver evidence remain incomplete. | Make paired-backend executable slices the proof; inspect layouts and IL on both targets; preserve each explicit supported alternate and reject missing mandatory work; record exact compiler, backend, hardware, driver, workload, and capture. |
| [Editor](../../Engineering/Modules/Editor.md) and [Tools engineering](../../Engineering/Modules/Tools.md) | **Strong source structure, executable proof deferred.** Cooking is out of process; publication is transactional; stale generations are rejected; previous accepted artifacts survive failure. Cooperative cancellation prevents pre-commit emission, and failures produce one bounded replay bundle. Progress/result UX and measured memory evidence remain incomplete. | Preserve transactional replacement. Add focused progress/results integration and prove cancellation, failure, and bounded-memory behavior through the Phase 12 executable route. |
| [Tasks Engineering](../../Engineering/Modules/Tasks.md) | **Strong source structure, executable proof deferred.** Bounded work uses one `SparkleTasks` graph with no second general pool, operation-local identical-input fan-out, deterministic result integration, and cooperative cancellation boundaries. It still lacks priority, serial-versus-N evidence, and measured memory-ceiling proof. | Never add a shader-only general worker pool. Prove serial, 1/2/N, cancellation, stale generation, failure, and memory behavior. Add worker processes only from measured compiler isolation/throughput need. |
| [Data-oriented design](../../Engineering/Foundations/DataAndMemory.md) | **Strong source structure, measurement incomplete.** Immutable jobs, sorted map entries, compact reflection records, content-addressed library code, and typed hash lookup replace path-led package loading and duplicate package bytecode. | Preserve one owner per identity and measure cook bytes, map/library open cost, runtime residency, and generation overlap before adding compression, streaming, or another cache. |
| [Naming and vocabulary](../../Engineering/Foundations/Naming.md) | **Strong source structure.** Shader type, compile-input hash, code hash, map entry, pipeline key, graph label, and runtime generation are distinct; authored package/program/pass-wrapper identity is absent. | Keep labels presentation-only, reserve hashes for stable identity, and do not leak native `PSO` or backend vocabulary into neutral public contracts. |
| [Validation, performance, and evidence](../../Engineering/Verification/ValidationAndEvidence.md) | **Missing as a complete gate.** One representative CLI target exists, but no paired all-shader validation, injected-defect proof, compile-time report, first-materialization evidence, or external capture pack was found. | Extend existing product/tool validation surfaces for every replaced contract, use temporary removed-before-handoff harnesses only where necessary, then run the paired DXIL/SPIR-V cook/load slice, negative ABI/compile/reload cases, serial/N matrices, first-materialization measurements, and capture-backed evidence. A document or successful build is not acceptance evidence. |
| Frame-graph execution ownership | **Strong source structure, executable proof deferred.** Graph setup declares resources from typed parameters and materializes generation-bound runtime state before Execute; Execute binds and records without shader I/O or pipeline creation. | Preserve one shader-visible schema, narrow graph-only envelopes, structural validation, and the no-creation-in-Execute invariant; prove them with the Phase 12 negative and capture routes. |
| Reload and GPU lifetime | **Meets the reviewed invariant.** A replacement generation is fully built before activation, a failure preserves the active generation, and retirement waits on `RhiSubmissionToken` state for all queues. | Preserve this path unchanged while replacing lookup and physical storage. Test delayed GPU completion, reload churn, invalid replacements, and device-loss/error paths. |

### Portfolio Requirement Contribution

The shader architecture directly owns `PGE-09` and contributes evidence to several broader requirements. It cannot alone complete requirements that also need scene, workload, GPU-capture, productization, or adoption evidence.

| Requirement | Current contribution | Missing proof or design gate |
| --- | --- | --- |
| `PGE-01` Partner adoption and collaboration | **Indirect partial.** CLI/editor workflows and architecture notes can become an adoption surface; no second-engineer shader integration record is proved here. | Capture adopter constraints, review history, setup/cook/debug/fallback steps, measured outcome, issue/reproducer, and second-person reproduction without requiring hidden repository knowledge. |
| `PGE-02` Real-time ray tracing, GI, and path tracing | **Source route present; executable proof blocked.** Sparkle has BLAS/TLAS, inline ray query, native pipeline/SBT source, dual GBuffer/shadow frontends, alpha any-hit, and one scene contribution plan. | Prove raster, ray-query, native pipeline, explicit supported-alternate, and missing-mandatory-product behavior under Bistro/San Miguel quality, temporal, latency, memory, and paired-capture gates in Phase 12. |
| `PGE-03` Neural graphics product feature | **No shader-lifecycle feature proof.** The architecture can carry future generated or fixed inference shaders, but it does not provide a trained model, runtime inference, or classical fallback. | Reuse this exact shader-class/map/library/ABI/pipeline/provenance path for a real model artifact and shader kernels; do not create a neural-only compiler/runtime authority. Evidence remains owned by the neural workload. |
| `PGE-04` Model-to-kernel translation | **Future contribution only.** Slang is a backend seam, but no model-to-shader generator, operator contract, or generated-kernel runtime was found. | Use provenance-recorded generated virtual sources/source maps, deterministic regeneration, the normal compile request and ABI validation, numerical reference checks, latency/memory, disassembly/counters, precision/layout/fusion decisions, and classical fallback. |
| `PGE-05` Whole-system performance | **Partial.** Cook summaries/code statistics and bounded shader-table bytes/build-update metrics exist in source; compile, map/library-open, native-pipeline, frame-impact, and percentile distributions remain unproved. | Record compile queue/wall/CPU time for every selected job, map/library open time, lazy pipeline creation, frame pacing, memory high-water, and p50/p95/p99 under a pinned workload. Native-cache, streaming, and preloading experiments belong to later measured proposals. |
| `PGE-06` Workload analysis and hard debugging | **Partial.** The catalog targets DXIL/SPIR-V and both backends expose markers/debug names, but captured shaders are not joined to compile provenance. | Capture the same workload on both APIs; inspect queues, barriers, descriptors, memory, pipelines, shaders, symbols, and one hard incident with hypotheses, experiments, root cause, and minimal reproducer. |
| `PGE-07` C++ and Python software engineering | **Partial.** A C++ CLI, out-of-process orchestration, transactional cook, and runtime validation exist; no useful Python shader-analysis automation is required or proved by this design alone. | Keep the C++ ownership narrow and tested; add Python only for a concrete report/conformance/analysis workflow; provide clean-clone commands, deterministic artifacts, and documentation matching executable behavior. |
| `PGE-08` Applied mathematics and modeling | **Indirect.** Shader infrastructure cannot prove estimator, signal-processing, stability, or cost mathematics. | Let shader-type metadata link to the owning feature's math/reference tests and preserve exact compile-input/code/capture identity so predicted cost/quality can be compared with measurement. |
| `PGE-09` Explicit APIs, shaders, compilers, and GPU ABI | **Partial.** Explicit D3D12/Vulkan, HLSL to DXIL/SPIR-V, reflection, cooked ABI validation, and diagnostics exist. | Produce a paired shader-source-to-runtime trace; inspect both compiled forms; prove layout/resource states and complete support matrix; inject defects; verify real fallbacks; join code/pipeline hashes to GPU events. |
| `PGE-10` CPU/GPU architecture and concurrency | **Partial.** Bounded cooker tasks and async-compute scheduling exist. | Compare serial/1/2/N compile execution with time/memory/cancellation; correlate IR/ISA register/LDS/scratch findings with runtime occupancy, divergence, cache/bandwidth, and synchronized queue evidence. |
| `PGE-11` Machine-learning fundamentals | **Out of shader-lifecycle scope.** No compile/package design demonstrates training, objectives, splits, optimization, quantization, or generalization. | Do not claim coverage. A future generated shader path consumes an independently validated frozen model artifact and records provenance; training evidence stays in its owning workflow. |
| `PGE-12` Training and inference workload engineering | **Partial infrastructure only.** The global shader map and code library are validated, atomically published, and lazy-materialized, which can support deterministic inference deployment; no real inference workload exists. | Measure export, cook, map/library open, lazy materialization, and inference latency/memory separately from training; preserve an explicit classical fallback under one normal runtime path. Variant and preload policy remain outside this unified migration. |
| `PGE-13` Productization, tools, and communication | **Partial.** CLI discovery/inspection, editor recook, and this source-linked design are credible beginnings. | Deliver edit-to-failure/replay/reload/trace workflows, stable navigation, clean cook/run, troubleshooting, bounded reports, adoption feedback, and deletion evidence for replaced concepts. |
| `PGE-14` Platform and ecosystem breadth | **Partial.** Windows D3D12/Vulkan code paths and compiler/tool references exist; native Linux/Vulkan behavior is not proved by this document. | Record OS, SDK, compiler, driver, capture/profiler, and build setup. Add native Linux/Vulkan cook-run-capture only before claiming it; keep platform limitations in the support matrix. |
| `PGE-15` Principal judgment and sustained influence | **Design target, not proof.** The proposal removes repeated authority, rejects premature streaming/RT/compiler complexity, and selects measured gates. | Demonstrate completed vertical slices, deleted old paths, fewer authored concepts, preserved capability/error quality, causal evidence, review/adoption, and a repository that became easier to explain and maintain. |

### Phase 0 End-to-End Lifecycle Gap Inventory

The table below originated as the pre-migration gap inventory used to design the phases and is retained as a migration ledger. Some rows include phase-local reconciliation, so it is not a uniformly current implementation inventory. Use the [Shader Compilation Capability Inventory](../Modules/Tools/ShaderCompiler/README.md) for the dated source snapshot and the per-phase source-consistency sections for migration context; Phase 12 remains the only executable acceptance owner.

| Lifecycle stage | Verdict from reviewed code | Recommended end state | Acceptance evidence |
| --- | --- | --- | --- |
| Write shader source | **Partial.** HLSL/HLSLI and recursive includes work, but physical search roots, absolute includes, and project-first shadowing define identity. | Canonical `/Engine`, `/Project`, and `/Plugin/<Name>` mounts; deterministic include ownership; no authored absolute path. | Mount collision, traversal, case policy, same-basename, source-move, and cross-checkout key tests. |
| Declare shader type | **Partial.** Explicit source, entry, stage, feature flags, and parameter descriptor exist. Static registration silently drops duplicates and freezes implicitly on first snapshot. | Lean immutable shader class with nested `Parameters`, declaration location, explicit catalog freeze, collision errors, and only compile hooks consumed by the base implementation. | Duplicate/late registration negative tests and a readable catalog dump. |
| Declare parameters and RDG resources | **Unsafe partial.** Typed pass resources drive graph declarations; a separate shader `FParameters` drives reflection; count-only binding compatibility can accept different layouts. | One schema owns every shader-visible field and is reused directly or composed into a pass envelope with graph-only fields; binding and structural signatures derive from that schema. | Direct one-shader, graph-only/copy, and composed-pass tests; a field reorder/kind/name/visibility/array/size defect fails before execution on both backends. |
| Name executable stages | **Partial.** Shared package strings group stages and allow the valid multi-file `GBuffer` case. | Compute dispatch names one compute shader class; graphics draw names the concrete vertex/pixel classes, narrow pass state, and prepared draw work while other PSO facts derive from their owners; RT uses only the focused typed composition required for exports/hit groups. | Direct compute, VS+PS graphics, and all-stage RT composition tests; no authored package string, universal program alias, complete caller PSO, or pass-registration macro. |
| Select permutations | **Explicitly deferred.** Variants are represented through free-form defines and separate registrations/packages. | Keep one registered variant per shader/target in the unified migration and add no permutation frontend, enumeration, callback, or cache dimension. | Final searches prove no new permutation API or policy; a future proposal must provide its own workload, owner map, and acceptance evidence. |
| Build compile input and dependencies | **Partial.** Options and transitive include contents are hashed, but physical directories and path bytes leak into identity. | One immutable compile request with virtual source identity, normalized dependency graph, compiler provenance, platform/features, parameter signature, and debug policy. | The same source tree in different checkout roots produces the same input hash; every meaningful input change invalidates it. |
| Schedule compilation | **Strong source structure, executable proof deferred.** Explicit logical jobs, `ShaderCompileInputHash`, operation-local identical-input fan-out, cooperative cancellation, and bounded deterministic result application use one `SparkleTasks` graph. Priority and measured session-memory evidence remain absent. | Preserve one task runtime and add priority or worker processes only from measured need. | Phase 4 serial/1/2/N, duplicate fan-out, cancellation, backend failure, stale result, and memory-ceiling checks. |
| Compile DXIL/SPIR-V | **Strong foundation, incomplete parity proof.** DXC and Slang expose target capabilities; DXC produces DXIL/SPIR-V and rich successful analysis artifacts; reflection is extracted for both formats. | A backend-neutral result contract with equivalent diagnostics/provenance and declared analysis capability differences. | Representative optimized DXIL and SPIR-V builds for all supported stage kinds; reflection/layout comparison and compiler-version record. |
| Enforce compiler policy/capabilities | **Unsafe partial.** Target/package capability filters exist, but DXC and Slang do not honor the same policy controls and schema-known stages exceed executable runtime support. | Generated matrix for language, backend, target, stage, package kind, feature, and debug/optimization/warning/symbol policy; unsupported requests fail before jobs. | Every matrix cell is produced by a capability probe and executable positive/negative test; no ignored policy or unreported target skip. |
| Compile selected shader inputs | **Simplified in current committed source.** Every selected job invokes the compiler; the local artifact store and its key/status/configuration surface are deleted. | `ShaderCompileInputHash` is independent from package/pass presentation identity and exists only for in-operation deduplication, diagnostics, and provenance. | Repeated identical cooks both compile; identical jobs within one operation fan out one result; cancellation and deterministic publication remain proven. |
| Cook and publish runtime data | **Strong publication, coarse storage.** Per-program packages, registry, and recook signal publish transactionally; code is duplicated and registry is not runtime authority. | One generated `GlobalShaderMap` plus unique code-hash library; physical file policy remains cooker-owned and invisible to Renderer callers. | Reproducible map/library hashes, one copy per unique bytecode, transactional rollback, and map/library validation. |
| Inspect and reproduce compilation | **Partial.** Catalog/package inspection and successful opt-in artifacts exist. | One trace command plus always-available failure bundle containing virtual dependencies, preprocess output where available, exact arguments/defines, compiler/version, parameter comparison, and replay command. | Injected syntax/include/ABI/backend failures replay outside the editor and navigate to portable virtual paths. |
| Load and validate runtime shader | **Strong.** Cooked schema, target format, hashes, records, stages, features, reflection, and logical layouts are validated before use. | `ShaderRef<Shader>` resolves through the active `GlobalShaderMap` and code hash, independent of physical library filenames. | Cold/warm map/library open time and memory, corrupt/truncated/wrong-backend/wrong-layout records, and typed lookup across generation replacement. |
| Prepare code and native objects | **No additional owner required in the base migration.** Current materialization is lazy and pass-runtime-owned. | Keep `RenderPassRuntimeCache` as the sole generation/lifetime owner, materialize during graph construction, and keep Execute free of loading or creation. Add preload/readiness only in a future measured proposal. | Map/library/native-object/pipeline timing and high-water evidence; backend-specific release tests; proof that Execute performs no loading or creation. |
| Integrate with RDG | **Partial to strong.** Resource uses are declared during setup; runtime is materialized before Execute; Execute only binds and records. | `Shader::Parameters` is the one shader-visible schema consumed directly by typed draw/dispatch; only real graph-only data uses a narrow owner envelope; optional per-instance event labels remain presentation. | Direct compute, graphics, composed, and shaderless pass tests; graph resource-state explanation, async-queue legality, no hidden creation in Execute, and meaningful capture markers. |
| Create/use graphics and compute pipelines | **Unsafe graphics frontend despite a correct lazy boundary.** Complete descriptors reach RHI, but GBuffer authors a duplicated aggregate, the key is only the shader pair, variants are eager, and backends supply unowned defaults. | Derive the complete graphics key/descriptor from typed shaders, narrow pass state, attachments, and prepared mesh/material work under `RenderPassRuntimeCache`; materialize exact requests before recording. Do not add precache or native-cache integration. | Key-field perturbation, attachment-derived compatibility, exact-only variant, paired native-state/capture, cold/warm timing, generation reuse, and no creation in recording. |
| Debug/profile on GPU | **Partial.** Semantic events and native object names exist; compiler artifacts are not joined to captured shader hashes or external symbols. | Stable capture correlation record from event label and pipeline identity to shader type, code hash, virtual source, compile request, and debug symbol. | PIX and RenderDoc paired captures plus Nsight or RGA analysis on the exact cooked shader; record counters, IL/ISA, hardware, driver, API, and workload. |
| Recook and hot reload | **Strong foundation, coarse invalidation.** Out-of-process cook, transactional signal, stale rejection, rollback, generation swap, and GPU-safe retirement exist; any change plans the whole catalog. | Persist reverse dependencies, select affected shader types, publish a complete new generation, and retain current rollback/lifetime behavior. | Root/include change selection, unrelated-shader exclusion, rapid edit coalescing, invalid replacement, delayed completion, and reload-churn tests. |
| Inline/pipeline capability and mandatory shadow production | **Dual source route present.** Duplicate no-query/device-address shaders are deleted; one shadow plan selects inline or pipeline before graph construction and rejects absence of both. | One semantic acceleration-structure parameter, scene table plan, and graph product serve classic/partitioned providers and both frontends; private RHI retains native descriptor/table lowering. | Phase 12 forced-provider paired tests/captures prove exact native descriptor writes, same-frame output parity, actionable unavailability, and no fabricated shadow product. |
| Full RT pipeline and SBT | **Product source route present; executable acceptance blocked.** Final map/library metadata, neutral descriptors, backend-private D3D12/Vulkan pipelines/tables/trace commands, typed graph trace, generation retirement, GBuffer/shadow compositions, alpha any-hit, and the two-ray-type scene table plan exist in committed source. | Phase 9 may expand coherent effect coverage; Phase 12 must close portable nonzero-local-record, intersection/callable, overall readiness, native execution, parity, reload/retirement, and performance evidence without a test-only product path. | Raygen/miss/hit/intersection/callable execution on D3D12/Vulkan; identifier-generation, alignment/index/bounds, local-record consumption, reload/retirement, corruption, alternate selection, mandatory-producer failure, cold/warm pipeline, SBT memory/update, and capture evidence. |
| Automated conformance | **Missing as a complete gate.** The CLI validation custom target covers catalog structure and one shader artifact only. | Extend existing tool/product validation and CI evidence gates for identity, dependency, compilation, layout, schema, and paired end-to-end cook/load/reload/pipeline behavior; use temporary local harnesses only when an existing surface cannot falsify the invariant. | Named executable checks, injected failures, all-shader catalog cook, paired backends, `architecture_boundary_check`, clean diff/build evidence, and no submitted test-only scaffold unless separately authorized. |

### Required Evidence Pack

Implementation is not accepted merely when the new API compiles. The completed shader lifecycle must produce one navigable evidence pack for a pinned engine commit and workload:

1. an authoring trace from the concrete shader class and nested `Parameters` to virtual source, entry, parameter signature, compile-input hash, code hash, global-shader-map record, runtime shader reference, pipeline identity, and GPU event, plus a graphics draw trace from pass state/attachments/mesh-material facts to the complete key and native object;
2. optimized DXIL and SPIR-V artifacts for representative graphics stages and compute shaders, with reflection/layout comparison and readable compiler diagnostics;
3. one deliberately broken binding contract and one deliberately broken compile/include, both rejected with portable source locations and replayable failure bundles;
4. repeated cook results for serial, 1, 2, and N workers, including selected/compiled job counts, wall/CPU time, peak memory, cancellation, in-operation deduplication, and identical-output checks;
5. cold and warm D3D12/Vulkan pipeline results, including exact requested variant counts, attachment/state parity, graph-time creation, generation reuse, first-use frame impact, and proof that recording performs no loading or creation;
6. paired PIX and RenderDoc captures for the same representative scene and settings, plus Nsight or RGA shader analysis where the finding requires source/IL/ISA counters;
7. exact engine commit, global-shader-map/library hash, compiler backend and version, target, optimization/debug policy, GPU, driver, API, scene, camera, resolution, warm-up, capture frame, and run count;
8. editor adoption evidence: edit, changed-dependency selection, successful reload, failed replacement rollback, direct error navigation, one-job replay, and GPU-safe old-generation retirement;
9. a clean build/cook/run from documented commands and a recorded issue or minimal reproducer created by a second adopter;
10. before/after authored-string, registration, parameter-declaration, forwarding-pass, compile-job, unique-code, cooked-code/library-byte, cook-time, map/library-open, and first-materialization counts so the simplification claim is measurable;
11. a generated support matrix proving backend/target/stage/shader-type/feature/policy status and a consumer report distinguishing registered, cooked, runtime-valid, selected, and captured shaders, supported alternates, and mandatory failures;
12. runtime-generation and lazy-materialization lifetime evidence, including state-object/pipeline, SBT layout/index/update/memory, trace dispatch, alternate selection, mandatory-product failure, reload, and paired capture evidence, plus only compression/eviction metrics that actually exist.

The [performance diagnostics architecture](PerformanceDiagnostics.md) owns the shared measurement and capture infrastructure. This document owns the shader-specific identities and joins that make those captures traceable. Evidence records belong under the repository's evidence path selected by the acceptance workload; they must not be embedded here as claims that age with hardware, drivers, or compiler versions.

## Target Capability Requirements

These requirements describe the accepted unified migration. They intentionally exclude permutations, PSO precaching/prewarming, preload/streaming controls, and native driver caches; full RT execution is included through the focused composition and ownership model above.

### 1. Canonical Virtual Shader Sources

Create one immutable `ShaderSourceMountTable` with `/Engine`, `/Project`, and `/Plugin/<Name>` roots. Registrations and root includes use canonical virtual paths. Relative includes resolve from the including virtual file. Physical paths are read locations only and never portable identity. Unknown/overlapping mounts, absolute authored paths, mount escape, duplicate virtual paths, case-policy collisions, and late mounts fail deterministically.

### 2. One Lean Shader Class Contract

Replace the current mixed `F*`/`T*` Unreal-prefix vocabulary with Sparkle names such as `GlobalShader<Shader>` and `ShaderRef<Shader>`. A concrete shader class owns:

- nested `Parameters` when it is the direct graph-dispatch binding owner; compute and ray generation declare them inline for the normal case;
- optional compile eligibility, environment mutation, and compiled-result validation hooks only when that shader uses them;
- no empty parameter carrier or composition-wide payload/attribute declaration on a non-dispatch RT stage.

`IMPLEMENT_GLOBAL_SHADER(Class, VirtualSource, Entry, Stage)` remains the one implementation declaration. Sparkle does not need separate `DECLARE_GLOBAL_SHADER` or `SHADER_USE_PARAMETER_STRUCT` macros because its CRTP base and implementation declaration already know the concrete type and parameter owner. Raw source scanning never substitutes for typed registration.

### 3. Frozen Shader Catalog

`ShaderTypeDesc` records deterministic `ShaderTypeId`, readable type/stage, declaration location, virtual source/entry, optional direct-binding `Parameters` metadata/signature, capability requirements, and optional compile hooks. The ray-generation shader type additionally owns payload/attribute/recursion compile metadata; the focused RT composition stores typed membership and hit-group compatibility and derives that shared ABI. Collect declarations, validate collisions, sort, freeze, then query. Duplicate or late declarations report both source locations and fail; static initialization order is not catalog order.

### 4. One Parameter Schema from Graph Setup through Binding

For a one-shader pass, `Shader::Parameters` is simultaneously the graph dependency declaration, parameter storage, reflection contract, structural signature, and runtime binding input. `FrameGraphBuilder::AllocParameters<Shader>()` allocates that exact nested type. Inputs/outputs use typed graph resource wrappers, so access and lifetime derive from the fields the author fills.

A real multi-stage graphics operation may compose `VertexShader::Parameters`, `PixelShader::Parameters`, and graph-only attachments in one small envelope. A ray trace uses the selected ray-generation shader's `Parameters` as its global graph/binding schema; optional local data has one bounded hit-group/stage record schema and no empty or mirrored carrier. A shaderless copy/clear pass owns graph-only parameters. No envelope restates a shader-visible field. Delete count-only compatibility and report the first structural mismatch before pipeline creation.

### 5. Thin Typed Graph Dispatch

Provide Unreal-like focused entry points over existing owners:

- `Dispatch<Shader>(parameters, groupCount)` and async equivalent for compute;
- an optional leading `RenderPassLabel` only for a distinct graph instance;
- typed graphics draw helpers that take the actual vertex/pixel shader types, parameters, narrow `RasterPassRenderState`, and prepared draw collaborator/data; attachment and mesh/material facts remain with their owners and the complete descriptor is internal;
- `TraceRays<RayGenerationShader>(composition, parameters, dimensions)` for RT pipelines, with the same optional diagnostic-label rule;
- no package/program/layout/pipeline strings at the call site.

The helper resolves `ShaderRef<Shader>` from the active `GlobalShaderMap`, derives the default label from the shader type, declares resources from `Shader::Parameters`, asks `RenderPassRuntimeCache` for the generation-bound layout/pipeline, and records binding/dispatch later through `PassCommandContext`. Delete one-method pass classes, duplicate `*PassParameters`, `GetDefinition`, `GetParameterMetadata`, and forwarding Execute bodies where the shader type and graph helper express the entire operation. Keep focused pass collaborators only when they own real mesh/draw/feature behavior.

For graphics, `RasterPassRenderState` is the only pass-authored fixed-state surface and exposes granular semantic blend/depth-stencil choices plus a dynamic stencil-reference choice where consumed. Frame-graph attachments derive target/depth formats, counts, samples, load/store/clear, and depth/stencil access. Prepared mesh/material work derives vertex input, topology, fill/cull, streams, and draw arguments. Typed shader references derive stage code and layout identity. `RenderPassRuntimeCache` combines the pipeline-affecting facts into the complete immutable `GraphicsPipelineKey`, creates the complete neutral `GraphicsPipelineDesc`, and asks the paired backend to lower it. Dynamic viewport/scissor/blend-constant/stencil-reference values remain command state and do not enter that key. No old fact is repeated in another owner.

### 6. Reproducible Compile Jobs without Persistent Compiler Results

Create one immutable `ShaderCompileRequest` and `ShaderCompileJob` per `(ShaderTypeId, Target)`. `ShaderCompileInputHash` covers the virtual source closure, entry, stage, compiler-affecting parameters/environment, target, backend/tool provenance, and compile policy. It excludes pass labels, packages, pipelines, and presentation strings.

Replace package-shaped cook nodes with jobs on the existing `TaskExecutor`. Keep one out-of-process cooker, bounded compiler sessions/memory, deterministic ordering/result merge, cancellation settlement, and in-flight duplicate fan-out. Every selected input invokes the compiler; `ShaderCompileInputHash` is identity and provenance, never a persisted-result lookup key. There is no compiler-result store, cache directory, enable/disable flag, storage schema, cache browser, or cleanup operation.

### 7. Global Shader Map and Cooked Code Library

Cooking publishes one deterministic generation:

1. `GlobalShaderMap`: sorted `(ShaderTypeId, Target) -> ShaderCodeHash + parameter signature + required runtime metadata`.
2. `CookedShaderLibrary`: validated code records addressed by exact `ShaderCodeHash`.
3. development-only provenance joining code/input hashes to virtual dependencies, compiler invocation, symbols, reflection, and diagnostics.

There is no authored or generated `ShaderProgramDesc`, `ShaderProgramId`, `TShaderProgram`, or mandatory program manifest in the architecture. Graphics pipeline stage composition is the typed draw request. Ray tracing alone uses `RayTracingPipelineComposition` because a native pipeline must name exports and hit groups. Phase 4 atomically deletes `.sparkshader` package readers/writers/cache/path derivation and publishes only the raster/compute map/library representation; Phase 6 extends that same representation to RT stages while delivering its native and graph consumer in the same CL. No compatibility container or compiler-only replacement remains.

### 8. Typed Runtime Lookup and One Generation Lifetime

`ShaderRef<Shader>` resolves through the active `GlobalShaderMap`; it never derives a path or package from a source basename. `RenderPassRuntimeCache` remains the sole active/replacement/retired generation owner and stores the map/library plus materialized binding layouts/pipelines for that generation. Lazy materialization may occur during graph construction, never pass recording. Reload validates a complete replacement before activation, preserves the current generation on failure, and retires old state only after all captured submission tokens complete.

### 9. Dependency-Directed Apply Changed

Persist forward and reverse virtual-source dependencies. Editor submits `Apply Changed`; Application snapshots changed virtual paths and routes one typed request; ShaderCompiler selects affected shader types, cooks, and publishes; Renderer validates and activates one replacement. Missing/corrupt dependency metadata fails with explicit `Rebuild All` guidance rather than silently broadening the normal operation. The Editor never scans artifact directories or mutates runtime state.

### 10. Bounded Diagnostics and Traceability

Failures identify shader type, virtual source, entry, stage, target, backend/tool, compile input hash, and declaration location. One bounded replay bundle contains the canonical request, dependency closure, preprocessed source where available, arguments, compiler diagnostics, and reflection mismatch. Successful debug artifacts remain opt-in.

One read-only trace joins shader type or graph/capture label to catalog declaration, virtual dependencies, compile job/input hash, compiler result, shader-map entry, code hash/record, generation, binding layout, pipeline key, graph consumers, and external symbol/capture paths. It reads authoritative metadata and creates no registry or permanent log stream.

### 11. Honest Backend and Ray-Tracing Boundaries

Report language/backend/target/stage/feature support as supported, unsupported, compiler-only, or runtime-validated. No backend silently ignores compile policy. Inline ray-query compute shaders use the normal compute class/map/dispatch route. Acceleration-structure storage and native descriptor representation are selected below Renderer shader/effect code: one semantic parameter and one shader code record cover classic and partitioned TLAS, and provider readiness is false unless the backend can bind that parameter through its exact native descriptor contract. Phase 6 may publish ray-generation/miss/hit/intersection/callable declarations only in the same CL that consumes them through typed composition, native pipelines, SBTs, graph trace dispatch, generation lifetime, and paired conformance; there is no accepted compiler-only RT checkpoint.

### 12. Explicitly Deferred Follow-Ups

After this unified migration is accepted, a separate measured proposal may add typed shader permutations. It must preserve a zero-boilerplate one-variant default, extend the shader-map key to `(ShaderTypeId, PermutationId, Target)`, keep permutation selection typed, and update compile-input identity, map publication, and diagnostics together. It does not reopen packages or program aliases.

PSO precaching/prewarming, preload/residency/streaming controls, Vulkan pipeline cache, D3D12 cached/library paths, material shaders, and vertex factories remain outside this plan. Persistent compiler-result storage is rejected rather than deferred. A future proposal for another listed capability requires current workload evidence and its own owner/AC; none is scaffolded here.

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
| Defer until measured | typed permutations; shader pipeline authoring types; PSO precaching/prewarming; preload/residency/streaming controls; native driver caches; material shaders; vertex factories; persistent local worker-process pool; complex plugin loading phases; library chunk/patch system; Vulkan shader objects/pipeline binaries; D3D12 partial programs; work graphs; mesh/task shaders |
| Require one complete future slice | RT state objects/pipelines, native identifiers/group handles, SBT build/indexing, trace-rays commands, graph integration, pipeline/generation lifetime, explicit alternate selection, mandatory-product failure, and paired execution evidence |

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
- an active shader-map record whose descriptor-indexing or semantic acceleration-structure requirement is not supported end to end by the active provider's graph/RHI/native descriptor contract
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

Rejected because Unreal's materials, vertex factories, cook-worker scale, plugin loading phases, platform count, and content scale solve workloads Sparkle does not currently have. Sparkle should copy responsibility boundaries, invariants, typed authoring, lifetime rules, and failure behavior while keeping the implementation proportional.

### Launch one compiler process per shader immediately

Rejected as a default because Sparkle already runs the cooker out of process and executes bounded stage tasks. A serializable job boundary is required now; a persistent worker pool is added only when compiler critical sections, crashes, leaks, or throughput measurements justify its process cost.


## Related Delivery Records

- [Shader System Delivery Plan](../../Plans/CrossModule/ShaderSystem.md) owns phase order, change boundaries, validation sequencing, and completion gates.
- [Shader System Migration Baseline](../../Research/ShaderSystemMigrationBaseline.md) preserves the frozen Phase 0 inventory and deletion ledger.
- [Ray-Tracing Pipeline And Dual-Execution Architecture](../Modules/Engine/Renderer/RayTracingExecution.md) owns enduring dual-execution and shader-table semantics.
