# Viewport Rendering Controls

**Status:** architecture navigation index

This folder separates the viewport's one mutually exclusive rendering mode from orthogonal contribution-visibility and shadow-evaluation controls:

- [Render View Modes](ViewModes.md) owns the implemented `RenderViewMode` contract and its request/View consumers.
- [Renderer Show Flags](ShowFlags.md) owns the target per-view lighting-contribution and shadow-evaluation contract plus its feature-named global developer gates.

Both values are Renderer semantics presented by Editor. Neither belongs to RHI, and show flags never encode a view mode.
