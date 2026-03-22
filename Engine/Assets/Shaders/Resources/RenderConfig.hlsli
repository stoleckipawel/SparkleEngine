#pragma once

static const uint FRAMES_IN_FLIGHT = 2;

static const uint MAX_DIRECTIONAL_LIGHTS = 2;
static const uint MAX_SHADOW_CASCADES = 2;
static const uint MAX_SHADOW_MAPS = MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES;

static const uint SHADER_MODEL_MAJOR = 6;
static const uint SHADER_MODEL_MINOR = 0;
