# F. Six-To-Twelve-Month Principal Graphics Roadmap

Status: execution plan
Date: 2026-07-26
Governing requirements: [A. Principal Graphics Engineering Requirements](A_PrincipalRenderingRequirements.md)
Current-state evidence: [C. Candidate and Repository Gap Assessment](C_CandidateAndRepositoryGapAssessment.md)
Canonical workload: [I. Bistro and San Miguel Acceptance Workloads](I_BistroAcceptanceWorkload.md)

## Outcome

At six months, the public body of evidence should support this claim:

> I can own a modern rendering feature across C++, shaders, D3D12/Vulkan, GPU architecture, profiling, math, and integration; I can train a bounded neural model, translate it into a real-time GPU path, measure the quality/performance frontier, and hand the result to another engineer.

The visible proof is a high-quality, performant Bistro exterior/interior flagship backed by San Miguel as a supported cross-scene correctness, indirect-lighting, and neural-generalization workload. Sponza remains the rapid regression loop.

At twelve months, it should additionally support:

> I can transfer the work across a second platform or user, influence direction with evidence, publish the result, and maintain a smaller trustworthy product rather than a personal prototype.

The roadmap assumes roughly 12–15 focused hours per week beside full-time work. If actual capacity differs, preserve the order and gates. Do not run multiple implementation phases in parallel.

`PGE-15` is the longitudinal gate across the entire roadmap: repeated ownership, technical direction, mentoring/review, honest tradeoffs, and simplification must be visible in how all other requirements are completed.

## Allocation

For the first six months:

| Work | Share | Reason |
| --- | ---: | --- |
| Neural graphics, ML, and model-to-kernel work | 40% | Largest and most differentiating `E0` gap. |
| Classical path-tracing and GPU workload evidence | 25% | Converts strong existing code into specialist-verifiable proof. |
| Build, tests, release, and reviewer experience | 20% | Makes all other work credible and discoverable. |
| Writing, talks, external review, interview practice | 15% | Converts personal knowledge into principal-level transfer and influence. |

Do not allocate more than 10% to editor, launcher, general engine usability, content breadth, or indie-engine features unless a flagship case study is blocked without them.

## Non-Negotiable Constraints

- One curated three-tier workload ladder: Sponza fast regression, Bistro primary, San Miguel secondary. Bistro and San Miguel remain external optional packs rather than source-depot weight.
- One classical flagship and one neural flagship, not ten incomplete effects.
- One Python evidence/analysis tool, not a tooling platform.
- One fixed neural topology and artifact format, not a general inference framework.
- One supported known-good hardware/driver configuration first; expand only after it is reproducible.
- D3D12 and Vulkan are both evidence targets on Windows. Linux is a later native slice, not a current claim.
- Every performance claim includes build, resolution, settings, hardware, driver, warm-up, sample count, statistic, and capture link.
- Every “optimization” has a baseline, hypothesis, controlled change, result, and rejected alternative.
- Every flagship has a fallback and a failure story.
- A weak experiment may be deleted and published as a negative result. Keeping it because it took time is not allowed.
- No confidential employer code, screenshots, metrics, unreleased details, or implied ownership enter the public portfolio.

## Six-Month Sequence

## Phase 0 — Freeze And Define, Days 1–3

### Work

- Freeze new engine features.
- Create a one-page backlog containing only tasks that advance `PGE-02`–`PGE-13`.
- Select the primary test GPU, OS build, driver, compiler, resolution, and two rendering configurations.
- Adopt the scene decision and exact gates in [I](I_BistroAcceptanceWorkload.md):
  - Sponza for startup and short regression;
  - Bistro exterior/interior for the flagship;
  - San Miguel 2.0 for supported cross-scene breadth and neural held-out evaluation;
  - external-pack provenance, attribution, immutable source hashes, deterministic transformations, and no heavyweight asset commit by default.
