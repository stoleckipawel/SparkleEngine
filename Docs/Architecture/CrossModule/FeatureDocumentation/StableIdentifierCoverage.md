# Stable Identifier Architecture Coverage

Status: exact identifier-to-owner routing ledger; no implementation or acceptance result is inferred

Scope: exact source-authority identifiers and their Architecture owner; feature-local capability and acceptance IDs remain owned by their dossiers

Source authorities: [Engineer Persona](../../../Strategy/EngineerPersona.md), [Requirements](../../../Strategy/Requirements.md), [Roadmap](../../../Strategy/Roadmap.md), [Feature Completion Reports](../../../Acceptance/FeatureCompletionReports.md), [First Release](../../../Acceptance/FirstRelease.md), and [Graphics Workloads](../../../Acceptance/GraphicsWorkloads.md)

## Persona Outcomes

| ID | Architecture coverage |
| --- | --- |
| `NS-REAL` | [RHI](../../Modules/Engine/RHI/README.md), [Renderer](../../Modules/Engine/Renderer/README.md), [Product Execution](../ProductExecutionTraces.md), [Build/Packaging](../../Modules/BuildAndPackaging/README.md) |
| `NS-MATH-DATA` | [Core](../../Modules/Engine/Core/README.md), [Renderer Lighting](../../Modules/Engine/Renderer/Features/Lighting/README.md), [Temporal Sampling](../../Modules/Engine/Renderer/Features/FrameExecution/TemporalSamplingAndHistory.md), [Neural Training/Evaluation](../NeuralGraphics/TrainingAndEvaluation.md) |
| `NS-EVIDENCE` | [Performance Diagnostics](../PerformanceDiagnostics/README.md), [RHI Diagnostics](../../Modules/Engine/RHI/Features/DiagnosticsAndCapture/README.md), [CI And Regression](../../Modules/BuildAndPackaging/ContinuousIntegrationAndRegression.md) |
| `NS-OWNERSHIP` | [Whole Repository Map](../../WholeRepositoryMap.md), module inventories, [Feature Execution](../FeatureExecutionTraces.md), [Product Execution](../ProductExecutionTraces.md) |
| `NS-ADOPTION` | [Adoption And Support](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md), [Packaging](../../Modules/BuildAndPackaging/PackagingAndInstallation.md), [Launcher](../../Modules/Tools/Launcher/README.md) |
| `NS-SIMPLIFY` | [Whole Repository Map](../../WholeRepositoryMap.md), [Strategy Coverage](../StrategyCoverage.md), and the explicit non-capability sections in each module/feature dossier |

## Principal Graphics Engineer Requirements

| ID | Architecture owner or explicit negative owner |
| --- | --- |
| `PGE-01` | [Adoption And Support](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md), [Launcher](../../Modules/Tools/Launcher/README.md), [Showcase](../../Modules/Projects/Showcase/README.md) |
| `PGE-02` | [Renderer Lighting](../../Modules/Engine/Renderer/Features/Lighting/README.md), [Renderer Ray Tracing](../../Modules/Engine/Renderer/Features/RayTracing/README.md), [RHI Ray Tracing](../../Modules/Engine/RHI/Features/PipelineAndExecution/RayTracing.md) |
| `PGE-03` | [Neural Graphics](../NeuralGraphics/README.md); current vendor-only inference remains under [Image Reconstruction](../../Modules/Engine/Renderer/Features/PostProcessing/ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md) |
| `PGE-04` | [Model To Kernel And Runtime Inference](../NeuralGraphics/ModelToKernelAndRuntimeInference.md), currently absent |
| `PGE-05` | [Performance Diagnostics](../PerformanceDiagnostics/README.md), [Frame Execution](../../Modules/Engine/Renderer/Features/FrameExecution/README.md), [Tasks](../../Modules/Engine/Tasks/README.md) |
| `PGE-06` | [RHI Diagnostics](../../Modules/Engine/RHI/Features/DiagnosticsAndCapture/README.md), [Renderer Diagnostics](../../Modules/Engine/Renderer/Features/ViewportAndDiagnostics/README.md), [ShaderCompiler](../../Modules/Tools/ShaderCompiler/README.md) |
| `PGE-07` | [Build/Packaging](../../Modules/BuildAndPackaging/README.md), module C++ inventories, [Python Automation And Analysis](../../Modules/Tools/PythonAutomationAndAnalysis.md) |
| `PGE-08` | [Core](../../Modules/Engine/Core/README.md), [Source Importers](../../Modules/Tools/SourceImporters/README.md), [Renderer](../../Modules/Engine/Renderer/README.md), [Neural Training](../NeuralGraphics/TrainingAndEvaluation.md) |
| `PGE-09` | [RHI](../../Modules/Engine/RHI/README.md), [Shader System](../ShaderSystem/README.md), [Shader Runtime](../../Modules/Engine/Renderer/Features/ShaderRuntime/README.md) |
| `PGE-10` | [Tasks](../../Modules/Engine/Tasks/README.md), [Multithreaded Engine](../MultithreadedEngine.md), [Frame Execution](../../Modules/Engine/Renderer/Features/FrameExecution/README.md) |
| `PGE-11` | [Neural Training And Evaluation](../NeuralGraphics/TrainingAndEvaluation.md), currently absent |
| `PGE-12` | [Neural Training](../NeuralGraphics/TrainingAndEvaluation.md) and [Runtime Inference](../NeuralGraphics/ModelToKernelAndRuntimeInference.md), currently absent as an owned route |
| `PGE-13` | Feature dossiers plus [ShaderCompiler](../../Modules/Tools/ShaderCompiler/README.md), [Cooking](../../Modules/Tools/Cooking/README.md), [Performance Diagnostics](../PerformanceDiagnostics/README.md), and [Adoption](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md) |
| `PGE-14` | [Platform](../../Modules/Engine/Platform/README.md), [Linux Platform Support](../../Modules/Engine/Platform/LinuxPlatformSupport.md), [RHI](../../Modules/Engine/RHI/README.md), [Build/Packaging](../../Modules/BuildAndPackaging/README.md) |
| `PGE-15` | [Whole Repository Map](../../WholeRepositoryMap.md), module/feature ownership routes, [Adoption And Support](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md) |

