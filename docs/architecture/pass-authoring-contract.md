# Pass Authoring Contract

Status: Stage 17 implementation contract, Stage 17A boilerplate-reduction target, Stage 17B pass-authoring friction budget target
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This document explains how renderer passes are authored today, why the current path is too ceremonial, and what the target rule is for the review-ready refactor.

Hard gate from the execution plan: adding an ordinary renderer shader pass must not require editing `Engine/RHI`.

Primary code references:

- [Passes](../../Engine/Renderer/Private/Passes)
- [ShaderPass.h](../../Engine/Renderer/Private/Passes/ShaderPass.h)
- [PassUtilities.h](../../Engine/Renderer/Private/Passes/PassUtilities.h)
- [FrameGraph.h](../../Engine/Renderer/Private/FrameGraph/FrameGraph.h)
- [RenderPassDefinition.h](../../Engine/Renderer/Private/Passes/RenderPassDefinition.h)
- [RenderPassDefinitionRuntime.h](../../Engine/Renderer/Private/Pipeline/RenderPassDefinitionRuntime.h)
- [RendererShaderPackages.h](../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h)
- [Renderer shader registrations](../../Engine/Renderer/ShaderRegistrations)
- [ShaderCompiler](../../Tools/Shaders/ShaderCompiler)
- [Target folder architecture](after/repository-target-folder-architecture.md)
- [Artifact validation matrix](artifact-validation-matrix.md)

Reference basis:

- Falcor documents render passes and render graphs as the normal way to prototype rendering techniques: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/getting-started.md
- Donut describes reusable rendering passes in its render module and separates shader building from the main framework: https://github.com/NVIDIA-RTX/Donut
- AMD Render Pipeline Shaders treats render-graph structure as authored data instead of hand-written backend plumbing: https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders
- Diligent Samples include render-state packager workflows that make pipeline state an authored/packaged artifact: https://github.com/DiligentGraphics/DiligentSamples
- NVIDIA Donut-Samples threaded rendering shows independent command-list recording by view/face: https://github.com/NVIDIA-RTX/Donut-Samples/tree/main/examples/threaded_rendering
- Repository threading readiness: [after/repository-threading-readiness.md](after/repository-threading-readiness.md)

## Contract Summary

A renderer pass owns:

- Pass name.
- Pass parameter struct.
- Shader package declaration.
- Graph resource declaration.
- Execute function.
- Pass-specific binding overrides.
- Pass-specific diagnostics.

A renderer pass must not own:

- RHI public interface changes for ordinary resources/pipelines.
- D3D12/Vulkan backend code.
- Global shader package infrastructure.
- Backend-native descriptor or PSO construction.

## Current Authoring Flow After Stage 17

```mermaid
flowchart TD
    PassHeader[Create pass header and parameter struct]
    Definition[Add RenderPassDefinition in pass cpp]
    Packages[Use RendererShaderPackages identity]
    Frame[Wire pass into Frame/* composition]
    ShaderReg[Add renderer-owned shader registration]
    Cook[Run ShaderCompiler cook]
    Runtime[PipelineStateManager lookup by pass definition]
    Execute[FrameGraph executes pass]

    PassHeader --> Definition
    Definition --> Packages
    Definition --> Frame
    Packages --> ShaderReg
    ShaderReg --> Cook
    Definition --> Runtime
    Cook --> Runtime
    Runtime --> Execute
```

This flow removes the Stage 16/17 central traits edit. A regular renderer pass can still require edits in:

- [Engine/Renderer/Private/Passes](../../Engine/Renderer/Private/Passes)
- [Engine/Renderer/Private/Frame](../../Engine/Renderer/Private/Frame)
- [Engine/Renderer/ShaderRegistrations](../../Engine/Renderer/ShaderRegistrations)

The renderer-owned shader registration location keeps pass package identity above RHI. Package and binding layout names are shared through [RendererShaderPackages.h](../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h) instead of duplicated per pass. Stage 17A removed the local shader class constants and moved package/path/entry/stage metadata into one source metadata declaration. Stage 17B extends that target to the whole pass-add workflow so pass authoring is measured from missing shader source to a pass executing in a frame.

