# Clang-Format Decision Record

Status: accepted decision record; executable authority lives in `.clang-format` and the binding coding-style standard

Date: 2026-08-02

Scope: owned C++20 in `Engine`, `Tools`, and `Projects`, plus owned HLSL/HLSLI in `Engine/Assets/Shaders`

## Executive Summary

Sparkle should adopt a deliberate Sparkle profile rather than copy one NVIDIA or AMD repository. The public vendor repositories do not share one style:

- NVIDIA NRD/NRI use attached braces, four spaces, no column limit, bin-packed arguments, unindented namespaces, and sorted includes.
- NVIDIA CCCL uses a 120-column, two-space, custom-Allman C++20 profile with one argument per line and sorted/regrouped includes.
- AMD Render Pipeline Shaders (RPS) uses 120 columns, four spaces, custom-Allman braces, indented namespaces, one argument per line, aligned declaration/assignment columns, and no include sorting.

Sparkle's current identity is closest to AMD RPS in brace and namespace structure and to NVIDIA CCCL in diff-friendly list wrapping. Its established tabs, ownership-oriented vertical wrapping, and preserved include blocks are coherent and should not be discarded merely to resemble a vendor.

The accepted target is:

- pin clang-format `22.1.3` for deterministic rollout;
- retain LLVM as the inheritance baseline, then explicitly set every behavior that affects Sparkle's visible style;
- retain Allman braces, four-column tabs for indentation, indented namespaces and case labels, and left-bound pointers/references;
- change access modifiers from the inherited `-2` offset to `-4`, eliminating the current two-space half-indent;
- use 140 columns for both C++ and shaders;
- retain one item per line once a call, declaration, initializer, or braced list wraps;
- forbid multiple inheritance; retain conventional trailing commas only as defensive formatting for legacy input;
- allow short in-class functions and short callback lambdas, while keeping all control-flow bodies and enums multiline;
- put non-assignment binary operators at the start of continuation lines;
- stop alignment that causes unrelated lines to move when one name changes;
- retain manually owned include blocks and order;
- prohibit namespace-end comments and remove existing comments during the dedicated migration;
- require HLSL attributes on their own line through authored policy and a repository check;
- keep clang-format non-semantic: no inserted/removed braces, reordered qualifiers, removed parentheses, sorted includes, or other source transformations;
- use an inherited shader-subtree override because HLSL is parsed as C++ by clang-format, not as a native HLSL language;
- format shaders only after a representative pilot passes DXIL/SPIR-V compile, reflection, and package-identity checks.

The selected 140-column limit preserves SparkleEngine's established baseline and minimizes one-time wrapping churn. The 120-column comparison remains useful evidence but was not selected.

## Decision Boundary

This record preserves the considered options and accepted decision. The executable policy is [the root `.clang-format`](../../.clang-format), its shader-subtree override, and the [binding coding-style standard](Standards/CodingStyle.md).

Accepting the profile does not authorize an unreviewed whole-repository rewrite. C++, shader, CMake, package, generated artifact, third-party source, and D3D12/Vulkan behavior remain unchanged until the dedicated migration passes its no-write review and validation gates.

## Repository Evidence

### Current enforced baseline

The coding-style standard currently binds:

- C++20;
- Allman braces;
- tabs for indentation at width four;
- a 140-column limit;
- no single-line control-flow bodies;
- left-bound pointers and references;
- explicit, diff-friendly wrapping;
- compact declarations, calls, assignments, return types, and signatures when they fit comfortably;
- preserved include grouping and ordering.

Those constraints support the guide's larger goals: code should expose ownership and lifetime, high-level functions should read as workflows, and formatting should not substitute for decomposition.

### Inventory

The read-only inventory excluded `Engine/RHI/Private/D3D12/ThirdParty` and found:

| Source family | Files | Lines over 100 | Lines over 120 | Lines over 140 | Lines over 160 |
| --- | ---: | ---: | ---: | ---: | ---: |
| C/C++ | 1,882 | 7,474 in 1,067 files | 2,526 in 632 files | 305 in 93 files | 131 in 43 files |
| HLSL/HLSLI | 113 | 217 in 52 files | 83 in 26 files | 8 in 4 files | 1 in 1 file |

Indentation is already strongly established:

