#include "S0.h"
#include "utils.h"
#include "../../Source/compiling_task.h"

#include <string>

#define Temp Version::AccessTemp()

using namespace Standards;
using namespace S0utils;

namespace Standards
{
	Version* S0Create()
	{
		Version* V = new Version;

		/*
			Compiling Conditions
		*/

		V->AddCondition("ContainsVertexShader",  "Lack of Vertex Shader!");
		V->AddCondition("ContainsPixelShader",   "Lack of Pixel Shader!");
		V->AddCondition("VertexLayoutSpecified", "Lack of Vertex Layout Specification!");

		/*
			Basic Types
		*/

		V->AddType("int", utils::IsInteger, IntegerToBinary);
		V->AddType("float", utils::IsFloat, FloatToBinary);

		V->SetVectorComponentsNames({ 'x', 'y', 'z', 'w' });
		V->SetGetComponentFromVectorFunction("float", 
			[V](utils::TextPointer& vector, uint8_t id)
			{
				auto args = utils::ExtrudeArguments(vector.begin, utils::BracketsType::parentheses).first;
				std::string result;
				for (int i = 0; i < args[id].length; i++)
				{
					result += *(args[id].begin + i);
				}
				return result;
			});	

		//Vectors with 2-4 dimensions
		V->AddType("vec2", 
			[V](utils::TextPointer& src) 
			{ 
				auto a = utils::ExtrudeArguments(src.begin, utils::BracketsType::parentheses);
				if (a.first.size() == 2)
					return IsFloat(a.first[0]) && IsFloat(a.first[1]);
				else
					return false;
			}, 
			[V](utils::TextPointer& to_bin)
			{
				auto args = utils::ExtrudeArguments(to_bin.begin, utils::BracketsType::parentheses);	
				auto x = V->ConvertLiteralToBinary(V->FindTypeIdFromName({ (char*)"float", 5 }), args.first[0]);
				auto y = V->ConvertLiteralToBinary(V->FindTypeIdFromName({ (char*)"float", 5 }), args.first[1]);
				x.insert(x.end(), y.begin(), y.end());
				return x;
			});

		V->AddType("vec3",
			[V](utils::TextPointer& src)
			{
				auto a = utils::ExtrudeArguments(src.begin, utils::BracketsType::parentheses);
				if (a.first.size() == 3)
					return IsFloat(a.first[0]) && IsFloat(a.first[1]) && IsFloat(a.first[2]);
				else
					return false;
			},
			[V](utils::TextPointer& to_bin)
			{
				auto args = utils::ExtrudeArguments(to_bin.begin, utils::BracketsType::parentheses);
				auto x = V->ConvertLiteralToBinary(V->FindTypeIdFromName({ (char*)"float", 5 }), args.first[0]);
				auto y = V->ConvertLiteralToBinary(V->FindTypeIdFromName({ (char*)"float", 5 }), args.first[1]);
				auto z = V->ConvertLiteralToBinary(V->FindTypeIdFromName({ (char*)"float", 5 }), args.first[2]);
				x.insert(x.end(), y.begin(), y.end());
				x.insert(x.end(), z.begin(), z.end());
				return x;
			});

		V->AddType("vec4",
			[V](utils::TextPointer& src)
			{
				auto a = utils::ExtrudeArguments(src.begin, utils::BracketsType::parentheses);
				if (a.first.size() == 4)
					return IsFloat(a.first[0]) && IsFloat(a.first[1]) && IsFloat(a.first[2]) && IsFloat(a.first[3]);
				else
					return false;
			},
			[V](utils::TextPointer& to_bin)
			{
				auto args = utils::ExtrudeArguments(to_bin.begin, utils::BracketsType::parentheses);
				auto x = V->ConvertLiteralToBinary(V->FindTypeIdFromName({ (char*)"float", 5 }), args.first[0]);
				auto y = V->ConvertLiteralToBinary(V->FindTypeIdFromName({ (char*)"float", 5 }), args.first[1]);
				auto z = V->ConvertLiteralToBinary(V->FindTypeIdFromName({ (char*)"float", 5 }), args.first[2]);
				auto w = V->ConvertLiteralToBinary(V->FindTypeIdFromName({ (char*)"float", 5 }), args.first[3]);
				x.insert(x.end(), y.begin(), y.end());
				x.insert(x.end(), z.begin(), z.end());
				x.insert(x.end(), w.begin(), w.end());
				return x;
			});

		/*
			Types Conversions
		*/

		V->AllowConversion("float", "int", [V](utils::TextPointer src) 
		{
			auto as_float = TextToFloat(src);
			return std::to_string(int(as_float));
		});

		V->AllowConversion("int", "float", [V](utils::TextPointer src)
			{
				float as_float = TextToInt(src);
				return std::to_string(as_float);
			});

		/*
			Basic Types Manipulations
		*/

//Return int type if r is total, float if not
#define IsTotal(r) V->FindTypeIdFromName({(char*)((r - int(r) == 0) ? "int" : "float"),		\
												uint64_t((r - int(r) == 0) ? 3 : 5) })		\

