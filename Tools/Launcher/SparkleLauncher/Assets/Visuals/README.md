# Sparkle Launcher Visual Assets

This folder owns curated imagery for the launcher Home, product cards, and contextual workflow surfaces.

Current images are intentionally derived from Showcase source/cooked material and launcher validation captures so the NVIDIA-inspired shell has product visuals without hardcoding machine-local screenshots into the UI. The launcher applies crop, fade, and accent overlays at render time so images support text, CTAs, and status chips rather than compete with them.

Home asset slots:

- `showcase-hero.png`: Home hero artwork.
- `showcase-editor.png`: Editor product card, profiler-focused at the product card ratio.
- `showcase-runtime.png`: Runtime product library card.
- `showcase-content.png`: Cooked content / scene asset tile.
- `sparkle-architecture.png`: Architecture evidence tile.
- `sparkle-source-tiers.png`: Source tier evidence tile.
- `sparkle-validation.png`: Validation evidence tile.
- `sparkle-package.png`: Package evidence tile.
- `sparkle-tools.png`: Tooling evidence tile.

Workflow banner asset slots:

- `workflow-home-quickstart.png`: Quick Start contextual banner.
- `workflow-editor-open.png`: Open Editor workflow banner.
- `workflow-runtime-open.png`: Open Runtime workflow banner.
- `workflow-project-run-editor.png`: Launch Project workflow banner when the selected target is Editor.
- `workflow-project-run-runtime.png`: Launch Project workflow banner when the selected target is Runtime.
- `workflow-open-ide.png`: Open IDE workflow banner.
- `workflow-toolchain-check.png`: Host readiness workflow banner.
- `workflow-source-sync.png`: Source tier sync workflow banner.
- `workflow-generate-build-files.png`: Generate build files workflow banner.
- `workflow-build-all.png`: Build all workflow banner.
- `workflow-launcher-build.png`: Launcher build workflow banner.
- `workflow-editor-build.png`: Editor build workflow banner.
- `workflow-runtime-build.png`: Runtime build workflow banner.
- `workflow-cook-tools.png`: Cook tool build workflow banner.
- `workflow-cook-all.png`: Full content cook workflow banner.
- `workflow-cook-shaders.png`: Shader cook workflow banner.
- `workflow-cook-textures.png`: Texture cook workflow banner.
- `workflow-cook-assets.png`: Scene asset cook workflow banner.
- `workflow-smoke-test.png`: Smoke validation workflow banner.
- `workflow-format-check.png`: Format check workflow banner.
- `workflow-package-release.png`: Release package workflow banner.
- `workflow-clean-workspace.png`: Clean workspace workflow banner.
- `workflow-build-generic.png`: Generic build fallback banner.
- `workflow-cook-generic.png`: Generic cook fallback banner.
- `workflow-fallback-tools.png`: Unknown workflow fallback banner.

Each referenced image filename is intentionally unique. If a workflow needs new artwork, add a new file rather than reusing one of the slots above.

Replace these with framed engine renders when the engine has dedicated capture tooling or saved showcase camera presets. Keep the filenames stable so launcher code does not need to change when art improves.
