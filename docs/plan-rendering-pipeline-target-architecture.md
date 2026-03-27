# Rendering Pipeline Target Architecture Plan

## Goal

Reshape Sparkle's renderer toward the pipeline architecture shown in the Vulkan tutorial reference:

- Scene data
- Scene culling
- Visible objects
- Render pass management
- Rendergraph and synchronization as supporting infrastructure
- Command generation
- Command buffers
- Execution
- Post-processing
- Final image

The key design rule is this:

The rendergraph is support infrastructure for pass scheduling, resource lifetime tracking, dependency analysis, and synchronization. It must not become the top-level owner of scene logic, feature orchestration policy, or renderer services.

## Target Architecture

### Stage 1: Scene Data

Responsibilities:

- Capture scene state from gameplay/runtime systems
- Build stable render-facing scene snapshots
- Resolve materials, textures, mesh references, lights, and cameras into renderable data

Current Sparkle mapping:

- `RenderSceneSnapshot`
- `RenderSceneDataBuilder`
- `MaterialCacheManager`
- `TextureManager`
- `RenderCamera`

Target state:

- Renderer consumes a frame-ready scene snapshot and render scene data package
- No framegraph types should leak into this stage

### Stage 2: Scene Culling

Responsibilities:

- Determine visible geometry and relevant lights/views
- Produce visible-object lists and shadow-view work lists

Current Sparkle mapping:

- `PerViewDataBuilder`
- `ShadowBuilder`
- `ShadowFrameBuilder`
- `ViewLightingBuilder`

Target state:

- Culling and view extraction happen before framegraph setup
- Framegraph pass registration consumes already prepared visibility and view data

### Stage 3: Render Pass Management

Responsibilities:

- Choose which passes exist this frame
- Define feature ordering at a logical level
- Decide which techniques are enabled for the frame

Current Sparkle mapping:

- `FrameGraphBuilder`
- `FrameGraph/Features/*.cpp`

Target state:

- Feature registration files remain the place where passes are composed
- Pass management becomes a renderer-owned orchestration layer above the framegraph
- The rendergraph receives a pass graph description, not high-level renderer policy

### Stage 4: Rendergraph And Synchronization

Responsibilities:

- Track resources
- Infer dependencies from pass declarations
- Build execution order
- Plan transitions and aliasing
- Resolve imported/transient resources

Current Sparkle mapping:

- `FrameGraph`
- `PassBuilder`
- `FrameGraphCompiler*`
- `ResourceRegistry`
- `FrameGraphExternalResources.cpp`
- `FrameGraphResourceResolution.cpp`

Target state:

- Keep the graph focused on dependency management, synchronization, and resource lifetime planning
- Do not let the graph become a pass manager, service locator, or renderer policy hub

### Stage 5: Command Generation

Responsibilities:

- Record GPU commands for each pass in dependency-safe order
- Bind pipelines, resources, and pass runtime state

Current Sparkle mapping:

- `FrameGraph::Execute(...)`
- authored pass `Execute(...)` bodies
- `ShaderPass.h`
- `PassUtilities.h`
- `RenderPassPipelineTraits.h`
- `D3D12PassBinder`

Target state:

- Graph execution remains thin
- Pass bodies own pass-specific command generation
- Renderer-owned runtime factories provide pass runtimes without concentrating all pass policy in one traits file

### Stage 6: Execution

Responsibilities:

- Submit recorded command lists
- coordinate frame fences, swap chain, and present flow

Current Sparkle mapping:

- `Renderer::BeginFrame()`
- `Renderer::SubmitFrame()`
- `Renderer::EndFrame()`

Target state:

- This remains renderer-owned and outside the framegraph

### Stage 7: Post-Processing

Responsibilities:

- Own post-lighting/full-screen effects as a distinct stage in the pipeline
- Consume lit scene outputs and produce final-presentable image

Current Sparkle mapping:

- Not yet a clear dedicated stage
- `ComputeClearShowcase` and presentation copy/UI composition are transitional examples, not a full post stack

Target state:

- Introduce an explicit post-processing feature layer
- Make final presentation consume a post-processed final color resource rather than ad hoc showcase output

## Current State Versus Target

### What Already Matches The Target Well

