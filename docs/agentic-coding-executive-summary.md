# Agentic Coding Executive Summary

Date: 2026-06-05

## Purpose

This document defines the Codex-first agentic coding foundation for SparkleEngine. The goal is to stop relying on one-off prompting and instead give Codex durable context, repeatable workflows, reliable verification, and clear quality gates.

This is not a recommendation to add every possible agent file immediately. The strongest setup is layered: small permanent guidance first, then reusable workflows, then tool integrations, then automation.

## Executive Summary

SparkleEngine should standardize on OpenAI Codex structures only:

- `AGENTS.md` for persistent repository instructions.
- `.agents/skills/<skill>/SKILL.md` for repeatable Sparkle workflows.
- `.codex/config.toml` only for trusted project-scoped Codex settings.
- User-level `~/.codex/config.toml` for personal model, auth, provider, notification, and private MCP setup.
- MCP servers for external systems such as OpenAI docs, current library docs, GitHub, Sentry, Figma, browser automation, or other trusted development context.
- Hooks only after local validation commands are stable.
- Subagents only when parallel specialist review or investigation is useful.

Do not add `CLAUDE.md`. If Claude is not part of the workflow, it becomes another stale instruction surface. Codex should have one canonical source of repo truth.

Because SparkleEngine may later be reviewed by professional graphics engineers, including people from GPU vendors such as NVIDIA or AMD, the AI setup must stay disciplined. Agentic tooling should look like a small quality system around the engine, not a pile of assistant-specific files. Every checked-in Codex artifact should be useful to a human reviewer, explainable in one sentence, and connected to build quality, code review quality, or workflow repeatability.

The immediate next move is:

1. Add root `AGENTS.md`.
2. Fix or restore local validation scripts referenced by CI.
3. Add the first repo skill: `sparkle-build-verify`.
4. Add launcher, shader/cook, import, and RHI skills only after they repeat across several real tasks.
5. Add MCP integrations for docs and external project systems at the user level first.

## AI Setup Clutter Policy

The repository should keep the AI surface intentionally small:

- One root `AGENTS.md`.
- A small number of domain skills under `.agents/skills`.
- Optional `.codex/config.toml` only when project-scoped configuration is truly shared.
- No checked-in personal prompts, scratch chats, model experiments, API keys, private MCP credentials, or vendor-specific hype files.
- No duplicated instruction files for tools that are not used.

Every AI-related file should answer:

- What problem does this solve?
- Who maintains it?
- When should Codex use it?
- What command, quality gate, or review behavior does it improve?

If a file cannot answer those questions, it should not be checked in.

Recommended maximum visible structure:

```text
AGENTS.md
.agents/
  skills/
    sparkle-build-verify/
      SKILL.md
    launcher-ui-pass/
      SKILL.md
    shader-cook-pipeline/
      SKILL.md
    rhi-change-review/
      SKILL.md
.codex/
  config.toml        # optional, only for shared trusted project wiring
```

Avoid creating:

- `prompts/`
- `ai-notes/`
- `copilot-instructions.md` unless actually used
- `CLAUDE.md`
- per-task prompt dumps
- experimental agent plans at repo root
- generated chat transcripts

Planning documents belong under `docs/plans/` only when they describe real engineering decisions.

## Professional Review Standard

For a future external review, the repository should communicate:

- The engine has clear module boundaries.
- Graphics/RHI work is validated by reproducible commands.
- Generated artifacts are separated from source.
- AI tools are used to reinforce engineering discipline, not replace it.
- Vendor-facing code paths avoid casual assumptions about D3D12, Vulkan, shader reflection, descriptor lifetimes, synchronization, memory ownership, and backend parity.
- Documentation explains decisions without turning into marketing language.

Codex setup should therefore optimize for:

- repeatable builds,
- narrow changes,
- explicit verification,
- high-signal reviews,
- low instruction drift,
- no secret leakage,
- no repo-root clutter.

This is especially important for RHI, Renderer, shader compiler, and cook pipeline work, where a professional reviewer will care less about "AI assisted" and more about whether the contracts are precise, testable, and maintainable.

## Target Role Alignment

The Codex setup should also support a specific professional trajectory: senior graphics/rendering engineering work at the boundary between game engines, GPU hardware, graphics APIs, shader performance, profiling, and cross-team technical communication.

The target role characteristics are:

