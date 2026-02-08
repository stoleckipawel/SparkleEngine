# Sparkle Engine — Scripts System Redesign Plan

> **Status:** ✅ Implemented — all steps completed.
> **Scope:** Full audit, redesign, and fix of every `.bat` and `.cmake` file under `Scripts/`.

---

## 1. Executive Summary

The current Scripts/ system works for the happy path but has **10 bugs** (3 critical),
**inconsistent patterns** across older vs newer scripts, and a **destructive default flow**
(`BuildSolution` nukes `build/_deps/` every time via `CleanIntermediateFiles`).

This plan proposes a redesign that follows the patterns used by Unreal Engine,
O3DE, and Godot — specifically the separation into **Setup → Generate → Build → Clean**
phases. We fix every bug, unify every convention, and end up with a system where a
first-time contributor can clone the repo and run a single script to get a working build.

---

## 2. Current State — Call Graph & Problems

### 2.1 Current Call Graph

```
User-facing scripts (run from repo root):
─────────────────────────────────────────
BuildSolution.bat ──┬── CheckDependencies.bat
                    ├── CleanIntermediateFiles.bat   ⚠ destroys build/_deps/
                    └── cmake configure              (fetches deps + generates .sln)

BuildProjects.bat ──┬── CheckThirdParty.bat ─── cmake configure (fetch-only)
                    └── Internal/BuildProjectsImpl.bat
                        └── BuildSolution.bat CONTINUE   ⚠ missing CONTINUE arg
                            (triggers full clean + regen cycle)

CheckDependencies.bat     (standalone tool check)
CheckThirdParty.bat       (standalone dep sync)
CleanIntermediateFiles.bat (standalone nuke everything)
CleanThirdParty.bat       (standalone nuke _deps/)
CreateNewProject.bat      (project scaffolding)
RunClangFormat.bat        (code formatting)

Internal/ (not user-facing):
    BootstrapLog.bat
    BuildProjectsImpl.bat
    BuildProjectsDebug.bat     ← never called by anything
    BuildProjectsRelease.bat   ← never called by anything
    BuildProjectsRelWithDebInfo.bat  ← never called by anything
```

### 2.2 Bug Inventory

