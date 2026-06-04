# Sparkle Launcher Visual Assets

This folder owns curated imagery for the launcher Home surface.

Current images are intentionally derived from Showcase source/cooked material and launcher validation captures so the NVIDIA-inspired shell has product visuals without hardcoding machine-local screenshots into the UI. They are darkened and composed to support text, CTAs, and status chips rather than compete with them.

Asset slots:

- `showcase-hero.png`: Home hero artwork.
- `showcase-editor.png`: Showcase Editor library card.
- `showcase-runtime.png`: Showcase Runtime library card.
- `showcase-content.png`: Cooked content / scene asset tile.
- `sparkle-architecture.png`: Architecture evidence tile.
- `sparkle-source-tiers.png`: Source tier evidence tile.
- `sparkle-validation.png`: Validation evidence tile.
- `sparkle-package.png`: Package evidence tile.
- `sparkle-tools.png`: Tooling evidence tile.

Replace these with framed engine renders when the engine has dedicated capture tooling or saved showcase camera presets. Keep the filenames stable so launcher code does not need to change when art improves.
