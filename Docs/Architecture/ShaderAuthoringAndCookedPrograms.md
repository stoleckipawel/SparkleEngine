# Shader Authoring and Cooked Program Architecture

Status: target proposal; not implementation evidence
Date: 2026-08-12
Scope: render-pass naming, shader-type registration, shader-program composition, cooking, lookup, validation, recooking, and hot reload

## Purpose

This document decides how SparkleEngine should identify render passes and shaders without requiring authors to maintain parallel strings such as `DirectLighting`, `RendererShaderPackages::DirectLighting`, binding-layout names, pipeline names, source basenames, and cooked-package names.

It refines the shader-specific conclusions from [External Renderer Repository Comparison](ExternalRendererComparison.md). That comparison remains the source-linked broad research document. This document owns the proposed local shader-authoring direction. Code and executable build configuration remain the authority for what is implemented today.

## Decision Summary

1. A semantic render-pass label is necessary for frame-graph diagnostics, GPU markers, errors, captures, and profiling.
2. `static constexpr const char* PassName = "DirectLighting";` is not necessary as a separately authored field on every pass. The default label should be generated from one typed pass declaration, with an explicit per-instance label only when the same pass type is scheduled for different work.
3. A shader filename must not be the render-pass identity. It is one compile input, not the meaning of a frame-graph operation.
4. Renderer authors should register typed shaders using source path, entry point, stage, and permutation/environment rules. They should compose those shader types into a typed shader program. They should not manually invent a cooked-package string.
5. Sparkle should keep its cooked shader artifact. The `.sparkshader` payload is a useful runtime and release boundary containing backend bytecode, reflection, layout data, feature requirements, and hashes. "Package" should describe that generated artifact, not an authoring chore.
6. The cooker should generate program membership, stage masks, layout identity, compile keys, artifact keys, and the runtime manifest. Every ambiguity or collision must be a deterministic error.

The short answer is therefore:

> Keep the concept of a pass name, remove the repeated `PassName` literal, do not replace it with the shader filename, remove manual renderer package names, and generate cooked artifacts from typed shader-program declarations.

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
  Renderer-owned authoring                         Generated/cooked data
  ========================                         =====================

  +-----------------------+                        +----------------------+
  | typed render pass     |-- default label ----->| frame graph / marker |
  | DirectLightingPass    |                        | diagnostics only     |
  +-----------+-----------+                        +----------------------+
              |
              | uses typed program
              v
  +-----------------------+       register         +----------------------+
  | DirectLightingProgram|------------------------>| shader catalog       |
  | stages:               |                         | validated, canonical |
  |   DirectLightingCS    |                         +----------+-----------+
  +-----------+-----------+                                    |
              |                                                | cook
              v                                                v
  +-----------------------+                         +----------------------+
  | shader type           |                         | .sparkshader         |
  | source + entry + stage|                         | DXIL / SPIR-V        |
  | parameters + variants |                         | reflection + layout  |
  +-----------------------+                         | hashes + provenance  |
                                                    +----------+-----------+
                                                               |
                                                               v
                                                    +----------------------+
                                                    | runtime shader map   |
                                                    | ProgramId -> artifact|
                                                    +----------+-----------+
                                                               |
                                                               v
                                                    +----------------------+
                                                    | pipeline cache       |
                                                    | program + RHI state  |
                                                    +----------------------+
```

The RHI owns generic shader descriptors, cooked schemas, validation primitives, and backend object creation. The Renderer owns its shader types, typed program compositions, pass types, and pass-to-program use. The shader compiler consumes the renderer's registration catalog without taking ownership of renderer runtime policy.

## Proposed Authoring Experience

The exact API spelling is an implementation choice. The intended amount of authored information is illustrated below:

```cpp
class DirectLightingCS final : public TGlobalShader<DirectLightingCS>
{
    // Typed parameters and compile policy.
};

IMPLEMENT_GLOBAL_SHADER(
    DirectLightingCS,
    "Passes/Deferred/DirectLighting.hlsl",
    "main",
    Compute);

using DirectLightingProgram = TShaderProgram<DirectLightingCS>;

