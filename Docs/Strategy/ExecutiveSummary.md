# G. Advanced Graphics Engine Executive Summary

Status: non-normative orientation and decision summary

Date: 2026-08-02

Scope: principal graphics direction for SparkleEngine

## Decision

Develop SparkleEngine as a compact renderer-first engine and evidence platform for principal-level graphics engineering. Prioritize a few complete, reproducible vertical slices over feature count, framework breadth, or documentation volume.

The engine should make it easy for a reviewer to answer:

- Where do game, renderer, frame graph, RHI, and backend authority begin and end?
- How do source assets and shaders become deterministic runtime artifacts?
- How are D3D12 and Vulkan behavior compared and debugged?
- Which path-tracing and neural features are real, supported, and measured?
- What do CPU/GPU time, pacing, memory, quality, and failure behavior look like on declared workloads?
- Can another engineer build, reproduce, understand, and adopt the result?

## Authority

This summary does not restate detailed contracts:

- [A. Requirements](Requirements.md) owns `PGE-01` through `PGE-15` and evidence meaning.
- [C. Gap Assessment](GapAssessment.md) is the dated state assessment.
- [F. Roadmap](Roadmap.md) owns sequencing and allocation.
- [H. Engineer Persona](EngineerPersona.md) owns the operating model.
- [I. Acceptance Workloads](../Engineering/BistroAndSanMiguelWorkloads.md) owns scene, quality, performance, and evidence gates.
- [Architecture](../Architecture/README.md) owns system decisions and repository maps.
- [Engineering Standards](../Engineering/Standards/README.md) own implementation and review rules.

If this summary conflicts with an owning document, the owning document controls.

## Product Identity

Sparkle is one coherent product with three reinforcing purposes:

1. prove deep real-time rendering, GPU systems, and developer-technology capability;
2. provide a focused platform for path-tracing and neural-graphics work;
3. remain a usable independent engine where product work strengthens the first two goals.

The intended result is smaller and more explicit, not broader:

- one mutable authority per domain;
- narrow public contracts and backend-neutral renderer policy;
- D3D12 and Vulkan as first-class supported paths;
- frame graph as render scheduling and barrier authority;
- deterministic source/shader/model artifacts;
- bounded tasks, memory, queues, and GPU lifetimes;
- existing debugger/capture capability used instead of new reporting frameworks;
- obsolete paths deleted when a replacement lands.

## Workload Ladder

The planning horizon uses one fixed ladder:

- **Sponza** — Tier 0 startup and rapid regression.
- **Bistro exterior/interior** — primary Tier 1 flagship and narrative spine.
- **San Miguel 2.0** — supported Tier 1 secondary scene and cross-scene quality/generalization check.

Tier 1 claims require Tier 1 evidence. The renderer and content pipeline must remain generic; no scene-specific engine or shader branches are accepted.

## Priority Outcomes

| Priority | Outcome | Evidence direction |
| --- | --- | --- |
| P0 | Reviewer trust and reproducibility | clean build/run path, tests/validation, accurate support claims, compact navigation |
| P0 | D3D12/Vulkan workload analysis | paired captures, resource/queue/barrier explanation, causal experiments, reduced issue reproducer |
| P0 | Shader and GPU ABI discipline | source-to-package trace, reflection/layout checks, DXIL/SPIR-V inspection, backend parity |
| P0 | Whole-system performance evidence | p50/p95/p99, pacing, memory high-water, CPU/GPU timelines, controlled before/after results |
| P1 | Path-traced flagship | reference math/images, RT resource lifetime, quality/latency/memory evidence on both Tier 1 scenes |
| P1 | Real neural graphics feature | bounded trained model/operator, deterministic artifact, optimized runtime inference, classical fallback, quality/performance frontier |
| P1 | Technology transfer | narrow adoption surface, failure/fallback guide, reproducible demo, technical note, external or peer review |

The [roadmap](Roadmap.md) owns the order and time budget. This table only states the durable outcome hierarchy.

## Non-Goals

Do not optimize the portfolio or engine for:

- maximum feature count;
- a general-purpose ML runtime or tensor framework;
- a second renderer, scheduler, scene authority, content database, or diagnostics product;
- speculative future-hardware APIs;
- large embedded source-scene depots;
- editor/launcher breadth unrelated to flagship evidence;
- claims based on names, scaffolding, one FPS number, one backend, or unverified generated output;
- documents that repeat requirements, architecture, standards, or validation contracts.

## Reviewer Path

An external reviewer should not read the whole repository documentation:

1. read this summary;
2. inspect the [repository map](../Architecture/WholeRepositoryMap.md) and [Renderer/RHI boundary](../Architecture/RendererRhiBoundary.md);
3. inspect one completed case study and its code, capture, measurements, and limitations;
4. use [Requirements](Requirements.md) only to evaluate the full principal-level evidence target;
5. use the remaining strategy, research, and implementation-plan documents only when auditing rationale or history.

## Success Bar

Sparkle succeeds when a skeptical engineer can reproduce a small number of meaningful results, trace their ownership from product input to GPU execution, understand the mathematics and tradeoffs, see exact backend and hardware evidence, and take over the code without receiving unwritten context.

The strongest final signal is not repository size. It is that completed advanced work made the codebase easier to understand, validate, and extend.
