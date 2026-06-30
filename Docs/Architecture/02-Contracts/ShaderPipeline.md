# Shader Pipeline And ABI

## Purpose

This document explains SparkleEngine's shader pipeline from source registration to cooked package emission and runtime pipeline creation. The goal is to make the shader compiler, reflection, contract validation, cooked package ABI, and runtime loading path easy to review as a principal-level engine/tooling strength.

This is a source-backed architecture document. If a detail is not clearly proven by the reviewed source, it is marked `Needs source confirmation`.

## Non-Goals

- This document does not change shader compiler code.
- This document does not claim support for a target, profile, backend, or runtime path unless source confirms it.
- This document does not redefine renderer pass architecture.
- Future neural rendering notes in this document are readiness notes only, not implementation plans.

## End-To-End Path

The current source-backed shader path is:

1. renderer or engine code registers global shaders through `GlobalShaderRegistry`
2. shader contracts are built from those registrations
3. package layouts are derived from parameter structs and merged per package
4. the shader compiler selects backends and targets
5. source is preprocessed with include expansion
6. backend compilation produces bytecode plus reflection
7. contract and parameter-struct verification run during cook planning/execution
8. compiled stages are cached by a stable cache key
9. cooked shader packages are emitted to disk with binary records, reflection records, layout records, and metadata
10. runtime RHI loads and validates cooked packages
11. RHI binding layouts and pipeline states are created from the loaded package plus expected parameter layout

That is the mental model reviewers should use.

## Source Shader Layout

The engine uses registration-driven shader discovery rather than scanning directories blindly.

Current source-backed registration model:

- renderer shader registrations live in `Engine/Renderer/ShaderRegistrations/`
- registration code uses `IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(...)` and related macros from `GlobalShader.h`
- each registration supplies:
  - shader name
  - package name
  - binding layout id
  - source path
  - entry point
  - stage
  - package kind
  - package features
  - parameter-struct descriptor builder

Examples visible in source:

- `GBufferVS` and `GBufferPS` in `GBufferShaders.cpp`
- `DirectLightingCS` and `DirectLightingDeviceAddressCS` in `DirectLightingShaders.cpp`

Current source path examples:

- `"Passes/Deferred/GBufferVS.hlsl"`
- `"Passes/Deferred/GBufferPS.hlsl"`
- `"Passes/Deferred/DirectLighting.hlsl"`
- `"Passes/Deferred/DirectLightingDeviceAddress.hlsl"`

Contract rule:

- shader source becomes part of the engine only when it is registered through a typed shader registration path
- package identity and binding layout identity are explicit authoring concepts, not accidental file-name conventions

## Include Closure Model

SparkleEngine preprocesses shader source itself before backend compilation.

Current source-backed behavior:

- `ShaderSourcePreprocessor` expands includes recursively
- `ShaderIncludeResolver` resolves includes from:
  - the including file's directory
  - the primary include directory
  - additional include directories
- `#pragma once` is honored
- recursive include cycles are detected
- `#line` directives are emitted so diagnostics map back to original files

Current source-backed hashing behavior:

- `IncludeClosureHasher` walks the same include graph
- every resolved file contributes a per-file hash
- the canonical include closure is sorted by normalized path key before final hashing

Contract rule:

- include closure is part of shader identity and cache identity
- include resolution failure is a hard cook failure, not a warning

## DXC Usage

DXC is a first-class backend in the current source.

Current source-backed facts:

- `DxcShaderBackend` is registered as a backend
- it creates `IDxcCompiler3` and `IDxcUtils`
- it compiles preprocessed HLSL source
- it supports both DXIL and SPIR-V output
- it extracts DXIL reflection through `DxilReflectionExtractor`
- it extracts SPIR-V reflection through `SpirVReflectionExtractor`
- it can capture debug artifacts such as:
  - compile arguments
  - compiler output
  - disassembly
  - preprocessed source
  - DXIL PDB symbols when available

Important source-backed compile behavior:

- HLSL language version is set to `2021`
- strictness and `all resources bound` are enabled
- warnings can be treated as errors
- debug info and optimization flags are configurable
- SPIR-V output is enabled through DXC `-spirv`
- SPIR-V ray query and ray tracing extensions are enabled only when package feature flags require them

