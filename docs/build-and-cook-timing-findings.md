# Build and Cook Timing Findings

This document tracks measured bottlenecks for SparkleEngine cook and build workflows. The goal is to collect enough precise timing data to decide what to fix next, with cook-all and asset recook performance first, then full Showcase/engine compilation.

## Measurement Rules

- Prefer engine/tool C++ timing through `SPARKLE_LOG_SCOPE`, `SPARKLE_CPU_SCOPE`, and the existing logging sinks.
- Keep batch and PowerShell layers thin; use them only to launch tools, clean generated outputs for scratch runs, or capture logs.
- Separate tool preparation/build time from actual asset cook time.
- Record whether a run is scratch, warm, recook, incremental build, or rebuild.
- Treat transient file-system failures separately from steady-state performance bottlenecks.

## Current Instrumentation

- AssetCooker CLI total time.
- AssetCooker dispatch time.
- AssetCooker shader, texture, and scene-asset stage times.
- AssetCooker child process times for ShaderCompiler and TextureCooker.
- TextureCooker application and request-file total time.
- TextureCooker request counts and cooked/skipped counts.
- TextureCooker per-request identity-hash, cache-check, cook, and metadata-publish phases.
- TextureCooker cooked-texture load, pipeline/process, serialize/write, and finalize phases.
- TextureCooker top request offenders by elapsed time.
- AssetCooker per-scene cook timings.
- MSBuild project, target, and task performance summaries for Showcase/engine builds.

## Cook-All Findings

### Scratch `Scripts\CookAllAssets.bat ALL Debug`

Run artifacts:

- Console stdout: `logs/measurements/cookall-scratch-stdout.txt`
- Console stderr: `logs/measurements/cookall-scratch-stderr.txt`
- C++ timing log: `logs/measurements/cookall-scratch.log`
- Exit code: `0`

Scratch inputs cleared before run:

- `build/Cooked`
- `build/Cache/Shaders`
- `build/Cook/Temp`

Top-level `.bat` workflow:

- Toolchain validation completed successfully.
- CMake configure/generate completed in about `12.2s` (`Configuring done (1.9s)`, `Generating done (10.3s)`).
- Cook-tool preparation then serially built `AssetCooker`, `AssetCookerDll`, `TextureCooker`, and `ShaderCompiler`.

Cook-tool preparation build offenders from MSBuild performance summaries:

| Serial target | Root project time | Main offenders shown by project/target summaries |
| --- | ---: | --- |
| `AssetCooker` | `724.905s` | `AssetCookerCore` `709.509s`, `MaterialCooker` `654.383s`, `SourceImportAdapters` `635.337s`, Assimp `586.592s`, `CL` `707.809s`, `ResolveProjectReferences` `2639.996s` cumulative |
| `AssetCookerDll` | `617.262s` | `AssetCookerCore` `607.341s`, `MaterialCooker` `554.873s`, `SourceImportAdapters` `534.096s`, Assimp `488.937s`, `CL` `602.821s`, `ResolveProjectReferences` `2228.471s` cumulative |
| `TextureCooker` | `156.651s` | `TextureCooker` `156.651s`, `CMP_Core` `21.656s`, `TextureCookShared` `5.853s`, `CL` `146.935s`, `ResolveProjectReferences` `47.443s` cumulative |
| `ShaderCompiler` | `98.770s` | `ShaderCompiler` `98.770s`, `CL` `89.527s`, `ResolveProjectReferences` `9.807s` cumulative |

Important interpretation: the `ResolveProjectReferences` numbers are cumulative MSBuild task totals and exceed wall time because project-reference traversal is counted repeatedly. The root project rows are the best quick comparison for serial target cost.

Actual AssetCooker C++ timing from the same scratch run:

| Stage | Time |
| --- | ---: |
| `AssetCooker.Cli.Run` | `391.970s` |
| `AssetCooker.DispatchPlan` | `391.869s` |
| `AssetCooker.Stage.Shaders` | `2.129s` |
| `AssetCooker.CollectTextureRequests` | `0.463s` |
| `AssetCooker.Stage.Textures` | `384.823s` |
| `TextureCooker.CookRequestFile` | `384.273s` |
| `AssetCooker.Stage.SceneAssets` | `4.694s` |

Scratch shader cook:

- `10` shader packages.
- `17` stage jobs.
- `17` backend invocations.
- `0` cache hits, `17` cache misses.
- Time inside AssetCooker shader stage: `2.129s`.

Scratch texture cook:

- `163` texture requests.
- `163` cooked, `0` skipped.
- Texture work dominates actual cook time: `384.823s` of `391.970s` (`~98.2%`).
- Per-texture phase samples show `ProcessTexture` dominates slow requests; source load, write, finalize, and metadata publish are usually milliseconds to a few hundred milliseconds.

Top scratch texture request offenders:

