## Plan: Vulkan Transition Milestones

Refine SparkleEngine into a two-milestone rendering transition. Milestone 1 makes the engine abstraction-ready while D3D12 remains the only concrete backend: renderer code must consume only backend-neutral services, all concrete D3D12 execution stays inside the RHI module, D3D12 debugging/profiling/capture support becomes first-class, and the shader pipeline shifts to a precompiled workflow with backend-neutral metadata. Milestone 2 fills in the Vulkan implementation against those stabilized abstractions until D3D12 and Vulkan reach feature parity. This keeps the high-risk architectural work ahead of the backend bring-up and avoids teaching Vulkan a D3D12-shaped runtime-compile model.

**Baseline**
- Current D3D12 strengths: validation layer wiring, InfoQueue filtering, live-object reporting, and shader symbol output already exist in the D3D12 path.
- Current D3D12 weaknesses: shared/public contracts still expose D3D12 or DXGI concepts; renderer private code still compiles shaders at runtime and still depends on D3D12 pipeline/binding types; there is no backend-neutral GPU diagnostics/profiling/capture layer.
- Current Vulkan state: enum placeholder exists, but there is no Vulkan backend implementation or Vulkan-facing shader/resource/pipeline path.

**Steps**
1. Milestone 1 / Phase 1A: Neutralize shared contracts while keeping D3D12 as the only backend.
   - What this phase brings: shared Renderer and shared RHI code stop encoding D3D12/DXGI concepts, so later backend work plugs into stable contracts instead of rewriting public types twice.
   - Prompt 1: replace D3D12 and DXGI concepts in shared/public headers with backend-neutral contracts.
     Positive guiderails: replace shared format/state/descriptor-facing types with neutral enums and descriptors; move D3D12-only concepts behind RHI implementation folders; keep renderer-facing APIs semantically rich enough for raster and compute.
     Negative guiderails: do not introduce Vulkan-specific types yet; do not leave temporary D3D12 compatibility fields in renderer-owned shared structs; do not broaden scope into a render-graph redesign.
     Reasoning: if this is skipped, every Vulkan step later either leaks Vulkan upward or preserves D3D12 leakage forever.
   - Prompt 2: quarantine native interop to explicit seams only.
     Positive guiderails: keep opaque native handles only for true interop boundaries such as editor host integration or external tooling; move any ID3D12Resource, descriptor-handle, and DXGI format ownership out of renderer-private abstractions where a neutral logical handle should exist.
     Negative guiderails: do not let renderer materials, framegraph resources, or pass runtime structs keep concrete D3D12 handles; do not add renderer-side backend branches.
     Reasoning: the renderer should depend on abstract services, not on native resource identity.
   - Phase exit snapshot:
     Completed: shared/public contracts are backend-neutral; D3D12 remains the sole working backend; renderer-facing APIs no longer require D3D12 or DXGI types.
     WIP: renderer private execution and material/binding internals may still be migrating behind the new contracts.
     Todo: shader pipeline hardening, GPU tooling abstraction, and all Vulkan implementation work.
2. Milestone 1 / Phase 1B: Rebase renderer execution, binding, and transient-resource ownership onto the abstraction.
   - What this phase brings: the renderer becomes a backend consumer instead of a shadow D3D12 implementation, which is the key architectural gate before Vulkan code starts.
   - Prompt 1: replace D3D12-shaped command recording and resource resolution with backend-neutral execution services.
     Positive guiderails: move native transition/barrier/resource-view application into backend-owned translators; keep framegraph authoring intact where possible; define neutral command-list or command-encoder concepts that can support raster and compute now and async compute later.
     Negative guiderails: do not add a lowest-common-denominator wrapper that hides important capabilities; do not reintroduce direct D3D12 command-list usage in renderer code; do not create a brand-new render graph unless the existing one truly blocks the abstraction.
     Reasoning: this is the point where the engine stops conflating renderer logic with D3D12 execution details.
   - Prompt 2: redesign material and descriptor ownership around a portable bindful model.
     Positive guiderails: materials should store logical resource-view or descriptor-table identities rather than raw GPU descriptor handles; stable/draw-time binding concepts should stay shared while heap/set implementation details stay in backend code.
     Negative guiderails: do not pivot into bindless; do not keep D3D12 GPU descriptor handles in material runtime structs; do not let Vulkan force a renderer-visible descriptor-set concept.
     Reasoning: bindful portability is the most direct path to parity and preserves a clean future extension point for optional bindless work.
   - Phase exit snapshot:
     Completed: renderer public and private code use backend-neutral command/binding/resource services; D3D12 concrete execution is concentrated in RHI.
     WIP: D3D12 adapter implementations behind the new seams may still be stabilizing and may need performance tuning.
     Todo: precompiled shader workflow, generic GPU diagnostics layer, D3D12 tooling completion, and Vulkan backend implementation.
