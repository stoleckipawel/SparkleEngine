# DLRR Toggle/Quality Investigation

## Original Reproduction

- Launch `ShowcaseEditor`.
- Open the settings window.
- If Ray Reconstruction starts enabled: switch `Ray Reconstruction -> Mode` from `NVIDIA DLRR` to `Off`, then back to `NVIDIA DLRR`.
- If Ray Reconstruction starts disabled: switch it to `NVIDIA DLRR`.
- In the original UI, with Ray Reconstruction enabled, change the separate Ray Reconstruction quality dropdown.
- Observed result: the image becomes very noisy, visually matching the raw/noisy input rather than the denoised Ray Reconstruction output.

## Confirmed Observations

- Startup can produce a good denoised image.
- The failure is transition-driven: toggling the feature or changing the Ray Reconstruction quality setting breaks the denoised result after startup.
- The editor run launched by Codex on `2026-07-08 20:00:42` exited before renderer evidence was available because it was started from the repository root. `Filesystem::DiscoverProjectRoot()` therefore resolved the repository workspace/root context instead of `Projects/Showcase`; `LevelRegistry` could not find `Levels.catalog`, and texture loading could not resolve `Defaults/default_checkerboard.stex`.
- The launch-context failure is not evidence against the DLRR fix. The cooked texture exists at `artifacts/dev/projects/Showcase/cooked/Textures/Defaults/default_checkerboard.stex`, and the project catalog exists at `Projects/Showcase/Levels.catalog`. Verification must launch `ShowcaseEditor.exe` with `Projects/Showcase` as the working directory so project discovery and cooked asset roots line up.
- Clean verification on `2026-07-08 20:38:33` launched `ShowcaseEditor.exe` with `Projects/Showcase` as the working directory, drove the real editor console commands, captured screenshots, and scanned the copied editor log for the old failure signatures.
- Streamline's DLRR plugin source recreates its NGX feature when mode, output size, normal/roughness mode, or presets change.
- DLRR is now treated as an on/off denoiser. Upscaler quality owns render resolution; DLRR no longer exposes its own quality setting.

## Proven Architecture Problems

### 1. Quality Changes Were Treated As Topology Changes

`RendererImageProviderStack::PackImageProviderGraphKey` included both DLSS and DLRR quality modes. That made a pure quality setting change look like a frame-graph/provider topology change.

Effect:

- `FramePipeline::BeginFrame` saw a new image-provider graph key.
- It called `RendererSystemRoot::RefreshImageProviders()`.
- The provider stack shut down and recreated Streamline-backed providers even though the render graph topology did not need to change.

This is the wrong boundary. Provider kind/mode changes can change graph topology. Quality changes should be live provider state and should request temporal history reset, not destroy and recreate the provider stack.

Proof from the original code:

```cpp
return static_cast<std::uint32_t>(upscalerSettings.RequestedProvider) |
       (static_cast<std::uint32_t>(upscalerSettings.QualityMode) << 4u) |
       (static_cast<std::uint32_t>(rayReconstructionSettings.Mode) << 8u) |
       (static_cast<std::uint32_t>(rayReconstructionSettings.QualityMode) << 12u);
```

### 2. Individual Providers Owned The Global Streamline Runtime

The original DLSS and DLRR runtimes each called `slInit` during provider initialization and `slShutdown` during provider shutdown.

That is not a safe ownership model for settings transitions because:

- Streamline is process/global integration state, not a per-setting object.
- The Streamline guide says `slInit` should happen early and `slShutdown` should happen before tearing down the device/API, not during ordinary quality/mode changes.
- Provider refresh on `On -> Off -> On` or quality changes could repeatedly tear down and recreate Streamline while the renderer, swap chain, device, and presentation hooks remained alive.

Proof from the original DLRR runtime:

```cpp
const sl::Feature features[] = {sl::kFeatureDLSS_RR};
sl::Result result = slInit(preferences, sl::kSDKVersion);
...
(void) slShutdown();
```

The original DLSS runtime had the same pattern with `sl::kFeatureDLSS`.

### 3. DLSS And DLRR Shared Viewport 0

The original DLSS and DLRR runtimes both used `sl::ViewportHandle{0u}`. A transition from DLRR to DLSS or back could therefore reuse the same Streamline viewport id for different features while resources/options were being torn down and recreated.