SPARKLE_RENDER_PASS(DirectLightingPass, DirectLightingProgram);
```

The final form should have these properties:

- `SPARKLE_RENDER_PASS` or equivalent registers one pass type and generates its default diagnostic label from the type token. There is no hand-written `"DirectLighting"` beside `DirectLightingPass`.
- `TShaderProgram<...>` declares the complete stage set. A compute program has one compute shader; a raster program can have vertex and pixel shader types; a ray-tracing program owns its library exports and hit groups.
- The shader registration still states source path, entry point, and stage because those are independent compile inputs.
- The build exposes shader registration objects to both the cooker and runtime as it does today. Raw shader directory scanning does not replace registration: not every `.hlsl` or `.hlsli` file is an entry shader.
- Frame-graph APIs obtain the default label through `RenderPassTraits<TPass>` or an equivalent generated trait. An overload accepts an instance label when one pass type is dispatched several times.
- Binding-layout and pipeline debug names are generated from the pass/program label. They are never lookup authorities.
- Compiler RTTI strings, `typeid(T).name()`, and implementation-specific pretty-function text are not serialized. The registration macro or generated catalog emits deterministic text and IDs.

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

Sparkle does need cooked shader artifacts; it does not need authors to name each one manually.

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

The target model uses three keys:

1. `ProgramId`: a stable, typed logical ID generated from the program declaration and used by renderer code.
2. `CompileKey`: a content hash of all stage compile inputs, including normalized source path, transitive includes, source contents, entry point, stage, defines, specialization values, target, compiler backend, and compiler version.
3. `ArtifactId`: a content hash of the complete validated program output and layout. A generated manifest maps `ProgramId` to the current `ArtifactId` for each platform/target.

This allows source edits to invalidate compilation, source moves to be diagnosed correctly, identical stage binaries to be deduplicated if measurement justifies it, and logical program lookup to remain independent of a file basename.

One `.sparkshader` file per logical program is an acceptable first implementation. Combining files into a larger library is a release-layout optimization, not an authoring requirement, and should only be done after load-time, I/O, and file-count evidence shows a benefit.

## External Precedent and What Sparkle Adopts

The sources below are precedents, not local implementation authority. Repository links are pinned to the reviewed revisions where possible.

### Unreal Engine as the Core Model

Epic's global-shader documentation registers a C++ shader type against a source file, entry point, and shader stage. Its example deliberately registers vertex and pixel shader types from the same source file with different entry points. Shader instances are then retrieved by shader type from the global shader map. Unreal's shader development documentation also states that compile-cache keys hash all compile inputs, including shader source, and that cooked global shaders are stored in a separate global shader file.

Unreal's Render Dependency Graph separately accepts an `FRDGEventName` when adding a pass, and Epic documents that this pass name is for debugging and profiling. Therefore Unreal does not treat a shader filename as the universal identity for shader type, graph pass, cache entry, and cooked storage.

Adopt:

- typed shader registration
- explicit source, entry point, and stage
- typed shader lookup
- compile keys derived from the full input closure
- cooked global shader storage generated by the build/cook pipeline
- render-graph event labels kept separate from shader identity

Do not copy yet:

- Unreal's full material, vertex-factory, and permutation universe
- Unreal's engine-scale macro and module complexity where Sparkle has no corresponding workload

Sources:

- [Epic: Adding Global Shaders](https://dev.epicgames.com/documentation/en-us/unreal-engine/adding-global-shaders-to-unreal-engine)
- [Epic: Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [Epic: Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Epic: `FRDGBuilder::AddPass`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RenderCore/FRDGBuilder/AddPass)

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

The 2026-08-12 source review found:

- 28 pass classes declare `static constexpr const char* PassName`.
- `RendererShaderPackages.h` declares 28 package strings.
- Renderer shader-registration sources make 29 calls to `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE` and no calls to the package-inferred `IMPLEMENT_GLOBAL_SHADER` path.
- The three names normally repeat: pass name, package constant, and generated binding/pipeline debug names.
- Most current programs contain one compute stage. `GBuffer` demonstrates the legitimate multi-file, multi-stage case with a vertex shader and a pixel shader sharing one cooked program.
- `BuildShaderPackageIdFromSourcePath` derives only the filename stem, so same-basename sources in different directories collide and a source rename changes the fallback package ID.
- `GlobalShaderRegistry::Register` silently ignores a repeated shader name instead of reporting a conflicting declaration.
- The contract validator catches empty IDs, duplicate shader names, package layout/kind mismatch, and duplicate stages, but the registry's early duplicate suppression can prevent the validator from seeing the conflict.
- `PassBinder` accepts either the same parameter-layout instance or merely the same parameter count. Equal counts do not prove equal binding order, kinds, names, visibility, or sizes.
- changed-source monitoring detects that some shader file changed, then invokes a full `cook`; it does not yet use an include dependency graph to select affected programs.
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
- [`CookedShaderPackage.h`](../../Engine/RHI/Public/Shaders/CookedShaderPackage.h)
- [`CookedPackageWriter.cpp`](../../Tools/Shaders/ShaderCompiler/Private/Cooking/CookedPackageWriter.cpp)
- [`CookedShaderPackageCache.cpp`](../../Engine/RHI/Private/Shaders/CookedShaderPackageCache.cpp)
- [`PassBinder.cpp`](../../Engine/Renderer/Private/Pipeline/PassBinder.cpp)
- [`ShaderSourceChangeTracker.cpp`](../../Engine/Application/Private/ShaderRecook/ShaderSourceChangeTracker.cpp)
- [`ShaderCompilerProcess.cpp`](../../Engine/Application/Private/ShaderRecook/ShaderCompilerProcess.cpp)
- [`ValidateShaderCompilerCli.cmake`](../../Tools/Shaders/ShaderCompiler/ValidateShaderCompilerCli.cmake)

## Changes to Make

### 1. Generate Pass Labels from Typed Pass Declarations

- Add one pass-registration trait or macro that maps a pass type to its parameter type, pipeline kind, and typed shader program.
- Generate the default diagnostic label from the declared pass type token.
- Change `FrameGraphBuilder`, parameter metadata, validation, and dispatch diagnostics to read the trait.
- Preserve the explicit-name overload for genuinely dynamic pass instances.
- Remove the 28 per-class `PassName` literals in the same change.

### 2. Replace Manual Package Strings with Typed Programs

- Introduce a small `TShaderProgram<Stages...>` or equivalent program descriptor.
- Make stage membership the source of expected-stage masks and package kind.
- Make typed parameters/reflection the source of binding-layout identity.
- Update compute, raster, and ray-tracing definitions to reference the typed program.
- Remove `RendererShaderPackages.h`, `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE`, and repeated `PackageId` fields in the same migration.
- Keep `IMPLEMENT_GLOBAL_SHADER` or its successor explicit about source, entry, and stage.

### 3. Make Generated IDs and Manifests Deterministic

- Normalize virtual source paths once, with one case policy.
- Generate stable program IDs from registered program declarations, not source basenames.
- Hash the complete compile-input closure and compiler provenance.
- Emit a deterministic manifest mapping logical programs to target artifacts.
- Detect canonical-ID, source/entry/stage, program-stage, and output-path collisions before compilation.
- Treat repeated identical declarations as a build error unless deliberate aliasing is represented by an explicit, typed alias feature.

### 4. Strengthen Contract Validation

- Make duplicate shader registration fail with both declaration locations.
- Validate full binding-layout structure or its canonical hash; remove parameter-count-only acceptance.
- Verify declared parameter metadata against reflection for every stage and backend target.
- Validate all registered programs on supported D3D12/DXIL and Vulkan/SPIR-V targets in CI, not only one representative artifact.
- Retain transactional hot reload: validate a complete new manifest generation, then publish it atomically; keep the previous generation on any failure.

### 5. Make Recooking Dependency-Aware

- Record normalized transitive includes during preprocessing.
- Maintain source-to-shader and shader-to-program reverse dependencies.
- Recook only programs affected by a changed source/include, plus any program whose compile environment changed.
- Fall back to a full catalog cook when dependency metadata is absent or incompatible; report the reason.

### 6. Keep Physical Packaging an Internal Optimization

- Start with one cooked artifact per typed program because it matches the current loader and hot-reload unit.
- Measure startup I/O, file-open count, artifact size, cache reuse, and replacement cost.
- Only then choose between per-program files, a global shader library, or grouped release libraries.
- Never expose that physical grouping as repetitive renderer pass authoring.

## Error Policy

No architecture can promise "error free." This design should make errors difficult to express and deterministic to detect.

The cooker or startup validation must fail on:

- duplicate shader type IDs
- duplicate program IDs
- two stages of the same kind in a non-library program
- an invalid stage set for the program kind
- missing source, entry point, parameter metadata, or backend output
- source-path canonicalization collisions
- inconsistent reflected layouts across stages or backends
- a manifest entry whose artifact hash, source hash, layout hash, feature set, or declared stages do not match
- a pass requesting a program whose parameter contract is structurally incompatible

Errors must name the logical program, shader type, normalized source, entry point, stage, target, and both conflicting declaration locations where relevant.

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

## Migration and Acceptance Gates

Implement this as one coherent ownership migration, with compiling intermediate commits if needed but no permanent compatibility subsystem.

1. Add typed pass traits and program descriptors with validation tests.
2. Migrate one compute program (`ComputeClear`), one raster program (`GBuffer`), and one ray-tracing program as vertical proofs.
3. Confirm DXIL and SPIR-V cooking, runtime lookup, markers, layout validation, and failed-hot-reload rollback for those proofs.
4. Migrate the remaining renderer programs mechanically.
5. Delete the replaced package namespace, explicit-package macro, per-pass name literals, and count-only binding fallback.
6. Add dependency-aware recooking and full-catalog CI gates.
7. Run `architecture_boundary_check`, relevant shader/compiler/runtime tests, both backend cooks, and `git diff --check`.

The migration is accepted only when:

- a pass author writes no package string, binding-layout debug string, pipeline debug string, or default `PassName` string
- `GBuffer` still cooks vertex and pixel stages into one logical program
- one file with several entry points and one program with stages from several files are both tested
- moving a shader source invalidates its compile key without silently renaming the render-pass label or logical program
- same-basename source paths cannot collide
- changed includes select every dependent program and no unrelated program when dependency data is valid
- all registered programs validate and cook for the supported targets
- an invalid replacement leaves the previous runtime generation active
- profiler and GPU-capture labels remain semantic and readable

## Final Position

Sparkle should follow Unreal's semantic center: C++ shader types registered with explicit source, entry point, stage, and compile policy; generated cooked global-shader data; and render-graph names used as diagnostic presentation. NVIDIA and AMD reinforce the same separation with filename-plus-entry shader loading, typed stage/pass selection, generated platform blobs, permutation keys, and independent GPU/pipeline labels.

The clean target is not "the filename is everything." It is "each identity has one job, authors state only irreducible facts, and tooling generates the rest."
