# Architecture Decisions

Status: canonical-decision index

These documents own accepted cross-system invariants. Implementation must reconcile them with current code and executable configuration.

| Decision | Responsibility |
| --- | --- |
| [Renderer And RHI Boundary](RendererRhiBoundary.md) | dependency direction, policy/mechanism split, graph responsibility, lifetime, and backend parity |
| [World Coordinate, Units, And Transform Contract](WorldCoordinateAndUnits.md) | axes, units, matrices, named spaces, import normalization, animation, and placement |
| [Editor Viewport Camera Architecture](EditorViewportCamera.md) | scene-camera versus editor-view ownership, persistence, exposure, and render-view publication |