- Write the three case-study titles now:
  1. shipped performance and partner integration, public-safe;
  2. Bistro from source to path-traced, profiled output across D3D12/Vulkan, with San Miguel breadth;
  3. neural GI denoising from PyTorch graph to real-time shader, presented on Bistro and tested on San Miguel.

### Gate

- The backlog has at most 20 items.
- Every item names a `PGE-*` gap and an expected evidence-level transition.
- The backlog contains the week-2 through week-26 Bistro/San Miguel gates from I and does not add a fourth user-facing scene.
- Anything unrelated is moved to “after application” or deleted.

## Phase 1 — Credibility Spine, Weeks 1–2

### Work

1. Make a fresh clone configure and build on a documented known-good Windows setup.
2. Repair the current mesh-diagnostics/build break and delete stale generated test assumptions.
3. Add a small active test target for:
   - shader package ABI/reference decoding;
   - frame-graph dependency/resource-state invariants;
   - task lifetime/dependency settlement;
   - math/reference functions needed by the flagship.
4. Add Windows CI for configure, non-GPU build, tests, formatting/boundary checks, and artifact publication where licensing allows it.
5. Add a root README with:
   - one-sentence identity;
   - one current hero image/video;
   - three evidence cards, with unfinished ones marked “in progress”;
   - seven-box-or-smaller architecture diagram;
   - exact known-good build/run command;
   - support matrix and limitations;
   - links to selected code and case studies.
6. Replace the placeholder license identity and add required third-party/content notices.
7. Set repository description, website, and topics.
8. Tag a known-good baseline release.
9. Record Bistro and San Miguel source pages, licenses, attribution, archive identity/hash, transformation policy, and external-pack layout.
10. Build the reusable asset inspection record needed to count geometry, materials, textures, alpha/emissive state, warnings, conversions, cooked size/time, and deterministic output.

### Gate

- A new clone reaches a deterministic captured frame using documented commands.
- CI is green for non-GPU gates.
- No stale `add_test` or deleted script reference remains.
- A reviewer reaches the executable path, flagship code, and limitations from the README in under two minutes.
- Do not start Phase 2 while the default branch is red.
- Bistro inspection is reproducible, San Miguel provenance is frozen, and no result depends on an undocumented workstation-only conversion.

## Phase 2 — Classical Rendering Proof, Weeks 3–6

### Scope

Use Sparkle’s existing reference path tracing and ReSTIR/real-time lighting work on Bistro and San Miguel. Do not implement a new renderer. Bistro is the case-study spine; San Miguel proves that the content, material, reference, and lighting result is not scene-specific.

### Work

- Import/cook/load Bistro exterior/interior and San Miguel high/low content through the shared deterministic inspection/conversion path.
- Classify every discovered material as exact, converted, approximated, or rejected; retain the honest support/fallback matrix.
- Treat transparent raster materials as a measured P0 gap: the current inspected pipelines disable blending. Implement only the scene-required ordering/compositing/depth behavior and validate it on both APIs; do not confuse stored alpha with rendered transparency.
- Freeze the `BIS-*` and `SMG-*` routes, seeds, scene revisions, shader revision, and settings defined in I.
- Define high-sample references and one or two real-time configurations for both scenes.
- Add CPU or analytical reference checks for the material/sampling components most likely to be wrong.
- Validate:
  - coordinate spaces and depth/motion conventions;
  - BRDF sampling and PDFs;
  - throughput and Russian-roulette logic if present;
  - accumulation/reset behavior;
  - reservoir weight/update behavior;
  - backend resource states, barriers, descriptors, and ray-tracing capabilities.
- Capture D3D12 in PIX and Vulkan in RenderDoc or an equivalent backend-appropriate tool.
- Record:
  - per-pass GPU time;
  - CPU submission/recording time;
  - frame p50/p95/p99 across a fixed run;
  - VRAM high-water and major allocations;
  - rays/samples and resolution;
  - queue utilization and visible synchronization;
  - validation-layer status.
