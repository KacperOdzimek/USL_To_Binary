#include "utils.h"
#include <cstring>

#define Temp Version::AccessTemp()

namespace Standards
{
	namespace S0utils
	{
		bool ContainsOnlyDigits(utils::TextPointer& src)
		{
			for (int i = 0; i != src.length; i++)
				if (!(src.begin[i] >= '0' && src.begin[i] <= '9'))
					return 0;
			return 1;
		}

		void PassStringToFieldsBuffor(utils::TextPointer& str)
		{
			int i = 0;
			while (i < str.length)
			{
				Temp->FieldsBuffor.push_back(str.begin[i]); i++;
			}
		}

		std::vector<uint8_t> IntegerToBinary(utils::TextPointer& str)
		{
			int32_t num = atoi(str.begin);
			std::vector<uint8_t> bin(4);
			for (int i = 0; i < 4; i++) bin[3 - i] = (num >> (i * 8));
			return bin;
		}

		std::vector<uint8_t> FloatToBinary(utils::TextPointer& str)
		{
			float num = atof(str.begin);
			uint8_t bytes[sizeof(num)];
			memcpy(&bytes[0], &num, sizeof(num));
			std::vector<uint8_t> bin;
			for (int i = sizeof(num) - 1; i > -1; i--) bin.push_back(bytes[i]);
			return bin;
		}

		//not using std::atoi because it's return 0 when there is a space after '-' eg. - 1
		int TextToInt(utils::TextPointer src)
		{
			bool negative = (src.begin[0] == '-');
			int offset = 0;
			if (negative)
			{
				do
					offset++;
				while (src.begin[offset] == ' ');
			}
			int abs = std::atoi(src.begin + offset);
			return abs * (negative ? -1 : 1);
		}

		float TextToFloat(utils::TextPointer src)
		{
			bool negative = (src.begin[0] == '-');
			int offset = 0;
			if (negative)
			{
				do
					offset++;
				while (src.begin[offset] == ' ');
			}
			float abs = std::atof(src.begin + offset);
			return abs * (negative ? -1 : 1);
		}

		std::string VectorXScalarOperation(utils::TextPointer a_src, utils::TextPointer b_src, OperatorType_T op, Version* V)
		{
			int a_t = V->FindTypeIdFromLiteral(a_src);
			int b_t = V->FindTypeIdFromLiteral(b_src);
			//If a type is int or float then extrude vector components from b otherwise from a
			auto args = (a_t == V->FindTypeIdFromName({ (char*)"int", 3 }) 
					  || a_t == V->FindTypeIdFromName({ (char*)"float", 5 })) ?
				utils::ExtrudeArguments(b_src.begin, utils::BracketsType::parentheses) :
				utils::ExtrudeArguments(a_src.begin, utils::BracketsType::parentheses);
			//If a type is int or float then set scalar to value of a otherwise b
			float scalar = (a_t == V->FindTypeIdFromName({ (char*)"int", 3 })
						 || a_t == V->FindTypeIdFromName({ (char*)"float", 5 })) ?
				TextToFloat(a_src) : TextToFloat(b_src);

			std::vector<char> result;
			result.push_back('(');
			for (auto& arg : args.first)
			{
				float vec_arg = TextToFloat(arg);
				float value = 0;
				switch (op)
				{
				case OperatorType_T::add:
					value = vec_arg + scalar; break;
				case OperatorType_T::sub:
					value = vec_arg - scalar; break;
				case OperatorType_T::mul:
					value = vec_arg * scalar; break;
				case OperatorType_T::div:
					value = vec_arg / scalar; break;
				case OperatorType_T::pow:
					value = std::pow(vec_arg, scalar); break;
				}
				auto as_string = std::to_string(value);
				for (auto& cha : as_string)
					result.push_back(cha);
				result.push_back(',');
			}
			result.push_back(')');
			return std::string(result.begin(), result.end());
		}

		std::string VectorXVectorOperation(utils::TextPointer a_src, utils::TextPointer b_src, OperatorType_T op, Version* V)
		{
			auto v1 = utils::ExtrudeArguments(a_src.begin, utils::BracketsType::parentheses).first;
			auto v2 = utils::ExtrudeArguments(b_src.begin, utils::BracketsType::parentheses).first;
			std::vector<float> return_vector;

			for (int i = 0; i < v1.size(); i++)
			{
				float arg1 = TextToFloat(v1.at(i));
				float arg2 = TextToFloat(v2.at(i));
				switch (op)
				{
				case OperatorType_T::add:
					return_vector.push_back(arg1 + arg2); break;
				case OperatorType_T::sub:
					return_vector.push_back(arg1 - arg2); break;
				case OperatorType_T::mul:
					return_vector.push_back(arg1 * arg2); break;
				case OperatorType_T::div:
					return_vector.push_back(arg1 / arg2); break;
				case OperatorType_T::pow:
					return_vector.push_back(std::pow(arg1, arg2)); break;
				}
			}

			std::string return_vector_str = "(";
			for (auto& num : return_vector)
			{
				std::string num_str = std::to_string(num);
				num_str.erase(num_str.find_last_not_of('0') + 1, std::string::npos);
				num_str.erase(num_str.find_last_not_of('.') + 1, std::string::npos);
				return_vector_str.insert(return_vector_str.end(), num_str.begin(), num_str.end());
			}
			return_vector_str += ')';
			return return_vector_str;
		}
	}
}

#undef Temp