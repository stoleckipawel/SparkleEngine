# Stage Learning Guide

Status: tutor-facing stage guide
Date: 2026-06-13

This file is meant to be read by you while working through the implementation stages. It explains the lesson behind each stage, what to inspect, and what a strong reviewer should notice.

For implementation prompts, use [../implementation/stage-prompt-packets.md](../implementation/stage-prompt-packets.md).

## How To Read A Stage

For each stage, ask:

- What architectural habit is this teaching?
- Which boundary or contract becomes clearer?
- What data crosses the boundary?
- What complexity is being removed or forced to justify itself?
- What would break if future work ran on multiple threads?
- What evidence would convince a skeptical reviewer?

## Stage Lessons

| Stage | What you are learning | What to inspect while implementing | Reviewer signal |
| --- | --- | --- | --- |
| 1 | Architecture starts with evidence, not vibes. | Coverage rows, status labels, owner/risk/evidence fields. | You can audit what exists before changing it. |
| 2 | Shared vocabulary prevents accidental architecture. | Glossary, maps, contract names, before/after navigation. | A reviewer can understand the system without guessing local meanings. |
| 3 | Boundaries become real when checks enforce them. | Forbidden include/dependency rules and transitional exceptions. | Architecture is mechanical, not only prose. |
| 4 | Renderer pass policy belongs above hardware abstraction. | Shader registration ownership, package ids, DirectLighting proof. | Adding a pass no longer edits RHI. |
| 5 | Early validation keeps refactors honest. | Boundary command, ShaderCompiler/package enumeration, affected builds. | The first migration is proven before deeper work begins. |
| 6 | Large interfaces need ownership maps before extraction. | RHI methods, callers, backend impact, service categories. | RHI slim-down is based on evidence, not random slicing. |
| 7 | Services should emerge from caller evidence. | Interop, capture, diagnostics, presentation service boundaries. | RHI services have real contracts and do not become a new god object. |
| 8 | Hosts orchestrate validation; backends own native work. | Application smoke code, D3D12 capture/readback, RHI/backend services. | Application is no longer a backend-native shortcut. |
| 9 | Vendor features need provider boundaries. | DLSS/Streamline/native metadata, fallback reasons, renderer provider code. | Vendor SDK details are isolated and backend-supported. |
| 10 | Backend parity is evidence, not hope. | D3D12/Vulkan logs, captures, launcher smoke, capability reports. | Both backends are compared through reproducible artifacts. |
| 11 | A facade should be a host protocol, not a hidden engine. | `Renderer`, system root, frame pipeline, lifecycle/diagnostics. | Renderer becomes navigable and future render-thread-friendly. |
| 12 | Presentation is a contract between renderer and hosts. | Viewport products, frame graph/resource transitions, Application/Editor callers. | Hosts receive products instead of driving internals. |
| 13 | Renderer should consume render-domain snapshots, not gameplay internals. | Scene data, mesh/material/texture managers, temporal/upscaling inputs. | GameFramework and Renderer can evolve independently. |
| 14 | A frame graph is a contract, not a bag of passes. | Setup/compile/execute phases, resource declarations, barriers, diagnostics. | Resource failures are actionable and future command batches are possible. |
| 15 | Milestone validation turns design into confidence. | Smoke logs, graph diagnostics, D3D12/Vulkan evidence. | The facade/frame graph work is proven before PSO/pass redesign. |
| 16 | Pipeline identity must be explicit and deterministic. | PSO keys, package identity, binding layouts, cache ownership. | Runtime pipeline behavior is inspectable across backends. |
| 17 | Pass authoring should be declarative and low-ceremony. | Pass definitions, pass catalog, graph setup, pipeline lookup. | Adding ordinary passes is localized and reviewable. |
| 18 | Ray tracing needs split ownership from scene to AS build. | BLAS/TLAS generations, AS requests, shadow data, RHI RT descriptors. | Renderer owns feature policy; RHI owns API-level AS work. |
| 19 | Backend symmetry exposes real API differences. | D3D12/Vulkan service maps, CMake scopes, cross-backend includes. | Shared semantics are common; API differences are explicit. |
| 20 | Full graphics validation should be reproducible. | Builds, shader packages, launcher smoke, captures, feature logs. | Claims about parity/reliability have artifacts behind them. |
| 21 | Portfolio presentation is engineering evidence. | README, reviewer path, screenshots, known issues, validation commands. | A stranger can inspect the work quickly and fairly. |
| 22 | Cleanup and rubric scoring are part of engineering. | Stale exceptions, duplicate paths, rubric scores, evidence index. | The first graphics track closes cleanly instead of leaving debt fog. |
| 23 | Whole-repo architecture starts with routing every root to active work. | Durable roots, owner/dependencies, folder policy, validation, active stage. | RHI/Renderer work cannot leave other modules as vague follow-up. |
| 24 | Core and Platform should be boring in the best way. | Foundation helpers, OS/window/input abstractions, diagnostics, events. | Low-level modules are reusable because they do not own domain policy. |
| 25 | GameFramework should own runtime/cooked concepts only. | Runtime scene, snapshots, AssetContracts, RenderContracts, forbidden edges. | Runtime and content pipeline are connected by schemas, not private code. |
| 26 | Runtime loaders and cooked schemas are two sides of one contract. | Mesh/material/texture/scene/animation/skeleton loaders and producers. | Loader bugs become diagnosable schema/report failures, not renderer mysteries. |
| 27 | Source import belongs before cooking and outside runtime. | SourceImporters, imported DTOs, import diagnostics, format-specific code. | Runtime never needs to know how a source format was parsed. |
| 28 | Focused cookers should transform one artifact family well. | Texture/Mesh/Material/Scene cookers, CookDiagnostics, ToolConsoleSupport. | Cook pipeline is deterministic, inspectable, and not a broad common bucket. |
| 29 | Shader tooling deserves contract-grade design too. | ShaderContracts, package manifests, reflection reports, backend options. | ShaderCompiler is a deterministic tool, not a renderer-runtime side door. |
| 30 | Orchestrators should orchestrate, not implement everything. | AssetCooker, AssetConverter, LauncherCore, Qt GUI, Application, Editor. | Workflows are process/report driven and UI/hosts do not own tool algorithms. |
| 31 | Artifacts need producer/schema/consumer proof. | Shader packages, textures, meshes, materials, scenes, inspectors. | Schema changes are validated beyond "it compiled." |
| 32 | Samples and built-in assets are architecture evidence. | Projects, Showcase, Engine/Assets, generated outputs, validation artifacts. | Sample content has ownership and proves real workflows. |
| 33 | Global guardrails prevent architectural backsliding. | Runtime-to-tools checks, folder checks, generated roots, CI/local wiring. | The repository enforces the design mechanically. |
| 34 | The evidence gate checks that everything agrees. | Coverage, target docs, code, CMake, tools, samples, validation logs. | The repo is coherent, not just locally fixed. |
| 35 | Threading-readiness is ownership discipline. | Mutable owners, phases, handoffs, isolation, ordering, diagnostics. | Future parallelism becomes a straightforward implementation step. |
| 36 | Review-ready means the whole repository scores well. | Stale paths, rubric scores, validation commands, known risks. | Non-rendering modules are not hidden behind strong graphics work. |

## Reflection Prompts

Use these after each stage:

1. What private dependency became unnecessary?
2. What data shape became more explicit?
3. Which subsystem became easier to test?
4. Which future change became safer?
5. What complexity was removed, renamed, or forced to justify itself?
6. What would a reviewer still distrust, and what evidence would answer them?