- Frame setup and scene extraction are largely outside the framegraph
- Framegraph compiler responsibilities are mostly graph-appropriate
- Feature-owned pass registration already exists
- Execution submission and swapchain flow are renderer-owned

### What Does Not Yet Match The Target

- `FrameGraph.h` exposes too much internal body and too many internal structs
- `FrameGraphBuilder` plus `FrameGraphDependencies` behaves like a transport bag rather than a real pass-management boundary
- `RenderPassPipelineTraits.h` centralizes too much pass-specific policy
- `ShaderPass.h` mixes authoring abstraction with validation and orchestration helpers
- `PassUtilities.h` mixes execute-time helpers with graph authoring helpers
- Post-processing is not yet a clear first-class stage
- Presentation is still mixed with generic copy/showcase flow rather than a dedicated final-image pipeline

## Design Rules To Enforce

### Rule 1: Framegraph Is Supporting Infrastructure

Allowed responsibilities:

- Resource registration
- Pass declaration capture
- Dependency inference
- Execution ordering
- Barrier generation
- Transient lifetime planning
- Runtime resource/view resolution

Disallowed responsibilities:

- Scene culling
- feature enable/disable policy
- top-level renderer orchestration
- general renderer service aggregation
- central ownership of all pass runtime recipes

### Rule 2: Pass Management Lives Above The Graph

The renderer should decide which features and pass families exist for the frame.
The framegraph should schedule those decisions, not make them.

### Rule 3: Command Generation Lives In Passes And Narrow Runtime Helpers

Pass-specific draw/dispatch logic should live with the pass.
Low-level binding/layout logic may live in RHI or narrow renderer runtime helpers.
Do not keep all pass recipes in one central traits header forever.

### Rule 4: Presentation And Post-Processing Must Be Explicit Pipeline Stages

Copy-to-backbuffer and UI composition are not enough as a final architecture.
The pipeline should visibly distinguish:

- scene rendering outputs
- post-processing outputs
- final presentation output

## Migration Plan

### Phase 1: Shrink The Framegraph Public Body

Objective:

- Keep framegraph public API small and graph-specific

Actions:

- Split internal compile/execution/transient structures out of `Engine/Renderer/Public/FrameGraph/FrameGraph.h`
- Move compiled plan records, barrier structs, and transient planning structs into private compiler/execution headers
- Keep in the public header only:
  - resource import/create APIs
  - pass registration APIs
  - setup/compile/execute entrypoints
  - runtime resolve helpers that passes truly need

Expected outcome:

- Framegraph becomes easier to reason about as infrastructure instead of architecture center

### Phase 2: Remove The Fake Pass-Management Boundary

Objective:

- Replace `FrameGraphDependencies` and make pass management explicit

Actions:

- Remove `FrameGraphDependencies`
- Simplify or remove `FrameGraphBuilder`
- Prefer one of these final forms:
  - renderer-owned `InitializeFrameGraph()` directly calls feature registration
  - a narrow `BuildFrameGraph(...)` function takes explicit inputs with no dependency bag

Preferred direction:

- Keep feature registration files
- Remove the generic dependency bag

Expected outcome:

- Pass management becomes a clear renderer orchestration concern, not a pseudo-abstraction

### Phase 3: Separate Pass Management From Graph Infrastructure

Objective:

- Make pipeline stages explicit above the graph

Actions:

- Introduce a renderer-owned pipeline orchestration concept, likely in `Renderer.cpp` or a dedicated renderer pipeline module
- Make the orchestration order explicit:
  - scene targets
  - shadow passes
  - opaque/main scene passes
  - lighting or resolve stage
  - post-processing stage
  - final presentation stage
- Keep each stage in feature-owned files

Expected outcome:

- The framegraph supports the pipeline instead of representing the entire pipeline architecture by itself

### Phase 4: Break Up Central Pass Policy Files

Objective:

- Remove central trait/policy concentration that makes command generation hard to extend

Actions:

- Split `Engine/Renderer/Public/Passes/ShaderPass.h` into:
  - authored pass abstraction
  - parameter validation helpers
  - bind/dispatch adapters if still needed
- Split `Engine/Renderer/Public/Passes/PassUtilities.h` into:
  - execute-time binding helpers
  - graph authoring helpers for transfer/presentation if still necessary
- Replace `Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h` with pass-owned or pass-family-owned runtime factory code