| Source family | Tab-indented lines | Four-or-more-space-indented lines | Other space-indented lines |
| --- | ---: | ---: | ---: |
| C/C++ | 86,509 | 4,085 | 1,062 |
| HLSL/HLSLI | 4,004 | 100 | 0 |

Changing from tabs to spaces would therefore create broad whitespace churn without improving the ownership or readability goals. Continuation alignment accounts for part of the existing space indentation and is not evidence of a competing source style.

### Pre-decision configuration drift

Before the accepted profile was applied, clang-format `22.1.3` accepted the previous file but exposed several issues:

- `Standard` is inherited as `Latest`, not the repository's explicit C++20 contract.
- `AccessModifierOffset` is inherited from LLVM as `-2`; with a four-column body this produces the prevalent two-space `public:`/`private:` half-indent.
- short enums are inherited as allowed even though other diff-sensitive constructs are multiline.
- trailing comments are aligned, so editing one declaration can move comments on neighboring lines.
- `SortUsingDeclarations` is inherited as `LexicographicNumeric` even though include ordering is manually owned.
- namespace comment repair is disabled, but the formatter cannot remove existing comments under the newly selected prohibition.
- EOF newline insertion is disabled.
- several spellings are deprecated or legacy compatibility forms, including return-type and constructor-initializer options.
- clang-format 22 changed `AlignAfterOpenBracket` from an enum to a boolean and split the former `AlwaysBreak` behavior into explicit bracket-breaking options. The compatibility spelling works, but hides the actual policy.

A read-only clang-format 22.1.3 scan using the pre-decision file found proposed replacements in 1,009 of 1,882 C/C++ files and 52 of 113 shader files, with no formatter errors. This is not an instruction to apply those replacements. It means the rollout must be treated as a reviewed format migration, not an assumed no-op.

## Vendor Comparison

These are repository-specific precedents, not company-wide NVIDIA or AMD standards.

| Behavior | Sparkle before decision | NVIDIA NRD/NRI | NVIDIA CCCL | AMD RPS | Interpretation for Sparkle |
| --- | --- | --- | --- | --- | --- |
| Base | LLVM | Google | LLVM | Google | Base matters less than explicit decisions and a pinned version. |
| Indent | tabs, width 4 | spaces, width 4 | spaces, width 2 | spaces, width 4 | Retain Sparkle's established tabs. |
| Width | 140 | unlimited | 120 | 120 | 120 is a strong C++ option; wider shaders remain defensible. |
| Braces | Allman | attached | custom Allman | custom Allman | Both vendor families use both forms; retain Sparkle Allman. |
| Namespace indentation | all | none | none | all | Sparkle and AMD RPS favor visible scope nesting. |
| Case labels | indented | indented | indented | not indented | Current Sparkle has more supporting evidence. |
| Wrapped arguments | one per line | bin-packed | one per line | one per line | Retain one-per-line wrapping. |
| Consecutive columns | off | assignments/declarations off; macros on | selected alignment on | assignments/declarations/macros on | Prefer off for stable diffs; tables can use format-off only when justified. |
| Includes | preserve blocks; do not sort | preserve blocks; sort | regroup and sort | do not sort | Retain Sparkle's ownership-sensitive manual order. |
| Comments | reflow and align trailing | reflow and align trailing | reflow; do not align trailing | do not reflow; align trailing | Use indentation-only reflow and no trailing alignment. |
| Pointer/reference | left/left | left/pointer | left/inherited | inherited | Retain explicit left/left. |
| Source mutation | no inserted braces | no inserted braces | insert braces | inherited off | Keep mutation off; clang-tidy already enforces braces. |
| Standard | inherited `Latest` | `Auto` | `c++20` | inherited | Set `c++20` explicitly. |

Reviewed sources, pinned to the inspected configuration revisions:

- [NVIDIA NRD `.clang-format` at `fd338276`](https://github.com/NVIDIA-RTX/NRD/blob/fd33827618aea7cb8b17b74a4923b0c1afc2c319/.clang-format)
- [NVIDIA NRI `.clang-format` at `56e42c2f`](https://github.com/NVIDIA-RTX/NRI/blob/56e42c2f50351538609fef0d1a642e679a8c3458/.clang-format)
- [NVIDIA CCCL `.clang-format` at `4bd9aaa4`](https://github.com/NVIDIA/cccl/blob/4bd9aaa474d608d2b42548ac7dfadd0c34a9d3a1/.clang-format)
- [AMD RPS `.clang-format` at `f3330f53`](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/f3330f5306d15af8529a310f6255225c864b0961/.clang-format)
- [clang-format 22.1 style-option reference](https://releases.llvm.org/22.1.0/tools/clang/docs/ClangFormatStyleOptions.html)
- [clang-format 22.1 tool and language reference](https://releases.llvm.org/22.1.0/tools/clang/docs/ClangFormat.html)

## Accepted Decision Ballot

The tables retain the considered alternatives and accepted selection for auditability.

The selected values below were accepted on 2026-08-02. Values under “Exact effect” describe the intended result and are validated against the pinned formatter where clang-format owns enforcement.

### 1. Tool and scope

| ID | Options | Selected | Exact effect |
| --- | --- | --- | --- |
| `G1` | Pin `22.1.3`; pin another version; allow any installed version | **Pin `22.1.3`** | Editors, scripts, and CI produce the same result. Any future upgrade is a deliberate format-only migration. |
| `G2` | One root profile; root plus shader override | **Root plus shader override** | C++ owns the main policy; `Engine/Assets/Shaders/.clang-format` inherits it and changes only shader-sensitive behaviors. |
| `G3` | All discovered files; tracked owned files | **Tracked owned files** | Use a `git ls-files` manifest. Exclude third-party, generated, build, artifact, cache, and fetched-dependency trees. |
| `G4` | Silent fallback; fail on config warnings | **Fail on warnings** | Validate with the pinned tool and do not permit older tools to ignore unknown options. |

### 2. Width and indentation

| ID | Options | Selected | Exact option/result |
| --- | --- | --- | --- |
| `L1` | 100; 120; 140; unlimited | **140 for C++ and shaders** | `ColumnLimit: 140` at root and in the inherited shader profile. |
| `L2` | spaces 4; tabs 4; spaces 2 | **tabs 4** | `IndentWidth: 4`, `TabWidth: 4`, `UseTab: ForIndentation`. Continuations may use spaces as required for columns. |
| `L3` | continuation 4; continuation 8 | **4** | `ContinuationIndentWidth: 4`; avoids deep stair-step indentation. |
| `L4` | access offset `-2`; `-4`; `0` | **`-4`** | `public:`/`private:` align with the class body's structural edge instead of receiving a two-space half-indent. |
| `L5` | namespace none; inner; all | **all** | `NamespaceIndentation: All`; retains visible ownership nesting and current shader structure. |
| `L6` | case labels aligned with switch; indented within switch | **indented** | `IndentCaseLabels: true`, `IndentCaseBlocks: false`. |
| `L7` | preprocessor none; before hash; after hash | **before hash** | `IndentPPDirectives: BeforeHash`, `PPIndentWidth: 2`; nested directives, including their `#`, move inward. |

### 3. Braces and compact forms

| ID | Options | Selected | Exact option/result |
| --- | --- | --- | --- |
| `B1` | attached; Allman; custom hybrid | **Allman** | `BreakBeforeBraces: Allman`; matches the binding guide and existing code. |
| `B2` | multiline control flow; short single-line control flow | **multiline** | `AllowShortIfStatementsOnASingleLine: Never`, short loops `false`, short blocks `Never`, short case labels `false`. |
| `B3` | short enum one line; enum values vertical | **vertical** | `AllowShortEnumsOnASingleLine: false`; enum additions remain one-line diffs. |
| `B4` | no short functions; class-only; all short | **class-only** | `AllowShortFunctionsOnASingleLine: InlineOnly`; supports trivial header accessors without merging top-level functions. The [coding-style standard](Standards/CodingStyle.md#headers) still decides which header bodies are allowed. |
| `B5` | no short lambdas; empty; callback-only; all | **callback-only** | `AllowShortLambdasOnASingleLine: Inline`; permits concise algorithms/callbacks such as `[this] { SubmitResize(); }` while keeping stored multi-step lambdas expanded. |
| `B6` | formatter inserts braces; formatter leaves structure | **leave structure** | `InsertBraces: false`, `RemoveBracesLLVM: false`; clang-tidy and review enforce braces without formatter AST-like edits. |

### 4. Calls, declarations, and initializers

| ID | Options | Selected | Exact option/result |
| --- | --- | --- | --- |
| `W1` | bin-pack; one per line once wrapped | **one per line** | `BinPackArguments: false`, `BinPackParameters: OnePerLine`, both “all on next line” allowances `false`. |
| `W2` | align under `(`; fixed continuation indent | **fixed continuation indent** | Use the clang-format 22 bracket controls to break after an opening function/braced-list bracket when needed; do not depend on deprecated `AlwaysBreak`. |
| `W3` | closing `)`/`}` on own line; closing token after last item | **after last item** | Keep `BreakBeforeCloseBracketFunction/BracedList: false`; matches the guide's compactness rule. |
| `W4` | bin-packed constructor initializers; one per line | **one per line** | `PackConstructorInitializers: Never`, `BreakConstructorInitializers: AfterColon`. |
| `W5` | trailing commas; leading commas once wrapped | **trailing commas** | `BreakInheritanceList: BeforeColon`; when the list wraps, place the colon before the first base and retain conventional trailing commas. Do not force a short single-base declaration to wrap. |
| `W6` | break after return type; preserve compact signature | **compact signature** | `BreakAfterReturnType: None` and a high return-type break penalty. |
| `W7` | trailing operator; leading operator | **leading non-assignment operator** | `BreakBeforeBinaryOperators: NonAssignment`, `BreakBeforeTernaryOperators: true`; long expressions read as an explicit sequence of operations. |
| `W8` | allow packing very long braced lists; always one per line | **always one per line once wrapped** | `BinPackLongBracedList: false`; large descriptor and shader-data tables remain reviewable. |

### 5. Alignment and spacing

| ID | Options | Selected | Exact option/result |
| --- | --- | --- | --- |
| `A1` | align consecutive assignments; do not align | **do not align** | `AlignConsecutiveAssignments: None`; changing one identifier does not move neighboring lines. |
| `A2` | align consecutive declarations; do not align | **do not align** | `AlignConsecutiveDeclarations: None`. |
| `A3` | align macros; do not align | **do not align** | `AlignConsecutiveMacros: None`; use a narrowly justified format-off table only when column structure carries meaning. |
| `A4` | align trailing comments; preserve; never align | **never align** | `AlignTrailingComments: { Kind: Never }`; avoids diff cascades. |
| `A5` | left/right/do-not-align escaped macro newlines | **left** | `AlignEscapedNewlines: Left`; retains readable multi-line shader-authoring macro bodies without aligning unrelated declarations. |
| `A6` | fixed continuation; align; align after operator | **fixed continuation** | `AlignOperands: DontAlign`; used with leading non-assignment operators to avoid stair-step alignment and diff churn. |
| `A7` | pointer/reference left; right; mixed | **left/left** | `PointerAlignment: Left`, `ReferenceAlignment: Left`, `DerivePointerAlignment: false`. |
| `A8` | normalize qualifier order; leave source order | **leave** | `QualifierAlignment: Leave`; clang-format warns that qualifier rewriting can be semantically unsafe. |
| `A9` | C++ cast gap; no gap | **C++ gap, shader no gap** | Root `SpaceAfterCStyleCast: true`; shader override `false`, preserving normal HLSL `(float)(expression)` spelling. |
| `A10` | spaces before all parentheses; control statements only; none | **control statements only** | `SpaceBeforeParens: ControlStatements`. |
| `A11` | spaces inside delimiters; none | **none** | No spaces inside parentheses, brackets, angle brackets, C-style casts, or container literals. |

### 6. Includes, comments, blank lines, and file hygiene

| ID | Options | Selected | Exact option/result |
| --- | --- | --- | --- |
| `F1` | sort/regroup includes; sort within blocks; preserve exact order | **preserve exact order** | `SortIncludes: Never`, `IncludeBlocks: Preserve`; include order remains an owned compile-boundary decision. |
| `F2` | sort using declarations; preserve | **preserve** | `SortUsingDeclarations: Never`; removes the current inherited reordering behavior. |
| `F3` | reflow comments; indentation only; untouched | **C++ indentation only, shader untouched** | Root `ReflowComments: IndentOnly`; shader override `ReflowComments: Never`. This protects formulas, diagrams, resource tables, and vendor integration notes. |
| `F4` | maintain namespace comments; leave them; prohibit them | **prohibit and remove** | `FixNamespaceComments: false` prevents additions. clang-format cannot delete existing comments, so the coding standard and dedicated migration/check own removal. |
| `F5` | retain up to two blank lines; one | **one** | `MaxEmptyLinesToKeep: 1`; logical stages remain separated without vertical drift. Keep empty lines at the start of blocks disabled. |
| `F6` | derive line endings; force LF | **force LF** | `LineEnding: LF`, `InsertNewlineAtEOF: true`; pair later with a precise `.gitattributes` LF policy. |
| `F7` | automatically separate definition blocks; leave intentional spacing | **leave** | `SeparateDefinitionBlocks: Leave`; the coding-style standard's acquire/transform/commit/publish staging remains authored, not guessed. |

### 7. C++20 and shader-specific syntax

| ID | Options | Selected | Exact option/result |
| --- | --- | --- | --- |
| `M1` | `Latest`; `Auto`; `c++20` | **`c++20`** | `Standard: c++20`; matches CMake and makes parsing reproducible. |
| `M2` | templates/concepts attached; templates/concepts separated when multiline | **keep short constraints attached** | `BreakTemplateDeclarations: No`, `BreakBeforeConceptDeclarations: Never`, and `RequiresClausePosition: WithPreceding`. Width still forces unavoidable wrapping. |
| `M3` | keep current HLSL attribute placement; manually place attributes on their own line | **manually place attributes on their own line** | The shader override records `BreakAfterAttributes: Always`, but clang-format's C++ parser does not recognize single-bracket HLSL attributes. A source check/review must enforce `[numthreads]`, `[loop]`, and `[unroll]` placement. |
| `M4` | assume HLSL language support; treat shader formatting as guarded C++ parsing | **guarded C++ parsing** | clang-format 22 does not list HLSL as a supported language. Use a path-specific inherited C++ profile and validate every syntax family. |
| `M5` | skip macro definition bodies; format and validate them | **format and validate** | `SkipMacroDefinitionBody: false`, `AlignEscapedNewlines: Left`; include the shader-parameter and global-shader registration macros in the C++ pilot. |

## Accepted Profile Snapshot

This table summarizes the accepted executable profile. The complete option spelling lives in `.clang-format` and the shader override.

| Area | Root C++ selection | Shader override |
| --- | --- | --- |
| Tool | clang-format 22.1.3 | same tool |
| Standard/parser | C++20 | C++ parser applied cautiously to HLSL/HLSLI |
| Width | 140 | 140 |
| Indent | tabs, width 4 | inherit |
| Braces | Allman | inherit |
| Namespace/case/preprocessor | indent all/indent labels/indent before hash | inherit |
| Access modifiers | offset `-4` | not material |
| Wrapped lists | one per line | inherit |
| Short functions | in-class only | none expected |
| Short lambdas | inline callback arguments | none expected |
| Operators | leading non-assignment, fixed continuation | inherit |
| Column alignment | no assignment/declaration/macro/trailing-comment columns | inherit |
| Includes | preserve blocks and exact order | inherit |
| Comments | indent only; prohibit namespace-end comments | no reflow; prohibit namespace-end comments |
| Attributes | leave | own-line HLSL convention; no reliable parser control |
| C-style cast gap | yes | no |
| Structural mutation | disabled | disabled |
| File hygiene | LF, final newline, max one empty line | inherit |

## Options That Should Remain Off

These options are not useful style preferences for this repository; they perform risky or ownership-obscuring changes:

- qualifier reordering;
- parenthesis removal;
- brace removal;
- automatic brace insertion by clang-format;
- include sorting or regrouping;
- using-declaration sorting;
- automatic namespace compaction;
- automatic integer-literal separator insertion/removal;
- automatic trailing-comma insertion;
- formatting macro bodies without an audited macro vocabulary;
- broad `OneLineFormatOffRegex` exclusions.

The existing clang-tidy configuration already treats readability checks, including braces-around-statements, as errors. Keep syntax enforcement there and keep clang-format focused on whitespace and line layout.

## Shader Policy

### Why a separate profile is warranted

clang-format 22 documents C, C++, Java, JavaScript, JSON, Objective-C, Protobuf, C#, TableGen, TextProto, and Verilog-related modes, but not HLSL. A `.hlsl` filename is therefore handled through the C++ parser/fallback path. The current file's `Language: Cpp` confirms the effective style seen by shader input.

Most existing shader syntax is C++-like enough to format successfully, including namespaces, structs, `cbuffer`, resource declarations, semantics, vector constructors, and ordinary control flow. Success without a parser error is not proof of semantic preservation. Sensitive constructs include:

- `[numthreads]`, `[loop]`, and `[unroll]` attributes;
- HLSL semantics after `:`;
- `cbuffer`, resource, sampler, and `groupshared` declarations;
- scalar casts such as `(float)(expression)`;
- preprocessor-controlled BRDF and ray-tracing paths;
- register/space annotations when introduced;
- ray-query and future Slang-specific syntax;
- comments that document shader math or ABI layout.

### Accepted shader override

Use `Engine/Assets/Shaders/.clang-format` with `BasedOnStyle: InheritParentConfig`. Override only:

- `ColumnLimit: 140`;
- `ReflowComments: Never`;
- `SpaceAfterCStyleCast: false`;
- `BreakAfterAttributes: Always` to record the intended own-line attribute convention, with a separate check because clang-format 22.1.3 does not enforce it for HLSL syntax.

Preprocessor indentation, include preservation, Allman braces, tabs, namespace indentation, and one-per-line wrapping should remain shared unless the shader pilot demonstrates a concrete parser or readability problem.

### Shader acceptance gate

Before formatting the tracked owned shader manifest:

1. select a corpus covering compute, deferred, ray tracing, BRDF, resource ABI, nested preprocessor conditionals, semantics, and attributes;
2. capture the pre-format source identities, compiled DXIL/SPIR-V blobs, reflection, package keys, and binding-layout hashes;
3. format the corpus with the pinned tool and inspect the full diff;
4. compile through the repository's actual DXC/Slang cook path for D3D12 and Vulkan;
5. require identical reflection and binding-layout contracts;
6. investigate binary differences rather than assuming whitespace is inert;
7. expand to all shaders only after the corpus passes;
8. use narrow `clang-format off/on` regions only for syntax that the formatter demonstrably damages, with a reason on the off marker.

## Remaining Rollout

The 2026-08-30 migration pass completed the following repository-owned pieces:

1. `CMake/CodeStyle.ps1` now provides pinned check and format entry points over a tracked owned-source manifest, with matching `code_style_check` and `code_style_format` CMake targets.
2. The check covers namespace-end comments, anonymous namespaces, multiple inheritance, and inline HLSL attributes in addition to clang-format drift.
3. A no-write inventory was reviewed by module and source family. At committed revision `61fe39d9`, the manifest contains 1,919 tracked C/C++ files and 120 tracked shader files after excluding D3D12 third-party source.
4. The C/C++ subset was migrated with clang-format 22.1.3, existing namespace-end comments were removed, and the check-only C/C++ pass is clean. Existing Launcher implementation edits were preserved rather than replaced or reverted.
5. Post-format `DevelopmentEditor` builds passed for Core, Tasks, Platform, GameFramework, SourceImporters, Launcher, AssetCooker, MaterialCooker, MeshCooker, SceneCooker, and ToolConsoleSupport. The architecture-boundary check also passed.

The shader and final enforcement steps remain open:

1. Establish a passing pre-format ShaderCompiler baseline. The current source build stops in existing RHI compilation errors, while the deployed ShaderCompiler artifact is stale and cannot resolve the current shader catalog. No shader source was formatted without this gate. The current no-write shader check reports formatter drift in 44 of 120 files and 79 authored-policy violations.
2. Run the DXIL/SPIR-V shader acceptance gate, normalize HLSL attributes, and migrate the shader subset only after its compile, reflection, binding-layout, and package-identity evidence passes.
3. Rebuild the remaining RHI-dependent owned targets, repeat the shader validation, and run applicable tests after the current RHI compilation defects are resolved. Generated test projects do not substitute for tests present and registered in source.
4. Enable CI check-only enforcement only after both source families have a clean accepted baseline. Until then, `-SourceFamily Cpp` is the passing migration check; the normal `All` check must continue to expose shader drift.

The repository-wide format remains a mechanical migration, not authorization for semantic cleanup. It was applied in an already dirty worktree only after explicit continuation approval. Committed revision `61fe39d9` contains both the pre-existing Launcher ownership change and the migration; the style tooling did not create that commit or a staging boundary.
