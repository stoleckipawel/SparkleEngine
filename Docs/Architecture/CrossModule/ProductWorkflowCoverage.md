# Product And Developer Workflow Coverage

Status: capability snapshot; horizontal workflow inventory; not usability, runtime, package, or release evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; current module inventories, build membership, product/tool entry points, and release contracts reconciled; evidence `S` only

Scope: user-visible and contributor-visible journeys that cross Application, Editor, Launcher, tools, content, build, project, and delivery owners

Owners: each concrete module named in a row owns its implementation; this document owns only the cross-module comparison

Vertical companion: [Product Execution Traces](ProductExecutionTraces.md)

Graphics companions: [Graphics Feature Coverage Matrix](GraphicsCoverageMatrix.md) and [Graphics Feature Execution Traces](FeatureExecutionTraces.md)

Evidence and disposition: [Capability Evidence Plan](../../Plans/CapabilityEvidence.md), [Feature Completion Reports](../../Acceptance/FeatureCompletionReports.md), and [First Release Acceptance](../../Acceptance/FirstRelease.md)

## How To Read This Matrix

`Implemented path` means the source/build route exists. `Partial` means the journey has a usable subset or stops before a required outcome. `Capability-gated` means external content, tools, SDKs, devices, or configuration decide availability. `Not found` means the inspected tree has no current owner for the required outcome. Every row remains source evidence only until its real actor completes the journey and retained evidence records the result.

## Workflow Matrix

| ID | Actor and question | Current entry -> intended outcome | State | Current boundary and missing proof |
| --- | --- | --- | --- | --- |
| `WF-001` | First source adopter: “How do I understand, obtain, and start Sparkle?” | Repository -> Docs/Launcher -> configured workspace -> Showcase | Partial | Deep Docs and Launcher discovery exist, but no root `README.md`, frozen clean-clone instructions, supported-host manifest, or independent first-user result exists. |
| `WF-002` | Developer: “What prerequisite is missing?” | Launcher readiness/Quick Start -> next concrete dependency, configure, content, cook, or run operation | Implemented path | Capability graph, provider status, recovery hints, and downstream invalidation exist. Truthfulness, offline/proxy/permission behavior, and non-author comprehension remain unexecuted. |
| `WF-003` | Developer: “Can I configure and build the intended product?” | CMake/Launcher profile and target selection -> development artifacts | Implemented path | Six profiles, MSVC/clang-cl routes, target-owned outputs, and optional features exist. Clean reproducibility and the frozen support matrix remain open. |
| `WF-004` | Developer: “Can I obtain a selected example scene safely?” | Level catalog -> pack readiness -> download/hash/extract -> source content | Capability-gated | Catalog/provider route exists only for declared downloadable/runtime-supported packs. Resume, disk-full, proxy, interrupted extraction, provenance, and destructive containment need evidence. |
| `WF-005` | Developer: “Can I cook everything the runtime needs?” | Project/catalog -> shader/texture/scene plan -> transactional cooked products | Implemented path | Shaders, textures, meshes, materials, scenes, skeletons, and animations have owned cook paths. Determinism, stale-product removal, corruption/failure atomicity, and package completeness remain open. |
| `WF-006` | Developer/user: “Did the selected level actually launch?” | Launcher run -> editor/runtime child -> requested level activation | Implemented path | Product selection, profile/API environment, readiness checks, and child launch exist. Evidence must follow the final child and activation log; initial Launcher success/PID and fallback `Empty` are insufficient. |
| `WF-007` | Editor user: “Can I open, inspect, edit, undo, save, and see the result?” | Editor workspace -> level/world view -> typed transaction -> world publication -> Renderer -> save | Partial | Open/save, selection, fixed property edits, undo/redo, viewport, and diagnostics exist. Dirty state, Save As, autosave/recovery, general creation/deletion, import UX, and independent usability remain absent or unproven. |
| `WF-008` | Runtime user: “Can I select a scene, understand controls, configure visuals, recover, and quit?” | ShowcaseRuntime -> startup selection -> camera/world/render loop -> exit | Partial | Startup level/environment selection, camera input, persisted Renderer settings, optional console, and ordered shutdown exist. A packaged first-run chooser/help/settings/reset/recovery journey and standard-user proof do not. |
| `WF-009` | Content engineer: “Will my source asset preserve intended semantics?” | glTF/GLB/FBX -> importer -> cooker -> cooked loaders -> world -> Renderer | Partial | Concrete geometry/material/camera/light/skin/morph/animation subsets exist with format asymmetries. Fidelity fixtures, unsupported-input UX, provenance, bounds, and per-format end-to-end results remain open. |
| `WF-010` | Shader engineer: “Can I identify, compile, diagnose, publish, and reload a program?” | Typed registration/source -> compiler plan -> DXIL/SPIR-V products -> validated generation -> Renderer reload | Capability-gated | Catalog, compilation, reflection/ABI checks, transactional publication, diagnostics, and editor recook exist. Clean tool discovery, every registered job/target, failure recovery, runtime parity, and Shipping erasure need evidence. |
| `WF-011` | Graphics engineer: “What path is active and how do I inspect it?” | Setting/CVar/editor selector -> requested/active Renderer plan -> diagnostics/debug view/capture | Partial | Settings, debug views, Renderer/RHI diagnostics, and async captures exist. Requested-versus-active reporting is not uniformly product-visible; diagnostic truth, output encoding, capture correctness, and external-tool correlation remain open. |
| `WF-012` | Operator/developer: “Can I cancel, retry, reload, switch levels, and shut down safely?” | Launcher/editor/runtime request -> task/process/GPU work -> settlement and cleanup | Partial | Multiple owners implement cancellation, generation checks, queue retirement, and ordered shutdown. Cross-owner retry semantics, descendant-process cleanup, interruption atomicity, repeat-switch growth, and device-loss behavior are not proven. |
| `WF-013` | Release owner: “Can I stage, install, and run immutable bytes outside the repository?” | Built/cooked artifacts -> manifest/stage/sign/package -> standard-user clean-machine run | Not found | Development artifact copying exists; no formal install/CPack/package manifest/archive/signing owner exists. This journey stops before release packaging. |
| `WF-014` | Reviewer: “Can I tell what ships and why?” | Inventory row -> evidence plan -> candidate completion report -> release disposition | Partial | Architecture, plan, acceptance, persona, roadmap, and report schemas exist. Reachable selectors are still Pending and no candidate-bound reports currently close the decision. |
| `WF-015` | User/reviewer: “How are crashes, security reports, updates, and withdrawals handled?” | Failure/public report -> diagnostic/support intake -> severity/patch/advisory/withdrawal | Not found | Local diagnostic mechanisms exist, but no public support/security intake, crash-consent/upload, release-channel update, patch, advisory, or withdrawal operation is implemented. |
| `WF-016` | Project integrator: “How do I add a project or embed an engine product?” | Project marker/CMake convention -> editor/runtime targets -> catalog/assets -> cook/run | Partial | Automatic project discovery and Showcase conventions exist, but no supported-project creation guide, stable external SDK/API/ABI, template lifecycle, migration policy, or independent integration result exists. |
| `WF-017` | Performance investigator: “Where did time or memory go, and what change fixes it?” | Reproducible workload -> bounded live diagnostics -> external CPU/GPU capture -> causal experiment -> regression evidence | Partial | Diagnostics owners, external-profiler runbook, and workload contracts exist. No accepted end-to-end bottleneck investigation or automated performance regression path is recorded. |
| `WF-018` | Contributor/reviewer: “What is the smallest trustworthy validation for this change?” | Changed claim -> owner/consumer analysis -> focused check -> retained result -> escalation only if needed | Partial | Claim-driven validation and custom architecture/style routes exist, but no CTest inventory, CI workflow, generated affected-check map, or clean hosted result exists. |

