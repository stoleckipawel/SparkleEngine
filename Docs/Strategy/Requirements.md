# A. Principal Graphics Engineering Requirements

Status: strategy contract; canonical vendor-neutral capability baseline
Date: 2026-07-26
Scope: principal-level real-time graphics, developer technology, GPU systems, rendering research productization, workload tooling, and neural graphics

## Authority And Reading Order

This is the canonical requirements contract for SparkleEngine and the target engineering persona. The stable identifiers are `PGE-01` through `PGE-15`.

- [B. Role Source Archive](RoleSources.md) preserves the supplied screenshots and CV in normalized text and maps every source bullet to these identifiers.
- [C. Candidate and Repository Gap Assessment](Assessments/GapAssessment.md) grades current evidence.
- [F. Release-First Principal Graphics Roadmap](Roadmap.md) converts the gaps into sequenced work.
- [G. Advanced Graphics Engine Executive Summary](ExecutiveSummary.md) is the short engine decision document.
- [H. Advanced Graphics Engineer Persona](EngineerPersona.md) describes who the engineer must become.
- [I. Bistro and San Miguel Acceptance Workloads](../Acceptance/GraphicsWorkloads.md) defines the canonical Sponza/Bistro/San Miguel workload ladder and the exact proof produced by the two Tier 1 scenes.
- [First Release Feature Completion Reports](../Acceptance/FeatureCompletionReports.md) applies these targets to per-feature polish and explanation; it cannot assign a higher `PGE-*` evidence level without the proof required here.

When another document conflicts with this matrix, this matrix controls the capability target and the evidence meaning. Architecture documents may impose stricter implementation rules.

## North-Star Requirement Crosswalk

The persona's [`NS-*` North Star outcomes](EngineerPersona.md#north-star) and these capability requirements are complementary. `NS-*` asks whether an iteration behaves like trustworthy principal engineering; `PGE-*` asks which capability and evidence level it advances. Every material iteration marks both, but only when applicable.

| North Star outcome | Primary capability targets | Iteration question |
| --- | --- | --- |
| `NS-REAL` | `PGE-01`, `PGE-02`, `PGE-03`, `PGE-07`, `PGE-09`, `PGE-13`, `PGE-15` | Is there a complete consumer-visible or adopter-visible production result rather than vocabulary or scaffolding? |
| `NS-MATH-DATA` | `PGE-02`, `PGE-03`, `PGE-04`, `PGE-08`, `PGE-09`, `PGE-10`, `PGE-11`, `PGE-12` | Are semantics, math, numerical behavior, data transformations, layouts, and cost understood and tested? |
| `NS-EVIDENCE` | `PGE-02` through `PGE-12`, `PGE-14` | Do the declared correctness, quality, performance, API/compiler/driver, neural, and failure claims have reproducible evidence? |
| `NS-OWNERSHIP` | `PGE-01`, `PGE-05`, `PGE-07`, `PGE-09`, `PGE-10`, `PGE-12`, `PGE-15` | Are authority, lifetime, synchronization, capability selection, fallback, failure, and retirement explicit? |
| `NS-ADOPTION` | `PGE-01`, `PGE-06`, `PGE-07`, `PGE-13`, `PGE-14`, `PGE-15` | Can a non-author discover, integrate, reproduce, diagnose, support, and communicate the result? |
| `NS-SIMPLIFY` | `PGE-07`, `PGE-10`, `PGE-13`, `PGE-15` | Did the work delete superseded authority and reduce maintenance or public surface without losing capability? |

The crosswalk is a routing aid, not evidence. An iteration records each selected `PGE-*` as `advance`, `preserve`, `not applicable`, or `blocked`; a grouped range never hides an individual target's result. [Change Lifecycle](../Engineering/Workflow/ChangeLifecycle.md#create-the-iteration-control-record) owns the iteration record format.

## Target Proposition

Become the engineer who can take an ambiguous rendering, GPU-compute, workload-analysis, or neural-graphics problem from first principles to a measured product result in an unfamiliar engine.

