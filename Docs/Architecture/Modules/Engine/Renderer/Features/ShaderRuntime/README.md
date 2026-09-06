# Renderer Shader Runtime

Status: Renderer feature-family index

Scope: route exact shader-program membership and the runtime materialization, binding, generation, and retirement contract

| Document | Open it for |
| --- | --- |
| [Shader Program Catalog](ShaderProgramCatalog.md) | exact registered program, source, entry, stage, target, and declared-metadata membership |
| [Pipeline Materialization And Typed Binding](PipelineMaterializationAndTypedBinding.md) | runtime ABI validation, complete pipeline identity, typed binding, cache behavior, generation activation, and retirement |

The catalog proves membership only. Runtime materialization owns readiness and use; it cannot infer a usable pipeline from a catalog row. The parent [Renderer Feature Dossiers](../README.md) index owns capability routing.
