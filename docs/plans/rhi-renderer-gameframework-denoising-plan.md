# RHI, Renderer, and GameFramework Denoising Plan

## Intent

Continue the SparkleEngine denoising work across RHI, Renderer, and GameFramework with equal attention to code shape and file structure. The goal is to make module boundaries read like a modern GPU-facing engine: public contracts are intent-based, backend details stay private, and folder names make ownership obvious before opening a file.

This plan is a roadmap, not an implementation patch. Each phase should remove the replaced path in the same phase rather than adding compatibility headers or long-lived forwarding shims.

## Vendor Naming And Pattern Compass

Use AMD/NVIDIA-style naming as a practical compass, not as a reason to copy any one SDK wholesale.

- Prefer `Device`, `CommandList`, `Queue`, `SwapChain`, `Resource`, `Texture`, `Buffer`, `Descriptor`, `BindingLayout`, `BindingSet`, `PipelineState`, `RenderPass`, and `RenderGraph` shaped concepts.
- Use `Desc` for creation descriptions, `Handle` for opaque ids, `View` for resource views or camera/view concepts, `State` for resource/pipeline state, `Barrier` for synchronization, and `Context` only for scoped services or execution context.
- Keep backend interop explicit: `D3D12*` and `DXGI*` belong in private D3D12 implementation folders or a narrow native interop header, not in high-level Renderer/GameFramework public headers.
- Prefer backend-neutral `Format`/`PixelFormat` in public contracts; translate to DXGI/Vulkan formats inside backend conversion code.
- Preserve Sparkle's existing UE/RDG leaning for framegraph decisions where it already fits, but keep the naming readable to someone familiar with NVIDIA NVRHI/Falcor or AMD GPUOpen/Cauldron patterns.

## Current Boundary Observations

- `Engine/RHI/Public/Interop/RenderHardwareInterface.h` is a monolithic public header. It currently contains native handles, descriptor handles, sampler descriptions, resource descriptions, ray tracing descriptions, pipeline descriptions, diagnostics interfaces, `RenderCommandList`, and `RenderHardwareInterface`.
- `Engine/Renderer/Public/Renderer.h` still exposes `NativeGraphicsCommandListHandle` through `TransitionRenderProduct`, which keeps Application and validation code aware of native command-list handles.
- `Engine/Renderer/Public/FrameGraph/RenderGraphPassContext.h` is public but forwards private execution concepts such as `CommandContext`, `FrameContext`, and `RenderPassContext`.
- `Engine/Renderer/Private/GPU/CommandContext.*` wraps the RHI command list and diagnostics. The name is generic and the folder name `GPU` is broad compared with the actual responsibility.
- `Engine/Renderer/Private/Frame/RenderViewContext.h` is data-only, so `Context` is noisier than its role.
- `Engine/RHI/Public/D3D12/Textures/CookedTextureAsset.h`, `Engine/RHI/Public/Resources/TextureTypes.h`, and `Engine/Renderer/Public/Textures/TextureDiagnostics.h` still surface DXGI-shaped format naming in public-facing contracts.
- `Engine/GameFramework/Public/Scene/RuntimeScenePayload.h` is a cooked scene load payload, not persistent scene runtime ownership. The current name is broader than the role.
- `Engine/GameFramework/Private/Assets/Loaders/LoadedCookedAssets.h` groups loaded cooked records, while `MaterialAssetTranslator` and `SceneAssetManager` do assembly work nearby. The shape is close, but folder names can make cooked asset assembly more obvious.

## Phase 1: Split The RHI Public Surface

Goal: turn the RHI public API into small, vendor-readable contracts without changing behavior.

Planned moves and renames:

- Split `Engine/RHI/Public/Interop/RenderHardwareInterface.h` into focused headers:
  - `Engine/RHI/Public/Core/RhiBackendApi.h`
  - `Engine/RHI/Public/Interop/RhiNativeHandles.h`
  - `Engine/RHI/Public/Descriptors/RhiDescriptorHandles.h`
  - `Engine/RHI/Public/Samplers/RhiSamplerDesc.h`
  - `Engine/RHI/Public/Resources/RhiResourceDesc.h`
  - `Engine/RHI/Public/RayTracing/RhiRayTracingDesc.h`
  - `Engine/RHI/Public/Pipeline/RhiPipelineStateDesc.h`
  - `Engine/RHI/Public/Diagnostics/RhiDiagnostics.h`
  - `Engine/RHI/Public/Commands/RenderCommandList.h`
  - `Engine/RHI/Public/Device/RenderHardwareInterface.h`
