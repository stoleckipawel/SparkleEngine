# C. Candidate And Repository Gap Assessment

Status: evidence-backed current-state audit
Assessment date: 2026-07-26
Repository snapshot: `master` at `f74c26e9` plus an uncommitted renderer/residency worktree
Inputs: [canonical requirements](Requirements.md), [source archive](RoleSourceArchive.md), supplied CV, [public professional profile](https://www.linkedin.com/in/stoleckipawel/), [public website](https://stoleckipawel.dev/), [public GitHub profile](https://github.com/stoleckipawel), local source inspection, build attempt, and public profiles of engineers already operating at principal/staff level

## Executive Verdict

The candidate is already a strong production rendering engineer with unusual art-to-engine breadth, shipped cross-platform optimization, lighting and shader depth, public speaking, partner collaboration, and a substantial independent D3D12/Vulkan renderer.

The candidate is not yet a complete match for the full target persona.

The decisive gaps are not another raster feature, another editor panel, or more engine architecture. They are:

1. no demonstrated machine-learning fundamentals, Python workflow, trained model, model-to-shader translation, or neural-graphics runtime result;
2. no reviewer-ready D3D12/Vulkan workload investigation with captures, causal counters, reduced reproducer, and a concise conclusion;
3. weak public verification around the engine: no root README, no repository description/topics/site, no release, no active source-level test suite, no CI, and a placeholder license;
4. no native Linux build/run/capture evidence;
5. a CV that hides relevant detail from both ATS extraction and a skeptical software-engineering reviewer;
6. the showcase has only a Sponza-class verified loop: the existing external Bistro catalog hook is unavailable, and neither Bistro nor San Miguel has a runnable level, frozen reference, workload record, or public result;
7. a principal-tenure gap that cannot be solved by repository size and must be compensated by exceptional proof, influence, and transferability.

Directional grade today:

| Dimension | Grade | Meaning |
| --- | --- | --- |
| Shipped real-time rendering and performance | `A-` | Strongest evidence; already credible. |
| C++, shaders, RHI, and graphics architecture | `B+` | Substantial code and professional work, but public verification and tests lag the code volume. |
| Ray tracing, GI, and path tracing | `B` | Real code paths exist; no compact reproducible case study proves correctness/quality/performance across backends. |
| GPU debugging and workload analysis | `B-` | Professional claims and engine diagnostics exist; named capture workflows and causal public studies are missing. |
| Research-to-product and collaboration | `B+` | CV and recommendations are strong; independent public adoption evidence is missing. |
| Technical communication | `A-` | Multiple talks and a high-quality frame-graph series are strong signals. |
| ML, neural graphics, and inference optimization | `F` | Core target requirement is absent rather than weak. |
| Portfolio discoverability and trust | `D+` | The public surface makes a reviewer work too hard and exposes preventable trust defects. |
| Principal-level completeness | `C+` | A credible stretch profile today, not a low-risk full match. |

These are readiness grades, not hiring probabilities.

## Audit Boundary

The repository was inspected locally rather than inferred from folder names. The audit included:

- 2,073 tracked files and 943 commits;
- approximately 138k C/C++/shader source lines across `Engine`, `Tools`, and runnable project code;
- module, public-header, backend, shader, test, CI, platform, and ML keyword inventories;
- representative RHI, renderer, frame graph, shader compiler, ray-tracing, task, diagnostics, and content-pipeline paths;
- a live `DevelopmentEditor` renderer build after CMake regeneration;
- the public repository and professional surfaces as an unauthenticated reviewer sees them.

The working tree contained ongoing renderer/residency edits before this review. No source code was changed by the review. Build findings must therefore be read as a worktree snapshot, not as a claim about the last committed revision.

## Acceptance-Scene Readiness

The scene decision is binding in [I. Bistro and San Miguel Acceptance Workloads](../../Engineering/Validation/BistroAndSanMiguelWorkloads.md): Sponza is Tier 0, Bistro is the Tier 1 primary flagship, and San Miguel is the Tier 1 secondary supported scene.

| Capability | Current level | Repository fact | Next truthful gate |
| --- | ---: | --- | --- |
| Sponza fast loop | `E2.5` | Required startup level and committed glTF/textures exist; raster and RT-related paths are exercised in prior focused validation. | Freeze visual references and short automated regression thresholds. |
| Bistro catalog/product intent | `E1` | `Levels.catalog` declares an unavailable external optional pack rooted at `Assets/Meshes/Bistro`. | Record provenance, acquire immutable source, create deterministic import/cook inventory, and add grouped exterior/original-interior/wine-interior variants. |
| Bistro material correctness | `E0` | No Bistro content or support/fallback matrix exists. | Account for every source material/texture, render debug contact sheets, and compare frozen cameras to a high-sample reference. |
| Bistro performance evidence | `E0` | No load, route, capture, timing distribution, memory high-water, residency trace, or AS record exists. | Run the protocol in I and complete three causal bottleneck studies. |
| San Miguel support | `E0` | No catalog entry, content pack, level, deterministic route, or reference exists. | Add high/low variants as an external optional pack through the same inventory pipeline; publish correct raster/reference views, controlled geometry-scaling benchmark, and high-detail hero result. |
| Cross-scene neural evidence | `E0` | No trained model exists; therefore no generalization result exists. | Keep final San Miguel cameras held out, implement real shader inference, and report Bistro plus held-out San Miguel quality/cost/failures. |

The selection is strong because it reduces ambiguity rather than adding content breadth. Bistro forces production-shaped outdoor/interior scale and material/performance work; San Miguel forces a distinct, iconic indirect-lighting case; Sponza keeps iteration affordable.

## Requirement-By-Requirement Scorecard

Scale: `E0` absent, `E1` claimed/scaffolded, `E2` implemented, `E3` verified, `E4` transferred. Half levels express a mixed body of evidence; they are not a new formal level.

| ID | Current level | Strongest current evidence | Missing proof / next gate | Priority |
| --- | ---: | --- | --- | --- |
| `PGE-01` | `E3` professional / `E1` public repo | Current-role partner coordination; cross-functional recommendations; training and talks. | An independent Sparkle integration/adoption exercise with peer feedback and reproducible handoff. | P1 |
| `PGE-02` | `E2.5` | Shipped lighting work; tracing/denoising recommendation; Sparkle reference path tracing, path-traced lighting, ReSTIR, and RT scene code. | Frozen Bistro flagship and San Miguel breadth routes with reference checks, convergence/quality study, captures, latency/memory table, and paired backend result. | P0 |
| `PGE-03` | `E0` | Provider-ready reconstruction plumbing is not a neural feature. | A trained neural rendering vertical slice that replaces a classical path and has a real fallback. | P0 |
| `PGE-04` | `E0` | No tensor/operator/model-to-kernel path found. | PyTorch/ONNX graph analysis, fixed-topology export, HLSL/Slang inference kernels, numerical checks, and optimization study. | P0 |
| `PGE-05` | `E3` professional / `E1.5` repo | 60 FPS console target; cross-platform CPU/GPU/memory optimization; renderer timing/memory/residency surfaces. | Reproducible Bistro and San Miguel benchmark protocol with percentile frame data, causal captures, and regression gates. | P0 |
| `PGE-06` | `E2.5` | Public recommendation names GPU profiling/frame debugging; D3D12/Vulkan diagnostics, markers, validation, capture, and disassembly code exist. | Named PIX/RenderDoc walkthroughs, backend workload comparison, difficult incident report, and reduced reproducer. | P0 |
| `PGE-07` | `E2` | Large modern C++20 codebase, CMake, shader/cooking tools, narrow module boundaries, format/tidy policies. | No Python; no active source test targets; no CI; current worktree does not build; public API and report surfaces remain broad. | P0 |
| `PGE-08` | `E2` | PBR/light-transport talk; BRDF, sampling, temporal, denoising, and reservoir code. | Derivations, CPU references, numerical-error tests, statistics, and predicted-versus-measured performance. | P1 |
| `PGE-09` | `E2.5` | Near-symmetric D3D12/Vulkan RHI, HLSL, SM6-style pipeline, DXC/Slang backends, reflection, cook/cache/runtime packages. | Clean paired run and capture, explicit parity matrix, ABI regression tests, and reviewed shader disassembly. | P0 |
| `PGE-10` | `E2.5` | Professional low-level profiling; task system; render thread/frame queue; allocators, descriptor and queue ownership; shader disassembly. | Counter-driven cache/bandwidth/occupancy experiment, low-level ISA reading, concurrency stress suite, and architecture-scoped conclusions. | P1 |
| `PGE-11` | `E0.5` | A public LLM-systems credential and use of AI tools do not prove ML fundamentals. | A reproducible training project with data split, loss/metric rationale, baselines, ablations, failure cases, and independent verification. | P0 |
| `PGE-12` | `E0` | No training, export, model artifact, or inference workload found. | Separate training and inference profiles, deterministic export, artifact versioning, precision/layout/batch study, and runtime deployment. | P0 |
| `PGE-13` | `E3` | Multiple public talks; four-part frame-graph series; shader/compiler/cooker/launcher tools; extensive design material. | Convert documentation volume into Bistro-led evidence packages with San Miguel breadth; add a bounded graphics-analysis tool and a productization/deletion case study. | P1 |
| `PGE-14` | `E2` | Windows, PC, and multiple console contexts; D3D12 and Vulkan code; English talks and writing; Git/CMake/debugger experience. | Native Linux/Vulkan windowing/build/run/capture; explicit travel status; clean-clone instructions and automation. | P2 at six months, P1 at twelve |
| `PGE-15` | `E2.5` | Approximately nine years across technical-art/rendering roles; shipped titles; current technical leadership; teaching, talks, partner work, and recommendations. | Target source asks as much as 15+ years for the highest level. Need repeated public end-to-end decisions, mentoring/review evidence, external adoption, and simplification. | Continuous |

## Candidate Evidence Audit

### What is already strong

The CV and public profile establish evidence that Sparkle does not need to simulate:

- a shipped 60 FPS console rendering target;
- direct/indirect/volumetric lighting and advanced shader work;
- GPU, CPU, and memory optimization across a wide hardware range;
- a handheld-console rendering adaptation;
- transition from technical art into a dedicated rendering role;
- internal lighting-system leadership and external partner coordination;
- several English technical talks from PBR through production lighting and engine programming;
- recommendations that explicitly validate first-principles reconstruction, custom tracing and denoising, comparisons, profiling, easy handoff, knowledge sharing, and partner work.

This is meaningful principal-track evidence. The unusual advantage is the combination of engine depth and understanding of art/lighting production. Preserve that. Do not rewrite the profile into a generic low-level programmer who appears disconnected from product users.

### What the CV currently under-sells

The one-page PDF creates six avoidable problems:

1. Its typography extracts as spaced characters, making ATS parsing and accessibility unreliable.
2. Most positions have titles and dates but no ownership or outcome bullets.
3. The current role contains the strongest claims, but lacks problem scale, personal action, measurement method, and scope boundaries.
4. D3D12, Vulkan, Python, PIX, RenderDoc, SM6, Slang/DXC, ray tracing, ReSTIR/path tracing, ML, and Linux are not clearly categorized as demonstrated, learning, or absent.
5. Talks are labeled “Publications,” mixing different evidence types.
6. The embedded profile URL is not the current URL supplied for this assessment.

The prior technical-art titles are not a liability if each is expressed as software/rendering ownership. Without bullets, a recruiter may interpret only the 2025–present period as engineering experience.

### Required CV rewrite

Use an ATS-safe single-column source. Keep one primary two-page version and derive a one-page recruiter version.

The top third must contain:

- identity: “Rendering Engineer — Real-Time Graphics, GPU Performance, and Rendering Systems”;
- years and product scope stated truthfully;
- four core areas: C++/HLSL, real-time lighting and ray tracing, GPU profiling/optimization, cross-platform engine integration;
- links to the current public profile, website, GitHub, and the three flagship case studies;
- a compact skills ledger that separates `production`, `independent verified`, and `learning`.

Every recent role needs two to four bullets in this form:

> Delivered `[measurable product result]` under `[platform/frame/memory/production constraint]` by owning `[specific algorithm/system/tool]`, validating with `[capture/metric]`, and coordinating `[relevant teams]`.

Do not publish confidential numbers, imply ownership of team output, or name unreleased details. Use bounded wording such as “owned the optimization investigation for…” or “led the rendering-side integration of…” where accurate.

### Public profile and website

The current public profile is stronger than the PDF:

- it has a focused rendering/performance headline and links to the personal site and GitHub;
- it surfaces the frame-graph series and shipped projects;
- it has roughly two thousand followers and strong recommendations;
- the recommendations provide specific third-party validation rather than generic praise.

The [personal site](https://stoleckipawel.dev/) is clean and credible. Its [About page](https://stoleckipawel.dev/about/) identifies real-time graphics, performance-critical systems, PBR, ray tracing, PC/console, shipped games, contribution examples, and four talks. The landing page, however, is dominated by one frame-graph series. A principal reviewer needs three evidence cards, not only four articles about one architecture topic.

Required site structure:

1. **Shipped performance and partner integration** — public-safe case study.
2. **Sparkle path tracing and D3D12/Vulkan workload study** — code, captures, math, and measured result.
3. **Neural rendering from training graph to shader inference** — dataset/model/runtime/quality/performance.
4. Talks and frame-graph writing as supporting evidence.

## SparkleEngine Evidence Audit

### High-value assets to keep and sharpen

| Evidence surface | Current repository signal | Why it matters |
| --- | --- | --- |
| Explicit graphics backends | About 80 D3D12 files / 14.4k lines and 76 Vulkan files / 12.7k lines under [RHI](../../../Engine/RHI). | Direct match for paired explicit-API ownership. |
| Renderer architecture | About 32.4k source lines under [Renderer](../../../Engine/Renderer), including frame graph, persistent scene data, resources, passes, temporal state, providers, and ray tracing. | Proves system breadth if the review path selects the right slices. |
| Path tracing and ReSTIR | [ray-tracing effects](../../../Engine/Renderer/Private/RayTracing/Effects), [lighting assembly](../../../Engine/Renderer/Private/Frame/Lighting), and [ray-tracing shaders](../../../Engine/Assets/Shaders/Passes/RayTracing). | Best existing flagship candidate for `PGE-02`, `PGE-05`, `PGE-08`, and `PGE-09`. |
| Shader toolchain | [ShaderCompiler](../../../Tools/Shaders/ShaderCompiler) has DXC and Slang backends, reflection, contracts, cache, cooking, inspection, and runtime package support. | Distinctive evidence; stronger than another visual effect. |
| Frame graph | [FrameGraph](../../../Engine/Renderer/Private/FrameGraph) plus the public article series. | Code and communication reinforce one another. |
| Concurrency | [Tasks](../../../Engine/Tasks), immutable extraction, render thread, bounded frame queue, and persistent render data. | Useful for `PGE-05`, `PGE-07`, and `PGE-10` once tested and measured. |
| Asset/cook pipeline | Source import, scene/material/texture cooking, shader cooking, and launcher workflows; FBX and glTF importers already exist. | Bistro FBX and San Miguel OBJ conversion/import can turn this into productization evidence if inventory losses and deterministic outputs are exposed. |

### Trust and verification defects

These defects matter more than their implementation cost suggests:

- The public repository has no root README.
- GitHub shows no repository description, website, or topics.
- The MIT license still contains `[year] [fullname]`.
- There are no public releases or packages.
- `.github` contains no workflows.
- Current source contains no active `add_test` definitions. The existing generated build tree listed two stale task tests: one old executable passed; the dependency-boundary test failed because its referenced CMake script had been deleted.
- The live renderer build regenerated CMake and built core, task, game-framework, D3D12, Vulkan, and provider targets, then failed in `MeshDiagnosticsCollector.cpp` because a diagnostic aggregate no longer matched a changed mesh-handle structure.
- No Python files were found.
- No neural, tensor, ONNX, PyTorch, DirectML, or ML-operator implementation was found.
- Vulkan presentation currently creates a Win32 surface; the repository does not prove native Linux support.
- Public RHI/renderer diagnostics expose many snapshots, reports, services, and capability surfaces. Some may be valuable, but they increase the review burden unless each serves a flagship workflow.

The correct response is not a large testing framework or telemetry platform. It is a small credibility spine:

1. clean-clone configure/build;
2. deterministic Sponza, Bistro, and San Miguel routes at their declared test tiers;
3. one headless or scripted validation path;
4. focused math/ABI/reference tests;
5. Windows CI for build and non-GPU tests;
6. explicit manual GPU-capture gates;
7. release artifact and known-good configuration.

### Discoverability defect

The [public Sparkle repository](https://github.com/stoleckipawel/SparkleEngine) exposes 943 commits and substantial code, but the unauthenticated landing page shows a folder tree, placeholder license, two stars, no README, and “No description, website, or topics provided.”

That is the opposite of the intended signal. It asks the reviewer to infer quality from repository size. A reviewer is more likely to infer unfinished packaging.

The root README should not document the whole engine. It should be a reviewer router:

- one-sentence product identity;
- hero video or image;
- three measured claims with configuration;
- “start here” links to three case studies;
- architecture diagram with five to seven boxes;
- known-good build/run commands;
- support matrix and honest limitations;
- selected code links;
- license and third-party notice.

## What Publicly Successful Profiles Have In Common

### Research method

The review sampled public profiles and official staff biographies of people currently holding principal, staff, senior-staff, or closely comparable graphics/GPU roles, plus strong public graphics portfolios. It did not assume access to private hiring packets or infer why any individual was hired.

The sample deliberately covered:

- production developer-technology and game-engine integration;
- GPU/HPC application optimization;
- real-time rendering and ray tracing research;
- neural-rendering and low-level kernel work;
- open-source/product leadership;
- portfolios that make graphics work easy to review.

Public sources:

- [official principal production-transfer profile](https://developer.nvidia.com/blog/author/pkozlowski/)
- [official principal application-optimization profile](https://developer.nvidia.com/blog/author/alan-gray/)
- [official principal partner-rendering profile](https://developer.nvidia.com/blog/author/iesser/)
- [official principal engine/performance profile](https://developer.nvidia.com/blog/author/ehart/)
- [official principal work-graph/engine biography](https://developer.nvidia.com/blog/?p=78794)
- [public principal ray-tracing/architecture profile](https://cz.linkedin.com/in/tomasdavidovic)
- [public senior-staff neural-rendering/GPU profile](https://jp.linkedin.com/in/adyaman)
- [public principal rendering/open-source profile](https://cz.linkedin.com/in/felix-palmer-153a7624)
- [high-signal rendering portfolio example](https://brandonshihabi.com/)
- [open-source ray-tracing portfolio example](https://tverhoef.com/)

Selection bias is real: public sources overrepresent speakers, authors, researchers, and open-source contributors; confidential industry impact is underrepresented. The patterns below are therefore useful evidence-design patterns, not causal hiring rules.

### Recurring success characteristics

| Anonymous archetype | Recurring public evidence | Allocation lesson |
| --- | --- | --- |
| Production technology-transfer principal | More than a decade in graphics; first-of-kind real-time ray-tracing integration in shipped AAA work; repeated work across games and engines; advanced degree; technical articles and talks. | One shipped feature is not enough at principal level. Show repeated transfer across constraints and make the method teachable. |
| Partner-facing rendering principal | Helps external software teams solve rendering, VR, multi-GPU, large-surface, or large-dataset problems; publishes capture/replay or optimization guidance. | Adoption, debugging, and documentation are core technical output, not soft extras. |
| Engine/performance principal | Twenty-year-scale career; recognizable specialties such as HDR and performance; practical public guidance tied to engine integration. | A narrow specialty repeated deeply over time is more credible than an encyclopedic feature list. |
| Research/architecture principal | Long research career; global illumination, acceleration structures, software architecture; C/C++, Python, Windows, Linux; papers or open source. | First-principles depth, publications, and implementable systems reinforce one another. |
| Neural/GPU senior staff engineer | Graduate graphics research; ray tracing and neural rendering; future-hardware workload evaluation; fully fused kernels written close to the ISA; public launch demo; open-source identity. | The valuable unit is model + kernel + hardware conclusion + visible result, not “used PyTorch.” |
| Product/open-source principal | Strong mathematical degree; founder or long-lived product ownership; maintains open-source software used by others. | Real users and maintenance history can prove influence better than repository size. |
| High-signal public portfolio | Each project has a short problem statement, exact personal work, named hardware/API constraints, images/video, source, write-up, fallback, and measured outcome. | The recruiter should never need to reconstruct the story from directories or commits. |

### What the sample does not show

The successful profiles do not primarily market:

- number of classes, subsystems, or lines of code;
- a general-purpose engine feature checklist;
- long architecture policy documents;
- unsupported “future ready” abstractions;
- screenshots without ownership and measurements;
- machine-learning framework familiarity without an optimized deployed result.

They market recognizable outcomes, repeated ownership, expert depth, transfer to other teams, and public explanation.

## Priority Diagnosis

### Focus now

1. Fix public trust and build reproducibility.
2. Turn the existing path tracer/ReSTIR/backend work into one rigorous Bistro-led classical graphics case study, with San Miguel proving cross-scene breadth.
3. Learn Python and ML through a fixed neural-denoising feature whose output enters Sparkle.
4. Translate that trained model into a small GPU inference implementation and optimize it.
5. Produce a workload-debugging incident and D3D12/Vulkan comparison with real captures.
6. Package all three as reviewer-sized evidence and seek external reproduction.

### Maintain, do not expand

- D3D12/Vulkan RHI;
- frame graph;
- shader compiler/reflection/cook/runtime ABI;
- the curated Sponza/Bistro/San Miguel ladder and one shared deterministic content-inspection path;
- task/render-thread/persistent-scene work needed by the flagship;
- one fallback upscaler/denoiser path;
- essential editor controls for the demo.

### Deprioritize

- new editor panels and general usability work;
- launcher feature breadth;
- additional import formats;
- more raster/post effects;
- more provider integrations;
- generic ECS expansion;
- broad asset-management productization;
- a general ML runtime, training UI, or model marketplace;
- new diagnostics that do not serve a named case study;
- further architecture prose before code, evidence, and the public review path catch up.

### Remove or consolidate when encountered

- stale validation/report surfaces;
- placeholder or reserved feature switches;
- duplicate provider-specific plumbing;
- generated or heavyweight content not needed for the curated scene;
- public diagnostic APIs used only by an internal panel;
- documents whose decisions have been superseded and merged into a canonical source.

## Honest Application Position

Today, the profile is credible for advanced/senior rendering engineering and is a plausible stretch for staff or partner-facing advanced-graphics work. The highest principal target is possible to apply for, but it is not yet a low-risk match:

- the strongest supplied target explicitly asks for 15+ years or research-equivalent depth;
- the CV spans roughly 2017–present, with the dedicated rendering title beginning in 2025;
- the neural/ML half of the target is absent;
- the independent engine is impressive but not yet packaged as independently verifiable influence.

Six focused months can make this a much stronger stretch application by closing the neural-feature and evidence-packaging gaps. Twelve months can add external adoption, Linux/Vulkan, a second hardware-aware optimization study, and a stronger publication/talk cycle.

No roadmap can manufacture tenure. The correct strategy is to make the existing years count clearly, build rare missing depth, and let the application support multiple leveling outcomes without presenting any of them as a personal failure.
