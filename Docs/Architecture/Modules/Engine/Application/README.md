# Application Capability Inventory

Status: capability snapshot; current, but not release approval or executable evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; `Engine/Application` runtime/editor hosts, command-line/configuration, task ownership, console, shader-recook, and CMake split inspected; evidence `S` only

Scope: process entry, runtime loop, host composition, input/world/render sequencing, threaded-render selection, runtime console, editor composition, shader recook, capture coordination, startup/shutdown

Owner: `Engine/Application` / `SparkleApplication` and `SparkleApplicationEditor`

Evidence and disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

## Product Split

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `APP-001` | Shared application lifecycle | Implemented path | `Application::Run` drives virtual Initialize/Tick/Shutdown; process configuration applies registered CVars from command line before host creation. | `S` |
| `APP-002` | Cooked runtime host | Implemented path | `RuntimeApplication` composes Timer, Window, InputSystem, Tasks, GameWorld, LevelSession, Renderer, camera input, and optional runtime console. It links no source importer or cooker. | `S` |
| `APP-003` | Editor host | Implemented path | `EditorApplication` embeds the runtime host with runtime console disabled and UI render packets enabled, then adds editor UI, operations, shader recook, and viewport capture coordination. | `S` |
| `APP-004` | Build-time editor erasure | Implemented path | `SparkleApplication` excludes editor, shader-recook, capture, and editor-operation sources; `SparkleApplicationEditor` owns them and privately links `SparkleEditor`. Game products link only the runtime target. Package inspection is still required. | `S` |

## Runtime Loop And Ownership

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `APP-005` | Frame pump | Implemented path | Input begin -> window poll -> deferred input -> timer tick -> close/minimize decision -> pending level change -> simulation begin/update/end -> render submission -> render. Minimized/zero-size windows wait for an event and skip rendering. | `S` |
| `APP-006` | Task ownership | Implemented path | Application owns the executor and root Application scope and passes them to GameWorld, level loads, Renderer asset work, editor/tool operations. Shutdown destroys consumers before task runtime. | `S` |
| `APP-007` | Renderer execution selection | Capability-gated | `r.ThreadedRenderer` selects threaded Renderer ownership when options allow it; otherwise serial. `r.RenderPipelineDepth` accepts 0..2 and must remain below `r.MaximumFramesInFlight`, otherwise startup fatals. | `S` |
| `APP-008` | Camera input bridge | Implemented path | Keyboard/mouse input becomes a `CameraInputIntent`; runtime applies it to the active world camera. Editor supplies its viewport camera as view-only override without mutating world camera each frame. | `S` |
| `APP-009` | Viewport request/products | Implemented path | Host forwards viewport extent/mode/exposure requests and returns Renderer products for ImGui presentation. Runtime console can request UI packets too. | `S` |
| `APP-010` | Ordered shutdown | Implemented path | Console -> Renderer -> LevelSession -> input collector -> GameWorld -> Tasks -> Input -> Window -> Timer are released in an explicit order. Repeated/device-loss shutdown remains unrun. | `S` |

## Configuration And Tools

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `APP-011` | Command-line CVar assignment | Implemented path | `--cvar=name=value`, `--cvar name=value`, and `--set-cvar name=value` are case-insensitive switches. Unknown names, malformed assignments, and failed parses are silently ignored by this adapter. | `S` |
| `APP-012` | Runtime console | Implemented path | Optional tilde-driven ImGui overlay registers Core built-ins for help/list/get/set, with filtering, bounded session output, history, and autocomplete, then submits a UI packet through Renderer. No level, shader-recook, or Renderer-specific command registration was found in the runtime host. | `S` |
| `APP-013` | Editor shader recook | Capability-gated | Editor-only coordinator tracks changed sources, launches `ShaderCompiler`, supports all/selected/changed requests and cancellation, reads the publication signal, then requests Renderer generation reload. Requires tool/source workspace state. | `S` |
| `APP-014` | Editor operation service | Implemented path | Serializes/updates long-running editor operations and exposes progress/result to UI, currently including shader recook. It is not a generic plugin task framework. | `S` |
| `APP-015` | Viewport capture | Implemented path | Editor capture coordinator turns a UI request into Renderer capture work and reports completion/failure through the editor host. File correctness and UX evidence remain open. | `S` |

## Vertical Runtime Trace

Project `main` calls the editor or runtime launch function -> command-line CVars are applied -> host constructs native/application/world/render owners -> `LevelSession` begins startup activation -> each ready frame advances world systems and extracts one frame submission -> Renderer returns viewport/UI products -> close request exits Tick -> explicit shutdown unwinds owners in dependency order.

## Explicit Non-Capabilities And Risks

- No service mode, headless executable, multi-window host, suspend/resume lifecycle, crash recovery, telemetry upload, or platform abstraction beyond Windows was found.
- The CVar command-line adapter does not surface invalid/unknown assignments, so a typo can look accepted.
- Shader recook is editor/workspace tooling and must not be advertised in packaged game products.
- Source-level editor erasure is implemented, but final binary/import/file inspection has not proved a clean ShippingGame package.