- Strong C/C++ and modern rendering engineering.
- Direct3D 12 and Vulkan fluency.
- Shader authoring and optimization in HLSL, GLSL, Slang, or similar languages.
- Low-level GPU performance analysis, including command submission, memory behavior, synchronization, occupancy, wave/wrap behavior, cache pressure, and bandwidth.
- Ability to diagnose complex graphics issues with tools such as PIX, RenderDoc, Radeon GPU Profiler, Nsight Graphics, or equivalent.
- Awareness of GPU driver architecture and hardware/software boundaries.
- Experience with rasterization, ray tracing, global illumination, physically based rendering, and modern real-time graphics algorithms.
- Ability to prototype advanced rendering techniques and measure visual-quality/performance tradeoffs.
- Ability to build developer-facing tools and plugins that make graphics workflows easier.
- Ability to communicate technical findings clearly to engine teams, hardware architects, partner studios, and reviewers.
- Growing awareness of ML/neural rendering workloads, GPU kernels, and inference-oriented optimization where relevant.

This translates into concrete repository expectations:

- RHI and Renderer work should make API contracts explicit, not implicit.
- Shader compiler and cook pipeline work should preserve reflection, binding, cache, and package correctness.
- Performance-sensitive changes should include a hypothesis, measurement plan, and result summary when possible.
- New rendering features should document which pipeline stage they affect: import, cook, shader compile, RHI resource setup, renderer scheduling, runtime scene state, or editor tooling.
- Backend-facing changes should state whether they affect D3D12, Vulkan, or shared abstractions.
- Debugging and validation tools should be treated as first-class engineering outputs, not side utilities.
- Documentation should explain tradeoffs in reviewer language: cost, memory, bandwidth, synchronization, quality, portability, and failure modes.

### Setup Rules For This Goal

`AGENTS.md` should teach Codex to bias toward hardware-aware engineering:

```md
For RHI, Renderer, shader compiler, and cook pipeline changes, identify the affected GPU/API contract before editing. State whether the change touches D3D12, Vulkan, shared RHI abstractions, shader reflection, cooked assets, or runtime scene data. Prefer explicit validation and measured claims over intuition.
```

The first domain skills should reinforce this target:

- `sparkle-build-verify`: keeps basic engineering quality tight.
- `rhi-change-review`: checks API/backend parity, synchronization, resource lifetime, descriptor ownership, and debug validation risks.
- `shader-cook-pipeline`: checks shader reflection, binding layouts, package outputs, cache behavior, and recook correctness.
- `rendering-performance-pass`: future skill for profiling-driven optimization, including hypothesis, capture plan, metrics, and before/after notes.
- `developer-tooling-pass`: future skill for launcher/tools improvements that make engine workflows easier for other developers.

The setup should avoid making unmeasured performance claims. If Codex changes performance-sensitive code and cannot run profiling tools, it should say so and provide a measurement plan rather than declaring a speedup.

### Evidence To Build Over Time

For a GPU-vendor-grade review, the repo should gradually accumulate evidence in source, tests, docs, or diagnostics:

- Reproducible build and smoke validation commands.
- RHI validation paths that catch bad resource, descriptor, pipeline, and synchronization usage.
- Shader compiler diagnostics that explain reflection/binding/package failures clearly.
- RenderDoc/PIX/RGP/Nsight capture notes for important rendering or performance changes, when available.
- Small technical writeups for complex features: problem, constraints, implementation, validation, and tradeoffs.
- Developer tools that reduce friction: launcher workflows, cook/build/test commands, diagnostics, artifact discovery.
- Clean separation between source assets, cooked assets, runtime data, and generated diagnostics.

Codex should be used to strengthen these evidence trails. The point is not to make the repo look AI-assisted; the point is to make the repo easier for a serious graphics engineer to inspect, trust, and extend.

### Goal Translation Matrix

Use this matrix when deciding whether a new Codex rule, skill, script, or document belongs in the repo.

