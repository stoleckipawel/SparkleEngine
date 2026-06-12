# Architecture Review Acceptance Rubric

Status: initial criteria draft
Date: 2026-06-12
Use with: `docs/plans/rhi-renderer-architecture-review.md`
Execution plan: `docs/plans/rhi-renderer-review-ready-implementation-plan.md`

Reviewer architecture docs:

- `docs/architecture/rendering-glossary.md`
- `docs/architecture/rendering-system-map.md`
- `docs/architecture/rhi-contract-map.md`
- `docs/architecture/frame-graph-contract.md`
- `docs/architecture/ray-tracing-contract.md`
- `docs/architecture/pass-authoring-contract.md`
- `docs/architecture/pipeline-runtime-contract.md`

## Purpose

This document defines how we will judge future SparkleEngine RHI/Renderer architecture proposals.

The point is to avoid judging by taste alone. A proposal should be accepted because it improves clear architectural qualities: problem framing, separation of concerns, maintainability, reliability, performance reasoning, operability, portability, and reviewability.

## Source Basis

The criteria below are synthesized from established architecture-review and interview sources:

- AWS Well-Architected Framework: evaluates architectures through consistent questions and tradeoffs around reliable, secure, efficient, cost-effective, and sustainable systems.
  - https://docs.aws.amazon.com/wellarchitected/latest/framework/welcome.html
- Azure Well-Architected Framework: frames architecture review around quality-driven tenets, decision points, and review tools.
  - https://learn.microsoft.com/en-us/azure/well-architected/
- Google Cloud Architecture / Well-Architected guidance: emphasizes reliable, secure, efficient, high-performing, and cost-optimized workloads.
  - https://cloud.google.com/learn/certification/cloud-architect
  - https://docs.cloud.google.com/architecture/framework/reliability
- CMU SEI ATAM: evaluates architecture against quality-attribute goals, exposes risks, and analyzes tradeoffs between goals.
  - https://www.sei.cmu.edu/library/architecture-tradeoff-analysis-method-collection/
- ISO/IEC 25010: provides terminology for software quality evaluation, including product quality characteristics relevant to maintainability, reliability, performance efficiency, security, compatibility, and portability.
  - https://www.iso.org/standard/35733.html
- arc42: recommends documenting crosscutting concepts, architectural decisions, quality requirements, risks, technical debt, and glossary terms.
  - https://arc42.org/overview
- System design interview guidance: common senior/staff evaluation signals include clarifying ambiguous requirements, defining constraints, reasoning about tradeoffs, designing robust systems, and communicating rationale.
  - https://www.hellointerview.com/learn/system-design/in-a-hurry/introduction
  - https://interviewing.io/guides/system-design-interview/part-three
  - https://www.hellointerview.com/guides/meta/e5
- NVIDIA recruiting/interview guidance: reported recruiter guidance emphasizes role-relevant technical skills, problem solving, reasoning, communication, understanding the company's work, and firsthand experience with the technology.
  - https://www.businessinsider.com/nvidia-internship-college-graduates-recruitment-careers-2024-8
- Current graphics/GPU job requirements from NVIDIA/AMD-style roles repeatedly mention C/C++, graphics APIs, shader programming, GPU architecture, rendering algorithms, validation, performance testing, and temporal upscaling.
  - https://jobs.nvidia.com/careers/job/893393627432
  - https://jobs.nvidia.com/careers/job/893393886049
  - https://careers.amd.com/careers-home/jobs/86334?lang=en-us
  - https://careers.amd.com/careers-home/jobs/80570?lang=en-us
  - https://careers.amd.com/careers-home/jobs/86359?lang=en-us
- AMD GPUOpen graphics-programming guidance frames Vulkan and DirectX 12 as APIs that force lower-level understanding of GPU work, explicit resource control, and what happens on the GPU.
  - https://gpuopen.com/learn/how_do_you_become_a_graphics_programmer/
- GitHub portfolio guidance: employers and interviewers may inspect README quality, code, project structure, testing, commit history, screenshots/videos, and whether the project lets them understand the candidate's decisions quickly.
  - https://flatironschool.com/blog/github-profile-and-git-practices-for-job-seekers/
  - https://www.hackerrank.com/blog/what-to-put-on-github/
  - https://coding-boot-camp.github.io/full-stack/github/professional-readme-guide/