#define ModNumbers(a, b, op) std::pair<int, std::string>{IsTotal((double)a op (double)b),	\
								std::to_string((double)a op (double)b)}						\

#define Power(a, b) std::pair<int, std::string>{IsTotal(std::pow(a, b)),	\
								std::to_string(std::pow(a, b))}				\

#define ScalarPrecomputation(type_a, type_b, type_return, operato, operator_char)														\
		V->AllowPrecomputation(#type_a, #type_b, #type_return, OperatorType_T::operato,													\
			[V](utils::TextPointer& a_src, utils::TextPointer& b_src) {																	\
			return ModNumbers(TextToFloat(a_src), TextToFloat(b_src), operator_char);													\
			});																															\

#define ScalarPrecomputationBundle(type_a, type_b, type_return)																			\
		ScalarPrecomputation(type_a, type_b, type_return, add, +)																		\
		ScalarPrecomputation(type_a, type_b, type_return, sub, -)																		\
		ScalarPrecomputation(type_a, type_b, type_return, mul, *)																		\
		ScalarPrecomputation(type_a, type_b, type_return, div, /)																		\
		V->AllowPrecomputation(#type_a, #type_b, #type_return, OperatorType_T::pow,														\
		[V](utils::TextPointer& a_src, utils::TextPointer& b_src) {																		\
		return Power(TextToFloat(a_src), TextToFloat(b_src));																				\
			});																															\

		ScalarPrecomputationBundle(int, int, int)
		ScalarPrecomputationBundle(float, float, float)
		ScalarPrecomputationBundle(int, float, float)


