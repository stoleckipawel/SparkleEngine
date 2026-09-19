#Graphics Feature Coverage Matrix

**Status : **capability snapshot;
horizontal cross - system inventory;
not release approval
    or executable evidence

            ** Snapshot : **2026
                          - 09
                          - 19 at source input revision `a884b6946802e933fafc9fe4c6cdfb93c4cde7e4` plus the current frame
                          - composition,
    visualization, and presentation worktree;
current RHI, Renderer, shader - registration, settings, and build surfaces inspected;
evidence `S` only

**Scope:** how each selectable graphics capability crosses user/configuration entry points, Renderer passes and resources, shader programs, RHI prerequisites, backend-specific limits, fallback behavior, and evidence gaps

**Owners:** `Engine/RHI`, `Engine/Renderer`, `Tools/Shaders`, and their current producer modules named per row

**Module inventories:** [RHI](../Modules/Engine/RHI/CapabilityInventory.md), [Renderer](../Modules/Engine/Renderer/CapabilityInventory.md), and [Shader Compilation](../Modules/Tools/ShaderCompiler/README.md)

**Vertical companion:** [Feature Execution Traces](FeatureExecutionTraces.md)

**Release classification authority:** [First Release Acceptance Contract](../../Acceptance/FirstRelease.md)

## Reading The Matrix

“D3D12” or “Vulkan” below means that the current source has an eligible implementation path behind runtime capability checks. It does not mean that the path was built or exercised on the snapshot date. “None” under fallback means the selection becomes unavailable or fails instead of silently becoming a non-ray implementation.

## End-To-End Feature Coverage

| Product capability | Selection and default | Renderer production path | Shader/traversal path | Required runtime capability | D3D12 source eligibility | Vulkan source eligibility | Fallback or failure boundary | Evidence still missing |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Raster GBuffer | `r.GBuffer.Algorithm=Rasterized`;
default | Batched mesh draw into seven GBuffer targets, then sky motion and scene - depth compute | `GBufferVS` + `GBufferPS`;
bindful per - material texture table | Graphics pipeline, required attachment formats,
    vertex / index resources | Yes | Yes | This is an explicit GBuffer algorithm,
    not an automatic fallback from unavailable ray tracing | Backend render / capture and raw attachment comparison | | Visibility
    and raster draw preparation | `r.MeshAutoBatching`;
on by default;
frustum visibility always runs | Per - view AABB / frustum classification, candidate validation, authored - group preservation,
    stable opaque sort / batch and transparent single - item distance sort | CPU preparation;
accepted batches feed `GBufferVS` + `GBufferPS` | Prepared scene / view plus valid GPU mesh / material / group identity
    | Common CPU path before D3D12 lowering | Common CPU path before Vulkan lowering | Task failure clears the full raster list;
invalid candidates reject;
no occlusion / LOD / GPU - driven / stereo fallback is implied | `REN - E32` analytic visibility, exact batch ledger, batching equivalence,
    failure and negative reachability | | Ray GBuffer
    | `r.GBuffer.Algorithm = RayTracing` | TLAS pass,
      exactly one ray GBuffer frontend, sky motion,
      scene depth | Shared automatic resolver prefers native Pipeline and otherwise selects Inline | AS + descriptor indexing + pipeline
    or inline query | Tier - dependent | Extension / feature / function - pointer dependent
        | Absence of a complete frontend rejects graph preparation rather than substituting raster | Both adapters,
      unavailable - device behavior,
      and raster semantic parity | | ReSTIR direct lighting | always - owned real - time lighting route | Temporal reservoir,
      spatial reservoir, visibility, direct resolve | Reservoir compute;
visibility uses the same automatic Inline / Pipeline resolver | AS + descriptor indexing; inline or pipeline for visibility | Tier-dependent | Feature-dependent | Shadow frontend has no raster shadow-map fallback | Static/motion reuse, bias, disocclusion, frontend parity |
| ReSTIR indirect lighting | part of the real-time lighting route | Temporal reservoir, spatial reservoir, resolve;
writes indirect lighting and optional reconstruction guides | Three compute shaders containing inline ray queries
    | AS + inline ray query + descriptor indexing | DXR tier 1.1 | Vulkan ray query
    | No native RT - pipeline adapter and no non - ray fallback | Bounce edges,
    convergence / ghosting, backend coverage | | Reference Path Tracer | Per - view `RenderViewMode::ReferencePathTracer`;