- Reference engine/repo presentation patterns: serious public graphics repositories make platform/API support, documentation, build status, modularity, and backend abstraction visible from the repository entry points.
  - https://github.com/NVIDIA-RTX/Donut
  - https://github.com/NVIDIAGameWorks/nvrhi
  - https://github.com/NVIDIAGameWorks/Falcor
  - https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
  - https://github.com/DiligentGraphics/DiligentEngine

## Portfolio Review Skill Signals

Use this section to decide what SparkleEngine should deliberately showcase in a GitHub repo reviewed by NVIDIA, AMD, Intel, Arm, Apple, Epic, Unity, or similar graphics/system-software teams.

This is not a claim that every reviewer will inspect every item. It is a list of qualities that make the repo easier to trust when an engineer does open it.

| Skill / quality to show | What a reviewer is likely trying to infer | Sparkle evidence we should make obvious | Acceptance signal |
| --- | --- | --- | --- |
| Role relevance | The repo matches the role, not just generic programming ability. | README and docs state Sparkle is a C++20 renderer/engine with D3D12, Vulkan, frame graph, ray tracing, shader tooling, and upscaling. | A reviewer can understand the engine's technical scope in under two minutes from repo entry points. |
| Modern C++ systems skill | Candidate can build maintainable low-level systems, not just isolated demos. | Ownership model, RAII patterns, allocator/resource lifetime rules, command abstractions, build profiles, clear module boundaries. | Core engine code has coherent ownership, narrow interfaces, no obvious lifetime shortcuts, and build commands documented. |
| Graphics API fluency | Candidate understands D3D12/Vulkan semantics, resource states/layouts, descriptors, synchronization, PSOs, and swap chain/presentation. | RHI contracts, D3D12/Vulkan backend parity table, command list implementations, barrier/resource diagnostics. | D3D12 and Vulkan behavior is documented as shared semantics plus explicit API-specific differences. |
| Shader and pipeline systems | Candidate understands host/shader contracts, reflection, cooked packages, binding layouts, PSO keys, permutations, and reload/caching. | Shader compiler/cook path, shader package cache, pass runtime, PSO key design, pass authoring contract. | Adding an ordinary renderer pass requires no RHI edits and has a short documented path. |
| Rendering fundamentals | Candidate understands lighting, PBR, GBuffer data, normals, depth, shadows, temporal behavior, and debug views. | GBuffer, lighting, shadow, visualize-buffer, ray-tracing, and upscaling passes with screenshots/captures. | Lit and normal/debug captures exist for both D3D12 and Vulkan, with known-difference notes. |
| GPU architecture and performance reasoning | Candidate can reason about memory bandwidth, cache behavior, occupancy, synchronization, CPU/GPU work split, and measurement. | Profiling notes, GPU markers, timing reports, memory monitor, performance hypotheses separated from measurements. | Performance claims include data or an explicit measurement plan; no undocumented "faster" claims. |
| Cross-backend architecture | Candidate can separate renderer intent from backend implementation. | RHI/Renderer layer map, forbidden include checks, backend-private folders, vendor SDK boundaries. | `RHI -> Renderer` dependency count is zero and renderer does not include backend-private headers. |
| Debuggability and validation | Candidate can make failures explainable and reproducible. | Frame graph diagnostics, validation layers, smoke validation, capture/readback utilities, capability reports. | Development smoke runs fail on unresolved graph resources and produce per-backend evidence. |
| Reliability and fallback behavior | Candidate handles unavailable features deterministically. | DLSS/RT/device capability reporting, passthrough upscaler, backend feature checks. | Missing DLSS, ray tracing, or extension support produces an actionable reason and stable fallback. |
| Testability and CI thinking | Candidate can prove behavior repeatedly, not manually hope it works. | Build profiles, smoke tests, shader compiler validation, formatting/tidy checks, backend parity reports. | README/docs list exact validation commands and expected artifacts. |
| Documentation and onboarding | Candidate can make a complex system reviewable by strangers. | Architecture review, glossary, system map, pass authoring guide, screenshots, diagrams, run instructions. | A reviewer can find: how to build, how to launch D3D12/Vulkan, how a frame is rendered, and where to add a pass. |
| Communication and design rationale | Candidate can explain tradeoffs and not just dump code. | Design notes, acceptance criteria, risks, non-goals, rejected alternatives, architecture decision records. | Strategic changes include owner, alternatives, risks, validation, and rollback plan. |
| Git/review hygiene | Candidate works in a way that could fit a serious engineering team. | Small commits, descriptive commit messages, no generated junk in source history, clean docs, CI status where possible. | Public history shows reviewable increments rather than only one massive unreviewable drop. |
| Product/demo clarity | Candidate can make technical work inspectable quickly. | Screenshots/videos, sample project, launcher/editor instructions, feature matrix, known issues. | Repo front page links to current screenshots and a reproducible Showcase launch path. |
| Collaboration readiness | Candidate understands open-source/team maintenance expectations. | CONTRIBUTING notes, issue templates or bug report guidance, license, coding standards, diagnostics instructions. | External reviewer knows how to build, report a bug, run checks, and understand project status. |

