# Renderer Front-End Review And Directions

Date: 2026-05-03
Status: Review draft for team alignment

## Why This Document

Goal: make Sparkle front-end frame authoring feel closer to Unreal and NVIDIA sample style:

- one obvious place to read frame order
- low ceremony for adding a pass
- explicit parameter/resource flow
- keep strong typing and current framegraph backend

This review focuses on front-end authoring only. It does not propose replacing the framegraph compiler/transient allocator architecture.

## Executive Summary

Sparkle already has strong foundations:

- typed pass parameters
- framegraph dependency and barrier planning
- cooked shader pipeline and runtime registry

Main issue is front-end readability and onboarding cost. Frame order, pass registration, parameter setting, and resource story are spread across multiple files and abstraction layers.

Recommendation:

1. Keep framegraph core as-is.
2. Introduce a single top-level frame narration file (UE-like read path).
3. Move pass registration to a single manifest/source-of-truth.
4. Standardize pass authoring contract to: Declare -> SetParams -> Execute.
5. Add frame-report output for pass/resource/barrier visibility.

---

## Current Readability Map

### Current browse path (developer trying to understand one frame)

```text
Renderer.cpp
  -> BuildFrameContext
  -> FrameGraphBuilder.cpp
      -> Feature helpers (GBufferPasses, PresentationPasses, etc.)
      -> Add pass + allocate pass params
  -> FrameGraph::Setup/Compile/Execute
      -> pass Execute()
          -> parameter sync/bind helpers
```

### Problem shape

```text
What runs this frame?
  spread across:
    [Renderer.cpp]
      [FrameGraphBuilder.cpp]
        [Feature*.cpp]
          [Pass*.cpp]
            [PassUtilities + RuntimeRegistry + Traits]
```

This is powerful, but not friendly for quick inspection.

---

## Unreal Reference Anchor (Keep As Ongoing Design Guide)

This document intentionally keeps Unreal-specific references as orientation anchors.

Purpose:

- preserve a familiar mental model for navigation and naming
- validate that Sparkle front-end reads frame-first, not tool-first
- keep references stable across refactors

Anchor patterns to mirror:

1. Frame-first renderer entrypoint shape (Unreal deferred renderer style):
  - renderer-level `Render(...)` is the narrative root
  - graph object (`FRDGBuilder&`) is an argument/tool, not module identity
2. Section-oriented composition:
  - frame is composed of large sections first (geometry/base, lighting, post, presentation)
  - deeper detail is discovered by opening the relevant section files
3. Explicit pass authoring:
  - pass declaration and scheduling are explicit
  - readability comes from centralized orchestration and consistent pass contracts

Sparkle counterpart intent (reference map):

- `BuildFrame(...)` is the frame narrative root
- `FrameFrontEnd` owns section-level composition only
- section files own pass grouping and pass order/details
- framegraph objects remain implementation tools passed through APIs

Boundary note:

- This is a reference model, not strict API mimicry. Sparkle keeps its typed framegraph backend and compile-time safety model.

---

## Core Offenders, Options, And Direction

## Offender 0: Naming Is Tool-Centric Instead Of Frame-Centric

Symptom:

- front-end naming and file skeleton emphasize framegraph internals instead of the frame being authored
- developers read "FrameGraph*" first, while intent should start from "BuildFrame" and then cascade into sections

Reference pattern to mimic (Unreal style):

- Unreal front-end renderer entry typically reads as frame orchestration first, with RDG passed in as a tool/argument (for example the Render function shape centered around renderer logic and taking FRDGBuilder&).
- The graph object is critical, but not the identity of the front-end module.

### Decision: Option B (selected)

Frame-first naming with framegraph passed as a build tool.

Example shape:

- `Frame/Frame.cpp` (or `Frame/BuildFrame.cpp`) owns `BuildFrame(...)`
- `BuildFrame(...)` receives graph builder/context as an argument
- section functions are intent-named: `BuildGeometrySection`, `BuildLightingSection`, `BuildPostSection`, `BuildPresentationSection`

Pros:

- aligns with the hierarchical root model the team wants
- mirrors Unreal/NVIDIA readability better
- keeps framegraph implementation power while reducing naming noise

Cons:

- one-time rename/move churn
- requires documentation and convention updates

Principle: the product is a frame, the framegraph is a tool used to build it.

## Offender 1: No Single Frame Narration File

Symptom:

- frame sequence is not narrated in one place end-to-end
- developer must jump through builder/features/passes to understand intent

### Decision: Option B (selected)

Add a dedicated `FrameFrontEnd` file that only narrates pass order and calls existing helpers.

Pros:

- UE-like top-level readability
- low risk, incremental
- no framegraph rewrite

Cons:

- some duplication until old paths are trimmed

Make one canonical file for frame narration and keep feature helpers as implementation details.

### Required hierarchy model

The frame front-end should follow this information hierarchy:

```text
Frame
  -> Sections
    -> Pass Groups
      -> Passes
```

Reference hierarchy:

```text
BuildFrame(...)
  -> BuildGBuffer(...)
    -> AddGBufferBasePass(...)
    -> AddGBufferMaterialPass(...)
  -> BuildLighting(...)
    -> BuildDirectLighting(...)
      -> AddDeferredDirectLightPass(...)
    -> BuildIndirectLighting(...)
      -> AddIndirectDiffusePass(...)
    -> BuildVolumetrics(...)
      -> AddVolumetricIntegratePass(...)
  -> BuildPostProcessing(...)
    -> AddToneMapPass(...)
    -> AddPostCompositePass(...)
  -> BuildPresentation(...)
    -> AddCopyToBackBufferPass(...)
```

Navigation intent:

- `FrameFrontEnd` shows section-level composition only.
- Each section gets its own dedicated file.
- Complex sections (starting with Lighting) can be split into dedicated lighting files (`Direct`, `Indirect`, `Volumetrics`).
- Pass lists live in section files, not in the frame root file.

---

## Offender 2: Pass Registration Touches Too Many Places

Symptom:

Adding one pass usually means touching multiple files:

- runtime type registry
- pipeline trait specialization
- pipeline storage tuple
- builder wiring
- pass implementation files

Reality check (Unreal/NVIDIA style):

- Unreal does not make pass onboarding fully automatic. You still author the pass class/function, parameter struct, shader type/permutation declarations, and the callsite where the pass is added to the frame.
- What Unreal does better is centralizing intent and reducing boilerplate churn around registration. The frame read path is obvious, and most repetitive glue is hidden behind consistent patterns.
- NVIDIA samples/tools also keep explicit pass authoring, but usually present a cleaner "declare pass + schedule pass" surface with less registry noise.

### Decision: Option B (selected)

Central pass manifest that generates runtime registry and storage tuple glue.

Keep typed compile-time safety but centralize declaration of active passes. This gives most of the workflow benefit people associate with Unreal while staying aligned with Sparkle's current typed framegraph model.

---

## Offender 3: Parameter Flow Is Not Obvious

Symptom:

Parameter values are assembled partly in builder code and partly in pass Execute methods, with implicit Sync lifecycle.

### Decision: Option B (selected)

Standard pass contract:

- DeclareResources(builder, frame)
- SetParameters(frame, runtime, params)
- Execute(commands, runtime, params)

Favor explicit, readable pass code over heavy automation.

---

## Offender 4: Resource Story Is Hard To Inspect At Front-End Level

Symptom:

Resource lifetimes, aliasing, and barriers are correct, but hard to inspect unless reading compiler internals.

### Decision: Option A (selected)

Keep as-is for now.

---

## Offender 5: Duplicate Front-End Extent/Target Setup Logic

Symptom:

Repeated width/height/scene-extent logic in multiple feature files.

### Decision: Option B (selected)

Central `FrameSizing` / `FrameTargets` helper for front-end.

---

## Target Front-End Shape (Unreal/NVIDIA-inspired)

### Top-level frame narration