Editor and Game / runtime submit the same rendering semantic | Feature - local GPU transport, session, identity, resources,
    and accumulation inside the ordinary `FramePipeline` shell
    | Shared automatic frontend resolver plus one Reference kernel and thin Inline / Pipeline adapters | Ray tracing plus the feature's documented material/light domain | Source-present; executable proof pending | Source-present; executable proof pending | No Lit estimator fallback or relabeling; active route remains distinct from completed reference output | Stage-local source checks now; later UX, backend parity, oracle, statistical, and release gates |
    | Lighting composite and sky | real - time lighting route | Three direct and two indirect lobes composite into `SceneColor`;
sky fills background from scene depth | `LightingCompositeCS`, then `SkyCS` | Compute, scene / lighting formats,
    sky texture | Yes | Yes | Sky is not volumetric atmosphere | Raw lobe accounting and backend image evidence | | Volumetric lighting,
    fog, and atmosphere | no selector or current product | No media / fog payload, integration pass, volumetric target / history,
    atmosphere, or aerial - perspective stage | No registered volumetric shader | No owned RHI requirement because the feature is absent
        | No | No | Explicitly unavailable;
sky, alpha masking,
    and wrap subsurface are not fallbacks | `REN - E24` negative / reachability audit before any future claim | | Deferred decals
        | no selector
    or current product | No authored / scene decal data,
    post - GBuffer resolve, or secondary - ray composition;
target design is separate | No registered decal shader | No current additional RHI contract | No | No | Explicitly unavailable;
ordinary alpha blending is not a GBuffer - material fallback | `REN - E25`;
later delivery / acceptance only after roadmap admission | | Automatic / manual exposure | Automatic default;
manual and automatic settings / CVars | Reduction / downsample moment chain, 1x1 exposure resolve,
    persistent history | Four reduction variants + `ExposureCS` | Compute,
    UAV / SRV on float textures | Yes | Yes | History invalidates on view / topology resets;
no documented CPU exposure fallback | Step response, cut / resize reset, limits and finite output | | Resolution, sampling,
    and anti - aliasing policy | viewport extent or window fallback;
Linear default;
DLSS quality modes when eligible | Output extent then provider - resolved render extent;
optional lighting denoising precedes exactly one presentation reconstruction;
exact diagnostics replace filtered reconstruction with point selection | Current scene shaders plus point, Linear, or provider evaluation;
no standalone AA shader | Valid extents, compatible attachment / pipeline sample count,
    optional provider capability | Linear / single - sample eligible;
DLSS / RR capability - gated | Linear / single - sample eligible;
external provider evaluation refused | Provider failure resolves Linear / Off;
no MSAA / TAA / FXAA / SMAA / dynamic - resolution path may be inferred
    | `REN - E33` dimension / sample / provider / history and negative - mode matrix | | Linear upscaling | `r.Upscaler.Provider = Linear`;
default | Compute creates `ResolvedSceneColor` at output extent | `LinearUpscaleCS` | Compute and `R16G16B16A16_Float` SRV / UAV | Yes | Yes
    | Always - available engine - owned baseline path | Scale - factor quality and timing | | Exact - value point reconstruction
    | selected display - linear exact debug mode | Compute point - selects render - extent diagnostic color into `ResolvedSceneColor`
    | `PointUpscaleCS` | Compute and `R16G16B16A16_Float` SRV / UAV | Yes | Yes
    | Mode - domain policy selects this instead of filtered / provider reconstruction
    | Exact scalar / category values across extent ratios and backends | | NVIDIA DLSS Super Resolution | Provider `NVIDIA DLSS`;
quality NativeAA / Quality / Balanced / Performance / UltraPerformance
    | External - provider graph pass consumes color / depth / motion / exposure and writes resolved color | Streamline provider,
    not a global shader registration | Build option + SDK / binaries + D3D12 external evaluation + supported NVIDIA adapter / driver
        | Eligible when Streamline target is enabled
    and runtime checks pass | Not eligible : RHI explicitly reports external provider evaluation false
                                             | Initialization failure resets provider to linear
    and logs warning | Supported / unsupported hardware,
    every quality, resize / history, staged binaries | | NVIDIA DLSS Ray Reconstruction | `r.RayReconstruction.Mode`;
