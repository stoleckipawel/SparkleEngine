# External Capture Application Icons

Editor icon catalog artwork, not tool runtime dependencies. Each file is an unchanged
32x32 top-down, unpremultiplied RGBA8 application icon (4096 bytes), extracted on
2026-10-10 using Windows ExtractIconEx index 0/large icon and Icon.ToBitmap.
No crop, recoloring or generated substitute logo is used. Icons.json declares
names, extents and eligible profiles. Generic EditorIconAssets.cmake embeds
these pixels in the private per-configuration catalog; EditorIconService owns
their shared atlas resources. Runtime never reads installed tool executables. Application icons/trademarks remain the property of
their respective owners and identify the external tool without endorsement.

| Asset | Installed application identity | Executable SHA-256 | RGBA SHA-256 |
| --- | --- | --- | --- |
| pix.rgba | Microsoft PIX 2603.25 / WinPix.exe | `87476b75a9c8ebf83f53720a4d1b4a6797741b961b0e015355f46ce17bc24887` | `a49c08432940af6ca2fa9fbb3263ecd3f4295d04c4a2d43069e3cd060f120adf` |
| nsight.rgba | NVIDIA Nsight Graphics 2026.3.1 / ngfx-ui.exe | `d741e530f466ee87deaa764705fbb7cd588e390ae4a2fc2938e6e6a695bfa6dc` | `cd4747768f2b7596cc3276a356e19032868e278402736e214837b8760c00e4d5` |
| renderdoc.rgba | RenderDoc 1.46 / qrenderdoc.exe | `c5e9d3f254d80a7649fbd9eae1c67618cb19278c603710370c220de469397277` | `30bb8d041ce1c1424bdff03705974e348c64476fee15bd1961e4371feb33b091` |

Regeneration uses the same tool identities and extraction route, retains exact
source/pixel hashes, and reviews the resulting native toolbar. Shipping/Game
catalogs omit this collection, and capture action source is excluded. Full product/import/package
erasure remains a separate acceptance claim. Native capture and package acceptance
remain separate gates in the owning [dossier](../../../../../Docs/Architecture/CrossModule/PerformanceDiagnostics/ExternalCapture/README.md).

For asset addition, lifetime and client usage, see the owning
[Editor icon service](../../../../../Docs/Architecture/Modules/Engine/Editor/Icons.md).