| # | Severity | Script | Bug |
|:--|:---------|:-------|:----|
| 1 | **Critical** | `BuildProjects.bat` | `PROJECTS_DIR` = `%~dp0\projects` → resolves to `Scripts\projects\`. Should be `%~dp0..\Projects`. Project discovery always fails. |
| 2 | **Critical** | `CreateNewProject.bat` | `ROOT_DIR` = `%SCRIPT_DIR%` (= `Scripts\`). Template path, projects dir, and BuildSolution call all resolve incorrectly. |
| 3 | **Critical** | `BuildSolution.bat` | Always calls `CleanIntermediateFiles` which deletes `build/` (including `_deps/`). Every invocation re-downloads all third-party deps (~5-15 min). |
| 4 | **High** | `BuildProjects.bat` | `BIN_DIR` = `%~dp0bin\` → `Scripts\bin\`. Executable launch always fails. |
| 5 | **High** | `BuildProjectsImpl.bat` | Calls `BuildSolution.bat` without `CONTINUE`. When solution is missing, triggers interactive prompts inside an automated chain. |
| 6 | **Medium** | `BuildSolution.bat` | `call :FINISH 1` inside `if` blocks — subroutine call returns, execution falls through. Uses the older `call :FINISH` pattern instead of `goto :FINISH`. |
| 7 | **Medium** | `CleanIntermediateFiles.bat` | Always returns exit code 0, even when `rmdir` fails on locked files. |
| 8 | **Medium** | `CMakeLists.txt` | `CMAKE_RUNTIME_OUTPUT_DIRECTORY` uses `${CMAKE_BUILD_TYPE}` — empty for multi-config VS generator. Binaries go to `bin/` with no config subfolder. |
| 9 | **Low** | `RunClangFormat.bat` | Scans nonexistent `samples/` dir; doesn't scan `Projects/`. No BootstrapLog integration. No exit code. |
| 10 | **Low** | `CreateNewProject.bat` | No BootstrapLog integration. Calls `BuildSolution.bat` without setting `PARENT_BATCH`, triggering double-pause. |

### 2.3 Consistency Issues

| Area | Problem |
|:-----|:--------|
| **Path resolution** | Some scripts use `%~dp0` (= `Scripts\`), some use `%~dp0..` (= repo root). At least 2 scripts get the repo root wrong. |
| **BootstrapLog** | `RunClangFormat.bat` and `CreateNewProject.bat` don't use it — their output isn't captured in logs. |
| **`goto :FINISH` vs `call :FINISH`** | Newer scripts use `goto :FINISH` + `EXIT_RC` variable (correct). `BuildSolution.bat` still uses `call :FINISH 1` (broken inside `if` blocks). |
| **Dead code** | `BuildProjectsDebug/Release/RelWithDebInfo.bat` are never called. `Scripts/build/` is empty. `Scripts/Clang/.clang-tidy` is unused. Ninja cleanup code in `CleanIntermediateFiles.bat` is dead. |

---

## 3. Design Decisions

### 3.1 Follow Industry Standard Script Phases

Real engines (Unreal: `Setup.bat` → `GenerateProjectFiles.bat` → build in IDE;
O3DE: `python/get_python.bat` → `cmake --preset` → build;
Godot: `scons` handles everything) separate concerns into distinct phases:

| Phase | Our Script | Responsibility |
|:------|:-----------|:---------------|
| **Setup** | `Setup.bat` (NEW) | First-time one-shot: validate tools, fetch deps, generate solution |
| **Generate** | `GenerateProjectFiles.bat` (rename of `BuildSolution.bat`) | cmake configure only — incremental, never cleans |
| **Build** | `BuildProjects.bat` | msbuild on selected project + config |
| **Clean** | `Clean.bat` (merge of 2 scripts) | Parameterized: `--all`, `--deps`, `--build-only` |

**Why rename `BuildSolution.bat`?** Because it doesn't build anything — it generates
project files. `GenerateProjectFiles.bat` is what Unreal calls it and is self-documenting.

### 3.2 Never Clean By Default

**Current (broken):** `BuildSolution.bat` → `CleanIntermediateFiles.bat` → deletes
`build/` including `_deps/` → cmake configure re-downloads everything.

**Proposed:** `GenerateProjectFiles.bat` runs `cmake configure` incrementally.
If the user wants a clean state, they explicitly run `Clean.bat`.
This is how every real engine works — incremental by default, clean on demand.

### 3.3 Unified Path Resolution

Every script will resolve the repo root the same way:

```batch
:: Resolve repo root (parent of Scripts/)
for %%I in ("%~dp0..") do set "ROOT_DIR=%%~fI"
```

All paths derived from `ROOT_DIR`:
- `%ROOT_DIR%\build` — CMake build dir
- `%ROOT_DIR%\build\_deps` — FetchContent deps
- `%ROOT_DIR%\bin` — Output binaries
- `%ROOT_DIR%\Projects` — User projects
- `%ROOT_DIR%\Engine` — Engine source

### 3.4 Shared Config Module

Create `Internal\Config.bat` — called by every top-level script after BootstrapLog.
Sets all shared variables once:

```batch
:: Internal\Config.bat — Shared path & tool configuration
set "ROOT_DIR=..."
set "BUILD_DIR=%ROOT_DIR%\build"
set "BIN_DIR=%ROOT_DIR%\bin"
set "PROJECTS_DIR=%ROOT_DIR%\Projects"
set "DEPS_DIR=%BUILD_DIR%\_deps"
set "ENGINE_DIR=%ROOT_DIR%\Engine"
set "GENERATOR=Visual Studio 17 2022"
set "ARCH=x64"
```

**Why?** Eliminates the root cause of bugs #1, #2, #4. Single place to maintain paths.

### 3.5 Fix CMake Multi-Config Output Directory

Replace `CMAKE_BUILD_TYPE` with per-config output:

```cmake
# Before (broken for VS multi-config):
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")

