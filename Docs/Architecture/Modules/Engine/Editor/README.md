# Editor Capability Inventory

Status: capability snapshot; current, but not usability, correctness, or packaged-editor evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; `Engine/Editor`, `SparkleApplicationEditor`, world-edit, Renderer diagnostic, and CMake surfaces inspected; evidence `S` only

Scope: workspace UI, viewport, level actions, scene inspection/editing, undo/redo, rendering settings, shader/mesh/texture tools, console, capture, restart, and runtime separation

Owner: `Engine/Editor` / `SparkleEditor`; editor hosting and long-running operations belong to `SparkleApplicationEditor`

Evidence and disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

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

## Explicit Non-Capabilities And Risks

- No general content browser, drag-and-drop import, material graph/editor, mesh authoring, entity creation/deletion UI, arbitrary component inspector, prefab editor, animation timeline, sequencer, plugin UI, multi-document tabs, or collaborative editing was found.
- Save does not imply dirty tracking, autosave, crash recovery, source control integration, or Save As.
- Renderer selectors visible in UI still need a finite release disposition; a control existing does not make its backend/feature combination shippable.
- `SparkleEditor` exists in all configured workspace builds, while Game products avoid linking it. Only final package inspection proves erasure.
