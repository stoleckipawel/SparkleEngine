#include "../../PCH.h"
#include "Passes/Presentation/OutputEncodingCVars.h"

ConsoleVariable<EngineOutputColorEncoding> CVarOutputColorEncoding(
    "r.OutputColorEncoding",
    EngineOutputColorEncoding::Automatic,
    "Final SDR output encoding. 0=automatic from back buffer format, 1=linear, 2=sRGB encode in shader.");
