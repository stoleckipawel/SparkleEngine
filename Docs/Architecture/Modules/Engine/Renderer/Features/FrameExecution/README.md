# Renderer Frame Execution

Status: Renderer feature-family index

Scope: route the independently maintained contracts that turn admitted frame state into scheduled work, coherent temporal state, and correctly ordered latency coordination

| Document | Open it for |
| --- | --- |
| [Frame Graph And Scheduling](FrameGraphAndScheduling.md) | graph declaration, compilation, queues, dependencies, resource lifetime, execution, and completion |
| [Temporal Sampling And History](TemporalSamplingAndHistory.md) | per-view sample identity, jitter, previous transforms, invalidation, motion, reprojection, and temporal consumers |
| [Latency Coordination](LatencyCoordination.md) | simulation/render markers, provider readiness, ordering, fallback, and measurable latency boundaries |

These contracts share frame identity and ordering but remain separate because graph topology, temporal continuity, and latency-provider coordination have different owners and proof obligations. The parent [Renderer Feature Dossiers](../README.md) index owns capability routing.
