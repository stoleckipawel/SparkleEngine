# H. Advanced Graphics Engineer Persona

**Status:** operating model; personal target, not an implementation standard or evidence matrix

**Current evidence:** [Current Feature Readiness](../Acceptance/CurrentReadiness.md) owns feature scores. This page describes the engineering judgment to develop.

**Date:** 2026-08-02

**Scope:** principal-level advanced graphics, developer technology, rendering, GPU systems, and neural graphics engineering

## Read This Page

- **See the target:** [Persona statement](#persona-statement) and [technical pillars](#technical-pillars).
- **Apply it to work:** [Think, build, review and communicate](#operating-model); [principal judgment](#principal-level-judgment).
- **Check alignment:** [North Star](#north-star) and [authority](#purpose-and-authority-boundary).

## Persona Statement

Become the graphics systems engineer who can take a rendering or GPU-compute problem from research-shaped ambiguity to a lean, measured, product-quality implementation that another engineer can adopt.

The target is not a generalist who knows engine vocabulary. It is an engineer who reasons from first principles across C++, shaders, explicit graphics APIs, CPU/GPU architecture, data and concurrency, debugging tools, research translation, and product constraints.

> I build advanced real-time graphics systems that are technically deep, causally measured, honest about limits, and clean enough for another team to own.

## Technical Pillars

### Explicit Graphics API Ownership

Understand and modify D3D12 and Vulkan device, queue, command, resource, descriptor, pipeline, synchronization, memory, presentation, and capability behavior without leaking backend policy into renderer code.

Be able to explain the complete path from renderer intent through frame-graph scheduling, neutral RHI contracts, backend-native commands, GPU completion, and deferred retirement.

### Renderer Feature Depth

Own features across CPU preparation, scene/resource data, pass scheduling, shader execution, temporal state, backends, configuration, fallback, debugging, and quality/performance validation.

Depth matters more than feature count. One path-traced or neural feature that survives realistic workloads, both supported APIs, failure cases, and a skeptical review is stronger than many incomplete toggles.

### Shader and Kernel Craft

Work fluently in HLSL/SM6 and understand GLSL, Slang, DXIL, and SPIR-V well enough to reason about cross-target ABI and compiler behavior. Make layout, precision, memory traffic, wave behavior, divergence, occupancy, register pressure, dispatch shape, and numerical stability explicit.

Treat shader packaging, reflection, parameter layout, and cooked runtime ABI as product code rather than build-system trivia.

### CPU/GPU Architecture Thinking

Connect algorithm choices to CPU caches, SIMD, topology, tasks, allocation, and submission overhead, and to GPU waves, caches, bandwidth, latency hiding, occupancy, atomics, descriptors, queues, and acceleration-structure cost.

Use captures, counters, traces, disassembly, controlled experiments, and serial/reference paths to distinguish cause from correlation. Scope conclusions to the hardware, driver, compiler, API, build, and workload actually tested.

### Neural Graphics Productization

Understand data, training/evaluation boundaries, model graphs, operators, precision, export, deployment, inference scheduling, quality metrics, generalization, and failure cases well enough to translate a bounded model into efficient GPU work.

Keep offline experimentation separate from runtime ownership. A real neural feature has a deterministic artifact, validated operator path, classical baseline/fallback, measured quality/latency/memory, and an honest capability boundary. Empty tensor frameworks, mock models, provider enums, and disconnected microbenchmarks are not completion.

### Debugging and Developer Technology

Use PIX, RenderDoc, Nsight, native validation, shader/compiler diagnostics, crash data, markers, timestamps, and reduced reproducers to solve difficult application/API/driver/hardware problems.

Design integration surfaces, fallback behavior, diagnostic entry points, and documentation so another engineer can configure, capture, debug, tune, and maintain the result without broad internal access.

## Operating Model

### Think

- Start from authority: who owns mutable state and when can it change?
- Start from the frame: which pass reads/writes each resource and on which queue?
- Start from data: what is produced, consumed, transferred, retained, and measured?
- Start from the shader: what layout, precision, traffic, and execution pattern does it create?
- Start from the API: which state, synchronization, lifetime, or capability rule can fail?
- Start from the adopter: how is the feature selected, integrated, debugged, and retired?
- Start from deletion: which old path, abstraction, or depot weight can the result remove?

### Build

- Prefer complete vertical slices over broad infrastructure.
- Prefer direct ownership over wrappers and service locators.
- Keep public contracts smaller than private implementation.
- Establish correctness and a serial/reference contract before parallel optimization.
- Stabilize the product path before a profiling campaign.
- Preserve real capabilities while deleting scaffolding and duplicated authority.
- Integrate research only when it answers a current product problem and has a deletion/fallback decision.

### Review

- Trace lifetime, publication, failure, cancellation, and reclamation end to end.
- Ask what evidence would falsify the design or performance claim.
- Separate API correctness, driver behavior, hardware behavior, and application policy.
- Reject abstractions without current consumers and measurements without causal controls.
- Treat AI-generated code, tests, math, citations, shader logic, and claims as untrusted until independently checked.
- Leave the ownership path easier to navigate than before.

### Communicate

- Use precise technical language and distinguish fact, measurement, inference, and hypothesis.
- State exact support, fallback, configuration, limitations, and unavailable evidence.
- Explain negative results and rejected alternatives as clearly as successes.
- Produce the shortest artifact that lets its intended audience reproduce or adopt the result.
- Never use documentation volume, vendor vocabulary, or unsupported credentials as a substitute for implementation evidence.

## Principal-Level Judgment

### Research-Informed Professional Focus

The selected focus is rendering/developer technology with GPU-system depth, supported by neural productization and useful tools. [Actual role families](Research/GraphicsRoleDepth.md) distinguish this from compiler, driver and research-scientist specialties. [Public engineers](Research/GraphicsEngineerProfiles.md) show several valid shapes: research-to-game integration, explicit API optimization, mathematical reconstruction, high-performance kernels and small durable tools. Their titles and GitHub portfolios are examples, not an internal NVIDIA ladder or proof that copying their projects leads to hiring.

Develop depth through the existing three headline cases. For each, explain the problem selection, exact personal contribution, mathematical/API assumptions, executed source identity, quality/cost tradeoff and external feedback. A principal-shaped result includes the decision that limited scope, removed authority or rejected an attractive optimization. Do not turn every neighboring specialty into a new mandatory engine subsystem.

The minimum program needs one independent reproduction and review. Later influence work should add a second distinct consumer context and a post-adoption revision driven by actual feedback through `PGD-09`. Two self-authored demos do not satisfy this progression, and one successful adoption does not establish years of organizational leadership.

### Observable Behaviors

| Situation | Expected action | Inspectable evidence |
| --- | --- | --- |
| An impressive but wrong output | Find the earliest incorrect product, check units/domain/identity and refute competing explanations before tuning. | Raw-product lineage, one counterexample and the incident/regression record. |
| A proposed optimization | Name the mechanism and falsifier, freeze quality/observer/decision rules, then compare matched candidates. | Per-run native/timing evidence, uncertainty and a retained rejected/inconclusive alternative. |
| A research model or paper | Establish rights and reproducible inputs, derive the operators, prove numerical conformance, then integrate one real consumer. | Dataset/model/operator cards, fixed FP32 shader fixtures, runtime/fallback and package proof. |
| A cross-module feature | Identify one state/lifetime owner, reuse existing mechanisms and justify each hook/copy; remove replaced paths. | Consumer/deletion ledger and a defect-detecting architecture check. |
| Another engineer cannot reproduce it | Observe their first-use failure, improve the owner/runbook and retest without private instructions. | Independent transcript, review feedback and the resulting simplification. |

These are judgment targets tied to the [catalog](FeatureDeliveryCatalog.md), not an employment-level certification. Gate exits measure completed outputs; sustained influence and mentoring still require genuine external records.

Principal behavior is visible when the engineer:

- chooses a small number of high-leverage problems and finishes them end to end;
- connects research, product, content, API, compiler, driver, hardware, and user constraints;
- gives another team a stable capability rather than transferring hidden knowledge;
- leads incidents and uncertain investigations through competing hypotheses and reproducible evidence;
- raises review quality and teaches the reasoning behind decisions;
- deletes obsolete paths and resists architecture whose maintenance cost exceeds its product value;
- limits public claims to what the repository and professional record can truthfully prove.

Repository evidence cannot prove education, tenure, employment history, partner involvement, or willingness to travel. Use the appropriate professional artifact for those facts.

## Skill Ladder

| Level | Operating scope |
| --- | --- |
| Renderer contributor | Implements and debugs a pass; understands resources, barriers, shaders, and local conventions. |
| Feature owner | Owns a feature across C++, shaders, frame graph, RHI, configuration, both backends, and failure behavior. |
| Graphics systems engineer | Designs coherent rendering systems, reasons about data/concurrency/memory, and produces causal performance evidence. |
| Strategic graphics engineer | Sets renderer direction from first principles, transfers technology, and simplifies cross-team integration. |
| Principal target | Repeatedly owns high-risk path-traced/neural/driver-facing work end to end, develops other engineers, and makes the system more trustworthy and maintainable. |

## Anti-Persona

Avoid becoming the engineer who:

- adds logs, validation layers, wrappers, flags, or dashboards instead of fixing ownership;
- accumulates scenes, panels, reports, and abstractions to make incomplete work appear substantial;
- profiles unstable paths or reports speed without quality, memory, pacing, and workload context;
- copies vendor architecture or terminology without adapting it to Sparkle's real constraints;
- calls readiness, scaffolding, a provider toggle, or a mock network a product feature;
- confuses vendor-specific tuning with an untested universal rule;
- uses AI tools, citations, counters, or captures as authority without independent reasoning;
- writes more policy when clearer code, one deletion, or one executable check would communicate the rule better.

## North Star

For Sparkle, the target engineer can truthfully say:

> This feature is real. The math and data are understood. The CPU/GPU, API, compiler, driver, and quality evidence is reproducible. Ownership and fallback are explicit. Another engineer can adopt it. The repository became simpler after the change.

Use these stable outcomes in iteration records; the quote above remains the complete standard and the facets below make omissions visible:

| ID | Required outcome | Rejected substitute |
| --- | --- | --- |
| `NS-REAL` | A current consumer reaches the complete production path and receives the promised result. | Source presence, scaffolding, a selectable noun, or an author-only demo. |
| `NS-MATH-DATA` | Semantics, mathematics, numerical assumptions, data transformations, identity, and lifetime are understood and reviewable. | Unexplained constants, opaque payloads, or equations disconnected from implementation. |
| `NS-EVIDENCE` | Correctness, quality, CPU/GPU, API/compiler/driver, failure, and limitation claims are reproducible on the declared matrix. | A build, responsive process, beauty capture, average FPS, or unsupported generalization. |
| `NS-OWNERSHIP` | Mutable state, publication, synchronization, fallback, failure, recovery, and retirement each have one explicit owner. | Coordination objects, retries, logs, or compatibility paths that conceal ambiguous authority. |
| `NS-ADOPTION` | A non-author can discover, use, diagnose, reproduce, and retire the capability without private repair. | Author knowledge, undocumented environment state, or instructions corrected during acceptance. |
| `NS-SIMPLIFY` | The delivered path removes superseded code/data/configuration and leaves a smaller authority and maintenance surface. | File-count reduction, renamed duplication, speculative abstraction, or deferred cleanup. |

Every material iteration selects the applicable `NS-*` outcomes before implementation and records `advance`, `preserve`, `not applicable`, or `blocked` at handoff through the [Change Lifecycle iteration control record](../Engineering/Workflow/ChangeLifecycle.md#create-the-iteration-control-record). A feature cannot pass by excelling at one facet while failing another applicable facet.

## Purpose and Authority Boundary

The [current primary-source study](Research/PrincipalGraphicsRoles.md) checks this target against developer-technology, tools, neural-rendering and GPU-performance expectations. The [delivery catalog](FeatureDeliveryCatalog.md) gives observable outputs, and the [roadmap](Roadmap.md) orders them by proof gates without delivery dates or time estimates.

This document describes how the target engineer thinks, builds, reviews, and communicates. It does not redefine:

- the canonical [`PGE-01` through `PGE-15` capabilities and evidence](Requirements.md);
- the [Sponza/Bistro/San Miguel acceptance workloads](../Acceptance/GraphicsWorkloads.md);
- the [delivery sequence](Roadmap.md);
- the [repository implementation standards](../Engineering/README.md);
- the [first-release feature completion report contract](../Acceptance/FeatureCompletionReports.md), which applies this operating model to per-feature polish and explanation;
- subsystem architecture under `Docs/Architecture`.

Those documents own what must be achieved and how repository changes are accepted. This persona owns the professional operating model used to reach that bar.
