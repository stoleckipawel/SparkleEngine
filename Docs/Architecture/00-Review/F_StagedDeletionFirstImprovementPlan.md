# F. Staged Deletion-First Improvement Plan

Status: staged architecture refinement plan
Date: 2026-07-04
Scope: actions derived from the whole-repo map, external reference comparison, and advanced graphics requirements

## Prime Directive

Every implementation change in this plan should be net-negative in code or depot weight unless explicitly approved as a foundational replacement. The default acceptance rule is:

- deleted source lines > added source lines
- deleted public API count > added public API count
- deleted files >= added files
- deleted depot bytes > added depot bytes
- no new wrapper layer unless it removes more code than it introduces
- no cosmetic-only changes
- no new documentation, logs, validation panels, diagnostic reports, scaffolding, wrappers, or debug artifacts as substitutes for real product behavior
- no new abstraction unless it removes more code than it introduces in the same change series

Keep these:

- fatal API/result checks
- shader/cooked package ABI checks
- graphics API debug layer support
- PIX/RenderDoc/Nsight markers, object names, and GPU timing
- screenshot/BMP capture as a hardened editor/tool capability with a narrow owner and low runtime cost
- allocator budget/pressure facts that drive runtime policy
- one small, real build/cook/run workflow

Delete or collapse these unless they are product-owned:

- smoke harnesses
- report generators
- debug artifact bundles
- default diagnostic summaries
- future feature scaffolding
- public observation APIs
- duplicated enum/name/debug tables
- heavy content in the core repo

## Persona Alignment

This plan should grow the engine and the engineer in the same direction described in `H_AdvancedGraphicsEngineerPersona.md`.

The target persona is not proven by more text. It is proven when Sparkle's codebase becomes a smaller, sharper advanced graphics engine:

- explicit D3D12/Vulkan ownership
- renderer feature depth across RT, GI/path tracing, post-processing, denoising, upscaling, shaders, and passes
- strong shader/cook/runtime ABI discipline
- professional debugging and capture support
- neural rendering readiness without runtime ML bloat
- public APIs that expose behavior, not status noise
- feature additions that delete or simplify nearby code

Use this control matrix when deciding whether a cleanup or feature change belongs in the staged plan:

| Persona pillar | Engine direction | Preserve | Remove or avoid |
| --- | --- | --- | --- |
| Explicit graphics API ownership | Keep D3D12/Vulkan concepts recognizable through RHI and frame graph. | Backend services, descriptors, barriers, memory allocators, ray tracing services, native debugger support. | Wrapper layers that hide the real API without deleting code. |
| Renderer feature depth | Make real renderer features coherent before measuring them. | RT, GI/path tracing, post-processing, denoising, upscaling, screenshot capture, shader passes. | Feature placeholders, future-pack scaffolding, panels that only make incomplete work look complete. |
| Shader/kernel craft | Keep shader compiler/cook/runtime ABI as a centerpiece. | HLSL/Slang path, reflection, package load, layout safety, shader libraries. | Default debug bundles, duplicate demo shaders, stats files that do not drive decisions. |
| GPU architecture thinking | Keep compact facts that affect runtime policy. | GPU markers, timestamps, object names, allocator pressure, descriptor pressure if consumed by policy. | New profiling systems before feature cleanup; broad reports by default. |
| Neural rendering readiness | Stay ready for inference-like renderer features. | Slang/HLSL flexibility, provider-shaped resources, denoising/upscaling/ray reconstruction paths. | PyTorch/TensorFlow/ONNX Runtime integration before a feature needs it. |
| Debugging and tool fluency | Keep professional tools and capture paths reliable. | PIX/RenderDoc/Nsight markers, backend debug layers, screenshot/BMP capture. | Smoke-owned capture, validation panels, durable diagnostic artifacts. |
| Product engineering discipline | Make the repo easier for another engineer to trust. | Small public API, clear ownership, curated levels, launcher workflows. | Uncataloged content, catch-all launcher UI, public observation surfaces. |

