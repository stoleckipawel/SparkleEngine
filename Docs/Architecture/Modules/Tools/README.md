# Tools Module Architecture

Status: Tools module index

This index mirrors the durable tool ownership boundaries under `Tools`.

| Module | Owns | Module documentation |
| --- | --- | --- |
| Cooking | project, scene, mesh, material, texture, and animation cooking | [Cooking](Cooking/README.md) |
| Launcher | discovery, readiness, configure/build/cook/acquire/run/clean workflows, cancellation, and handoff | [Launcher](Launcher/README.md) |
| ShaderCompiler | shader compilation, validation, publication, recook, and runtime delivery contracts | [ShaderCompiler](ShaderCompiler/README.md) |
| SourceImporters | source-format ingestion, transforms, materials, animation, validation, and known losses | [SourceImporters](SourceImporters/README.md) |
| ToolSupport | shared command-line presentation and progress contracts used by tools | [ToolSupport](ToolSupport/README.md) |

Tool-specific implementation rules live in [Tools Engineering](../../../Engineering/Modules/Tools.md); interactive UI rules live in [Editor Engineering](../../../Engineering/Modules/Editor.md). Product consumers remain under [Projects](../Projects/README.md).
