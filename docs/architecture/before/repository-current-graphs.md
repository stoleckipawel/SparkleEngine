# Repository Current-State Graphs

Status: before/current architecture graphs
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This page shows the current architecture as graphs. Use it to understand existing module edges, artifact flow, and transitional risk points before continuing the global refactor.

Target threading-readiness comparison: [../after/repository-threading-readiness.md](../after/repository-threading-readiness.md)

## Current Global Module Graph

```mermaid
flowchart TD
    Core[SparkleCore]
    Platform[SparklePlatform]
    RHI[SparkleRHI]
    D3D12[D3D12 backend]
    Vulkan[Vulkan backend]
    RendererRegs[SparkleRendererShaderRegistrations]
    Renderer[SparkleRenderer]
    Game[SparkleGameFramework]
    Editor[SparkleEditor]
    App[SparkleApplication]
    AppEditor[SparkleApplicationEditor]

    ShaderCompiler[ShaderCompiler]
    Import[SourceImportAdapters]
    Texture[TextureCooker]
    Mesh[MeshCooker]
    Material[MaterialCooker]
    Scene[SceneCooker]
    AssetCooker[AssetCooker]
    Converter[AssetConverter]
    LauncherCore[SparkleLauncherCore]
    LauncherGui[SparkleLauncher Qt GUI]
    Projects[Projects/Showcase]

    Platform --> Core
    RHI --> Core
    RHI --> Platform
    RHI --> D3D12
    RHI --> Vulkan

    RendererRegs --> RHI
    Renderer --> RendererRegs
    Renderer --> RHI
    Renderer --> Game

    Game --> Core
    Game --> Platform
    Game --> RHI

    Editor --> Renderer
    Editor --> RHI
    Editor --> Game
    App --> Renderer
    App --> Game
    App --> Platform
    AppEditor --> App
    AppEditor --> Editor

    ShaderCompiler --> Core
    ShaderCompiler --> RHI
    ShaderCompiler --> RendererRegs

    Import --> Core
    Import --> Game
    Texture --> Core
    Texture --> RHI
    Mesh --> Import
    Mesh --> Game
    Material --> Import
    Material --> Texture
    Material --> Game
    Scene --> Mesh
    Scene --> Material
    Scene --> Import
    Scene --> Game
    AssetCooker --> Import
    AssetCooker --> Mesh
    AssetCooker --> Material
    AssetCooker --> Scene
    AssetCooker --> Texture
    Converter --> Import
    Converter --> Mesh
    Converter --> Material
    Converter --> Scene

    LauncherGui --> LauncherCore
    LauncherCore --> Core
    LauncherCore --> Projects
```

## Current Graphics And Shader Flow

```mermaid
flowchart LR
    ShaderSource[Engine/Assets/Shaders]
    RendererRegs[Renderer shader registrations]
    ShaderCompiler[ShaderCompiler]
    CookedPackages[Cooked shader packages]
    RHIShader[RHI shader package/runtime primitives]
    RendererRuntime[Renderer pass and PSO runtime]
    Backends[D3D12/Vulkan PSO creation]

    ShaderSource --> RendererRegs
    RendererRegs --> ShaderCompiler
    ShaderCompiler --> CookedPackages
    CookedPackages --> RHIShader
    RHIShader --> RendererRuntime
    RendererRuntime --> Backends
```

Current note:

- Renderer pass registrations now live above RHI.
- The remaining risk is keeping `ShaderCompiler` linked to the narrow registration target, not full renderer runtime behavior.

## Current Content Pipeline Flow

```mermaid
flowchart LR
    SourceAssets[Source assets: glTF/FBX/images]
    Import[SourceImportAdapters]
    Texture[TextureCooker]
    Mesh[MeshCooker]
    Material[MaterialCooker]
    Scene[SceneCooker]
    AssetCooker[AssetCooker orchestration]
    CookedArtifacts[Cooked runtime artifacts]
    Game[GameFramework loaders]
    Renderer[Renderer resource managers]
    RHI[RHI upload/runtime contracts]

    SourceAssets --> Import
    SourceAssets --> Texture
    AssetCooker --> Import
    AssetCooker --> Texture
    AssetCooker --> Mesh
    AssetCooker --> Material
    AssetCooker --> Scene
    Import --> Mesh
    Import --> Material
    Import --> Scene
    Material --> Texture
    Mesh --> CookedArtifacts
    Material --> CookedArtifacts
    Texture --> CookedArtifacts
    Scene --> CookedArtifacts
    CookedArtifacts --> Game
    Game --> Renderer
    Renderer --> RHI
```

Current note:

- This flow is the area most likely to be damaged by graphics refactors unless artifact schemas and runtime consumers are checked together.

## Current Host And Validation Flow

```mermaid
flowchart TD
    Launcher[Launcher smoke workflow]
    App[Application validation]
    Renderer[Renderer]
    RHI[RHI]
    D3D12[D3D12 native capture debt]
    Vulkan[Vulkan backend]
    Artifacts[Validation artifacts]

    Launcher --> App
    App --> Renderer
    App --> RHI
    App -. transitional Stage 8 .-> D3D12
    RHI --> D3D12
    RHI --> Vulkan
    App --> Artifacts
```

Current note:

- The dotted edge is the known Application validation debt: backend-native capture/readback must move behind RHI/backend validation services.

## Current Boundary Risk Graph

```mermaid
flowchart TD
    Runtime[Runtime modules]
    Tools[Tools implementation]
    Game[GameFramework]
    RendererPrivate[Renderer/Private]
    RHIPrivate[RHI/Private]
    Renderer[Renderer]
    NativeAPI[D3D12/Vulkan native API]
    Provider[Provider integration paths]

    Runtime -. forbidden .-> Tools
    Game -. forbidden .-> RendererPrivate
    Game -. forbidden .-> RHIPrivate
    Renderer -. forbidden outside providers .-> NativeAPI
    Provider -. transitional Stage 9 .-> NativeAPI
```

Current boundary checks protect the RHI/Renderer edges today. Stage 28 expands the same mechanical protection to the rest of this graph.

## Current Threading Readiness Risk Graph

```mermaid
flowchart TD
    LiveGame[Mutable GameFramework scene]
    Renderer[Renderer frame and passes]
    FrameGraph[Frame graph setup/compile/execute]
    RHI[RHI command and queue services]
    Tools[Import/cook/shader tools]
    Launcher[LauncherCore and Qt UI]
    FutureRuntime[Future runtime workers]
    FutureQueues[Future GPU queues]
    FutureToolJobs[Future tool jobs]

    LiveGame -. must become immutable RenderContracts snapshots .-> Renderer
    Renderer -. must become frame data and command batches .-> FrameGraph
    FrameGraph -. must name queue, batch, resource access, and fences .-> RHI
    RHI -. central submission must protect queues .-> FutureQueues
    Tools -. must become deterministic job requests and reports .-> FutureToolJobs
    Launcher -. UI must send process requests, not own work .-> FutureToolJobs

    FutureRuntime -. must not read producer-private mutable state .-> LiveGame
    FutureRuntime -. must not mutate pass/global state .-> Renderer
```

Current note:

- The repository is not required to implement multithreading now. The risk is preserving live mutable owner access in places where future workers, render-thread work, async queues, cook jobs, shader jobs, or launcher workflows would need stable handoff data.