That engineer:

- owns C++, shaders, explicit graphics APIs, CPU/GPU behavior, tools, and runtime integration rather than only one layer;
- translates research prototypes and model graphs into efficient, debuggable product code;
- diagnoses application, API, driver, compiler, and hardware behavior without confusing correlation with cause;
- makes another team faster through narrow interfaces, reduced reproducers, integration guidance, reviews, and clear tradeoffs;
- can prove correctness, image quality, latency, memory, portability, and failure behavior;
- publishes a concise technical narrative that a recruiter, hiring manager, specialist, or conference audience can evaluate without reading the whole repository.

SparkleEngine's priority order is:

1. evidence for this persona;
2. a focused research and learning platform that produces that evidence;
3. a usable independent game engine, where product work reinforces rather than competes with the first two priorities.

The product workload decision is fixed for this planning horizon:

1. Sponza is the Tier 0 startup and rapid regression scene;
2. Bistro exterior/interior is the Tier 1 primary flagship;
3. San Miguel 2.0 is the Tier 1 secondary supported scene and cross-scene quality/generalization test.

The engine is not permitted to substitute Sponza-only proof for a Tier 1 requirement. The detailed acquisition, material, quality, performance, neural, and presentation gates are binding in [I](../Acceptance/GraphicsWorkloads.md).

## Evidence Scale

Every grade in the assessment uses this scale. File count or a class name is never sufficient by itself.

| Level | Meaning | Acceptable proof |
| --- | --- | --- |
| `E0` | Absent | No truthful claim or artifact. |
| `E1` | Claimed or scaffolded | CV/profile claim, interface, placeholder, or unverified code path. |
| `E2` | Implemented | Reviewable code or a completed professional example, but no independently reproducible public result. |
| `E3` | Verified | Clean build/run, deterministic scenario, tests or reference checks, captures, measurements, configuration, and limitations. |
| `E4` | Transferred | Another engineer adopted or reproduced it; result is shipped, externally reviewed, published, upstreamed, or taught with credible feedback. |

A principal-ready portfolio needs `E3` on every technical core requirement and `E4` on several requirements involving productization, collaboration, and communication. Credentials and confidential employment evidence may satisfy parts of a requirement without being stored in the repository.

## Canonical Requirement Matrix

