## Plan: Compute Auto Exposure Tonemapping

Implement tonemapping as a real post-process path, not a backbuffer-side tweak. The recommended shape is: move scene rendering to an HDR intermediate, add a dedicated tonemap pass before UI composition, then add compute-capable frame-graph infrastructure so log-average auto exposure can run fully on GPU with temporal adaptation. Keep first scope to renderer/runtime controls plus editor tuning UI; do not add level serialization or level-file parsing in this milestone.

**Steps**
1. Phase 1: Define the output pipeline and scope boundaries.
2. Update the render path so scene color is no longer written straight into the SDR backbuffer. Forward opaque should target a transient HDR scene-color texture, tonemapping should write the final SDR result into the imported backbuffer, and UI composition should remain the last raster pass over the tonemapped backbuffer. This depends on the existing frame-graph insertion point between forward rendering and UI.
3. Explicitly keep LevelDesc, LevelParser, and section parsers out of scope for this milestone. The current open LevelParser.cpp file is only relevant if runtime tuning later needs persistence; for now, settings live in renderer CVars and editor-side live controls only.
4. Phase 2: Add compute-ready frame-graph and command-context support.
5. Extend frame-graph pass kinds to support Compute in addition to Raster, then propagate that through pass validation, execution-kind reporting, and compile-time assumptions that currently treat Raster as the only legal pass kind. This blocks all GPU auto-exposure work.
6. Extend resource declarations so passes can express UAV writes and compute-readable resources cleanly. ResourceState already contains UnorderedAccess, but ResourceUsage and state inference currently stop at RenderTarget, Depth, ShaderRead, and Present. Add the missing usage/state mapping and ensure compiler barrier planning handles UAV transitions correctly.
7. Extend CommandContext with the compute operations needed by exposure passes: compute root-signature binding, compute descriptor-table binding if separate from graphics is required by current wrappers, pipeline binding compatibility, dispatch, and UAV/resource barriers as needed. This depends on step 5 and step 6.
8. Audit PSO/root-signature ownership so the renderer can build and store compute pipeline state objects alongside the current graphics PSOs. If current root signatures are graphics-only in layout assumptions, either generalize them or introduce dedicated compute root signatures for the luminance extraction/reduction/adaptation stages.
9. Phase 3: Build the HDR plus tonemap pass path.
10. Add an HDR scene-color texture format and descriptor choice, preferably a float format such as RGBA16F, and create that texture in the frame graph instead of rendering scene color directly to the SDR backbuffer.
11. Add a dedicated TonemapPass that reads HDR scene color and the current exposure result, applies ACES fitted tonemapping, and writes into the imported backbuffer before UIComposition. This depends on step 10 and can begin in parallel with step 12 once compute infrastructure is stable enough to define the exposure resource interface.
12. Add the shader-side constant layout needed for tonemap and exposure parameters. Reuse the per-frame constant-buffer path if the parameter count stays small and the register layout remains coherent; otherwise introduce a dedicated post-process constant buffer and keep the HLSL constant-buffer definitions synchronized.
13. Phase 4: Implement fully GPU auto exposure.
14. Add a luminance extraction compute pass that reads HDR scene color and writes log-luminance or luminance statistics into a reduction-friendly intermediate resource. Favor a texture or structured buffer layout that can be reduced repeatedly without adding CPU readback.
15. Add one or more compute reduction passes that collapse the luminance data down to a compact exposure input, ideally to a 1x1 or tiny buffer/texture result. Keep the first implementation log-average based, as chosen, rather than histogram based.
16. Add a persistent exposure history resource per frame in flight so adaptation uses previous-frame exposure instead of snapping immediately. This depends on the reduction output format being defined.
17. Add an adaptation compute pass that consumes previous exposure, current measured luminance, delta time, and user tuning parameters to produce the exposure scalar used by TonemapPass. The adaptation policy should support separate brighten/darken speeds so the user gets eye adaptation rather than immediate changes.
18. Ensure the renderer updates pass ordering to: shadows -> forward opaque HDR -> exposure compute chain -> tonemap raster/fullscreen pass -> UI composition -> present. This depends on steps 11 and 14 through 17.
19. Phase 5: Expose runtime controls and editor tuning UI.
20. Add renderer CVars for tonemap enable/mode, exposure compensation, min/max exposure clamp, target middle gray if needed, and brightening/darkening adaptation speeds. Keep these as the runtime source of truth for this milestone.
21. Thread those values into the per-frame or post-process constant-buffer update path so both compute exposure passes and the final tonemap pass consume one coherent parameter set.
22. Add an editor-facing panel or inspector section for tonemapping and exposure controls using existing ImGui utility patterns. The leanest fit is either a dedicated render-settings panel created alongside current default panels or a new section inside the existing scene inspector if you want all live scene tuning consolidated. This is parallel with step 20 after the parameter model is settled.
23. Surface useful debug readouts in the UI, such as current exposure, measured average luminance, and whether tonemapping/auto exposure are enabled. This depends on step 17.
24. Phase 6: Hardening and validation.
25. Validate that frame-graph compilation still produces correct barriers and execution order across mixed raster/compute workloads, especially around HDR scene-color SRV/UAV transitions and the final backbuffer write.
26. Validate resize, swapchain recreation, and frame-in-flight lifetime for HDR scene color and persistent exposure resources so no stale descriptors or mismatched dimensions survive resize.
27. Validate UI compositing stays in SDR/output space and is not tonemapped a second time.
28. Add targeted debug tooling where useful: event labels for exposure passes, optional visualization of exposure inputs, and temporary CVars to disable auto exposure or freeze exposure for debugging.

