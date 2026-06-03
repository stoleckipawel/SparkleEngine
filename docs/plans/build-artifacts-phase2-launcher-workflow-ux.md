# Phase 2 Launcher Dependency And Workflow UX Handoff

Date: 2026-06-03

Scope:

- refactor launcher presentation around user workflows instead of internal helper functions
- separate installed host prerequisites from syncable source dependency groups
- make optional rebuild, recook, and package work visible without implying they are mandatory for first-run use
- keep current output roots and package assembly behavior unchanged

Implemented:

- The workflow rail now uses `Start`, `Setup`, `Build`, `Cook`, `Run`, `Package`, and `Maintenance`.
- `Start` contains primary launch actions for editor/runtime.
- `Run` contains validation and custom launch actions.
- `Setup` contains host verification, source dependency sync, project-file generation, and IDE opening.
- `Build` and `Cook` are described as optional local refresh/rebuild workflows.
- `Package` is visible as a release assembly lane but intentionally disabled until package assembly phases wire real outputs.
- Every selected workflow now shows a `Workflow Impact` group that explains what the action changes.
- `Verify Host Environment` no longer displays source dependency group inventories.
- `Sync Source Dependencies` is described as source dependency/configure work and explicitly says it does not install host tools.
- Build-file readiness now includes recovery guidance for stale generator/platform cache mismatches.
- Launch readiness now looks for `Cooked scene assets` after the Phase 1 naming change.
- Launch readiness includes a bundled runtime component state, marked as package-pending until package roots are implemented.
- Process output now uses the same `Generate project files` wording as the launcher action.

Current Workflow Contract:

| Workflow | User outcome | Primary output/state changed |
| --- | --- | --- |
| `Start` | Launch ready editor/runtime components | process launch and diagnostics |
| `Setup` | Verify machine, sync source dependencies, refresh IDE files | host diagnostics, dependency cache, generated workspace state |
| `Build` | Optional local rebuilds for development/customization | build outputs |
| `Cook` | Optional local recook of project content | cooked outputs |
| `Run` | Validation or customized project execution | process launch and diagnostics |
| `Package` | Future release assembly | package outputs, deferred |
| `Maintenance` | Clean or format generated/source state | selected generated outputs or source formatting |

Intentional Deferrals:

- Package assembly is not implemented in Phase 2.
- Final package validation was not run.
- Final build validation was not run.
- Product output roots were not migrated.
- Bundled runtime executable resolution is documented in readiness state but not activated because package roots do not exist yet.

Next-phase guidance:

- When package roots exist, launch resolution should prefer bundled editor/runtime components before local build outputs.
- Package workflows should become runnable only after manifests, dist layout, redistributables, symbols, cooked content, and validation rules exist.
- Keep host prerequisite checks and source dependency sync separate in all new launcher actions.