Stage 31 records the package compatibility evidence in [artifact-validation-matrix.md](artifact-validation-matrix.md): `ShaderCompiler.exe list-shaders --validate` reports `17` typed shader registrations and `10` valid renderer packages, and targeted Sky cook/inspection proves package, reflection, and pipeline layout data are inspectable before runtime smoke.

## Target Authoring Flow

```mermaid
flowchart TD
    Definition[Renderer pass definition]
    ShaderSource[Shader source / package declaration]
    Cook[ShaderCompiler cook and verify]
    Runtime[Pipeline runtime lookup by explicit key]
    FrameGraph[FrameGraph pass declaration]
    Execute[Execute pass]

    Definition --> FrameGraph
    Definition --> Runtime
    ShaderSource --> Cook
    Cook --> Runtime
    FrameGraph --> Execute
    Runtime --> Execute
```

Target rule:

- A normal pass addition touches Renderer pass/frame code and shader/tooling package definitions only.
- No D3D12/Vulkan files.
- No `Engine/RHI/Private/Shaders` renderer-specific registration.
- No broad RHI public interface changes unless the pass needs a genuinely new GPU/API concept.
- Target folders are `Engine/Renderer/Private/Passes`, `Engine/Renderer/Private/PassCatalog`, `Engine/Renderer/Shaders`, `Engine/Contracts/Shader`, and ShaderCompiler inspection/cook folders.

## Current Pass Pieces

| Piece | Current owner | Current examples | Target direction |
| --- | --- | --- | --- |
| Pass parameter struct | Renderer pass | `GBufferPass::Parameters`, `VisualizeBuffersPassParameters` | Keep in Renderer or neutral shader-authoring layer if shared. |
| Parameter metadata | Renderer public shader parameter helpers | [ShaderParameters](../../Engine/Renderer/Public/ShaderParameters) | Decide final consolidation in Stage 17. |
| Shader package declaration | Renderer pass definition plus renderer-owned shader registration constants | `RenderPassDefinition`, [RendererShaderPackages.h](../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h) | Keep renderer pass package identity above RHI with one shared package/layout id source. |
| Graph setup | Renderer frame graph/pass helpers | `AddRasterPass`, `AddComputePass`, `ShaderPass::Setup` | Keep Renderer-owned. |
| Execute callback | Renderer pass | `GBufferPass::Execute`, `SkyPass::Execute`, etc. | Keep Renderer-owned. |
| Runtime definition adapter | Renderer pipeline | [RenderPassDefinitionRuntime.h](../../Engine/Renderer/Private/Pipeline/RenderPassDefinitionRuntime.h) | Generic conversion from pass definition to runtime storage. |
| Binding | Renderer pipeline plus RHI binding commands | [PassBinder.cpp](../../Engine/Renderer/Private/Pipeline/PassBinder.cpp) | Keep renderer parameter binding above RHI; RHI binds generic handles/addresses/tables. |
| Backend PSO creation | RHI backend | D3D12/Vulkan pipeline files | Keep backend-owned. |

## Disposition Decisions

| Current piece | Disposition | Target decision |
| --- | --- | --- |
| Pass classes and parameter structs | Keep and refine | Preserve renderer ownership and improve diagnostics/package identity. |
| Renderer shader parameter helpers | Improve and extract | Keep if they remain renderer/pass-facing; move shared metadata to `ShaderContracts` only when tools need it. |
| Separate pass code plus registration files | Improve and extract | Stage 17 shares package/layout identity through `RendererShaderPackages`; later ShaderContracts work can turn this into manifest/catalog data. |
| `RenderPassPipelineTraits<TPass>` | Removed | Stage 17 deleted the central per-pass construction registry. |
| `ShaderCompiler` package discovery through renderer runtime | Replace or redesign | Compiler reads `ShaderContracts`, not runtime renderer implementation. |
| Backend-specific pass setup | Replace or redesign | Ordinary passes must not add D3D12/Vulkan code; use RHI descriptors/capabilities. |

## Folder Target

