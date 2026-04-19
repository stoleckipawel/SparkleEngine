
  6. Milestone 1 / Phase 1F: Design the neutral GPU diagnostics surface.
   - What this phase brings: diagnostics stop being a pile of D3D12 helpers and become an engine-level contract that any backend can implement.
   - Prompt 1: define the diagnostics API surface the renderer is allowed to use.
     What: specify neutral calls for resource naming, scoped GPU markers, debug annotations, timestamp queries, message routing, and optional crash or leak diagnostics.
     Why: if the renderer cannot ask for these capabilities through one neutral surface, tool support will be re-embedded directly into backend-specific code paths.
     How: extend the RHI-facing contracts with small, focused interfaces that express intent rather than tool names, and separate always-available calls from best-effort optional features.
     Where: `Engine/RHI/Public/Interop/RenderHardwareInterface.h`, `Engine/RHI/Public/Interop/RendererBackendServices.h`, and any renderer-private execution context that currently lacks a place to carry diagnostics handles.
     Guardrails: do not leak PIX, RenderDoc, InfoQueue, or DRED naming into shared renderer interfaces; do not collapse every tool concept into one giant catch-all debug object.
     Current contract shape:
     - `RenderCommandList` owns backend-neutral diagnostic scopes and single-point markers because those behaviors live at command-recording time.
     - `RenderDiagnostics` exposes focused sub-interfaces for object naming, timestamp queries, debug-message collection, and optional failure diagnostics.
     - `RenderPassContext` carries the diagnostics hub so passes can request timing or message services without seeing backend-native tool objects.
   - Prompt 2: thread diagnostics ownership through frame execution without polluting passes.
     What: decide where diagnostics state lives during frame setup, pass recording, and present, and how passes ask for markers or timers without seeing native tool objects.
     Why: the architecture only stays clean if diagnostics enter the renderer the same way resources and pipelines now do: through services, not through native handles.
     How: attach marker and timing helpers to the existing frame or command execution context, make pass code describe scopes and labels only, and keep backend translation at the command-list boundary.
     Where: renderer-private frame execution and pass binding code under `Engine/Renderer/Private`, especially execution context plumbing that already touches `RenderCommandList` and per-frame runtime services.
     Guardrails: do not let individual passes call PIX or RenderDoc APIs directly; do not make editor UI code the owner of general renderer diagnostics.
     Current ownership shape:
     - `Renderer::RecordFrame()` creates one renderer-private `FrameExecutionDiagnostics` state per recorded frame from the neutral RHI diagnostics hub.
     - `FrameGraph::Execute()` wraps each compiled pass in automatic marker and timer scopes using the compiled pass `eventScopeLabel` and `displayLabel`, so authored passes inherit stable outer diagnostics without extra boilerplate.
     - `RenderGraphPassContext` carries a lightweight `PassExecutionDiagnostics` helper for optional nested labels or timers, while backend translation still happens only through `CommandContext` and `RenderCommandList`.
   - Prompt 3: define capture, crash, and message-routing boundaries.
     What: split responsibility for frame capture markers, runtime debug message collection, live-object reporting, and optional crash diagnostics.
     Why: these concerns live at different lifetimes, and mixing them into one path makes backend implementations hard to reason about and test.
     How: treat capture markers as per-command-recording behavior, message routing as device or frame lifecycle behavior, and crash or live-object diagnostics as backend shutdown or failure-reporting services.
     Where: `Engine/RHI/Private/D3D12/D3D12Rhi.cpp`, `Engine/RHI/Private/D3D12/D3D12DebugLayer.cpp`, `Engine/Editor/Private/UI.cpp`, plus the neutral RHI interfaces that expose these concepts.
     Guardrails: do not assume one tool handles all three jobs; do not make crash diagnostics mandatory for the initial abstraction if the backend support is still partial.
     Current boundary shape:
     - `RenderCommandList` remains the only renderer-facing place that can eventually emit capture-friendly markers or nested event scopes, because capture tooling lives at command recording time.
     - `D3D12DebugLayer` owns InfoQueue configuration, D3D12-message translation, and any best-effort failure artifacts, while `D3D12Rhi` decides when those services are initialized, drained, or reported during device lifetime and shutdown.
     - `D3D12PipelineState` and similar backend failure paths consume translated backend diagnostics through `D3D12Rhi` instead of reading `ID3D12InfoQueue` directly.
     - `UI.cpp` stays a narrow editor host seam that submits ImGui work through native handles, not the owner of general renderer diagnostics or failure reporting.
   - Phase exit snapshot:
     Completed: the engine has a backend-neutral diagnostics contract and a clean ownership model for markers, timers, messages, and capture hooks.
     WIP: D3D12 still needs to implement the contract end to end.
     Todo: D3D12 mapping, validation, and milestone hardening.