Expected outcome:

- Adding a new pass no longer requires editing a giant central policy file
- Pass-specific command generation becomes more local and maintainable

### Phase 5: Introduce Explicit Pipeline Outputs

Objective:

- Match the tutorial architecture where rendering stages produce named outputs that feed later stages

Actions:

- Define explicit scene-stage outputs, lighting outputs, post-processing outputs, and presentation inputs
- Stop treating compute showcase output as a stand-in for the final image pipeline
- Introduce a `FinalColor` or equivalent resource that is the input to presentation/UI composition

Expected outcome:

- Data flow becomes visible and maintainable
- The pipeline diagram maps cleanly onto the codebase

### Phase 6: Add A Real Post-Processing Stage

Objective:

- Establish the post-processing box from the target architecture as a real subsystem

Actions:

- Create a post-processing feature module under framegraph features or a neighboring renderer pipeline folder
- Start with one simple pass family:
  - tonemap
  - gamma/output transform
  - optional UI composite at the final stage
- Route the final presented image through this stage

Expected outcome:

- Post-processing becomes a clear extension point instead of an afterthought

### Phase 7: Harden Synchronization And Resource Responsibilities

Objective:

- Keep synchronization inside the graph and keep low-level descriptor work in narrow resource helpers

Actions:

- Keep barrier inference and aliasing in compiler files
- Move raw descriptor construction details out of large `FrameGraph` method bodies when practical
- Keep imported resource sync as a graph-adjacent resource responsibility, not a renderer feature responsibility
- Generalize root-pass selection away from pure backbuffer-write rooting

Expected outcome:

- Synchronization remains automatic and local to the graph implementation
- Renderer stages stay policy-focused rather than barrier-focused

## Proposed Target Code Layout

### Renderer-Level Pipeline

- `Renderer/Private/Frame/` remains frame data building and culling preparation
- `Renderer/Private/Pipeline/` becomes renderer-owned pipeline orchestration and runtime factories
- `Renderer/Private/FrameGraph/Features/` remains feature stage registration
- `Renderer/Private/Passes/` remains authored pass implementation

### Framegraph-Level Infrastructure

- `Renderer/Public/FrameGraph/` contains only graph-facing author APIs and small public types
- `Renderer/Private/FrameGraph/Compiler/` owns dependency, barrier, and transient planning
- `Renderer/Private/FrameGraph/Execution/` owns execution and barrier emission
- `Renderer/Private/FrameGraph/Resources/` owns registration, imported resource sync, transient materialization, and runtime resolution

### Pass Runtime And Binding

- `Renderer/Private/Passes/` or neighboring runtime files own pass-specific runtime factory logic
- `RHI/D3D12/Pipeline/` keeps low-level compiled binding layout, root signature, binder, and pipeline state logic

## Concrete Refactor Order

### Step 1

Split `FrameGraph.h` internals into private headers.

### Step 2

Remove `FrameGraphDependencies` and flatten `FrameGraphBuilder` usage.

### Step 3

Make renderer-owned stage ordering explicit in framegraph setup.

### Step 4

Break up `RenderPassPipelineTraits.h`, `ShaderPass.h`, and `PassUtilities.h`.

### Step 5

Introduce explicit `FinalColor` and post-process stage outputs.

### Step 6

Generalize root-pass selection and tighten graph/resource boundary rules.

## Definition Of Done

We are at the target architecture when the following are true:

- Scene extraction and culling are fully outside the framegraph
- Pass management is explicit and renderer-owned
- The framegraph only handles graph/resource/synchronization concerns
- Command generation is pass-owned, with only narrow shared runtime helpers
- Post-processing exists as a real named pipeline stage
- Final presentation consumes a clear final-image resource
- No dependency bag like `FrameGraphDependencies` is needed to explain the architecture
- `FrameGraph.h` is no longer the largest conceptual container in the renderer

## Recommendation

Start with structural debt, not new rendering features.

The fastest path toward the target architecture is:

1. shrink `FrameGraph.h`
2. remove `FrameGraphDependencies`
3. flatten renderer-owned stage orchestration
4. split central pass policy files
5. add explicit post-processing and final-image stages

That order reduces architectural ambiguity first, which will make later additions like deferred shading, Forward+, PBR, tonemapping, and other post effects much cleaner to implement.