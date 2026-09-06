# L. Change Integration Standard

Status: binding integration contract and stable standards entry point

Applies to: owned code in `Engine`, `Tools`, `Projects`, build files, shaders, tests, and directly related documentation

Last restructured: 2026-09-06

## Responsibility

This document owns the invariants that make a change one coherent SparkleEngine integration. It routes implementation work to the [Engineering task map](../README.md#choose-by-task) and [Change Lifecycle](ChangeLifecycle.md); it does not repeat their workflow, detailed language/domain rules, architecture decisions, capability requirements, or workload gates.

Use the owning sources directly:

- [Whole Repository Architecture Map](../../Architecture/WholeRepositoryMap.md) for accepted and target system-design routes;
- [Engineering task map](../README.md#choose-by-task) for applicable implementation rules;
- [Principal Graphics Requirements](../../Strategy/Requirements.md) for `PGE-*` capability/evidence definitions;
- [Acceptance Workloads](../../Acceptance/GraphicsWorkloads.md) for scene and proof gates.

## How to Apply It

For every material change:

1. preserve the integration invariants below;
2. follow [Change Lifecycle](ChangeLifecycle.md);
3. apply each document selected from the [Engineering task map](../README.md#choose-by-task);
4. preserve the accepted architecture and executable configuration for the touched path;
5. classify principal requirements or workload gates only when the change affects them or makes a claim against them.

An older prompt that says “apply the integration style guide in full” means this routed document set, not a monolithic standards folder. A task may be stricter, but it may not silently weaken ownership, correctness, lifetime, deterministic behavior, supported features/backends, or required evidence.

Recommended prompt attachment:

> Apply `Docs/Engineering/Workflow/ChangeIntegration.md`, follow `Docs/Engineering/Workflow/ChangeLifecycle.md`, and select every applicable document from `Docs/Engineering/README.md#choose-by-task`. Reconcile the current ownership path, preserve supported behavior, and apply the current clean-break policy: update every owned producer and consumer, delete replaced representations and compatibility machinery, regenerate local artifacts, and add no internal versioning or legacy path. Report exact validation and limitations.

## Integration Invariants

Every integration MUST:

- deliver or preserve a real runtime, editor, renderer, RHI, cooking, launcher, capture, debugging, or validation workflow;
- fail before scheduling or publication when required work has no real producer; never manufacture apparent success with a no-op, clear, copy, dummy value, or swallowed failure;
- extend the existing owner and production path instead of creating a parallel subsystem or duplicate authority;
- keep one mutable authority and explicit lifetime/publication boundaries;
- prefer a complete vertical slice over a broad unfinished framework;
- keep public APIs smaller and more stable than their private implementations;
- introduce only complexity required by current behavior, with a named owner, consumer, lifetime, and deletion or falsification condition;
- remove the path, adapter, flag, representation, or compatibility spelling it replaces;
- classify performance impact and eliminate unnecessary work, movement, allocation, synchronization, and instrumentation before adding machinery to make them faster;
- preserve supported D3D12/Vulkan and rendering paths when the change crosses them;
- distinguish product, preview, research, unsupported, and deleted states honestly;
- improve the complete touched ownership path, including the directly exposed debt needed for one coherent result, without expanding into unrelated cleanup.

The desired change is usually additive in capability and reductive in structure. Speculative frameworks, scene-specific architecture, duplicate schedulers/graphs/caches, and names whose claims exceed their evidence do not satisfy this contract.

## Required Work and Alternate Implementations

A required product or operation MUST be produced by a real implementation of its semantic contract. Missing capability, input, state, or implementation fails at the earliest owner that can make the complete decision, before partial scheduling, mutation, or publication whenever possible. The failure must name the missing requirement and must not be converted into plausible output.

Do not add a pass, callback, adapter, default object, or error path whose only purpose is to clear, copy, fabricate, or retain a value so downstream code behaves as though required work ran. Null checks, invalid handles, `HasBeenProduced`-style state, assertions, and validation are detectors for a broken contract; they are not reasons to manufacture production.

An alternate implementation is valid only when it genuinely implements the same product contract, is selected by the policy owner before work is scheduled, has inspectable requested/active state when selection is user-visible, and has proportional correctness evidence. Unsupported or deferred work is not an alternate implementation. Keep the current real path mandatory or block the vertical slice until the replacement can land coherently; do not bridge the gap with a silent fallback or placeholder.

## Current Clean-Break Policy

SparkleEngine currently has no active users, shipped compatibility contract, supported persisted user data, public SDK/plugin ABI, or network protocol. Until an explicit product decision updates this standard, every owned change MUST assume that source, configuration, APIs, and owned representations can change in place, while serialized/cooked/generated data, caches, and other local artifacts can be discarded and regenerated from source.

For an owned contract change:

- change the authoritative representation in place and update every producer, consumer, build entry, fixture, configuration, and document in the same coherent change;
- delete the replaced representation and regenerate disposable local artifacts from source;
- fail, clear, or regenerate when old local output is encountered instead of interpreting, upgrading, or preserving it;
- remove pre-existing compatibility machinery in the touched ownership path rather than extending it.

Do not add an internal content, schema, ABI, protocol, or contract version to an owned Sparkle representation. Invalidate disposable output by deleting and regenerating it, not by encoding revision dispatch. Do not add migration/upgrade/downgrade readers or writers, compatibility adapters or shims, deprecated aliases, old/new dispatch, dual read/write paths, legacy feature flags, fallbacks to the replaced representation, or a delayed cleanup gate. A boundary adapter remains valid only when it converts a currently supported external API, tool, or source format into Sparkle's one current representation; it MUST NOT preserve an older Sparkle representation.

External API/tool/file-format versions required to consume the current external contract, build and evidence provenance, and sequence/generation counters used for stale-handle or lifetime rejection are not compatibility versions. They must not select or decode an obsolete Sparkle-owned representation.

This policy controls conflicts with older documents or existing code that still describe internal versioning or compatibility migration. Such code is cleanup debt, not precedent: remove it when its ownership path is changed. A request to support real persisted user data or an installed external consumer is a product-policy change and must update this section before compatibility code is introduced.

## Subject Authority

Authority follows the [knowledge-area map](../../README.md#knowledge-areas) and the [Engineering task map](../README.md#choose-by-task). Current code and executable configuration prove what exists; plans, snapshots, research, summaries, and historical prompts do not override their named owners.

Resolve ambiguity in the document that owns the subject, then update dependent links in the same change. Existing code is precedent only where it satisfies current authority.

## Completion Test

A reviewer must be able to identify:

- the mutable and lifetime owner;
- the authoritative definition of each changed capability/invariant and why every material use belongs at its abstraction and module level;
- the input, output, publication, and failure boundaries;
- the reason each changed file exists;
- the essential complexity introduced and the obsolete complexity removed;
- the old path that was removed;
- the supported behavior and backends that remain;
- the performance classification, affected budgets, and proportional evidence;
- the exact evidence and limitations.

If those answers are difficult to find, the integration is not finished.
