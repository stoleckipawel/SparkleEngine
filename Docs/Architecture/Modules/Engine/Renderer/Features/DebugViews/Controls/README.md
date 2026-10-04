# Viewport Rendering Controls

**Status:** architecture navigation index

This folder separates the viewport's one mutually exclusive rendering mode from the Editor frontend for shared lighting-feature execution controls:

- [Render View Modes](ViewModes.md) owns the implemented `RenderViewMode` contract and its request/View consumers.
- [Lighting Show Menu And Feature Execution Controls](ShowFlags.md) owns the target Editor-to-CVar interaction, feature-local activation, disabled-work removal, and product/history contract.

`RenderViewMode` is per-view Renderer state. Show is Editor presentation of process-global feature CVars, not a Renderer flag set or request/View field. Neither adds RHI policy, and the menu never encodes a view mode.
