#pragma once
#include "commons.h"
#include "USL_version.h"
#include "math_parser_utils.h"

namespace math_parser
{
	bool ParseMath(utils::TextPointer exp, int requested_type, Version* version, std::vector<std::string>& issues);
}