| Rank | Time | Asset ID | Source |
| ---: | ---: | --- | --- |
| 1 | `12.958s` | `175F8A825882C72F` | `Projects/Showcase/Assets/Textures/ABeautifulGame/Chessboard_base_color.jpg` |
| 2 | `12.723s` | `E65807EC3C600EE0` | `Projects/Showcase/Assets/Textures/ABeautifulGame/Knight_black_base_color.jpg` |
| 3 | `11.366s` | `091AEDCA1DBB99FE` | `Projects/Showcase/Assets/Textures/ABeautifulGame/Bishop_white_base_color.jpg` |
| 4 | `11.338s` | `73D87447DC8C39F4` | `Projects/Showcase/Assets/Textures/DamagedHelmet/Default_albedo.jpg` |
| 5 | `11.112s` | `DBFB35169A0799E1` | `Engine/Assets/Textures/Sky/evening_road_01_puresky_4k.exr` |
| 6 | `10.939s` | `05BA82986CA4F2BC` | `Projects/Showcase/Assets/Textures/ABeautifulGame/Bishop_black_base_color.jpg` |
| 7 | `10.928s` | `AE9659F70664F0A2` | `Projects/Showcase/Assets/Textures/ABeautifulGame/King_black_base_color.jpg` |
| 8 | `10.508s` | `DA610BCB80ED74F4` | `Projects/Showcase/Assets/Textures/ABeautifulGame/King_white_base_color.jpg` |
| 9 | `10.153s` | `1A050CDEED6AC547` | `Projects/Showcase/Assets/Textures/ABeautifulGame/Queen_white_base_color.jpg` |
| 10 | `9.635s` | `2F78F38C54C09C35` | `Projects/Showcase/Assets/Textures/ABeautifulGame/Queen_black_base_color.jpg` |

Scratch scene cook offenders:

- Whole scene stage: `4.694s`.
- Scene cooking is not a top offender compared with texture processing or cook-tool preparation.

Initial conclusions:

- The full `.bat` workflow is dominated by cook-tool preparation before the actual cook starts.
- The preparation path repeats expensive dependency/project traversal across serial targets. `AssetCooker` and `AssetCookerDll` both pull the same heavy dependency chain, including Assimp and source import adapters.
- The actual C++ cook is dominated by texture processing/compression, not shader cooking, scene cooking, request collection, metadata publishing, or output writes.
- Slow texture requests are mostly ABeautifulGame base-color textures plus one DamagedHelmet albedo and the EXR sky texture.

## Asset Recook Findings

### Warm `Scripts\CookAllAssets.bat ALL Debug`

Run artifacts:

- Console stdout: `logs/measurements/cookall-warm-stdout.txt`
- Console stderr: `logs/measurements/cookall-warm-stderr.txt`
- C++ timing log: `logs/measurements/cookall-warm.log`
- Exit code: `0`

Warm inputs retained from the scratch run:

- `build/Cooked`
- `build/Cache/Shaders`
- `build/Cook/Temp`

Cook-tool preparation remained expensive even though cooked asset outputs and shader cache were warm:

| Serial target | Root project time | Main offenders shown by project/target summaries |
| --- | ---: | --- |
| `AssetCooker` | `724.840s` | `AssetCookerCore` `709.098s`, `MaterialCooker` `655.779s`, `SourceImportAdapters` `635.192s`, Assimp `587.656s`, `CL` `707.264s`, `ResolveProjectReferences` `2637.566s` cumulative |
| `AssetCookerDll` | `617.600s` | `AssetCookerCore` `611.548s`, `MaterialCooker` `555.644s`, `SourceImportAdapters` `535.098s`, Assimp `487.250s`, `CL` `604.439s`, `ResolveProjectReferences` `2236.344s` cumulative |
| `TextureCooker` | `158.481s` | `CMP_Core` `21.852s`, `CookCommon` `6.434s`, `TextureCookShared` `4.059s`, `CL` `146.714s`, `ResolveProjectReferences` `50.116s` cumulative |
| `ShaderCompiler` | `92.894s` | `ShaderCompiler` `92.894s`, `CL` `86.013s`, `ResolveProjectReferences` `7.396s` cumulative |

Warm C++ recook timing after tool preparation completed:

| Stage | Time |
| --- | ---: |
| `AssetCooker.Cli.Run` | `6.668s` |
| `AssetCooker.DispatchPlan` | `6.560s` |
| `AssetCooker.Stage.Shaders` | `1.194s` |
| `AssetCooker.CollectTextureRequests` | `1.021s` |
| `AssetCooker.Stage.Textures` | `4.013s` |
| `TextureCooker.CookRequestFile` | `2.714s` |
| `AssetCooker.Stage.SceneAssets` | `1.191s` |

Warm shader recook:

- `10` shader packages.
- `17` stage jobs.
- `0` backend invocations.
- `17` cache hits, `0` cache misses.
- Time inside AssetCooker shader stage: `1.194s`.

Warm texture recook:

- `163` texture requests.
- `0` cooked, `163` skipped.
- Texture request-file pass: `2.714s`.
- Slowest skipped request was the EXR sky texture at `757ms`; the next slowest were `101ms` and `83ms`. Most skipped textures were single-digit to low double-digit milliseconds.

Top warm skipped-texture offenders:

| Rank | Time | Asset ID | Source |
| ---: | ---: | --- | --- |
| 1 | `757ms` | `DBFB35169A0799E1` | `Engine/Assets/Textures/Sky/evening_road_01_puresky_4k.exr` |
| 2 | `101ms` | `B3D7A049E9982C38` | `Projects/Showcase/Assets/Textures/Sponza/14118779221266351425.jpg` |
| 3 | `83ms` | `FF7B7549FE5D169D` | `Projects/Showcase/Assets/Textures/Sponza/11474523244911310074.jpg` |
| 4 | `27ms` | `FEB04CFBDE61B424` | `Projects/Showcase/Assets/Textures/Sponza/5061699253647017043.png` |
| 5 | `17ms` | `8AC1E435C7A9FCB4` | `Projects/Showcase/Assets/Textures/ABeautifulGame/Castle_ORM.jpg` |

Warm recook interpretation:

- Asset recook itself is not the big problem once the tools are built and the caches are current.
- The current public `.bat` path hides that by always running the heavy tool-preparation path first.
- `PrepareAssetCooker` always validates, regenerates, then serially builds `AssetCooker`, `AssetCookerDll`, `TextureCooker`, and `ShaderCompiler`. The repeated project-reference graph causes the same heavyweight dependencies to be traversed and compiled repeatedly.
- The EXR sky texture still costs noticeably more than other skipped textures, likely because identity hashing reads a large source file before the cache-current check can skip cooking.

### Aggregate Cook-Tool Target Validation

Follow-up change:

- Added CMake target `SparkleCookTools` depending on `AssetCooker`, `AssetCookerDll`, `TextureCooker`, and `ShaderCompiler`.
- Updated `PrepareCookTools` and `PrepareAssetCooker` to build `SparkleCookTools` instead of four independent targets.

Validation artifacts:

- Console stdout: `logs/measurements/cookall-warm-aggregate-stdout.txt`
- Console stderr: `logs/measurements/cookall-warm-aggregate-stderr.txt`
- C++ timing log: `logs/measurements/cookall-warm-aggregate.log`
- Exit code: `0`

Measured result:

| Workflow | Cook-tool build roots | Prep build time comparison |
| --- | --- | ---: |
| Before aggregate target | `AssetCooker` `724.840s` + `AssetCookerDll` `617.600s` + `TextureCooker` `158.481s` + `ShaderCompiler` `92.894s` | `~1593.815s` serial root sum |
| After aggregate target | `SparkleCookTools` `847.521s` | `~746.294s` lower root sum |

Aggregate target top offenders remain the same heavy dependency chain:

| Project | Time |
| --- | ---: |
| `SparkleCookTools.vcxproj` | `847.521s` |
| `AssetCooker.vcxproj` | `610.651s` |
| `AssetCookerCore.vcxproj` | `596.330s` |
| `MaterialCooker.vcxproj` | `542.244s` |
| `SourceImportAdapters.vcxproj` | `519.500s` |
| `assimp.vcxproj` | `475.494s` |
| `TextureCooker.vcxproj` | `145.139s` |
| `ShaderCompiler.vcxproj` | `86.429s` |

Aggregate target task/target offenders:

- `ClCompile`: `827.198s` across `24` target calls.
- `CL`: `827.135s` across `36` task calls.
- `ResolveProjectReferences`: `3059.212s` cumulative across `31` target calls.

Aggregate warm C++ recook after tool preparation:

| Stage | Time |
| --- | ---: |
| `AssetCooker.Cli.Run` | `5.933s` |
| `AssetCooker.Stage.Shaders` | `1.269s` |
| `AssetCooker.Stage.Textures` | `3.352s` |
| `TextureCooker.CookRequestFile` | `2.098s` |
| `AssetCooker.Stage.SceneAssets` | `1.035s` |

Updated interpretation:

- The aggregate target removes duplicate public cook-tool build invocations and is a real improvement to `CookAllAssets.bat`.
- It does not solve the largest remaining issue: `GenerateSolution.bat` still precedes every cook and invalidates enough generated build state to recompile the import stack once.
- The next fix should avoid unconditional CMake regeneration for normal cook-all runs, or make regeneration not mark the heavy tool/import graph out of date.

## Showcase And Engine Build Findings

### `ALL_BUILD.vcxproj` Debug Build

Run artifacts:

- Console stdout: `logs/measurements/allbuild-debug-stdout.txt`
- Console stderr: `logs/measurements/allbuild-debug-stderr.txt`
- Exit code: `0`

Measured with direct MSBuild because CMake Tools returned an empty target list in this workspace.

Top project offenders:

| Rank | Project | Time |
| ---: | --- | ---: |
| 1 | `ALL_BUILD.vcxproj` | `981.290s` |
| 2 | `Tools/AssetConverter/AssetConverter.vcxproj` | `578.934s` |
| 3 | `Tools/MaterialCooker/MaterialCooker.vcxproj` | `536.193s` |
| 4 | `Tools/SourceImportAdapters/SourceImportAdapters.vcxproj` | `517.238s` |
| 5 | `_deps/assimp-build/code/assimp.vcxproj` | `471.064s` |
| 6 | `Tools/TextureCooker/TextureCooker.vcxproj` | `117.393s` |
| 7 | `Tools/ShaderCompiler/ShaderCompiler.vcxproj` | `83.988s` |
| 8 | `_deps/ktx-build/ktx.vcxproj` | `62.077s` |
| 9 | `Tools/AssetCooker/AssetCooker.vcxproj` | `37.103s` |
| 10 | `_deps/ktx-build/lib/astc-encoder/Source/astcenc-avx2-static.vcxproj` | `26.356s` |