| Folder | Target role | Rejected use |
| --- | --- | --- |
| `Engine/Renderer/Private/Passes` | Pass execution code, resource declarations, parameter structs, diagnostics, and current pass definitions. | Backend-native code or unrelated global registries. |
| `Engine/Renderer/Private/PassCatalog` | Future shared manifest/catalog source for pass package identity, entry points, shader paths, expected stages, binding layout IDs, and pass capabilities. | A second registry next to `Engine/Renderer/ShaderRegistrations`. |
| `Engine/Renderer/Shaders` | Renderer pass shader source grouped by pass or feature. | Generic RHI fixtures or project/sample shader overrides. |
| `Engine/Contracts/Shader` | Schema shared with ShaderCompiler: package manifest, reflection records, binding layout identity, pass catalog records. | Renderer runtime execution or backend compiler implementation. |
| `Tools/Shaders/ShaderCompiler` | Compile, cook, verify, inspect pass packages from `ShaderContracts`. | Full renderer runtime dependency. |
| `Engine/RHI/Private/Shaders` | Generic shader package infrastructure only. | Renderer pass declarations or pass-specific uniform structs. |

## Pass Complexity Budget

| Pass authoring complexity | Earns its right when | Remove or redesign when |
| --- | --- | --- |
| Pass definition object | It names resources, shader package, pipeline kind, render state, feature requirements, diagnostics, and validation expectations. | It only wraps another function without reducing pass setup cost. |
| Pass-specific parameters | They are shader-visible or pass-owned data with reflection/diagnostic value. | They duplicate frame/global data or hide ownership. |
| Shared package identity entry | It is the single source for package and binding layout names consumed by ShaderCompiler registration and runtime pass definitions. | Package identity is duplicated in pass code and registration files. |
| Shader registration manifest/generator | It removes repeated class-local constants while preserving typed parameter metadata, paths, entry points, stages, feature flags, and diagnostics. | It is only a macro disguise that makes navigation or error reporting worse. |
| Runtime specialization | It is required for a genuinely unusual pass. | It exists only because central traits require every ordinary pass to add ceremony. |

## Current End-To-End Pass-Add Workflow

This is the current cost of adding an ordinary shader pass from no shader to a running frame. The list is intentionally explicit because pass authoring is a frequent renderer operation and complexity here compounds quickly.

