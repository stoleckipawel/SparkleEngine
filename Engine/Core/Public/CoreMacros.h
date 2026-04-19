#pragma once

#define SPARKLE_PP_CONCAT_INNER(a, b) a##b
#define SPARKLE_PP_CONCAT(a, b) SPARKLE_PP_CONCAT_INNER(a, b)
