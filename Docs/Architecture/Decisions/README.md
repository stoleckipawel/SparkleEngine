# Architecture Decisions

**Status:** canonical-decision index

These documents own accepted cross-system invariants. Implementation must reconcile them with current code and executable configuration.

## How To Use A Decision

1. Read the decision before changing a named responsibility, coordinate/ABI convention, or view/scene boundary.
2. Trace the current producers and consumers in source; a record does not prove the code still conforms.
3. Apply the invariant across every affected module and delete the superseded internal path.
4. Put evolving implementation detail in the owning module/feature dossier, not back into the decision history.

The three decisions interact: world coordinates define data meaning, the Renderer/RHI boundary defines where frame policy becomes GPU mechanism, and viewport-camera ownership defines which view state may vary without mutating the scene.

## Decisions

| Decision | Responsibility |
| --- | --- |
| [Renderer And RHI Boundary](RendererRhiBoundary.md) | dependency direction, policy/mechanism split, graph responsibility, lifetime, and backend parity |
| [World Coordinate, Units, And Transform Contract](WorldCoordinateAndUnits.md) | axes, units, matrices, named spaces, import normalization, animation, and placement |
| [Editor Viewport Camera Architecture](EditorViewportCamera.md) | scene-camera versus editor-view ownership, persistence, exposure, and render-view publication |