| Touch point | Current file or area | Why it is touched today | Disposition |
| --- | --- | --- | --- |
| Shader source | Renderer shader source path referenced by registration, such as `Passes/Deferred/*.hlsl` or `Passes/Compute/*.hlsl`. | Provides shader code and entry point. | Keep as intentional author input. |
| Pass parameter shape | Pass header in [Engine/Renderer/Private/Passes](../../Engine/Renderer/Private/Passes). | Names shader-visible resources/constants and parameter metadata. | Keep when the pass has unique bindings; generate or default common full-screen/simple cases where safe. |
| Pass class and runtime alias | Pass header in [Engine/Renderer/Private/Passes](../../Engine/Renderer/Private/Passes). | Declares pass name, runtime type, parameter instance, `GetDefinition()`, `DeclareResources()`, and `Execute()`. | Reduce to intent and execution only; derive pass/runtime boilerplate where possible. |
| Parameter metadata build | Pass cpp in [Engine/Renderer/Private/Passes](../../Engine/Renderer/Private/Passes). | Builds and validates shader parameter layout. | Keep validation, but centralize repeated skeleton code. |
| Pass definition | Pass cpp using [RenderPassDefinition.h](../../Engine/Renderer/Private/Passes/RenderPassDefinition.h). | Names package, layout, pipeline kind, feature requirements, render state, and debug names. | Keep as pass intent; derive package/layout/debug names from one authoring record. |
| Resource declaration | Pass cpp `DeclareResources()` and frame graph helpers. | Converts frame resources into SRV/UAV/RTV/DSV/pass parameters. | Keep close to pass intent; allow generated defaults only for simple patterns. |
| Parameter update | Pass cpp `SetParameters()` or equivalent. | Copies per-frame/view/feature data into shader-visible parameter blocks. | Keep only data movement that is pass-specific. |
| Dispatch or draw | Pass cpp `Execute()`. | Records compute dispatch or draw through runtime helpers. | Shared helpers should own ordinary dispatch/draw boilerplate; pass code states dimensions, draw policy, or custom behavior. |
| Shader registration class | [Engine/Renderer/ShaderRegistrations](../../Engine/Renderer/ShaderRegistrations). | Registers shader name, package id, binding layout id, parameters, shader path, entry point, and stage. | Stage 17A removed per-class constants; Stage 17B removed the central C++ registration aggregator; Stage 29 added `ShaderContracts` catalog DTOs consumed by ShaderCompiler commands and cook planning. |
| Package identity constant | [RendererShaderPackages.h](../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h). | Shares package/layout names between registration and pass definition. | Replace with generated/catalog source if it becomes a second editable registry. |
| Former central registration call | Deleted `Engine/Renderer/ShaderRegistrations/RendererGlobalShaders.cpp`. | Previously forced static registration function references for ShaderCompiler/runtime bootstrap. | Completed in Stage 17B through CMake-owned renderer shader registration object inclusion. |
| Frame insertion function | [Engine/Renderer/Private/Frame](../../Engine/Renderer/Private/Frame). | Allocates parameters, declares resources, adds compute/raster pass, and captures runtime lookup. | Keep as intentional graph composition, but remove repeated boilerplate with typed helpers. |
| Frame composition edge | Higher-level frame composition file such as [Frame.cpp](../../Engine/Renderer/Private/Frame/Frame.cpp) or feature composition files. | Places the pass in ordering relative to other passes. | Keep as intentional architecture decision. |
| Shader compiler enumeration | [Tools/Shaders/ShaderCompiler](../../Tools/Shaders/ShaderCompiler). | Lists, validates, cooks, and inspects packages/reflection. | Must consume pass catalog/manifest without internal edits per ordinary pass. |
| Runtime validation | Launcher-shaped smoke and package validation. | Proves the pass cooks, loads, binds, records, and presents on target backends. | Keep as evidence, automate where practical. |

## Boilerplate Classification

| Current repetition | Why it is a problem | Target owner |
| --- | --- | --- |
| `kShaderName`, package id, and binding layout id in each shader registration class. | These values are package metadata, not pass logic; repeated strings create typo and drift risk. | Completed in Stage 17A for renderer registrations. |
| `RendererShaderPackages` plus pass definition package strings plus registration package strings. | Multiple editable identity surfaces can disagree. | One pass/shader catalog identity source. |
| Empty renderer `Register*Shaders()` functions plus central `RegisterRendererGlobalShaders()` calls. | Registration ordering and reachability are mechanical. | Completed in Stage 17B through CMake-owned renderer shader registration object inclusion. |
| Repeated metadata-builder/validation skeletons. | Validation is valuable, but the repeated wrapper is not. | Shared pass authoring/runtime helper. |
| Repeated debug-name strings derived from pass/package name. | The information is useful, the hand-written duplication is not. | Derived defaults with explicit override support. |
| Repeated full-screen compute dispatch math for ordinary viewport-sized passes. | It is common pass mechanics and easy to mistype. | Shared dispatch helper fed by pass definition/viewport extent. |

## Pass Authoring Friction Budget

Stage 17B owns this budget. A pass that exceeds it must name the extra complexity and why it earns its right to exist.

| Pass kind | Intended author edits | Forbidden ordinary edits |
| --- | --- | --- |
| Simple compute pass | Shader source, one pass intent record or pass definition, one frame insertion/composition edge. | RHI, D3D12, Vulkan, `PipelineStateManager`, ShaderCompiler internals, central shader registration list, duplicated package/layout constants. |
| Simple raster pass | Shader source(s), one pass intent record or pass definition with render state, one frame insertion/composition edge. | RHI/backend edits, central trait/runtime edits, duplicate shader/package/layout constants, hand-written package registration plumbing. |
| Special pass | Shader/source inputs, pass intent, frame insertion, plus a documented special-case adapter. | Silent expansion of shared common code or generic registries to serve one unusual pass. |

Target metrics:

- Simple compute pass: no more than three intentional author-owned touch points before validation.
- Simple raster pass: no more than four intentional author-owned touch points before validation.
- Zero ordinary-pass edits in RHI, backend folders, `PipelineStateManager`, ShaderCompiler internals, or central registration lists.
- All generated/mechanical outputs are deterministic and either checked in intentionally or regenerated by a documented local/CI command.

## Stage 17B Touch-Count Evidence

Stage 17B introduced [FrameGraphBuilder::AddComputeShaderPass](../../Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h) and `AddRasterShaderPass` as the first low-risk workflow reduction. This keeps frame composition explicit while moving repeated pass-name/runtime-lookup/pass-construction/execute plumbing behind the frame graph authoring boundary.

| Proof area | Before Stage 17A/17B | After Stage 17A/17B | Remaining friction |
| --- | --- | --- | --- |
| `ComputeClear` simple compute proof | Pass header/cpp, shader registration constants, package identity constant, central registration call, shader source path declaration, validation. No active frame insertion exists today. | Pass header/cpp, package-aware source metadata declaration, package identity constant, validation. Future frame insertion can use `AddComputeShaderPass<ComputeClearPass>()`. | Stage 29 added contract-based compiler enumeration; a later manifest/scaffolder stage is still needed to reach the three-touch budget. |
| `VisualizeBuffers` compute frame insertion | Frame composition allocated parameters, declared resources, repeated pass name, called `AddComputePass`, created a lambda, looked up runtime, constructed the pass, and called `Execute`. | Frame composition allocates parameters, declares resources, and calls `AddComputeShaderPass<VisualizeBuffersPass>(parameters)`. Renderer shader registration object files are linked by CMake, so no central registration function is edited. | Pass class, shader registration declaration, and package identity are still separate authoring surfaces; ShaderCompiler now consumes them through a deterministic `ShaderContracts` catalog. |
| `GBuffer` raster frame insertion | Frame composition allocated parameters, declared resources, repeated pass name, called `AddRasterPass`, created a lambda, looked up runtime, constructed the pass, and called `Execute`. | Frame composition allocates parameters, declares resources, and calls `AddRasterShaderPass<GBufferPass>(parameters)`. | Raster pass definition/render-state data is still hand-authored and should later move toward a pass catalog only if diagnostics stay strong. |

`ShaderCompiler.exe list-shaders --validate` now performs authoring-record validation instead of only counting registrations. It reports actionable shader, package, layout, source, entry, stage, and reason fields for empty names, empty package/layout/source/entry metadata, missing parameter descriptor builders, invalid stage/package-kind combinations, package binding-layout drift, package-kind drift, and duplicate stages inside a non-library package.

## Current Checklist

A current ordinary pass usually needs:

1. Add or update pass class in [Passes](../../Engine/Renderer/Private/Passes).
2. Define `PassName`.
3. Define `Parameters` and metadata through shader parameter helpers.
4. Add `GetDefinition()` with package identity, pipeline kind, render state, feature requirements, and diagnostics names.
5. Add frame graph setup/execute wiring in [Frame](../../Engine/Renderer/Private/Frame).
6. Add shader registration in [Engine/Renderer/ShaderRegistrations](../../Engine/Renderer/ShaderRegistrations), using [RendererShaderPackages.h](../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h). Stage 17A removed duplicated local constants and Stage 17B removed central registration-list edits; Stage 29 may replace this with manifest/generated registration records for ordinary shaders.
7. Cook/inspect shader package through [ShaderCompiler](../../Tools/Shaders/ShaderCompiler).
8. Validate pass in D3D12 and Vulkan smoke if shader-visible layout or resource states changed.

## Target Checklist

After Stage 17, a normal pass should need:

1. Add pass definition in Renderer.
2. Add shader package/source declaration in `ShaderContracts` pass catalog or renderer-owned shader authoring location that exports that catalog.
3. Add frame graph wiring where the pass belongs.
4. Cook/inspect package.
5. Run targeted validation.

It should not require:

- RHI public interface edits.
- RHI private shader registration edits.
- D3D12/Vulkan backend edits.
- A central per-pass traits specialization for ordinary compute/raster passes.
- Hand-written repeated shader/package/layout constants.
- A central renderer registration list edit; CMake owns registration object inclusion for runtime consumers.

## Ownership Rules

| Question | Owner |
| --- | --- |
| What resources does this pass read/write? | Renderer pass setup through frame graph. |
| What shader package does this pass use? | Renderer pass definition/package declaration. |
| What bindings must be present? | Renderer pass parameter metadata plus shader reflection validation. |
| How are descriptors/CBVs/UAVs bound to the API? | RHI command list and backend binding layout. |
| What fixed-function pipeline state is needed? | Renderer pass definition normalized into RHI pipeline desc. |
| How does D3D12/Vulkan create the native PSO? | RHI backend pipeline service. |
| How is the pass debugged? | Renderer diagnostics, frame graph diagnostics, RHI markers, shader package inspection. |

## Threading Readiness Contract

Passes should be authorable as future command-recording jobs without changing their ownership model.

| Pass part | Threading-ready rule |
| --- | --- |
| Pass definition | Immutable pass metadata: name, package id, resource access, pipeline intent, feature requirements, diagnostics label. |
| Setup | Declares all graph resource reads/writes and does not record commands or mutate global pass state. |
| Execute | Consumes `PassExecutionContext`, frame data, pipeline runtime, and graph resources; any mutation is local to the recording batch or explicitly owned runtime cache. |
| Parameters | Shader-visible data is copied into pass/frame parameter blocks, not read from live GameFramework objects. |
| Diagnostics | Errors include pass name, package id, frame/batch label when available, binding/resource name, and backend. |

Forbidden shortcuts:

- Do not use static mutable pass state for per-frame data.
- Do not let execute discover hidden resources that setup did not declare.
- Do not make pass execution depend on current UI/GameFramework mutable objects.
- Do not make ordinary passes submit command lists or wait on queues directly.

## Current Gaps

| Gap | Evidence | Owning stage |
| --- | --- | --- |
| Shader registration and pass definition remain in separate files. | [RendererShaderPackages.h](../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h) is now the shared identity source; `ShaderContracts` gives the compiler a deterministic catalog, but a future manifest/scaffolder is still needed to remove the remaining authoring split. | Stage 31 or later |
| Renderer shader registrations still require explicit package/path/entry/stage declarations. | Stage 17A removed local `kShaderName`, `kShaderPackageName`, and `kBindingLayoutId`; Stage 17B removed the central C++ aggregator; Stage 29 made compiler consumption contract-based, while source metadata declarations remain until a manifest/scaffolder earns its right to exist. | Stage 31 or later |
| Adding a pass still requires several intentional edits. | Current workflow touches pass header/cpp, shader registration, package identity, frame insertion, shader source, cook/list validation, and smoke evidence. The central renderer registration-list edit is gone. | Stage 29 |
| Adding a pass still requires understanding cook/runtime/PSO evidence. | Pass definition, shader registration, cook tooling, and binding validation are intentionally separate but documented. | Stage 17B, Stage 20, Stage 29 |

