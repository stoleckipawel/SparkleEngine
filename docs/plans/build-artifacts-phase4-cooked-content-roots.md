# Phase 4 Cooked Content And Asset Capability Roots Handoff

Date: 2026-06-03

Scope:

- move generated cooked content into the development artifact model
- keep optional cook groups optional
- preserve launcher-only and editor-only workflows that do not need cooking
- do not assemble final `dist/` packages
- do not run final build/package validation

Implemented Cooked Roots:

| Domain | Root |
| --- | --- |
| Project cooked content | `artifacts/dev/projects/<Project>/cooked/` |
| Shared cooked content | `artifacts/dev/projects/Shared/cooked/` |
| Legacy migration fallback | `build/Cooked/<Project>/` and `build/Cooked/Shared/` |

Cooked Content Layout:

| Capability | Cooked subfolder | Launcher action |
| --- | --- | --- |
| Scene, mesh, material assets | `SceneManifests/`, `Meshes/`, `Materials/` | `Cook Scene Assets` |
| Texture assets | `Textures/` | `Cook Textures` |
| Shader packages and registry | `Shaders/Packages/`, `Shaders/ShaderPackageRegistry.sreg` | `Cook Shaders` |
| Full project cook | all cooked subfolders | `Cook All` |

Implemented Changes:

- Runtime filesystem cooked roots now resolve to `artifacts/dev/projects/<Project>/cooked`.
- Tool-side `AssetCooker` plans now write cooked content to `artifacts/dev/projects/<Project>/cooked`.
- Cook plan, summary, and temporary request diagnostics now write under `artifacts/diagnostics/cook`.
- Launcher `GetCookedProjectDirectory` now returns the project artifact cooked root.
- Launcher launch readiness checks project cooked content, shared cooked content, and legacy `build/Cooked` fallback roots.
- Direct `Cook Shaders` launcher workflows run from the selected project root so ShaderCompiler uses the same project-owned cooked root.
- AssetCooker nested tool resolution now prefers `artifacts/dev/tools/<Tool>/<Config>` with legacy `build/bin/<Config>` fallback.
- Missing launch readiness now points to exact cook actions: `Cook Scene Assets`, `Cook Textures`, or `Cook Shaders`.
- Clean scopes remove project/shared cooked roots without deleting editor/runtime artifact folders or source dependency caches.

Package-Facing Staging Draft:

- Stage project cooked content from `artifacts/dev/projects/<Project>/cooked/`.
- Stage shared cooked content from `artifacts/dev/projects/Shared/cooked/` when present.
- Preserve cooked subfolder names in packages so runtime lookup remains stable: `Shaders`, `Textures`, `SceneManifests`, `Meshes`, and `Materials`.
- Package validation should verify shader registry, shader packages, texture files, scene manifests, mesh files, and material files independently.
- Package assembly remains deferred to later phases; this phase only prepares stable source roots for staging.

Validation:

- Read-only searches were used to verify cooked roots and clean scopes.
- Final build validation was not run.
- Final package validation was not run.