Engine and Showcase rows in the same full build were small compared with tool/import dependencies:

| Project | Time |
| --- | ---: |
| `SparkleApplication.vcxproj` | `1.422s` |
| `SparkleEditor.vcxproj` | `0.971s` |
| `SparkleRenderer.vcxproj` | `0.500s` |
| `ShowcaseEditor.vcxproj` | `3.973s` |
| `ShowcaseRuntime.vcxproj` | `0.787s` |

Task/target offenders:

- `ClCompile`: `951.632s` across `37` target calls.
- `CL`: `938.940s` across `60` task calls.
- `ResolveProjectReferences`: `2585.806s` cumulative across `45` target calls.
- `CustomBuild`: `11.961s` across `45` target calls.
- `Link`: `8.411s` across `10` target calls.

Interpretation:

- Full solution build time is overwhelmingly compile-bound in tool/import dependencies.
- The engine and Showcase launch targets themselves are not the top compile offenders in this measurement.
- Tool projects pull large third-party dependency trees into the build, especially Assimp and KTX/TextureCooker dependencies.
- Project-reference traversal is very noisy and repeated, but the root project rows identify the wall-time offenders more clearly.

Third-party warning noise observed during the full build:

- `ktx.vcxproj`: clang-cl ignored `-fvisibility=hidden`.
- `ktx.vcxproj`: `astc_encode.cpp` has a `-Wcast-function-type-mismatch` warning for a Windows thread proc cast.

These are separate from the Assimp bundled-zlib warning flood that was fixed by using external zlib. They are not the top build-time offender, but they are remaining warning noise.

### Showcase Debug C++ Rebuilds

These measurements are C++ build-only `MSBuild /t:Rebuild` runs. They do not run asset cooking.

`ShowcaseRuntime.vcxproj` rebuild artifacts:

- Console stdout: `logs/measurements/showcase-runtime-rebuild-stdout.txt`
- Console stderr: `logs/measurements/showcase-runtime-rebuild-stderr.txt`
- Exit code: `0`

`ShowcaseEditor.vcxproj` rebuild artifacts:

- Console stdout: `logs/measurements/showcase-editor-rebuild-stdout.txt`
- Console stderr: `logs/measurements/showcase-editor-rebuild-stderr.txt`
- Exit code: `0`

Scratch/rebuild project results:

| Target | Root project time | Compile | Lib | Link | Project-reference traversal |
| --- | ---: | ---: | ---: | ---: | ---: |
| `ShowcaseRuntime.vcxproj` | `369.137s` | `341.889s` | `3.718s` | `1.416s` | `1025.037s` cumulative |
| `ShowcaseEditor.vcxproj` | `362.432s` | `348.623s` | `2.849s` | `1.209s` | `1023.121s` cumulative |

Top project rows for `ShowcaseRuntime` rebuild:

| Rank | Project | Time |
| ---: | --- | ---: |
| 1 | `ShowcaseRuntime.vcxproj` | `369.137s` |
| 2 | `SparkleApplication.vcxproj` | `351.626s` |
| 3 | `SparkleGameFramework.vcxproj` | `126.711s` |
| 4 | `SparkleRenderer.vcxproj` | `115.808s` |
| 5 | `SparkleRHI.vcxproj` | `64.538s` |
| 6 | `SparkleCore.vcxproj` | `44.355s` |
| 7 | `SparklePlatform.vcxproj` | `22.863s` |
| 8 | `imgui.vcxproj` | `12.079s` |

Top project rows for `ShowcaseEditor` rebuild:

| Rank | Project | Time |
| ---: | --- | ---: |
| 1 | `ShowcaseEditor.vcxproj` | `362.432s` |
| 2 | `SparkleApplication.vcxproj` | `356.713s` |
| 3 | `SparkleEditor.vcxproj` | `281.110s` |
| 4 | `SparkleGameFramework.vcxproj` | `128.516s` |
| 5 | `SparkleRenderer.vcxproj` | `119.175s` |
| 6 | `SparkleRHI.vcxproj` | `65.068s` |
| 7 | `SparkleCore.vcxproj` | `39.897s` |
| 8 | `SparklePlatform.vcxproj` | `20.399s` |
| 9 | `imgui.vcxproj` | `9.457s` |

Interpretation:

- A scratch Showcase C++ rebuild is compile-bound at about six minutes on this machine/configuration.
- The slowest real code-build row is `SparkleApplication`, because it sits at the top of the shared runtime/editor dependency stack.
- `ShowcaseRuntime` and `ShowcaseEditor` themselves are tiny; their root times mostly include referenced engine projects.
- `SparkleEditor` adds a large editor-specific rebuild cost for the editor target, but the total root time is similar because shared dependencies dominate both runs.
- Link and lib time are small compared with compilation.