## Feature Completion Report Families

### Product, Build, And Delivery

| ID | Architecture owner |
| --- | --- |
| `FCR-PROD-01` | [Showcase](../../Modules/Projects/Showcase/README.md) and [Application](../../Modules/Engine/Application/README.md) |
| `FCR-PROD-02` | [Build/Packaging](../../Modules/BuildAndPackaging/README.md) and [Adoption](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md) |
| `FCR-PROD-03` | [Launcher](../../Modules/Tools/Launcher/README.md) |
| `FCR-PROD-04` | [Application](../../Modules/Engine/Application/README.md) and [Editor](../../Modules/Engine/Editor/README.md) |
| `FCR-PROD-05` | [Packaging And Installation](../../Modules/BuildAndPackaging/PackagingAndInstallation.md) |
| `FCR-PROD-06` | [Adoption, Support, And Incident Response](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md) and [RHI Diagnostics](../../Modules/Engine/RHI/Features/DiagnosticsAndCapture/README.md) |

### Foundation, World, And Content

| ID | Architecture owner |
| --- | --- |
| `FCR-CORE-01` | [Core](../../Modules/Engine/Core/README.md) |
| `FCR-PLAT-01` | [Platform](../../Modules/Engine/Platform/README.md), with explicit [Linux boundary](../../Modules/Engine/Platform/LinuxPlatformSupport.md) |
| `FCR-TASK-01` | [Tasks](../../Modules/Engine/Tasks/README.md) and [Multithreaded Engine](../MultithreadedEngine.md) |
| `FCR-WORLD-01` | [GameFramework](../../Modules/Engine/GameFramework/README.md) |
| `FCR-WORLD-02` | [GameFramework](../../Modules/Engine/GameFramework/README.md) and [Scene/View Preparation](../../Modules/Engine/Renderer/Features/SceneAndViewPreparation/README.md) |
| `FCR-WORLD-03` | [GameFramework](../../Modules/Engine/GameFramework/README.md), [Geometry/Resources](../../Modules/Engine/Renderer/Features/GeometryAndResources/README.md), and [Lighting](../../Modules/Engine/Renderer/Features/Lighting/README.md) |
| `FCR-CONT-01` | [Source Importers](../../Modules/Tools/SourceImporters/README.md) |
| `FCR-CONT-02` | [Cooking](../../Modules/Tools/Cooking/README.md) |
| `FCR-CONT-03` | [Engine Assets](../../Modules/Engine/Assets/README.md) |
| `FCR-SHDR-01` | [Shader System](../ShaderSystem/README.md), [ShaderCompiler](../../Modules/Tools/ShaderCompiler/README.md), and [Shader Runtime](../../Modules/Engine/Renderer/Features/ShaderRuntime/README.md) |
| `FCR-TOOL-01` | [Tool Support](../../Modules/Tools/ToolSupport/README.md) |

