# Editor Viewport Camera Architecture

Status: canonical architecture decision

Scope: the editor viewport camera, navigation preferences, projection, exposure overrides, and the effective render-view boundary

## Decision

Sparkle has separate authored scene-camera and editor-viewport-camera owners. Ordinary editor navigation changes only the viewport session. It never edits a camera actor, creates an undo transaction, or dirties level data.

```text
InputSystem device state
          |
          v
CameraInputIntentCollector -- raw CameraInputIntent
          |
          +--> GameWorld ---------------- runtime navigation settings
          |
          `--> EditorViewportSession ----- Saved/Config/EditorViewport.ini
                        |
                        | immutable RenderCameraData
                        v
              Renderer camera and viewport presentation
```

The editor starts its free view from the active scene camera when a world generation is first observed. After that initialization, the free view has its own position, yaw, pitch, projection, and orthographic height. A world-generation change initializes a new free view from the new world.

## Ownership

| Concern | Mutable owner | Persistence |
| --- | --- | --- |
| Authored camera transform, vertical FOV, near/far planes, projection, and active state | GameFramework scene camera | Level data |
| Free editor view pose | `EditorViewportSession` | Current editor session |
| Viewport move speed, rotation speed, invert-Y, projection, orthographic height, and exposure overrides | Editor viewport settings | Per-workspace user file under `Saved/Config/EditorViewport.ini` |
| Runtime navigation policy | `GameWorld` | Runtime world session only |
| Effective render camera | Immutable `RenderCameraData` value | One submitted frame |
| Renderer display defaults and tone mapper | Renderer settings | Existing renderer-settings owner |
| Per-property exposure deviations | `ViewportExposureOverrides` on the viewport request | Editor viewport settings |

`CameraInputIntentCollector` owns only device-state collection and emits raw semantic intent. It does not own move speed, sensitivity, inversion, acceleration, or any mutation policy.

The viewport header is a projection of `EditorViewportSession`; it owns no camera truth. `ViewportTopPanel` only composes toolbar capabilities, `ViewportCameraProperties` owns the camera-property widgets, and `ExposureSettingsEditor` owns the shared exposure editing widgets. These presentation types mutate their existing settings owners rather than becoming new state owners. The Renderer consumes only the resolved camera and exposure values and has no Editor dependency. GameFramework does not depend on Renderer or Editor.

## Navigation And Projection

`CameraNavigation` owns shared pose math for runtime and editor navigation. The consuming session supplies immutable navigation settings: `GameWorld` for runtime and `EditorViewportSession` for editor navigation. `CameraInputIntent` contains only transient input samples. Navigation settings never become authored scene-camera component data.

Perspective uses the viewport camera's vertical FOV and actual viewport aspect ratio. Orthographic uses a viewport-owned world-space height and derives width from that aspect ratio. Switching projection does not mutate the scene camera.

The current editor exposes Perspective and Orthographic. Axis-aligned top, bottom, front, back, left, and right views; camera piloting; actor preview; bookmarks; and free-view pose restoration across process sessions are not part of this decision.

## Exposure And Presentation

Every viewport exposure field has an explicit override bit. An unchecked field resolves from the current renderer setting on each frame; checking it snapshots the displayed renderer default and makes the viewport value authoritative for that field. Manual and automatic modes share this resolution path.

The exposure metering method changes frame-graph topology. A change to its effective value rebuilds frame execution and resets temporal state. Other exposure values and the tone mapper are pass constants and resolve without a topology rebuild.

Viewport scene color is a display product, so all viewports run tone mapping and output encoding. Presentation to the host backbuffer adds only the final copy. An embedded editor viewport must never publish HDR scene color while labelling it as an LDR display product.

Tone mapping remains a renderer default rather than a camera override. Renderer tone-mapper changes are nevertheless visible in the editor because the editor consumes the same tone-mapped, output-encoded presentation product as runtime.

## Persistence And Clean Break

Navigation preferences and viewport exposure state are not serialized into camera actors or level descriptions. `MoveSpeed` was removed from the current level representation, project level files, ECS camera storage, world read/edit contracts, and scene-camera inspector. Old generated/local data is regenerated under the repository's clean-break policy; there is no migration reader, alias, or dual representation.

`Saved/Config/EditorViewport.ini` is disposable local editor state. Failure to read it uses sanitized defaults. Failure to write it leaves the accepted in-memory session state active and does not affect level saving.

## Cost Model

The CPU path performs one bounded camera-value update and one constant-size exposure-resolution operation per editor frame. Persistence writes occur only when a viewport property changes. No world scan, allocation proportional to scene size, or extra renderer synchronization is introduced.

The editor now executes the existing tone-mapping and output-encoding passes that its display product requires. This is additional editor GPU work relative to the incorrect raw-HDR publication path, but it reuses the runtime presentation path and adds no duplicate passes to runtime or a second presentation subsystem.

## Enforcement And Review

- Scene camera serialization and editor inspection must not regain navigation speed or sensitivity fields.
- Input collection must remain policy-free; runtime policy belongs to `GameWorld` and editor policy belongs to `EditorViewportSession` and its settings owner.
- Free editor navigation must not submit world-edit commands or publish camera input to the active scene camera.
- `ViewportTopPanel` must remain a capability orchestrator and must not implement camera-property or exposure widgets.
- The effective editor camera must use the requested viewport extent rather than the host-window aspect ratio.
- Exposure overrides must remain typed viewport request data; panels must not write renderer CVars to simulate a local override.
- Renderer presentation must publish tone-mapped, output-encoded scene color for both embedded and backbuffer viewports.
- Changes across Renderer/RHI boundaries still require `architecture_boundary_check`; this decision does not relax the canonical [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md).