```text
+--------------------------------------------------+
| BuildFrame(RenderGraphTool& graph, FrameInputs) |
+--------------------------------------------------+
| 1) Build scene/view context                      |
| 2) Build frame targets                           |
| 3) BuildGBuffer                                  |
| 4) BuildLighting                                 |
|    - Direct                                      |
|    - Indirect                                    |
|    - Volumetrics                                 |
| 5) BuildPostProcessing                           |
| 6) BuildPresentation                             |
| 7) BuildDebug                                    |
+--------------------------------------------------+
```

Key convention:

- front-end files and symbols should be frame-intent named
- framegraph types can appear in parameters and internals, not as the primary identity of the front-end layer

### Pass authoring shape

```text
Pass file responsibilities:

[Declare]
  - parameter schema
  - resource usage declarations

[SetParameters]
  - map frame/runtime data -> typed params

[Execute]
  - only GPU commands/dispatch/draw
```

### Resource visibility shape

```text
Frame Report (example)

Pass 0: GBuffer
  writes: GBufferBaseColor, GBufferNormal, MainDepth
Pass 1: DeferredLighting
  reads : GBufferBaseColor, GBufferNormal, MainDepth
  writes: SceneColor
Pass 2: CopySceneColorToBackBuffer
  reads : SceneColor
  writes: BackBuffer

Transient lifetimes:
  GBufferBaseColor: [0..1], block=2
  SceneColor      : [1..2], block=1

Barriers:
  GBufferNormal: RenderTarget -> ShaderResource before DeferredLighting
```

---

## Simulated Example: Adding A New Pass

Example pass: `ScreenSpaceFogPass`

## Current-style experience (approximate)

```text
Touchpoints (typical):
  1) New pass files (.h/.cpp, shader registration)
  2) Runtime type registry
  3) Pipeline traits specialization
  4) Pipeline storage tuple
  5) Frame builder wiring
  6) Feature helper wiring (sometimes)
```

Pseudo code pattern today:

```cpp
// 1) pass class + params
class ScreenSpaceFogPass {
public:
    struct Parameters { ... };
    static void Execute(RenderGraphPassContext&, ParameterInstance&);
};

// 2) runtime trait specialization
template <> struct RenderPassPipelineTraits<ScreenSpaceFogPass> { ... };

// 3) runtime registry list update
using RenderPassRuntimeRegistry = TypedRenderPassRuntimeRegistry<
    GBufferPass,
    DeferredLightingPass,
    ScreenSpaceFogPass,
    ComputeClearPass>;

// 4) PipelineStateManager tuple update
using PassRuntimeStorageTuple = std::tuple<
    RenderPassRuntimeStorage<GBufferPass>,
    RenderPassRuntimeStorage<DeferredLightingPass>,
    RenderPassRuntimeStorage<ScreenSpaceFogPass>,
    RenderPassRuntimeStorage<ComputeClearPass>>;

// 5) builder wiring
auto& fogParams = frameGraph->AllocPassParameters<ScreenSpaceFogPass>();
fogParams->SceneColor = frameGraph->CreateUAV(sceneTargets.SceneColor);
fogParams->Depth = frameGraph->CreateSRV(sceneTargets.MainDepth);
frameGraph->AddComputePass<ScreenSpaceFogPass>("ScreenSpaceFog", fogParams);
```

## Target-style experience

```text
Touchpoints (goal):
  1) New pass files (.h/.cpp + shader)
  2) One pass manifest entry
  3) One line in top-level frame narration
```

Pseudo code pattern target:

```cpp
// PassManifest.inl (single source of truth)
SPARKLE_PASS(GBufferPass,           Graphics)
SPARKLE_PASS(DeferredLightingPass,  Compute)
SPARKLE_PASS(ScreenSpaceFogPass,    Compute)
SPARKLE_PASS(ComputeClearPass,      Compute)
```

```cpp
// FrameFrontEnd.cpp (single readable frame order)
void BuildFrame(FrameBuilder& b, const FrameInputs& in)
{
    auto targets = b.CreateSceneTargets(in.extent);

  BuildGBuffer(b, in, targets);
  BuildLighting(b, in, targets);
  BuildPostProcessing(b, in, targets);
  BuildPresentation(b, in, targets);
}
```