# After (works for multi-config):
foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER}
        "${CMAKE_SOURCE_DIR}/bin/${CONFIG_TYPE}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER}
        "${CMAKE_SOURCE_DIR}/bin/${CONFIG_TYPE}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER}
        "${CMAKE_SOURCE_DIR}/bin/${CONFIG_TYPE}")
endforeach()
```

### 3.6 Clean Script Consolidation

Merge `CleanIntermediateFiles.bat` + `CleanThirdParty.bat` → `Clean.bat`
with a selection menu:

```
============================================================
  Clean Options
============================================================

  1) Build artifacts only   (build/, bin/, .vs/)
  2) Third-party deps only  (build/_deps/)
  3) Everything             (full clean)

============================================================
Enter choice [1-3]:
```

When called with `PARENT_BATCH`, accepts an argument: `Clean.bat BUILD` / `DEPS` / `ALL`.

### 3.7 Setup.bat — First-Time Experience

New script that provides the "clone → working build" path:

```
============================================================
  Sparkle Engine — First-Time Setup
============================================================

  [OK]   CMake
  [OK]   MSBuild
  [OK]   git
  [OK]   git-lfs
  [WARN] Clang not found (MSVC will be used)

  [LOG] Fetching third-party dependencies...
  [OK]   Dear ImGui
  [OK]   cgltf
  [OK]   stb
  [OK]   AMD Compressonator
  [OK]   KTX-Software

  [LOG] Generating Visual Studio solution...
  [SUCCESS] Setup complete.

  Next steps:
    1. Open build\Sparkle.sln in Visual Studio
    2. Set DemoProject as startup project
    3. Build and run (F5)
```

Internally: `CheckDependencies` → `CheckThirdParty` → `cmake configure`.
No cleaning. Idempotent — safe to run again.

---

## 4. Proposed Script Layout

```
Scripts/
├── Setup.bat                      NEW — first-time setup (tools + deps + generate)
├── GenerateProjectFiles.bat       RENAME from BuildSolution.bat — incremental cmake configure
├── BuildProjects.bat              FIX — correct paths, fix launch, fix FINISH pattern
├── Clean.bat                      MERGE CleanIntermediateFiles + CleanThirdParty
├── CreateNewProject.bat           FIX — correct ROOT_DIR, add BootstrapLog
├── RunClangFormat.bat             FIX — add BootstrapLog, fix scan dirs, add exit codes
├── CheckDependencies.bat          KEEP — minor fixes only
├── CheckThirdParty.bat            KEEP — already correct
├── FetchDependencies.cmake        KEEP — no changes
│
├── Internal/
│   ├── BootstrapLog.bat           KEEP — no changes
│   ├── Config.bat                 NEW — shared path/tool config
│   └── BuildProjectsImpl.bat     FIX — correct paths, fix CONTINUE arg
│
├── Clang/
│   └── .clang-tidy                MOVE to repo root (standard location) or DELETE
│
└── build/                         DELETE (empty placeholder)

Deleted:
  - Scripts/Internal/BuildProjectsDebug.bat      (dead code)
  - Scripts/Internal/BuildProjectsRelease.bat     (dead code)
  - Scripts/Internal/BuildProjectsRelWithDebInfo.bat  (dead code)
  - Scripts/Clang/.clang-tidy-ignore              (unused)
  - Scripts/CleanIntermediateFiles.bat             (merged into Clean.bat)
  - Scripts/CleanThirdParty.bat                    (merged into Clean.bat)
```

---

## 5. Redesigned Call Graph

```
Setup.bat (first-time)
  └── BootstrapLog → Config → CheckDependencies → CheckThirdParty → cmake configure

GenerateProjectFiles.bat (incremental regenerate)
  └── BootstrapLog → Config → CheckDependencies → cmake configure (no clean!)

BuildProjects.bat (interactive build)
  └── BootstrapLog → Config → CheckThirdParty → project menu → config menu
      └── Internal/BuildProjectsImpl.bat
          └── GenerateProjectFiles.bat CONTINUE  (only if .sln missing)
              └── msbuild

