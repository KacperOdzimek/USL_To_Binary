#pragma once
#include "commons.h"

namespace Standards
{
	namespace S0utils
	{
		bool ContainsOnlyDigits(utils::TextPointer& src);
		void PassStringToFieldsBuffor(utils::TextPointer& str);
		std::vector<uint8_t> IntegerToBinary(utils::TextPointer& str);
		std::vector<uint8_t> FloatToBinary(utils::TextPointer& str);
		int TextToInt(utils::TextPointer src);
		float TextToFloat(utils::TextPointer src);
		std::string VectorXScalarOperation(utils::TextPointer a_src, utils::TextPointer b_src, OperatorType_T op, Version* V);
		std::string VectorXVectorOperation(utils::TextPointer a_src, utils::TextPointer b_src, OperatorType_T op, Version* V);
	}
}