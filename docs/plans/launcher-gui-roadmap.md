# Sparkle Launcher GUI Frontend Plan

## Overview
Design and implement a modern Windows-only GUI frontend for the Sparkle Launcher, inspired by Unity Hub and Epic Launcher. The new launcher will fully replace the current console app for user-facing workflows, providing a seamless, user-friendly experience. The GUI should be visually inspired by the Sparkle Engine Editor UI, but codebases must remain decoupled.

---

## 1. Platform & Technology
- **Target OS:** Windows only
- **GUI Framework:** Qt 6.8 LTS or newer, using Qt Widgets
- **Distribution:** Ship as a ready-to-use .exe within the same depot as the engine

## 2. Features & Parity
- **Goal:** Full feature parity with the current console launcher
- **Console Replacement:** All user interactions and workflows currently handled by the console app must be available in the GUI
- **No Console Exposure:** Users should not see or interact with the console app; all operations are managed within the GUI

## 3. User Experience & Design
- **Design Language:** Visually inspired by the Sparkle Engine Editor UI (colors, layout, iconography), but do not share code
- **Project Management:** Use the existing launcher’s project/workspace management model
- **Navigation:** Simple, intuitive navigation modeled after Unity Hub/Epic Launcher (project list, actions, settings, etc.)
- **No Account/Cloud:** No user authentication or cloud sync required (unless future requirements change)

## 4. Integration & Architecture
- **Backend Integration:** Re-implement or wrap all console logic as internal modules/libraries, not by shelling out to the console app
- **No Console Window:** All operations should be performed natively in the GUI process
- **Extensibility:** Design for future extensibility (e.g., plugin support), but not required for v1

## 5. Distribution & Updates
- **Deployment:** Distribute as a standalone .exe in the depot
- **Updates:** No auto-update required for v1; updates handled via depot refresh

## 6. Branding & Customization
- **Branding:** Match the Sparkle Engine Editor’s look and feel, but do not couple codebases
- **Customization:** Allow for future theming/branding, but not required for v1

---

## Open Questions / Next Steps

---

## Decisions
- **GUI Framework:** Qt (Windows only)
- **Design References:** Epic Launcher & Unity Hub (UI/UX inspiration)
- **Goal:** Full replacement of the console app with a production-ready, fully functional GUI launcher for wide usage

---

## Phased Implementation Plan


### Phase 1: Foundation & Architecture
**Implementation Prompts:**
- Select and document the Qt version and project structure.
- Define core modules: project management, settings, backend integration (list responsibilities for each).
- Establish a communication layer for backend logic (wrap/port console logic as Qt modules, do not shell out).
- Implement a basic window, navigation, and placeholder UI (inspired by Unity Hub/Epic Launcher).

**Positive Guiderails:**
- Favor clear module boundaries and single-responsibility classes.
- Use Qt idioms for UI and event handling.
- Document all architectural decisions and rationale.

**Negative Guiderails:**
- Do not couple code with the Editor UI; only use visual inspiration.
- Do not shell out to the console app for backend logic.
- Avoid premature optimization or feature creep.

**Validation & Acceptance Criteria:**
- All core modules are stubbed with clear interfaces and responsibilities.
- Placeholder UI launches and navigates between main sections.
- All architectural decisions are reviewed and documented.
- No build or integration with legacy code at this stage.

**Status:** Implemented source-only scaffold. See [launcher-gui-phase1-architecture.md](launcher-gui-phase1-architecture.md).

---

### Phase 2: Feature Parity & Backend Integration
**Implementation Prompts:**
- Inventory all console launcher features and map to Qt backend modules.
- Implement project/workspace management UI and backend logic.
- Integrate all user workflows (build, run, cook, etc.) as native Qt actions.
- Add error handling, progress reporting, and user feedback mechanisms.

**Positive Guiderails:**
- Ensure every console feature is mapped and tested in the GUI.
- Use Qt signals/slots for workflow orchestration.
- Validate backend logic with unit tests (no full build yet).

**Negative Guiderails:**
- Do not expose or invoke the console app.
- Avoid UI-only implementations for backend features.
- Do not build or package the app yet.

**Validation & Acceptance Criteria:**
- All console features are available and testable in the GUI (via mocks or unit tests).
- Backend logic is fully decoupled from the console app.
- Feature inventory is checked off and reviewed.
- No build or distribution performed at this stage.

**Status:** Implemented source-only backend integration. See [launcher-gui-phase2-feature-map.md](launcher-gui-phase2-feature-map.md).

---

### Phase 3: UI/UX Polish & Design Consistency
**Implementation Prompts:**
- Apply Sparkle Editor-inspired visual style (colors, icons, layout) to all UI elements.
- Refine navigation, dialogs, and user flows (reference Unity Hub/Epic Launcher).
- Add tooltips, help, and onboarding elements.
- Conduct usability testing and iterate on feedback.

**Positive Guiderails:**
- Match the Editor’s look and feel without code sharing.
- Prioritize clarity, accessibility, and user feedback.
- Document all UI/UX decisions and test results.

**Negative Guiderails:**
- Do not introduce new features not present in the console app.
- Avoid over-customization or deviation from established design references.
- No build or packaging at this stage.

**Validation & Acceptance Criteria:**
- UI matches design references and passes usability review.
- All dialogs, flows, and help elements are present and functional.
- UI/UX documentation is complete.
- No build or distribution performed at this stage.

**Status:** Implemented source-only UI polish. See [launcher-gui-phase3-ux-review.md](launcher-gui-phase3-ux-review.md).

---

### Phase 4: Production Readiness & QA
**Implementation Prompts:**
- Perform full regression and integration testing (manual and automated).
- Address performance and stability issues.
- Prepare documentation for users and maintainers.
- Finalize branding and review for consistency.

**Positive Guiderails:**
- Ensure all workflows are robust and error-tolerant.
- Validate with real-world project scenarios.
- Document all known issues and limitations.

**Negative Guiderails:**
- Do not remove legacy code or perform builds until all tests pass.
- Avoid last-minute feature additions.

**Validation & Acceptance Criteria:**
- All tests pass and QA sign-off is obtained.
- Documentation is complete and reviewed.
- Branding is finalized and consistent.
- No build or legacy code removal at this stage.

**Status:** Implemented source-only production readiness hardening. See [launcher-gui-phase4-production-readiness.md](launcher-gui-phase4-production-readiness.md).

---

### Phase 5: Build, Release & Legacy Removal
**Implementation Prompts:**
- Perform the first full build of the launcher as a standalone .exe.
- Package for depot distribution.
- Remove all legacy console app code and references.
- Ship production-ready launcher to depot.
- Gather user feedback and monitor for issues.

**Positive Guiderails:**
- Ensure build is reproducible and matches all acceptance criteria.
- Validate that no legacy code remains.
- Plan for future extensibility (plugin support, theming, etc.).

**Negative Guiderails:**
- Do not ship if any acceptance criteria from previous phases are unmet.
- Avoid partial or incremental legacy code removal.

**Validation & Acceptance Criteria:**
- Launcher builds and runs as a standalone .exe.
- All legacy console code is removed.
- Launcher is distributed and available in the depot.
- User feedback is collected and tracked for future improvements.

**Status:** Implemented executable build, package output, and legacy console removal. See [launcher-gui-phase5-release.md](launcher-gui-phase5-release.md).

**Redesign Note:** The first production layout felt too complex in practice. A simpler one-screen redesign has been drafted in [launcher-gui-simplified-redesign.md](launcher-gui-simplified-redesign.md) before changing Qt code again.

---

*Last updated: May 23, 2026*