| ID | Capability | The engineer must be able to | Required portfolio evidence |
| --- | --- | --- | --- |
| `PGE-01` | Partner adoption and cross-functional collaboration | Discover another team's constraints; integrate advanced rendering or AI technology into an existing engine; coordinate research, hardware, driver, compiler, content, and production concerns; give useful review feedback. | One adoption case study with requirements, constraints, alternatives, integration steps, failure/fallback behavior, measured result, review history, and external or peer reproduction. |
| `PGE-02` | Real-time ray tracing, GI, and path tracing | Derive and implement raster, ray-query/pipeline, global-illumination, sampling, accumulation, and path-tracing techniques under a real-time budget. | Bistro path-traced flagship evidence plus San Miguel cross-scene evidence: deterministic references, quality comparison, latency and memory budgets, temporal behavior, paired backend captures, and honest limitations. |
| `PGE-03` | Neural graphics product feature | Replace or materially improve a classical rendering path with a real trained model or neural operator path rather than an empty provider abstraction. | Model and data provenance, immutable artifact, real runtime inference, classical baseline/fallback, quality metrics, temporal evaluation, latency, memory, and supported-capability matrix. |
| `PGE-04` | Model-to-kernel translation and GPU kernel optimization | Read PyTorch/ONNX-like graphs; understand operator mathematics and computational requirements; translate a fixed model into efficient HLSL/Slang or compute kernels; tune layouts, precision, fusion, memory traffic, and dispatch. | Operator derivation, tensor shapes/layout, fixed-topology export, shader/kernel implementation, numerical reference checks, per-stage profile, disassembly/counters where useful, ablations, and quality/performance frontier. |
| `PGE-05` | Whole-system performance engineering | Treat CPU work, GPU work, memory, queues, shader compilation, residency, streaming, frame pacing, input-to-present latency, and concurrent workloads as one bounded system. | Repeatable Bistro and San Miguel benchmark protocol; warm-up and sample policy; p50/p95/p99; CPU/GPU timelines; memory high-water; causal experiments; before/after result; regression threshold. |
| `PGE-06` | Graphics workload analysis and hard debugging | Analyze how D3D12 and Vulkan workloads use queues, command lists, barriers, descriptors, memory, pipelines, shaders, and presentation; isolate application, API, driver, and hardware causes; use PIX, RenderDoc, native validation, and equivalent profilers effectively. | One workload comparison and one difficult incident report with capture markers, exact hardware/driver/build configuration, minimal reproducer, competing hypotheses, experiment log, root cause, fix, and scoped conclusion. |
| `PGE-07` | C++ and Python software engineering | Write cohesive modern C++ and practical Python; debug ownership, lifetime, races, numerical errors, and API misuse; design durable systems; maintain coding standards and high-quality documentation. | Clean-clone build, narrow public APIs, representative C++ review, a useful Python automation/analysis tool, tests, sanitizer or native-validation evidence where applicable, and documentation that matches behavior. |
| `PGE-08` | Applied mathematics and performance modeling | Use linear algebra, calculus, differential reasoning, probability, statistics, Monte Carlo estimators, stochastic optimization, signal processing, and numerical analysis to make rendering and ML decisions. | Feature math note with definitions and assumptions, CPU reference implementation or known values, error/stability tests, predicted cost or quality behavior, and measurement that confirms or falsifies the model. |
| `PGE-09` | Explicit APIs, shaders, compilers, and GPU ABI | Work deeply with D3D12, Vulkan, HLSL/SM6, GLSL or Slang, DXIL/SPIR-V, reflection, shader packaging, binding layouts, synchronization, graphics/compute/ray pipelines, and capability/fallback behavior. | Paired backend vertical slice, shader-source-to-runtime trace, reflection/ABI checks, shader inspection, resource-state explanation, compiler diagnostics, and fallback verification. |
| `PGE-10` | CPU/GPU architecture and low-level concurrency | Explain CPU cache/topology/SIMD/threading and GPU cores/waves/cache/bandwidth/registers/occupancy/divergence/atomics/memory model; read useful low-level code or ISA; use modern concurrency without lifetime or oversubscription mistakes. | Counter/disassembly-informed optimization, serial control, cache or bandwidth model, thread/task trace, concurrency stress evidence, rejected alternatives, and architecture-scoped conclusion. |
| `PGE-11` | Machine-learning fundamentals | Understand automatic differentiation, computational graphs, broadcasting, data splits, objectives, loss and evaluation metrics, optimization, overfitting/generalization, quantization, and deployment constraints. Use AI coding tools as fallible accelerators. | Reproducible training experiment, model card, train/validation/test separation, baseline, ablation, failure cases, independent verification of AI-assisted work, and no unsupported model claim. |
| `PGE-12` | Training and inference workload engineering | Treat offline preparation/training and low-latency inference as different systems; profile batching, precision, layout, memory, concurrency, export, startup, and runtime scheduling. | Separate training and inference profiles, deterministic export, versioned artifact contract, precision/batch/layout sweep, runtime loading path, latency/memory budget, and deployment decision. |
| `PGE-13` | Research productization, tools, and technical communication | Turn a proof of concept into mature code, a useful graphics tool or plugin, or a deleted negative result; write design notes and best practices; build demos; present conference- or whitepaper-quality work; prioritize under limited time. | Hypothesis, source review, prototype, productization/deletion decision, tool or integration surface, code review, polished Bistro flagship demo, San Miguel breadth proof, concise technical paper, talk outline or recording, and priority/deletion ledger. |
| `PGE-14` | Platform and ecosystem breadth | Develop and debug on Windows; demonstrate native Linux/Vulkan work before claiming Linux; understand source control, build systems, debuggers, profilers, and driver-facing workflows; work effectively in written and spoken English and travel when the role requires it. | Windows/D3D12 and Windows/Vulkan evidence; a native Linux/Vulkan build-run-capture slice if claimed; reproducible build instructions; Git history; tool workflow; clear English writing and talk evidence. |
| `PGE-15` | Principal-level judgment and sustained influence | Supply depth equivalent to advanced education or substantial relevant experience; set technical direction from first principles; repeatedly own high-risk work end to end; mentor, review, organize, and simplify rather than accumulate architecture. | Several completed vertical slices over time, shipped impact, decisions under constraints, peer/partner recommendations, mentoring or teaching evidence, incident leadership, and a repository that became easier to understand and change. |

