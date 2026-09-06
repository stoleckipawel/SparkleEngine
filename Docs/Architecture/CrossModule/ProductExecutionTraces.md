# Product And Developer Execution Traces

Status: capability snapshot; vertical cross-module traces; not workflow success, package proof, or release approval

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; current module inventories, source/build ownership, and product/tool routes reconciled; evidence `S` only

Scope: the vital non-graphics journeys from developer/user intent through build, content, world, editor, diagnostics, publication, cancellation, and delivery boundaries

Owners: concrete modules own each stage; this document owns only the cross-module trace and unresolved boundary visibility

Horizontal companion: [Product Workflow Coverage](ProductWorkflowCoverage.md)

Graphics companion: [Graphics Feature Execution Traces](FeatureExecutionTraces.md)

## Trace 1: Quick Start To A Live Level

| Stage | Owner and transition | Observable result | Open evidence/failure boundary |
| --- | --- | --- | --- |
| Request | Launcher GUI/shell receives project, product, profile, API, and level intent | Typed operation request | Invalid/unsupported combinations must fail before destructive/process work. |
| Resolve | Launcher capability graph checks host tools, dependencies, workspace, content, cooked products, and executable | One next unmet operation or runnable capability | Truthfulness must be exercised from clean, stale, partial, and offline states. |
| Repair/build | Provider/planner invokes dependency sync or CMake configure/build for the selected scope | Process result, logs, refreshed stamps/artifacts | Exit code, expected artifacts, cancellation, and descendant cleanup—not output text alone—define success. |
| Acquire | Level catalog and content provider resolve pack metadata, hash, archive, and extraction root | Required source content becomes ready | Network, proxy, disk, hash, interruption, path containment, and provenance remain evidence gates. |
| Cook | AssetCooker delegates shader/texture work and publishes scene product sets | Complete cooked generation and registry | Mixed generations, stale products, or silent partial success block launch readiness. |
| Launch | Launcher starts ShowcaseEditor or ShowcaseRuntime with project working directory and requested environment | Final child process and log identity | Launcher PID/exit does not prove the child stayed alive. |
| Activate | Application creates LevelSession; cooked registry/load graph assembles GameWorld; requested level becomes active | Requested catalog identity is logged/published and rendered | Fallback `Empty`, wrong level, or responsive window without activation is failure. |
| Exit | Application unwinds console, Renderer, level/world/tasks/input/window owners | Final process exit and settled resources | Repeated launch/exit and in-flight cancellation require execution evidence. |

## Trace 2: Authored Asset To Rendered Result

| Stage | Owner and transition | Contract crossing the boundary | Open evidence/failure boundary |
| --- | --- | --- | --- |
| Catalog/source | Showcase level/catalog identifies glTF, GLB, or FBX source and required packs | Stable scene identity plus source/provenance metadata | Missing, unsupported, oversized, path-escaping, or unlicensed input needs explicit rejection. |
| Import | SourceImporters parse and normalize geometry, transforms, materials, textures, cameras, lights, skin, morph, and animation | Imported semantic scene in engine coordinates | glTF/FBX coverage differs; unsupported semantics must not look successfully preserved. |
| Plan | AssetCooker discovers selected scenes and deduplicates stable asset requests | Ordered shader -> texture -> scene-assets plan | Conflicting IDs/references and missing tools must fail before publication. |
| Transform | Texture/Mesh/Material/Scene cookers build runtime schemas | Validated products and cross-references | Numeric fidelity, bounds, resource limits, and deterministic identity remain open. |
| Publish | Core file-set publication replaces complete generations | Scene registry plus manifests/assets visible together | Interruption must preserve the previous complete generation. |
| Load | GameFramework cooked-only readers validate magic/counts/ranges and assemble level data | World resources/components with stable IDs | Schema versioning is absent; stale/incompatible/corrupt products need rejection evidence. |
| Simulate/publish | GameWorld systems update and `RenderFrameSubmissionExtractor` publishes scene deltas, dynamic arrays, resource tables, and view input | Immutable/movable frame submission | Copy budget, generation safety, serial/parallel determinism, and cancellation remain open. |
| Render/present | Renderer caches resources, prepares scene/view, executes frame graph/RHI, and presents/captures | Actor-visible image and diagnostics | A plausible image alone does not prove semantic fidelity, backend parity, or resource retirement. |

## Trace 3: Editor Inspection And Transaction

| Stage | Owner and transition | Contract crossing the boundary | Open evidence/failure boundary |
| --- | --- | --- | --- |
| Open | Editor requests a catalog level through Application/LevelSession | Level-load state and eventual immutable read view | Loading, cancellation, missing assets, fallback, and active identity must be visible. |
| Inspect | World read view/change journal -> Editor scene model/outliner/inspectors | Presentation rows keyed to world identity/generation | Stale rows or held snapshots must not become mutable world authority. |
| Edit | Inspector emits a fixed typed forward command and inverse at current generation | Queued `WorldEditCommand` | Unsupported properties and stale generations require visible rejection. |
| Commit | GameWorld validates and applies the command; journal/change sequence advances | New world state and change identity | Atomicity and failure must be observable; UI optimism cannot masquerade as commit. |
| Render | Subsequent extraction publishes structural/dynamic change to Renderer | Updated frame without direct Editor mutation of Renderer scene data | View-only camera/settings remain separate from authored world state. |
| Undo/redo | Transaction history resubmits stored inverse/forward command through the same route | Reversible committed world result | Level replacement, stale identity, repeated undo/redo, and partial failure need evidence. |
| Save | Editor serializes its supported level-document changes | Updated source level document | Dirty-state, Save As, autosave, recovery, arbitrary schema preservation, and source-control integration are not implied. |