Contract rule:

- DXC is not just the D3D path; it is also a codegen path for SPIR-V in current source

## Slang Usage

Slang is also a first-class backend in the current source.

Current source-backed facts:

- `SlangShaderBackend` creates a global Slang session
- it supports DXIL and SPIR-V code generation
- it preprocesses the same source text before loading it into Slang
- it loads a module from source string
- it resolves and checks entry points by stage
- it composes and links a program before emitting bytecode
- reflection is extracted from `slang::ProgramLayout` through `SlangReflectionExtractor`

Debug artifact note:

- Slang debug artifact capture currently stores:
  - compile arguments
  - diagnostics
  - preprocessed source
- disassembly capture is currently a placeholder string rather than real disassembly

Contract rule:

- Slang is a real backend in the current toolchain, not a future placeholder
- shader architecture must remain backend-neutral enough for both DXC and Slang to consume the same contract surface

## Target Formats

Current source-confirmed target families are:

- DXIL
- SPIR-V

Current source-confirmed target enum values:

- `DxilSm60`
- `DxilSm61`
- `DxilSm62`
- `DxilSm63`
- `DxilSm64`
- `DxilSm65`
- `DxilSm66`
- `DxilSm67`
- `SpirV14`
- `SpirV15`
- `SpirV16`

Current source-confirmed binary formats:

- `CookedShaderBinaryFormat::Dxil`
- `CookedShaderBinaryFormat::SpirV`

Current source-confirmed default target:

- `DxilSm66`

Current source-confirmed profile naming:

- DXIL profiles use `sm_6_x`
- SPIR-V target profile names use `spirv_1_4`, `spirv_1_5`, `spirv_1_6`
- shader-stage target profiles are built as `vs_6_x`, `ps_6_x`, `cs_6_x`, or `lib_6_x` for ray tracing library packages

## Reflection Model

The compiler pipeline uses a normalized in-memory reflection model before serialization.

Current source-backed in-memory reflection data includes:

- resource bindings
- constant buffers
- constant-buffer members
- input elements
- push constant ranges
- specialization constants
- thread group size
- entry flags
- wave size

Current reflection extractors:

- `DxilReflectionExtractor`
- `SpirVReflectionExtractor`
- `SlangReflectionExtractor`

Current contract shape:

- all backends must translate their native reflection into the shared `ShaderReflection` structure
- package emission serializes that shared reflection into cooked package records
- runtime inspection reads those serialized records back without needing the original compiler backend

## Shader Contracts

The contract layer is built from shader registrations, not from compiled binaries alone.

Current source-backed contract structures include:

- `ShaderContractStage`
- `ShaderContractPackage`
- `ShaderContractCatalog`
- `ShaderContractVerificationFailure`
- `ShaderContractJobIdentity`

Current source-backed contract responsibilities:

- map registrations into package/grouped stage contracts
- compute binding layout identity
- carry package kind and package feature flags
- carry ray tracing export metadata where relevant
- capture parameter-struct descriptors for verification

Current source-backed validation responsibilities:

- unique shader names
- non-empty package id
- non-empty binding layout id
- non-empty source path
- non-empty entry point
- presence of parameter descriptor builder
- package kind consistency
- package binding layout consistency
- duplicate stage detection
- ray tracing library stage rules

Contract rule:

- source registration is the authoritative shader ABI declaration
- compiled reflection must agree with authored contracts rather than replacing them

## Shader Registration Model

Registrations come from `GlobalShaderRegistry` and related helpers in `GlobalShader.h`.

Current source-backed registration concepts:

- `ShaderRegistrationDesc`
- `RayTracingHitGroupRegistrationDesc`
- package id derived from package name or source path
- binding layout id derived from explicit value or package id
- typed auto-registration helpers for graphics/compute and ray tracing shaders

Current source-backed package naming:

- package ids can be explicitly grouped, for example renderer packages such as:
  - `ComputeClear`
  - `DirectLighting`
  - `DirectLightingDeviceAddress`
  - `GBuffer`
  - `IndirectLighting`
  - `LightingComposite`
  - `Sky`
  - `VisualizeBuffers`

