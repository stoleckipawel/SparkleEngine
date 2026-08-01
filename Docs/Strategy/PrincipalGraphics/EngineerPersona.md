# H. Advanced Graphics Engineer Persona

Status: personal capability target
Date: 2026-07-26
Scope: persona and growth direction for principal-level advanced graphics, developer technology, rendering, GPU systems, and neural graphics engineering

## Persona Statement

Become a graphics systems engineer who can take a rendering or GPU-compute idea from research-shaped ambiguity to a lean, working, product-quality implementation.

This persona is not a generalist who only knows engine vocabulary. It is someone who can reason from first principles across C++, shaders, explicit graphics APIs, GPU architecture, renderer feature design, debugging tools, and cross-team integration.

The center of gravity is:

- modern C++ and shader engineering
- D3D12 and Vulkan fluency
- real-time rendering, rasterization, ray tracing, GI, and path tracing
- GPU architecture and memory-model awareness
- graphics debugging with PIX, RenderDoc, Nsight, and native API layers
- neural rendering and GPU inference readiness without bloating the engine
- practical productization: small APIs, clean ownership, useful tools, and code that other engineers can maintain

The concrete proving ground is [the canonical workload ladder](../../Engineering/Validation/BistroAndSanMiguelWorkloads.md): Sponza for fast regression, Bistro as the primary flagship, and San Miguel as a supported secondary scene. The persona must be able to make both Tier 1 scenes correct, beautiful, performant, measurable, and easy to present without hardcoding the renderer around either asset.

## Principal Graphics Engineering Additive Target

The supplied principal graphics engineering role set raises this persona beyond strong internal renderer ownership. The target must also demonstrate the ability to help demanding game teams adopt advanced rendering and AI technology, tune it on real hardware, work at the architecture/driver boundary, and communicate the result at principal level.

This is additive. Nothing in this section weakens the existing requirements for compact architecture, D3D12/Vulkan parity, deletion-first integration, deterministic behavior, low coupling, professional debugging, or measured performance.

### Evidence Boundary

A repository cannot prove a degree, a number of years in industry, willingness to travel, or employment history. It can prove equivalent technical depth, independent judgment, collaboration readiness, and communication quality through code, captures, reviews, incident analyses, reproducible demos, and technical writing. Public portfolio wording must never claim credentials or professional experience that are not true.

### Persona Interpretation Of The Canonical Matrix

The identifiers are defined authoritatively in [A. Principal Graphics Engineering Requirements](Requirements.md). The table below is the concise persona interpretation; it does not narrow A.