- Compare quality with at least PSNR or FLIP and an explicit temporal/error visualization. Use perceptual images as supporting, not sole, proof.
- Produce material/debug contact sheets and difficult close-ups for both Tier 1 scenes.
- Use the matched San Miguel high/low variants as a controlled geometry-scaling experiment for CPU extraction, draw generation, memory, BLAS/TLAS, and traversal cost.
- Investigate one real performance or correctness issue to root cause and retain the reduced reproducer or smallest failing scene.
- Explain any backend delta above 10%; do not force identical performance.

### Deliverables

- `CASE-01: Bistro And San Miguel From Source Asset To Correct Pixel`
- `CASE-02: One Bistro Frame Across Two Explicit APIs`
- `CASE-03: Path-Traced Bistro Under Budget`
- San Miguel hero/reference comparison and cross-scene performance appendix.
- 60–90 second no-narration comparison video.
- Five-minute narrated architecture/performance video.
- One-page architecture map linked to exact code.
- Capture files or a lawful, compact capture-derived evidence pack.
- Benchmark data and the script that calculates statistics.
- Incident report: symptom → hypotheses → experiments → root cause → fix → regression gate.

### Gate

- `PGE-02`, `PGE-05`, `PGE-06`, `PGE-08`, and `PGE-09` reach at least `E3`.
- A specialist can reproduce the result without reading broad architecture documents.
- No validation error is hidden.
- San Miguel renders through the same material/shader path without a scene-specific shader fork.

## Phase 3 — Python And Workload Lab, Weeks 7–9

### Scope

Build a narrow analysis tool that serves the case studies. Do not build a general profiler or parse proprietary capture formats.

### Work

- Learn and use Python packaging, typing, `pytest`, NumPy, pandas or Polars, and Matplotlib only as needed.
- Emit one versioned, stable benchmark record from Sparkle containing:
  - build and git identity;
  - scene, route, source/cooked manifest, camera, and material-support identity;
  - backend and capability state;
  - hardware/driver/configuration;
  - per-frame CPU/GPU timings;
  - memory high-water;
  - selected pass counters;
  - validation outcome.
- Write a CLI that:
  - validates records;
  - rejects incomparable runs;
  - calculates p50/p95/p99 and confidence intervals or bootstrap ranges;
  - produces one comparison table and two useful plots;
  - applies explicit regression thresholds;
  - never claims causality from timing data alone.
- Add unit tests with malformed, missing, warm-up, and outlier cases.
- Use the tool across Bistro and San Miguel for the classical case study and later neural ablations.

### Gate

- `PGE-07` has real Python evidence.
- Another engineer can run the analysis from a checked-in small data sample.
- The tool replaces manual spreadsheet work and has fewer than five user-facing commands.

## Phase 4 — Neural Feature Definition And Training, Weeks 10–14

### Selected feature

A fixed-topology neural denoiser for low-sample demodulated diffuse indirect lighting.

Why this feature:

- Sparkle already owns path-traced/reSTIR indirect signals and reference output;
- the input/output semantics are understandable and testable;
- it exercises sampling, denoising, temporal data, model training, shader kernels, quality metrics, and real-time budgets;
- it can have a classical spatial/temporal denoiser fallback;
- it does not require a general ML runtime.

### MVP contract

Inputs:

- low-sample demodulated diffuse indirect radiance;
- world/view normal encoded explicitly;
- linear depth;
- roughness or material class only if ablation proves value;
- motion/history only after a correct spatial baseline.

Output:

- denoised demodulated diffuse indirect radiance, remodulated by the existing rendering path.

Initial topology:

- small fixed residual CNN;
- 3×3 convolutions;
- approximately 8–16 feature channels;
- no dynamic shapes;
- FP32 reference and FP16 inference candidate;
- exact layer/operator list frozen in a versioned model contract.