Contract rule:

- package grouping is a deliberate API surface because it drives binding layout generation, cook packaging, cache identity, and runtime load

## Cooked Shader Package Layout

Cooked packages are serialized binary files with explicit headers and record arrays.

Current source-backed top-level file shape:

- `CookedShaderPackageHeader`
- binary records
- binding records
- pipeline layout records
- specialization input records
- reflection records
- resource binding records
- constant buffer records
- constant buffer member records
- input element records
- push constant range records
- specialization constant records
- ray tracing export records
- ray tracing hit group records
- ray tracing local parameter records
- string table
- binary blob

Current source-backed header metadata includes:

- magic/version
- declared stages
- shader model major/minor
- package kind
- package features
- record counts
- shader package key
- source identity hash
- binding layout hash
- ray tracing payload/attribute/max-recursion metadata

Current source-backed package kinds:

- `Graphics`
- `Compute`
- `RayTracingLibrary`

## Cache Key Model

SparkleEngine uses a stable `ShaderCacheKey` over authored and produced identity.

Current source-backed cache-key inputs:

- shader cache backend version string
- cooked shader package version
- shader cache schema version
- shader package key
- binding layout id
- binding layout hash
- stage prefix
- target profile
- source hash
- include closure hash
- compile options hash
- backend name
- backend version

Current source-backed hashing behavior:

- the canonical key string is hashed with FNV-1a 64
- zero hashes are remapped to the FNV offset basis

Contract rule:

- cache identity includes both authored ABI and backend/codegen identity
- changing backend version or layout hash is intentionally cache-invalidating

## Runtime Load And Pipeline Creation Relationship

Cooked packages are not the final pipeline object. They are validated inputs to pipeline creation.

Current source-backed runtime relationship:

1. `CookedShaderPackageCache` loads a package from disk
2. the package is validated against:
   - `ShaderPackageDefinition`
   - expected `PassParameterLayout`
   - required binary format
3. RHI uses `LoadedShaderPackage` plus `RenderBindingLayoutCompileDesc` to create a binding layout
4. graphics or compute pipeline state descriptors point at:
   - the binding layout
   - the loaded shader package
   - the expected shader stages

Important source-backed RHI seams:

- `RenderBindingLayoutCompileDesc`
- `GraphicsPipelineStateDesc`
- `ComputePipelineStateDesc`
- `RhiPipelineService::CreateBindingLayout(...)`
- `RhiPipelineService::CreateGraphicsPipelineState(...)`
- `RhiPipelineService::CreateComputePipelineState(...)`

Contract rule:

- shader package validation happens before backend pipeline state creation
- package/binding-layout mismatch is an ABI failure, not a renderer convenience issue

## Feature And Profile Capability Matrix

This matrix is limited to what the reviewed source confirms.

| Area | DXC backend | Slang backend | Notes |
| --- | --- | --- | --- |
| HLSL source compilation | Yes | Yes | both backends consume preprocessed shader source |
| DXIL codegen | Yes | Yes | source-confirmed |
| SPIR-V codegen | Yes | Yes | DXC uses `-spirv`; Slang uses SPIR-V target format |
| Shader model targets `sm_6_0` to `sm_6_7` | Yes | Yes for DXIL target path | profile naming/source-confirmed target enums exist |
| SPIR-V target profiles `1.4` / `1.5` / `1.6` | Yes | Yes | source-confirmed |
| DXIL reflection extraction | Yes | `Needs source confirmation` as a separate native extractor | Slang reflection uses Slang program layout, not DXIL reflection symbols |
| SPIR-V reflection extraction | Yes | `Needs source confirmation` as native SPIR-V reflection output path | Slang reflection also uses Slang program layout |
| Unified normalized reflection output | Yes | Yes | both populate shared `ShaderReflection` |
| Graphics packages | Yes | Yes | source-confirmed |
| Compute packages | Yes | Yes | source-confirmed |
| Ray tracing library package kind | Yes | `Needs source confirmation` for backend emission support | contract/runtime/package support exists broadly |
| DXIL ray tracing library backend capability | Yes | `Needs source confirmation` | DXC backend explicitly reports support |
| SPIR-V ray tracing library backend capability | `Needs source confirmation` | `Needs source confirmation` | no reviewed source proved active support bit for both backends |
| DXIL inline ray query capability bit | Yes | `Needs source confirmation` | DXC backend explicitly reports support |
| SPIR-V inline ray query capability bit | Yes | `Needs source confirmation` | DXC backend explicitly reports support |
| Debug artifact capture | Yes | Partial | Slang disassembly capture is placeholder text today |