```cpp
// Pass implementation contract
struct ScreenSpaceFogPass
{
    static void DeclareResources(PassBuilder& pb, const FrameInputs& in, FogParams& p);
    static void SetParameters(const FrameInputs& in, const RenderPassContext& ctx, FogParams& p);
    static void Execute(RenderGraphPassContext& rg, FogParams& p);
};
```

---

## Prioritized Improvement Plan

No build validation is required between phases. Run a full validation only after Phase 4. Each phase removes legacy code as it replaces it so the codebase is easier to reason about at each stage.

---

### Phase 1 — Frame-First Structure and Section Layout

**Goal:** create the frame narration root and section files, wire existing feature helpers into sections, and delete the now-redundant `FrameGraphFeatures::` namespace shell.

**What exists today (context for the agent):**

- `FrameGraphBuilder.cpp` includes `Frame/Frame.h` which does not exist yet. It calls `BuildRenderLoopFrame` and uses `FrameLoopBuildResult` — both must be created.
- Feature helpers live in `Engine/Renderer/Private/FrameGraph/Features/` under `namespace FrameGraphFeatures`: `GBufferPasses.h/.cpp`, `PresentationPasses.h/.cpp`, `ComputeShowcasePasses.h/.cpp`.
- Product types (`FrameGraphSceneTargets`, `FrameGraphGBufferTargets`) are in `FrameGraph/Features/FrameGraphProducts.h`.
- No `DeferredLightingPasses` feature file exists. Lighting is currently unhooked; a feature file must be created.
- `FrameGraphBuilder::Build()` (`FrameGraph/Builder/FrameGraphBuilder.cpp`) is the current owner of frame composition.

**Deliverables:**

1. Create `Engine/Renderer/Private/Frame/Frame.h` and `Frame/Frame.cpp`:
   - Declare and define `struct FrameBuildResult` (replaces `FrameLoopBuildResult`) with `SceneTargets` and `GBufferTargets`.
   - Declare and define `FrameBuildResult BuildFrame(FrameGraph& graph, const Window& window, const RenderViewportExtent& sceneExtent, bool presentToBackBuffer)`.
   - `BuildFrame` body calls: `BuildGBuffer(...)`, `BuildLighting(...)`, `BuildPresentation(...)`. No section calls in the body for sections that have no passes yet.

2. Create `Engine/Renderer/Private/Frame/GBuffer.h` and `GBuffer.cpp`:
   - `FrameGraphGBufferTargets BuildGBuffer(FrameGraph& graph, const Window& window, const RenderViewportExtent& sceneExtent, const FrameGraphSceneTargets& sceneTargets)`
   - Body: move the content currently in `FrameGraphFeatures::AddGBufferPass` here verbatim.

3. Create `Engine/Renderer/Private/Frame/Lighting.h` and `Lighting.cpp`:
   - `void BuildLighting(FrameGraph& graph, const Window& window, const RenderViewportExtent& sceneExtent, const FrameGraphSceneTargets& sceneTargets, const FrameGraphGBufferTargets& gbuffer, const RenderPassRuntimeRegistry& runtime)`
   - Body: call `BuildDirectLighting(...)`. Stub out `BuildIndirectLighting` and `BuildVolumetrics` as empty functions with a `// TODO` comment.
   - Create `Engine/Renderer/Private/Frame/DirectLighting.h` and `DirectLighting.cpp`:
     - `void BuildDirectLighting(FrameGraph& graph, const FrameGraphSceneTargets& sceneTargets, const FrameGraphGBufferTargets& gbuffer, const RenderPassRuntimeRegistry& runtime)`
     - Body: move the content of the deferred lighting pass wiring here (currently absent; wire `DeferredLightingPass` via `frameGraph.AddComputePass<DeferredLightingPass>` with its parameters).

4. Create `Engine/Renderer/Private/Frame/Presentation.h` and `Presentation.cpp`:
   - `void BuildPresentation(FrameGraph& graph, const FrameGraphSceneTargets& sceneTargets)`
   - Body: move the content of `FrameGraphFeatures::AddCopyToBackBufferPass` here verbatim.