Clean.bat (explicit clean)
  └── BootstrapLog → Config → clean menu → rmdir

CreateNewProject.bat (project scaffolding)
  └── BootstrapLog → Config → copy template → prompt → GenerateProjectFiles.bat

RunClangFormat.bat (code quality)
  └── BootstrapLog → Config → scan Engine/ + Projects/ → clang-format

CheckDependencies.bat (standalone diagnostic)
  └── BootstrapLog → Config → tool checks → third-party status

CheckThirdParty.bat (standalone sync)
  └── BootstrapLog → Config → dep check → cmake configure (if needed)
```

### Key Improvements

1. **No script ever cleans unless explicitly asked** — incremental by default
2. **Config.bat** eliminates path duplication — single source of truth
3. **Setup.bat** gives first-timers one command to a working build
4. **GenerateProjectFiles.bat** name clearly communicates "this generates, not builds"
5. **Clean.bat** gives fine-grained control over what to clean
6. **Every script** uses BootstrapLog, `goto :FINISH`, and LOGGING_STYLE.md conventions

---

## 6. Detailed Change List

### 6.1 NEW: `Setup.bat`

| Aspect | Detail |
|:-------|:-------|
| Purpose | One-shot first-time setup: validate → fetch → generate |
| Calls | `CheckDependencies` → `CheckThirdParty` → `cmake configure` |
| Mode | Interactive only (not called by other scripts) |
| Exit code | 0 = ready to build, 1 = something failed |
| Idempotent | Yes — safe to run multiple times, skips work already done |

### 6.2 RENAME + FIX: `BuildSolution.bat` → `GenerateProjectFiles.bat`

| Change | Detail |
|:-------|:-------|
| Rename | `BuildSolution.bat` → `GenerateProjectFiles.bat` |
| Remove | `CleanIntermediateFiles.bat` call — no more destructive clean |
| Fix | `call :FINISH 1` → `goto :FINISH` with `EXIT_RC` |
| Fix | Incremental: only runs cmake if `CMakeCache.txt` is missing OR user passes `--force` |
| Keep | CONTINUE argument for child invocation |
| Keep | ClangCL detection |
| Keep | VS open prompt |

### 6.3 FIX: `BuildProjects.bat`

| Change | Detail |
|:-------|:-------|
| Fix | `PROJECTS_DIR` → `!ROOT_DIR!\Projects` (via Config.bat) |
| Fix | `BIN_DIR` → `!ROOT_DIR!\bin` (via Config.bat) |
| Fix | `call :FINISH` patterns already correct (uses `goto :FINISH`) |
| Keep | Project selection menu, config menu, launch prompt |

### 6.4 MERGE: `CleanIntermediateFiles.bat` + `CleanThirdParty.bat` → `Clean.bat`

| Change | Detail |
|:-------|:-------|
| Merge | Combine both cleanup scripts into one with a menu |
| Arguments | `BUILD` = build artifacts only, `DEPS` = third-party only, `ALL` = everything |
| Interactive | Shows menu when run standalone |
| Fix | Return proper exit codes on `rmdir` failure |
| Remove | Dead Ninja cleanup code |

### 6.5 FIX: `CreateNewProject.bat`

| Change | Detail |
|:-------|:-------|
| Fix | `ROOT_DIR` → `%~dp0..` (parent of Scripts/) |
| Fix | `TEMPLATE_DIR` → `%ROOT_DIR%\Projects\TemplateProject` |
| Fix | `PROJECTS_DIR` → `%ROOT_DIR%\Projects` |
| Fix | Call `GenerateProjectFiles.bat CONTINUE` with `PARENT_BATCH=1` |
| Add | BootstrapLog integration |
| Add | `goto :FINISH` exit pattern |

### 6.6 FIX: `RunClangFormat.bat`

| Change | Detail |
|:-------|:-------|
| Add | BootstrapLog integration |
| Fix | Scan `Engine/` + `Projects/` (remove dead `samples/` scan) |
| Add | Config.bat for paths |
| Add | `goto :FINISH` exit pattern with proper exit code |

### 6.7 FIX: `Internal\BuildProjectsImpl.bat`

| Change | Detail |
|:-------|:-------|
| Fix | Calls `GenerateProjectFiles.bat CONTINUE` (not bare invocation) |
| Fix | Sets `PARENT_BATCH=1` before calling GenerateProjectFiles |
| Keep | PROJECTS_DIR already resolves correctly from `%~dp0..\..` |

### 6.8 NEW: `Internal\Config.bat`

| Aspect | Detail |
|:-------|:-------|
| Purpose | Shared path resolution and tool detection |
| Sets | `ROOT_DIR`, `BUILD_DIR`, `BIN_DIR`, `PROJECTS_DIR`, `DEPS_DIR`, `ENGINE_DIR` |
| Sets | `GENERATOR`, `ARCH`, `USE_CLANG` |
| Called by | Every top-level script, after BootstrapLog |

### 6.9 FIX: `CMakeLists.txt`

| Change | Detail |
|:-------|:-------|
| Fix | Replace `CMAKE_RUNTIME_OUTPUT_DIRECTORY` with per-config `_${CONFIG_UPPER}` variants |
| Result | Binaries go to `bin/Debug/`, `bin/Release/`, `bin/RelWithDebInfo/` correctly |

### 6.10 DELETE: Dead Code

| File | Reason |
|:-----|:-------|
| `Internal/BuildProjectsDebug.bat` | Never called by any script |
| `Internal/BuildProjectsRelease.bat` | Never called by any script |
| `Internal/BuildProjectsRelWithDebInfo.bat` | Never called by any script |
| `Scripts/build/` | Empty directory |
| `Scripts/Clang/.clang-tidy-ignore` | Unused by any tooling |

### 6.11 MOVE: `.clang-tidy`

Move `Scripts/Clang/.clang-tidy` → repo root `.clang-tidy` (standard location,
auto-detected by clang-tidy and IDEs).

---

## 7. Flow Diagrams

### 7.1 First-Time User (Clone → Build)

```
git clone <repo>
cd LearningEngine
Scripts\Setup.bat
  │
  ├─ BootstrapLog (creates logs/logTools_*.txt)
  ├─ Config.bat   (resolve ROOT_DIR, BUILD_DIR, etc.)
  │
  ├─ CheckDependencies
  │   ├── cmake ✓ / ✗ (exit 1)
  │   ├── msbuild ✓ / ✗ (exit 1)
  │   ├── git ✓ / ✗ (exit 1)
  │   ├── git-lfs ✓ / warn
  │   └── clang ✓ / warn (MSVC fallback)
  │
  ├─ CheckThirdParty (auto-sync mode)
  │   ├── Check 5 deps → missing?
  │   │   └── cmake configure → FetchDependencies.cmake
  │   └── Re-validate → all present ✓
  │
  ├─ cmake configure (generates Sparkle.sln)
  │
  └─ [SUCCESS] Open build\Sparkle.sln → F5