- Keep `RenderHardwareInterface` as the high-level device facade, but stop making it the home for every support type.
- Rename `RendererBackendServices` to a device/service name that does not imply renderer ownership of a backend, likely `RhiDeviceServices` or `RenderDeviceServices`.
- Move `RendererBackendServices.h` from `Public/Interop` to `Public/Device` with the selected name.
- Move public D3D12 cooked texture schema out of `Public/D3D12/Textures` into a backend-neutral texture/cooked asset location. Replace public `dxgiFormat` naming with a neutral `format`/`pixelFormat` field where possible, and isolate DXGI conversion in private D3D12 loaders.

Done criteria:

- Renderer and GameFramework public headers no longer include the monolithic RHI interop header for simple handle/descriptor/sampler types.
- `Public/D3D12` is either empty or limited to a deliberately documented native interop contract.
- `D3D12`/`DXGI` search hits in RHI public headers are intentional and limited to backend identity or native interop.
- `git diff --check` passes.

## Phase 2: Hide Native Command Lists From Renderer Consumers

Goal: finish the viewport/editor boundary cleanup by replacing native-command-list exposure with intent-based Renderer/RHI calls.

Planned moves and renames:

- Replace `Renderer::TransitionRenderProduct(NativeGraphicsCommandListHandle, ...)` with a higher-level method such as `TransitionRenderProduct(RenderProductHandle, ResourceState before, ResourceState after)` or a more direct `PrepareRenderProductForPresentation` / `RestoreRenderProductAfterPresentation` pair.
- Move present-pass orchestration behind a small RHI present helper that accepts `RenderCommandList&` or an internal current-frame command list, not a native command-list handle.
- Update `EditorApp`, `RhiSmokeValidation`, `RuntimeConsoleHost`, `RuntimeConsoleOverlay`, and `UI` call sites to stop fetching `GetGraphicsCommandListHandle` directly for ordinary presentation/overlay rendering.
- Keep native handle access only for backend interop seams, with names that say so.

Done criteria:

- `Engine/Renderer/Public/Renderer.h` does not name `NativeGraphicsCommandListHandle`.
- Application/editor validation paths express present and overlay intent instead of command-list plumbing.
- Public native command-list access is either removed or isolated under `RHI/Public/Interop/RhiNativeHandles.h` with limited consumers.
- Targeted compile of touched modules passes.

## Phase 3: Make Renderer Framegraph Execution Private

Goal: keep public Renderer contracts useful while removing pass execution implementation from the public surface.

Planned moves and renames:

- Move `Engine/Renderer/Public/FrameGraph/RenderGraphPassContext.h` to `Engine/Renderer/Private/FrameGraph/Execution/RenderGraphPassContext.h`.
- Move `PassBuilder` and pass resource declarations private if no external module needs to author renderer framegraph passes.
- Keep only reusable handle/description contracts public if shader parameter code still needs them, potentially under:
  - `Engine/Renderer/Public/RenderGraph/RenderResourceHandle.h`
  - `Engine/Renderer/Public/RenderGraph/RenderTextureHandle.h`
  - `Engine/Renderer/Public/RenderGraph/RenderBufferHandle.h`
- If the team wants to keep the existing `FrameGraph` name, keep it consistently. If we choose `RenderGraph`, rename the folder and types in one phase, not piecemeal.
- Avoid making `CommandContext`, `FrameContext`, or `RenderPassContext` visible from public headers.

Done criteria:

- No public Renderer header forwards or exposes private execution types.
- External modules can still consume viewport products, shader reload results, diagnostics snapshots, and renderer entry points.
- Pass authoring remains internal unless a real plugin/module use case exists.

## Phase 4: Rename Renderer Execution And View Data Files

Goal: make private Renderer folders communicate ownership and reduce broad names.

Planned moves and renames:

- Rename `Engine/Renderer/Private/GPU/CommandContext.*` to a narrower command wrapper name, likely `Engine/Renderer/Private/Commands/RenderCommandContext.*`.
- Move frame execution diagnostics out of broad `GPU` into `Private/Diagnostics` or `Private/Frame/Diagnostics`.
- Move `GPUMesh.*` and `GPUMeshCache.*` into `Private/Meshes` or `Private/GpuResources/Meshes`, depending on whether the chosen vocabulary is domain-first or backend-resource-first.
- Rename `RenderViewContext.h` to `RenderViewData.h` or `ViewUniformData.h`, because it stores per-view constants, GPU address, viewport, and scissor data.
- Review `Private/Frame` versus `Private/Passes` split. Keep `Frame` for pass scheduling/stage assembly and `Passes` for executable pass classes, or rename them to `FramePipeline` and `RenderPasses` if that makes the distinction clearer.

Done criteria:

- Broad `GPU` folder either disappears or only contains genuinely shared GPU resource abstractions.
- Data-only records use `Data` or a domain name instead of `Context`.
- Pass/stage folders make it clear whether the file builds a frame stage or executes a render pass.

## Phase 5: Consolidate Renderer Resource And Texture Diagnostics Naming

