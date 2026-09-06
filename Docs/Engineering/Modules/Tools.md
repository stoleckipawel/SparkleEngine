# Tools Engineering

Status: binding Tools integration standard

Applies to: `Tools`, source import, cooking, shader compilation, Launcher operations, command-line workflows, publication, and tool background work

Tools transform source or user intent into deterministic products; runtime loading remains cooked-only. Tools preserve the canonical basis, units, spaces, and artifact representation defined by the [World Coordinate, Units, and Transform Contract](../../Architecture/Decisions/WorldCoordinateAndUnits.md). Owned output is regenerated from source when that contract changes; tools do not add compatibility versions, migration readers, or legacy artifacts.

Interactive tool frontends also follow the [intent-first workflow rules](Editor.md#intent-first-frontend-workflows); this document owns tool execution and publication rather than duplicating those UI rules.

## Operation Ownership

- Keep each operation's immutable request, progress, cancellation, result, and cleanup under one tool owner.
- Use `SparkleTasks` or an owned subprocess boundary according to the existing module path; do not add a tool-specific general thread pool.
- Progress is bounded and coalesced. Logs and replay detail remain available without becoming a second result authority.
- Cancellation settles owned work and preserves the previous accepted product.
- File/process work is isolated from frame-critical capacity.
- Concurrency has a weighted memory budget for HDR textures, scenes, compiler sessions, and third-party workers.

## Import, Cooking, Compilation, And Publication

- Separate read, decode, transform, validate, and transactional publication by real ownership.
- Runtime loading remains cooked-only.
- Outputs are deterministic and transactionally replaced.
- Validate the complete artifact set before publication; partial success does not replace the accepted generation.
- Cancellation or failure preserves the previous accepted artifact or world and reports one actionable root cause.
- Stable source/product identity crosses tool boundaries; internal worker, cache, temporary path, and native compiler details do not become public engine vocabulary.
- Do not add a second asset database, async loader family, shader-registration authority, or tool thread pool.

## Tool Review Questions

- Is the operation owned by the existing importer, cooker, compiler, launcher, or shared-support module?
- Are requests immutable and are progress, cancellation, failure, result, and cleanup explicit?
- Are imports and generated artifacts deterministic, validated, and transactionally published?
- Does failure preserve the previous accepted product and expose an actionable cause?
- Are runtime and tool authorities kept separate?
- Does an interactive frontend follow the shared intent-first workflow without exposing incidental backend machinery?