Stage-to-persona mapping:

| Stage | Persona capability exercised | Engine result expected |
| --- | --- | --- |
| 0 | Product judgment | Existing docs state a renderer-first identity without creating new planning sprawl. |
| 1 | Repository discipline | Stale docs/logs/CMake noise are removed so review starts from trustworthy files. |
| 1A | Engineering restraint | No new docs, diagnostics, wrappers, validation, or scaffolding enter the near-term path. |
| 2 | Product/content hygiene | Many levels remain supported, but large content is cataloged and optional. |
| 3 | Debugging/tool judgment | Fatal checks, debugger hooks, and screenshot capture survive; smoke/report systems do not. |
| 4 | Tool productization | Launcher becomes build/cook/run/clean/package if owned, not a diagnostic cockpit. |
| 5 | API taste | Renderer/RHI public surfaces shrink around behavior and capability. |
| 6 | Ray tracing depth | Classic TLAS and PTLAS both work; PTLAS becomes minimal, direct, and backend-real. |
| 7 | Rendering correctness | Reference path tracing has a clear purpose and does not accumulate ambiguous buffers. |
| 8 | Shader craft | Shader debug/demo duplication is removed while ABI discipline remains. |
| 9 | Engine boundary discipline | Core/GameFramework expose fewer implementation details. |
| 10 | Feature ownership | RT, GI/path tracing, post-processing, denoising, upscaling, frame graph, shaders, and passes are hardened before profiling. |
| 11 | Measurement maturity | Profiling uses existing professional hooks after the feature surface is worth measuring. |
| 12 | Product packaging | Build/package outputs reflect real product ownership. |
| 13 | Strategic readiness | The next feature can land without broadening the repo by default. |

No stage is complete if it only makes Sparkle look more advanced. A stage is complete when the engine becomes easier to understand, easier to modify, and closer to the advanced graphics persona through less code, clearer ownership, or a real preserved capability.

Common gate for every refactor batch:

1. Persona pillar: name the exact pillar developed by the change.
2. Engine capability: name the preserved or improved capability.
3. Code/depot delta: identify what source, public API, files, or content weight is removed.
4. Backend impact: state whether D3D12, Vulkan, or both are touched.
5. Shader impact: state whether shader source, offline cooked shader packages, reflection, or cook output changes.
6. Runtime impact: state whether frame graph, RHI services, renderer passes, GameFramework, launcher, or cookers are touched.
7. Public/private impact: shrink public API or justify why the public surface remains.
8. Tooling impact: preserve PIX/RenderDoc/Nsight markers and screenshot/BMP capture where relevant.
9. Content impact: preserve multi-level support and avoid uncataloged heavy assets.
10. Profiling impact: avoid new measurement systems unless this is the late measurement stage.
11. No-pollution check: no new docs, logs, validation systems, wrappers, diagnostic panels, or future scaffolding.
12. Exit condition: any build/run/cook risk is recorded for final stabilization; full build/cook/run is deferred to the final readiness pass unless a checkpoint is intentionally chosen.

If a batch cannot pass this gate, split it until it can. The preferred batch is small enough to review, but meaningful enough to delete code or harden a real renderer feature.

## Stage 0: Declare The Product Line

Priority: immediate
Expected code delta: no source change; update existing planning text only
Decision required: yes

Choose one primary identity:

| Option | Meaning | Code consequence |
| --- | --- | --- |
| Renderer-first engine | Sparkle is a compact engine focused on realtime/path-traced rendering, editor viewport, shader pipeline, and cooked showcase levels. | Keep Engine, Renderer, RHI, shader compiler, minimal cookers, minimal launcher, and curated multi-level support. |
| Rendering framework plus sample | Sparkle is closer to Donut/Cauldron: framework, passes, RHI, sample app. | Shrink GameFramework, Editor, Application, Launcher, and asset pipeline aggressively. |
| Full game engine prototype | Sparkle owns runtime/editor/tools/content as product. | Keep more systems, but every public API must be product-quality and documented. |