| ID | Capability expectation | Additive Sparkle interpretation | Required evidence |
|---|---|---|---|
| `PGE-01` | Collaborate with leading game developers to adopt advanced GPU and AI technology | Design narrow integration contracts, diagnose partner-shaped workloads, preserve product constraints, and leave handoff-quality code and guidance | One integration case study with requirements, constraints, before-state, design review, adoption steps, failure handling, and measured result |
| `PGE-02` | Advanced real-time rendering including path tracing | Own a path-traced feature through renderer, shaders, frame graph, RHI, temporal state, tools, and both supported APIs | Bistro flagship and San Miguel cross-scene correctness images, D3D12/Vulkan validation, GPU captures, quality/performance comparison, and documented limitations |
| `PGE-03` | Neural graphics and broader AI adoption in games | Deliver at least one real neural-graphics vertical slice that replaces or materially improves an existing denoising, reconstruction, sampling, texture/material, animation, or rendering path | Real model/operator path, classical baseline/fallback, deterministic assets, runtime integration, quality metrics, latency, memory, and backend/capability matrix |
| `PGE-04` | Translate models into optimized GPU kernels | Read model graphs, understand operator mathematics, choose layout/precision/fusion deliberately, implement fixed-topology GPU shaders or kernels, profile end-to-end inference, and tune the actual bottleneck rather than wrapper overhead | Operator derivation, tensor shapes/layouts, numerical conformance, precision/fusion study, per-stage capture, ablation, quality/performance frontier, and retained negative results |
| `PGE-05` | Optimize GPU and system performance for fluid gameplay and complex AI behavior | Treat frame time, pacing, input-to-present latency, CPU/GPU overlap, memory, and background AI contention as one product budget | Correlated Bistro/San Miguel CPU/GPU timelines, p50/p95/p99, memory high-water, queue behavior, frame pacing, and bounded degradation under representative load |
| `PGE-06` | Analyze graphics workloads and solve application/API/driver problems | Analyze D3D12/Vulkan workloads with PIX, RenderDoc, native validation, captures, and counters; separate application, API, driver, compiler, and hardware causes; produce minimal reproducers | Workload comparison, hardware/driver/config matrix, capability gates, backend-native validation, disassembly/counters where useful, root-cause analysis, and a reduced reproducer |
| `PGE-07` | Expert C++ and practical Python software engineering | Build ownership-correct, low-overhead, reviewable C++ systems; use Python for reproducible analysis/training/automation; debug races, lifetime faults, API misuse, memory pressure, and performance pathologies | Code review evidence, useful Python tool, clean build, tests, CI, sanitizer/native validation where available, incident reports, and before/after implementation-shape reconciliation |
| `PGE-08` | Strong mathematics including linear algebra and calculus for problem solving and performance modeling | Derive coordinate transforms, sampling estimators, reconstruction filters, gradients, error metrics, numerical stability, and cost models rather than copying formulas blindly | A math note tied to executable tests, reference values, numerical-error bounds, and a prediction checked against measurements |
| `PGE-09` | Excellent real-time graphics, GPU, shader-language, DirectX, and Vulkan knowledge | Maintain deep HLSL/Slang, DXIL/SPIR-V, D3D12/Vulkan, raster, compute, RT, synchronization, memory, and shader ABI competence | Paired backend implementation/captures, shader inspection, resource-state reasoning, and feature-specific validation |
| `PGE-10` | In-depth CPU/GPU architecture and concurrency with hands-on low-level optimization | Explain CPU caches/SIMD/topology/threading and GPU cache, bandwidth, waves, divergence, occupancy, registers, scheduling, atomics, memory model, driver submission, and useful ISA evidence | Causal experiments using counters/disassembly/traces, serial controls, concurrency stress evidence, rejected alternatives, and architecture-specific conclusions |
| `PGE-11` | Solid AI fundamentals and effective use of AI tools to program and design new solutions | Understand training/inference, optimization, generalization, datasets, loss/metrics, quantization, deployment, and tool-assisted engineering while independently verifying generated work | Model card/provenance, train/validation separation, reproducible export, inference contract, verification of AI-assisted code/design, and no unreviewed generated code |
| `PGE-12` | Machine-learning algorithms plus inference and training workload optimization | Treat training and inference as different workloads; keep training/offline experimentation isolated from the runtime while proving informed optimization of both | Profiled training or fine-tuning study, runtime inference profile, batch/precision/memory/concurrency sweeps, deterministic artifact export, and deployment tradeoff |
| `PGE-13` | Productize research, build useful tools, and communicate through demos, papers, and talks | Turn one bounded research or architecture hypothesis into a product-relevant experiment/tool, keep it only if evidence wins, document best practices, and make the result teachable and reproducible | Hypothesis and rejected alternatives, prototype-to-product/deletion decision, tool or integration surface, polished Bistro demo, San Miguel breadth proof, technical note, talk/deck, review record, and priority/deletion ledger |
| `PGE-14` | Platform, debugger, build, and ecosystem breadth | Deepen Windows/D3D12 and Vulkan driver-facing work now; demonstrate native Linux/Vulkan before claiming it; use build, source-control, debugger, profiler, English communication, and travel workflows professionally | Correct API/driver ownership, clean-clone build, validation/capture/crash workflow, platform audit, and Linux evidence only after native build/run/validation on Linux |
| `PGE-15` | Principal maturity, education-equivalent depth, and sustained influence | Demonstrate repeated end-to-end ownership, adversarial review, honest tradeoffs, cross-domain debugging, technical direction, mentoring-quality explanation, and sustained simplification | Multiple completed vertical slices, shipped outcomes, partner/peer validation, teaching/review evidence, and incident reports that can be independently reproduced and defended |

