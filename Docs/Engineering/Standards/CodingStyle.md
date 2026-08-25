# Coding Style

Status: binding coding and formatting standard

Applies to: owned C++, HLSL/HLSLI, build code, tests, and adjacent implementation documentation

## Purpose and Scope

Coding style has three layers:

1. **Mechanical formatting** — owned by [`.clang-format`](../../../.clang-format).
2. **Machine-checkable semantics** — owned by [`.clang-tidy`](../../../.clang-tidy), compiler warnings, and build gates.
3. **Human design conventions** — owned by this document and its focused companions.

This document owns language conventions and physical source organization. Read it with:

- [Naming and Vocabulary](NamingAndVocabulary.md) for identifiers and domain terms;
- [Repository Structure and Ownership](RepositoryStructureAndOwnership.md) for responsibility, APIs, and lifetimes;
- [Validation, Performance, and Evidence](ValidationPerformanceAndEvidence.md) for comments, assertions, logging, and tests.

## Binding Baseline

Executable configuration wins for exact behavior. The accepted baseline is:

- clang-format 22.1.3 for owned C++ and shader formatting;
- C++20 for owned engine targets;
- Allman braces;
- tabs for indentation at width four;
- a 140-column formatting limit for C++ and shaders;
- braces and multiline bodies for control flow;
- left-bound pointers and references;
- one item per line after a call, declaration, initializer, or long braced list wraps;
- no diff-sensitive consecutive declaration, assignment, macro, or trailing-comment alignment;
- authored include and `using` order preserved;
- indentation-only C++ comment formatting and no shader comment reflow;
- warnings-as-errors for configured clang-tidy checks.

The [Clang-Format Decision Record](../ClangFormatDecisionRecord.md) preserves the evidence, alternatives, and accepted ballot. [`.clang-format`](../../../.clang-format) and the inherited [shader override](../../../Engine/Assets/Shaders/.clang-format) remain authoritative for exact formatter behavior.

### Formatter Enforcement Boundary

clang-format owns whitespace and line layout. It MUST NOT insert or remove braces, reorder qualifiers, remove parentheses, sort includes or `using` declarations, or perform other semantic-looking rewrites. Run version 22.1.3 with `--Werror`; a version change is a deliberate formatting migration.

Two accepted source-format rules are outside clang-format's reliable enforcement boundary:

1. **No namespace-end comments.** Close named namespaces with `}` only. `FixNamespaceComments: false` prevents automatic additions but does not delete existing comments; remove existing `// namespace ...` suffixes in the dedicated format migration and reject new ones in review or a repository check.
2. **HLSL attributes use their own line.** Place `[numthreads]`, `[loop]`, `[unroll]`, and equivalent shader attributes immediately above the declaration or statement they govern. clang-format 22.1.3 parses HLSL through its C++ fallback and does not reliably move these attributes, so shader review or a repository check owns enforcement.

```cpp
namespace Renderer
{
    class RenderDevice final : public DeviceContract
    {
    };
}
```

```hlsl
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    Dispatch(dispatchThreadId);
}
```

## Readability Rules

- Use blank lines to expose acquire, transform, commit, and publish stages.
- Keep consecutive initialization or mutation of one record together.
- Do not fragment one cohesive condition, expression, or initialization sequence with arbitrary whitespace.
- Keep a declaration, call, assignment, return type, or signature on one line when it fits the configured limit and remains readable.
- When a call or aggregate wraps, group elements by meaning; avoid stair-step fragmentation of simple access, casts, names, and ternaries.

[Repository Structure and Ownership](RepositoryStructureAndOwnership.md) owns function/class decomposition and orchestration/mechanism boundaries; formatting does not substitute for that review.

## Established C++ Conventions

- Use `nullptr`, scoped enums, explicit constructors, and domain types where they clarify contracts.
- Use `final` when a concrete class is not designed for inheritance.
- Use `noexcept` only when the real error contract supports it.
- Prefer `enum class` or a domain value over combinations of boolean mode parameters.
- Prefer direct typed code over macros, reflection, type erasure, or template metaprogramming when both express the same requirement.
- Use designated initialization where it improves contract readability and matches the type.

Ownership/lifetime and inheritance rules belong to [Repository Structure and Ownership](RepositoryStructureAndOwnership.md); units, domains, and ABI vocabulary belong to [Naming and Vocabulary](NamingAndVocabulary.md).

### Owner-local helper placement

