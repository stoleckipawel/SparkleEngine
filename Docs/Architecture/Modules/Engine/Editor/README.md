# Editor Capability Inventory

**Status:** capability snapshot; current, but not usability, correctness, or packaged-editor evidence

**Snapshot:** 2026-09-08 at committed `master` revision `ffe60e3a`; `Engine/Editor`, `SparkleApplicationEditor`, world-edit, Renderer diagnostic, save, shutdown, and CMake surfaces inspected; evidence `S` only

**Scope:** workspace UI, viewport, level actions, scene inspection/editing, undo/redo, rendering settings, shader/mesh/texture tools, console, capture, restart, and runtime separation

**Owner:** `Engine/Editor` / `SparkleEditor`; editor hosting and long-running operations belong to `SparkleApplicationEditor`

**Evidence and disposition:** [Capability Evidence Plan](../../CapabilityEvidencePlan.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

**Current readiness:** **45/100** — the development Editor is source-integrated; usability, correctness, performance, distribution classification, and non-author evidence remain open. See [Current Feature Readiness](../../../../Acceptance/CurrentReadiness.md#product-build-and-delivery).

## At A Glance

| Workflow | Current result | Main limitation |
| --- | --- | --- |
| workspace and level | fixed ImGui workspace, registered-level open, progress gating, and Save All | no general docking, new/Save As, or dirty-document workflow |
| scene inspection/editing | outliner plus typed camera/light/mesh/sky/variant commands with generation checks | not a full content/model/material authoring environment |
| history | command-based undo/redo for owned world edits | multi-document persistence and conflict behavior unproved |
| rendering controls | viewport camera, display/settings controls, diagnostics, console, and capture | requested/active feature truth and per-viewport isolation still need evidence |
| developer operations | shader recook and mesh/texture diagnostic tools through editor host services | workspace/tool availability and cancellation remain product constraints |

```mermaid
flowchart LR
    Intent[Editor UI intent] --> Command[Typed generation-checked command]
    Command --> World[World or settings owner commits]
    World --> ReadModel[Rebuild immutable editor read model]
    ReadModel --> Viewport[Renderer product and diagnostics]
    Viewport --> UI[Present result and operation state]
```

The Editor should expose owner commands and read models, not mutate Renderer/RHI/native state directly. That makes undo, concurrency, and failure tractable at the cost of explicit command/result plumbing.

## Workspace And Level Surface

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `ED-001` | Fixed workspace shell | Implemented path | ImGui main menu/title bar, scene outliner left, viewport/top controls center, scene inspector right, console dock, and floating utility panels. It is a purpose-built layout, not a general dockable workspace. | `S` |
| `ED-002` | Native window controls | Implemented path | Custom minimize, maximize/restore, drag, and close controls delegate to Platform; fullscreen hides these controls. | `S` |
| `ED-003` | Level open/save | Implemented path | File menu lists registered selected/ready levels, requests asynchronous activation, disables interaction during change, and Save All serializes the active level. No Save As, new level, or dirty-document prompt was found. | `S` |
| `ED-004` | Level progress/failure gating | Implemented path | UI interaction is disabled while LevelSession is changing; last load diagnostics and events are available to host/panels. Cancellation/recovery UX still requires execution evidence. | `S` |

## Scene Inspection And Editing

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `ED-005` | Scene outliner | Implemented path | Builds a read model of cameras, lights, meshes, sky, and material variants; supports object selection, type/visibility presentation, and active-camera choice. | `S` |
| `ED-006` | Camera inspector | Implemented path | Edits local transform, camera description/projection fields, visibility, and active camera through generation-checked world commands. | `S` |
| `ED-007` | Light inspector | Implemented path | Edits the current directional/point/spot/rect light description through one typed world command pair. Renderer limits remain elsewhere. | `S` |
| `ED-008` | Mesh inspector | Implemented path | Edits local transform and visibility and presents mesh/material/skeleton/source identity. It does not author geometry, replace meshes, or edit material properties. | `S` |
| `ED-009` | Sky inspector | Implemented path | Enables/disables and edits the current sky environment through world commands. It does not import/cook a new texture. | `S` |
| `ED-010` | Material variant selector | Implemented path | Lists cooked/imported named variants and changes the active variant. There is no variant authoring interface. | `S` |
| `ED-011` | Undo/redo | Implemented path | Ctrl+Z/Ctrl+Y execute inverse/forward world commands; history supports coalescing and is invalidated on world-generation change. Coverage is limited to the current seven world-edit payloads. | `S` |
| `ED-012` | Stale edit protection | Implemented path | Every edit submits with expected world generation and reports Accepted/Stale/Rejected; transaction history retains the last result. UI presentation of every rejection path needs evidence. | `S` |

## Viewport And Rendering Controls

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `ED-013` | Editor viewport | Implemented path | Publishes requested extent, renders Renderer scene-color products into ImGui, registers exact input bounds, and owns a separate navigable viewport camera. | `S` |
| `ED-014` | View modes | Implemented path | Top panel exposes the Renderer view-mode catalog, including lit and diagnostic modes. Exact 16-mode semantics/availability live in [Debug Views](../Renderer/Features/DebugViews/README.md). | `S` |
| `ED-015` | Exposure overrides | Implemented path | Per-viewport manual/automatic exposure controls override view presentation without changing scene state. | `S` |
| `ED-016` | Rendering settings | Implemented path | Searchable sections expose implemented renderer selectors: display/tone/output, exposure, upscaling, ray reconstruction, ray-tracing scene/provider, and broader render settings. Color grading, chromatic aberration, and frame generation have no settings because those features are absent. Apply/restart behavior follows each setting owner. | `S` |
| `ED-017` | Restart service | Implemented path | Settings can request an editor restart when a process-level setting requires it. Recovery/session restoration is not a current documented capability. | `S` |
| `ED-018` | Viewport capture | Implemented path | File menu queues a viewport capture through Application/Renderer coordination and reports completion. Format/path/content correctness remains open evidence. | `S` |

## Developer Tools

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `ED-019` | Editor console | Implemented path | Tilde shortcut opens a scoped console with parsing, history, autocomplete, output severity, built-in commands, and host-registered render/shader commands. | `S` |
| `ED-020` | Shader tools | Capability-gated | Lists registered/cooked generation rows; filters; shows source, reflection, disassembly, parameter match, and compile request artifacts; reloads generation; recooks all or selected shader. Requires workspace source/tools. | `S` |
| `ED-021` | Mesh diagnostics | Implemented path | Filterable used-mesh table with source/identity, CPU/GPU load and byte estimates, instancing summary, and local preview controls/geometry. Values are diagnostics, not profiler-certified truth. | `S` |
| `ED-022` | Texture diagnostics | Implemented path | Filterable used-texture table with source/path, format/dimensions/mips, residency and byte estimates, selection inspector, and preview. | `S` |
| `ED-023` | Memory/renderer diagnostics | Implemented path | Host wires Renderer memory/mesh/texture/shader providers to UI. External capture and performance authority remain outside the editor. | `S` |
| `ED-024` | Icon/theme assets | Capability-gated | Custom palette/theme and Font Awesome Solid glyphs; CMake fails configuration if the required font is absent. | `S` |

## Vertical Edit Trace

World publishes immutable read view/change sequence -> `EditorSceneModelBuilder` creates presentation rows -> user selects an object -> inspector creates forward and inverse typed commands -> transaction history submits with current world generation -> GameWorld queues and commits accepted edits -> change journal advances -> editor rebuilds affected presentation -> Renderer receives the resulting structural/dynamic publication. Undo runs the stored inverse through the same path.

## `FCR-PROD-04` Editor Contract

The Editor is **Developer-only** for `v0.1.0`. Its source-built journey must be truthful, but it is not a consumer prerequisite or runtime-archive component. [Application](../Application/README.md#product-contract-boundary) owns hosting; this section owns the feature-local verdict contract.

| ID | Binary acceptance criterion |
| --- | --- |
| `AC-PROD04-01` | A source adopter on the supported toolchain can build and start `ShowcaseEditor` from the documented repository/project context, identify its Developer-only status and workspace prerequisites before use, and exit while owned operations settle within the frozen Editor budget. |
| `AC-PROD04-02` | For one admitted authored level, open -> inspect -> supported typed edit -> undo -> redo -> explicit Save All -> reload produces the expected world/source result; only explicit Save All may mutate the selected `.level` source. |
| `AC-PROD04-03` | Missing source/tool/shader/cooked content, stale world generation, rejected edit, failed save, cancelled recook/capture, and restart-required setting remain distinguishable and preserve the previous accepted world/source/generation. |
| `AC-PROD04-04` | Runtime archive/import/file inspection contains no `ShowcaseEditor`, `SparkleEditor`, `SparkleApplicationEditor`, Editor UI/assets, shader-recook/capture operation, or Editor-only dependency. |

| ID | Cause/injection, safe result, and affected criterion |
| --- | --- |
| `FM-PROD04-01` | Omit a required workspace/tool input. Configure/start/operation fails with the missing owner and recovery; it never degrades to a consumer promise or reports success (`AC-PROD04-01`, `AC-PROD04-03`). |
| `FM-PROD04-02` | Change world generation or force an unsupported/stale edit. The command is rejected visibly, history stays coherent, and no optimistic UI state becomes source truth (`AC-PROD04-02`, `AC-PROD04-03`). |
| `FM-PROD04-03` | Deny or fault the selected level save. The existing source bytes retain their hash and the failure remains actionable; no temp/partial file is accepted (`AC-PROD04-02`, `AC-PROD04-03`). |
| `FM-PROD04-04` | Inspect the runtime archive for Editor targets/symbols/strings/imports. Any reachable or delivered Editor surface fails `AC-PROD04-04`. |

| Check | Claims falsified | Smallest route and oracle |
| --- | --- | --- |
| `CHK-PROD04-01` | `AC-PROD04-01`, `AC-PROD04-02`, `AC-PROD04-03`; `FM-PROD04-01`–`03` | Focused supported-toolchain Editor build, then one scripted non-author open/edit/undo/redo/save/reload/exit route on a disposable source copy with each named fault. World/source hashes and operation results are the oracle. |
| `CHK-PROD04-02` | `AC-PROD04-04`; `FM-PROD04-04` | Independently inventory the verified runtime stage/archive and `ShowcaseRuntime` dependency/import surface against an Editor denylist. Zero matches is required. |

Candidate results belong in `FCR-PROD-04`; no usability, build, runtime, save, or package result was produced by this contract pass.

## Explicit Non-Capabilities And Risks

- No general content browser, drag-and-drop import, material graph/editor, mesh authoring, entity creation/deletion UI, arbitrary component inspector, prefab editor, animation timeline, sequencer, plugin UI, multi-document tabs, or collaborative editing was found.
- Save does not imply dirty tracking, autosave, crash recovery, source control integration, or Save As.
- Renderer selectors visible in UI still need a finite release disposition; a control existing does not make its backend/feature combination shippable.
- `SparkleEditor` exists in all configured workspace builds, while Game products avoid linking it. Only final package inspection proves erasure.