### Binding Interpretation

- "Neural rendering readiness" is no longer the final bar. It remains the architectural prerequisite for `PGE-03` and `PGE-04`.
- At least one neural-graphics feature must eventually execute a real model or neural operator path. A provider toggle, empty tensor abstraction, capability enum, mock model, shader demo disconnected from the renderer, or architecture-only note does not satisfy the requirement.
- The neural feature must replace or improve a real current path. It must not add a second renderer, general ML runtime, generic tensor framework, or permanent research scheduler.
- Training may remain an isolated offline workflow, but model provenance, dataset boundaries, export/cook reproducibility, precision/layout decisions, and runtime inference ownership must be explicit.
- AI-assisted programming is allowed only with normal review. Generated code, tests, math, shader logic, citations, and performance claims are treated as untrusted until inspected and independently validated.
- Partner collaboration is represented by integration quality: a narrow adoption surface, reproducible issue capture, precise constraints, graceful fallback, useful documentation, and code another engineer can own.
- Future-hardware thinking means capability-driven design and measured hypotheses. It does not permit speculative public APIs, vendor branding, or claims about hardware that was not tested.
- Whitepaper/conference quality means a technically rigorous explanation of a completed result. Documentation never substitutes for implementation or evidence.

### Prompt-Level Persona Gate

Every future implementation prompt must:

1. list the applicable `PGE-*` requirements;
2. state how the change advances or preserves them;
3. name the concrete artifact or measurement that will demonstrate the claim;
4. avoid adding role-shaped scaffolding without a current product use;
5. report the requirement status as **advanced**, **preserved**, **not applicable**, or **blocked**, with a reason;
6. identify any new gap exposed by the work without claiming it is solved.

## Core Identity

The target identity is:

> I build advanced real-time graphics systems that are technically deep, measurable when needed, and clean enough for other engineers to extend.

This implies a specific way of behaving:

- Think in frame captures, command streams, resources, descriptors, barriers, queues, pipelines, shaders, and memory pressure.
- Prefer a working, minimal feature path over a broad framework.
- Preserve important capabilities, but delete scaffolding around them.
- Translate research ideas into small renderer-owned vertical slices.
- Communicate through clean code, crisp names, small public surfaces, and short decision text when needed.
- Use profiling late and deliberately, after the feature surface is worth measuring.

## Technical Pillars

### 1. Explicit Graphics API Ownership

Be able to explain and modify:

- D3D12 and Vulkan device/queue/command submission models
- resource states, barriers, synchronization, and queue ownership
- descriptor heaps, descriptor sets, root signatures, pipeline layouts, and binding models
- render pass, compute pass, ray tracing pipeline, and frame-graph scheduling
- GPU memory allocation, upload, readback, residency, and transient resources
- backend capability queries and feature selection without leaking backend-specific details into renderer code

Sparkle evidence:

- RHI remains explicit and backend-native details stay private or provider-bridged.
- Frame graph owns scheduling and barriers above the RHI.
- D3D12 and Vulkan paths remain understandable without adding wrapper layers.

### 2. Renderer Feature Depth

Own features deeply enough to debug them without hand-waving:

- deferred shading and material pipelines
- physically based BRDFs and light transport basics
- shadowing, GI, direct lighting reservoirs, and path/reference modes
- ray tracing BLAS/TLAS lifecycle
- classic TLAS and PTLAS as equal product RT capabilities
- post-processing, denoising, temporal accumulation, upscaling, and ray reconstruction
- screenshot/BMP capture as a preserved, low-cost editor/tool capability