## Sparkle Portfolio Acceptance Checklist

Before using SparkleEngine as a portfolio artifact for NVIDIA/AMD-style interviews, the repo should have these visible signals:

- A top-level README section that explains the renderer/RHI architecture, supported APIs, current features, known limitations, and exact build/launch commands.
- A feature matrix covering D3D12, Vulkan, ray tracing, debug view modes, DLSS/upscaling, shader compiler/cook, frame graph diagnostics, and smoke validation.
- A short "reviewer's path" that points to the most representative systems: RHI contract, Vulkan backend, D3D12 backend, frame graph, pass authoring, shader runtime/PSO, ray tracing, and upscaling.
- Screenshots or captures for D3D12 and Vulkan lit output plus GBuffer normal/debug modes.
- A documented "add a shader pass" walkthrough showing that ordinary pass work stays in Renderer/shaders/tools and does not require RHI edits.
- Architecture diagrams for layer direction, frame execution, shader package flow, PSO creation, and backend parity.
- Validation commands that a reviewer can run locally, plus the expected log/capture artifacts.
- A known-issues section that is honest about remaining gaps instead of hiding them.
- CI or local scripts for formatting, forbidden include checks, shader compiler validation, and runtime/editor smoke where practical.
- Commit and PR conventions that make large rendering changes reviewable.

## The Criteria

Use this table for architecture/design proposals. A proposal does not need to be perfect in every category, but weak scores must be explicit risks.

| Criterion | What Reviewers Usually Look For | Sparkle Renderer/RHI Acceptance Evidence |
| --- | --- | --- |
| Problem framing | The candidate/proposal clarifies what is being solved and what is out of scope. | Proposal names the exact subsystem, bug class, performance goal, or review concern. Non-goals are listed. |
| Requirements and constraints | Functional and non-functional requirements are made explicit before design. | D3D12/Vulkan parity, ray tracing, DLSS, frame graph, debug view, shader cooking, and platform constraints are stated. |
| Separation of concerns | Clear module ownership and dependency direction. | RHI does not include Renderer private headers. Renderer does not include backend-private D3D12/Vulkan headers. Vendor SDK code is isolated. |
| Cohesion and interface size | Interfaces are focused; classes have one understandable reason to change. | Large interfaces such as `RenderHardwareInterface` are classified by responsibility before new methods are added. |
| Tradeoff reasoning | Alternatives are considered; tradeoffs are named instead of hidden. | Proposal compares at least two options when changing RHI contracts, frame graph ownership, memory lifetime, or native interop. |
| Quality attributes | Design is judged against reliability, performance, maintainability, portability, operability, and security/safety where relevant. | Proposal includes a quality-attribute impact section, even if some entries say "not affected." |
| Risk and technical debt visibility | Known risks are listed, prioritized, and connected to mitigation. | Proposal adds or updates a risk table when it changes API contracts, backend behavior, shader-visible layouts, or synchronization. |
| Runtime behavior clarity | The system can be explained as an execution path, not just a folder tree. | Diagrams or ordered flows show frame setup, pass declaration, graph compile, resource resolution, command recording, submission, and presentation. |
| Observability and diagnostics | Failures can be found and explained. | New architecture preserves logs, validation layers, debug names, GPU markers, smoke evidence, and capability reports. |
| Reliability/failure handling | Expected failures have deterministic handling. | Missing DLSS/RT support falls back with reason. Resource resolution failures are not silent. Device capability gaps are reported. |
| Performance reasoning | Performance claims are supported by measurement or marked as hypotheses. | Proposal identifies likely GPU/CPU bottlenecks and gives a measurement plan before claiming improvement. |
| Portability/backend parity | API-specific behavior is contained; shared semantics are explicit. | D3D12/Vulkan differences are mapped at the RHI boundary, with parity tests or known-difference notes. |
| Maintainability and naming | Names reveal role and ownership; file locations help reviewers navigate. | Orchestration files, implementation files, contracts, and backend files follow documented naming rules. |
| Testability | Claims can be validated repeatedly. | Proposal includes targeted build/smoke tests and, for graphics, capture or log evidence for both APIs where relevant. |
| Communication/reviewability | The design can be reviewed by someone new to the repo. | Proposal includes diagrams, owner map, decision record, acceptance criteria, and exact files touched. |