Initial runtime target:

- 1920×1080;
- at most 2.0 ms on the named primary GPU;
- at most 64 MiB incremental persistent/transient memory;
- no unbounded history;
- no CPU readback;
- equal or better objective quality than the classical fallback at an equal measured budget on at least one held-out scene.

The target is a hypothesis. If the hardware cannot meet it, publish the measured frontier and change the product decision, not the data.

### Training work

- Generate inputs and high-sample targets from frozen Sparkle scenes and seeds.
- Separate training, validation, and held-out test scenes/cameras. Bistro supplies the primary presentation routes; the final `SMG-*` cameras remain excluded from training and model selection.
- Record color space, exposure, demodulation, clamping, normalization, and target-sample count.
- Implement a deterministic PyTorch training pipeline.
- Start with L1/Charbonnier and justify any structural or perceptual term.
- Establish:
  - classical denoiser baseline;
  - unprocessed noisy baseline;
  - parameter-count and FLOP estimate;
  - overfit-one-batch sanity check;
  - loss curves and held-out metrics;
  - failure cases on emissive edges, disocclusion, glossy leakage, and exposure extremes.
- Run ablations for input guides, width, precision proxy, and loss terms.
- Create a model card and immutable export manifest.

### Gate

- Training is reproducible from a small public sample or a documented generator.
- Test data never influences model selection except through the declared final evaluation.
- The model beats the noisy input and has a credible path to the classical baseline.
- `PGE-08`, `PGE-11`, and the offline half of `PGE-12` reach `E3`.

## Phase 5 — Model-To-Shader Inference, Weeks 15–19

### Work

- Export weights and the fixed graph into a minimal versioned artifact.
- Write a small Python reference runner for exported artifacts.
- Implement the operator path in HLSL or Slang so it uses the existing DXIL/SPIR-V cook/runtime ABI.
- Do not embed a general ONNX runtime.
- Add tiny-tensor numerical conformance tests between PyTorch, Python export reference, and GPU output.
- Start with direct convolutions, then profile:
  - channel layout;
  - texture versus buffer storage;
  - FP32 versus FP16;
  - dispatch dimensions;
  - shared-memory tiling;
  - layer fusion;
  - weight packing;
  - register pressure and occupancy;
  - bandwidth and cache behavior.
- Preserve a classical fallback and explicit capability/failure state.
- Integrate into the existing frame graph with declared resources and history invalidation.
- Run both D3D12 and Vulkan using the same model artifact.
- Run the same artifact on Bistro and the held-out San Miguel routes; no scene-specific weights or shader branch is allowed.

### Required experiments

1. FP32 correctness baseline.
2. FP16 quality and performance.
3. direct versus tiled convolution.
4. at least one fusion attempt.
5. channel-count quality/performance ablation.
6. cold-start versus steady-state inference.
7. interference with the rest of the frame, not an isolated kernel only.

### Gate

- Numerical tolerance is defined and passes.
- No backend has an unexplained correctness difference.
- `PGE-03`, `PGE-04`, `PGE-05`, `PGE-09`, `PGE-10`, and the inference half of `PGE-12` reach at least `E2`; most should reach `E3` after Phase 6.
- If the model cannot beat the classical alternative at an honest budget, the default remains classical and the result becomes a negative case study.

## Phase 6 — Productization And Transfer, Weeks 20–24

### Work

- Freeze model, code, Bistro/San Miguel manifests, routes, captures, and benchmark versions.
- Run the full held-out and temporal evaluation, including San Miguel generalization and failure cases.
- Produce:
  - PSNR/SSIM or justified quality metrics;
  - FLIP or another rendering-aware error view;
  - temporal stability/error sequence;
  - latency p50/p95/p99;
  - memory high-water;
  - per-layer/operator timings;
  - quality/performance Pareto chart;
  - failure gallery.