| Target characteristic | What SparkleEngine should demonstrate | Codex setup translation |
| --- | --- | --- |
| Strong C/C++ engineering | Clear ownership, explicit lifetimes, narrow headers, stable module boundaries, readable CMake targets | `AGENTS.md` should require narrow edits, respect existing style, avoid broad refactors, and preserve module layering |
| D3D12/Vulkan API fluency | Shared RHI contracts that do not hide backend-specific correctness risks | `rhi-change-review` should check backend parity, synchronization, descriptor/resource lifetime, and API-specific assumptions |
| Shader expertise | Shader source, reflection, parameter structs, cooked packages, and runtime bindings stay consistent | `shader-cook-pipeline` should verify reflection, binding layouts, cache keys, package outputs, and recook behavior |
| GPU performance analysis | Performance claims are tied to captures, counters, frame timing, memory/bandwidth reasoning, or a measurement plan | `rendering-performance-pass` should require hypothesis, capture plan, metrics, before/after notes, and uncertainty when profiling was not run |
| Hardware-aware optimization | Code discusses waves/warps, occupancy, bandwidth, cache locality, barriers, async work, and CPU/GPU submission costs where relevant | RHI/Renderer guidance should ask Codex to identify the likely hardware bottleneck before proposing an optimization |
| Graphics debugging | Failures produce actionable diagnostics, not silent fallbacks or vague logs | Skills should ask for diagnostic messages, validation paths, artifact paths, and reproduction commands |
| Advanced rendering techniques | Features are introduced with clear pipeline ownership and tradeoffs | Feature docs should include problem, constraints, implementation, validation, quality/performance tradeoffs, and known limits |
| Developer tooling | Tools reduce friction for build, cook, launch, inspect, validate, and package workflows | `developer-tooling-pass` should prioritize workflow reliability, logs, recovery paths, and clear user-facing operations |
| Cross-team communication | Technical decisions are explainable to engine programmers, rendering specialists, tools engineers, and external reviewers | Codex final responses and docs should state what changed, why, how it was verified, and what risk remains |
| ML/neural rendering awareness | The repo is prepared to reason about GPU kernels, inference workloads, data movement, and shader/compute tradeoffs when those features appear | Future skills should separate graphics rendering, compute kernels, ML model integration, and runtime inference validation |

### Codex Prompting Defaults For This Goal

When asking Codex to work on SparkleEngine, prompts should usually include:

- The subsystem: RHI, Renderer, ShaderCompiler, Cook pipeline, Launcher, Import, Application, or Project.
- The goal: correctness, performance, visual quality, tooling, cleanup, review, or documentation.
- The validation expectation: build target, smoke test, cook command, capture plan, or docs-only.
- The risk tolerance: exploratory prototype, production-quality implementation, review-only, or minimal fix.
- The evidence wanted: code diff only, measurements, screenshot, capture notes, diagnostics, or design note.

Recommended prompt shape:

```text
Work in <subsystem>. Goal: <correctness/performance/tooling/etc>.
Keep changes production-quality and narrow. Respect Sparkle module boundaries.
Before editing, identify affected contracts and validation.
After editing, run the smallest meaningful validation or explain why it was not run.
Report changed files, verification, and remaining risk.
```

For performance-sensitive work:

```text
Do not claim a speedup without measurement. If profiling tools are unavailable, provide a capture/measurement plan and state the expected bottleneck as a hypothesis.
```

For RHI/Renderer work:

```text
Call out whether this affects D3D12, Vulkan, shared RHI abstractions, shader-visible layouts, synchronization, resource lifetime, descriptor ownership, or cooked runtime data.
```

### Setup Decision Rules

Use these rules to keep the Codex setup efficient and uncluttered:

- Put always-on engineering standards in root `AGENTS.md`.
- Put subsystem-specific always-on standards in nested `AGENTS.md` only after repeated mistakes.
- Put repeatable procedures in `.agents/skills`.
- Put mechanical validation in scripts, then call those scripts from skills.
- Put personal preferences and credentials in user-level Codex config, never the repo.
- Put current external knowledge behind MCP, not copied notes.
- Put one-off reasoning in the current conversation, not checked-in prompt files.
- Promote a lesson into repo guidance only after it prevents future mistakes.

### Review-Ready Behavior Rules

Codex should behave as if a senior graphics engineer may review every significant change:

- Prefer precise technical language over broad claims.
- State assumptions explicitly.
- Separate correctness, performance, visual quality, and tooling concerns.
- Identify affected pipeline stages before changing cross-cutting systems.
- Preserve debug and validation surfaces when refactoring.
- Avoid hiding backend-specific behavior behind vague abstractions.
- Leave a reproduction path for nontrivial bugs and performance issues.
- Treat logs, diagnostics, and artifact paths as part of developer experience.

## Current Repository Shape

SparkleEngine is a custom C++20 game/renderer engine built with CMake. The repository has about 1,782 files under `Engine`, `Tools`, and `Projects`, including about 919 C++ headers and sources.

