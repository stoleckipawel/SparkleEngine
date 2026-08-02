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

## Language-Specific Profiles

### C++

C++ source must make ownership, mutability, error behavior, and cost visible in review. Module and subsystem owners decide where product policy, algorithms, lifetime, and host integration belong.

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

Anonymous namespaces are forbidden in owned source; the expected repository count is zero. Do not replace them with `Detail`, `Internal`, `Private`, `Local`, `Implementation`, `Helpers`, `Common`, or `Misc` buckets. Behavior supporting one owner belongs to that owner; free functions belong only in a genuine established domain namespace.

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