## Stage 4 Completion Packet

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 4 - Move Renderer Shader Registration Out Of RHI. Full build and executable package enumeration remain Stage 5 validation work. |
| Status | Fully completed for ownership movement. Reopen only if `Engine/RHI` regains renderer pass registrations, `Renderer/Private` includes, or ordinary renderer pass additions require RHI edits. |
| Target docs opened | `docs/architecture/pass-authoring-contract.md`, `docs/architecture/pipeline-runtime-contract.md`, `docs/architecture/tooling-pipeline-contract.md`, `docs/architecture/rhi-contract-map.md`, `docs/architecture/after/repository-target-folder-architecture.md`, `docs/architecture/after/repository-threading-readiness.md`, `docs/plans/rhi-renderer-review-ready-implementation-plan.md`. |
| Contract surfaces touched | Renderer-owned shader registration target, RHI generic shader authoring primitives, ShaderCompiler registration bootstrap, pass package metadata, and boundary-check evidence. |
| Ownership proof | `Engine/RHI/Private/Shaders/BuiltinGlobalShaders.cpp` registers only RHI generic hello/test shaders. Renderer pass registrations live under `Engine/Renderer/ShaderRegistrations`; Stage 17B deleted the central renderer registration function and makes the registration objects executable link inputs through CMake. |
| DirectLighting proof | `Engine/Renderer/ShaderRegistrations/DirectLightingShaders.cpp` owns the DirectLighting shader package and can include renderer-private `RayTracedShadowUniformData.h`; `Engine/RHI` has no `Renderer/Private` include hits. |
| ShaderCompiler handoff | `Tools/Shaders/ShaderCompiler/CMakeLists.txt` links `SparkleRendererShaderRegistrations`, `ShaderContracts`, and public RHI shader primitives, not the full renderer runtime. Shader registration static metadata is converted into a deterministic `ShaderContractCatalog`. |
| Package identity preserved | Static registration still declares the expected renderer package ids: `GBuffer`, `DirectLighting`, `IndirectLighting`, `LightingComposite`, `Sky`, `VisualizeBuffers`, and `ComputeClear`. |
| Refactor disposition | Keep and refine `SparkleRendererShaderRegistrations` as a narrow migration target. Stage 17B removed central registration calls; Stage 29 added `ShaderContracts` DTOs and catalog validation. A future manifest/scaffolder can remove the remaining hand-authored metadata split. |
| Complexity right to exist | The narrow registration target earns its complexity because it lets ShaderCompiler enumerate renderer packages without linking full renderer runtime behavior. `ShaderContracts` earns its complexity by making package catalog, verification failures, and shader job identity explicit. |
| Data transfer contract | Renderer pass metadata transfers through renderer-owned registration APIs and the `SparkleRendererShaderRegistrations` target. RHI receives only generic cooked shader package, reflection, binding layout, and runtime primitives. |
| Threading readiness handoff | Package registration becomes immutable catalog data before cook planning. Future parallel shader cook jobs consume package/job DTOs rather than live renderer runtime objects. |
| Validation | Static checks on 2026-06-13: `rg "Renderer/Private" Engine/RHI` returned no matches; `ArchitectureBoundaryCheck.cmake` passed with no new violations; RHI `RegisterBuiltinGlobalShaders()` contains no renderer pass calls; ShaderCompiler CMake links the narrow registration target. |

## Stage 17 Completion Packet

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 17 - Introduce Declarative Pass Definition And Migrate Passes. |
| Status | Fully completed for ordinary passes. |
| Definition model | [RenderPassDefinition.h](../../Engine/Renderer/Private/Passes/RenderPassDefinition.h) names pass, package declaration, shader package, pipeline kind, feature requirements, diagnostics debug names, and graphics render state. |
| Runtime adapter | [RenderPassDefinitionRuntime.h](../../Engine/Renderer/Private/Pipeline/RenderPassDefinitionRuntime.h) converts definitions to shader runtime storage and normalized RHI graphics/compute descriptors. |
| Migrated passes | `ComputeClear`, `GBuffer`, `DirectLighting`, `IndirectLighting`, `LightingComposite`, `Sky`, and `VisualizeBuffers` expose `GetDefinition()` and `PipelineRuntime`. |
| Removed path | `Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h` was deleted. No source references to `RenderPassPipelineTraits` or `DescribeShaderPackage` remain under `Engine/Renderer`. |
| Package identity | [RendererShaderPackages.h](../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h) is shared by pass definitions and renderer shader registrations. |
| Validation | `ShowcaseEditor`, `ShaderCompiler`, and `architecture_boundary_check` passed in `build/windows-vs2026-stage5`; `ShaderCompiler.exe list-shaders --validate` reported 17 valid typed registrations. |

## Acceptance Evidence

The pass authoring contract is accepted when:

- A proof pass can be added without touching `Engine/RHI`.
- Pass definition includes graph resource usage, shader package identity, pipeline state intent, diagnostics name, and validation expectations.
- ShaderCompiler can cook and inspect the pass package.
- Runtime errors mention pass name, package id, binding layout id, backend, and missing binding/resource.
- Old renderer-specific RHI private shader registrations are removed or moved.