7. Milestone 1 / Phase 1G: Wire D3D12 diagnostics, timing, and capture support.
   - What this phase brings: D3D12 becomes the fully instrumented control backend used to debug both the abstraction and the later Vulkan bring-up.
   - Prompt 1: map the neutral diagnostics API onto D3D12 tooling.
     What: implement naming, markers, debug messages, live-object reporting, and optional crash diagnostics using the new neutral surface.
     Why: this is the proof that the abstraction is strong enough to host real tooling instead of toy wrapper calls.
     How: translate neutral marker scopes into PIX-friendly and RenderDoc-friendly annotations, wire debug messages through the InfoQueue path, and expose live-object or DRED-style reports through backend shutdown or failure hooks.
     Where: `Engine/RHI/Private/D3D12/D3D12Rhi.cpp`, `Engine/RHI/Private/D3D12/D3D12DebugLayer.cpp`, `Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp`.
     Guardrails: do not make PIX the only supported annotation path; do not reintroduce direct D3D12 debug calls into renderer passes just because the backend implementation exists.
  Current D3D12 mapping shape:
  - Object naming stays on the neutral object-diagnostics path and maps directly to `ID3D12Object::SetName(...)` for device, queue, command list, resource, and owned allocation objects.
  - Command-list scopes and markers route through an optional WinPixEventRuntime ABI loader inside the D3D12 RHI, so renderer code still speaks only `RenderCommandList` while backend captures get PIX-compatible GPU events when the runtime DLL is present.
  - Debug messages continue to drain from `ID3D12InfoQueue`, but they are translated into neutral `RhiDiagnosticMessage` records before backend failure paths consume them.
  - Live-object reporting stays a shutdown-time backend concern, while optional crash diagnostics use DRED enablement plus post-removal breadcrumb and page-fault harvesting behind `RenderFailureDiagnostics`.
   - Prompt 2: add timestamp-query based GPU timing.
     What: implement per-scope or per-pass GPU timing that the renderer can request through the neutral diagnostics API.
     Why: timing is one of the main reasons to build a real diagnostics surface early; it gives a trustworthy D3D12 baseline before Vulkan arrives.
     How: allocate timestamp query resources backend-side, associate them with labeled scopes, resolve them after execution, and publish results in a form the engine can log or display without depending on backend-native types.
     Where: `Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp`, device and frame-management code under `Engine/RHI/Private/D3D12`, and whichever renderer-private frame coordinator consumes diagnostics results.
     Guardrails: do not bury timing results in tool-only viewers; do not make pass authors manage raw query heaps or resolve buffers.
  Current D3D12 timing shape:
  - `D3D12RenderTimingDiagnostics` now owns a fixed per-frame timestamp query pool plus persistently mapped readback buffers, so each neutral query handle stays backend-private while still resolving to CPU-visible ticks after GPU completion.
  - Each timestamp write immediately records both `EndQuery(...)` and `ResolveQueryData(...)` on the active D3D12 command list, which keeps resolve work inside normal command recording instead of introducing a second backend-only finalize pass.
  - `Renderer` keeps one `FrameExecutionDiagnostics` object per frame-in-flight slot, resolves the completed slot at the next `BeginFrame()`, and publishes labeled scope timings through the engine logger without exposing D3D12 query heaps or readback resources to passes.
   - Prompt 3: validate runtime, editor, and capture workflows on D3D12.
     What: prove that runtime scenes, editor viewport rendering, markers, naming, timings, and captures all behave correctly through the abstraction.
     Why: Milestone 1 is not done when the API exists; it is done when the D3D12 control path is debuggable and observable without architectural leakage.
     How: run smoke scenes, editor overlay rendering, capture sessions, and error-path tests; confirm that labels show up in tools, timings are sane, and diagnostic messages preserve enough context to debug failures.
     Where: `Engine/Editor/Private/UI.cpp`, `Projects/Showcase`, D3D12 RHI diagnostics files, and whichever test or smoke validation paths are used by the current build flow.
     Guardrails: do not declare success based only on compilation; do not rely on one sample frame if resize, overlay, or failure paths are untested.
   - Phase exit snapshot:
     Completed: D3D12 has first-class diagnostics, timing, and capture support behind the neutral API.
     WIP: milestone hardening and explicit gate coverage may still be incomplete.
     Todo: close Milestone 1 with regression checks and an architecture audit.
