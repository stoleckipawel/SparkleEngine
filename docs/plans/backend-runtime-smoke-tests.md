# Backend Runtime Smoke Tests

This checklist records host-mode launch expectations for RHI backend parity validation.

Run smoke validation from the project directory so `.sparkle-project` discovery resolves cooked assets and logs correctly:

```powershell
Push-Location Projects\Showcase
..\..\build\bin\DevelopmentGame\ShowcaseRuntime.exe --rhi=D3D12
..\..\build\bin\DevelopmentGame\ShowcaseRuntime.exe --rhi=Vulkan
..\..\build\bin\DevelopmentEditor\ShowcaseEditor.exe --rhi=D3D12
..\..\build\bin\DevelopmentEditor\ShowcaseEditor.exe --rhi=Vulkan
Pop-Location
```

Backend selection is supported through both command-line switches and environment variables:

- `--rhi=D3D12`, `--rhi=Vulkan`, and `--graphics-api=` select the runtime backend for a single launch.
- `SPARKLE_RHI_BACKEND` selects the backend for automation runs that do not pass command-line switches.

Set `SPARKLE_SMOKE_VALIDATE_RHI=1` to run the automated runtime/editor smoke path. Optional controls include `SPARKLE_SMOKE_TRACE`, `SPARKLE_SMOKE_FRAME_LIMIT`, `SPARKLE_SMOKE_RESTORE_FRAME`, `SPARKLE_SMOKE_MAXIMIZE_FRAME`, and `SPARKLE_SMOKE_SHADER_RELOAD_FRAME`.

Expected log evidence:

- `Creating RHI backend` names the selected backend.
- `RHI smoke diagnostics capabilities` reports diagnostics support for the selected backend.
- `ShowcaseRuntime` launches without editor-host dependencies.
- `ShowcaseEditor` launches through the editor host and may exercise editor viewport smoke evidence.