5. Update `FrameGraphBuilder.cpp`:
   - Remove the `#include "Frame/Frame.h"` include (it already exists as the new file).
   - Replace the `BuildRenderLoopFrame` call with `BuildFrame(...)`.
   - Update result unpacking from `FrameLoopBuildResult` to `FrameBuildResult`.

6. **Remove legacy files** once their content has been moved:
   - Delete `FrameGraph/Features/GBufferPasses.h` and `GBufferPasses.cpp`.
   - Delete `FrameGraph/Features/PresentationPasses.h` and `PresentationPasses.cpp`.
   - Remove `namespace FrameGraphFeatures` from any remaining files.
   - If `ComputeShowcasePasses` is unused by active render paths, delete it too; otherwise leave it and note it as deferred cleanup.

7. Rename product types in `FrameGraphProducts.h` from `FrameGraphSceneTargets` -> `SceneTargets` and `FrameGraphGBufferTargets` -> `GBufferTargets`. Update all usages.

**Naming rules to enforce in this phase:**
- Files under `Frame/` are named after the section they own: `Frame.cpp`, `GBuffer.cpp`, `Lighting.cpp`, `DirectLighting.cpp`, `Presentation.cpp`.
- Function names: `BuildFrame`, `BuildGBuffer`, `BuildLighting`, `BuildDirectLighting`, `BuildPresentation`.
- No `FrameGraph*` prefix on any front-end function or product type name.
- `FrameGraph` type appears only as a parameter type, never in front-end symbol names.

---

### Phase 2 — Pass Manifest and Registry Centralization

**Goal:** eliminate the three separate places where each pass must be registered (runtime traits, runtime registry typedef, pipeline storage tuple) and replace with a single manifest.

**What exists today (context for the agent):**

- `Engine/Renderer/Private/FrameGraph/RenderPassRuntime.h`: contains per-pass runtime structs (`GBufferPassRuntime`, `DeferredLightingPassRuntime`, `ComputeClearPassRuntime`), per-pass `RenderPassRuntimeTraits` specializations, and the `RenderPassRuntimeRegistry` typedef (hardcoded as `TypedRenderPassRuntimeRegistry<GBufferPass, DeferredLightingPass, ComputeClearPass>`).
- `Engine/Renderer/Private/Pipeline/PipelineStateManager.h`: `PassRuntimeStorageTuple` is a hardcoded `std::tuple<RenderPassRuntimeStorage<GBufferPass>, RenderPassRuntimeStorage<DeferredLightingPass>, RenderPassRuntimeStorage<ComputeClearPass>>`.
- `Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h`: per-pass `RenderPassPipelineTraits` specializations (one per pass, each self-contained).

**Deliverables:**

1. Create `Engine/Renderer/Private/Pipeline/PassManifest.inl`:
   ```cpp
   SPARKLE_PASS(GBufferPass,          Graphics)
   SPARKLE_PASS(DeferredLightingPass, Compute)
   SPARKLE_PASS(ComputeClearPass,     Compute)
   ```

2. Update `RenderPassRuntime.h`:
   - Remove the hardcoded `using RenderPassRuntimeRegistry = TypedRenderPassRuntimeRegistry<...>` line.
   - Replace with a macro-driven expansion:
     ```cpp
     using RenderPassRuntimeRegistry = TypedRenderPassRuntimeRegistry<
     #define SPARKLE_PASS(PassType, Kind) PassType,
     #include "Pipeline/PassManifest.inl"
     #undef SPARKLE_PASS
     ... >;
     ```
     (Adjust syntax to your preferred X-macro pattern.)

3. Update `PipelineStateManager.h`:
   - Remove the hardcoded `PassRuntimeStorageTuple` typedef.
   - Replace with macro-driven expansion from `PassManifest.inl`.

4. Verify `RenderPassPipelineTraits.h` specializations are untouched (they remain per-pass, which is correct).