8. Milestone 1 / Phase 1H: Prove the D3D12 control backend is production-ready.
   - What this phase brings: Milestone 1 stops being a code-change milestone and becomes an evidence-backed acceptance gate.
   - Prompt 1: add shader artifact regression checks.
     What: verify cooked shader generation, stale-artifact detection, and pipeline creation from cooked metadata.
     Why: the shader ABI is now a core engine contract and needs regression coverage before Vulkan depends on it.
     How: add validation around artifact generation, loading, and compatibility checks, and make build or smoke flows fail when cooked shader expectations drift.
     Where: `Tools/ShaderCompiler/CMakeLists.txt`, `Scripts/CookAllAssets.bat`, runtime shader-loading code, and milestone validation scripts or project smoke targets.
     Guardrails: do not test only artifact existence; also test that the runtime consumes the artifacts successfully.
     Acceptance gate shape:
     - The public cook path must deterministically emit or validate every shader package declared by the engine and active project manifests, rather than relying on ad hoc developer-only commands.
     - The regression path must cover both positive and negative cases: fresh cooked artifacts succeed, missing artifacts fail, stale compatibility identifiers fail, and the runtime never falls back to source compilation.
     - At least one graphics PSO path and one compute PSO path must be exercised from cooked metadata during the same gate, so the check proves runtime consumption instead of package presence alone.
     - Failure output must identify the logical `packageId`, `variantId`, required stage mask, and whichever compatibility field drifted, so shader ABI regressions are actionable instead of generic startup failures.
   - Prompt 2: add D3D12 diagnostics smoke validation.
     What: validate markers, timing scopes, debug messages, and capture-friendly labeling in a repeatable smoke workflow.
     Why: tooling regressions are expensive to rediscover during Vulkan bring-up when two backends are already in flight.
     How: run a repeatable scene and editor validation path, assert that expected markers and timing scopes appear, and preserve logs or capture instructions for future debugging.
     Where: `Projects/Showcase`, editor startup paths, D3D12 diagnostics files, and any automated or semi-automated smoke harness currently used by the repo.
     Guardrails: do not reduce validation to "tool opened successfully"; verify that the right labels, timings, and messages are actually exposed.
     Acceptance gate shape:
     - The repeatable runtime path must load a representative scene, run for a bounded multi-frame window, and prove that diagnostics capabilities, object naming, and timing resolution are alive on the D3D12 control backend.
     - The repeatable editor path must prove viewport plus overlay rendering through the sanctioned host seam and must exercise at least one resize- or window-state-sensitive path instead of validating only a static first frame.
     - Evidence must distinguish three states explicitly: feature observed, feature unsupported in the local environment, and feature regressed. Missing PIX runtime, RenderDoc, or other external tooling is an environment limitation, not a silent pass.
     - The stored evidence package must preserve exact launch instructions, expected log or capture signatures, and the conditions that should fail the phase, so future debugging does not depend on oral history.
   - Prompt 3: run the final architecture audit for Milestone 1.
     What: confirm that renderer, application, game framework, and editor public surfaces remain API-neutral and that only sanctioned seams name native D3D12 types.
     Why: this audit is the last defense against declaring Milestone 1 done while hidden backdoors still exist.
     How: scan public and shared code, inspect interop seams deliberately, and document every remaining native reference as either sanctioned or backlog work.
     Where: shared headers under `Engine/Renderer/Public`, `Engine/RHI/Public/Interop`, `Engine/Application`, `Engine/GameFramework`, and editor public surfaces.
     Guardrails: do not excuse a shared-surface leak just because D3D12 is still the only backend; do not keep temporary compatibility shims that will undermine Vulkan parity.
     Acceptance gate shape:
     - Audit scope is every shared or public surface named in `Where`, plus any sanctioned interop seam touched by editor startup or renderer bootstrap during Milestone 1 closeout.
     - Every remaining native reference must be classified as one of: sanctioned seam, removable leak, or explicit deferred backlog item with written justification. Unclassified references do not pass the gate.
     - Sanctioned seams must stay narrow: backend-private RHI implementation, neutral native-handle bridges already modeled by the abstraction, and the editor host seam that must submit ImGui work through native handles.
     - The audit result must be written down as an inventory, not a verbal claim, so Milestone 1 sign-off leaves a stable record before Vulkan implementation begins.
   - Phase acceptance package:
     - A shader regression command or target that can fail CI or local milestone validation when cooked shader expectations drift.
     - Runtime and editor smoke invocations plus preserved evidence showing diagnostics capabilities, timing visibility, and any environment-specific tooling limitations.
     - An architecture audit inventory listing sanctioned seams, invalid leaks, and any deferred backlog items with explicit justification.
   - Phase exit snapshot:
     Completed: D3D12 is fully hosted behind the abstraction, consumes cooked shaders, exposes diagnostics through neutral contracts, and passes milestone acceptance checks.
     WIP: none inside Milestone 1 beyond explicitly documented environment limitations or deferred backlog items recorded by the acceptance package.
     Todo: Vulkan implementation against the stabilized architecture.
9. Milestone 2 / Phase 2A: Stand up Vulkan build, loader, and backend bootstrap.
   - What this phase brings: Vulkan becomes a real backend target in the build and startup pipeline instead of a placeholder enum.
   - Prompt 1: wire Vulkan dependencies and backend selection.
     What: add Vulkan SDK linkage, allocator dependency wiring, and backend-selection plumbing so the build can produce a Vulkan-capable RHI path.
     Why: without deterministic build ownership, every later Vulkan prompt becomes environment-specific and hard to reproduce.
     How: update module CMake files, dependency discovery, and backend bootstrap selection while keeping D3D12 as the default control backend until parity grows.
     Where: `Engine/RHI/CMakeLists.txt`, top-level engine CMake wiring, and backend-creation logic reachable from `Engine/RHI/Public/Interop/RendererBackendServices.h`.
     Guardrails: do not broaden scope into cross-platform packaging yet; do not force renderer code to branch on backend availability.
   - Prompt 2: create Vulkan instance, device, queue, and present bootstrap.
     What: initialize the Vulkan instance, choose the physical device, create the logical device and graphics queue, and bring up the Windows presentation path.
     Why: this is the minimum boot sequence needed before any meaningful resource or command work can be validated.
     How: keep the bootstrap layered behind the existing RHI creation flow, expose only neutral capability and handle concepts upward, and prefer a simple graphics-first queue model initially.
     Where: new Vulkan backend code under `Engine/RHI/Private`, plus the backend instantiation path behind the existing renderer-facing services.
     Guardrails: do not change shared abstractions unless a proven mismatch exists; do not optimize for async compute or Linux before the first frame can present.
   - Prompt 3: enable Vulkan validation and early naming from day one.
     What: hook validation layers, debug names, and basic debug labels into the new Vulkan backend as soon as it boots.
     Why: early validation keeps backend bugs local and reduces the odds of debugging the renderer when the real fault is in backend bring-up.
     How: map the neutral diagnostics naming and marker surfaces onto Vulkan debug utilities immediately, even before full pass coverage exists.
     Where: Vulkan backend bootstrap and diagnostics files added under `Engine/RHI/Private`, plus the neutral diagnostics interfaces already created in Milestone 1.
     Guardrails: do not postpone validation until later parity phases; do not add Vulkan-specific marker calls in renderer code.
   - Phase exit snapshot:
     Completed: Vulkan can build, initialize, create a device, and present a minimal frame while exposing validation and basic naming hooks.
     WIP: memory ownership, command submission depth, and shader or pipeline translation are still incomplete.
     Todo: resource lifetime, submission, and shader translation.
