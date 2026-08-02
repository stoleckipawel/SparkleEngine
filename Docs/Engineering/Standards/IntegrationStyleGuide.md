# L. SparkleEngine Integration Style Guide

Status: binding integration contract and stable standards entry point

Applies to: owned code in `Engine`, `Tools`, `Projects`, build files, shaders, tests, and directly related documentation

Last restructured: 2026-08-02

## Responsibility

This document owns the invariants that make a change one coherent SparkleEngine integration. It routes implementation work to the [standards index](README.md) and [Change Process](ChangeProcess.md); it does not repeat their workflow, detailed language/domain rules, architecture decisions, capability requirements, or workload gates.

Use the owning sources directly:

- [Architecture](../../Architecture/README.md) for accepted and target system design;
- [Engineering Standards](README.md#standards-map) for implementation rules;
- [Principal Graphics Requirements](../../Strategy/Requirements.md) for `PGE-*` capability/evidence definitions;
- [Acceptance Workloads](../BistroAndSanMiguelWorkloads.md) for scene and proof gates.

## How to Apply It

For every material change:

1. preserve the integration invariants below;
2. follow [Change Process](ChangeProcess.md);
3. apply each standard selected from the [standards map](README.md#standards-map);
4. preserve the accepted architecture and executable configuration for the touched path;
5. classify principal requirements or workload gates only when the change affects them or makes a claim against them.

An older prompt that says “apply `IntegrationStyleGuide.md` in full” means this routed document set, not the obsolete monolithic file. A task may be stricter, but it may not silently weaken ownership, correctness, lifetime, deterministic behavior, supported features/backends, or required evidence.

Recommended prompt attachment:

> Apply `Docs/Engineering/Standards/IntegrationStyleGuide.md`, follow `ChangeProcess.md`, and select every applicable subject standard from the standards map. Reconcile the current ownership path, preserve supported behavior, delete replaced paths, and report exact validation and limitations.

## Integration Invariants

Every integration MUST:

- deliver or preserve a real runtime, editor, renderer, RHI, cooking, launcher, capture, debugging, or validation workflow;
- extend the existing owner and production path instead of creating a parallel subsystem or duplicate authority;
- keep one mutable authority and explicit lifetime/publication boundaries;
- prefer a complete vertical slice over a broad unfinished framework;
- keep public APIs smaller and more stable than their private implementations;
- remove the path, adapter, flag, representation, or compatibility spelling it replaces;
- preserve supported D3D12/Vulkan and rendering paths when the change crosses them;
- distinguish product, preview, research, unsupported, and deleted states honestly;
- improve the complete touched ownership path without expanding into unrelated cleanup.

The desired change is usually additive in capability and reductive in structure. Speculative frameworks, scene-specific architecture, duplicate schedulers/graphs/caches, and names whose claims exceed their evidence do not satisfy this contract.

## Subject Authority

Authority follows the subject map in the [documentation root](../../README.md) and [standards authority model](README.md#authority-model). Current code and executable configuration prove what exists; plans, snapshots, research, summaries, and historical prompts do not override their named owners.

Resolve ambiguity in the document that owns the subject, then update dependent links in the same change. Existing code is precedent only where it satisfies current authority.

## Completion Test

A reviewer must be able to identify:

- the mutable and lifetime owner;
- the input, output, publication, and failure boundaries;
- the reason each changed file exists;
- the old path that was removed;
- the supported behavior and backends that remain;
- the exact evidence and limitations.

If those answers are difficult to find, the integration is not finished.