- Ask one graphics engineer who did not implement the feature to:
  - clone/build/run;
  - switch classical/neural paths;
  - reproduce one table row;
  - locate the model contract and fallback;
  - report confusing steps and one technical criticism.
- Apply the feedback without broad new infrastructure.
- Write:
  - eight-to-twelve-page technical report;
  - two-page integration guide;
  - one-page model card;
  - ten-slide talk;
  - 90-second comparison video;
  - ten-minute narrated deep dive.
- Publish the Bistro exterior/interior hero pair, the San Miguel hero, and the deterministic Bistro door traversal without forcing reviewers through the editor.

### Gate

- Another engineer reproduces the result.
- The code has an explicit owner, artifact version, capability gate, fallback, and deletion condition.
- The case study states whether the neural result won, lost, or traded quality for performance.
- Bistro and San Miguel each have an honest material/quality/performance support record; the neural result states whether it generalized to the frozen San Miguel test routes.
- Relevant requirements reach `E3`, with `PGE-01` or `PGE-13` reaching `E4` through transfer or publication.

## Phase 7 — Application Package, Weeks 25–26

### CV

- Create ATS-safe two-page and derived one-page versions.
- Give every 2021–present role measurable software/rendering bullets.
- Separate production skills, independently verified skills, and current learning.
- Link only the three strongest evidence pages.
- Remove the generic personal-data consent paragraph unless a specific application legally requires it.
- Call talks “Talks,” articles “Writing,” and papers “Publications.”
- Verify all URLs and PDF text extraction.

### Public profile

- Headline: exact identity and specialty, not an aspirational title.
- About section: four sentences—scope, strongest shipped result, independent flagship, collaboration/communication.
- Featured section:
  1. 90-second reel;
  2. Bistro path-tracing/workload case with San Miguel cross-scene appendix;
  3. neural model-to-shader case;
  4. strongest talk or article.
- Experience bullets match the CV and do not overclaim confidential work.

### Interview package

Prepare three 20-minute stories:

1. shipped performance investigation;
2. D3D12/Vulkan path-tracing incident;
3. neural model-to-kernel optimization.

Each story must answer:

- What was the constraint?
- What did you own?
- What were the competing hypotheses?
- What did the capture/math say?
- What failed?
- What did you change?
- What was measured?
- How did another team adopt it?
- What would you do differently?

### Six-month application gate

Apply when:

- the repository builds from a clean clone;
- the README routes a reviewer in under two minutes;
- the classical case is `E3`;
- the neural case has a real runtime model and honest result;
- one external engineer reproduced a case;
- the CV passes text extraction and contains no stale URL;
- you can defend the code, math, GPU behavior, model, data split, captures, and limitations without notes.

Do not wait for the engine to become an indie product.

## Months 7–12: From Strong Stretch To Sustained Principal Evidence

## Quarter 3 — Platform Breadth And External Contribution

Choose two, not all:

### Native Linux/Vulkan slice

- Add platform/window/input/build support sufficient for the curated scene.
- Use Sponza for the first native smoke, then one Tier 1 route; do not make Linux support wait for every external asset pack.
- Build and run natively on Linux.
- Capture with Vulkan validation and RenderDoc.
- Add non-GPU Linux CI.
- Publish an exact parity/limitation table.

### Upstream graphics-tool or compiler contribution

- Select one real issue in RenderDoc, DXC, Slang, Vulkan tooling, or another directly used open project.
- Reproduce it, discuss the approach with maintainers, submit a bounded patch, and respond to review.
- Prefer a shader reflection, capture, synchronization, or cross-platform defect related to the case study.

### Second-hardware study

- Run the same Bistro and San Miguel model/classical records on a materially different GPU architecture.
- Predict the likely bottleneck before capturing.
- Explain changed cache/bandwidth/occupancy behavior.
- Avoid a universal conclusion from two devices.