10. Milestone 2 / Phase 2B: Implement Vulkan memory ownership and frame submission.
   - What this phase brings: Vulkan stops being a boot stub and starts owning the same resource lifetime responsibilities D3D12 already carries.
   - Prompt 1: implement allocator-backed buffer and image lifetime management.
     What: add Vulkan buffer, image, staging, and allocation ownership behind the neutral resource creation APIs.
     Why: without real lifetime management, later framegraph, texture, and transient-resource work will be faked or duplicated.
     How: prefer VMA-backed allocation unless a real engine requirement disproves it, keep upload and default-resource patterns expressible through the neutral resource descriptors, and preserve clear ownership for destruction and deferred release.
     Where: new Vulkan resource and allocator files under `Engine/RHI/Private`, plus resource creation entry points behind `RenderHardwareInterface`.
     Guardrails: do not push Vulkan allocation objects into shared renderer code; do not bypass the neutral descriptors just to get one resource type working.
   - Prompt 2: implement command buffers, synchronization, and frame pacing.
     What: add Vulkan command recording, submission, fences, semaphores, and frame-lifetime pacing behind the existing execution seam.
     Why: the renderer already thinks in frame execution and command recording terms; Vulkan must satisfy that contract instead of forcing a renderer rewrite.
     How: translate the existing command-list abstraction into Vulkan command-buffer lifetimes, queue submission, and frame synchronization objects, starting with the single graphics-queue path.
     Where: Vulkan backend execution files under `Engine/RHI/Private`, plus RHI services that currently own begin-frame, submit, and present sequencing.
     Guardrails: do not introduce renderer-visible Vulkan submission objects; do not overbuild async or multi-queue support before the single-queue model is stable.
   - Prompt 3: map neutral resource-state expectations onto Vulkan barriers.
     What: implement the barrier and synchronization translation that turns neutral resource-state transitions and transient-resource expectations into Vulkan synchronization primitives.
     Why: this is the point where the existing framegraph and renderer resource model proves whether the abstraction was designed correctly.
     How: translate state transitions at the backend boundary, keep aliasing or transient ownership rules consistent with the current framegraph design, and document any real abstraction mismatch instead of papering over it in renderer code.
     Where: Vulkan backend resource-state translation code under `Engine/RHI/Private`, and the backend-facing resource transition entry points already used by the renderer.
     Guardrails: do not add Vulkan-only branches in framegraph logic; do not fall back to overly conservative synchronization everywhere without documenting the cost.
   - Phase exit snapshot:
     Completed: Vulkan owns real buffer and image lifetime, command submission, frame pacing, and resource-state translation.
     WIP: cooked shader consumption, binding translation, and full renderer-pass support are still pending.
     Todo: SPIR-V output and Vulkan pipeline or descriptor translation.
11. Milestone 2 / Phase 2C: Extend the cooker to emit SPIR-V from the same shader source pipeline.
   - What this phase brings: the shader asset contract becomes genuinely dual-backend instead of being a DXIL format with Vulkan aspirations.
   - Prompt 1: emit SPIR-V alongside DXIL from the same logical shader definition.
     What: extend the offline shader pipeline so one source definition can produce both DXIL and SPIR-V payloads under the same cooked shader identity.
     Why: if Vulkan needs a different authoring path, the asset system has already failed its central purpose.
     How: reuse the cooked shader schema from Milestone 1, add backend-specific binary payload slots, and keep shared metadata identical unless a true backend divergence must be modeled explicitly.
    Where: shader-cooking code under `Tools/ShaderCompiler/Private/Cooking`, `Tools/ShaderCompiler/CMakeLists.txt`, and the public shader artifact definitions under `Engine/RHI/Public/Shaders`.
     Guardrails: do not fork source-authoring conventions between D3D12 and Vulkan; do not duplicate metadata blocks just because the binary payloads differ.
   - Prompt 2: validate metadata compatibility across both binary outputs.
     What: prove that the same logical binding metadata describes both DXIL and SPIR-V payloads accurately enough for backend pipeline creation.
     Why: this is the core claim of the neutral shader ABI and must be validated before descriptor or pipeline translation goes deeper.
     How: compare stage expectations, binding counts, specialization hooks, and debug identity between both outputs and tighten the schema if a real mismatch appears.
    Where: shader artifact definitions under `Engine/RHI/Public/Shaders`, cooking code under `Tools/ShaderCompiler`, and backend loaders that validate cooked package compatibility.
     Guardrails: do not let one backend silently ignore metadata fields that the other backend requires; do not rely on backend runtime reflection to patch missing schema information.
   - Prompt 3: define how feature toggles and specialization values flow through cooked shaders.
     What: formalize how backend-neutral feature switches become specialization constants, permutations, or cooked variants.
     Why: if this remains ad hoc, Vulkan will grow a separate shader-variation system and the cross-backend asset model will fracture.
     How: keep the decision at the cooked-asset layer, define explicit identifiers for variants or specialization inputs, and make both backends consume the same logical selection model.
     Where: `Engine/RHI/Public/Shaders/ShaderCompileOptions.h`, shader artifact metadata, and the cooking toolchain.
     Guardrails: do not encode backend-specific specialization mechanisms directly into renderer-facing types; do not let pass code choose variants using D3D12- or Vulkan-specific names.
   - Phase exit snapshot:
     Completed: the cooker can emit DXIL and SPIR-V from the same source definition, and the cooked metadata contract remains shared.
     WIP: Vulkan still needs to consume the cooked data through descriptors and pipelines.
     Todo: descriptor translation, pipeline layout creation, and pass coverage.