## Requirement Coverage From The Supplied Sources

The source archive assigns each individual source bullet to one or more IDs. At cluster level:

| Supplied source cluster | Canonical coverage |
| --- | --- |
| Principal advanced rendering and AI adoption | `PGE-01`–`PGE-15` |
| D3D12/Vulkan workload-analysis tools | `PGE-01`, `PGE-05`–`PGE-07`, `PGE-09`, `PGE-10`, `PGE-13`–`PGE-15` |
| Graphics software development and advanced rendering | `PGE-01`, `PGE-02`, `PGE-05`, `PGE-07`–`PGE-10`, `PGE-11`, `PGE-13`–`PGE-15` |
| Neural-rendering research inference | `PGE-01`, `PGE-03`–`PGE-15` |
| Research-to-product advanced rendering | `PGE-01`–`PGE-05`, `PGE-07`–`PGE-15` |
| Short public hiring summary | `PGE-02`, `PGE-05`–`PGE-07`, `PGE-09`, `PGE-14` |

No supplied responsibility or qualification is outside the matrix.

## Non-Repository Requirements

SparkleEngine cannot prove these alone:

- years of full-time relevant employment;
- degree subject and level;
- confidential shipped-product scope;
- cross-company collaboration;
- code-review or mentorship impact inside an employer;
- spoken communication and willingness to travel.

Use a CV, public profile, recommendations, talks, references, and interview stories for these. Never turn an employment or credential gap into a false repository claim.

## Portfolio Review Contract

The portfolio is ready only when all four review paths work:

| Reviewer | Time budget | They must see |
| --- | --- | --- |
| Recruiter | 60–90 seconds | Exact professional identity, current role level, three relevant outcomes, flagship demo image/video, target skills, location/travel status, and working links. |
| Hiring manager | 10 minutes | Three evidence cards: problem, constraints, personal ownership, implementation, measured outcome, code/capture/write-up links, and limitations. |
| Graphics specialist | 45–60 minutes | Math, API/resource states, GPU captures, shader/kernel code, profiling method, backend differences, model/data decisions, alternatives, and failure cases. |
| Partner or adopter | 30 minutes plus build time | Clean-clone instructions, supported configuration, sample workload, integration surface, fallback, diagnostics, expected output, and issue/reproducer path. |

The whole engine is not the portfolio. It is the evidence substrate. The public surface must select and explain the small parts that prove the matrix.

Scene-specific assets, routes, `CASE-01` through `CASE-05`, and presentation gates belong to [I. Acceptance Workloads](../Acceptance/GraphicsWorkloads.md). This document owns only the reviewer/evidence meaning above.

### Case-Study Structure

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

## Completion Rule

Do not add a feature because its noun appears in a job description. Add or retain work only when it advances a named `PGE-*` requirement to a higher evidence level and produces an artifact a reviewer can inspect.

If two possible tasks advance the same requirement, prefer the one that:

1. closes an `E0` or `E1` gap;
2. strengthens one of the three flagship case studies;
3. yields causal measurement or external reproduction;
4. reduces or consolidates more code than it adds;
5. improves the 10-minute review path.