### Direct Showcase Debug Builds

`ShowcaseEditor.vcxproj` run artifacts:

- Console stdout: `logs/measurements/showcase-editor-build-stdout.txt`
- Console stderr: `logs/measurements/showcase-editor-build-stderr.txt`
- Exit code: `0`

`ShowcaseRuntime.vcxproj` run artifacts:

- Console stdout: `logs/measurements/showcase-runtime-build-stdout.txt`
- Console stderr: `logs/measurements/showcase-runtime-build-stderr.txt`
- Exit code: `0`

Direct project build results:

| Target | Root project time | Compile | Link | Main cost |
| --- | ---: | ---: | ---: | --- |
| `ShowcaseEditor.vcxproj` | `12.318s` | `0.620s` | `1.247s` | `ResolveProjectReferences` `21.371s` cumulative, `CustomBuild` `7.793s` |
| `ShowcaseRuntime.vcxproj` | `11.808s` | `0.708s` | not a top target entry | Dependency traversal/custom rules; `SparkleApplication` `6.525s` |

Interpretation:

- Normal direct Showcase builds are not compile-bound after the shared outputs are already current.
- Direct Showcase builds still run boundary validators and generated custom rules, so even a tiny launch target build costs about `12s`.
- The project source files themselves are tiny. The recurring overhead is dependency traversal and custom validation, not editor/runtime source compilation.

## Known Reliability Findings

- One measured cook attempt failed while publishing `6B885AB229076FE2.stex.cookmeta.tmp` after about 87 seconds, then a later run succeeded and produced the metadata. Track this as likely transient file-write contention or fragile atomic write behavior, not a deterministic texture content failure.
- Existing VS Code/MSBuild tasks that pass `/clp:ErrorsOnly;Summary` through PowerShell split at the semicolon. Use `/clp:ErrorsOnly` or quote the full argument when measuring through PowerShell-wrapped tasks.

## Production Iteration Improvement Plan

Sparkle now has enough measurement coverage to separate three different problems:

- Warm asset recook is fast once tools are already built and caches are current.
- Scratch asset cook is dominated by texture processing and compression.
- Clean C++ rebuilds are dominated by compilation, not linking.

The production goal is to make the default developer path aggressively incremental while moving expensive correctness sweeps into explicit validation or CI paths. Large productions usually do not make every local build prove every architectural rule, regenerate every build file, rebuild every tool, and recook every unchanged asset. They split those concerns into tiers.

### Current Bottleneck Map

| Area | Current measured symptom | Main cause | Production reading |
| --- | --- | --- | --- |
| Warm `CookAllAssets.bat ALL Debug` | Actual C++ recook is about `5.9s`, but cook-tool prep is about `847.5s` after aggregate-target improvement | Public cook path still refreshes build files and rebuilds cook tools before cooking | Local recook is structurally fast but hidden behind build-prep work |
| Scratch cook | `AssetCooker.Cli.Run` `391.970s`, `TextureCooker.CookRequestFile` `384.273s` | Texture processing/compression dominates | Needs asset-pipeline scaling, not script tuning |
| Full solution build | `ALL_BUILD.vcxproj` `981.290s` | Tool/import dependency compilation, especially `AssetConverter`, `MaterialCooker`, `SourceImportAdapters`, `assimp` | Tool dependencies should stay out of normal engine/project iteration |
| Showcase clean rebuilds | `ShowcaseRuntime` `369.137s`, `ShowcaseEditor` `362.432s` | `ClCompile` is about `342-349s`; link/lib are tiny | Compile acceleration matters more than linker work |
| Direct current-output Showcase builds | About `12s` with almost no real project compilation | Dependency traversal, generated custom rules, validation overhead | Local no-op builds still have avoidable fixed costs |

### 1. Make Validation And Formatting Explicit

Status: implemented in the root CMake build graph.

What:

- Added `SPARKLE_BUILD_VALIDATION_ON_BUILD`, default `OFF`.
- Added `SPARKLE_RUN_CLANG_FORMAT_ON_BUILD`, default `OFF`.
- Added explicit aggregate target `sparkle_validation_check` for all boundary-validation targets.
- Kept `clang_format_check` available as an explicit dry-run formatting target when `clang-format` is present.

Why:

- The root CMake previously attached boundary checks and `clang_format_check` to normal targets.
- That made tiny local builds pay for policy checks even when the developer only wanted to compile and run.
- Large production codebases usually separate fast local iteration from presubmit or CI correctness sweeps.

Who owns it:

- Build system owner, with engine owners defining which validation gates remain mandatory in CI.

How it works:

- Local path: `BuildProject.bat Showcase Editor Debug` builds code only.
- Explicit validation path: build `sparkle_validation_check`.
- Explicit format check path: build `clang_format_check`.
- CI path: configure with `SPARKLE_BUILD_VALIDATION_ON_BUILD=ON`, `SPARKLE_RUN_CLANG_FORMAT_ON_BUILD=ON`, or run the explicit check targets as separate CI steps.

Expected impact:

- Reduces fixed overhead in current-output builds.
- Keeps architectural boundaries enforced without making every local compile feel like a presubmit.
- Needs follow-up measurement on direct current-output Showcase builds to quantify the no-op build reduction.