## Trace 4: Rendering Setting To Active GPU Path

| Stage | Owner and transition | Contract crossing the boundary | Open evidence/failure boundary |
| --- | --- | --- | --- |
| Request | DefaultEngine.ini, command-line CVar, editor settings, or runtime console selects a value | Requested setting with source/scope | Command-line unknown/parse failures are currently silent; precedence and malformed persistence need evidence. |
| Persist/apply | Renderer settings persistence writes its fixed section; Application applies persisted values before host creation; editor commits runtime changes | Typed CVar/settings state | View mode is session-only; adapter preference and back-buffer format require restart. Write failure/package path behavior is open. |
| Resolve | Renderer combines settings with RHI/provider/device capability | Active GBuffer/lighting/traversal/provider/presentation plan or unavailable reason | Every selector needs requested-versus-active reporting; silent substitution blocks inclusion. |
| Reconfigure | Coordinator sends control state; FramePipeline rebuilds topology or provider/shader generation where needed | New graph/generation and invalidated history | In-flight settings changes, retirement, resize, and failure rollback require evidence. |
| Execute | Frame graph materializes passes/pipelines and RHI lowers work to the selected backend | Presented/captured output plus diagnostics | Each unlike backend/mode cell retains an independent verdict. |

## Trace 5: Shader Change To Safe Runtime Generation

The complete graphics-side trace is [Shader Authoring To Runtime Generation](FeatureExecutionTraces.md#trace-6-shader-authoring-to-runtime-generation). From the developer's workflow perspective:

| Stage | Owner and transition | Required visible behavior |
| --- | --- | --- |
| Detect/select | Editor or command line chooses all, one registered shader ID, or changed-source closure | Exact affected set and prerequisites are inspectable. |
| Execute | Editor operation service launches ShaderCompiler with cancellation signal and captures command/output/result | Progress, cancellation, nonzero exit, and diagnostic source remain visible. |
| Publish | Compiler validates ABI/reflection and atomically publishes map/library/dependency data plus publication identity | Partial/stale/outside-authority products are rejected. |
| Admit | Application verifies publication identity, paths, hashes, and freshness before asking Renderer to reload | Failure leaves the previous generation active and says why. |
| Swap/retire | Renderer builds a complete replacement generation, publishes it, and retires old GPU objects after queue completion | No mixed generation, device-idle shortcut, stale pipeline, or hidden source/compiler dependency in Shipping. |

## Trace 6: Capture And Diagnostic Handoff

| Stage | Owner and transition | Contract crossing the boundary | Open evidence/failure boundary |
| --- | --- | --- | --- |
| Request | Editor/public Renderer API selects final or intermediate viewport product | Capture ID plus product intent | Unavailable product/format must return a useful failure. |
| Resolve/read back | Renderer capture service resolves current resource/generations; RHI performs asynchronous texture readback | Completed pixel payload with extent/format/identity | Color encoding, row pitch, orientation, depth interpretation, and resize races need numeric proof. |
| Publish | Renderer polls completion and exposes bounded completed results to Application/Editor | Movable result or explicit pending/absent state | Completed queue drops oldest beyond capacity; UI must not present absence as success. |
| Inspect/correlate | Editor diagnostics plus RHI markers/object names/native validation support PIX/RenderDoc/Nsight/RGP/WPA investigation | Human-readable report and retained external artifact | Counters/timings/memory must be reconciled with one native capture and observer cost. |

## Trace 7: Cancellation, Failure, And Shutdown Settlement

| Boundary | Current owner | Required settlement question |
| --- | --- | --- |
| Launcher operation | Launcher background operation/task plus Core child-process runner | Does cancellation stop the owning process tree, report the final state once, preserve unrelated files, and invalidate only affected readiness? |
| Import/cook | Asset/texture/shader tool plus Core publication helper | Does any item failure prevent mixed publication, release memory leases, and preserve the previous readable generation? |
| Level load/switch | LevelSession task graph and GameWorld generation owner | Can an old/cancelled request ever commit after a newer selection, and are mapped/decoded resources released? |
| Editor operation | EditorOperationService and operation-specific owner | Is only one result consumed, does closure settle outstanding work, and does UI distinguish cancelled/failed/succeeded? |
| Renderer control/frame | RenderCoordinator, FramePipeline, RHI queue tokens | Are submitted frames/control requests settled and GPU-owned resources retired only after completion? |
| Process shutdown | Application | Are console, Renderer, level/world, Tasks, input/window, and timer destroyed in dependency order without new work starting? |

These questions are intentionally retained as evidence placeholders. Source contains partial settlement mechanisms, but only controlled cancellation/failure injection and repeat shutdown can answer them.

## Trace Closure Rule

A product trace is complete only when the actor can discover the entry, prerequisites and destructive effects are visible, the requested result is distinguishable from the active result, every representation has one owner, cancellation/failure leaves a defined safe state, success is observable at the final consumer, and candidate-bound evidence records the exact environment and artifacts. If a trace stops because no implementation owner exists—currently formal packaging and public support/update operations—keep that discontinuity explicit rather than drawing a future arrow.