### RHI And GPU Execution

| ID | Architecture owner |
| --- | --- |
| `FCR-RHI-01` | [RHI Capability Inventory](../../Modules/Engine/RHI/CapabilityInventory.md) and [RHI Features](../../Modules/Engine/RHI/Features/README.md) |
| `FCR-RHI-02` | [Command Submission And Synchronization](../../Modules/Engine/RHI/Features/PipelineAndExecution/CommandSubmissionAndSynchronization.md) and [Presentation](../../Modules/Engine/RHI/Features/PresentationAndInterop/Presentation.md) |
| `FCR-RHI-03` | [Backend Selection And Device Capabilities](../../Modules/Engine/RHI/Features/DeviceAndResources/BackendSelectionAndDeviceCapabilities.md) plus the RHI feature families |
| `FCR-RHI-04` | [RHI Ray Tracing](../../Modules/Engine/RHI/Features/PipelineAndExecution/RayTracing.md) |
| `FCR-RHI-05` | [RHI Diagnostics And Capture](../../Modules/Engine/RHI/Features/DiagnosticsAndCapture/README.md) |
| `FCR-RHI-06` | [Device Lifecycle And Failure Recovery](../../Modules/Engine/RHI/Features/DeviceAndResources/DeviceLifecycleAndFailureRecovery.md) |

### Renderer

| ID | Architecture owner |
| --- | --- |
| `FCR-REN-01` | [Renderer](../../Modules/Engine/Renderer/README.md) and [Frame Execution](../../Modules/Engine/Renderer/Features/FrameExecution/README.md) |
| `FCR-REN-02` | [Scene And View Preparation](../../Modules/Engine/Renderer/Features/SceneAndViewPreparation/README.md) |
| `FCR-REN-03` | [Frame Graph And Scheduling](../../Modules/Engine/Renderer/Features/FrameExecution/FrameGraphAndScheduling.md) |
| `FCR-REN-04` | [Geometry, Materials, And GBuffer](../../Modules/Engine/Renderer/Features/GeometryAndResources/GeometryMaterialsAndGBuffer.md) |
| `FCR-REN-05` | [Renderer Ray Tracing](../../Modules/Engine/Renderer/Features/RayTracing/README.md) and [Geometry/GBuffer](../../Modules/Engine/Renderer/Features/GeometryAndResources/GeometryMaterialsAndGBuffer.md) |
| `FCR-REN-06` | [Direct Lighting](../../Modules/Engine/Renderer/Features/Lighting/DirectLighting.md) |
| `FCR-REN-07` | [Indirect Lighting](../../Modules/Engine/Renderer/Features/Lighting/IndirectLighting.md) |
| `FCR-REN-08` | [Offline Path Tracer](../../Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/README.md) |
| `FCR-REN-09` | [Exposure](../../Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/Exposure.md) |
| `FCR-REN-10` | [Image Reconstruction And Upscaling](../../Modules/Engine/Renderer/Features/PostProcessing/ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md) |
| `FCR-REN-11` | [Debug Views](../../Modules/Engine/Renderer/Features/DebugViews/README.md) and [Diagnostics/Capture](../../Modules/Engine/Renderer/Features/ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) |
| `FCR-REN-12` | [Renderer Ray Tracing](../../Modules/Engine/Renderer/Features/RayTracing/README.md) and [RHI Ray Tracing](../../Modules/Engine/RHI/Features/PipelineAndExecution/RayTracing.md) |
| `FCR-REN-13` | [UI And Viewport Composition](../../Modules/Engine/Renderer/Features/ViewportAndDiagnostics/UiAndViewportComposition.md) |
| `FCR-REN-14` | [Tone Mapping](../../Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/ToneMapping.md) |
| `FCR-REN-15` | [Presentation And Output](../../Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/PresentationAndOutput.md) |
| `FCR-REN-16` | [Pipeline Materialization And Typed Binding](../../Modules/Engine/Renderer/Features/ShaderRuntime/PipelineMaterializationAndTypedBinding.md) |
| `FCR-REN-17` | [Mesh And Texture Residency](../../Modules/Engine/Renderer/Features/GeometryAndResources/MeshAndTextureResidency.md) |
| `FCR-REN-18` | [Temporal Sampling And History](../../Modules/Engine/Renderer/Features/FrameExecution/TemporalSamplingAndHistory.md) |
| `FCR-REN-19` | [Settings State And Persistence](../../Modules/Engine/Renderer/Features/RuntimeConfiguration/SettingsStateAndPersistence.md) |
| `FCR-REN-20` | [Latency Coordination](../../Modules/Engine/Renderer/Features/FrameExecution/LatencyCoordination.md) |
| `FCR-REN-21` | [Visibility And Draw Preparation](../../Modules/Engine/Renderer/Features/GeometryAndResources/VisibilityAndDrawPreparation.md) |
| `FCR-REN-22` | [Resolution, Sampling, And Anti-Aliasing](../../Modules/Engine/Renderer/Features/PostProcessing/ReconstructionAndGeneration/ResolutionSamplingAndAntiAliasing.md) |