```

### 7.2 Daily Development (Incremental)

```
# Solution already exists — just rebuild
Scripts\BuildProjects.bat
  │
  ├─ CheckThirdParty → all present → continue
  ├─ Project menu → select DemoProject
  ├─ Config menu → select Debug
  └─ msbuild DemoProject.vcxproj /p:Configuration=Debug
     └─ Launch DemoProject.exe? [Y/N]

# Regenerate after CMakeLists.txt change
Scripts\GenerateProjectFiles.bat
  │
  ├─ CheckDependencies → all OK
  └─ cmake configure (incremental — preserves _deps)
```

### 7.3 Clean Scenarios

```
# Clean build artifacts only (keep deps)
Scripts\Clean.bat
  └─ Select: 1) Build artifacts only
     └─ Remove: build/ (except _deps/), bin/, .vs/

# Clean deps for fresh re-download
Scripts\Clean.bat
  └─ Select: 2) Third-party deps only
     └─ Remove: build/_deps/

# Nuclear clean
Scripts\Clean.bat
  └─ Select: 3) Everything
     └─ Remove: build/, bin/, .vs/
```

---

## 8. Implementation Order

Ordered to minimize broken states — each step leaves the system functional.

| Step | Action | Risk | Depends On |
|:-----|:-------|:-----|:-----------|
| 1 | Create `Internal\Config.bat` | None | — |
| 2 | Fix `CMakeLists.txt` output directories | Low | — |
| 3 | Create `Clean.bat` (new merged script) | None | Step 1 |
| 4 | Rename `BuildSolution.bat` → `GenerateProjectFiles.bat` + fix | Medium | Steps 1, 3 |
| 5 | Fix `BuildProjects.bat` (paths, BIN_DIR) | Medium | Steps 1, 4 |
| 6 | Fix `Internal\BuildProjectsImpl.bat` (CONTINUE, paths) | Low | Steps 1, 4 |
| 7 | Fix `CreateNewProject.bat` (ROOT_DIR, BootstrapLog) | Low | Steps 1, 4 |
| 8 | Fix `RunClangFormat.bat` (BootstrapLog, scan dirs) | Low | Step 1 |
| 9 | Create `Setup.bat` | None | Steps 1, 4 |
| 10 | Delete dead code + move `.clang-tidy` | None | — |
| 11 | Update `README.md` with new script names | None | Steps 4, 9 |

---

## 9. What We're NOT Changing

| Item | Reason |
|:-----|:-------|
| `FetchDependencies.cmake` | Works correctly — stb/compressonator on master is acceptable for a learning engine |
| `BootstrapLog.bat` | Already solid — PowerShell Tee-Object pattern is correct |
| `CheckThirdParty.bat` | Already audited and tested — only needs Config.bat integration |
| `CheckDependencies.bat` | Already audited — only needs Config.bat integration |
| Batch → PowerShell migration | Batch is fine for this scope — PS would add a dependency |
| CI/CD scripts | Out of scope for this plan |

---

## 10. Success Criteria

After implementation, these scenarios must all work:

| Scenario | Expected Result |
|:---------|:----------------|
| Fresh clone → `Setup.bat` | Tools validated, deps fetched, .sln generated, exit 0 |
| `Setup.bat` run twice | Second run is nearly instant (skips existing work), exit 0 |
| `GenerateProjectFiles.bat` (no build/) | Creates build/, runs cmake, generates .sln, exit 0 |
| `GenerateProjectFiles.bat` (with build/) | Incremental cmake reconfigure, preserves _deps, exit 0 |
| `BuildProjects.bat` → Debug → DemoProject | Builds successfully, offers launch, exit 0 |
| `BuildProjects.bat` → All configs → All projects | All 3 configs built, exit 0 |
| `Clean.bat` → Build artifacts only | Removes build/ (sans _deps/), bin/, .vs/ — deps preserved |
| `Clean.bat` → Third-party only | Removes build/_deps/ only |
| `Clean.bat` → Everything | Removes build/, bin/, .vs/ entirely |
| `CreateNewProject.bat MyGame` | Creates Projects/MyGame from template, offers regen |
| `RunClangFormat.bat` | Scans Engine/ + Projects/, logs output, proper exit code |
| All scripts log to `logs/logTools_*.txt` | Including CreateNewProject, RunClangFormat |

---

## 11. Open Questions

1. **Keep `BuildSolution.bat` as a thin wrapper?** We could keep a `BuildSolution.bat`
   that just calls `GenerateProjectFiles.bat` with a deprecation notice. This avoids
   breaking muscle memory. Recommendation: **Yes, keep a 3-line wrapper for one release cycle.**

2. **`Clean.bat` option for "Build Artifacts Only" — include or exclude `build/` cmake cache?**
   Option A: Remove everything in `build/` except `_deps/` (forces complete regen).
   Option B: Remove only `bin/`, `.vs/`, leave `build/` intact (incremental cmake still works).
   Recommendation: **Option A** — "clean build" means clean. User runs `GenerateProjectFiles.bat` after.

3. **Should `Setup.bat` set `PARENT_BATCH` when calling children?**
   Yes — `Setup.bat` is a pipeline, children shouldn't pause individually.

---

*Awaiting approval — no code changes will be made until this plan is reviewed.*