Sparkle evidence:

- Classic TLAS and PTLAS both work where supported.
- PTLAS is minimized toward the original reference flow: capability check, compact descriptor input, backend build/update, resource lifetime, and trace use.
- GI/path tracing work improves feature correctness instead of adding debug surfaces.

### 3. Shader And Kernel Craft

Be strong in code that actually runs on the GPU:

- HLSL SM6 as the primary shader language
- GLSL and Slang familiarity for cross-target thinking
- shader library design for BRDF, geometry, lighting, ray tracing, material, and display code
- wave/subgroup operations, memory access patterns, LDS/shared memory, occupancy, divergence, and bandwidth
- shader package cooking, reflection, parameter layout, and runtime ABI discipline
- compute-kernel style thinking for denoisers, upscalers, neural operators, and image processing

Sparkle evidence:

- Shader compiler/cook/runtime ABI remains a centerpiece.
- Debug bundles and stats artifacts are opt-in or removed from default workflows.
- New shader code deletes duplication or directly serves renderer features.

### 4. GPU Architecture Thinking

Develop a mental model below the API:

- cache hierarchy, memory coalescing, bandwidth pressure, and latency hiding
- wavefront/warp execution, divergence, occupancy, and register pressure
- async compute tradeoffs and queue overlap
- acceleration structure build/update cost
- descriptor pressure, pipeline count, and shader permutation cost
- CPU-to-GPU submission overhead and frame setup cost

Sparkle evidence:

- Performance work happens late, after feature cleanup.
- Existing GPU markers, timestamps, object names, and debugger support remain.
- New measurement code is avoided unless it replaces existing scattered diagnostics.

### 5. Neural Rendering Readiness

The target is not to bolt a machine-learning framework into the engine. The target is to understand enough to translate neural ideas into efficient GPU work:

- tensors, shapes, broadcasting, layouts, precision, and memory footprint
- operator basics: convolution, matmul, attention-like access patterns, activation, normalization, resampling
- automatic differentiation concepts well enough to talk with researchers
- PyTorch/ONNX model shape as an input format, not as an engine dependency
- HLSL/Slang/HIP/CUDA-style implementation thinking for kernels and inference-like passes
- denoising, upscaling, ray reconstruction, and neural texture/sampling ideas as renderer features

Sparkle evidence:

- Slang/HLSL ABI stays flexible.
- Neural work begins as a renderer feature slice only when it can replace or improve an existing path.
- The completed portfolio includes at least one real neural graphics feature with a deterministic model/export contract, efficient runtime inference, a classical fallback, and measured quality/performance.
- Training, fine-tuning, or model-conversion experiments remain isolated from the runtime package and publish only validated immutable artifacts.
- No heavy runtime ML framework is added before the engine has a concrete feature need.

### 6. Debugging And Tool Fluency

Be comfortable solving hard graphics bugs with professional tools:

- PIX, RenderDoc, Nsight
- backend debug layers
- GPU markers, object names, and timestamps
- shader debugging and disassembly when needed
- frame capture triage: resource lifetime, barriers, descriptors, pipelines, pass order, and shader inputs
- CPU debugging around frame setup, asset loading, shader package load, and command submission

Sparkle evidence:

- Debugger/profiler hooks are preserved.
- Screenshot/BMP capture is preserved and hardened.
- Bespoke reports, logs, and validation panels are deleted unless they are product-owned.

## Operating Style

### How This Engineer Thinks

- Start from the frame: what resources exist, which pass writes them, which pass reads them, and which queue executes them.
- Start from the shader: what data layout does it expect, what memory does it touch, and what access pattern does it create.
- Start from the API: what state, binding, synchronization, and lifetime rule can break this feature.
- Start from the user: how does an engineer select the feature, debug it, and know what backend supports it.
- Start from deletion: what old code can this feature remove.