## Score Scale

Use a 0-3 score for each criterion.

| Score | Meaning |
| --- | --- |
| 0 | Missing. The proposal does not address this criterion. |
| 1 | Weak. It is mentioned but lacks evidence or clear acceptance criteria. |
| 2 | Acceptable. It has evidence and a reasonable validation path. |
| 3 | Strong. It is explicit, measured or testable, and easy for an external reviewer to audit. |

Suggested decision rule:

- Accept: no 0 scores, no critical category below 2, and total score is at least 70%.
- Revise: any critical category is 1, or total score is 50-69%.
- Reject/defer: any critical category is 0, or total score is below 50%.

Critical categories for RHI/Renderer work:

- Requirements and constraints
- Separation of concerns
- Tradeoff reasoning
- Runtime behavior clarity
- Observability and diagnostics
- Portability/backend parity
- Testability

## Sparkle-Specific Review Questions

Ask these during each design session.

1. What exact behavior, risk, or review concern is this proposal solving?
2. Which layer owns the decision: RHI common, D3D12 backend, Vulkan backend, Renderer frame graph, Renderer pass, shader compiler/cook, or vendor provider?
3. What would NVIDIA/AMD reviewers need to inspect first to trust this design?
4. What are the D3D12 and Vulkan semantics, and where do they intentionally differ?
5. Does this add a renderer concept to RHI, or an API concept to Renderer?
6. Does it change shader-visible layout, resource state, descriptor lifetime, command ordering, memory lifetime, or synchronization?
7. What logs, validation messages, smoke tests, captures, or debug views prove it works?
8. What failure path is expected, and is that path deterministic?
9. What alternative was rejected, and why?
10. What future change becomes easier after this?

## Rubric Template

Copy this into future design notes.

```markdown
## Acceptance Rubric

| Criterion | Score | Evidence | Risk/Follow-up |
| --- | ---: | --- | --- |
| Problem framing |  |  |  |
| Requirements and constraints |  |  |  |
| Separation of concerns |  |  |  |
| Cohesion and interface size |  |  |  |
| Tradeoff reasoning |  |  |  |
| Quality attributes |  |  |  |
| Risk and technical debt visibility |  |  |  |
| Runtime behavior clarity |  |  |  |
| Observability and diagnostics |  |  |  |
| Reliability/failure handling |  |  |  |
| Performance reasoning |  |  |  |
| Portability/backend parity |  |  |  |
| Maintainability and naming |  |  |  |
| Testability |  |  |  |
| Communication/reviewability |  |  |  |

Decision: Accept / Revise / Reject
Critical blockers:
Next validation:
```

## How This Changes Our Planning

Before implementing a renderer/RHI architecture change, we should now require:

1. A small design note.
2. A layer ownership statement.
3. A D3D12/Vulkan impact statement.
4. A quality-attribute impact statement.
5. A validation plan.
6. A rubric score.

This does not mean every tiny bug fix needs ceremony. It means structural changes should be reviewable before they become code.
