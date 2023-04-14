#include "compiling_temp.h"

bool Compiling_Temp::IsVarValiding(utils::TextPointer& var)
{
	for (auto& V : Variables)
		if (V.first == var)
			return 1;
	return 0;
}

bool Compiling_Temp::IsExternValiding(utils::TextPointer& ext)
{
	for (auto& V : ExternVariables)
		if (V.first == ext)
			return 1;
	return 0;
}

bool Compiling_Temp::IsStructValiding(utils::TextPointer& struc)
{
	for (auto& S : Structs)
		if (S.first == struc)
			return 1;
	return 0;
}

int Compiling_Temp::GetFunctionId(utils::TextPointer& func, std::vector<int> args_types)
{
	int error = -1;
	for (int i = 0; i < FunctionsHeaders.size(); i++)
		if (FunctionsHeaders[i].first == func)
		{
			if (args_types.size() < FunctionsHeaders[i].second.ArgumentsTypes.size()) error = -4;
			else if (args_types.size() < FunctionsHeaders[i].second.ArgumentsTypes.size()) error = -3;
			else
				for (int j = 0; j < args_types.size(); j++)
				{
					if (args_types[j] != FunctionsHeaders[i].second.ArgumentsTypes[j])
					{
						error = -2;
						break;
					}
				}
			return i;
		}
	return error;
}

std::pair<utils::TextPointer, std::pair<int, int>>* Compiling_Temp::GetVar(utils::TextPointer& var)
{
	for (auto& V : Variables)
		if (V.first == var)
			return &V;
	return nullptr;
}

int Compiling_Temp::GetVarId(utils::TextPointer& var)
{
	for (int i = 0; i < Variables.size(); i++)
		if (Variables[i].first == var)
			return i;
	return -1;
}