Goal: keep Renderer diagnostics and texture ownership backend-neutral.

Planned moves and renames:

- Replace public `DxgiFormat` fields in `TextureDiagnosticsRow` with `PixelFormat` or a display string produced by the backend/texture manager.
- Audit `TextureTypes.h` and texture diagnostics for backend leakage. Public rows should describe `width`, `height`, `mips`, `format`, `group`, `source`, and residency/state in neutral terms.
- Keep backend-specific format conversion in RHI private D3D12 type conversion code.
- Consider moving `DefaultTextures.h` and `TextureDiagnostics.h` under a clearer public folder such as `Public/Resources/Textures` if Renderer public resources become broader than textures.

Done criteria:

- Renderer public texture diagnostics are backend-neutral.
- D3D12/DXGI format vocabulary does not appear in Renderer public headers.
- Existing editor diagnostics panels still show readable texture information.

## Phase 6: Rename GameFramework Cooked Scene Assembly

Goal: keep GameFramework runtime scene ownership clean while making cooked-load assembly names precise.

Planned moves and renames:

- Rename `RuntimeScenePayload` to a cooked-load name such as `LoadedSceneAssetData`, `SceneAssetPayload`, or `CookedSceneLoadData`. Preferred direction: `SceneAssetPayload`, because it belongs to loading/append flow and avoids claiming ownership of runtime scene state.
- Rename `SceneAssetLoadResult::payload` to match the selected record name, for example `sceneAssetData` or `payload` if the type name is already specific.
- Rename `GameScene::AppendRuntimeScenePayload` to `AppendSceneAssetPayload` or `AppendLoadedSceneAsset`.
- Move `RuntimeScenePayload.h/cpp` out of `Public/Scene` if the type is only a scene asset loading exchange record. Candidate path: `Public/Assets/SceneAssetPayload.h`.
- Move `MaterialAssetTranslator.*` under a cooked assembly folder and rename it to `CookedMaterialTranslator.*` or `MaterialRuntimeTranslator.*`.
- Split or rename `LoadedCookedAssets.h` into focused loaded-record headers only if it keeps includes smaller. Good candidates are `LoadedSceneManifest.h` and `LoadedMaterialAsset.h` under `Private/Assets/Cooked`.

Done criteria:

- `GameScene` owns live scene state; scene asset loading owns payload assembly.
- Public GameFramework scene folder contains scene concepts, not loading exchange records.
- Level loading flow still reads as: level description -> scene asset load -> append loaded scene data -> capture snapshots for renderer.

## Phase 7: Prune Include Roots And Public Module Shape

Goal: make file moves stick by removing include-path crutches and broad public exposure.

Planned cleanup:

- Remove extra public include roots from `Engine/GameFramework/CMakeLists.txt` for `Public/Scene` and `Public/Scene/Camera` after includes use module-root structured paths consistently.
- Keep the existing CMake glob workflow, but use file moves to improve Visual Studio source grouping naturally.
- Avoid adding umbrella headers unless they are true module entrypoints.
- Add or update validation scripts only after the new structure is settled, so checks enforce the chosen shape rather than the transitional one.

Done criteria:

- Modules build with module public root plus private root, without per-subfolder include crutches.
- Public headers include other public headers through stable module-root paths or relative sibling paths according to the local convention.
- `git diff --check` and targeted compile pass.

## Suggested Execution Order

1. RHI header split and native handle isolation.
2. Renderer public native command-list removal.
3. Renderer framegraph execution privatization.
4. Renderer private folder/file rename pass.
5. Renderer diagnostics/backend-neutral format cleanup.
6. GameFramework cooked scene assembly rename/move pass.
7. Include-root pruning and validation rule updates.

This order keeps blast radius controlled: first shrink RHI public includes, then clean Renderer consumers, then rename private folders, then adjust GameFramework load/scene exchange names.

## Validation Strategy

For each phase:

- Run `git diff --check`.
- Run targeted grep checks for stale names and backend leaks.
- Prefer targeted builds of the touched modules after structural moves.

At the end of the sequence:

- Run `Scripts\BuildProject.bat Showcase DevelopmentEditor`.
- Run `Scripts\CookAllAssets.bat Showcase DevelopmentGame` when GameFramework cooked-load names or runtime asset flow changes.
- Re-run the existing CMake boundary targets after resolving the known logging/tool public seam blockers, or run the specific unaffected boundary checks independently when possible.

## Non-Goals

- Do not introduce bindless material/resource binding as part of this cleanup.
- Do not add compatibility headers for old paths unless the phase explicitly chooses a temporary migration window.
- Do not move D3D12 implementation out of RHI private folders.
- Do not collapse GameFramework scene ownership into Renderer.
- Do not rename `FrameGraph` to `RenderGraph` unless the whole framegraph public/private surface is moved coherently in one phase.