off by default; only constructed for ReSTIR graph | External RR consumes lighting/GBuffer/guide/history inputs and publishes denoised render-resolution scene color;
the selected presentation upscaler runs afterward | Streamline RR provider
    | Same external - provider gates plus ReSTIR mode and reconstruction guide production | Eligible when compiled / runtime - supported
    | Not eligible by current external - evaluation capability | Initialization failure disables RR;
graph only calls provider when present | Input tagging, provider loss, reset,
    quality / performance | | Tone mapping | ACES approximate default;
Reinhard and ACES fitted filmic also selectable
    | Output - resolution compute maps scene - referred HDR modes from resolved color plus exposure to a display - linear intermediate;
exact modes bypass the pass | `ToneMappingCS` | Compute;
float SRV / UAV | Yes | Yes | Invalid selection is a settings defect;
exact bypass is exhaustive presentation policy rather than a public `None` mapper | Numeric ramps, finite HDR values,
    all three mapper choices, exact - mode omission | | Color grading | no selector or current product | No grading parameter stack,
    transform / LUT asset path, pass, or editor workflow | No registered grading shader
        | No owned RHI requirement because the feature is absent | No | No | Explicitly unavailable;
fixed tone - mapper curves and transfer encoding are not grading | `REN - E26` negative / reachability audit before any future claim |
        | Chromatic aberration | no selector
    or current product | No lens / channel distortion model,
    pass, viewport setting, or history | No registered chromatic - aberration shader
        | No owned RHI requirement because the feature is absent | No | No | Explicitly unavailable;
reconstruction / filtering fringes are artifacts,
    not support | `REN - E27` negative / reachability audit before any future claim | | Frame generation | no selector
    or current product | No optical - flow / synthesis provider,
    generated - frame identity, UI policy, pacing, or extra presentation path | No DLSS - G
    or other frame - generation evaluation | No RHI generated - frame / swapchain contract | No | No | Explicitly unavailable;
Streamline PCL / Reflex, DLSS SR,
    and DLSS RR do not generate frames | `REN - E28` negative / provider / presentation audit before any future claim | | Output encoding
    | Automatic default;
Linear or sRGB explicit | Compute encodes display - linear intermediate,
    then copies to back buffer
    or viewport product | `OutputEncodingCS` | Output format UAV plus copy / present support | Yes | Yes
        | Current output contract has no HDR display transfer function
    or metadata path | Format / encoding pair tests and capture | | Debug buffer visualization
        | Per - view `ViewportRenderRequest::ViewMode`;
focused GBuffer, lighting, and instance modes share one mode domain with Lit, Wireframe,
    and Reference | `RenderViewBuilder` freezes the mode;
each family privately early - outs unless selected and overwrites scene color before presentation;
Wireframe is consumed by raster state | `GBufferVisualizationCS`, `LightingVisualizationCS`,
    or `GpuSceneVisualizationCS` reads only its family's inputs | Compute and selected mode | Yes | Yes | Scene-referred modes use normal display mapping; display-linear exact modes use point reconstruction and bypass exposure/tone mapping while retaining output encoding | Every mode, family isolation, C++/HLSL parity, two-view isolation, unavailable combinations, presentation semantics |
        | Viewport capture | Public capture request | Frame pipeline resolves a named viewport product and submits asynchronous readback
        | No dedicated shader;
RHI capture service | Readback + valid product / format | Yes | Yes | Invalid / unsupported request is rejected;
completion is polled | Known - color / size / channel evidence and depth - compatible case |

    ##Ray - Tracing Frontend And Top - Level - AS Matrix

    | Concern | Classic TLAS | Partitioned TLAS on D3D12 | Partitioned AS operations on Vulkan | | -- -| -- -| -- -| -- -|
    | Capability source | Standard RHI AS support populates build,
    update, GPU - readable instance - buffer support | NVAPI provider query on an NVIDIA device,
    after DXR availability | `VK_NV_partitioned_acceleration_structure` extension, feature,
    and loaded functions on NVIDIA | | Runtime selection | Selected whenever AS exists and a usable PTLAS descriptor path is not selected
        | Selected only when `r.RayTracing.PreferPartitionedTlas` requests it
    and the provider reports both support and descriptor access | Never selected by the current code:
capability records `SupportsDescriptorAccess = false`,
                   so selection falls back to classic with reason `vulkan - ptlas - descriptor - path - unavailable` |
    | Renderer update path | Initial build; optional refit/update for later rigid changes via `r.RayTracing.Tlas.Refit` | Renderer builds its partition operation strategy and routes the RHI build through NVAPI | Low-level create/size/build operation surface exists, but it is not the Renderer’s active TLAS binding provider |
| Shader binding | Standard acceleration-structure descriptor | Provider-selected partitioned-AS descriptor when supported | Vulkan binding-layout code can name the partitioned descriptor type, but current provider selection cannot reach it |
| Honest claim | Implemented, capability-gated, unexecuted | Vendor/API-gated partial path, unexecuted | Low-level vocabulary and operations exist;
selectable Renderer PTLAS is not currently supported |

        Native RT pipelines currently adapt two effects only : ray GBuffer
    and direct - shadow visibility.Reference direct / indirect lighting
    and ReSTIR indirect remain inline
            - query compute paths.

              ##Binding And Material Coverage

        | Material / resource concern | Raster GBuffer | Ray GBuffer | Direct shadow | Reference path | ReSTIR indirect |
        | -- -| -- -| -- -| -- -| -- -| -- -| | Texture binding model | Bindful material table | Fixed descriptor array
        | Fixed descriptor array | Fixed descriptor array | Fixed descriptor array | | Capacity | Eight semantic slots per material
        | Shared fixed capacity of 4096 texture descriptors | Same shared table | Same shared table | Same shared table | | Texture roles
        | Base color,
    normal, roughness, metallic, AO, emissive, subsurface color,
    subsurface strength | Hit material indices address the same authored roles through the shared array
    | Primarily base - color alpha rejection plus hit material lookup | Full hit - material evaluation | Full hit - material evaluation |
    | Descriptor prerequisites | Ordinary bindful layout | Non - uniform sampled - image indexing and partially - bound arrays | Same | Same
    | Same | | Alpha coverage | Opaque and alpha mask | Opaque and alpha mask;
any - hit handles rejection in pipeline mode | Any - hit / inline rejection | Hit evaluation respects mask data
    | Hit evaluation respects mask data | | Missing product coverage | No complete transparent blend / transmission pass | Same
    | No translucent shadow model | No complete transmissive path | No complete transmissive path |

“Bindless” therefore describes a fixed - capacity material - texture array on selected ray paths,
    not a runtime - sized engine - wide resource model
    and not the raster GBuffer path.

            ##Render
            - Target And History Contract

        | Resource family | Exact current members | Format
    and lifetime | | -- -| -- -| -- -| | GBuffer | BaseColor,
    Normal, Material, Emissive, Subsurface, DeviceZ,
    MotionVector | `R8G8B8A8_UNorm`, `R16G16B16A16_Float`, `R8G8B8A8_UNorm`, `R16G16B16A16_Float`, `R8G8B8A8_UNorm`,
    raster `D32_Float` or ray `R32_Float`, `R16G16_Float`;
per graph execution | | Lighting lobes | DirectDiffuse, DirectSpecular, DirectSubsurface, IndirectDiffuse,
    IndirectSpecular | ReSTIR uses `R16G16B16A16_Float`;
reference uses `R32G32B32A32_Float`;
per frame | | ReSTIR direct transients | Visibility, reservoir sample, reservoir weight | Each `R32G32B32A32_Float`;
per frame | | Reconstruction guides | DiffuseAlbedo, SpecularAlbedo, Roughness, SpecularHitDistance | Two RGBA16F plus two R32F;
scene extent when RR is enabled, otherwise 1x1 placeholders | | Scene / presentation | SceneColor, linear SceneDepth, ResolvedSceneColor,
    ToneMappedSceneColor, EncodedSceneColor | Scene / resolved / tone - mapped are RGBA16F;
linear depth R32F; encoded target uses the linear variant of the selected output format |
| Exposure history | Current exposure and prior exposure | 1x1 RGBA32F texture history |
| Reference Path Tracer history | Accumulated Reference Path Tracer output | Scene extent RGBA32F texture history |
| Reservoir history | Sample, Weight, Surface for direct and indirect reservoirs | Sample/Weight RGBA32F;
Surface RGBA16F; invalidated independently or as complete frame history |