Placement is part of coding style. A `static` helper, private class, or short local switch is not correctly placed merely because it is hidden inside one source file.

- A rule expressed entirely in neutral/domain vocabulary belongs to the narrow owner of that vocabulary. Format classification belongs with the format contract; descriptor validity belongs with the descriptor; graph-resource validity belongs with the graph resource.
- Place a function where the governing contract changes, not beside whichever caller first needed it. A helper that accepts only another subsystem's types and uses no caller-local state, native API fact, or capability is presumed misplaced until its caller-specific responsibility is demonstrated.
- Backend-private code owns native type translation, API/device capability queries, native-object construction, and evidence-backed backend workarounds. It must not independently redefine neutral format, descriptor, resource, shader-stage, or state validity.
- Before adding a local helper or validation block, search sibling backends/providers and the neutral owner for the same cases, predicates, switches, or error meaning. A second implementation of one invariant is a placement defect, even when both copies currently agree.
- Do not respond by creating `Common`, `Utilities`, a generic helper bag, or a forwarding wrapper. Move the invariant to its narrow semantic owner and leave each consumer with one direct call; keep genuinely native conversions local.
- When touching one implementation in a backend/provider family, audit its siblings. Review evidence names which logic is shared policy and which remains local because its native type, capability, lifetime, or failure contract is genuinely different. A repair is incomplete while a mirrored enum/record, duplicated derived field, copied case list, conversion between identical concepts, or old helper remains reachable.

Code review blocks a wrong-owner or duplicated invariant as an ownership/maintainability defect rather than dismissing it as optional style.

### One-field types

Status: binding for owned C++ and shader-facing records.

Do not introduce a nominal struct whose only instance field is immediately unwrapped by every consumer. Pass the value directly, query the authoritative owner, or put the field on the record that owns its lifetime. A distinct name is not enough justification for another carrier, snapshot, result, settings, validity, or context type.

A one-field type is permitted only when the type itself enforces a contract that the underlying value cannot express:

- a strong ID, handle, unit, or index with distinct overload resolution and explicit validity, comparison, generation, or ownership semantics;
- an RAII/PIMPL owner whose destructor, move contract, or incomplete-type boundary is material;
- a tagged command/variant alternative or ECS component whose nominal type is consumed by dispatch or schema registration;
- a required C++/shader, cooked-file, platform, vendor, reflection, or other external ABI record;
- a fixed-size mathematical object whose array field represents the object's elements;
- a generic storage implementation whose wrapper is required to preserve the contained type, lifetime, or type erasure.

An allowed type MUST expose or participate in that contract at its owner. Do not keep a one-field aggregate for possible future fields, naming symmetry, member-access aesthetics, or to avoid changing producers and consumers. When a touched type no longer satisfies an exception, remove it and update every consumer in the same change.