### 2. Stop Unconditional CMake Regeneration In Normal Build And Cook Scripts

Status: implemented for normal project builds and cook-tool preparation.

What:

- Replaced unconditional `GenerateSolution.bat` calls in `BuildProject.bat` and `AssetCooking.bat` with `Scripts/Internal/EnsureBuildFiles.bat`.
- Keep `GenerateSolution.bat` as the explicit public owner of full build-file refresh.
- Added `SPARKLE_FORCE_CONFIGURE=1` as the escape hatch for forcing a refresh.
- Added `Scripts/Internal/Test-BuildFilesCurrent.ps1` to decide whether generated build files are current.
- Added `build/BuildFilesFreshness.json` as the generated freshness stamp.

Why:

- The current public cook path calls `GenerateSolution.bat` before building tools.
- The measurements indicate this invalidates enough generated state to rebuild the heavy tool/import stack.
- This is the largest remaining warm cook problem.

Who owns it:

- Build scripts owner.

How it works:

- The stale detector checks CMake cache/solution existence, generator, platform, toolset, CMake inputs, project markers, and a hash of source file paths.
- The source-list hash catches added/removed source files without treating ordinary `.cpp` edits as a reason to regenerate.
- If nothing relevant changed, normal build/cook scripts skip configure and build the requested target directly.
- If CMake files, project markers, toolchain settings, or source-list inputs changed, run `GenerateSolution.bat`.
- If a developer needs to force the old behavior, run with `SPARKLE_FORCE_CONFIGURE=1`.

Expected impact:

- Turns warm cook-all from a tool-prep-dominated workflow into a mostly real recook workflow.
- Preserves correctness when project structure or CMake inputs change.
- Validated `EnsureBuildFiles.bat` first refreshed and wrote the stamp, then skipped `GenerateSolution.bat` on the second run.
- Validated `BuildProject.bat Showcase Editor Debug` now reports `Build files are current. Skipping GenerateSolution.` before building.

### 3. Split Developer Cook From Verified Full Cook

What:

- Keep `CookAllAssets.bat` as a safe verified entrypoint.
- Add fast local modes such as `CookChangedAssets`, `CookProjectFast`, or `CookAllAssets.bat --no-configure --no-build`.

Why:

- Warm cook after prep is already fast: `AssetCooker.Cli.Run` about `5.9s` in the aggregate validation run.
- Developers need that fast path by default when tools and build files are current.
- CI and release workflows still need the full verified path.

Who owns it:

- Tools owner and build scripts owner.

How it works:

- Fast cook verifies required executables exist and are fresh enough, then launches `AssetCooker`.
- Verified cook performs toolchain check, configure, tool build, validation, and cook.
- CI uses verified cook; local iteration defaults to fast cook.

Expected impact:

- Makes the common content iteration loop seconds instead of many minutes.
- Keeps the safer full path available for clean machines and automation.

### 4. Treat Cook Tools As Freshness-Checked Developer Infrastructure

What:

- Build cook tools only when tool inputs changed.
- Keep the existing `SparkleCookTools` aggregate target, but gate whether it needs to run.

Why:

- The aggregate target reduced repeated public cook-tool build roots from about `1593.815s` to `847.521s`.
- That is a real improvement, but still far too expensive for a warm recook.
- Production pipelines treat tools as versioned infrastructure, not something rebuilt before every content action.

Who owns it:

- Tools/build owner.

How it works:

- Store a tool freshness manifest with executable paths, relevant source timestamps or hashes, CMake input stamps, build configuration, and toolchain identity.
- If current, skip `SparkleCookTools` build.
- If stale, build `SparkleCookTools` once, then update the manifest.

Expected impact:

- Removes most of the warm cook wall while keeping tool correctness.
- Gives a clear reason when a tool rebuild is required.

### 5. Keep Tool-Only Dependencies Out Of Normal Runtime Iteration

What:

- Ensure `assimp`, KTX, Compressonator, SourceImportAdapters, and tool-only cook dependencies are not pulled into normal runtime/editor builds unless a tool target requires them.
- Keep standalone tools out of default `ALL_BUILD` for developer configurations where possible.

Why:

- Full solution build is dominated by tool/import dependencies, not the Showcase launch targets.
- `ALL_BUILD.vcxproj` spends most of its time in `AssetConverter`, `MaterialCooker`, `SourceImportAdapters`, `assimp`, `TextureCooker`, and `ShaderCompiler`.

Who owns it:

- Build owner and tool module owners.

How it works:

- Tool-only third-party projects remain `EXCLUDE_FROM_ALL`.
- Runtime/editor targets do not depend on import/cook libraries.
- Tool builds are explicit via `SparkleCookTools` or specific tool targets.

Expected impact:

- Keeps project and engine iteration from paying for source import infrastructure.
- Makes full tool builds intentional.

### 6. Make Build Parallelism A Profile, Not A Permanent Global Compromise

What:

- Keep the current safe MSBuild settings as one profile.
- Add a measured fast profile that restores parallelism once PDB and memory contention are controlled.