History is invalidated for scene generation, graph topology, invalid temporal view state, and effect-specific prepared-scene hashes. Provider history is reset alongside relevant ReSTIR or view invalidation.

## Cross-Cutting Renderer Mechanisms

| Mechanism | Shared Renderer contract | Horizontal boundary |
| --- | --- | --- |
| typed pipeline materialization | registered/cooked shader metadata and typed parameters become capability-checked graphics/compute/ray layouts, pipelines, tables, and bindings;
complete graphics state participates in cache identity | D3D12 / Vulkan native validation, cache - key mutation, build membership,
    and reload retirement remain open under `REN - PIPE - *` | | mesh / texture residency
    | separate cache - owned state machines bound 16 concurrent CPU jobs,
    256 requests, 512 MiB decoded, 256 MiB pending upload, and 2 GiB resident by default;
tokens gate activation / eviction | defaults are per cache, not global pressure management;
fallback asymmetry, stalls,
    and heavy - content behavior remain open | | temporal sampling / history | each view owns the active 16 - frame Halton jitter,
    previous camera / jitter, common validity, nine invalidation causes, unjittered motion,
    and reprojection / provider convention | source - only patterns are not modes;
multi - view, backend, deformation, reset,
    and provider agreement remain open under `REN - TEMP - *` | | visibility / draw preparation | each prepared view owns frustum results,
    material routing, validated raster indices,
    authored / preserved / automatic / single batches and workload counts | transparent preparation is partial;
occlusion, LOD, GPU - driven / indirect,
    stereo and multiview are absent under `REN - VIS - *` | | resolution / sampling / AA | output extent, provider - resolved render extent,
    Halton jitter and active single - sample attachment policy join before one output - extent result
    | RHI multisample and dormant jitter vocabulary are not Renderer MSAA;
standalone post AA and dynamic resolution are absent under `REN - RESO - *` | | settings lifecycle | 26 - field requested snapshot,
    26 persisted CVar names, separately owned per - viewport view mode, startup - before - command - line ordering,
    and serial / render - thread commit | writer truncates in place and reports no parse / write failure;
package / durability and requested - versus - active closure remain open under `REN - SET - *` | | latency coordination
    | optional host simulation markers join D3D12 submit / present PCL markers;
supported Reflex sleeps at SimulationStart | no Vulkan equivalent, no selector / active report,
    64 - bit ID narrows to 32 - bit provider token,
    and no measured benefit under `REN - LAT - *` |

    ##Backend Contract Surface

    | RHI contract | D3D12 implementation | Vulkan implementation | Coverage boundary | | -- -| -- -| -- -| -- -| | Version / minimum
    | Requests feature level 12_1;
records actual feature level | Requires / selects Vulkan 1.3;
records API version | Actual supported adapter / driver set is unexecuted | | Descriptor model | Descriptor tables;
modeled as one set;
limits use Tier - 2 heap constants;
256 push - constant bytes | Descriptor sets;
limits from physical - device properties | Limits are reported,
    not yet release - tested at edges | | Descriptor indexing | Reports non - uniform sampled images
    and partially - bound arrays | Reports enabled device features | Renderer additionally demands the 4096 - entry material capacity |
        | Queues | Graphics,
    compute, copy all reported available and independent | Graphics required;
compute / copy and independence reflect selected families | Availability is not proof of overlap
    or synchronization correctness | | Upload / readback | Buffer upload,
    texture upload, readback | Same | Stress, row - pitch, and format matrix unexecuted | | Pipelines | Graphics, compute,
    ray tracing | Graphics, compute,
    ray tracing | Mesh / task shaders report false on both backends | | Presentation throttle | Frame - latency waitable object
    | Swap - chain image acquisition | Resize / pacing stability unexecuted | | Memory
    | D3D12 managed allocator plus budget / delayed - destruction diagnostics | Vulkan managed allocator plus equivalent queried diagnostics
    | Residency behavior is not measured | | External providers | Native device,
    queue, command list / resources / states;