**Relevant files**
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\FrameGraph\Builder\FrameGraphBuilder.cpp — currently builds BackBuffer/MainDepth plus ForwardOpaque and UIComposition; primary insertion point for HDR scene color, exposure passes, and TonemapPass ordering.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\Renderer.cpp — current frame loop calls Setup, Compile, and Execute; likely orchestration checkpoint for any persistent exposure-history lifecycle or resize-triggered refresh.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\FrameGraph\FrameGraphPassFlags.h — currently raster-only; must gain Compute support.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\FrameGraph\ResourceUsage.h — must grow the usage vocabulary needed for UAV/compute exposure passes.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\FrameGraph\ResourceState.h — already contains UnorderedAccess and copy states; keep usage/state inference aligned with this file.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\FrameGraph\Compiler\FrameGraphCompiler.h — compiler assumptions and barrier/state inference work needed for mixed raster/compute execution.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\FrameGraph\FrameGraph.h — pass/resource API surface for new pass kinds and exposure resources.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\CommandContext.h — needs compute dispatch and any missing compute binding helpers/barriers.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\RenderConfig.h — current backbuffer format and likely home for HDR scene-color format constants.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Public\D3D12\Resources\D3D12ConstantBufferData.h — current PerFrameConstantBufferData layout; likely place to extend for tonemap/exposure parameters unless a dedicated post-process CB is cleaner.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\RHI\Private\D3D12\Resources\D3D12ConstantBufferManager.cpp — current per-frame update point that already pulls renderer CVars and timer/window data; best runtime parameter injection point.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Assets\Shaders\Resources\ConstantBuffers.hlsli — shader-side CB layout that must stay synchronized with CPU definitions.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\Passes\ForwardOpaquePass.h — reference pattern for pass object structure and current scene rendering target assumptions.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Public\RendererCVars.h — current renderer control pattern; expand for tonemap/exposure CVars.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Renderer\Private\RendererCVars.cpp — instantiate/register the new CVars.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Private\UI.cpp — default panel creation and placement if a dedicated render-settings panel is added.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Private\Panels\SceneInspectorPanel.cpp — reusable live-editing pattern if tonemap controls are folded into the existing inspector.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\Editor\Private\Util\UiUtil.cpp — reusable slider/input/key-value row widgets for exposure tuning and debug readouts.
- c:\Users\stole\Documents\GitHub\SparkleEngine\Engine\GameFramework\Private\Level\LevelParser.cpp — explicitly unchanged in this milestone unless scope later expands to persistent level render settings.

**Verification**
1. Build the engine and verify the frame graph still compiles after introducing Compute pass kinds and new resource usages.
2. Run a scene with strong luminance contrast and confirm the render order is HDR scene render first, then exposure passes, then tonemap, then UI.
3. Resize the window repeatedly and verify HDR/exposure resources resize correctly without stale descriptors, incorrect barriers, or flicker.
4. Toggle tonemapping and auto exposure independently through CVars and confirm behavior changes immediately at runtime.
5. Verify ACES fitted tonemapping clamps HDR highlights into SDR output while the UI remains visually stable and un-tonemapped.
6. Verify eye adaptation over time by moving between dark and bright areas and observing distinct brighten/darken response speeds.
7. Add or use GPU markers/event scopes to confirm compute exposure passes execute in the intended order and do not alias with UI or backbuffer presentation incorrectly.
8. If a dedicated editor panel is added, verify those controls remain responsive during play/editor updates and reflect the same runtime values as the CVars.

**Decisions**
- Include: ACES fitted tonemapping, compute-based log-average auto exposure, temporal adaptation, runtime CVars, and editor-facing live tuning UI.
- Exclude: level serialization, LevelDesc changes, new level-file sections, and edits to LevelParser/section parsers for this first milestone.
- Exclude: histogram-based exposure, bindless or broader renderer architecture changes unrelated to tonemapping/exposure.
- Prefer a real HDR intermediate plus final tonemap pass instead of trying to tonemap in place on the SDR backbuffer.
- Because the user explicitly chose the larger path, compute/UAV/frame-graph support is treated as required foundation work rather than deferred follow-up.

**Further Considerations**
1. Panel placement recommendation: use a dedicated render-settings panel if you want tonemap/exposure controls always visible; use the existing scene inspector only if you want render settings to feel like scene-wide properties rather than a renderer workspace.
2. Exposure-history ownership recommendation: keep exposure history renderer-private and frame-scoped, not in scene/camera objects, so camera/level code stays free of renderer-specific transient state.
3. Shader architecture recommendation: keep luminance extraction/reduction/adaptation as separate passes first, then fuse later only if profiling shows the extra dispatches matter.