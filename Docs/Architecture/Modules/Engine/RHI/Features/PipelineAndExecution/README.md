# RHI Pipeline And Execution

Status: RHI feature-family index

Scope: route immutable pipeline/shader contracts, command recording and submission, synchronization, and ray-tracing execution

| Document | Open it for |
| --- | --- |
| [Pipeline And Shader Contracts](PipelineAndShaderContracts.md) | shader/reflection ABI, complete pipeline identity, validation, caching, and backend lowering |
| [Command Submission And Synchronization](CommandSubmissionAndSynchronization.md) | recording leases, queues, barriers, submits, waits, completion tokens, and shutdown settlement |
| [Ray Tracing](RayTracing.md) | acceleration structures, traversal, SBT contracts, capability gates, providers, and lifetime |

Pipeline identity, queue execution, and ray-tracing capability remain independent contracts even though they meet during command recording. The parent [RHI Feature Dossiers](../README.md) index owns capability routing.