## Ray Tracing Shader Expectations

The shader pipeline clearly supports ray tracing package metadata and runtime validation.

Current source-backed support includes:

- `CookedShaderPackageKind::RayTracingLibrary`
- ray tracing export kinds:
  - ray generation
  - miss
  - closest hit
  - any hit
  - intersection
  - callable
- ray tracing hit group registration support
- ray tracing export records and hit-group records in cooked packages
- ray tracing payload, attribute size, and max recursion depth metadata
- runtime validation through `LoadedShaderPackage::ValidateRayTracingLibraryMetadata(...)`

Current source-backed caveat:

- the reviewed registration files did not show concrete renderer ray tracing shader registrations using `IMPLEMENT_RAY_TRACING_SHADER(...)`

So the current contract statement is:

- ray tracing shader ABI support exists in the compiler/runtime pipeline
- concrete renderer-side active ray tracing shader registrations still need source confirmation in a deeper pass

## Future Neural Rendering Profile Gates

Readiness notes only:

- the toolchain already has dual-backend compilation and explicit target/profile naming
- Slang is integrated as a backend, which is a useful readiness point for future neural or advanced shader-language workflows
- package features and profile/target selection give Sparkle a place to add future capability gates without changing the basic package ABI

Current source-backed limitation:

- no reviewed source proved dedicated neural rendering profiles, tensor/cooperative-vector flags, or neural-specific shader package features

Contract rule:

- future neural rendering work should extend profile/capability gates and package metadata explicitly rather than piggybacking on unrelated flags

## Inspection And Debug Commands

Current source-backed `ShaderCompiler` CLI commands:

```text
ShaderCompiler cook [--package <package-id> | --shader-id <registered-shader-name>] [--no-cache] [--cache-dir <path>] [--target <name>] [--backend <name>] [--debug-artifacts <dir>] [--analysis <pass>] [--debug-info] [--disable-optimizations]
ShaderCompiler list-backends
ShaderCompiler list-targets
ShaderCompiler inspect-package <path>
ShaderCompiler list-shaders [--validate]
ShaderCompiler inspect-shader <shader-id>
```

Reviewer-useful commands:

```powershell
ShaderCompiler list-backends
ShaderCompiler list-targets
ShaderCompiler list-shaders --validate
ShaderCompiler inspect-shader GBufferVS
ShaderCompiler cook --package GBuffer --backend dxc --target DxilSm66
ShaderCompiler cook --package GBuffer --backend dxc --target SpirV16
ShaderCompiler inspect-package <path-to-cooked-package>
```

`Needs source confirmation`:

- exact default cooked package output locations and best reviewer-friendly package path examples for this repo setup

## New Shader Feature Checklist

Use this checklist when adding a new shader feature or package.

1. Define the authoring shape.
   - new shader
   - new package
   - new stage in an existing package
   - new ray tracing export or hit group

2. Register it through `GlobalShaderRegistry`.
   - provide package id or intentional package grouping
   - provide binding layout id
   - provide source path and entry point
   - provide stage or ray tracing library metadata

3. Define the parameter struct.
   - use the shader parameter struct macros
   - ensure the descriptor builder exists

4. Validate package layout impact.
   - confirm merged layout remains compatible for every stage in the package
   - confirm binding layout hash changes are intentional

5. Set package feature flags explicitly.
   - inline ray query
   - acceleration structure
   - acceleration structure device address
   - future flags only when truly needed

6. Confirm backend and target expectations.
   - DXIL
   - SPIR-V
   - shader model / SPIR-V target floor