Why:

- `CMakeHelpers.bat` currently uses `/m:1`, disables multi-tool tasks, disables file tracking, and disables node reuse.
- That is useful for reliability after observed PDB/OOM issues, but it guarantees slow compile-bound rebuilds.
- Showcase rebuilds are compile-bound at about six minutes.

Who owns it:

- Build/toolchain owner.

How it works:

- Add something like `SPARKLE_BUILD_PROFILE=Safe|Fast|CI`.
- `Safe`: current conservative flags.
- `Fast`: measured parallel MSBuild settings, with controlled PDB behavior and memory limits.
- `CI`: deterministic settings suitable for automation.

Expected impact:

- Allows fast local rebuilds on machines that can handle parallel compilation.
- Keeps the known-safe fallback for unstable environments.

### 7. Add Compiler Cache And Selective Unity Build Support

What:

- Add optional support for a clang-cl compatible compiler cache such as `sccache` or `clcache`.
- Evaluate CMake unity builds per target, default off until measured.

Why:

- Clean Showcase rebuilds are dominated by `ClCompile`; link/lib time is very small.
- Compiler caching and unity builds are common production levers for compile-bound projects.

Who owns it:

- Build owner, with module owners validating unity-build compatibility.

How it works:

- Compiler cache keys object output by compiler flags, source content, includes, and toolchain identity.
- Rebuilding unchanged translation units becomes a cache hit instead of a compile.
- Unity builds batch multiple `.cpp` files into generated jumbo units for selected stable modules.

Expected impact:

- Compiler cache improves rebuilds across clean build folders and repeated branch switches.
- Unity builds can reduce scratch build time, but must be applied selectively because they can hurt incremental granularity and expose include-order bugs.

### 8. Audit PCH Discipline

Status: implemented first pass.

What:

- Reviewed PCH contents for `SparkleApplication`, `SparkleEditor`, `SparkleRenderer`, `SparkleRHI`, and tool targets.
- Removed `Config/RenderConfig.h` from `SparkleRHI`'s PCH so render configuration changes do not force every RHI translation unit through the PCH dependency.
- Enabled the existing stable `TextureCooker` PCH in CMake; the target already included `Private/PCH.h` from its `.cpp` files, but it was not configured as a real precompiled header.

Why:

- PCH is already used, which is good.
- If unstable project or engine headers are included in PCH, small edits can invalidate too much of the build.
- `RenderConfig.h` is a configuration header and is more likely to change during renderer/RHI iteration than STL, Windows, WRL, DirectXMath, or D3D12 platform headers.
- `TextureCooker` is one of the measured cook-tool build offenders, and its existing `PCH.h` contains stable STL/file-system headers suitable for precompilation.

Who owns it:

- Module owners, guided by build measurements.

How it works:

- PCH should contain stable STL, platform, and third-party headers.
- Frequently edited engine headers should stay out unless they provide a clear measured win.
- Track PCH rebuild cost in build summaries.
- RHI files that need `RenderConfig` already include it directly or through their owning public headers, keeping dependencies local instead of relying on the PCH.
- TextureCooker now gets a real CMake PCH for its stable private header set.

Expected impact:

- Improves incremental rebuild reliability.
- Reduces surprising full-module rebuilds after small header edits.
- Reduces unnecessary RHI PCH invalidation when render configuration changes.
- Should improve TextureCooker compile throughput on clean and partial rebuilds without making the PCH volatile.

### 9. Promote Texture Cache Into A Derived Data Cache Model

What:

- Make texture cook outputs addressable by a deterministic key: source content hash, cook settings hash, tool version hash, and output format hash.
- Optionally support a shared local/network cache later.

Why:

- Scratch texture cook is real work: `TextureCooker.CookRequestFile` `384.273s`.
- Warm cache behavior is already strong: `163` skipped texture requests and only a few seconds spent in texture stage.
- Production asset pipelines avoid recomputing expensive derived data when the exact same inputs were cooked before.

Who owns it:

- Asset pipeline owner.

How it works:

- Each texture request computes a stable cache key.
- If a matching cooked artifact exists, copy/link it into the cooked output location.
- If missing, cook once and publish to the cache.
- CI can prewarm shared cache entries for common project assets.

Expected impact:

- Reduces scratch cook cost on clean workspaces and across machines.
- Makes expensive texture compression reusable instead of per-workspace work.

### 10. Parallelize Texture Cooking With A Bounded Worker Pool

What:

- Process independent texture requests in parallel inside `TextureCooker`.
- Bound workers by CPU and memory budget.

Why:

- Scratch cook has `163` independent texture requests.
- Top texture requests cost about `9-13s` each.
- This is exactly the work shape production cookers parallelize.

Who owns it:

- Texture pipeline owner.

How it works:

- `TextureCooker` reads the request file, resolves request identities, then dispatches cache misses to a worker pool.
- Cache hits can remain lightweight and mostly serial or batched.
- Metadata publish must remain atomic and contention-safe.
- Start conservatively, for example `min(logical cores / 2, memory budget)`.

Expected impact:

- Reduces scratch texture cook wall time when many requests miss cache.
- Keeps warm recook fast because skipped requests remain cheap.

