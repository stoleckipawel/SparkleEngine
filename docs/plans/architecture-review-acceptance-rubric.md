# Architecture Review Acceptance Rubric

Status: initial criteria draft  
Date: 2026-06-12  
Use with: `docs/plans/rhi-renderer-architecture-review.md`

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