This is risky because Streamline stores per-viewport options/resources internally. DLSS and DLRR should have independent viewport ids unless they are explicitly designed as a single combined feature path.

### 4. Native-Resolution DLRR Was Exposed As Upscaling Quality Modes

The settings UI exposes `Quality`, `Balanced`, and `Performance` for DLRR, but the renderer currently passes identical render and output extents to DLRR. Streamline/NGX quality modes are tied to an expected lower render resolution. The current renderer does not allocate lower-resolution DLRR inputs for those modes.

Therefore, when the renderer is operating at native 1:1 extent, the only internally consistent Streamline mode is DLAA/native mode. Quality/Balanced/Performance should not drive Streamline mode changes until the renderer has real dynamic/render-resolution scaling for the DLRR input contract.

Proof from Streamline's DLRR plugin:

- `slDLSSDSetOptions` stores per-viewport `DLSSDOptions`.
- `dlssdBeginEvent` treats changes to `mode`, output size, normal/roughness mode, or presets as `modeOrSizeChanged`.
- When `modeOrSizeChanged` is true, the plugin releases/recreates the NGX Ray Reconstruction feature.
- During creation, the plugin calls `slGetData` and uses `viewport.settings.optimalRenderWidth/Height` as the NGX feature input size.

That means changing DLRR from Quality to Balanced/Performance is not a cosmetic setting. It changes the expected render input size. The Sparkle renderer did not change the actual `SceneColor`, depth, motion-vector, albedo, normal, roughness, or hit-distance extents to match.

### 5. Presentation Interface Upgrade Was Not Idempotent

After the first fix, changing the original Ray Reconstruction quality dropdown no longer broke the result because the DLRR runtime stayed alive. The remaining failure was isolated to `On -> Off -> On`, which destroys and recreates providers.

The D3D12 Streamline integration uses manual presentation hooks, so provider initialization calls through `RhiInteropService::UpgradePresentationInterface()` to `D3D12SwapChain::UpgradeNativeInterface()`, which calls Streamline's `slUpgradeInterface`.

The original D3D12 swap-chain bridge always called the upgrade callback again:

```cpp
void* upgradedInterface = m_swapChain.Get();
if (!callback(&upgradedInterface, userData) || upgradedInterface == nullptr)
{
    return false;
}
```

That is not safe across provider recreation. Once the swap chain has been upgraded, the RHI must treat the presentation bridge as already upgraded for the swap-chain lifetime. A second provider initialization should not call `slUpgradeInterface` again on the already-upgraded interface and then mark DLRR unavailable if that second upgrade fails.

Fix: `D3D12SwapChain` now records `m_nativeInterfaceUpgraded`. After the first successful upgrade, later calls return success while the swap chain still exists.

### 6. Native Device Binding Was Still Per Provider

The next verification run after making presentation upgrade idempotent proved the remaining `On -> Off -> On` failure:

```text
Ray reconstruction provider initialization failed for NVIDIA DLRR: Streamline native device setup failed: Result::eErrorInvalidIntegration
```

That failure came from `SetStreamlineNativeDevice()` during DLRR provider reinitialization. This is the same class of lifetime bug as repeated presentation upgrade: `slSetD3DDevice`/`slSetVulkanInfo` is global Streamline runtime state, not per-provider state.

Quality changes worked because they did not recreate the provider and therefore did not call `slSetD3DDevice` again. `On -> Off -> On` recreated the provider, called native device setup a second time, and Streamline returned `eErrorInvalidIntegration`; the ray reconstruction subsystem then dropped the provider and the DLRR pass copied the noisy input.

Fix: native device setup now happens inside `AcquireSharedStreamlineRuntime()` only when the shared Streamline runtime is first created. Later DLSS/DLRR provider reacquisitions validate the render API and increment the shared session reference count without rebinding the native device.

## Fix Direction

- Keep Streamline initialized for the renderer lifetime, not per feature toggle.
- Bind the native Streamline device/queue once for the shared Streamline runtime lifetime, not per provider initialization.
- Only include provider kind/mode in the frame-graph key, not quality.
- Remove DLRR quality from the runtime/UI/config contract. DLRR is an on/off denoiser; upscaler quality owns render resolution.
- Submit DLRR to Streamline in DLAA/native mode.
- Use separate Streamline viewport ids for DLSS and DLRR.
- Make Streamline presentation-interface upgrade an RHI/swap-chain lifetime operation, not a per-provider-init operation.