12. Milestone 2 / Phase 2D: Translate cooked bindings and pipelines into Vulkan objects.
   - What this phase brings: Vulkan can finally consume the same logical shader and binding model already proven on D3D12.
   - Prompt 1: translate logical binding layouts into Vulkan descriptor set and pipeline layouts.
     What: map the neutral binding layout model to Vulkan descriptor set layouts, pipeline layouts, and any supporting descriptor allocation policy.
     Why: this is where the bindful abstraction either proves portable or reveals hidden D3D12 assumptions.
     How: keep logical descriptor-table handles as the renderer-facing concept, translate them entirely inside the backend, and preserve clear ownership of descriptor pools and set layouts.
     Where: new Vulkan descriptor and pipeline-layout code under `Engine/RHI/Private`, plus the shared binding-layout abstractions already consumed by the renderer.
     Guardrails: do not invent a Vulkan-only renderer-visible descriptor-set model; do not reshape material layouts around backend-native terminology.
   - Prompt 2: implement Vulkan descriptor allocation, updates, and runtime binding.
     What: allocate descriptor pools or sets, populate them from neutral resource-view data, and bind them during command recording behind the existing renderer execution seam.
     Why: shader modules alone are not enough; the runtime binding path must also prove that the existing logical descriptor ownership model works on Vulkan.
     How: translate descriptor-table handles and resource views into Vulkan descriptors, manage lifetime backend-side, and keep per-frame or persistent descriptor strategy hidden from renderer code.
     Where: Vulkan descriptor backend files under `Engine/RHI/Private`, plus renderer-facing RHI calls that already allocate or bind logical descriptor tables.
     Guardrails: do not expose descriptor set handles upward for convenience; do not bypass material cache ownership rules already established for the bindful model.
   - Prompt 3: implement Vulkan raster and compute pipeline creation from cooked metadata.
     What: create Vulkan graphics and compute pipelines from the neutral render-state descriptors and cooked shader binaries.
     Why: once this works, Vulkan is no longer blocked on shader translation and can start carrying real pass coverage.
     How: consume the same neutral pipeline-state descriptors already used by D3D12, translate them into Vulkan create-info structures backend-side, and keep caching keyed on logical shader plus state identities.
     Where: Vulkan pipeline backend files under `Engine/RHI/Private`, and the shared pipeline-state abstractions already referenced by `Engine/Renderer/Private/Pipeline/PipelineStateManager.h`.
     Guardrails: do not add Vulkan-only pipeline description types to shared renderer code; do not shortcut one pass by building raw Vulkan pipelines directly inside renderer code.
   - Phase exit snapshot:
     Completed: Vulkan can translate logical bindings, descriptors, and pipeline state from the shared cooked metadata model.
     WIP: renderer pass coverage and editor or tooling parity are still incomplete.
     Todo: vertical-slice renderer bring-up.
13. Milestone 2 / Phase 2E: Port renderer features in vertical slices.
   - What this phase brings: Vulkan becomes visually meaningful and debuggable feature by feature instead of through one risky parity push.
   - Prompt 1: bring up the basic frame slice first.
     What: validate clear, present, viewport output, and the minimal framegraph path needed to display a stable frame.
     Why: a stable frame loop is the foundation for every later rendering feature and tool capture.
     How: start with the smallest viable pass set, validate resize and presentation behavior, and make sure the framegraph can materialize the required resources through the backend without renderer-side Vulkan branches.
     Where: renderer-private frame execution and framegraph code under `Engine/Renderer/Private`, plus Vulkan backend resource and present-path implementations.
     Guardrails: do not jump straight to full scene rendering; do not accept a basic frame path that bypasses the real framegraph or present abstractions.
   - Prompt 2: port shadowed opaque rendering and material texture sampling.
     What: bring up the core scene slice that proves meshes, materials, textures, samplers, depth, and shadows all work through Vulkan.
     Why: this is the smallest slice that exercises most of the architecture decisions made in Milestone 1.
     How: port the forward opaque path, shadow path, texture bindings, and fallback material behavior incrementally, checking results against the D3D12 control backend after each slice.
     Where: renderer-private pass code, material caching and scene-data utilities under `Engine/Renderer/Private`, and Vulkan backend binding or resource code.
     Guardrails: do not validate only by "it draws something"; verify that bindings, shadow maps, fallback textures, and depth behavior match D3D12 intent.
   - Prompt 3: port transient-resource and resize-sensitive paths.
     What: validate imported resources, transient allocations, resize handling, and any compute clear or utility passes that depend on those flows.
     Why: these paths are usually where backend abstraction mistakes surface after the obvious geometry path already works.
     How: exercise aliasing-sensitive framegraph flows, resize events, and transient attachment recreation while watching validation and capture tools for hidden hazards.
     Where: `Engine/Renderer/Private/FrameGraph`, backend transient-resource and barrier code, and viewport or resize orchestration paths.
     Guardrails: do not postpone transient-resource validation until the final milestone gate; do not patch over resize bugs with renderer-visible Vulkan special cases.
   - Phase exit snapshot:
     Completed: Vulkan renders real scene content with materials, textures, shadows, and transient resources through the shared renderer path.
     WIP: editor integration, texture cooking parity, profiling depth, and residual pass gaps may still remain.
     Todo: tooling and asset-pipeline parity.
