# Performance Diagnostics Visualizations

Status: design visualization; not proof of current implementation or measured Sponza performance

These mockups translate [Diagnostics Product And UX Research](DiagnosticsUxResearch.md) and [Performance Diagnostics Architecture](PerformanceDiagnosticsArchitecture.md) into a concrete visual product. Values, frame identities, classifications, and profiler recommendations are illustrative.

For plain-text layouts of every fixed `Stat` group, each Performance workspace view, `ProfileGpu`, evidence export, and external-profiler handoff, see [Performance Diagnostics ASCII Tool Wireframes](PerformanceDiagnosticsAsciiWireframes.md).

## Recommended Reading Order

1. Overview shows the normal triage surface integrated into the current Editor shape.
2. CPU shows physical thread ownership, logical phases, task lanes, waits, and ready delay.
3. GPU shows the bounded captured-frame hierarchy and inclusive/exclusive marker costs.
4. Memory shows distinct RAM/GPU definitions, event history, and controlled A/B/C checkpoints.
5. System Scope shows the implementation boundary from engine producers through first-party surfaces and external profilers.

## Integrated Performance Overview

![SparkleEngine Performance Overview mockup](Images/Diagnostics/sparkle-performance-overview-mockup-v2.png)

This is the first reviewer and triage screen: fixed configuration, frame navigator, aligned CPU/GPU/memory facts, one likely-domain explanation, and one next action. It does not claim causal proof.

## CPU And Threading

![SparkleEngine CPU diagnostics mockup](Images/Diagnostics/sparkle-performance-cpu-mockup.png)

Physical Sparkle threads are the rows. Gameplay and renderer phases remain logical blocks inside the thread that actually executes them. Task lanes are separate and are never summed into a fabricated CPU total.

## GPU Captured Frame

![SparkleEngine GPU captured-frame mockup](Images/Diagnostics/sparkle-performance-gpu-capture-mockup.png)

Queue lanes, RDG marker hierarchy, and adjacent inclusive/exclusive columns answer marker-level attribution. The selected marker carries its stable path into PIX, RenderDoc, Nsight, or RGP for API, resource, shader, and hardware evidence.

## RAM, GPU Memory, And Residency

![SparkleEngine memory diagnostics mockup](Images/Diagnostics/sparkle-performance-memory-mockup.png)

Working set, private commit, tracked resources, allocator blocks, local/non-local API usage and budget, and retirement remain distinct. The A/B/C checkpoint workflow supports controlled load/unload analysis without prematurely declaring a leak.

## Complete System Scope

![SparkleEngine diagnostics system-scope visualization](Images/Diagnostics/sparkle-performance-system-scope.png)

Sparkle owns bounded collection, immutable correlation, quick orientation, stable semantic identity, and benchmark linkage. External profilers continue to own call stacks, OS scheduling, API/resource state, hardware counters, ISA, allocation maps, BVH inspection, and crash dumps.

## Scope Reminder

These images illustrate the target product and information hierarchy. They do not add implementation requirements beyond the canonical architecture, replace workload acceptance gates, or imply that every pictured field already has an authoritative producer.