evaluation enabled when handles are available | Native handles / resources / states exposed,
    but evaluation and runtime provider checks explicitly false | Current DLSS / RR release surface is D3D12 - only |
    | Latency - provider markers | Active D3D12 interposer can emit render - submit / present PCL markers;
host simulation markers enter the same optional Streamline runtime | No equivalent inspected Vulkan marker producer;
Streamline initialization refuses non - D3D12 | Capability - gated marker / sleep route only;
not backend parity, frame generation, or measured latency reduction | | Diagnostics | Object names, GPU events, timestamps, messages,
    live objects,
    crash data according to created service | Same common capability fields according to created service
    | Exact availability depends on validation / device / runtime support | | Device aggregate lifecycle
    | one owner - thread D3D12 service composition;
settle waits idle and clears recording allocation state;
resize replaces swapchain - dependent identity;
DRED may diagnose removal | one owner - thread Vulkan composition;
settle clears acquire state and waits idle;
resize / out - of - date recreates swapchain - dependent identity;
result handling names device loss | No in - process whole - device recreation was found;
swapchain recovery, orderly shutdown, crash diagnostics, and terminal device loss are distinct under `RHI-LIFE-*` |

## User-Reachable Control Surface

| Control | Values | Default | Product boundary |
| --- | --- | --- | --- |
| Backend CLI/env | `--renderer`, `--rhi`, or `--graphics-api`, as `key=value` or next token;
`SPARKLE_RHI_BACKEND`;
aliases D3D12 / DX12 / Direct3D12 and Vulkan / VK | Build - selected backend,
    normally D3D12 | Invalid explicit value resolves to Unknown and must fail visibly | | GBuffer | Rasterized,
    Ray tracing | Rasterized | Editor settings and CVar | | Ray - tracing frontend | automatic only | Pipeline when complete,
    otherwise Inline | one Renderer policy shared by GBuffer, direct shadows, and Reference Path Tracer;
no per - effect control | | Lighting | ReSTIR Path Tracing,
    Reference Path Tracer | ReSTIR | Both currently require ray capabilities somewhere in the path | | Upscaler | Linear,
    NVIDIA DLSS | Linear | DLSS may fall back to linear at provider initialization | | Upscaler quality | NativeAA, Quality, Balanced,
    Performance, UltraPerformance | NativeAA | Meaning is provider - defined;
linear ignores vendor quality semantics | | Ray reconstruction | Off,
    NVIDIA DLSS RR | Off | ReSTIR - only graph integration | | Exposure | Automatic or Manual;
parallel reduction or mip - chain metering;
manual / compensation / target / min / max / speeds | Automatic,
    parallel reduction | Persisted and live - applied except restart - only device choices | | Tone / output | Three tone mappers;
Automatic / Linear / sRGB encoding | ACES approximate,
    Automatic | SDR - style output only in current contract | | Diagnostics | Marker verbosity, GPU timing,
    parallel graph recording | Frame / pass markers, timing off,
    parallel on | Diagnostic toggles are not correctness evidence | | Settings persistence | 26 - name owned INI section;
per - viewport view mode is separately session - owned;
adapter / format report pending restart | Current runtime / CVar defaults
        | Aggregate requested state does not expose every console / RHI control
    or resolved active / fallback state; in-place silent writes are a known gap |

## Negative Capability Ledger

Targeted source review did not find a complete current product path for runtime-sized bindless resources, raster shadow maps, a non-ray lighting fallback, transparent blending/transmission, volumetric lighting/fog/atmosphere, deferred decals, color grading, chromatic aberration, frame generation, 3D textures, mesh/task shaders, tessellation/geometry pipelines, multiple primitive topologies, HDR display transfer/metadata, variable-rate shading, sampler feedback, or a persistent cross-invocation shader compile cache. Low-level enums, target designs, packaged SDK files, latency markers, or compiler vocabulary must not be promoted into product claims without a producer, consumer, selector, and executable evidence.

## Evidence Linkage

The executable closure items for these rows are `RHI-E01` through `RHI-E16`, `REN-E01` through `REN-E33`, and `SHD-E01` through `SHD-E12` in the [Capability Evidence Plan](../Modules/CapabilityEvidencePlan.md). Candidate-specific results belong in [Feature Completion Reports](../../Acceptance/FeatureCompletionReports.md), not in this snapshot.
