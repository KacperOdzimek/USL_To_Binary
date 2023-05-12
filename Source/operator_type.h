#pragma once
#include <cstdint>

enum class OperatorType_T : uint8_t
{
	add, sub, mul, div, pow, vec2, vec3, vec4, get, array_get, matrix_get
};