## Current Patch State

- Added a shared Streamline runtime session that loads both `sl::kFeatureDLSS` and `sl::kFeatureDLSS_RR`; ordinary provider shutdown now releases feature resources but does not call `slShutdown`.
- `RendererSystemRoot` shuts down Streamline once during renderer shutdown.
- DLSS uses Streamline viewport `1`; DLRR uses viewport `2`.
- DLSS and DLRR color inputs are tagged/evaluated as shader-readable resources. Fallback copy paths transition to copy states only when the provider fails.
- Ray Reconstruction quality was removed from runtime/UI/config because DLRR is a denoiser and should not own render-resolution policy.
- DLSS accepts live quality changes and requests history reset instead of forcing graph/provider recreation.
- DLRR always submits Streamline `DLSSMode::eDLAA`; render resolution is controlled by the upscaler quality setting before DLRR runs.
- D3D12 swap-chain presentation upgrade is now idempotent, so `On -> Off -> On` provider recreation does not fail by attempting to upgrade an already-upgraded Streamline presentation interface.
- Shared Streamline acquisition now owns native device setup; provider reinitialization no longer calls `slSetD3DDevice`/`slSetVulkanInfo` again.
- The final code keeps only requested-provider initialization failure logging. Temporary proof logs for success/fallback transitions were removed after verification so the frame passes remain clean.
- DLSS and DLRR implementation names are separated: DLSS lives under `Upscaling/NvidiaDlss`, DLRR lives under `RayReconstruction/NvidiaDlrr`, and the shared build target is named for NVIDIA Streamline rather than for either feature.
- The ray-reconstructed scene-color intermediate is named `ReconstructedSceneColor`, so the frame graph does not describe DLRR output as an upscaler input.

## Upscaler Selection And Shading Resolution Refinement

The renderer now treats upscaler selection and ray reconstruction as separate systems:

- `r.Upscaler.Provider` selects the final-color upscaler: `Linear` or `NVIDIA DLSS`.
- `r.Upscaler.QualityMode` drives realtime shading resolution through `FrameResolution`.
- Ray Reconstruction remains controlled by `r.RayReconstruction.Mode` and runs at the realtime render extent.
- The image-provider stack passes render-to-output extents to the upscaler, but render-to-render extents to Ray Reconstruction.

The architectural boundary is:

- `FramePipeline` resolves frame render/output extents and orchestrates setup. It does not know the quality-mode scale table.
- `Frame/Core/FrameResolution` owns the policy that maps output extent plus upscaler quality into render extent.
- `Frame/Presentation/Upscaling` owns Linear-vs-DLSS presentation assembly and provider fallback.
- `Passes/Presentation/LinearUpscalePass` implements the renderer-owned bilinear upscale path.
- `EngineRenderingUpscalingSettings` owns persisted/public settings conversion, keeping editor UI code out of private CVar details.
- The NVIDIA Streamline provider build target contains separate DLSS and DLRR source groups plus shared SDK integration code. Feature-specific code remains in its feature module; only Streamline lifetime/presentation/device ownership is shared.

This means Linear and DLSS can be selected independently of Ray Reconstruction. Quality mode changes now resize the realtime shading resources; provider selection changes only the final upscaling implementation.

## Verification Evidence

### Linear Upscaler Crash Reproduction

Artifact: `logs/Projects/Showcase/Full/ShowcaseEditor_2026-07-08_23-52-20.log`

The current DLSS-to-Linear crash reproduced before any settings automation ran. The editor exited while building the Linear upscaling pass because the Showcase cooked shader output did not contain the new `LinearUpscale` package:

```text
Runtime validation rejected cooked shader package 'LinearUpscale' for pass 'LinearUpscale' ... Failed to open ... AFEA69852025BAD9.sparkshader
```

This is a cooked-content freshness failure, not a DLSS/DLRR lifetime failure. `LinearUpscale` is registered in `Engine/Renderer/ShaderRegistrations/LinearUpscaleShader.cpp`, and a project-local shader cook from `Projects/Showcase` generated:

```text
artifacts/dev/projects/Showcase/cooked/Shaders/Packages/AFEA69852025BAD9.sparkshader
artifacts/dev/projects/Showcase/cooked/Shaders/ShaderPackageRegistry.sreg
```