### 11. Add Cook Quality Profiles

Status: implemented as the Sparkle build-profile matrix.

What:

- Replaced the previous standalone build configurations with exactly six profiles: `DebugEditor`, `DebugGame`, `DevelopmentEditor`, `DevelopmentGame`, `ShippingEditor`, and `ShippingGame`.
- Added `CMake/SparkleBuildProfiles.cmake` as the inspectable source of truth for profile names, profile groups, compiler flags, and target-shape definitions.

Why:

- Local iteration does not always need final compression effort or final-quality derived data.
- Production engines commonly distinguish fast editor/dev cooks from shipping cooks.
- Editor and editorless game runtimes need to be selectable from the build profile itself, matching the two-keyword model used by Unreal-style configurations.

Who owns it:

- Rendering owner and asset pipeline owner.
- Build owner for the profile matrix and target selection semantics.

How it works:

- `Debug*` profiles use no optimization and full debug information.
- `Development*` profiles use optimized builds with debug information and developer diagnostics.
- `Shipping*` profiles use optimized shipping-style builds and the leanest diagnostics policy.
- `*Editor` profiles build editor-capable launch targets; `*Game` profiles build editorless runtime targets.
- Cook scripts and `AssetCooker` accept only the six named profiles, keeping cooked outputs aligned with the selected build profile.

Expected impact:

- Improves artist/programmer iteration on visual assets.
- Keeps final quality protected by explicit profile selection and CI validation.
- Removes ambiguous standalone `Debug`, development, or shipping choices that do not say whether the output is editor-capable or editorless runtime.

### 12. Produce Machine-Readable Timing Summaries

Status: implemented first pass for C++ cook tools; repository-wide coverage is still incomplete.

What:

- Keep the current C++ timing scopes, but also emit compact structured summaries for cooks.
- AssetCooker writes `build/Cook/Summaries/<Project>-<Profile>-assetcook-summary.json`.
- TextureCooker writes `build/Cook/Summaries/<Project>-<Profile>-texturecook-summary.json` when launched by AssetCooker.
- Treat these as cook-stage summaries, not full repository build/cook summaries yet.

Why:

- Production teams track iteration time as a product metric.
- The current findings document is useful, but manual analysis should not be the only way to catch regressions.
- JSON summaries let local tools and CI consume the same timing evidence that humans see in logs.
- Full repository coverage must include build-file freshness checks, CMake configure/generate time, target build time, cook-tool preparation, child cook tools, and final cooked output counts.

Who owns it:

- Tools/CI owner.
- AssetCooker and TextureCooker own the C++ summary emission.

How it works:

- AssetCooker records project, profile, plan path, total elapsed time, scene counts, output records, and per-stage elapsed times.
- TextureCooker records request file, total elapsed time, request count, cooked count, skipped/cache-hit count, cooked/cache-miss count, and the top ten slowest requests.
- The local console path prints AssetCooker top stages and TextureCooker top requests from C++ before exiting.
- CI can store the JSON files under `build/Cook/Summaries/` as artifacts and compare them against baselines.
- Build summaries remain a follow-up unless build timing is moved behind an engine/tool executable; MSBuild logs are currently outside the C++ timing infrastructure.

Coverage map:

| Area | Current coverage | Gap |
| --- | --- | --- |
| AssetCooker project cook | JSON summary with total time, per-stage times, scene counts, output records | Need aggregate summary across multiple projects when cooking `ALL` |
| TextureCooker request batch | JSON summary with request counts, cooked/skipped counts, cache hit/miss counts, and top requests | Need the same structured model for ShaderCompiler and scene/material/mesh stages |
| Cook-tool preparation | Human-readable MSBuild output only | Need machine-readable prepare/build step summary |
| Build-file freshness/configure | Human-readable script/CMake output only | Need machine-readable stale reason and configure/generate elapsed time |
| Project/engine builds | MSBuild can emit performance summaries, but they are not normalized into Sparkle JSON | Need repository build summary covering target, project, task, and top offender timings |
| CI comparison | Summary files can be stored as artifacts | Need baseline comparison tooling after summaries are complete |

Expected impact:

- Makes performance regressions visible quickly.
- Keeps optimization work grounded in data instead of anecdotes.

### Recommended Implementation Order

| Phase | Focus | Work | Expected result |
| --- | --- | --- | --- |
| 1 | Remove accidental local taxes | Detach validation/format from normal builds, stale-gate configure, add fast no-configure/no-build cook path, add cook-tool freshness manifest | Warm cook and no-op build iteration become much faster |
| 2 | Scale scratch cook | Derived data cache, bounded parallel texture cooking, cook quality profiles | Scratch asset cook stops being serial texture recompression work |
| 3 | Accelerate compilation | Fast build profile, compiler cache, PCH audit, selective unity builds, stronger third-party/tool dependency isolation | Clean rebuild and full solution times improve without weakening correctness |

The bold production pattern is simple: local commands should touch only what changed; full validation should be explicit or CI-owned; tools should be freshness-checked; cooked data should be cache-keyed; expensive content work should be parallel; and iteration time should be tracked continuously.