7. Confirm reflection behavior.
   - resource bindings
   - constant buffers
   - push constants
   - thread group size
   - ray tracing metadata where applicable

8. Confirm cache identity impact.
   - source changes
   - include changes
   - compile-option changes
   - backend/version changes
   - layout changes

9. Confirm runtime consumption.
   - package definition
   - expected stages
   - expected binding layout
   - required binary format
   - pipeline creation path

10. Add inspection coverage.
   - `list-shaders --validate`
   - `inspect-shader`
   - `inspect-package`

## Known Gaps

- A compact reviewer diagram of cook-to-runtime package loading does not yet exist outside source and this document.
- Slang debug artifact disassembly capture is not implemented yet.
- The source clearly supports ray tracing package metadata, but active renderer ray tracing shader registrations were not confirmed in the reviewed registration files.
- The exact runtime package-output path examples for reviewer commands still need a source-confirmed canonical recipe.
- A dedicated neural-rendering profile/feature matrix does not yet exist in source.

## Source Anchors

Primary reviewed files for this contract:

- `Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md`
- `Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md`
- `Tools/Shaders/ShaderCompiler/CMakeLists.txt`
- `Tools/Shaders/ShaderContracts/Public/ShaderContractCatalog.h`
- `Tools/Shaders/ShaderCompiler/Source/main.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Cli/CommandRegistry.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Cli/ListBackendsCommand.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Cli/ListTargetsCommand.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Cli/ListShadersCommand.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Cli/InspectShaderCommand.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Cli/InspectPackageCommand.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Compiler/ShaderSourcePreprocessor.h`
- `Tools/Shaders/ShaderCompiler/Private/Compiler/ShaderSourcePreprocessor.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Compiler/ShaderIncludeResolver.h`
- `Tools/Shaders/ShaderCompiler/Private/Compiler/ShaderIncludeResolver.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Compiler/ShaderCompileProfile.h`
- `Tools/Shaders/ShaderCompiler/Private/Compiler/ShaderCompileProfile.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Backend/ShaderTarget.h`
- `Tools/Shaders/ShaderCompiler/Backends/Dxc/DxcShaderBackend.h`
- `Tools/Shaders/ShaderCompiler/Backends/Dxc/DxcShaderBackend.cpp`
- `Tools/Shaders/ShaderCompiler/Backends/Slang/SlangShaderBackend.h`
- `Tools/Shaders/ShaderCompiler/Backends/Slang/SlangShaderBackend.cpp`
- `Tools/Shaders/ShaderCompiler/Backends/Dxc/DxilReflectionExtractor.h`
- `Tools/Shaders/ShaderCompiler/Backends/Dxc/SpirVReflectionExtractor.h`
- `Tools/Shaders/ShaderCompiler/Backends/Slang/SlangReflectionExtractor.h`
- `Tools/Shaders/ShaderCompiler/Private/ShaderReflection.h`
- `Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractCatalogBuilder.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Contracts/ShaderContractValidator.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderPackageCooker.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Cooking/Cache/ShaderCacheKey.h`
- `Tools/Shaders/ShaderCompiler/Private/Cooking/Cache/ShaderCacheKey.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Cooking/Cache/IncludeClosureHasher.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Cooking/CookedShaderPackageEmitter.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Cooking/CookedPackageWriter.cpp`
- `Tools/Shaders/ShaderCompiler/Private/Inspection/CookedPackageInspection.cpp`
- `Engine/RHI/Public/Shaders/Authoring/GlobalShader.h`
- `Engine/RHI/Public/Shaders/CookedShaderPackage.h`
- `Engine/RHI/Public/Shaders/CookedShaderPackageUtils.h`
- `Engine/RHI/Public/Shaders/CookedShaderPackageCache.h`
- `Engine/RHI/Public/Pipeline/RhiPipelineStateDesc.h`
- `Engine/RHI/Public/Pipeline/RhiPipelineService.h`
- `Engine/RHI/Public/Shaders/ShaderPackageLayoutBuilder.h`
- `Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h`
- `Engine/Renderer/ShaderRegistrations/GBufferShaders.cpp`
- `Engine/Renderer/ShaderRegistrations/DirectLightingShaders.cpp`