### How This Engineer Builds

- Small vertical slices over broad infrastructure.
- Direct integration over new wrappers.
- Renderer-owned policies over scattered feature flags.
- Minimal public API over convenience/status surfaces.
- Product behavior over diagnostic artifacts.
- Feature hardening before profiling campaigns.

### How This Engineer Communicates

- Use precise technical language.
- Explain tradeoffs without inflated claims.
- State what is product-owned, experimental, skipped, or deleted.
- Keep existing docs accurate, but do not create new docs as a substitute for code cleanup.
- Prefer code shape, names, and ownership boundaries that make the decision obvious.
- Translate a complex feature into an adoption guide, reduced reproducer, design review, live demo, and conference-quality explanation without overstating the result.
- Communicate priorities, negative results, risks, hardware limits, and fallback policy as clearly as successes.

## Evidence To Build In Sparkle

The repo should gradually show this persona through code, not through more documents:

1. RHI and frame graph are explicit, small, and understandable.
2. D3D12 and Vulkan remain first-class.
3. Classic TLAS and PTLAS both remain usable RT features.
4. PTLAS becomes smaller and closer to the reference implementation.
5. Shader compiler/cook/runtime ABI remains strong.
6. Screenshot/BMP capture remains preserved, hardened, and low-cost.
7. Post-processing, denoising, upscaling, GI, path tracing, shaders, and passes are cleaned before measurement work.
8. The launcher and cookers become workflow tools, not diagnostic shells.
9. Multiple levels remain supported through catalogs/manifests without depot pollution.
10. Every feature addition removes or simplifies nearby code when possible.
11. At least one path-traced workload has paired API, quality, latency, and architecture evidence.
12. At least one real neural-graphics feature meets `PGE-03`, `PGE-04`, `PGE-11`, and `PGE-12`.
13. Neural training/export and runtime inference are separate, deterministic, and performance-budgeted.
14. A hardware/driver issue can be reduced, classified, validated, and communicated professionally.
15. Completed work has an adoption-quality integration note, a whitepaper-quality result, and a live technical demonstration.
16. AI-assisted contributions are reviewed and validated to the same bar as manually authored work.
17. Bistro exterior/interior satisfies the primary material, lighting, paired-API, path-tracing, neural, and whole-system performance gates in I.
18. San Miguel is a supported secondary scene with deterministic import/cook/run, reference quality, benchmark evidence, and held-out neural evaluation.
19. Sponza remains the cheap regression tier; the renderer contains no Bistro- or San-Miguel-specific shader or architecture fork.

## Refactor Plan Contract

Use this persona together with [K. Multithreaded Engine Implementation Prompt Series](../../Architecture/Multithreading/ImplementationPromptSeries.md) and [L. SparkleEngine Integration Style Guide](../../Engineering/Standards/IntegrationStyleGuide.md) as the filter for every staged integration.

A staged refactor is aligned with the persona only when it does at least one of these:

- makes D3D12/Vulkan ownership more explicit
- makes a renderer feature more real with less surrounding scaffolding
- preserves a capability the engine genuinely needs, such as PTLAS or screenshot capture
- shrinks public API around behavior instead of observation
- removes diagnostic/report/log code from default workflows
- keeps shader ABI, cook, and runtime package behavior strong
- improves content/project organization without reducing multi-level support
- delays profiling until there is a stable feature path worth measuring
- advances a concrete `PGE-*` requirement with code and evidence
- improves partner adoption, reproducibility, mathematical rigor, hardware/driver diagnosis, or technical communication
- advances a named Bistro or San Miguel acceptance gate through general renderer/content behavior

A staged refactor is misaligned when it:

- adds documentation instead of changing code shape
- adds a wrapper because the current ownership is uncomfortable
- adds validation or logging as a substitute for a simpler path
- adds a diagnostic UI to make an unfinished feature look complete
- removes a valuable capability instead of hardening and narrowing it
- treats external architecture as something to copy rather than something to learn from
- adds empty neural/AI abstractions, mock workloads, or role-keyword scaffolding without a replacement feature
- uses AI-generated output without independent code, math, source, security, and performance review