## Journey Interaction Contract

Every workflow proposed for inclusion must eventually record these fields in its candidate-bound completion report:

| Phase | Questions that require an answer |
| --- | --- |
| Discover | Where does the actor start, what is the expected outcome, and which constraints are visible before work begins? |
| Prepare | Which prerequisites, permissions, content, tools, device features, disk/memory budgets, and destructive effects are preflighted? |
| Select | What exact product/profile/backend/mode/map/asset is requested, and how is the active result shown? |
| Execute | What progress, duration, cancellation, concurrency, and background ownership are visible? |
| Succeed | What observable artifact, active scene, image, state, or report proves success rather than mere process responsiveness? |
| Fail | What failed, at which owner, what remains safe, and what action can the actor take next? |
| Recover | Can the actor retry, resume, revert, reload, or clean without hidden state or data loss? |
| Persist | Which settings/products/history survive restart, where are they stored, and what invalidates them? |
| Inspect | Which logs, diagnostics, captures, manifests, hashes, and requested-versus-active state support diagnosis? |
| Exit | What work settles or cancels, what resources retire, and how is a clean final state observed? |
| Prove | Which revision/environment/oracle/artifacts/reviewer establish the result, and what change invalidates it? |

An unanswered field is not automatically a feature defect, but it is a documentation/evidence gap. Mark it `Unknown`, `Not applicable`, or route it to a stable `INV-*`, evidence-plan, risk, failure-mode, or acceptance item; never leave the reader to infer it.

## Current Horizontal Closure

The source tree has recognizable development journeys from discovery through development artifacts and from authored content through a rendered result. The largest product discontinuity is after development staging: package, standard-user first run, support/security, update, and independent adoption do not yet have implemented owners. Inside implemented journeys, the recurring evidence gaps are requested-versus-active truth, failure/recovery behavior, bounded resource behavior, and proof on the complete actor-visible route.

This matrix does not choose release scope. Freeze Included, Experimental, Excluded, or Removed only through the acceptance owner, then execute the smallest checks in the capability evidence plan.