Enforcement is an exact definition/consumer audit in review. A raw field-count check is advisory because strong handles, ABI records, tagged alternatives, and RAII owners are intentional positives. The accepted external precedents are narrow: Unreal Engine 5.8 keeps production state on the graph resource through [`FRDGViewableResource::HasBeenProduced`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FRDGViewableResource), AMD RPS revision [`f3330f5`](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/f3330f5306d15af8529a310f6255225c864b0961/tools/rps_hlslc/rpsl/rpsl.h) makes temporal layers and read/write access properties of declared resources, and AMD FidelityFX revision [`1680d1e`](https://github.com/GPUOpen-Effects/FidelityFX-FSR2/blob/1680d1edd5c034f88ebbbb793d8b88f8842cf804/src/ffx-fsr2-api/ffx_types.h) exposes a one-field `FfxResourceInternal` only as an actual internal resource handle. NVIDIA Falcor revision [`eb540f6`](https://github.com/NVIDIAGameWorks/Falcor/blob/eb540f6748774680ce0039aaf3ac9279266ec521/Source/Falcor/RenderGraph/RenderGraph.cpp) resolves graph inputs/outputs as resources and uses null resource references for absence. Sparkle adopts owner/resource queries and strong-handle exceptions; it does not copy those frameworks' APIs or scale.

## Language-Specific Profiles

### C++

C++ source must make ownership, mutability, error behavior, and cost visible in review. Module and subsystem owners decide where product policy, algorithms, lifetime, and host integration belong.

#### Console-variable access

Runtime code MUST NOT pass a console variable, or a value copied solely from a console variable, through application-defined function parameters, settings records, context objects, or forwarding helpers. A subsystem that consumes console-controlled policy queries the registered CVar directly at its narrow decision point. Pass only the non-CVar data or focused capability required to resolve that decision. Calls to standard-library algorithms and immediate construction of the owned output are ordinary value use, not CVar plumbing.

Cached topology owners may retain the resolved topology they actually built so they can detect when reconstruction is required. That cache is observation state, not another policy authority, and MUST NOT be forwarded back into feature construction. Editor and persistence boundaries may capture or apply complete settings state because their responsibility is explicitly to present or serialize CVar values; this exception does not permit runtime CVar plumbing.

Code review searches both direct calls such as `Consumer(CVarName.Get())` and indirect copies placed in `*Settings`, `*Context`, `*Options`, or similarly generic records. Remove the carrier and update the consumer to query its CVar owner in the same change.

### HLSL/HLSLI

Shader formatting may use a scoped executable override when C++ parsing produces unsafe or unreadable output. A shader-style change requires representative DXIL and SPIR-V compilation, reflection/layout checks, and package validation. Shader semantic rules live in [Graphics Engineering](GraphicsEngineering.md).

Shader attributes MUST appear on a separate line immediately before the declaration or control statement they govern:

```hlsl
[unroll]
for (uint sampleIndex = 0u; sampleIndex < sampleCount; ++sampleIndex)
{
    AccumulateSample(sampleIndex);
}
```

### Build and Tool Languages

CMake, scripts, and generated inputs follow their language tools and nearby owned precedent. They still use precise names, narrow responsibility, explicit failure, and deterministic output. Do not force C++ punctuation or casing rules onto another language without a repository-specific reason.

## Decision Status

Use these labels while the detailed style is being iterated:

- **Binding** — required now and backed by prose, tool configuration, or both.
- **Candidate** — evidence-backed proposal awaiting an explicit decision.
- **Open** — intentionally undecided; follow strong local precedent without expanding inconsistency.
- **Rejected** — considered and not adopted.

The following architecture lets the coding-style work proceed without inventing rules prematurely:

| Topic | Status | Final owner | Evidence needed before changing it |
| --- | --- | --- | --- |
| Formatter version, width, indentation, braces, wrapping | Binding; accepted 2026-08-02 | `.clang-format`, shader override, and this baseline | representative no-write diff, C++ build, and shader validation for migrations |
| Naming cases and domain vocabulary | Binding | [Naming and Vocabulary](NamingAndVocabulary.md) and `.clang-tidy` | migration impact and ABI/serialization review for changes |
| Files, headers, includes, namespaces | Binding | this document | compile boundaries and dependency graph |
| Ownership, RAII, API and lifetime expression | Binding | [Repository Structure and Ownership](RepositoryStructureAndOwnership.md) | concrete owner/consumer analysis |
| `auto`, type aliases, deduction, structured bindings | Open | future section in this document | readability audit across source, debugger/review cost, vendor comparison |
| `const`, `constexpr`, `consteval`, `[[nodiscard]]` | Open beyond existing compiler/tidy checks | future section in this document | contract value, noise, compile-time and migration cost |
| casts, conversions, numeric narrowing | Open beyond existing tidy checks | future section in this document | shader/ABI/math use cases and warning coverage |
| lambdas, captures, callbacks | Open beyond lifetime rules | future section in this document | capture-lifetime audit and task/UI callback patterns |
| error handling and exceptions | Open beyond owner-specific rules | future section in this document | module boundaries, build flags, failure taxonomy, existing error types |
| class/member ordering and function length guidance | Candidate | this document and [Source Files and Headers](#source-files-and-headers) | representative navigation review; avoid arbitrary numeric gates |
| standard-library use and custom primitives | Open | this document plus module owners | platform/compiler support, ABI, allocations, compile-time impact |
| comments and API documentation | Binding core; detailed conventions may expand | [Validation, Performance, and Evidence](ValidationPerformanceAndEvidence.md) | generated-doc needs and review signal |
| tests and benchmark source style | Binding core | this document; validation owns behavior and evidence | test framework precedent and CI behavior |

Do not convert a candidate or external precedent into review feedback as if it were already binding. Accept a decision by updating its authoritative document, executable checks where possible, migration scope, and validation plan together.

## Adding a Detailed Rule

When the coding-style iteration accepts a topic, document it with the same compact shape:

1. **Status and scope** — binding/candidate/rejected and the source families it affects.
2. **Rule** — one unambiguous requirement.
3. **Rationale** — readability, correctness, portability, compile cost, runtime cost, or review stability.
4. **Examples** — the smallest good/bad pair that resolves ambiguity; avoid building a tutorial.
5. **Enforcement** — formatter, clang-tidy, compiler, repository check, or human review.
6. **Exceptions** — narrow owner, evidence, and review/deletion gate.
7. **Migration** — touched-code policy or bounded repository-wide conversion with validation.

If a rule cannot state its owner or enforcement path, keep it a candidate rather than adding subjective review folklore.

## Source Files and Headers

### Placement

- Public files contain stable contracts required across module boundaries.
- Private files contain mechanisms, worker records, caches, compilation, backend policy, editor models, and implementation types.
- A filename matches its primary public type or cohesive operation; a `.cpp` matches what it implements.
- Give a substantive class a file pair when it has independent state, lifetime, responsibility, or evolution. Small private records may co-locate with their sole owner.
- A one-file directory is justified only by a real ownership/growth boundary. Do not create synonym folders.

### Headers

Owned headers are declaration surfaces. Function bodies are limited to templates and trivial direct getters, setters, or accessors. Move constructors, destructors, algorithms, transforms, validation, orchestration, factories, non-template operators, nontrivial `constexpr`, and policy-bearing convenience functions to the matching `.cpp`. Deleted special-member declarations remain because they express the type contract.

Do not define classes, structs, or enum-like implementation records inside functions. Place them on their owner, at source scope with a responsibility-bearing name, or in a private collaboration header when several implementation units genuinely share the concept.

### Namespaces

Anonymous (unnamed) namespaces are forbidden in owned source; the expected repository count is zero. Do not replace them with `Detail`, `Internal`, `Private`, `Local`, `Implementation`, `Helpers`, `Common`, or `Misc` buckets. Behavior supporting one owner belongs to that owner; free functions belong only in a genuine established domain namespace.

Indent every named namespace. Do not place a trailing `// namespace ...` comment after its closing brace. Existing namespace-end comments are migration debt and MUST be removed by the dedicated formatting change or when their file is otherwise modified.

### Includes and Compile Boundaries

- Include what the file directly uses; do not rely on transitive includes.
- Prefer forward declarations when complete-type and ownership rules allow.
- Keep PCH assumptions private to the configured module.
- Public headers MUST NOT include private module headers.
- Backend-native headers remain backend-private.
- Remove dead includes after moves/refactors.
- Preserve repository-owned include groups/order; current `.clang-format` does not sort includes.

Detailed include ordering remains open. Follow strong local module precedent without broad reorder-only churn until an accepted rule and enforcement path exist.

When changing files, reconcile module/subsystem/visibility/name/responsibility, CMake membership, source groups, exports, package rules, includes, tests, and documentation. Search deleted paths/names for zero unintended production references and compile consumers, not only the defining target.

## Design Precedents

External repositories inform decisions but never become Sparkle authority by association:

- [NVIDIA RTX Remix engineering standards](https://docs.omniverse.nvidia.com/kit/docs/rtx_remix/latest/docs_dev/code-quality/engineering-standards.html) separate engineering correctness from visual formatting and emphasize root-cause fixes.
- [AMD Cauldron's design overview](https://gpuopen.com/radeon-cauldron-new-sdk-framework/) supports responsibility-bearing feature files and backend separation without imposing one-class-per-file mechanically.
- [Epic's C++ standard](https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine) demonstrates a single navigable engine convention, but Unreal-specific prefixes/reflection rules do not transfer automatically.
- [Filament's code style](https://github.com/google/filament/blob/d7e34116f823d5064bcc8ed89712a650351436b9/CODE_STYLE.md) pairs human rules with executable tooling and repository-specific file/include policy.

Formatter precedents, pinned revisions, and the accepted comparison belong to the [Clang-Format Decision Record](../ClangFormatDecisionRecord.md). Any new precedent must record the exact artifact/revision, observed behavior, local adoption boundary, migration cost, and falsifying check.

## What Consistency Means

Consistent style is not visual sameness alone. A cohesive repository has:

- one documented authority for each rule;
- formatting that is reproducible by a pinned tool;
- names that mean the same thing across CPU, GPU, serialization, and UI boundaries;
- similar responsibilities placed and shaped similarly;
- local exceptions that are explicit, narrow, and evidence-backed;
- no broad reformat mixed with semantic changes;
- code review focused on design because mechanical style is automated.
