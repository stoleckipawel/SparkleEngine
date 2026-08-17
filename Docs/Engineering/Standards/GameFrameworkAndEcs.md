# GameFramework and ECS

Status: binding domain integration standard

Applies to: world ownership, ECS storage, systems, loading, animation, publication, and render extraction

World, transform, animation, skinning, and extraction semantics must preserve the [World Coordinate, Units, and Transform Contract](../../Architecture/WorldCoordinateAndUnits.md).

## Ownership

- ECS storage remains private to GameFramework.
- `EntityId` is runtime identity; dense positions remain private and ephemeral.
- Components are data, not virtual update/render objects.
- Heavy assets remain immutable handle-addressed resources, not embedded mutable component ownership.
- World read views are immutable and generation-pinned.
- Renderer and Editor consume explicit derived products, never live world storage.

## Systems and Execution

- Systems declare phase, typed component reads/writes, non-ECS resource access, prerequisites, and execution/grain policy.
- Read/read may overlap; write hazards require an explicit order or graph rejection.
- Structural composition is frozen during queries.
- Workers use typed task-local command buffers; the world owner commits deterministically.
- Systems receive narrow views, not `GameWorld&`, registry access, a controller facade, renderer, UI, or platform services.
- Small workloads preserve the final-contract serial path; parallel work follows [Concurrency](Concurrency.md).

## Derived State and Extraction

- Transform and other derived state are evaluated explicitly; a published `const` view does not lazily mutate caches.
- Extraction is a bulk transform from a frozen world epoch into renderer-owned values or handles.
- Structural and dynamic data use distinct paths when their frequency and lifetime differ.
- Resolve asset-generation relationships and stable output slots before hot loops.
- Publication is immutable and one-way. A lagging incremental consumer has sequence/generation checks and a full-resync fallback.

## Animation

Decompose animation by operation and lifetime: sampling, pose evaluation, morph evaluation, skinning evaluation, output storage, transform, and extraction. Do not rebuild an animation god subsystem or blur stored joint matrices with the skinning operation; use the vocabulary in [Naming and Vocabulary](NamingAndVocabulary.md).

## Rejected Patterns

Do not introduce:

- parallel virtual `Entity::Update`;
- a public general-purpose ECS SDK;
- component pointers retained by panels or workers;
- structural mutation during iteration;
- completion-order entity allocation or command merge;
- GUID joins in hot queries;
- a world/controller facade that exposes arbitrary mutable execution;
- a second world representation that can mutate independently.

## Domain Review Questions

- Is there exactly one world/ECS authority?
- Are IDs stable while dense locations remain private?
- Are access declarations sufficient to derive hazards?
- Are structural changes deferred and deterministically committed?
- Are views prevented from escaping their epoch?
- Is render extraction explicit, immutable, and free of world pointers?
- Are animation operations and data lifetimes independently navigable?