14. Milestone 2 / Phase 2F: Close editor, tooling, and asset-pipeline parity gaps.
   - What this phase brings: Vulkan stops being runtime-only and becomes usable in the same day-to-day workflows as D3D12.
   - Prompt 1: validate editor viewport and overlay integration on Vulkan.
     What: make the editor host path, viewport rendering, and overlays behave correctly when Vulkan is the active backend.
     Why: parity is incomplete if Vulkan only works in runtime builds but not in the main engine workflow used for iteration.
     How: keep editor-specific native interop isolated to the sanctioned host seam, verify overlay ordering and resize behavior, and avoid teaching the editor about backend-native submission details.
     Where: `Engine/Editor/Private/UI.cpp`, editor application startup, and the backend present or host-integration path.
     Guardrails: do not expand editor public interfaces with backend-specific methods; do not let the editor bypass the RHI abstraction because host integration is awkward.
   - Prompt 2: close texture cooking and runtime asset loading parity.
     What: ensure texture cooking, cooked texture loading, and runtime format mapping behave consistently across D3D12 and Vulkan.
     Why: shader parity is not enough if cooked textures or format assumptions still encode one backend's worldview.
     How: remove backend-specific cooking assumptions where needed, validate KTX or DDS paths against both backends, and keep runtime format interpretation driven by neutral format descriptions.
     Where: `Tools/AssetConverter/Private/Cooking/KtxTextureCooker.cpp`, `Engine/RHI/Private/D3D12/Textures`, and the future Vulkan texture-loading path under `Engine/RHI/Private`.
     Guardrails: do not let the cooker stay permanently DXGI-shaped if Vulkan must consume the same assets; do not fork texture asset formats per backend without a hard requirement.
   - Prompt 3: add Vulkan profiling and external tool workflows.
     What: validate RenderDoc capture quality, timestamp queries, debug labels, and Nsight workflows once command recording is stable.
     Why: these tools are how parity bugs and performance gaps will actually be closed in practice.
     How: map the existing neutral diagnostics surface onto Vulkan tooling, verify label and timing quality in captures, and keep vendor-specific extras optional unless real crash triage demands them.
     Where: Vulkan diagnostics backend code under `Engine/RHI/Private`, plus the neutral diagnostics interfaces introduced in Milestone 1.
     Guardrails: do not import PIX concepts into Vulkan; do not add Nsight-specific calls to shared renderer code.
   - Phase exit snapshot:
     Completed: Vulkan participates in editor workflows, texture asset flows, and practical profiling or capture workflows.
     WIP: only formal parity proof and final leak cleanup remain.
     Todo: close the transition with a documented acceptance pass.
15. Milestone 2 / Phase 2G: Run the parity closeout and lock the architecture.
   - What this phase brings: D3D12 and Vulkan become supported peer backends instead of a control path and a tech demo.
   - Prompt 1: run a formal parity matrix.
     What: verify scene output, shadows, textures, transient resources, resize handling, editor overlays, shader loading, diagnostics, and capture workflows on both backends.
     Why: parity should be an explicit acceptance gate, not an assumption based on a few representative screenshots.
     How: compare the D3D12 control backend against Vulkan using a fixed checklist, record any deliberate exclusions, and require each exclusion to be written down rather than implied.
     Where: project smoke scenes, editor workflows, cooked asset flows, and both backend implementations under `Engine/RHI/Private`.
     Guardrails: do not widen scope into bindless, a render-graph rewrite, or broad platform expansion during closeout.
   - Prompt 2: remove or reject the last backend leaks outside sanctioned seams.
     What: scan the repo for remaining native-type references and eliminate anything that escaped backend folders or explicit interop boundaries.
     Why: if this final audit is skipped, the architecture may look abstracted while still carrying hidden coupling that will break future features.
     How: perform a deliberate repo-wide audit, classify remaining native references as sanctioned or invalid, and fix invalid cases before sign-off.
     Where: shared renderer, shared RHI interop, application, game framework, and editor public surfaces.
     Guardrails: do not leave "temporary" escape hatches in place after parity; do not keep compatibility layers that undermine the chosen architecture.
   - Prompt 3: document the post-transition roadmap.
     What: close the plan by naming what is done, what is intentionally deferred, and what future renderer work is now safe to consider.
     Why: this prevents the parity milestone from dissolving into an unbounded backlog and helps future work build on the stabilized design.
     How: separate parity-complete work from optional future investments such as async compute depth, ray tracing, or bindless exploration.
     Where: this plan, supporting architecture docs, and any follow-up backlog or roadmap documents.
     Guardrails: do not relabel deferred stretch goals as hidden parity requirements; do not imply that future work should reopen the abstraction that just stabilized.
   - Phase exit snapshot:
     Completed: D3D12 and Vulkan are feature-parity backends; shared renderer code is abstraction-only; tooling and shader workflows support both APIs.
     WIP: optional future work only.
     Todo: beyond-scope investments such as async compute depth, ray tracing, bindless, or broader platform expansion.