The main working areas are:

- `Engine/`: layered engine modules such as Core, Platform, RHI, Renderer, GameFramework, Editor, and Application.
- `Tools/`: developer tools, content cooking, import/conversion tools, shader compiler, and Qt-based launcher.
- `Projects/Showcase/`: runnable sample/editor project and content.
- `CMake/`: build profiles, dependency fetches, artifact layout, Qt discovery, and release assembly.
- `docs/plans/`: active design and implementation planning notes.

This is not a simple app repo. Agentic coding here must respect module boundaries, generated artifacts, custom build profiles, third-party dependency fetching, graphics backends, content pipelines, Qt UI, and Windows toolchain realities.

## Why The Current Workflow Leaves Power On The Table

Right now, Codex has to rediscover the same facts every session:

- Sparkle uses CMake with custom profiles: `DevelopmentEditor`, `DevelopmentGame`, `DebugEditor`, `DebugGame`, `ShippingEditor`, `ShippingGame`.
- The default profile is `DevelopmentEditor`.
- Qt 6.8 Widgets is required for `SparkleLauncher`.
- The shader compiler depends on Vulkan SDK DXC and Slang locations.
- Formatting is governed by `.clang-format`.
- Static analysis policy is in `.clang-tidy`, with warnings as errors.
- Launcher UI work is concentrated under `Tools/Launcher/SparkleLauncher`.
- Build and cook workflows are product features, not incidental scripts.
- Generated folders such as `build`, `cmake-build-debug`, `artifacts`, `dist`, and `logs` should be treated differently from source.

Repeated rediscovery wastes context and increases drift. The fix is not better prompting; it is a Codex-native workspace contract.

## Codex Operating Stack

### 1. `AGENTS.md`: Repository Contract

Create this first. OpenAI documents `AGENTS.md` as Codex's durable project guidance. Codex reads global guidance from the Codex home directory and repository guidance from the project root down to the current working directory. Nested files can specialize behavior for subtrees.

Root `AGENTS.md` should include:

- Repository map.
- Source vs generated folders.
- Build profiles.
- Preferred configure/build commands.
- Formatting and static analysis expectations.
- Module boundary rules.
- Launcher UI rules.
- Shader/cooking/import pipeline rules.
- Verification ladder for small, medium, and risky changes.
- Dirty worktree discipline.

Keep it compact. Target 100-180 lines. If it grows into a manual, split specialized rules into nested `AGENTS.md` files or skills.

Recommended nested guidance later:

- `Engine/AGENTS.md`: module layering, ABI/export macros, runtime constraints.
- `Engine/RHI/AGENTS.md`: D3D12/Vulkan contract rules, descriptor/resource lifetime checks.
- `Tools/Launcher/AGENTS.md`: Qt UI/product workflow rules.
- `Tools/Shaders/AGENTS.md`: shader compiler, reflection, cook/package boundaries.
- `Tools/Import/AGENTS.md`: source import vs cook/runtime ownership.

Do not add nested files until repeated mistakes prove they are useful.

### 2. `.agents/skills`: Repeatable Workflows

OpenAI documents skills as reusable workflow packages with `SKILL.md`, optional scripts, optional references, and optional assets. Codex can discover skills by metadata and load the full instructions only when needed, which keeps everyday context smaller.

Recommended first skills:

- `sparkle-build-verify`: choose and run the right CMake configure/build/format/cook validation for a change.
- `launcher-ui-pass`: Qt launcher UI workflow, screenshot/evidence expectations, and product-system rules.
- `shader-cook-pipeline`: shader compiler, reflection, cache, and Showcase cook validation.
- `gltf-import-feature`: source import to cook to runtime feature workflow.
- `rhi-change-review`: RHI/Renderer review checklist for backend parity, lifetime, and ABI risks.

Keep this list capped. Add a new skill only when the workflow has repeated enough that it is cheaper to maintain the skill than to explain the workflow in prompts. For a professional codebase, five excellent skills are better than twenty vague ones.

Skill rule of thumb:

- Use `AGENTS.md` for rules Codex should always obey.
- Use a skill for a workflow Codex should perform on demand.
- Use a script inside a skill when the workflow has mechanical steps.
- Use MCP with a skill when it needs external systems.

### 3. `.codex/config.toml`: Trusted Project Wiring

