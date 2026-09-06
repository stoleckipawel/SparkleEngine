# RHI Engineering

Status: binding RHI integration standard

Applies to: `Engine/RHI`, public GPU contracts, D3D12 and Vulkan backends, native resources and commands, synchronization, presentation, validation, capture interop, hardware, and drivers

This standard owns RHI and backend change guardrails. The canonical [Renderer and RHI Architecture Boundary](../../Architecture/Decisions/RendererRhiBoundary.md) owns the dependency and responsibility split. [Renderer Engineering](Renderer.md) owns scene/view/frame policy, render products, frame-graph semantics, and graphics feature selection.

## Neutral Contract Ownership

- Public RHI contracts express backend-neutral resources, descriptors, commands, queues, synchronization, presentation, diagnostics, and capabilities without exposing native object types.
- Neutral pixel-format traits, capture-format/layout mapping, and graphics/compute descriptor validity are defined once by the public RHI contract.
- D3D12 and Vulkan invoke those authorities before native work; backend-local code retains only native enum/structure translation, resource/copy construction, device/API capability checks, and native failure handling.
- Do not duplicate the same neutral case list, byte layout, descriptor predicate, feature default, or validation policy in each backend.
- An unsupported neutral request fails through the RHI contract before incomplete native work is recorded. A backend must not fabricate a successful no-op or substitute a semantically different result.

## Pipeline Materialization

- RHI materializes the complete neutral pipeline descriptor supplied by Renderer; it does not infer render-target formats, shader stages, geometry layout, or feature defaults from backend state.
- Backend-private code lowers the neutral descriptor to D3D12 or Vulkan and validates every exposed state it consumes.
- Native cache identity and lifetime must preserve the full neutral identity and owning generation. Backend shortcuts must not merge semantically different pipelines.
- Native resources and pipelines remain valid until every recorded queue consumer has retired. CPU object lifetime, command-list lifetime, queue submission, and GPU completion are distinct boundaries.

## Synchronization And Diagnostics

- State transitions, UAV ordering, aliasing, queue waits, and presentation ownership are explicit and follow the public command/submission contract.
- Native validation, device-removal data, debug labels, and capture correlation preserve stable engine identities so failures can be traced back to the requesting owner.
- A validation-disabled build does not justify removing ownership assertions that can reject invalid engine use before the API call.
- Backend parity claims require equivalent semantic results and failure behavior, not merely matching method names or successful device creation.

## Hardware And Driver-Facing Work

- Record vendor, adapter/architecture, device ID when available, driver, OS, API, compiler, and feature capability for driver-sensitive conclusions.
- Prove API validity and ownership with engine assertions and native validation first.
- Reduce a suspected driver issue to the smallest resource/pipeline/command/synchronization reproducer before external attribution.
- Backend workarounds live in the owning backend-private file, have an exact predicate, cite evidence, and state removal/retest conditions.
- Renderer-wide policy branches on neutral capability, not vendor or driver identity.
- A vendor fast path retains a correct neutral fallback and does not redefine the public feature contract.
- Future hardware remains a measured hypothesis behind existing capability seams until evidence exists.
- Linux support requires native configure/build/run, Vulkan validation, capture/debug, package, and shutdown evidence.

## RHI Review Questions

- Is the request a neutral GPU contract rather than Renderer feature policy or a leaked native backend object?
- Does each neutral format, layout, descriptor, state, and capability invariant have exactly one public RHI owner?
- Are D3D12 and Vulkan limited to native translation and real capability differences instead of copied policy?
- Are resources, descriptors, pipelines, command objects, submissions, and retirement tokens valid for every consumer lifetime?
- Do transitions, queue waits, presentation, validation, device loss, and failure propagation remain explicit?
- Does every backend or hardware claim name the exact API, adapter, driver, build, workload, and evidence?