3. Milestone 1 / Phase 1C: Introduce the precompiled shader workflow and D3D12-first diagnostics/profiling/capture support.
   - What this phase brings: a production-worthy D3D12 control backend with stable shader ABI and real tooling, which becomes the reference target for Vulkan parity.
   - Prompt 1: replace runtime pass-driven shader compilation with a precompiled workflow backed by neutral metadata.
     Positive guiderails: move shader compilation out of renderer startup and pass traits; define cooked shader artifacts that carry stage binaries, resource-layout metadata, specialization information if needed, and debug-symbol references; ensure the schema has room for DXIL now and SPIR-V next.
     Negative guiderails: do not freeze metadata around D3D12 root-signature specifics; do not depend on runtime reflection for shipping flows; do not wait until Vulkan implementation begins to define the shader asset schema.
     Reasoning: this is the best timing for the precompiled workflow. Earlier than this, the shared binding/layout ABI is still in flux and you risk baking D3D12 assumptions into cooked metadata. Later than this, Vulkan bring-up will inherit the wrong runtime-compile model and duplicate the cleanup.
   - Prompt 2: add a backend-neutral GPU diagnostics and profiling surface, then implement it for D3D12.
     Positive guiderails: add resource naming, scoped GPU event markers, timestamp-query based GPU timing, debug message routing, and crash/leak diagnostics surfaces at the abstraction layer; wire D3D12 to PIX-friendly events, InfoQueue logging, DRED-style diagnostics if adopted, live-object reporting, and RenderDoc-friendly labels/capture points.
     Negative guiderails: do not put PIX, RenderDoc, or tool SDK calls directly into renderer passes; do not make PIX the only supported diagnostic path; do not start Vulkan tooling work here beyond shaping the abstraction.
     Reasoning: tooling belongs in Milestone 1 because you need a strong D3D12 reference backend for debugging the abstraction refactor itself and for later comparing Vulkan behavior and performance against a trustworthy control path.
   - Prompt 3: create the Milestone 1 gate and regression checks.
     Positive guiderails: add build and smoke validation for D3D12-only execution, shader artifact generation, pipeline creation, GPU timing path sanity, and editor viewport/overlay rendering through the abstraction.
     Negative guiderails: do not declare readiness based only on compilation; do not add Vulkan parity checks yet.
     Reasoning: Milestone 1 is successful only if D3D12 is cleanly abstracted and still operational, observable, and debuggable.
   - Phase exit snapshot:
     Completed: D3D12 is the only concrete backend but is fully hosted behind the abstraction; renderer consumes only neutral interfaces; precompiled shader artifacts produce DXIL plus neutral metadata; D3D12 has first-class diagnostics/profiling/capture support.
     WIP: editor UI abstraction or residual D3D12-only tool seams may still be finishing.
     Todo: Vulkan backend implementation and full dual-backend parity.
4. Milestone 2 / Phase 2A: Bring up the Vulkan foundation against the stabilized abstraction.
   - What this phase brings: Vulkan stops being a planning target and becomes a working backend foundation for Windows-first execution.
   - Prompt 1: implement Vulkan device, queue, swapchain, synchronization, and resource-allocation services behind the existing abstraction.
     Positive guiderails: keep Windows-first scope; map capability queries through the shared contract; prefer VMA by default for allocator robustness unless a concrete engine requirement disproves it; keep queue abstractions extensible for later async compute while shipping one graphics queue first.
     Negative guiderails: do not change shared abstractions unless a proven Vulkan mismatch exists; do not add Vulkan-only renderer branches to compensate for missing backend work; do not optimize for Linux or broad portability yet.
     Reasoning: Vulkan should fit the architecture stabilized in Milestone 1, not force a second architecture pass.
   - Prompt 2: establish Vulkan validation and early capture/debug readiness.
     Positive guiderails: enable validation layers, resource naming, debug labels, and RenderDoc-friendly submission naming from the start; design the same annotation API introduced in Milestone 1 to map cleanly onto Vulkan debug utilities.
     Negative guiderails: do not chase full feature parity in this phase; do not entangle Nsight-specific logic with shared renderer code.
     Reasoning: early validation reduces bring-up ambiguity and keeps backend bugs local.
   - Phase exit snapshot:
     Completed: Vulkan can initialize, allocate resources, record/submit simple work, present, and expose the same high-level diagnostics/annotation hooks as D3D12.
     WIP: full pass coverage, shader/pipeline translation depth, and editor/runtime feature completeness.
     Todo: descriptor/pipeline/shader parity and full feature validation.
