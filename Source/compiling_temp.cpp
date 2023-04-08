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