#define VectorPrecomputation(vec_type, sec_type, operato)																					\
		V->AllowPrecomputation(#vec_type, #sec_type, #vec_type, OperatorType_T::operato,													\
			[V](utils::TextPointer& a_src, utils::TextPointer& b_src) {																		\
			return std::pair<int, std::string>{V->FindTypeIdFromName({ (char*)#vec_type, 4 }),												\
				VectorXScalarOperation(a_src, b_src, OperatorType_T::operato, V)};															\
			});																																\

#define VectorPrecomputationsBundle(vec_type, sec_type)			\
		VectorPrecomputation(vec_type, sec_type, add)			\
		VectorPrecomputation(vec_type, sec_type, sub)			\
		VectorPrecomputation(vec_type, sec_type, mul)			\
		VectorPrecomputation(vec_type, sec_type, div)			\
		VectorPrecomputation(vec_type, sec_type, pow)			\

		VectorPrecomputationsBundle(vec2, int)
		VectorPrecomputationsBundle(vec2, float)

		VectorPrecomputationsBundle(vec3, int)
		VectorPrecomputationsBundle(vec3, float)

		VectorPrecomputationsBundle(vec4, int)
		VectorPrecomputationsBundle(vec4, float)

#undef VectorPrecomputation
#undef VectorPrecomputationsBundle

#undef ScalarPrecomputation
#undef ScalarPrecomputationBundle

#undef IsTotal
#undef ModNumbers

		/*
			Signatures
		*/

		V->AddSignature("VertexShader:", { Context_t::GlobalScope }, []() 
			{
				if (Temp->CompilationConditions.at("ContainsVertexShader"))
					Temp->SignatureWritedFunctionErrors.push_back("Shader program can contain only one vertex shader");

				if (!Temp->CompilationConditions.at("VertexLayoutSpecified"))
					Temp->SignatureWritedFunctionErrors.push_back("Vertex shader program cannot be created when vertex layout is not specified");

				Temp->CompilationConditions.at("ContainsVertexShader") = true;
				Temp->Deepness++;
				Temp->Context = Context_t::Shader;

				utils::TextPointer name((char*)"Vertex", 6);
				Temp->Variables.push_back({name, {Temp->layout_type_id, 1}});
			});

		V->AddSignature("PixelShader:", { Context_t::GlobalScope }, []()
			{
				Temp->CompilationConditions.at("ContainsPixelShader") = true;
				Temp->Deepness++;
				Temp->Context = Context_t::Shader;
			});

		//return instruction
		V->AddSignature("return ?e", { Context_t::Shader, Context_t::CustomFunction }, nullptr);

		//Variable declaration without initialization
		V->AddSignature("?t ?n", { Context_t::Shader, Context_t::StructDeclaration, Context_t::CustomFunction }, []()
			{
				bool struct_declaration = Temp->Context == Context_t::StructDeclaration;

				if (struct_declaration)
				{
					//Add check
					for (auto& member : Temp->Structs.back().second.Members)
						if (member.first == Temp->NamesBuffor[0])
							goto var_dec_name_error;
					Temp->Structs.back().second.Members.push_back({ Temp->NamesBuffor[0], Temp->FieldsBuffor[0] });
				}
				else
				{
					if (Temp->IsVarValiding(Temp->NamesBuffor[0]))
						goto var_dec_name_error;
					Temp->Variables.push_back({ Temp->NamesBuffor[0], {Temp->FieldsBuffor[0], Temp->Deepness} });
				}
				return;
				var_dec_name_error:
				std::string error = struct_declaration ? "Member " : "Variable ";
				for (int i = 0; i < Temp->NamesBuffor[0].length; i++)
					error += (*(Temp->NamesBuffor[0].begin + i));
				Temp->SignatureWritedFunctionErrors.push_back(error + " already exists");
			});

		//Variable declaration with initialization
		V->AddSignature("?t ?n = ?l", { Context_t::Shader, Context_t::CustomFunction }, []()
			{
				if (!Temp->IsVarValiding(Temp->NamesBuffor[0]))
					Temp->Variables.push_back({ Temp->NamesBuffor[0], {Temp->FieldsBuffor[0], Temp->Deepness} });
				else
				{
					std::string error = "Variable ";
					for (int i = 0; i < Temp->NamesBuffor[0].length; i++)
						error += (*(Temp->NamesBuffor[0].begin + i));
					Temp->SignatureWritedFunctionErrors.push_back(error + " already exists");
				}
			});

		//Variable declaration with initialization by math expression
		V->AddSignature("?t ?n = ?e", { Context_t::Shader, Context_t::CustomFunction }, []()
			{
				if (!Temp->IsVarValiding(Temp->NamesBuffor[0]))
					Temp->Variables.push_back({ Temp->NamesBuffor[0], {Temp->FieldsBuffor[0], Temp->Deepness} });
				else
				{
					std::string error = "Variable ";
					for (int i = 0; i < Temp->NamesBuffor[0].length; i++)
						error += (*(Temp->NamesBuffor[0].begin + i));
					Temp->SignatureWritedFunctionErrors.push_back(error + " already exists");
				}	
			});

		//Struct declaration
		V->AddSignature("struct ?n", { Context_t::GlobalScope }, []()
			{
				Temp->Context = Context_t::StructDeclaration;
				Temp->Deepness += 1;
				Temp->Structs.push_back({ Temp->NamesBuffor[0], (uint8_t)Temp->Structs.size() });
			});

		//Function declaration
		V->AddSignature("?t ?n ?f", { Context_t::GlobalScope }, []()
			{
				Temp->Context = Context_t::CustomFunction;
				Temp->Deepness += 1;

				std::vector<int> Args_Types;

				//Add function parameters to variables list
				for (int i = 1; i < Temp->NamesBuffor.size(); i++)
				{
					Temp->Variables.push_back({ Temp->NamesBuffor[i], {Temp->FieldsBuffor[i], Temp->Deepness} });
					Args_Types.push_back(Temp->FieldsBuffor[i]);
				}

				//Give compiler an information about function's args types and return type
				Temp->FunctionsHeaders.push_back({ Temp->NamesBuffor.front(), {Temp->FieldsBuffor[0], Args_Types} });
			});

		V->AddSignature("using layout ?t", { Context_t::GlobalScope }, [V]()
			{
				if (Temp->CompilationConditions.at("VertexLayoutSpecified"))
					Temp->SignatureWritedFunctionErrors.push_back("using layout can be used only once per shader");
				else
				{
					Temp->CompilationConditions.at("VertexLayoutSpecified") = true;
					Temp->layout_type_id = Temp->FieldsBuffor.at(0);
				}
			});

		return V;
	}
}

#undef Temp