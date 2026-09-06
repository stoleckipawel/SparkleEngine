# Renderer Presentation And Output

Status: current feature dossier; source-backed, not colorimetric, numerical, display, capture, backend, or release evidence

Verified: 2026-09-06 against source revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Scope: `REN-POST-08` through `REN-POST-10`; debug-to-presentation handoff, output encoding, back-buffer copy, viewport-product publication, and the explicit absence of HDR-display output

Parent family: [Post Processing](../README.md); [Tone Mapping](ToneMapping.md) owns the preceding HDR-to-display-linear transform

## Feature Promise

Sparkle accepts one output-extent display-linear result from tone mapping, applies one output encoding, then copies it to the imported back buffer or publishes an offscreen viewport product.

```text
ResolvedSceneColor
  -> optional Debug Views replacement
  -> Tone Mapping
  -> Output Encoding
  -> back-buffer copy or FinalSceneColor viewport product
```

| Stage | Choices | Current boundary |
| --- | --- | --- |
| Debug handoff | Lit or one of 15 diagnostic modes | [Debug Views](../../DebugViews/README.md) owns modes/products; current diagnostics still enter the common exposure/tone/output chain |
| Output encoding | Automatic from output format, Linear, sRGB shader encoding | writes the linear counterpart of the presentation format before copy/publication |
| HDR display | none | no PQ, scRGB, HDR10 metadata, display-nit contract, or HDR swapchain negotiation |

The current debug handoff makes `REN-POST-10` Partial: bounded, false-color, and already-preview-mapped diagnostics can be changed by exposure/tone/encoding. The [Debug View Presentation Architecture](../../DebugViews/PresentationArchitecture.md) defines a target scene-referred versus exact display-linear split; source does not yet implement it.

## Output Ownership

- Viewport ID 0 imports the presentable back buffer and copies encoded color into it.
- Nonzero viewport requests retain `FinalSceneColor`; `SceneDepth` and `Normals` may also be exported as viewport products.
- Product identity carries viewport-request generation, extent, and format. Capture provenance remains incomplete for shader/graph generation and requested-versus-resolved product/color interpretation.
- UI composition happens after graph execution and is owned by [UI and Viewport Composition](../../ViewportAndDiagnostics/UiAndViewportComposition.md), not by the color-conversion passes.
- Frame generation is absent; one executed render frame leads to the normal submission/present route without a generated-frame owner.

## Failure, Tradeoffs, And Evidence

- Invalid encoding selection, unsupported HDR request, wrong format pairing, double encoding, clipping, NaN/Inf, and stale output identity must be visible failures.
- One common chain prevents each lighting/provider path from inventing a color pipeline, but exact diagnostics need an explicit presentation-domain contract.
- `REN-E17` owns numerical mapper/encoding combinations; `REN-E18` owns debug presentation; `RHI-E04` and `RHI-E12` own format and present behavior. The feature-local contract is defined below.

## Acceptance Criteria

- `AC-OUT-01` — Automatic, Linear, and sRGB choices resolve deterministically from the selected output format and encode a pinned display-linear ramp exactly once within declared tolerance.
- `AC-OUT-02` — the output-encoding target uses the required linear-format counterpart and the final copy/publication preserves channel order, alpha policy, dimensions, and viewport rectangle.
- `AC-OUT-03` — viewport 0 reaches the imported presentable back buffer; nonzero viewports publish generation-bound final color and requested depth/normal products without cross-viewport or stale-generation reuse.
- `AC-OUT-04` — resize, minimize/restore, output-format change, viewport destruction/recreation, and scene reload rebuild/invalidate the right products and never present a stale prior image as current.
- `AC-OUT-05` — unsupported PQ/scRGB/HDR10/display-nit/HDR-swapchain requests remain explicitly unavailable; Linear encoding is not advertised as HDR display support.
- `AC-OUT-06` — current debug modes are described and captured as passing through the common tone/output chain until the target exact-presentation contract is implemented.
- `AC-OUT-07` — D3D12 and Vulkan decoded back-buffer/offscreen products agree with the numerical oracle and produce no uncategorized native presentation/format validation issue.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-OUT-01` | invalid encoding or incompatible format pair | graph/settings resolution rejects before copy/present and names both values | `CHK-OUT-01` |
| `FM-OUT-02` | stale viewport/product generation after resize/recreate | product/draw/capture is refused or retired; stale image is not current output | `CHK-OUT-02` |
| `FM-OUT-03` | minimize/zero extent or unavailable back buffer | no out-of-bounds dispatch/copy occurs; recovery rebuilds on valid extent | `CHK-OUT-02` |
| `FM-OUT-04` | NaN/Inf/extreme display-linear input or double encoding | numerical check exposes the declared policy or fails the candidate | `CHK-OUT-01` |
| `FM-OUT-05` | HDR display setting/claim introduced without complete contract | selector/source/package audit rejects the support claim | `CHK-OUT-03` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-OUT-01` | shader/copy readback of known ramps over encoding × output-format × alpha/extreme-value cells | `AC-OUT-01`, `AC-OUT-02`, `AC-OUT-07`; `FM-OUT-01`, `FM-OUT-04` |
| `CHK-OUT-02` | swapchain and two offscreen viewports through resize, zero extent, minimize/restore, format change, destruction/recreation, reload, and capture | `AC-OUT-03`, `AC-OUT-04`; `FM-OUT-02`, `FM-OUT-03` |
| `CHK-OUT-03` | inspect selectors, RHI swapchain formats/metadata, package/runtime UI, debug captures, and documentation for HDR/exact-debug claims | `AC-OUT-05`, `AC-OUT-06`; `FM-OUT-05` |
| `CHK-OUT-04` | paired D3D12/Vulkan presentation/offscreen run with decoded artifacts and native validation | `AC-OUT-02`, `AC-OUT-03`, `AC-OUT-07` |

This contract is **defined but unproved**. Passing SDR encoding/publication does not close `REN-POST-09` or the target `REN-POST-10` exact-presentation work.

## Primary Source Routes

- [`Presentation.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp)
- [`OutputEncodingSettings.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Presentation/OutputEncodingSettings.cpp)
- [`PostProcessing.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp)