This persona should make the refactor plan stricter, not broader. If a task cannot explain which persona pillar it develops and which code or depot weight it removes, it should wait.

## Skill Ladder

### Level 1: Capable Renderer Contributor

- Can implement and debug a render pass.
- Understands shader inputs, outputs, resources, and barriers.
- Uses RenderDoc/PIX/Nsight to inspect a frame.
- Writes C++ and HLSL that match existing engine patterns.

### Level 2: Renderer Feature Owner

- Owns a feature across C++, shader code, RHI resources, frame-graph scheduling, and editor/runtime selection.
- Can debug D3D12/Vulkan backend differences.
- Can reason about memory pressure and GPU pass cost.
- Keeps public API small and removes obsolete code.

### Level 3: Advanced Graphics Systems Engineer

- Can design ray tracing, GI, denoising, upscaling, and shader pipeline work as coherent product features.
- Can translate research-shaped ideas into minimal renderer integrations.
- Can preserve D3D12/Vulkan capability without multiplying abstractions.
- Can review other engineers' graphics code for architecture, performance risk, and maintainability.

### Level 4: Strategic Graphics Engineer

- Shapes renderer direction from first principles.
- Connects GPU architecture, API behavior, shader design, and product needs.
- Helps other teams integrate advanced features without accepting unnecessary complexity.
- Builds systems that are impressive because they are smaller, sharper, and easier to trust.

### Level 5: Principal Graphics Engineering Target

- Leads adoption of path-traced and neural graphics technology across unfamiliar, partner-shaped engine constraints.
- Tunes algorithms, models, shaders, CPU work, GPU work, memory, and frame pacing as one system.
- Works effectively at the application, API, driver, and architecture boundary on current hardware while preparing capability-driven paths for future hardware.
- Can profile and explain both training/offline model preparation and low-latency runtime inference without coupling the runtime to a research stack.
- Produces reduced reproducers, best-practice guidance, technical demos, whitepaper-quality analysis, and conference-ready explanations.
- Raises other engineers' effectiveness through precise reviews, principled prioritization, and maintainable integration surfaces.

## Anti-Persona

Avoid becoming:

- the engineer who adds logs instead of fixing ownership
- the engineer who adds validation systems instead of simplifying the path
- the engineer who adds wrappers because the current boundary is uncomfortable
- the engineer who accumulates unrelated sample scenes, panels, reports, and toggles to make a feature feel real instead of completing the declared Sponza/Bistro/San Miguel ladder
- the engineer who profiles too early before the feature path is stable
- the engineer who treats PTLAS, upscaling, denoising, or neural rendering as branding instead of implementation work
- the engineer who copies external architecture without adapting it to the engine's actual shape
- the engineer who calls an empty tensor type, provider toggle, or mock shader "neural graphics"
- the engineer who reports model quality without runtime cost, or runtime speed without quality and dataset context
- the engineer who uses AI tools as an authority instead of a fallible accelerator
- the engineer who confuses vendor-specific tuning with an untested universal best practice

## Personal North Star

The target is a rare mix:

- low-level enough to understand GPU execution
- high-level enough to design renderer architecture
- practical enough to ship features
- disciplined enough to remove code
- collaborative enough to make other engineers faster
- curious enough to keep learning advanced rendering, GPU compute, and neural techniques
- rigorous enough to derive and validate the math
- practical enough to transfer technology into a partner-shaped engine
- articulate enough to defend the work in a design review, whitepaper, live demo, or conference talk

For Sparkle, this means becoming the engineer who can say:

> This feature is real, the math and model behavior are understood, the CPU/GPU and driver evidence is reproducible, the backend and fallback behavior are explicit, another engineer can adopt it, and the repo got simpler after the change.