5. **Remove legacy:** delete any `#include` lines or explicit pass-list comments that enumerate pass types by hand outside the manifest.

**Adding a new pass after this phase requires only:** one `SPARKLE_PASS` line in `PassManifest.inl`, a new `RenderPassPipelineTraits` specialization, and a callsite in the relevant section file.

---

### Phase 3 — Pass Contract Standardization

**Goal:** give every pass a consistent three-stage shape so value flow is readable and parameter assembly is never inside Execute.

**What exists today (context for the agent):**

- `Engine/Renderer/Private/Passes/GBufferPass.h/.cpp`
- `Engine/Renderer/Private/Passes/DeferredLightingPass.h/.cpp`
- `Engine/Renderer/Private/Passes/ComputeClearPass.h/.cpp`
- Each pass currently exposes a static `Execute(RenderGraphPassContext&, ParameterInstance&)` method. Parameter setup lives in the section/feature helper that adds the pass.

**Deliverables:**

For each pass (`GBufferPass`, `DeferredLightingPass`, `ComputeClearPass`):

1. Add a static `DeclareResources(FrameGraph& graph, const FrameInputs& in, ParameterInstance& p)` method that owns resource creation and binding setup. Move resource/binding setup out of the section file into this method.
2. Add a static `SetParameters(const FrameContext& frame, const RenderPassRuntimeRegistry& runtime, ParameterInstance& p)` method that sets CPU-side cbuffer / descriptor values.
3. Keep `Execute(RenderGraphPassContext& ctx, ParameterInstance& p)` for GPU commands only.
4. Update section files (`GBuffer.cpp`, `DirectLighting.cpp`, `Presentation.cpp`) to call the three-stage pattern:
   ```cpp
   auto& p = graph.AllocPassParameters<GBufferPass>();
   GBufferPass::DeclareResources(graph, in, p);
   GBufferPass::SetParameters(frame, runtime, p);
   graph.AddRasterPass<GBufferPass>(GBufferPass::PassName, p);
   ```
5. Remove any parameter-setting code from `Execute` bodies.

---

### Phase 4 — FrameTargets / FrameSizing Helper

**Goal:** remove the duplicated `window.GetWidth()` / `sceneExtent.IsValid()` boilerplate that appears in every section file.

**What exists today (context for the agent):**

- `GBufferPasses.cpp` (now moved to `GBuffer.cpp`) contains:
  ```cpp
  const uint32_t width = static_cast<uint32_t>(window.GetWidth());
  const uint32_t height = static_cast<uint32_t>(window.GetHeight());
  const uint32_t sceneWidth  = sceneExtent.IsValid() ? sceneExtent.Width  : width;
  const uint32_t sceneHeight = sceneExtent.IsValid() ? sceneExtent.Height : height;
  ```
- Similar logic may exist in other section files.

**Deliverables:**

1. Create `Engine/Renderer/Private/Frame/FrameTargets.h`:
   ```cpp
   struct FrameSizing
   {
       uint32_t Width;
       uint32_t Height;
       uint32_t SceneWidth;
       uint32_t SceneHeight;

       static FrameSizing From(const Window& window, const RenderViewportExtent& sceneExtent);
   };
   ```
2. Create `Engine/Renderer/Private/Frame/FrameTargets.cpp` with the implementation of `FrameSizing::From`.
3. Update all section files that compute width/height to use `FrameSizing::From(window, sceneExtent)` instead.
4. **Remove legacy:** delete all duplicated width/height/extent blocks from section files once replaced.

---

### Phase 5 — Full Build Validation

**Goal:** confirm the codebase compiles cleanly and renders correctly after all structural changes.

Run:

1. Build `ShowcaseRuntime` (Debug x64) — expect zero errors.
2. Build `ShaderCompiler` (Debug x64) — expect zero errors.
3. Cook shader packages for active showcase projects.
4. Run `ShowcaseRuntime` and confirm the deferred lighting frame renders without D3D12 errors in the debug output.

If errors appear, diagnose from the most recent phase change first. Do not roll back earlier phases — fix forward.