Quarter-3 gate: at least one `PGE-06`, `PGE-10`, or `PGE-14` item reaches `E4`.

## Quarter 4 — Influence, Publication, And Product Restraint

- Submit the neural/path-tracing result to a credible graphics conference, developer conference, journal-of-practice venue, or detailed public technical series.
- Deliver the talk publicly or in a recorded review session.
- Mentor or review another engineer’s related implementation and retain public-safe feedback evidence.
- Invite a second adopter or contributor to use the fixed feature boundary.
- Tag a stable Sparkle evidence release with archived captures/data/model card.
- Reduce documentation and runtime surfaces made obsolete by the completed case studies.
- Begin indie-engine usability work only where a real external user is blocked:
  - binary/source onboarding;
  - one project template;
  - asset import and external-pack onboarding for the supported Sponza/Bistro/San Miguel set;
  - crash/issue reporting;
  - stable versioning.

Do not add physics, networking, audio, scripting, marketplace, broad editor tooling, or game-framework depth during this year unless the primary evidence roadmap is complete and an actual user need is documented.

## Weekly Operating Rhythm

For a 14-hour week:

| Work | Hours |
| --- | ---: |
| Flagship implementation | 7 |
| ML/math/GPU architecture study tied to the next experiment | 3 |
| Tests, benchmark, capture, and evidence curation | 2 |
| Technical writing or video | 1 |
| C++/systems interview practice and retrospective | 1 |

Every week ends with:

- one demonstrable result or falsified hypothesis;
- updated benchmark/model record where applicable;
- no red default branch;
- one paragraph: what changed, what was learned, what is next, what was deleted;
- backlog reordered by gap closure, not novelty.

## Backlog Scoring

Score each proposed task from 0–3 on:

- closes a current `E0/E1` requirement;
- strengthens one of the three case studies;
- produces reproducible evidence;
- teaches a missing interview-critical skill;
- enables external adoption;
- removes or consolidates existing code.

Subtract 0–3 for:

- creates a new subsystem;
- needs broad editor/launcher/content work;
- depends on unowned hardware or a fragile SDK;
- produces only a screenshot or claim;
- duplicates an existing engine mechanism.

Do the highest positive score. Reject any task with zero evidence output.

## Case-Study Template

Every public case uses the same order:

1. one-sentence result;
2. problem and product constraint;
3. personal ownership;
4. system boundary and relevant code;
5. baseline;
6. hypothesis and alternatives;
7. math/model/API design;
8. capture and experiment method;
9. result table with configuration;
10. failure cases and limitations;
11. adoption/fallback;
12. what was simplified or deleted;
13. source, data, video, capture, and reproduction links.

The first screen contains the result, not the architecture history.

## Stop List

Stop and reassess immediately if:

- two consecutive weeks produce only infrastructure;
- a neural runtime is being generalized before one topology works;
- a measurement lacks a frozen configuration;
- a case study requires more than ten minutes before the result appears;
- a new feature does not close an identified gap;
- the default branch remains broken while new work continues;
- documentation grows faster than executable evidence;
- work is motivated by removing anxiety rather than answering a technical question.

## Readiness Dashboard

Track monthly:

| Metric | Six-month target | Twelve-month target |
| --- | ---: | ---: |
| Technical `PGE-*` requirements at `E3+` | 11 of 13 technical requirements | 13 of 13 |
| Requirements with external transfer/publication `E4` | 2 | 5 |
| Flagship case studies | 3 including one public-safe professional case | 4 |
| Clean-clone supported configurations | 1 Windows configuration, two graphics backends | plus native Linux/Vulkan |
| Active automated test domains | task, frame graph, shader ABI, math/model export | plus platform and regression coverage |
| Public releases | 1 evidence release | 2 stable evidence releases |
| External reproductions or upstream reviews | 1 | 3 |
| Public talks/articles from the roadmap | 1 | 3 |

The dashboard is a planning instrument, not a résumé claim.