Use repo-local `.codex/config.toml` sparingly. OpenAI documents that user-level config lives in `~/.codex/config.toml`, while project-scoped `.codex/config.toml` loads only for trusted projects. Project config cannot override machine-local provider, auth, app metadata, notification, profile, telemetry, or model-provider routing keys.

Good project-local candidates:

- Project root markers if Sparkle ever needs them beyond `.git`.
- Repo-scoped skill enablement overrides if needed.
- Repo-local MCP entries that do not contain personal credentials.
- Hooks after local scripts are stable.
- Subagent role definitions if Sparkle develops repeatable specialist review roles.

Avoid putting these in repo config:

- API keys.
- Personal model choices.
- Provider routing.
- Notifications.
- Telemetry.
- Private MCP credentials.
- Machine-specific Qt/Vulkan paths.

Those belong in user-level config or environment variables.

If `.codex/config.toml` is added, it should start with a short comment explaining why repo-local config exists and what must not be placed there.

### 4. MCP: External Context And Tools

OpenAI documents that MCP connects Codex to third-party tools and context, and that the CLI and IDE extension share MCP configuration through `config.toml`.

Recommended personal MCP setup for Sparkle:

- OpenAI Docs MCP: official Codex/OpenAI docs lookup.
- Context7 or equivalent docs MCP: current library/framework docs.
- GitHub MCP: pull requests, issues, code review context beyond local git.
- Playwright or Chrome DevTools MCP: browser/app inspection when Sparkle gains web docs, dashboards, or web tooling.
- Figma MCP: only if launcher/product UI designs live in Figma.
- Sentry or logging MCP: only if Sparkle runtime/editor telemetry exists.

Start with user-level MCP. Promote to repo-local only when every Sparkle contributor should have the same server and no private secrets are embedded.

### 5. Hooks: Automation With Restraint

Hooks are useful, but they should come after the command surface is reliable. OpenAI documents project-local hooks through `.codex/hooks.json` or inline `[hooks]` in `.codex/config.toml`; project-local hooks load only for trusted projects.

Useful future hooks:

- Warn before editing generated folders.
- Warn before broad recursive deletes in source roots.
- After C++ edits, suggest the narrow CMake target to build.
- Before final response, remind Codex to report validation run or explain why it was skipped.

Do not start with hooks that run expensive builds automatically. They will slow iteration and train everyone to ignore the automation.

### 6. Subagents: Specialist Focus

Subagents are useful when a task benefits from isolated specialist passes. They should not replace clear repo guidance.

Good Sparkle subagent candidates later:

- `build-verifier`: inspect build files and recommend the narrow validation command.
- `rhi-reviewer`: review RHI/Renderer changes for backend parity and resource lifetime risks.
- `launcher-ux-reviewer`: review Qt launcher changes against the product UI system.
- `cook-pipeline-reviewer`: review import/cook/shader package changes across pipeline boundaries.

Use subagents for review and investigation. Keep implementation ownership in the main thread unless the task is naturally parallel.

## Sparkle Quality System

### Verification Ladder

Codex should not run the biggest command every time. It should choose the smallest validation that proves the change.

Suggested ladder:

- Docs-only: no build; check links/paths and report no runtime validation.
- Formatting-only C++ change: `clang_format_check` when available.
- Launcher UI/core change: build `SparkleLauncher`.
- Shader compiler change: build `ShaderCompiler`, then run shader cook/cache validation when scripts exist.
- Import/cook change: build affected cooker/converter and run a targeted sample cook.
- RHI/Renderer change: build affected runtime/editor target and run smoke validation on D3D12/Vulkan when available.
- CMake/dependency/profile change: fresh configure plus targeted build.