## Release Gates And Risks

| ID | Architecture coverage |
| --- | --- |
| `REL-00` | [Runtime Configuration](../../Modules/Engine/Renderer/Features/RuntimeConfiguration/README.md), module inventories, [Adoption](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md) |
| `REL-01` | [Packaging](../../Modules/BuildAndPackaging/PackagingAndInstallation.md), [Assets](../../Modules/Engine/Assets/README.md), [Showcase](../../Modules/Projects/Showcase/README.md) |
| `REL-02` | [Build/Packaging](../../Modules/BuildAndPackaging/README.md), [CI/Regression](../../Modules/BuildAndPackaging/ContinuousIntegrationAndRegression.md) |
| `REL-03` | [Packaging And Installation](../../Modules/BuildAndPackaging/PackagingAndInstallation.md) |
| `REL-04` | All module/feature dossiers; exact current families are mapped above |
| `REL-05` | [Renderer Features](../../Modules/Engine/Renderer/Features/README.md), [Showcase](../../Modules/Projects/Showcase/README.md), [Offline Path Tracer](../../Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/README.md) |
| `REL-06` | [Performance Diagnostics](../PerformanceDiagnostics/README.md), [Frame Execution](../../Modules/Engine/Renderer/Features/FrameExecution/README.md), residency owners |
| `REL-07` | [RHI Diagnostics](../../Modules/Engine/RHI/Features/DiagnosticsAndCapture/README.md), [Device Lifecycle](../../Modules/Engine/RHI/Features/DeviceAndResources/DeviceLifecycleAndFailureRecovery.md) |
| `REL-08` | [Packaging](../../Modules/BuildAndPackaging/PackagingAndInstallation.md), [Adoption](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md) |
| `REL-09` | [Adoption And Support](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md), [Product Workflows](../ProductWorkflowCoverage.md) |
| `REL-10` | [Packaging](../../Modules/BuildAndPackaging/PackagingAndInstallation.md), [Adoption/Support](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md) |
| `REL-11` | [Adoption, Support, And Incident Response](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md) |
| `RISK-REL-01` | `REL-00` owners |
| `RISK-REL-02` | `REL-01` owners |
| `RISK-REL-03` | `REL-02` owners |
| `RISK-REL-04` | `REL-03` and `REL-08` owners |
| `RISK-REL-05` | `REL-04` feature owners |
| `RISK-REL-06` | `REL-05` owners |
| `RISK-REL-07` | `REL-06` owners |
| `RISK-REL-08` | `REL-07` owners |
| `RISK-REL-09` | `REL-08` and `REL-09` owners |
| `RISK-REL-10` | `REL-10` and `REL-11` owners |
| `RISK-REL-11` | [CI/Regression](../../Modules/BuildAndPackaging/ContinuousIntegrationAndRegression.md) and every evidence-producing feature owner |
| `RISK-REL-12` | [Strategy Coverage](../StrategyCoverage.md) and release sequencing authority |
| `RISK-REL-13` | [Offline Path Tracer](../../Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/README.md) |

### Common Release Failure Modes