5. Milestone 2 / Phase 2B: Fill in Vulkan pipeline, descriptor, shader, and pass support until feature coverage matches D3D12.
   - What this phase brings: the Vulkan backend becomes functionally meaningful rather than just bootable.
   - Prompt 1: implement Vulkan pipeline-layout, descriptor, and shader-module translation from the shared metadata model.
     Positive guiderails: consume the same precompiled shader metadata used by D3D12; map logical binding tables to Vulkan descriptor set layouts, pools, and sets behind the backend boundary; keep raster and compute support first-class.
     Negative guiderails: do not fall back to Vulkan-runtime reflection as the primary shipping path; do not introduce separate Vulkan-only material layouts or pass schemas; do not re-shape the shader asset format around Vulkan semantics alone.
     Reasoning: if the cooked shader ABI is truly neutral, both backends should consume it with different translators rather than diverging asset pipelines.
   - Prompt 2: port renderer features one system at a time until Vulkan covers the D3D12 set.
     Positive guiderails: close gaps explicitly for material textures, shadow maps, transient resources, compute clear or equivalent compute passes, viewport outputs, resize paths, and editor-facing scene outputs; verify each feature against the D3D12 behavior rather than only against "it renders."
     Negative guiderails: do not declare parity on a scene-by-scene eyeball test; do not postpone core pass validation until the end.
     Reasoning: phased feature closure is easier to diagnose and less risky than a giant final parity push.
   - Prompt 3: add Vulkan-side profiling and external tool workflows once command recording is stable.
     Positive guiderails: validate RenderDoc capture quality, Vulkan debug labels, timestamp queries, and Nsight Graphics or Systems workflows on stabilized command paths; keep the abstraction-level marker/timer API shared with D3D12.
     Negative guiderails: do not add PIX concepts to Vulkan; do not attempt Nsight Aftermath or vendor-specific crash tooling unless crash-dump needs justify the extra complexity.
     Reasoning: this is the best moment for Nsight support. Doing it earlier adds churn while the Vulkan command path is still unstable; doing it later delays the very tools needed to close remaining parity and performance gaps.
   - Phase exit snapshot:
     Completed: Vulkan implements the same logical binding model, shader metadata model, and major renderer features as D3D12.
     WIP: residual parity gaps, editor integration polish, and performance balancing may remain.
     Todo: milestone acceptance, gap closure, and explicit parity proof.
6. Milestone 2 / Phase 2C: Harden parity, document residual exclusions, and close the transition.
   - What this phase brings: D3D12 and Vulkan become supported peers rather than a primary backend and a tech demo.
   - Prompt 1: run a formal parity closeout across features, outputs, and tooling.
     Positive guiderails: validate runtime rendering, asset cooking/runtime texture loading, editor viewport/overlays, GPU diagnostics, and profiling workflows on both APIs; document any intentionally deferred items explicitly.
     Negative guiderails: do not widen scope into bindless, a full render-graph rewrite, or broad platform expansion during parity closeout.
     Reasoning: parity should be a defined acceptance gate, not an open-ended aspiration.
   - Prompt 2: remove or reject remaining backend leakage outside sanctioned seams.
     Positive guiderails: verify that only backend folders and narrowly defined interop points name D3D12 or Vulkan native types; keep renderer and game/editor shared code API-neutral.
     Negative guiderails: do not leave "temporary" backend escape hatches in renderer code; do not keep compatibility shims that undermine the chosen architecture.
     Reasoning: this is where the architecture is either truly clean or merely cosmetically abstracted.
   - Phase exit snapshot:
     Completed: D3D12 and Vulkan are feature-parity backends; shared renderer code is abstraction-only; tooling and shader workflows support both APIs.
     WIP: optional future work such as async-compute execution depth, ray tracing, or bindless exploration.
     Todo: beyond-scope future investments only.