Recommended choice:

- Renderer-first engine.

Acceptance criteria:

- [ ] Existing review docs state what Sparkle is, what it is not, and which workflows are first-class.
- [ ] All later deletion stages cite this identity.

## Stage 1: Repository Hygiene And Review Surface

Priority: immediate
Expected delta: small negative
Risk: low

Actions:

1. Keep the docs index accurate.
2. Remove stale references to deleted review docs.
3. Remove generated logs that are tracked.
4. Remove mojibake/decorative CMake comments that reduce trust.
5. Remove duplicated CMake source groups.
6. Keep `.github/workflows` empty unless CI is actively owned.

Candidate files:

- `Docs/README.md`
- `Engine/CMakeLists.txt`
- `Engine/RHI/CMakeLists.txt`
- `Projects/Showcase/StreamlineLogs/sl.log` if tracked

Acceptance criteria:

- [ ] `Docs/README.md` links only existing docs.
- [ ] No tracked `.log` files remain.
- [ ] `rg "mojibake|deleted review doc|stale review doc" Docs CMake Engine` has no stale doc/index hits unless intentionally retained in history notes.
- [ ] Deleted/changed lines exceed added lines.

## Stage 1A: Establish No-New-Pollution Rule

Priority: immediate
Expected delta: negative or zero
Risk: low

Problem:

- The repo already has enough planning material for the next cleanup pass.
- Adding documentation, diagnostics, logs, validation, scaffolding, wrappers, or abstractions now would contradict the cleanup goal.
- Feature work is allowed, but only when it hardens or simplifies the existing renderer/RHI/shader/frame-graph context.

Actions:

1. Do not add new docs or policy notes unless they replace existing planning text.
2. Do not add new diagnostics, logs, validation paths, report formats, wrapper layers, or future-feature scaffolding.
3. Keep shader compiler/cook/runtime ABI as an existing strength; harden it by deletion and simplification only.
4. Prioritize renderer/RHI/frame-graph/shader/pass cleanup before measurement-only work.
5. Allow new rendering features only when their integration is direct, contextual, and net-simplifying.

Acceptance criteria:

- [ ] `Docs/Architecture/00-Review/G_AdvancedGraphicsEngineExecutiveSummary.md` exists and is company-neutral.
- [ ] No new runtime logs, panels, or report systems are introduced.
- [ ] No new documentation files are added for near-term work.
- [ ] No new validation, contract, or scaffold layer is added.
- [ ] Deleted or consolidated code exceeds any feature support code added.

## Stage 2: Organize Multi-Level Content Without Repo Pollution

Priority: highest depot-size win
Expected delta: very large depot byte reduction
Risk: medium, because project launch/cook paths must still work

Problem:

- `Projects` is about 2788.88 MB, mostly Showcase assets.
- This dwarfs source code weight and makes every clone/review carry content cost.

Actions:

1. Keep multi-project and multi-level support as a first-class capability.
2. Define a curated default level set for normal build, cook, run, and review workflows.
3. Move Bistro/heavy textures/meshes/HDRIs into one or more optional content packs:
   - separate repo,
   - release zip,
   - Git LFS,
   - or media delivery script.
4. Add level/content catalog metadata with pack name, URL/hash/version if needed, tags, and default-workflow inclusion.
5. Make launcher/cooker able to select any available level and show "optional content missing" without failing the default level set.
6. Delete heavyweight assets from the core repo only after they are represented by cataloged optional packs.

Candidate paths:

- `Projects/Showcase/Assets/Meshes/Bistro`
- large DDS/TGA texture sets
- large HDR skyboxes if not essential for the curated default level set

Acceptance criteria:

- [ ] Core repo size drops by at least 1 GB.
- [ ] Multiple Showcase levels remain supported and selectable.
- [ ] Default Showcase level set launches and cooks without optional packs.
- [ ] Optional heavy levels/content are discoverable through one documented path.
- [ ] Missing optional packs fail gracefully without breaking default build/cook/run.
- [ ] No source code increases except small manifest/resolver logic; depot bytes are strongly net-negative.

## Stage 3: Delete Smoke, Validation, And Report Scaffolding

Priority: highest code-slimming win
Expected delta: large negative
Risk: medium

Problem:

- The engine has valuable fatal guardrails and profiler/debug support, but also report/test/scaffold surfaces.
- These make the repo feel larger and less product-focused.

Actions:

1. Delete application-level RHI smoke validation if still present.
2. Delete launcher smoke orchestration if still present.
3. Delete renderer smoke snapshot APIs if still present.
4. Preserve RHI screenshot/BMP capture, but harden and narrow it so smoke/ad hoc paths do not own the API.
5. Delete AssetCooker plan/timing diagnostic artifacts from default cook.
6. Delete shader debug artifact bundles from default launcher/cook path.
7. Delete duplicated debug view name/parser tables after smoke deletion.

Candidate areas:

- `Engine/Application/Private/Validation/RhiSmoke*`
- `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/*`
- `Engine/Renderer/Public/Diagnostics/*Smoke*`
- screenshot/BMP capture call sites that are only smoke/ad hoc paths
- `Tools/Cooking/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp`
- `Tools/Cooking/AssetCooker/Private/Discovery/AssetCookerDiscovery.*`
- shader debug artifact writers and launcher toggles

Acceptance criteria:

- [ ] `rg "RhiSmoke|SmokeDiagnostics|SPARKLE_SMOKE|asset-cooker-plan-v1|asset-cooker-summary-v1|debug-artifacts"` returns only intentionally preserved product references.
- [ ] `CaptureTextureToBmp` and BMP writer references remain only in the hardened screenshot/capture capability.
- [ ] Normal editor/runtime launch still works.
- [ ] Shader cook still writes runtime shader packages.
- [ ] Asset cook still writes cooked assets and clear fatal errors.
- [ ] PIX/RenderDoc/Nsight markers and GPU timing remain.
- [ ] Deleted files and lines greatly exceed added lines.

## Stage 4: Slim The Launcher Into A Workflow Shell

Priority: high
Expected delta: large negative
Risk: medium-high because launcher is user-facing

Keep:

- build workspace
- build editor/runtime
- prepare cook tools
- cook shaders/assets
- open editor/runtime
- clean generated outputs
- package only if release packaging is an active product workflow

Cut or collapse:

- diagnostic-only pages/actions
- advanced shader debug artifact controls
- cooked shader stats CSV controls unless actively used
- large status pages that mirror command output
- source dependency sync tiers that are not needed now
- package assembly UI if package release is not currently owned
- repeated GUI branches for each operation when operation metadata can drive them

Candidate files:

- `Tools/Launcher/SparkleLauncher/Private/Gui/Shell/LauncherMainWindowOptionPages.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/Shell/LauncherMainWindowStatusPages.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Shell/LauncherShell.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Build/*Planner*`
- `Tools/Launcher/SparkleLauncher/Private/Cook/*`
- `Tools/Launcher/SparkleLauncher/Private/Gui/Models/*`

Acceptance criteria:

- [ ] Launcher still builds.
- [ ] One command opens editor/runtime.
- [ ] One command cooks required assets/shaders.
- [ ] One command cleans generated outputs.
- [ ] Advanced debug/diagnostic toggles are gone unless tied to a product workflow.
- [ ] Launcher source lines drop by at least 25%.

## Stage 5: Narrow Public Renderer And RHI Observation APIs

Priority: high
Expected delta: medium negative
Risk: medium