Candidate commands, pending local verification:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal
```

### Verification Gap To Fix

The GitHub workflow references:

- `Scripts/CI/RunShaderCompilerCookCheck.ps1`
- `Scripts/CI/RunShaderCacheCheck.ps1`
- `Scripts/CI/RunShaderEditorPipelineCheck.ps1`

This checkout currently has no `Scripts` directory. Either those scripts need to be restored/added, or the workflow needs to be updated. Agentic development gets much stronger when the same commands work locally, in CI, and in Codex instructions.

### Module Boundaries

Recommended root `AGENTS.md` rule:

```md
Respect the engine layer order: Core -> Platform -> RHI -> Renderer -> GameFramework -> Editor/Application. Tools can depend on engine surfaces, but runtime modules should not depend on tool internals.
```

### Generated Artifacts

Agents should treat these as generated unless the user explicitly asks otherwise:

- `build/`
- `cmake-build-debug/`
- `artifacts/`
- `dist/`
- `logs/`

### Dirty Worktree Discipline

Sparkle often has active uncommitted work. Codex should preserve user changes, inspect before editing touched files, and avoid broad rewrites unless requested.

### Launcher UI Work

Recommended root or `Tools/Launcher/AGENTS.md` rule:

```md
For `Tools/Launcher/SparkleLauncher`, preserve the established visual direction, shared visual style helpers, icon library, widget separation, and action/history model separation. Prefer small extraction and consistency passes over large rewrites.
```

### Shader/Cooking Work

Recommended root or `Tools/Shaders/AGENTS.md` rule:

```md
For shader compiler, material/texture/scene cooking, or RHI shader contract changes, identify the affected pipeline stages before editing and run the narrowest available build/cook validation.
```

## Personal Codex Setup

Use personal Codex config for developer-specific behavior:

- `~/.codex/AGENTS.md`: personal working agreements, answer style, default review expectations.
- `~/.codex/config.toml`: model, auth, provider, notifications, private MCP, local environment behavior.
- `~/.agents/skills`: personal reusable workflows that are not Sparkle-specific.

Use repo files for team/repo behavior:

- `AGENTS.md`: Sparkle rules.
- `.agents/skills`: Sparkle workflows.
- `.codex/config.toml`: trusted project wiring only.

This works across Codex CLI and the Codex IDE extension. The repo should not need separate VS Code and Rider instruction files. The editor changes, but the Codex operating stack stays the same.

## Recommended Implementation Sequence

### Phase 1: Foundation

- Add root `AGENTS.md`.
- Keep it compact and Sparkle-specific.
- Include the verification ladder and generated-folder rules.
- Include dirty worktree discipline.
- Include launcher, shader/cook, import, and RHI routing guidance.

### Phase 2: Validation

- Restore or replace the missing `Scripts/CI/*.ps1` commands.
- Make local and CI verification names match.
- Add a short docs section that maps change type to command.

### Phase 3: First Skill

- Add `.agents/skills/sparkle-build-verify/SKILL.md`.
- Put build profile selection, target selection, and validation reporting there.
- Add scripts only after the manual command path is proven.

### Phase 4: Domain Skills

- Add `launcher-ui-pass`.
- Add `shader-cook-pipeline`.
- Add `rhi-change-review`.
- Add `gltf-import-feature` only when importer/cook/runtime feature work becomes active again.

### Phase 5: MCP And Automation

- Add OpenAI Docs MCP and current-library docs MCP in user config.
- Add GitHub MCP if PR/issue workflow becomes central.
- Add Figma MCP if designs move through Figma.
- Add hooks only for stable, cheap checks.
- Add subagents only after review patterns repeat.

## What "Locked In" Looks Like

The mature workflow should feel like this:

- You state the goal.
- Codex already knows the repo map and quality rules from `AGENTS.md`.
- Codex chooses the right skill when the task matches a known workflow.
- Codex uses MCP when current external context is needed.
- Codex edits narrowly and respects dirty work.
- Codex runs the smallest meaningful validation.
- Codex reports what changed, what was verified, and what risk remains.
- Recurring misses get codified into `AGENTS.md`, a skill, or a script.

That is the path from "prompt and hope" to a real agentic engineering loop.

## Success Criteria

The Codex setup is healthy when:

- A new reviewer can understand all checked-in AI files in under ten minutes.
- Root clutter stays minimal.
- Codex consistently finds the right subsystem before editing.
- Repeated corrections become durable rules or skills.
- Validation commands are real and shared between local work, CI, and agent instructions.
- No personal credentials or local machine assumptions are checked in.
- RHI/Renderer/shader changes receive stronger review and validation than ordinary docs or UI edits.

## Sources

- OpenAI Codex customization: https://developers.openai.com/codex/concepts/customization
- OpenAI Codex AGENTS.md guide: https://developers.openai.com/codex/guides/agents-md
- OpenAI Codex skills guide: https://developers.openai.com/codex/skills
- OpenAI Codex configuration reference: https://developers.openai.com/codex/config-reference
- OpenAI Codex advanced configuration: https://developers.openai.com/codex/config-advanced
- OpenAI Codex MCP guide: https://developers.openai.com/codex/mcp

## Action Menu

Use this as a pick-and-choose implementation menu. The recommended path is to start at the top and only add later items when they solve a real repeated problem.

### Foundation

1. Add root `AGENTS.md` as the single canonical Codex repo contract.
2. Keep root `AGENTS.md` compact: repo map, generated folders, module boundaries, dirty worktree rules, verification ladder, and graphics-review posture.
3. Do not add `CLAUDE.md`, prompt folders, chat dumps, AI scratch notes, or duplicated assistant instruction files.
4. Add nested `AGENTS.md` files only after repeated mistakes prove they are needed.
5. If nested guidance becomes useful, consider `Engine/AGENTS.md`, `Engine/RHI/AGENTS.md`, `Tools/Launcher/AGENTS.md`, `Tools/Shaders/AGENTS.md`, and `Tools/Import/AGENTS.md`.

### Validation

1. Restore or replace the missing `Scripts/CI/*.ps1` files referenced by `.github/workflows/shader-cook.yml`.
2. Make local validation command names match CI command names.
3. Document a validation ladder: docs-only, formatting-only, launcher build, shader compiler build, cook validation, RHI/runtime smoke, CMake configure.
4. Keep validation commands reproducible and narrow; do not make Codex run expensive full builds by default.
5. Add scripts before hooks. Hooks should call proven scripts, not invent new behavior.

### Skills

1. Add `.agents/skills/sparkle-build-verify/SKILL.md` first.
2. Add `rhi-change-review` for backend parity, descriptor/resource lifetime, synchronization, ABI, and validation risks.
3. Add `shader-cook-pipeline` for shader reflection, binding layouts, package outputs, cache behavior, and recook correctness.
4. Add `launcher-ui-pass` for Qt launcher workflow consistency, screenshots, diagnostics, and product UI discipline.
5. Add `gltf-import-feature` only when importer/cook/runtime feature work becomes active again.
6. Add `rendering-performance-pass` later for hypothesis, capture plan, metrics, before/after notes, and measured claims.
7. Add `developer-tooling-pass` later for launcher/tools workflows that improve build, cook, launch, inspect, validate, and package ergonomics.
8. Keep the skill list capped. Five excellent skills are better than twenty vague ones.

### Config And MCP

1. Keep personal model, auth, provider, notification, private MCP, and preferences in user-level `~/.codex/config.toml`.
2. Add repo `.codex/config.toml` only when there is shared trusted project wiring that belongs in the repo.
3. Never check in API keys, private MCP credentials, local machine paths, or personal model preferences.
4. Start MCP integrations at the user level: OpenAI Docs, current library docs, GitHub, browser tooling, Figma, or logging only when useful.
5. Promote MCP setup into repo config only when every contributor should share it and no secrets are embedded.

### Hooks And Subagents

1. Add hooks only after scripts are stable and cheap.
2. Useful future hooks: warn before editing generated folders, warn before broad deletes, suggest narrow build targets after C++ edits, remind Codex to report validation.
3. Avoid hooks that run expensive builds automatically.
4. Add subagents only after repeated review patterns emerge.
5. Candidate subagents: `build-verifier`, `rhi-reviewer`, `launcher-ux-reviewer`, and `cook-pipeline-reviewer`.

### GPU-Vendor-Grade Quality

1. Require RHI/Renderer changes to state whether they affect D3D12, Vulkan, shared abstractions, descriptors, resource lifetime, synchronization, shader-visible layouts, or cooked runtime data.
2. Require shader/cook changes to preserve reflection, binding, cache, package, and recook correctness.
3. Require performance work to include a hypothesis, measurement plan, metrics, and uncertainty when profiling was not run.
4. Treat diagnostics, logs, artifact paths, and reproduction steps as first-class developer experience.
5. Write feature notes for complex rendering work: problem, constraints, implementation, validation, quality/performance tradeoffs, and known limits.
6. Avoid unmeasured speedup claims. Codex should provide a profiling plan when tools are unavailable.
7. Build evidence over time with smoke tests, RHI validation, shader compiler diagnostics, cook validation, and capture notes.

### Professional Presentation

1. Keep repo-root AI clutter near zero.
2. Make every checked-in AI file explainable in one sentence.
3. Prefer precise engineering language over hype.
4. Make documentation useful to a human reviewer, not just to Codex.
5. Use Codex to strengthen engineering discipline, not to hide weak validation.
