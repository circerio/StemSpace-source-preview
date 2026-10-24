#include "stdafx.h"

DECLARE_COMPONENT_VERSION("StemSpace", "0.1.0-beta.2",
    "AI-assisted stereo-to-7.1 foobar2000 DSP/upmixer. Windows x64.\n"
    "StableMix C: original Mid/Side front image; AI LFE is zero.\n"
    "Native non-stereo input passes through unchanged. Dedicated high-priority "
    "non-MMCSS inference worker, one ORT thread without spinning, "
    "one batched output per input and 250 ms start/flush prebuffer.\n"
    "Requires the qualified external model and ONNX Runtime; neither is included.");
VALIDATE_COMPONENT_FILENAME("foo_stemspace.dll");
