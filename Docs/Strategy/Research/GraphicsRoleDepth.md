# Graphics Role Depth And Evidence

**Status:** employer-source study and local portfolio interpretation; not NVIDIA's internal career ladder or hiring rubric

**Inspected:** 2026-10-10. Indexed official employer descriptions were available for the roles below; some direct Workday pages return only a JavaScript shell. A retrieved description is not confirmation that applications remain open. The original supplied material stays in [Role Sources](../RoleSources.md).

**Preservation contract:** this page retains the substantive expectations in text. External job URLs identify provenance; readers do not need a live vacancy page to understand or use this study. The records below paraphrase the retrieved descriptions rather than reproduce whole postings. They preserve the observed responsibilities and technical focus, not uninspected details or the employer's current hiring policy. [Requirements](../Requirements.md) remains the target authority even if every recruitment link disappears.

## Role Families And Actual Expectations

| Role/source | Distinct responsibility observed | Sparkle evidence and scope boundary |
| --- | --- | --- |
| [Principal Developer Technology Engineer, JR2013547](https://nvidia.wd5.myworkdayjobs.com/nvidiaexternalcareersite/job/Principal-Developer-Technology-Engineer_JR2013547) | Advanced path tracing/neural graphics, game integration, whole-system optimization, collaboration with GPU/driver teams and communication through demos/publications. | Primary target: `PGD-02..08`. Join research derivation to a functioning product and a partner/adopter outcome. Do not substitute a graphics toggle for end-to-end ownership. |
| [Principal Graphics Developer Tools Engineer, JR2019836](https://nvidia.wd5.myworkdayjobs.com/NVIDIAExternalCareerSite/job/US-CA-Santa-Clara/Principal-Graphics-Developer-Tools-Engineer_JR2019836) | Set technical direction for ambiguous initiatives; prototype and ship; work across organizations; understand developer pain; mentor and influence. | `PGD-04/05/08/09`: capture incident, one useful analyzer, reviewed decision and external feedback. Building a tool is necessary evidence for a tools claim, but does not establish organizational influence by itself. |
| [Senior Graphics Driver Engineer, JR2025486](https://nvidia.wd5.myworkdayjobs.com/en-US/NVIDIAExternalCareerSite/job/US-CA-Santa-Clara/Senior-Graphics-Driver-Engineer_JR2025486) | Analyze representative graphics applications, isolate bottlenecks through experiments, detect quality/performance regressions and coordinate driver/architecture fixes. | `PGD-03/04`: causal paired-API studies and one difficult incident. Sparkle can prove workload analysis and a reduced bug; it cannot claim proprietary driver implementation experience. |
| [Senior Graphics System Software Engineer, JR2019241](https://nvidia.wd5.myworkdayjobs.com/NVIDIAExternalCareerSite/job/US-CA-Santa-Clara/Senior-Graphics-System-Software-Engineer_JR2019241) | Deliver graphics/AI features across explicit APIs, platforms and GPU generations, with studio and internal engineering collaboration. | `PGD-01/07`: capability, ABI, integration, failure and package matrix. Windows D3D12/Vulkan is the selected native route; Linux or another architecture needs separate proof. |
| [Senior Graphics Shader Compiler Engineer, JR2024686](https://nvidia.wd5.myworkdayjobs.com/en-US/NVIDIAExternalCareerSite/job/Senior-Graphics-Shader-Compiler-Engineer_JR2024686) | Implement compiler components/optimizations, resolve cross-team problems, contribute to LLVM/DXC and SPIR-V standards/code generation. | `PGE-09/10`, `PGD-04/07`: source/bytecode/ISA lineage and a reduced compiler-facing case. A shader cooker is not a compiler backend. An actual compiler contribution remains a conditional specialization. |
| [Neural Graphics Engineer, JR2013428](https://nvidia.wd5.myworkdayjobs.com/en-US/NVIDIAExternalCareerSite/job/Neural-Graphics-Engineer_JR2013428) | Connect trained models, rendering, shaders/runtime and practical C++/Python/ML tooling. | `PGD-06/07`: owned data/model, fixed operator lowering, conformance and measured inference. Vendor reconstruction integration does not prove training or kernel ownership. |
| [Senior Developer Technology Engineer, Simulation Performance, JR2025157](https://nvidia.wd5.myworkdayjobs.com/en-US/NVIDIAExternalCareerSite/job/Senior-Developer-Technology-Engineer---Simulation-Performance_JR2025157) | Deliver durable performance improvements, numerical/parallel engineering and stewardship of consequential public software. | Secondary cross-check for maintenance and feedback in `PGD-08/09`. It does not add simulation/physics features to this rendering program. |
| [Senior Software Engineer, Graphics Performance, JR2012523](https://nvidia.wd5.myworkdayjobs.com/en-US/NVIDIAExternalCareerSite/job/Senior-Software-Engineer--Graphics-Performance_JR2012523) | Maintain and optimize graphics-driver behavior on Linux, including Vulkan/OpenGL and compiler interactions. | External Linux driver expectation; excluded locally. Windows Vulkan cannot establish Linux driver or platform expertise. |

These are neighboring but distinct careers. The primary persona is **rendering/developer technology with GPU-system depth**, strengthened by neural productization and developer tools. It is not a promise to become simultaneously a research scientist, compiler architect, driver author and tools principal. Adjacent roles define useful depth checks and honest gaps.

## Preserved Role Expectation Records

The requisition IDs join these records to the source table above. All eight descriptions were inspected on 2026-10-10 through official employer text or its indexed representation; direct-page retrieval limits remain explicit. These are research snapshots, not implementation instructions, vacancy status or evidence of personal qualification.

### JR2013547 - Principal Developer Technology

**Observed expectations:** turn advanced rendering and neural ideas into useful game-engine results; understand path tracing and whole-system CPU/GPU cost; work with game developers and internal GPU architecture/driver teams; investigate and optimize demanding applications. Communicate the resulting techniques through practical demonstrations and technical publications. Rendering mathematics, C++ and explicit graphics API reasoning support that integration work.

**Local interpretation:** deliver one complete research-to-product case with a measured baseline, integration/failure constraints and independent adopter evidence. The role involves technology transfer and cross-team judgment, not just adding an algorithm. The existing `PGD-02..08` routes supply the bounded portfolio outputs.

### JR2019836 - Principal Graphics Developer Tools

**Observed expectations:** establish technical direction for ambiguous developer-tool initiatives; understand actual developer problems; investigate ideas through hands-on prototypes and carry useful work into production. Coordinate across engineering groups, influence decisions and mentor engineers. Graphics API and C++ depth must support useful, shipped tools rather than isolated demonstrations.

**Local interpretation:** solve a difficult capture/debugging problem, deliver a narrow analysis consumer and retain the feedback that changed a decision or interface. `PGD-04/05/08/09` distinguish tool usefulness, independent adoption and later influence; one self-authored utility cannot establish all three.

### JR2025486 - Senior Graphics Driver Engineering

**Observed expectations:** study representative real-world graphics applications; design experiments that isolate limiting behavior; use traces, hardware observations and analysis tooling to investigate performance and visual regressions. Work with driver and architecture engineers to resolve problems. Application representativeness and defensible causes matter alongside low-level expertise.

**Local interpretation:** `PGD-03/04` require paired-API workload analysis and a difficult incident with exact inputs, competing hypotheses, native evidence and a scoped fix. A reduced application-side reproducer is valuable; it does not claim access to or authorship of a proprietary driver.

### JR2019241 - Senior Graphics System Software

**Observed expectations:** deliver graphics and AI functionality across explicit APIs, operating systems and GPU generations; integrate advanced rendering with game studios and internal engineering teams. Robust system software, platform behavior and collaboration are part of the feature, not work deferred until after a demo.

**Local interpretation:** `PGD-01/07` preserve capability selection, host/shader ABI, synchronization/lifetime, failure and package behavior on the actual supported matrix. Demonstrate the native Windows D3D12/Vulkan route first; further platforms and GPU architectures remain separate evidence gaps.

### JR2024686 - Senior Graphics Shader Compiler

**Observed expectations:** design and implement substantial compiler components; analyze generated-code performance and develop optimizations. Resolve problems jointly with compiler, graphics-driver, GPU-architecture and application teams. The description includes LLVM/DXC contributions, GPU code generation and participation in SPIR-V extensions and standards. Strong C/C++ and parallel-programming knowledge underpins the compiler work.

**Local interpretation:** `PGE-09/10` need source-to-bytecode/native correlation and a reduced compiler-facing investigation. An actual compiler change or standards contribution is a conditional specialist output. Shader cooking, reflection and packaging demonstrate application integration, not compiler-backend implementation.

### JR2013428 - Neural Graphics

**Observed expectations:** connect neural models with production rendering; work across shaders, runtime integration and practical C++/Python/ML tooling. The studied description links model ideas to usable engine behavior rather than treating training and deployment as disconnected activities.

**Local interpretation:** `PGD-06/07` require owned data/model provenance, explicit operators and numerical conformance, actual shader inference, fallback and held-out quality/cost. This snapshot is a specialist cross-check, not a principal-level equivalence. Vendor reconstruction integration alone does not demonstrate those ownership obligations.

### JR2025157 - Senior Developer Technology, Simulation Performance

**Observed expectations:** improve consequential software through numerical and parallel-programming expertise; make performance work durable and useful to real users. Public-software stewardship and production integration matter alongside an optimization's technical novelty.

**Local interpretation:** use maintenance, documented user feedback and follow-up regression evidence in `PGD-08/09`. This is an adjacent professional-practice example; it introduces no simulation/physics requirement into the rendering program.

### JR2012523 - Senior Graphics Performance

**Observed expectations:** maintain and optimize Linux graphics-driver behavior, including Vulkan/OpenGL features and shader/compiler interactions. Performance engineering includes the platform and long-term correctness of the product, not just application-level graphics familiarity.

**Local interpretation:** Linux delivery is excluded from the selected program. Windows Vulkan work can support API reasoning, but cannot establish Linux execution or driver implementation experience.

## Senior, Expert And Principal

No inspected source establishes `Expert` as a universal NVIDIA rank. Here it means demonstrated depth in a chosen problem. Job titles vary by organization; this table is a strategic synthesis, not an HR mapping.

| Dimension | Senior ownership evidence | Expert depth evidence | Principal-scope evidence |
| --- | --- | --- | --- |
| Problem | Deliver a substantial feature or fix under known product constraints. | Resolve cases where naive implementations fail; derive and test the limiting assumptions. | Choose the consequential problem under ambiguity and align users, research, API, compiler/hardware and product constraints. |
| Implementation | Production code, lifetime/failure rules and regression proof. | Numerical/low-level explanation with independent controls and rejected alternatives. | A durable capability with narrow boundaries, explicit maintenance cost and a justified deletion/adoption strategy. |
| Performance | Correctly measure an optimization on a named workload. | Establish causality through source, native execution, counters/traces and quality-equivalent experiments. | Select investments from whole-system and user outcomes, including choosing not to ship an optimization. |
| Transfer | Another developer can use the documented result. | Explain difficult internals and diagnose an adopter's failure. | Repeated external adoption/review, decision influence, stewardship and teaching with genuine feedback. |
| Scope honesty | Identify tested APIs, hardware and failures. | Separate algorithm, compiler, driver and architecture effects. | Make reviewable support and investment decisions without inflating breadth or concealing missing evidence. |

The [public engineer study](GraphicsEngineerProfiles.md) supplies examples of these behaviors. Titles, degrees, years, repository stars and paper counts are not repository acceptance thresholds. No feature count can turn self-authored work into evidence of managing organizational risk or developing other engineers.

## Portfolio Review Questions

Use these questions alongside the canonical [E0–E4 evidence scale](../Requirements.md#evidence-scale), rather than inventing a second score:

1. Can the reviewer find the exact personal contribution, baseline, native result and limitations in each of the three headline cases?
2. Does the rendering case derive sampling/transport assumptions and show an independent numerical or image oracle?
3. Does the systems case identify the limiting work and retain at least one failed or inconclusive alternative?
4. Can source, compiler options, cooked bytecode, bindings and executed shader be joined for the difficult incident?
5. Does the neural case contain an owned model, fixed operator math, FP32 shader conformance and a measured product comparison?
6. Can a non-author reproduce a result, and did their feedback change implementation or guidance?
7. Is there an explicit decision record explaining rejected scope, interface/copy/hook costs and deleted authority?
8. Beyond the minimum advanced program, is there evidence from a second independent consumer context and one post-adoption repair or improvement prompted by real feedback?

Questions 1–7 refine the selected `PGD-*` outputs. Question 8 is the conditional `PGD-09` influence progression; it does not silently double the minimum adopter gate or certify sustained professional leadership. Professional and confidential evidence can supplement the public project with appropriate permission.

## What Changes In Delivery

The required work remains three deep cases, three causal studies, one incident, one analysis consumer and one owned neural product. The stronger requirement is **traceability and transfer**: every selected example now has an exact source, purpose, existing feature owner, measurable local question and non-adoption boundary in [Rendering Reference Examples](RenderingReferenceExamples.md).

For a compiler-specialist claim, new GPU-driven pipeline or research publication claim, create a bounded separately reviewed extension only after the current gates permit it. Merely adding LLVM, OptiX, CUDA, mesh shaders or vendor extensions would not establish those capabilities.