**Relevant files**
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\Interop\RenderHardwareInterface.h` - current abstraction seam to preserve while narrowing responsibilities.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\Resources\Texture.h` - currently exposes D3D12 descriptor-write semantics and must become neutral.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\Config\RenderConfig.h` - currently owns DXGI-based shared config that needs neutral format/capability ownership.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\FrameGraph\FrameGraphTextureDesc.h` - shared renderer surface still uses DXGI format types.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\GPU\CommandContext.h` - likely pivot point for backend-neutral command recording.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\FrameGraph\FrameGraph.h` - framegraph execution and resource resolution still expose D3D12-native types.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\SceneData\MaterialData.h` - material runtime state still stores D3D12 descriptor ownership.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\SceneData\Caching\MaterialCacheManager.cpp` - descriptor population is still D3D12-shaped.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\Pipeline\RenderPassPipelineTraits.h` - current runtime shader compilation call path and a key target for the precompiled workflow transition.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\D3D12\Shaders\ShaderCompileOptions.h` - current D3D12-only shader compile description.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\D3D12\Shaders\ShaderCompileResult.h` - current bytecode-only result type that needs evolution toward cooked metadata consumption.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Private\D3D12\Shaders\DxcShaderCompiler.cpp` - current runtime DXC compile path and shader-symbol emission.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Private\D3D12\D3D12Rhi.cpp` - current D3D12 device/queue/validation baseline.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Private\D3D12\D3D12DebugLayer.cpp` - current D3D12 validation and live-object baseline to extend into broader diagnostics.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Private\UI.cpp` - existing editor D3D12-only UI seam that must be abstracted during Milestone 1.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\CMakeLists.txt` - backend dependency split, tool SDK wiring, and backend-selection work starts here.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\AssetConverter\CMakeLists.txt` - shader and texture cooking boundaries need to stop depending on D3D12-only linkage assumptions.

**Verification**
1. Milestone 1 gate: shared Renderer and shared RHI headers contain no D3D12 or Vulkan native types outside sanctioned backend folders and explicit interop seams.
2. Milestone 1 gate: renderer no longer compiles shaders at runtime through pass traits and instead consumes precompiled shader artifacts plus neutral metadata.
3. Milestone 1 gate: D3D12 still renders runtime scenes and editor viewport/overlays correctly through the abstraction while exposing robust diagnostics, GPU timing, and capture markers.
4. Milestone 2 gate: the same shader source and cooked metadata pipeline produces DXIL for D3D12 and SPIR-V for Vulkan without backend-specific asset divergence by default.
5. Milestone 2 gate: Vulkan validation layers are clean enough for normal iteration, RenderDoc captures work on Vulkan, PIX remains healthy on D3D12, and any adopted Nsight workflows operate on stabilized Vulkan command paths.
6. Milestone 2 gate: parity checklist passes for scene color, depth, shadows, material textures, transient resources, viewport resize, editor overlays, and texture loading/cooking on both APIs.
7. Final architecture gate: only RHI backend folders and explicit interop seams name native D3D12 or Vulkan types; renderer, game, and most editor code remain API-neutral.

**Decisions**
- Milestone structure is now explicit: Milestone 1 is abstraction-ready D3D12-only; Milestone 2 is Vulkan implementation to D3D12 parity.
- Recommended shader timing: switch to the precompiled workflow in late Milestone 1, after neutral binding/layout/resource contracts stabilize and before serious Vulkan pipeline work begins.
- Recommended tooling timing: define the generic diagnostics/profiling/annotation API in Milestone 1; implement D3D12 support in Milestone 1 with PIX and RenderDoc-friendly behavior; implement Vulkan RenderDoc and Nsight workflows in Milestone 2 once the Vulkan command path is stable.
- Keep the renderer bindful for the parity effort; bindless is explicitly not part of the transition scope.
- Keep the existing framegraph direction and make execution backend-neutral rather than using Vulkan as a reason for a full render-graph rewrite.
- Prefer VMA as the default Vulkan allocator unless concrete residency or allocator-policy requirements later justify custom raw-Vulkan allocation.

**Further Considerations**
1. Why the precompiled shader workflow should not wait until Milestone 2:
   - If delayed, Vulkan bring-up will inherit the current runtime compilation path and D3D12-shaped assumptions, then both backends will need to be reworked again.
   - If done too early, before neutral binding/layout contracts settle, the cooked shader format may accidentally encode D3D12 root-signature concepts.
   - Best timing is after Milestone 1 has stabilized the logical binding/resource model and before Vulkan backend implementation depends on shader layout decisions.
2. PIX, RenderDoc, and Nsight timing guidance:
   - PIX should become first-class in Milestone 1 because D3D12 is the only concrete backend then and it provides the cleanest control path for abstraction debugging and performance baselining.
   - RenderDoc support should start as part of the same abstraction-level marker and naming work in Milestone 1 for D3D12, then become part of Vulkan validation in Milestone 2.
   - Nsight support is most valuable once Vulkan command recording, passes, and descriptor translation are stable enough to analyze; that makes Milestone 2 Phase 2B the best insertion point.
   - Vendor-specific crash tooling such as Aftermath should stay optional unless GPU crash triage becomes a dominant problem.
3. What Milestone 1 should feel like when done:
   - The engine still ships only on D3D12, but Vulkan no longer requires a renderer rewrite.
   - The renderer behaves as if multiple backends are possible even though only D3D12 is populated.
   - Shader assets, GPU markers, timings, and diagnostics already look like a cross-backend system rather than a D3D12-only one.