**Relevant files**
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\Interop\RenderHardwareInterface.h` - central abstraction seam for shader loading, diagnostics, command recording, and backend capabilities.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\Interop\RendererBackendServices.h` - renderer-facing bootstrap and backend orchestration seam.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\Pipeline\RenderPassPipelineTraits.h` - current runtime shader compilation call path and the main runtime entrypoint that must switch to cooked shader consumption.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\Passes\ShaderSourceDefinition.h` - current pass-to-shader source definition contract that must stop implying runtime compilation.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\Shaders\ShaderStage.h` - shared stage and stage-mask surface used by both compile-time and cooked-shader metadata.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\Shaders\CookedShaderPackage.h` - backend-neutral cooked shader package schema for stage binaries, logical binding metadata, specialization inputs, and compatibility identifiers.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\Shaders\ShaderCompileOptions.h` - public compile description that should evolve into offline shader-cooking input and variant metadata.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\Shaders\ShaderCompileResult.h` - public compile result contract that should evolve into cooked artifact metadata rather than a runtime compile return type.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\ShaderCompiler\Private\Compiler\DxcShaderCompiler.h` - tool-owned DXC wrapper used only by the standalone shader cook path.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\ShaderCompiler\Private\Compiler\DxcShaderCompiler.cpp` - tool-owned DXC compile path that stays outside normal runtime execution.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Core\Public\Diagnostics\Logger.h` - public logger registry surface for the chosen central-registry plus native `spdlog` access model.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Core\Public\Diagnostics\Verify.h` - surviving fatal and HRESULT diagnostics seam after the legacy logging facade removal.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Core\Private\Diagnostics\Logger.cpp` - SparkleCore-owned bootstrap, shared sink configuration, and named logger lifetime.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Core\Private\Diagnostics\Verify.cpp` - fatal, HRESULT, and debugger-break behavior that now stands apart from the deleted logger facade.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Core\CMakeLists.txt` - SparkleCore linkage point for the new logging dependency.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\CMake\Dependencies\FetchDependencies.cmake` - existing dependency fetch pipeline where spdlog should be pinned and added.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Private\D3D12\D3D12RenderHardwareInterface.cpp` - D3D12 backend translation point for cooked shader loading, diagnostics, timing, and backend service wiring.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Private\D3D12\D3D12Rhi.cpp` - D3D12 device and backend bootstrap path, including debug and diagnostics ownership.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Private\D3D12\D3D12DebugLayer.cpp` - D3D12 validation, InfoQueue, and live-object baseline to extend into the new diagnostics surface.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Private\UI.cpp` - sanctioned editor host seam where backend-specific present or UI integration behavior must stay isolated.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\CMakeLists.txt` - backend dependency wiring, shader tool linkage decisions, and future Vulkan module integration start here.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\AssetConverter\CMakeLists.txt` - asset cooking entrypoints for scene and texture conversion.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\ShaderCompiler\CMakeLists.txt` - standalone shader-cooking entrypoints and offline shader artifact ownership.
- `c:\Users\stole\Documents\GitHub\SparkleEngine\Tools\AssetConverter\Private\Cooking\KtxTextureCooker.cpp` - current texture cooking path that still needs neutrality work before full dual-backend asset parity.

**Verification**
1. Phase 1C gate: the current runtime shader path has been inventoried fully enough that the cooked shader schema contains every field required for runtime consumption.
2. Phase 1D gate: renderer startup no longer compiles pass shaders during normal execution; D3D12 consumes cooked shader artifacts plus neutral metadata.
3. Phase 1E gate: `Log.h`, `Log.cpp`, and the legacy custom logging internals are removed; engine and tool code use the chosen spdlog access model consistently; and any surviving verify or HRESULT helpers no longer depend on the deleted logging API.
4. Phase 1F and 1G gate: the renderer can request names, markers, timings, and debug messages through a backend-neutral API, and D3D12 implements that API with real tool-visible results.
5. Milestone 1 gate: D3D12 still renders runtime scenes and editor viewport or overlays correctly through the abstraction while exposing robust diagnostics, GPU timing, and capture markers.
6. Phase 2A and 2B gate: Vulkan can initialize, allocate resources, record and submit work, present, and expose validation or naming hooks without changing shared renderer code.
7. Phase 2C and 2D gate: the same shader source and cooked metadata pipeline produces DXIL for D3D12 and SPIR-V for Vulkan without backend-specific asset divergence by default.
8. Phase 2E and 2F gate: parity checklist passes for scene color, depth, shadows, material textures, transient resources, viewport resize, editor overlays, and texture loading or cooking on both APIs.
9. Final architecture gate: only RHI backend folders and explicit interop seams name native D3D12 or Vulkan types; renderer, game, and most editor code remain API-neutral.

**Decisions**
- Milestone structure is now explicit: Milestone 1 is abstraction-ready D3D12-only; Milestone 2 is Vulkan implementation to D3D12 parity.
- The new breakdown separates contract-definition phases from implementation phases on purpose: first define the shader, logging, or diagnostics ABI, then prove it on D3D12, then consume it from Vulkan.
- Recommended shader timing: switch to the precompiled workflow in late Milestone 1, after neutral binding and resource contracts stabilize and before serious Vulkan pipeline work begins.
- Recommended logging timing: replace the custom logger immediately after the shader compiler cutover, delete `Log.h` and `Log.cpp` as part of that phase, and settle the permanent spdlog access model before diagnostics surface work so later backend diagnostics build on the final route instead of a temporary bridge.
- Recommended tooling timing: define the generic diagnostics and profiling API only after the spdlog migration stabilizes repo-wide message routing; implement D3D12 support in Milestone 1 with PIX and RenderDoc-friendly behavior; implement Vulkan RenderDoc and Nsight workflows only after Vulkan command recording is stable.
- Keep the renderer bindful for the parity effort; bindless is explicitly not part of the transition scope.
- Keep the existing framegraph direction and make execution backend-neutral rather than using Vulkan as a reason for a full render-graph rewrite.
- Prefer VMA as the default Vulkan allocator unless concrete residency or allocator-policy requirements later justify custom raw-Vulkan allocation.

**Why This Sequence Matters**
1. Why shader work comes before Vulkan pipeline work:
   - If Vulkan bring-up starts before the cooked shader ABI exists, the new backend will inherit the old runtime-compile model and both backends will need to be cleaned up later.
   - Doing the shader contract too early would also be risky, which is why the plan waits until Phases 1A and 1B stabilize the logical binding and command model first.
2. Why logging replacement comes before diagnostics design:
  - GPU diagnostics, validation, shader loading failures, and capture workflows all need a stable repo-wide message route before backend-specific tooling work starts.
  - Replacing the logger after the diagnostics API lands would churn error routing, sink behavior, and fatal handling across the exact milestone that is supposed to make tooling trustworthy.
3. Why diagnostics are split into design first and implementation second:
   - A neutral diagnostics API is architecture work.
   - PIX, InfoQueue, DRED, RenderDoc labels, and GPU timestamps are backend work.
   - Combining both in one step makes it harder to see whether a bug comes from a bad abstraction or a bad D3D12 implementation.
4. Why Vulkan is brought up in layers instead of in one big phase:
   - Boot, memory, submission, shaders, descriptors, pipelines, passes, editor workflows, and parity proof fail for different reasons and require different debugging tools.
   - Small phases keep defects local and make it much clearer which abstraction assumption was wrong if something breaks.
5. Why vertical slices matter before the final parity audit:
   - A backend that can boot and compile is still not a renderer backend.
   - Basic frame output, shadowed opaque rendering, texture sampling, transients, resize, and editor overlays together exercise the important architectural seams much better than a late all-at-once parity test.

**ASCII Container Architecture / System Design**
```text
+---------------------------+        +----------------------------------+
| Source Assets             |        | Tools/ShaderCompiler            |
| - HLSL shader sources     | -----> | - validates shader manifests    |
| - textures and materials  |        | - cooks shader artifacts        |
| - scene content           |        | - emits DXIL now, SPIR-V next   |
+-------------+-------------+        +----------------+-----------------+
        |                                       |
        |                                       v
        |        +------------------------------+----------------+
        | -----> | Tools/AssetConverter                          |
        |        | - cooks scenes and textures                   |
        |        +------------------------------+----------------+
        |                                       |
        |                                       v
        |                         +-------------------------------+
        |                         | Cooked Content                |
        |                         | - scene data                  |
        |                         | - texture payloads            |
        +-----------------------> | - shader binaries            |
                                  | - neutral shader metadata     |
                                  +---------------+---------------+
                                                        |
                                                        v
                              +------------------------------------------------+
                              | Project Host                                    |
                              | - ShowcaseRuntime                               |
                              | - ShowcaseEditor                                |
                              +----------------------+-------------------------+
                                                     |
                                                     v
                              +------------------------------------------------+
                              | SparkleApplication                              |
                              | - startup                                       |
                              | - project and asset resolution                  |
                              | - backend selection                             |
                              +----------------------+-------------------------+
                                                     |
                           +-------------------------+------------------------+
                           |                                                  |
                           v                                                  v
          +--------------------------------------+         +--------------------------------+
          | SparkleRenderer                      | <-----> | SparkleEditor                  |
          | - framegraph                         |         | - editor host and viewport UI  |
          | - passes and scene assembly          |         | - sanctioned native host seam  |
          | - consumes neutral RHI services only |         +----------------+---------------+
          +------------------+-------------------+                          |
                             |                                              |
                             +----------------------+-----------------------+
                                                    |
                                                    v
                         +---------------------------------------------------+
                         | SparkleRHI                                         |
                         | - RenderHardwareInterface                          |
                         | - resource, pipeline, and descriptor services      |
                         | - shader loading                                   |
                         | - diagnostics, markers, timings, capture hooks     |
                         +----------------------+----------------------------+
                                                |
                           +--------------------+--------------------+
                           |                                         |
                           v                                         v
         +--------------------------------------+   +--------------------------------------+
         | D3D12 Backend                         |   | Vulkan Backend                        |
         | - current control backend             |   | - parity target backend               |
         | - DXIL consumption                    |   | - SPIR-V consumption                  |
         | - PIX, InfoQueue, DRED, RenderDoc     |   | - validation, RenderDoc, Nsight      |
         +------------------+-------------------+   +------------------+-------------------+
                            |                                          |
                            v                                          v
                 +------------------------+                 +------------------------+
                 | Native Graphics APIs   |                 | Native Graphics APIs   |
                 | - D3D12 / DXGI         |                 | - Vulkan / Win32       |
                 +------------------------+                 +------------------------+
```

- The renderer is the policy layer. It decides what to render, how passes depend on each other, and which logical resources or bindings are needed.
- The RHI is the translation layer. It owns native resource identity, native pipelines, diagnostics integration, and backend-specific submission details.
- SparkleCore should still own logger bootstrap, sink setup, and named logger lifetime, and the selected direction is a central logger registry with direct `spdlog` API usage at callsites after logger acquisition.
- ShaderCompiler is the offline shader boundary, while AssetConverter remains the offline scene and texture boundary. Runtime execution should be about loading and translating prepared data, not compiling or inventing metadata on the fly.
- D3D12 remains the control backend until Milestone 2 closes. Vulkan should match the same contracts rather than forcing the renderer to learn a second execution model.