# Plans

Status: implementation-plan navigation index

Plans own ordered delivery, dependencies, clean-break slices, stop conditions, and validation sequence. Placement follows the architecture owner: module-owned plans stay under that module category; truly shared delivery stays under CrossModule. Plans do not prove completion.

## Plan Groups

| Scope | Open it when... |
| --- | --- |
| [Renderer](Renderer/README.md) | delivery is primarily owned by `Engine/Renderer`, even when RHI or Editor consumers participate |
| [CrossModule](CrossModule/README.md) | delivery requires coordinated changes across several owners and cannot be expressed as one module's plan |
| [Capability Evidence](CapabilityEvidence.md) | closing evidence for source-present capabilities across the repository |

The release-wide sequence remains a strategy responsibility in the [Release-First Principal Graphics Roadmap](../Strategy/Roadmap.md). A plan's presence here is not authorization to start it. Every active phase begins with the [Change Lifecycle iteration record](../Engineering/Workflow/ChangeLifecycle.md#create-the-iteration-control-record), including its `NS-*`, `PGE-*`, roadmap/acceptance, `FCR-*`, risk, acceptance, failure, and key-check mappings.

Deferred GBuffer decals and geometry-cache animation remain unscheduled until `REL-11` plus an explicit later roadmap admission, as their linked acceptance contracts state. Other plans execute only when the current release gate selects them; they do not compete with the release WIP limit merely because they are documented.

An offline path-tracer implementation plan is intentionally absent. The [completion study](../Research/GraphicsArchitecture/OfflinePathTracerCompletion.md), [`PTD-00` discovery contract](../Architecture/Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/Discovery.md), and [eventual feature acceptance contract](../Architecture/Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/README.md) are the planning inputs; `PTD-00` must pass before `PTD-01` may add that plan, and its later presence will not itself authorize implementation.
