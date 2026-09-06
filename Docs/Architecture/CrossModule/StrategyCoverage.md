# Capability Coverage Against Persona, Roadmap, And Gap Assessment

Status: capability snapshot; current strategy crosswalk, not a replacement for strategy authority, release acceptance, or candidate evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; capability inventories reconciled with the [Engineer Persona](../../Strategy/EngineerPersona.md), [Requirements](../../Strategy/Requirements.md), [Gap Assessment](../../Strategy/Assessments/GapAssessment.md), and [Roadmap](../../Strategy/Roadmap.md); source evidence only

Scope: ensure the inventory is complete enough to support release scoping and the principal graphics engineer target, while distinguishing implemented product paths from scaffolding, prose, and unverified claims

Owner: this dated crosswalk is owned by `Docs/Architecture/CrossModule`; strategy documents retain target/sequence authority

## Product Identity Constraint

Sparkle's target is a compact renderer-first engine and evidence platform. Coverage therefore does not mean adding every conventional engine feature. A capability belongs in this inventory when it is one of:

1. a user-reachable product promise;
2. a required owner/producer/consumer/lifetime/build link in a current vertical slice;
3. a correctness, performance, diagnostics, or adoption surface needed to prove that slice;
4. an explicit unsupported boundary needed to prevent accidental advertising.

Unused enums, empty registries, future interfaces, and source files with no current producer-to-consumer path are scaffolding/vocabulary and are excluded from feature counts. They remain visible only when their presence could create a false public claim.

## Target Persona Outcomes

| Persona outcome | Current capability coverage | Principal gap before a credible claim |
| --- | --- | --- |
| `NS-REAL` complete real-time systems | D3D12/Vulkan RHI, deferred/ray rendering, world extraction, cooked delivery, editor/runtime hosts, Launcher vertical route | Packaged product, clean-machine run, representative correctness/stability/performance gates |
| `NS-MATH-DATA` math/data reasoning | BRDF/lighting, ray traversal/SBT, ReSTIR reservoirs, exposure, transforms/animation, import normalization, explicit buffer/ABI contracts | Canonical numerical oracles, sensitivity/convergence evidence, written derivations tied to results |
| `NS-EVIDENCE` rigorous evidence | Native RHI diagnostics/capture hooks, shader validation, task ETW, inventories, acceptance contracts, workload catalog | Actual retained build/runtime/native-validation/performance/package artifacts; automated regression path |
| `NS-OWNERSHIP` end-to-end ownership | Module boundaries and vertical traces now cover source -> cook -> world -> renderer -> product/tool | Release disposition for every reachable selector and one owner/reviewer per evidence item |
| `NS-ADOPTION` first-user success | Launcher discovery/readiness/Quick Start, catalog metadata, tool diagnostics, Docs routes | Root README, clean-clone procedure, package/install route, CI, independent first-user run |
| `NS-SIMPLIFY` simplification judgment | Cooked-only runtime, Application editor split, Renderer/RHI boundary, fixed semantic pipelines, source-backed negative coverage | Remove or hide unresolved vocabulary/options and avoid expanding editor/import/backend breadth before release closure |

## Principal Graphics Engineer Requirement Crosswalk

