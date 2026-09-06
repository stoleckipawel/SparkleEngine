# Shader Compilation Capability Inventory

Status: capability snapshot; not a successful cook, runtime-load record, or release approval

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; current shader contracts, Renderer registrations, compiler executable/CMake, DXC and Slang backends, cook/publication code, CLI validation script, editor recook, and Renderer runtime loading inspected; evidence `S` only

Scope: authored-language and stage coverage, compiler targets/backends, typed registration, source dependency planning, reflection/ABI validation, cooked publication, diagnostics, recook, and runtime loading

Owners: `Tools/Shaders` for offline compilation/publication, `Engine/Assets/Shaders` for engine shader sources, `Engine/Renderer/ShaderRegistrations` for Renderer global-program registration, and `Engine/Renderer` for runtime materialization/consumption

Current system architecture: [Shader System](../../../CrossModule/ShaderSystem/README.md)

Delivery plan: [Shader System Plan](../../../../Plans/CrossModule/ShaderSystem.md)

Evidence plan and release disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

Traceability: capability rows use durable `SHD-<family>-NN` identities; their primary proof destinations are listed in the [ShaderCompiler capability-to-evidence map](../../../../Plans/CapabilityEvidence.md#shadercompiler-capability-to-evidence-map).

Deeper routes: [exact Renderer shader program catalog](../../Engine/Renderer/Features/ShaderPrograms.md), [cross-system graphics coverage](../../../CrossModule/GraphicsCoverageMatrix.md), and [shader authoring-to-runtime trace](../../../CrossModule/FeatureExecutionTraces.md#trace-7-shader-authoring-to-runtime-generation)

## Build And Delivery Shape

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `SHD-BUILD-01` | Offline compiler executable | Capability-gated | `ShaderCompiler` builds when `SPARKLE_ENABLE_SHADER_COMPILER` is enabled. It is a host tool, not a runtime in-process compiler. | `S` | Pending |
| `SHD-BUILD-02` | Shared shader contracts | Implemented path | `ShaderContracts` defines target, stage, feature, reflection, parameter-layout, map/library, dependency, and publication identities shared by registration, tool, and runtime consumers. | `S` | Pending |
| `SHD-BUILD-03` | Contract-only registrations | Implemented path | Renderer shader registrations compile into a separate object target that the host tool can link without linking the full runtime Renderer implementation. | `S` | Pending |
| `SHD-BUILD-04` | Runtime shader source dependency | Not found for intended Shipping path | Runtime opens cooked map/library products and materializes RHI programs/pipelines. Application shader-recook sources are excluded from the runtime target. This needs package inspection before it becomes a Shipping proof. | `S` | Pending |
| `SHD-BUILD-05` | Tool dependencies | Capability-gated | DXC and Slang are found from configured dependency/Vulkan SDK routes; required DLLs and Slang standard modules are staged for the tool. Clean-machine discovery is unproven. | `S` | Pending |

## Command-Line Surface

| Capability ID | Command | Exact behavior in the current source | Evidence | Release disposition |
| --- | --- | --- | --- | --- |
| `SHD-CLI-01` | `cook` | Compile and publish all registered shaders, one `--shader-id`, or the registrations affected by repeatable `--changed` virtual source paths. Full, single-ID, and changed-source selection are mutually exclusive. | `S` | Pending |
| `SHD-CLI-02` | `list-backends` | Print registered compiler backends and reported binary/ray capability flags. | `S` | Pending |
| `SHD-CLI-03` | `list-targets` | Print all target descriptors supported by the tool vocabulary. | `S` | Pending |
| `SHD-CLI-04` | `list-shaders` | Print global registrations; optional `--validate` runs registration/contract validation. | `S` | Pending |
| `SHD-CLI-05` | `inspect-shader` | Look up one shader ID in the cooked publication and report each runtime target's code hash, parameter signature, and compile-input hash. | `S` | Pending |

### `cook` options

| Capability ID | Option family | Current coverage |
| --- | --- | --- |
| `SHD-OPT-01` | Selection | All registrations by default; one `--shader-id`; or one or more `--changed` canonical virtual paths. |
| `SHD-OPT-02` | Targets | Repeatable `--target`; defaults to `DxilSm66` and `SpirV16`. |
| `SHD-OPT-03` | Backend | Automatic selection or explicit `--backend`. Automatic selection considers source extension and target support. |
| `SHD-OPT-04` | Parallelism | `--parallel-compiles` from 1 through 8; default 4; 1 is the serial comparison path. |
| `SHD-OPT-05` | Diagnostics | Debug artifacts and optional `cooked-shader-stats` analysis. |
| `SHD-OPT-06` | Compiler policy | Debug information, optimization disable, warnings-as-errors (default enabled), and debug stripping choices. |
| `SHD-OPT-07` | Cancellation | Cooperative cancellation is checked before publication; editor/process control supplies the private cancellation route. |

## Authored Languages, Compiler Backends, And Targets

| Capability ID | Backend | Authored source | DXIL targets | SPIR-V targets | Ray coverage | Important limit | Evidence |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `SHD-TGT-01` | DXC | `.hlsl` | Shader Model 6.0 through 6.7 | SPIR-V 1.4, 1.5, 1.6 | Backend reports DXIL and SPIR-V ray-tracing-library plus inline-ray-query support | Runtime still consumes only registered programs and exact cooked targets; compiler capability alone is not feature coverage. | `S` |
| `SHD-TGT-02` | Slang | `.slang` | Shader Model 6.0 through 6.7 target descriptors | SPIR-V 1.4, 1.5, 1.6 target descriptors | Backend capability object reports DXIL/SPIR-V only | The inspected stage mapping handles Vertex, Pixel, and Compute; do not claim Slang ray-stage production. | `S` |

The eleven target descriptors are `DxilSm60`, `DxilSm61`, `DxilSm62`, `DxilSm63`, `DxilSm64`, `DxilSm65`, `DxilSm66`, `DxilSm67`, `SpirV14`, `SpirV15`, and `SpirV16`.

The current runtime/backend pairing is D3D12 with `DxilSm66` and Vulkan with `SpirV16`. The other target descriptors are compiler vocabulary and explicit tool choices; they are not automatically part of the first-release support matrix.

## Registered Runtime Program Inventory

The current Renderer contract target contains 35 typed global shader registrations.

| Capability ID | Stage | Registered count | Current program family coverage |
| --- | --- | ---: | --- |
| `SHD-CAT-01` | Compute | 25 | Clears, depth, sky/motion, lighting reservoirs/composite, reference/path/ReSTIR work, exposure, visualization, upscale, tone mapping, and output encoding. |
| `SHD-CAT-02` | Vertex | 1 | Raster GBuffer vertex program. |
| `SHD-CAT-03` | Pixel | 1 | Raster GBuffer pixel program. |
| `SHD-CAT-04` | Ray generation | 2 | Ray GBuffer and direct-shadow pipeline entry points. |
| `SHD-CAT-05` | Miss | 2 | Ray GBuffer and direct-shadow miss programs. |
| `SHD-CAT-06` | Closest hit | 2 | Ray GBuffer and direct-shadow triangle-hit programs. |
| `SHD-CAT-07` | Any hit | 2 | Alpha-tested ray GBuffer and direct-shadow rejection programs. |
| `SHD-CAT-08` | Geometry | 0 | Stage vocabulary only. |
| `SHD-CAT-09` | Hull | 0 | Stage vocabulary only. |
| `SHD-CAT-10` | Domain | 0 | Stage vocabulary only. |
| `SHD-CAT-11` | Intersection | 0 | RHI/composition vocabulary exists; no registered procedural intersection program. |
| `SHD-CAT-12` | Callable | 0 | RHI/composition vocabulary exists; no registered callable program. |

Counts are a dated inventory, not an architectural target. Recount from `Engine/Renderer/ShaderRegistrations` whenever registrations change.

## Registration And Contract Validation

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `SHD-REG-01` | Typed global registration | Implemented path | C++ shader class identity maps to canonical virtual source, entry point, stage, feature requirements, ray metadata, and parameter structure. | `S` | Pending |
| `SHD-REG-02` | Registry validation | Implemented path | Empty/duplicate names and IDs, missing source/entry/stage data, invalid parameter ownership, and inconsistent ray metadata are rejected. | `S` | Pending |
| `SHD-REG-03` | Parameter-structure contract | Implemented path | Non-ray programs and ray-generation programs require the appropriate typed parameter structure; non-ray-generation library stages cannot pretend to own root parameters. | `S` | Pending |
| `SHD-REG-04` | Ray contract | Implemented path | Payload size, attribute size, recursion, local-record bytes, and local-signature pairing are validated against registration semantics. | `S` | Pending |
| `SHD-REG-05` | Runtime registration closure | Implemented path | Renderer loading verifies that required registrations/targets exist and that metadata, parameter signatures, and pipeline compositions agree before generation publication. | `S` | Pending |

## Source Discovery And Compile Planning

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `SHD-PLAN-01` | Virtual shader roots | Implemented path | Canonical `/Engine` and `/Project` mount roots map virtual paths to owned shader-source trees. Traversal outside a mount is rejected. | `S` | Pending |
| `SHD-PLAN-02` | Include preprocessing | Implemented path | Source preprocessing resolves include closure and records canonical dependencies for compile identity and changed-source selection. | `S` | Pending |
| `SHD-PLAN-03` | Immutable jobs | Implemented path | Registration, target, backend choice, source/include closure, compile policy, binding remaps, and hashes form an immutable compile request/result boundary. | `S` | Pending |
| `SHD-PLAN-04` | Changed-source planning | Implemented path | Reverse dependency information maps changed canonical virtual paths to affected registered shaders. Unaffected registered entries are preserved; removed registrations are not retained. | `S` | Pending |
| `SHD-PLAN-05` | In-operation deduplication | Implemented path | Identical compile requests within one cook share the result rather than launch duplicate compiler sessions. | `S` | Pending |
| `SHD-PLAN-06` | Persistent result cache | Not found | No persistent compile-result cache across independent tool invocations was found. Cooked content identity still deduplicates publication bytes. | `S` | Pending |
| `SHD-PLAN-07` | Parallel compilation | Implemented path | Bounded worker execution uses the task system; results integrate by stable job index for deterministic publication. | `S` | Pending |

## Compilation, Reflection, And ABI Checks

| Capability ID | Capability | DXC | Slang | Evidence |
| --- | --- | --- | --- | --- |
| `SHD-ABI-01` | DXIL generation | Yes | Yes | `S` |
| `SHD-ABI-02` | SPIR-V generation | Yes | Yes | `S` |
| `SHD-ABI-03` | DXIL reflection | Native DXC reflection extraction | Compiled-result reflection route | `S` |
| `SHD-ABI-04` | SPIR-V reflection | SPIRV-Reflect extraction after optional binding normalization | Compiled-result reflection route | `S` |
| `SHD-ABI-05` | Resource reflection | Descriptor resources, binding/set/count/dimension and access metadata | Same cooked reflection contract where emitted | `S` |
| `SHD-ABI-06` | Constant data | Constant buffers, members, offsets/sizes/types and push-constant data | Same cooked reflection contract where emitted | `S` |
| `SHD-ABI-07` | Stage inputs | Vertex/input semantic and numeric shape metadata where applicable | Same cooked reflection contract where emitted | `S` |
| `SHD-ABI-08` | Specialization constants | Cooked reflection representation | Cooked reflection representation | `S` |
| `SHD-ABI-09` | C++ parameter ABI validation | Reflected resources/constants are checked against typed C++ parameter metadata, including ray-library registrations | Same shared validator for supported stages | `S` |
| `SHD-ABI-10` | Vulkan descriptor normalization | SPIR-V bindings are normalized to the engine layout contract; descriptor-indexing extension arguments are emitted where needed | No equivalent DXC normalizer path claimed | `S` |

This validates structural agreement in source/tool code. Only an executed cook and runtime pipeline creation can prove the actual compiler binaries and drivers accept the current inputs.

## Cooked Products And Publication

| Capability ID | Product/capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `SHD-PUB-01` | `GlobalShaderMap.smap` | Implemented path | Maps shader identity plus target to content hash, entry/stage/features, parameter signature, ray metadata, reflection/bindings, dependency/provenance, and shared publication identity. | `S` | Pending |
| `SHD-PUB-02` | `CookedShaderLibrary.slib` | Implemented path | Content-addressed storage holds unique compiled code blobs referenced by the map. | `S` | Pending |
| `SHD-PUB-03` | Shared publication hash | Implemented path | Map and library carry one publication identity and are rejected if they do not match. | `S` | Pending |
| `SHD-PUB-04` | Staged validation | Implemented path | Temporary products are opened and validated together before final publication. | `S` | Pending |
| `SHD-PUB-05` | Atomic file-set publish | Implemented path | Map, library, dependency manifest, and recook signal use a file-set publication route; failure cleanup preserves the prior valid generation. | `S` | Pending |
| `SHD-PUB-06` | Deterministic identity | Implemented path | Compile-input and content hashes, stable job integration, and content-addressed code support deterministic comparison. Actual repeat-cook equality is unrun in this snapshot. | `S` | Pending |
| `SHD-PUB-07` | Incremental preservation | Implemented path | A partial cook merges newly compiled affected entries with still-registered unaffected entries, while dropping registrations that no longer exist. | `S` | Pending |

## Diagnostics And Developer Workflow

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `SHD-DIAG-01` | DXC debug artifacts | Implemented path | Compile arguments, diagnostics, disassembly, preprocessed source, and available program-database output. | `S` | Pending |
| `SHD-DIAG-02` | Slang debug artifacts | Partial | Compile arguments, diagnostics, and source artifacts are written; equivalent disassembly coverage was not found. | `S` | Pending |
| `SHD-DIAG-03` | Cook analysis | Implemented path | Optional `cooked-shader-stats` output summarizes cooked products. | `S` | Pending |
| `SHD-DIAG-04` | Cooked inspection | Implemented path | `inspect-shader` exposes target/hash/signature/compile-input identity for one registered shader. | `S` | Pending |
| `SHD-DIAG-05` | Out-of-process editor recook | Capability-gated | Editor/application tooling launches `ShaderCompiler` for all, selected-ID, or changed-source recook; it tracks publication freshness, cancellation, completion, and then asks Renderer to reload. | `S` | Pending |
| `SHD-DIAG-06` | Used Shaders UI | Implemented path | Editor surface exposes recook-all and recook-selected workflows for observed shader use. | `S` | Pending |
| `SHD-DIAG-07` | Runtime reload | Implemented path | Renderer opens map/library, validates complete target and parameter/pipeline closure, materializes the replacement generation, swaps only after success, and retires the prior generation after GPU submissions. | `S` | Pending |
| `SHD-DIAG-08` | Shipping tool erasure | Partial | Recook implementation is excluded from the runtime Application target by CMake shape, but only a Shipping package manifest/binary/import inspection can prove compiler/source/debug routes are absent from shipped bytes. | `S` | Pending |

## Existing Executable Validation Route

`ShaderCompilerCliValidation` is an existing CMake target/script. Its current assertions cover:

- backend, target, and validated-registration listing;
- a full `DxilSm66` plus `SpirV16` cook with cooked-shader statistics;
- inspection of a representative shader in both targets;
- changed-source incremental cooking and deterministic preservation;
- repeat explicit-ID cooking with deterministic hashes;
- cancellation without corrupting the prior publication;
- traversal and missing-shader failure without corrupting the prior publication.

This describes what the validation target is designed to exercise. It was **not run** during this inventory and therefore supplies no current `B` or `R` mark.

## Explicit Non-Claims And Shipping Risks

- No compiler target was built, no DXC/Slang process was launched, and no cooked publication was created or reloaded for this snapshot.
- Eleven code-generation targets do not mean eleven supported runtime configurations. Current runtime selection is `DxilSm66` for D3D12 and `SpirV16` for Vulkan.
- DXC reports ray-library/inline-query capability; Slang does not currently expose equivalent ray-stage capability and maps only vertex/pixel/compute stages.
- Stage vocabulary beyond the 35 registrations is not a current Renderer feature.
- Contract/reflection validation is strong source machinery, but only the exact staged toolchain and both runtime backends can close ABI/pipeline evidence.
- In-operation deduplication and content-addressed storage are not a persistent cross-invocation compile cache.
- Editor recook/hot reload must not leak into the Shipping runtime product.
- Atomic publication code must still be tested against cancellation, locked files, partial writes, stale artifacts, and concurrent reader behavior on the supported Windows filesystem path.
- The [Shader System feature acceptance contract](../../../CrossModule/ShaderSystem/Acceptance.md) remains unproved; source consistency is not completion.

## Primary Source Routes

- Shared contracts: `Tools/Shaders/ShaderContracts`.
- Compiler executable and orchestration: `Tools/Shaders/ShaderCompiler`.
- DXC backend: `Tools/Shaders/ShaderCompiler/Backends/Dxc`.
- Slang backend: `Tools/Shaders/ShaderCompiler/Backends/Slang`.
- Existing CLI validation: `Tools/Shaders/ShaderCompiler/ValidateShaderCompilerCli.cmake`.
- Engine sources: `Engine/Assets/Shaders`.
- Renderer registrations: `Engine/Renderer/ShaderRegistrations`.
- Renderer runtime shader ownership/materialization: `Engine/Renderer/Private/Pipeline` and `Engine/Renderer/Private/RayTracing`.
- Editor/application recook ownership: `Engine/Application/Private/ShaderRecook` and the corresponding Editor consumers.
- Build membership: root CMake options plus `Tools/Shaders/ShaderContracts/CMakeLists.txt`, `Tools/Shaders/ShaderCompiler/CMakeLists.txt`, `Engine/Renderer/CMakeLists.txt`, and `Engine/Application/CMakeLists.txt`.