Problem:

- Public APIs currently include memory/mesh/texture diagnostics and capture.
- Public observation APIs make internal state look supported forever.

Actions:

1. Keep `Renderer` public render/product/frame APIs.
2. Move editor-only diagnostics behind an editor-private provider adapter or delete panels that are not product-critical.
3. Preserve screenshot/BMP capture capability through `RhiCaptureService` or a narrower product-owned capture entrypoint.
4. Collapse memory/descriptor snapshots into one compact "runtime pressure/status" object if needed.
5. Keep RHI debug layer, object names, GPU events, timestamp queries, failure diagnostics.
6. Remove public JSON dump/detail allocation lists unless they drive runtime policy.

Candidate files:

- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Public/Diagnostics/RendererMemoryDiagnostics.h`
- `Engine/Renderer/Public/Meshes/MeshDiagnostics.h`
- `Engine/Renderer/Public/Resources/Textures/TextureDiagnostics.h`
- `Engine/RHI/Public/Capture/RhiCaptureService.h`
- `Engine/RHI/Public/Diagnostics/RhiDiagnostics.h`
- `Engine/RHI/Public/Descriptors/RhiDescriptorService.h`
- editor panels consuming diagnostics

Acceptance criteria:

- [ ] Renderer public header count or public line count decreases.
- [ ] RHI public services decrease or split into smaller explicit runtime/dev surfaces.
- [ ] Editor still shows only product-owned panels.
- [ ] No new "diagnostics facade" replaces deleted APIs.
- [ ] GPU marker/timing/debug-layer support remains.

## Stage 6: Refactor TLAS And PTLAS As Equal RT Features

Priority: high
Expected delta: medium negative through PTLAS simplification
Risk: high because both D3D12 and Vulkan ray tracing paths must remain functional

Decision:

- Classic TLAS and PTLAS are both product ray tracing features.
- Users should be able to select either classic TLAS or PTLAS on supported D3D12 and Vulkan paths.
- PTLAS should be refactored down to the smallest functional implementation that reflects the original PTLAS reference shape.

Actions:

1. Preserve classic TLAS build/update/trace behavior.
2. Preserve PTLAS build/update/trace behavior for D3D12 and Vulkan where backend support exists.
3. Delete future GPU-pack placeholder passes/resources.
4. Delete PTLAS planner metrics, diagnostic structs, and CPU validation readback paths that are not required to build and trace PTLAS.
5. Collapse PTLAS strategy code toward the original minimal flow: capability check, compact descriptor input, backend build/update call, resource lifetime, and trace usage.
6. Keep only the user-facing selection policy needed to choose classic TLAS or PTLAS.
7. Avoid adding new abstraction layers around acceleration structure selection.

Candidate files:

- `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.*`
- `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPtlas*`
- `Engine/Renderer/Private/RayTracing/Diagnostics/RayTracingPtlas*`
- `Engine/RHI/Public/RayTracing/RhiPartitionedTlasDesc.h`
- D3D12/Vulkan PTLAS service files

Acceptance criteria:

- [ ] Classic TLAS remains functional.
- [ ] PTLAS remains functional on supported D3D12 and Vulkan paths.
- [ ] User-facing selection can choose classic TLAS or PTLAS without extra diagnostics/scaffolding.
- [ ] `rg "future GPU pack|placeholder|AllowCpuValidationReadback"` returns no default-path scaffolding.
- [ ] Ray tracing scene renders with classic TLAS and with PTLAS where supported.
- [ ] PTLAS code is smaller and closer to the original minimal reference implementation.
- [ ] Net source lines decrease.

## Stage 7: Make Reference Path Tracing Honest

Priority: medium-high
Expected delta: medium negative unless progressive accumulation is chosen
Risk: medium

Choose one:

| Option | Meaning | Code direction |
| --- | --- | --- |
| Debug reference | Simple, deterministic comparison path. | Delete provider handoff attempts, reconstruction expectations, extra guide products not used. |
| Offline/progressive reference | High-quality accumulation path. | Add only if it replaces/deletes current ambiguous paths; otherwise defer. |
| RTXPT-style product path | Pure path tracer with guide buffers and DLSS-RR. | Major feature; not deletion-first unless replacing deferred/reference complexity. |

Recommended:

- Debug reference first.

Actions:

1. Keep reference path's self-owned guide outputs only if consumed by product features.
2. Delete reference ray reconstruction hooks if motion vectors remain unsupported.
3. Delete settings/CVars that make it look like a second realtime renderer without product quality.
4. Share BRDF/material/light policy with realtime path.

Acceptance criteria:

- [ ] Reference mode has a one-sentence product role.
- [ ] No dead provider handoff remains.
- [ ] Reference-specific buffers/settings are fewer or explicitly consumed.
- [ ] Net source lines decrease.

## Stage 8: Slim Shader Debug And Duplication

Priority: medium-high
Expected delta: medium negative
Risk: medium

Keep:

- shader package ABI validation
- reflection extraction
- shader parameter struct verification
- DXC/Slang backends
- package inspection commands

Cut:

- default debug artifact bundles
- cooked shader stats CSV unless actively used
- HelloWorld/ComputeClear sample shader registrations from product runtime if only validation-oriented
- duplicated render view mode parser/name tables
- launcher UI switches for compiler diagnostics not used by daily workflows

Actions:

1. Keep shader compiler CLI flags for power users if they do not force runtime code.
2. Remove launcher exposure for rare debug outputs.
3. Collapse duplicate exposure shader registrations where possible.
4. Audit every shader package for product ownership.

Acceptance criteria:

- [ ] Runtime shader registrations include only product passes.
- [ ] Default shader cook writes runtime packages only.
- [ ] Debug outputs are absent by default.
- [ ] No generator is added unless it deletes more code than it creates.

## Stage 9: Reduce Core And GameFramework Public Surface

Priority: medium
Expected delta: medium negative
Risk: medium

Problem:

- `Core` is about 49.3% public by lines.
- `GameFramework` is about 28.0% public by lines.
- Public headers imply stable engine API.

Actions:

1. Re-run include/symbol searches for each public header.
2. Delete unused public headers.
3. Move loader/manifests/parsers private if only runtime loaders use them.
4. Collapse tiny validator/helper headers into implementation files.
5. Keep only scene/component/asset types that projects truly include.

Candidate areas:

- `Engine/Core/Public`
- `Engine/GameFramework/Public`
- `Engine/GameFramework/Private/Assets/Loaders`
- `Engine/GameFramework/Private/Level/Parsing`

Acceptance criteria:

- [ ] Public header count decreases.
- [ ] `GameFramework` still loads cooked showcase scene.
- [ ] No replacement aggregate header is added.
- [ ] Net source lines decrease.

## Stage 10: Rendering Feature Hardening Before Measurement

Priority: high
Expected delta: negative to neutral; feature additions must pay for themselves by deleting or simplifying surrounding code
Risk: medium

Rule:

- Before profiling/measurement projects, make the feature surface worth profiling: ray tracing, GI/path tracing, post-processing, denoising, upscaling, RHI, renderer frame graph, shaders, and passes.

Candidate improvements:

1. Harden the ray tracing and GI path around two product acceleration paths: classic TLAS and PTLAS.
2. Simplify post-processing and denoising pass ownership so pass inputs/outputs are obvious.
3. Preserve upscaling/ray reconstruction provider capability, but trim provider diagnostics and fallback scaffolding.
4. Reduce frame-graph and shader-pass duplication without adding a replacement graph abstraction.
5. Keep screenshot/BMP capture as a low-cost editor/tool capability.

Acceptance criteria:

- [ ] Feature-facing renderer/RHI code is smaller or simpler.
- [ ] No new docs, diagnostics, logs, validation systems, wrappers, or abstractions are added.
- [ ] Screenshot/BMP capture still works through the hardened path.
- [ ] Ray tracing/GI/path tracing/post-processing/denoising/upscaling paths remain buildable.

## Stage 11: Late CPU/GPU Profiling And Measurement

Priority: late
Expected delta: neutral to negative; must be justified by measured performance
Risk: medium

Rule:

- Do not add new performance systems until feature cleanup is complete and there is enough renderer capability worth measuring.
- Prefer PIX/RenderDoc/Nsight, existing markers, and existing timestamps over new in-engine reporting.

Candidate improvements:

1. Cache frame graph compiled topology when pass topology does not change.
2. Delete per-frame graph compile work that only repeats static dependency planning.
3. Keep per-frame resource binding/setup where frame data changes.
4. Remove dynamic strings/allocation-heavy pass names from hot paths only if simple.
5. Keep GPU timing scopes, but remove verbose diagnostic state not used by decisions.

Acceptance criteria:

- [ ] Before/after CPU frame time or profile evidence exists.
- [ ] Source line delta is non-positive, or an exception is explicitly approved.
- [ ] GPU markers/timestamps still work.
- [ ] No new log/report panel is added.

## Stage 12: Product Package Contract

Priority: medium
Expected delta: negative if package scope is narrowed
Risk: medium

Decide which packages are real:

- runtime
- editor
- launcher
- dev tools
- symbols
- optional showcase content

Actions:

1. Keep only package assembly code for real packages.
2. Remove "future manifest" fields and release note generation that is not used.
3. Move content pack out of runtime package.
4. Keep checksums/manifests only if a release workflow consumes them.

Candidate files:

- `CMake/SparkleReleaseAssembly.cmake`
- launcher package workflow files
- artifact contract CMake files

Acceptance criteria:

- [ ] One package command produces only real outputs.
- [ ] Package docs match generated layout.
- [ ] No "assembled for review only" placeholders remain in product package output, unless package assembly is explicitly a review-only workflow.
- [ ] Net package/build code decreases or package output bytes decrease.

## Stage 13: Final Architecture Readiness Bar

A cleanup series is complete when:

- [ ] Core repo size is much smaller, primarily by moving uncataloged heavy Showcase content.
- [ ] Multi-level project support remains intact through cataloged level/content selection.
- [ ] Public renderer/RHI API is smaller.
- [ ] Launcher source is smaller and workflow-focused.
- [ ] Default cook path writes assets, not diagnostics.
- [ ] Default shader cook writes runtime packages, not debug bundles.
- [ ] Classic TLAS and PTLAS are both product-owned and selectable where supported.
- [ ] Reference path has a clear role.
- [ ] Frame graph remains the one render scheduling abstraction.
- [ ] RHI remains explicit and backend-native usage remains private/provider-bridged.
- [ ] Profiler/debugger support remains strong.
- [ ] Screenshot/BMP capture remains preserved, hardened, and low-cost.
- [ ] The next feature addition can delete or replace something old.

## Bold First Cuts

If I were approving the first real cleanup batch, I would pick these:

1. Catalog `Projects/Showcase` levels and externalize heavy Bistro/media content.
2. Preserve and harden screenshot/BMP capture while deleting smoke/ad hoc capture ownership.
3. Remove AssetCooker plan/timing summary artifacts from default cook.
4. Remove launcher shader debug artifact/stat toggles from the default GUI.
5. Refactor PTLAS to the minimal functional D3D12/Vulkan implementation and delete future GPU-pack placeholders.
6. Move renderer mesh/texture/memory diagnostics out of public API unless the editor panels are declared product features.
7. Cut launcher operation/status UI until it is just build, cook, run, clean, package if owned.

These are not cosmetic. They reduce depot size, public API, runtime/tool code, and product ambiguity.
