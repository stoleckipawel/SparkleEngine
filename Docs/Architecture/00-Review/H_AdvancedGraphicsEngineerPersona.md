# H. Advanced Graphics Engineer Persona

Status: personal capability target
Date: 2026-07-04
Scope: persona and growth direction for becoming a stronger advanced graphics, rendering, and GPU systems engineer

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

## Refactor Plan Contract

Use this persona as a filter for `F_StagedDeletionFirstImprovementPlan.md`.

A staged refactor is aligned with the persona only when it does at least one of these:

- makes D3D12/Vulkan ownership more explicit
- makes a renderer feature more real with less surrounding scaffolding
- preserves a capability the engine genuinely needs, such as PTLAS or screenshot capture
- shrinks public API around behavior instead of observation
- removes diagnostic/report/log code from default workflows
- keeps shader ABI, cook, and runtime package behavior strong
- improves content/project organization without reducing multi-level support
- delays profiling until there is a stable feature path worth measuring

A staged refactor is misaligned when it:

- adds documentation instead of changing code shape
- adds a wrapper because the current ownership is uncomfortable
- adds validation or logging as a substitute for a simpler path
- adds a diagnostic UI to make an unfinished feature look complete
- removes a valuable capability instead of hardening and narrowing it
- treats external architecture as something to copy rather than something to learn from

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

## Anti-Persona

Avoid becoming:

- the engineer who adds logs instead of fixing ownership
- the engineer who adds validation systems instead of simplifying the path
- the engineer who adds wrappers because the current boundary is uncomfortable
- the engineer who adds sample scenes, panels, reports, and toggles to make a feature feel real
- the engineer who profiles too early before the feature path is stable
- the engineer who treats PTLAS, upscaling, denoising, or neural rendering as branding instead of implementation work
- the engineer who copies external architecture without adapting it to the engine's actual shape

## Personal North Star

The target is a rare mix:

- low-level enough to understand GPU execution
- high-level enough to design renderer architecture
- practical enough to ship features
- disciplined enough to remove code
- collaborative enough to make other engineers faster
- curious enough to keep learning advanced rendering, GPU compute, and neural techniques

For Sparkle, this means becoming the engineer who can say:

> This feature is real, the backend behavior is understood, the shader path is clean, the public API is small, the debug path is professional, and the repo got simpler after the change.