| Requirement | Current source-backed capability | Inventory route | Current gap/classification |
| --- | --- | --- | --- |
| `PGE-01` adoption | Launcher Quick Start, toolchain/dependency readiness, Showcase, deep Docs | [Launcher](../Modules/Tools/Launcher/README.md), [Showcase](../Modules/Projects/Showcase/README.md), [Build](../Modules/BuildAndPackaging.md) | Partial: no root README, package, CI, or independent clean-user proof. |
| `PGE-02` ray tracing/GI/path tracing | BLAS/TLAS, inline and native pipeline traversal, ray GBuffer/shadows, ReSTIR direct/indirect, reference path mode | [RHI](../Modules/Engine/RHI/README.md), [Renderer](../Modules/Engine/Renderer/README.md), [execution traces](FeatureExecutionTraces.md) | Strong source slice; no correctness/convergence/backend/performance evidence yet. |
| `PGE-03` neural graphics feature | DLSS Super Resolution and Ray Reconstruction providers through Streamline | [Renderer](../Modules/Engine/Renderer/README.md), [coverage matrix](GraphicsCoverageMatrix.md) | External inference integration only; no owned neural algorithm/model/training and D3D12/vendor gating applies. |
| `PGE-04` model-to-kernel implementation | No owned model/kernel/training path found | Negative coverage in this document | Gap. Shader kernels are graphics kernels, not evidence of ML model productization. |
| `PGE-05` whole-system performance | CPU/GPU timing, memory diagnostics, task trace, heavy workload catalog, bounded queues/cook memory | [Diagnostics](PerformanceDiagnostics/Capability.md), [Tasks](../Modules/Engine/Tasks/README.md), [Showcase](../Modules/Projects/Showcase/README.md) | Partial: instrumentation/source paths exist; no measured bottleneck study or accepted optimization result. |
| `PGE-06` hard debugging | RHI validation/DRED, capture, shader reflection/ABI validation, diagnostics UI, failure propagation | [RHI](../Modules/Engine/RHI/README.md), [Shader compilation](../Modules/Tools/ShaderCompiler/README.md), [Editor](../Modules/Engine/Editor/README.md) | Partial: facilities exist; no retained difficult-defect narrative with hypothesis/observation/fix evidence. |
| `PGE-07` C++ and Python | C++20 modular engine/tools; two project conversion scripts | [Build](../Modules/BuildAndPackaging.md), [Showcase](../Modules/Projects/Showcase/README.md) | C++ strong; Python is narrow offline conversion, not engineering/tooling depth. |
| `PGE-08` 3D math/numerics | transforms/frustum, coordinate conversion, animation interpolation, BRDF/sampling/reservoir/exposure math | [Core](../Modules/Engine/Core/README.md), [Importers](../Modules/Tools/SourceImporters/README.md), [Renderer](../Modules/Engine/Renderer/README.md) | Source present; numerical stability/oracle evidence missing. |
| `PGE-09` graphics APIs/shaders/ABI | D3D12/Vulkan, DXIL/SPIR-V, HLSL/Slang, reflection, typed registrations, explicit resources/barriers/descriptors | [RHI](../Modules/Engine/RHI/README.md), [Shader compilation](../Modules/Tools/ShaderCompiler/README.md), [shader catalog](../Modules/Engine/Renderer/Features/ShaderRuntime/ShaderProgramCatalog.md) | Strong source slice; paired native validation and final ABI evidence open. |
| `PGE-10` CPU/GPU/concurrency | Tasks DAG/lanes, world parallel systems, render thread, GPU queues, frame graph, upload/retirement | [Tasks](../Modules/Engine/Tasks/README.md), [GameFramework](../Modules/Engine/GameFramework/README.md), [Renderer](../Modules/Engine/Renderer/README.md) | Strong source architecture; race/deadlock/overlap/scaling evidence open. |
| `PGE-11` ML fundamentals | No owned learning/training/evaluation implementation found | Negative coverage | Gap; vendor SDK integration is not equivalent. |
| `PGE-12` training/inference | Streamline DLSS inference integration | [Renderer](../Modules/Engine/Renderer/README.md) | Partial external inference only; no datasets, training, model evaluation, deployment optimization, or owned runtime. |
| `PGE-13` research productization/tools/communication | Feature implementations, ShaderCompiler, cookers, diagnostics, capability/acceptance documentation | All inventories plus [evidence plan](../../Plans/CapabilityEvidence.md) | Partial until one research/technique choice is shown from source/paper -> implementation -> measured product result. |
| `PGE-14` platform/ecosystem | Windows, D3D12, Vulkan, NVIDIA Streamline/NVAPI, AMD Compressonator, PIX/RenderDoc hooks | [Platform](../Modules/Engine/Platform/README.md), [RHI](../Modules/Engine/RHI/README.md), [Build](../Modules/BuildAndPackaging.md) | Relevant but narrow: no Linux/macOS, mobile, console, CUDA, owned driver/compiler work, or multi-vendor neural path. |
| `PGE-15` judgment/influence | Explicit boundaries, clean-break standards, release contract, negative coverage, reviewer maps | Docs architecture/strategy/standards | Internal design evidence only; no external collaboration/adoption/influence artifact yet. |

