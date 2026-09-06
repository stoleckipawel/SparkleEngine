# Renderer Display Pipeline

Status: Renderer post-processing feature-family index

Scope: route scene-to-display transforms, explicitly absent display effects, output encoding, and target publication

| Document | Open it for |
| --- | --- |
| [Exposure](Exposure.md) | luminance measurement, history, manual/automatic exposure, and frame placement |
| [Tone Mapping](ToneMapping.md) | selectable scene-referred HDR to display-linear operators and their limits |
| [Color Grading](ColorGrading.md) | explicit negative capability boundary for grading and LUT workflows |
| [Chromatic Aberration](ChromaticAberration.md) | explicit negative capability boundary for the named lens effect |
| [Presentation And Output](PresentationAndOutput.md) | output encoding, format/HDR boundary, back-buffer or viewport publication, and debug handoff |

The parent [Post Processing](../README.md) dossier owns shared stage order. Each transformation retains a separate input/output and proof contract.