| ID | Architecture owner |
| --- | --- |
| `FM-REL-01` | [Platform](../../Modules/Engine/Platform/README.md), [Linux boundary](../../Modules/Engine/Platform/LinuxPlatformSupport.md), [RHI device capability](../../Modules/Engine/RHI/Features/DeviceAndResources/BackendSelectionAndDeviceCapabilities.md) |
| `FM-REL-02` | [Packaging And Installation](../../Modules/BuildAndPackaging/PackagingAndInstallation.md) |
| `FM-REL-03` | [Packaging And Installation](../../Modules/BuildAndPackaging/PackagingAndInstallation.md) |
| `FM-REL-04` | [Core](../../Modules/Engine/Core/README.md), [Packaging](../../Modules/BuildAndPackaging/PackagingAndInstallation.md) |
| `FM-REL-05` | [Assets](../../Modules/Engine/Assets/README.md), [Shader System](../ShaderSystem/README.md), [Showcase](../../Modules/Projects/Showcase/README.md) |
| `FM-REL-06` | [Packaging And Installation](../../Modules/BuildAndPackaging/PackagingAndInstallation.md), [Core](../../Modules/Engine/Core/README.md) |
| `FM-REL-07` | [Core](../../Modules/Engine/Core/README.md), [Renderer Settings](../../Modules/Engine/Renderer/Features/RuntimeConfiguration/SettingsStateAndPersistence.md) |
| `FM-REL-08` | [Feature Selectors](../../Modules/Engine/Renderer/Features/RuntimeConfiguration/FeatureSelectorCatalog.md), [Image Reconstruction](../../Modules/Engine/Renderer/Features/PostProcessing/ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md) |
| `FM-REL-09` | [Device Lifecycle](../../Modules/Engine/RHI/Features/DeviceAndResources/DeviceLifecycleAndFailureRecovery.md), [RHI Diagnostics](../../Modules/Engine/RHI/Features/DiagnosticsAndCapture/README.md) |
| `FM-REL-10` | [RHI Resource Lifetime](../../Modules/Engine/RHI/Features/DeviceAndResources/ResourceLifetimeAndMemory.md), [Renderer Residency](../../Modules/Engine/Renderer/Features/GeometryAndResources/MeshAndTextureResidency.md) |
| `FM-REL-11` | [Platform](../../Modules/Engine/Platform/README.md), [RHI Presentation](../../Modules/Engine/RHI/Features/PresentationAndInterop/Presentation.md), [Frame Execution](../../Modules/Engine/Renderer/Features/FrameExecution/README.md) |
| `FM-REL-12` | [Adoption And Support](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md), [Packaging](../../Modules/BuildAndPackaging/PackagingAndInstallation.md) |
| `FM-REL-13` | [Adoption And Support](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md), [RHI Diagnostics](../../Modules/Engine/RHI/Features/DiagnosticsAndCapture/README.md) |
| `FM-REL-14` | [Build/Packaging](../../Modules/BuildAndPackaging/README.md), [CI/Regression](../../Modules/BuildAndPackaging/ContinuousIntegrationAndRegression.md) |
| `FM-REL-15` | [Packaging](../../Modules/BuildAndPackaging/PackagingAndInstallation.md), [Adoption/Support](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md) |
| `FM-REL-16` | [Packaging](../../Modules/BuildAndPackaging/PackagingAndInstallation.md), [Adoption/Support](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md) |

## Graphics Map Gates