## Roadmap Release-Surface Closure

| Roadmap subsystem | Detailed inventory | Horizontal links | Vertical path | Evidence backlog | Coverage verdict |
| --- | --- | --- | --- | --- | --- |
| Core | [Core](../Modules/Engine/Core/README.md) | paths/config/diagnostics used across modules | [product traces](ProductExecutionTraces.md) for publication/process consumers | `CORE-*` in evidence plan | Covered at source depth |
| Tasks | [Tasks](../Modules/Engine/Tasks/README.md) | Application/world/renderer/tools consumers | [settlement trace](ProductExecutionTraces.md#trace-7-cancellation-failure-and-shutdown-settlement) | `TASK-*` | Covered at source depth |
| Platform | [Platform](../Modules/Engine/Platform/README.md) | Application/Editor/RHI window boundary | Win32-to-layered-input trace | `PLAT-*` | Covered at source depth |
| RHI | [RHI](../Modules/Engine/RHI/README.md) | [graphics matrix](GraphicsCoverageMatrix.md) | [graphics traces](FeatureExecutionTraces.md) | `RHI-*` | Covered at source depth |
| Renderer | [Renderer](../Modules/Engine/Renderer/README.md) | graphics matrix/shader catalog | graphics traces | `REN-*` | Covered at source depth |
| GameFramework/world | [GameFramework](../Modules/Engine/GameFramework/README.md) | Tasks/assets/Application/Renderer boundary | [asset-to-frame](ProductExecutionTraces.md#trace-2-authored-asset-to-rendered-result) and graphics frame traces | `GF-*` | Covered at source depth |
| Application | [Application](../Modules/Engine/Application/README.md) | [product workflow matrix](ProductWorkflowCoverage.md) | Quick Start, settings, capture, and shutdown in [product traces](ProductExecutionTraces.md) | `APP-*` | Covered at source depth |
| Engine source assets | [Engine Assets](../Modules/Engine/Assets/README.md) | shader/import/cook consumers | source-to-cooked routes | `EASSET-*` | Covered as source corpus |
| Source import | [Importers](../Modules/Tools/SourceImporters/README.md) | per-format semantic matrix plus [workflow matrix](ProductWorkflowCoverage.md) | [authored asset to rendered result](ProductExecutionTraces.md#trace-2-authored-asset-to-rendered-result) | `IMP-*` | Covered at source depth |
| Asset cooking | [Cooking](../Modules/Tools/Cooking/README.md) | product/format matrix plus [workflow matrix](ProductWorkflowCoverage.md) | Quick Start and asset-publication [product traces](ProductExecutionTraces.md) | `COOK-*` | Covered at source depth |
| Shader delivery | [Shader Compilation](../Modules/Tools/ShaderCompiler/README.md) | shader catalog/backend matrix | compile-to-runtime trace | `SHD-*` | Covered at source depth |
| Shared tool support | [Tool Support](../Modules/Tools/ToolSupport/README.md) | AssetCooker/TextureCooker/ShaderCompiler consumers | operation-to-captured-output trace | `TOOL-*` | Covered at source depth |
| Editor | [Editor](../Modules/Engine/Editor/README.md) | world/renderer/tool selectors plus [workflow matrix](ProductWorkflowCoverage.md) | [editor inspection and transaction](ProductExecutionTraces.md#trace-3-editor-inspection-and-transaction) | `ED-*` | Covered at source depth |
| Launcher | [Launcher](../Modules/Tools/Launcher/README.md) | build/cook/catalog/product operations plus [workflow matrix](ProductWorkflowCoverage.md) | [Quick Start to live level](ProductExecutionTraces.md#trace-1-quick-start-to-a-live-level) | `LAUNCH-*` | Covered at source depth |
| Showcase | [Showcase](../Modules/Projects/Showcase/README.md) | workload-to-feature matrix plus actor journeys | Quick Start and asset-to-frame product traces | `SHOW-*` | Covered at source depth |
| Build/package | [Build And Packaging](../Modules/BuildAndPackaging.md) | all target memberships plus [workflow matrix](ProductWorkflowCoverage.md) | Quick Start trace stops explicitly before formal package ownership | `BUILD-*` | Build covered; formal package absent |

No roadmap subsystem remains “Not yet audited” in this snapshot. That means the documentation map is complete at source depth; it does not mean any subsystem is approved for release.

## Iteration And Evidence Handoff

Capability IDs such as `CORE-*`, `RHI-*`, and `REN-*` identify source-inspected claims; they are not completion or test results. When a row enters active work, use the [Change Lifecycle iteration record](../../Engineering/Workflow/ChangeLifecycle.md#create-the-iteration-control-record) to connect it to the applicable `NS-*` and `PGE-*` target, roadmap `REL-*`/`MAP-*` gate, `FCR-*` family, `RISK-*`, binary `AC-*`, controlled `FM-*`, and one or more `CHK-*` actions. The [Capability Evidence Plan](../../Plans/CapabilityEvidence.md) supplies the smallest claim-falsifying backlog check; the candidate report owns results and retained artifacts.

An iteration is blocked when any applicable target, risk, criterion, failure, check, result, artifact, defect, or waiver is orphaned. Source evidence `S` can justify scope and test selection, but it cannot satisfy build, runtime, native-validation, performance, package, adoption, or approval evidence levels.

## 2026-09-06 Gap Revalidation

This table refreshes volatile observations from the dated Gap Assessment without rewriting its historical baseline.

| Earlier gap theme | Current tree observation | Effect on planning |
| --- | --- | --- |
| Root onboarding | Still no root `README.md`; `LICENSE.txt` is present; Docs routing is extensive. | Adoption remains open. |
| CI/tests | No `.github` automation and no CMake `enable_testing`/`add_test` found. | Automated regression and clean adoption remain open. |
| Python | Two Showcase source-conversion scripts now exist; no general Python tooling/test layer. | Narrow improvement, not closure of PGE Python depth. |
| Neural graphics | DLSS Super Resolution and Ray Reconstruction integrations are current source paths. | Prior blanket “no neural” statement is stale; classify accurately as external/vendor inference, not owned ML. |
| Platform breadth | Platform and public window/input path remain Win32; D3D12/Vulkan are graphics backends on that host. | Linux/macOS remain absent and should not be implied. |
| Build health | This documentation pass did not configure/build. | Prior build results are not current proof; `B` remains absent. |
| Packaging | Development artifact staging exists, but no install/CPack/package target or manifest was found. | Release remains blocked on a formal packaged product path. |

## Decision Rule For What To Do Next

The inventory now favors release closure over feature expansion. The next decisions should be made in this order:

1. freeze Included/Experimental/Excluded/Removed disposition for every reachable selector and Showcase level;
2. close clean configure/build plus cooked-runtime and editor/runtime erasure claims;
3. create and run the package/install path from clean bytes;
4. execute correctness/native-validation/stability evidence for the frozen graphics matrix;
5. measure the accepted workload set and write one end-to-end principal-level case study;
6. only then choose whether the largest persona gap is owned neural work, platform breadth, or another complete graphics slice.

Adding more effects, formats, panels, or generic systems before these gates close would increase the undocumented/release surface the user is trying to eliminate.
