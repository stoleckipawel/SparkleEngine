# Sparkle Launcher GUI Phase 4 Production Readiness

## Scope
Phase 4 hardens the Qt launcher for production use without adding new user-facing features, removing legacy code, packaging, or running a full build. The focus is workflow robustness, source-level QA coverage, documentation, and known limitation tracking before the final build/release checkpoint.

## Hardening Completed
- Prevented overlapping operation dispatch from the GUI by disabling preview/run controls and operation selection while a workflow is running.
- Added a guard that reports `Operation is already running` if the user attempts to preview or run during an active workflow.
- Capped the live operation output buffer at 1,000,000 characters so long tool output does not grow the text widget without bound.
- Preserved streamed output and completion status in the operation output panel.
- Kept destructive confirmation prompts and backend readiness checks in place.

## Regression Matrix
| Area | Source-Only Check | Acceptance |
| --- | --- | --- |
| Operation inventory | Phase 2 feature map lists every current CLI operation id. | No operation id is GUI-only or console-only by design. |
| Backend dispatch | GUI code calls native `Plan*Operation` and `Run*OperationPlan` APIs. | No `Sparkle.exe`, `SparkleCli`, `.bat`, `QProcess`, or `system` bridge is introduced. |
| Project selection | Project list feeds `LauncherOperationRequest::ProjectId`. | Selected project flows into previews and runs. |
| Settings parity | Settings controls map to CLI-equivalent request fields. | Profiles, targets, cook flags, validation filters, clean scope, and smoke options are preserved. |
| Error handling | Unknown operations emit preview failure; missing process runner returns failed completion. | User sees a status/output message instead of silent failure. |
| Long-running workflows | GUI disables operation controls while a workflow is active. | Users cannot start overlapping workflows from the same window. |
| Output stability | Operation output is capped. | Long logs do not grow the widget indefinitely. |
| Branding | Launcher-local stylesheet uses editor-inspired chrome without editor UI code. | Styling remains decoupled from editor implementation. |

## Known Limitations
- Qt headers and executable smoke validation are intentionally deferred to the final configure/build checkpoint.
- Phase 5 resolved the Qt SDK requirement with a local Qt 6.8.3 MSVC x64 SDK and packaged-launch smoke validation.
- Automated Qt widget tests are not yet added; the backend has a mockable `IProcessRunner` factory for future non-process workflow tests.
- Legacy console code remains in the depot until the final legacy-removal stage.

## QA Sign-Off Criteria Before Phase 5
- Source checks pass without whitespace issues.
- Forbidden console bridge grep remains clean for GUI code.
- Phase 1-4 documentation is present and linked from the roadmap.
- Final build checkpoint resolves Qt include diagnostics and validates the executable launcher.
- Legacy removal happens only after the first successful production build and release validation.