Command used:

```text
artifacts/dev/tools/ShaderCompiler/DevelopmentEditor/ShaderCompiler.exe cook --target DxilSm66 --target SpirV16
```

The working directory matters. Running the command from the repository root cooks the shared shader root; running it from `Projects/Showcase` refreshes the shader root that `ShowcaseEditor` loads.

### Upscaler/DLRR Runtime Transition Verification

Artifact: `artifacts/verification/upscaler-dlrr-automation-20260709-000227`

After recooking Showcase shaders, an editor run applied the same runtime CVars as the settings UI across the requested transition matrix:

```text
r.Upscaler.Provider       1 -> 0 -> 1 -> 0 -> 1
r.Upscaler.QualityMode    1 -> 3 -> 1
r.RayReconstruction.Mode  1 -> 0 -> 1
```

Evidence files:

- `ShowcaseEditor_automation_after_close.log` records each applied CVar transition and contains no `critical`, `error`, `fatal`, `failed`, `rejected`, `invalid`, `crash`, or `exception` matches.
- `process_status.txt` recorded `ShowcaseEditor` still responding after the sequence.
- `01_after_initial_dlss_quality_dlrr_on.png` through `10_settled_final.png` are full-size `1600x900` frame captures taken during and after the sequence.

### Instrumented Root-Cause Proof

Artifact: `artifacts/verification/dlrr-toggle-20260708-202659`

That run was intentionally instrumented to prove the old failure and the recovery path. The log first proved the remaining bug after presentation-upgrade hardening:

```text
Ray reconstruction provider initialization failed for NVIDIA DLRR: Streamline native device setup failed: Result::eErrorInvalidIntegration
```

After moving native device setup into the shared Streamline runtime, the same run showed DLRR provider initialization and first produced output after each re-enable:

```text
[20:28:18.067] Ray reconstruction provider initialized for NVIDIA DLRR.
[20:28:18.182] DLRR provider produced output after initialization.
[20:28:38.118] Ray reconstruction provider initialized for NVIDIA DLRR.
[20:28:38.236] DLRR provider produced output after initialization.
```

### Clean Final Verification

Artifact: `artifacts/verification/dlrr-toggle-clean-20260708-203833`

The clean build verification drove the real editor console:

```text
GetCVar r.RayReconstruction.Mode
SetCVar r.RayReconstruction.Mode 0
SetCVar r.RayReconstruction.Mode 1
SetCVar r.RayReconstruction.Mode 0
SetCVar r.RayReconstruction.Mode 1
GetCVar r.RayReconstruction.Mode
```

Evidence files:

- `01_off_noisy_viewport.png` -> `02_off_to_on_smooth_viewport.png` proves `Off -> On`.
- `03_on_off_noisy_viewport.png` -> `04_on_off_on_smooth_viewport.png` proves `On -> Off -> On`.
- `05_console_final_full.png` shows the command sequence in the editor console.
- `verification_report.txt` records the command sequence, screenshot paths, noise scores, and log scan result.
- `ShowcaseEditor_clean.log` was copied from the run; the scan found zero matches for `Ray reconstruction provider initialization failed`, `Streamline native device setup failed`, `eErrorInvalidIntegration`, warning, error, or fatal log lines.

Noise score from the saved viewport crops, lower is smoother:

```text
01_off_noisy:             3.432
02_off_to_on_smooth:      2.874
03_on_off_noisy:          3.379
04_on_off_on_smooth:      2.859
```

## Architecture Rules Going Forward

- Frame graph keys should encode topology only. Do not include tunable provider parameters unless they change pass/resource topology.
- Streamline/NGX ownership should live at renderer-system lifetime, not per settings state.
- Provider toggles may free feature resources, but must not tear down the Streamline runtime or presentation hooks.
- Provider quality/mode labels must be backed by matching render-resource contracts. Do not expose lower-resolution quality modes unless the renderer allocates lower-resolution input resources and reports those extents to Streamline.
- Permanent logs should report requested-provider initialization failures. Success/fallback proof belongs in targeted verification artifacts, not in per-frame provider passes.
- Streamline presentation hooks are owned by the RHI swap-chain lifetime. Provider initialization may request the bridge, but repeated requests must be idempotent.
- Streamline native device binding is owned by the shared Streamline runtime lifetime. Feature providers must not rebind the device during settings transitions.