| ID | Architecture coverage |
| --- | --- |
| `MAP-00` | [Showcase](../../Modules/Projects/Showcase/README.md), [Performance Diagnostics](../PerformanceDiagnostics/README.md), Renderer/RHI diagnostics |
| `MAP-01` | [Showcase](../../Modules/Projects/Showcase/README.md), [Geometry/GBuffer](../../Modules/Engine/Renderer/Features/GeometryAndResources/GeometryMaterialsAndGBuffer.md), [Lighting](../../Modules/Engine/Renderer/Features/Lighting/README.md) |
| `MAP-02` | [Showcase](../../Modules/Projects/Showcase/README.md), [Presentation/Output](../../Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/PresentationAndOutput.md) |
| `MAP-03` | [Showcase](../../Modules/Projects/Showcase/README.md), [Geometry/GBuffer](../../Modules/Engine/Renderer/Features/GeometryAndResources/GeometryMaterialsAndGBuffer.md) |
| `MAP-04` | [Showcase](../../Modules/Projects/Showcase/README.md), [GameFramework](../../Modules/Engine/GameFramework/README.md), [Temporal Sampling](../../Modules/Engine/Renderer/Features/FrameExecution/TemporalSamplingAndHistory.md) |
| `MAP-05` | [Showcase](../../Modules/Projects/Showcase/README.md), [Geometry/GBuffer](../../Modules/Engine/Renderer/Features/GeometryAndResources/GeometryMaterialsAndGBuffer.md), [Ray Tracing](../../Modules/Engine/Renderer/Features/RayTracing/README.md) |
| `MAP-06` | [Showcase](../../Modules/Projects/Showcase/README.md), [Residency](../../Modules/Engine/Renderer/Features/GeometryAndResources/MeshAndTextureResidency.md), [Visibility](../../Modules/Engine/Renderer/Features/GeometryAndResources/VisibilityAndDrawPreparation.md) |
| `MAP-07` | [Showcase](../../Modules/Projects/Showcase/README.md), [Source Importers](../../Modules/Tools/SourceImporters/README.md), Renderer feature owners |
| `MAP-08` | [Showcase](../../Modules/Projects/Showcase/README.md), [Source Importers](../../Modules/Tools/SourceImporters/README.md), [Lighting](../../Modules/Engine/Renderer/Features/Lighting/README.md) |
| `MAP-09` | [Showcase](../../Modules/Projects/Showcase/README.md), [Geometry/GBuffer](../../Modules/Engine/Renderer/Features/GeometryAndResources/GeometryMaterialsAndGBuffer.md), [Residency](../../Modules/Engine/Renderer/Features/GeometryAndResources/MeshAndTextureResidency.md) |
| `MAP-10` | [Showcase](../../Modules/Projects/Showcase/README.md), [Lighting](../../Modules/Engine/Renderer/Features/Lighting/README.md) |
| `MAP-11` | [Geometry Cache Animation](../GeometryCacheAnimation/README.md), [Temporal Sampling](../../Modules/Engine/Renderer/Features/FrameExecution/TemporalSamplingAndHistory.md), [Ray Tracing](../../Modules/Engine/Renderer/Features/RayTracing/README.md) |
| `MAP-12` | [Showcase](../../Modules/Projects/Showcase/README.md), [Source Importers](../../Modules/Tools/SourceImporters/README.md), [Residency](../../Modules/Engine/Renderer/Features/GeometryAndResources/MeshAndTextureResidency.md) |
| `MAP-13` | [Showcase](../../Modules/Projects/Showcase/README.md), [Offline Path Tracer](../../Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/README.md), [Lighting](../../Modules/Engine/Renderer/Features/Lighting/README.md) |

## Specialist Cases And Workload Ladder

| ID | Architecture coverage |
| --- | --- |
| `CASE-01` | SourceImporters, Cooking, Showcase, Geometry/GBuffer, Debug Views |
| `CASE-02` | [RHI Features](../../Modules/Engine/RHI/Features/README.md), [Graphics Coverage](../GraphicsCoverageMatrix.md), [Feature Execution](../FeatureExecutionTraces.md) |
| `CASE-03` | [Offline Path Tracer](../../Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/README.md), [Performance Diagnostics](../PerformanceDiagnostics/README.md) |
| `CASE-04` | [Neural Graphics](../NeuralGraphics/README.md), [Training/Evaluation](../NeuralGraphics/TrainingAndEvaluation.md), [Model To Kernel](../NeuralGraphics/ModelToKernelAndRuntimeInference.md) |
| `CASE-05` | [Packaging](../../Modules/BuildAndPackaging/PackagingAndInstallation.md), [Adoption/Support](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md), [Product Workflows](../ProductWorkflowCoverage.md) |
| `WL-01` | [Assets](../../Modules/Engine/Assets/README.md), [Source Importers](../../Modules/Tools/SourceImporters/README.md), [Cooking](../../Modules/Tools/Cooking/README.md) |
| `WL-02` | [Showcase](../../Modules/Projects/Showcase/README.md), source/cook owners |
| `WL-03` | Showcase and Renderer/RHI feature owners, including [Offline Path Tracer](../../Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/README.md) |
| `WL-04` | [Performance Diagnostics](../PerformanceDiagnostics/README.md), [RHI](../../Modules/Engine/RHI/README.md), [Renderer](../../Modules/Engine/Renderer/README.md) |
| `WL-05` | [Neural Training And Evaluation](../NeuralGraphics/TrainingAndEvaluation.md) |
| `WL-06` | [Model To Kernel And Runtime Inference](../NeuralGraphics/ModelToKernelAndRuntimeInference.md) |
| `WL-07` | [Performance Diagnostics](../PerformanceDiagnostics/README.md), [CI/Regression](../../Modules/BuildAndPackaging/ContinuousIntegrationAndRegression.md), [Adoption](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md) |
| `WL-08` | Feature dossiers plus [Adoption/Support](../../Modules/BuildAndPackaging/AdoptionSupportAndIncidentResponse.md); publication content remains a strategy/acceptance artifact, not an engine module |

All current identifiers in the named persona, requirement, FCR registry, release-gate/risk register, map queue, specialist-case table, and workload ladder occur explicitly above. New identifiers must be added here in the same change that introduces them.
