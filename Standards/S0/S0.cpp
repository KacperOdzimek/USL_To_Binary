#include "S0.h"
#include "utils.h"
#include "../../Source/compiling_task.h"
#include "../../Source/commons.h"

#include <string>

#define Temp Version::AccessTemp()

using namespace Standards;
using namespace S0utils;

using HeaderEntryType = USL_Translator::TranslationResult::HeaderEntry::Type;

void AddFunction(const char* name, const char* return_type, std::vector<const char*> args_types, Version* target)
{
	auto tp_name = utils::TextPointer::Get((char*)name);
	auto tp_return_type = utils::TextPointer::Get((char*)return_type);
	int return_id = target->FindTypeIdFromName(tp_return_type);
	std::vector<int> args;
	for (auto& arg : args_types)
		args.push_back(target->FindTypeIdFromName(utils::TextPointer::Get((char*)arg)));

	target->built_in_functions.push_back({tp_name, { return_id, args }});
}

namespace Standards
{
	Version* S0Create()
	{
		Version* V = new Version;

		/*
			Compiling Conditions
		*/

		V->AddCondition("ContainsVertexShader",  "Lack of vertex shader.");
		V->AddCondition("ContainsPixelShader",   "Lack of pixel shader.");
		V->AddCondition("VertexLayoutSpecified", "Lack of vertex layout specification.");

		V->AddCondition("VertexReturn",			 "Vertex shader must return vertex position.");
		V->AddCondition("PixelReturn",			 "Pixel shader must return pixel color.");

		/*
			Basic Types
		*/

		V->AddType("int",   4,  utils::IsInteger, IntegerToBinary);
		V->AddType("float", 4,  utils::IsFloat, FloatToBinary);

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
		V->AddType("vec2", V->GetTypeSize("float") * 2,
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

		V->AddType("vec3", V->GetTypeSize("float") * 3,
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

		V->AddType("vec4", V->GetTypeSize("float") * 4,
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

		V->AddType("array", 0,
			[](utils::TextPointer& src)	   {return false; }, 
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{};});


		V->AddType("buffer",	   0, [](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("texture_1d",   0, [](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("texture_2d",   0, [](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("texture_3d",   0, [](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("texture_cube", 0, [](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("texture_2d_ms", 0, [](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });


		V->AddType("mat2x2", V->GetTypeSize("float") * 2 * 2, 
			[](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("mat2x3", V->GetTypeSize("float") * 2 * 3,
			[](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("mat2x4", V->GetTypeSize("float") * 2 * 4, 
			[](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("mat3x2", V->GetTypeSize("float") * 3 * 2, 
			[](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("mat3x3", V->GetTypeSize("float") * 3 * 3, 
			[](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("mat3x4", V->GetTypeSize("float") * 3 * 4, 
			[](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("mat4x2", V->GetTypeSize("float") * 4 * 2, 
			[](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("mat4x3", V->GetTypeSize("float") * 4 * 3, 
			[](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

		V->AddType("mat4x4", V->GetTypeSize("float") * 4 * 4, 
			[](utils::TextPointer& src) {return false; },
			[](utils::TextPointer& to_bin) {return std::vector<uint8_t>{}; });

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
		V->AllowPrecomputation(#type_a, #type_b, #type_return, OperatorType_T::operato, true,											\
			[V](utils::TextPointer& a_src, utils::TextPointer& b_src) {																	\
			return ModNumbers(TextToFloat(a_src), TextToFloat(b_src), operator_char);													\
			});																															\

#define ScalarPrecomputationBundle(type_a, type_b, type_return)																			\
		ScalarPrecomputation(type_a, type_b, type_return, add, +)																		\
		ScalarPrecomputation(type_a, type_b, type_return, sub, -)																		\
		ScalarPrecomputation(type_a, type_b, type_return, mul, *)																		\
		ScalarPrecomputation(type_a, type_b, type_return, div, /)																		\
		V->AllowPrecomputation(#type_a, #type_b, #type_return, OperatorType_T::pow,	true,												\
		[V](utils::TextPointer& a_src, utils::TextPointer& b_src) {																		\
		return Power(TextToFloat(a_src), TextToFloat(b_src));																			\
			});																															\

		ScalarPrecomputationBundle(int, int, int)
			ScalarPrecomputationBundle(float, float, float)
			ScalarPrecomputationBundle(int, float, float)


#define VectorPrecomputation(vec_type, sec_type, operato)																					\
		V->AllowPrecomputation(#vec_type, #sec_type, #vec_type, OperatorType_T::operato, true,												\
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

#define VectorXVectorPrecomputation(vec_type, operato)											\
		V->AllowPrecomputation(#vec_type, #vec_type, #vec_type, OperatorType_T::operato, true,	\
			[V](utils::TextPointer& a_src, utils::TextPointer& b_src) {							\
			return std::pair<int, std::string>{V->FindTypeIdFromName({ (char*)#vec_type, 4 }),  \
			VectorXVectorOperation(a_src, b_src, OperatorType_T::operato, V)};					\
			});																					\

#define VectorXVectorPrecomputationsBundle(vec_name)	\
		VectorXVectorPrecomputation(vec_name, add)		\
		VectorXVectorPrecomputation(vec_name, sub)		\
		VectorXVectorPrecomputation(vec_name, mul)		\
		VectorXVectorPrecomputation(vec_name, div)		\
		VectorXVectorPrecomputation(vec_name, pow)		\

			VectorXVectorPrecomputationsBundle(vec2)
			VectorXVectorPrecomputationsBundle(vec3)
			VectorXVectorPrecomputationsBundle(vec4)

			V->AllowOperation("mat2x2", "mat2x2", "mat2x2", OperatorType_T::add, true);
			V->AllowOperation("mat3x3", "mat3x3", "mat3x3", OperatorType_T::add, true);
			V->AllowOperation("mat4x4", "mat4x4", "mat4x4", OperatorType_T::add, true);

			V->AllowOperation("mat2x2", "mat2x2", "mat2x2", OperatorType_T::sub, true);
			V->AllowOperation("mat3x3", "mat3x3", "mat3x3", OperatorType_T::sub, true);
			V->AllowOperation("mat4x4", "mat4x4", "mat4x4", OperatorType_T::sub, true);

			for (auto& m1 : { "mat2x2", "mat2x3", "mat2x4" , "mat3x2" , "mat3x3" , "mat3x4", "mat4x2" , "mat4x3" , "mat4x4" })
			{
				V->AllowOperation(m1, "int",   m1, OperatorType_T::mul, true);
				V->AllowOperation(m1, "float", m1, OperatorType_T::mul, true);
				for (auto& m2 : { "mat2x2", "mat2x3", "mat2x4" , "mat3x2" , "mat3x3" , "mat3x4", "mat4x2" , "mat4x3" , "mat4x4" })
				{
					if (int(m1[3] - '0') == int(m2[5] - '0'))
					{
						int columns = int(m1[3] - '0') > int(m2[3] - '0') ? int(m1[3] - '0') : int(m2[3] - '0');
						int rows = int(m1[5] - '0') > int(m2[5] - '0') ? int(m1[5] - '0') : int(m2[5] - '0');
						std::string name = "mat" + std::to_string(columns) + 'x' + std::to_string(rows);
						V->AllowOperation(m1, m2, name.c_str(), OperatorType_T::mul, false);
					}
				}
			}

			V->AllowOperation("mat2x2", "vec2", "vec2", OperatorType_T::mul, true);
			V->AllowOperation("mat2x3", "vec2", "vec2", OperatorType_T::mul, true);
			V->AllowOperation("mat2x4", "vec2", "vec2", OperatorType_T::mul, true);

			V->AllowOperation("mat3x2", "vec3", "vec3", OperatorType_T::mul, true);
			V->AllowOperation("mat3x3", "vec3", "vec3", OperatorType_T::mul, true);
			V->AllowOperation("mat3x4", "vec3", "vec3", OperatorType_T::mul, true);

			V->AllowOperation("mat4x2", "vec4", "vec4", OperatorType_T::mul, true);
			V->AllowOperation("mat4x3", "vec4", "vec4", OperatorType_T::mul, true);
			V->AllowOperation("mat4x4", "vec4", "vec4", OperatorType_T::mul, true);

#undef VectorXVectorPrecomputationsBundle
#undef VectorXVectorPrecomputation

#undef VectorPrecomputation
#undef VectorPrecomputationsBundle

#undef ScalarPrecomputation
#undef ScalarPrecomputationBundle

#undef IsTotal
#undef ModNumbers

		/*
			Shaders Return Types
		*/
		V->SetShadersReturnTypes({
			V->FindTypeIdFromName({ (char*)"vec4", 4 }),
			V->FindTypeIdFromName({ (char*)"vec4", 4 })
		});

		/*
			Signatures
		*/

		V->AddSignature("define ?n:", { Context_t::GlobalScope }, [V]()
			{
				if (Temp->writed_anything)
					Temp->SignatureWritedFunctionErrors.push_back("Define must be the first line of the code");
				if (Temp->NamesBuffor[0] == utils::TextPointer{(char*)"shader", 6})
				{
					Temp->FileType = FileType::Shader;
				}
				else if (Temp->NamesBuffor[0] == utils::TextPointer{ (char*)"library", 7 })
				{
					Temp->FileType = FileType::Library;
					Temp->Context = Context_t::Library;
					Temp->CompilationConditions.at("VertexLayoutSpecified") = true;
					Temp->CompilationConditions.at("ContainsVertexShader") = true;
					Temp->CompilationConditions.at("ContainsPixelShader") = true; 
					Temp->CompilationConditions.at("VertexReturn") = true;
					Temp->CompilationConditions.at("PixelReturn") = true;
				}
				else
				{
					Temp->SignatureWritedFunctionErrors.push_back("Invalid option");
				}
			});

		V->AddSignature("VertexShader:", { Context_t::GlobalScope }, [V]() 
			{
				if (Temp->CompilationConditions.at("ContainsVertexShader"))
					Temp->SignatureWritedFunctionErrors.push_back("Shader program can contain only one vertex shader");

				if (!Temp->CompilationConditions.at("VertexLayoutSpecified"))
					Temp->SignatureWritedFunctionErrors.push_back("Vertex shader cannot be created if vertex layout is not specified");

				Temp->CompilationConditions.at("ContainsVertexShader") = true;
				Temp->Deepness++;
				Temp->Context = Context_t::Shader;
				Temp->ShaderType = ShaderType_t::VertexShader;
				Temp->RequestedReturnType = V->FindTypeIdFromName({ (char*)"vec4", 4 });

				utils::TextPointer name((char*)"Vertex", 6);
				Temp->Variables.push_back({name, {Temp->layout_type_id, 1}});

				for (auto& ext : Temp->ExternVariables)
				{
					Temp->Variables.push_back({ ext.first, {ext.second, 1} });

					Compiling_Temp::Array* array_obj = nullptr;
					for (auto& ea : Temp->extern_arrays)
						if (ea.first == ext.first)
							array_obj = &ea.second;

					if (ext.second == V->FindTypeIdFromName({ (char*)"array", 5 }))
						Temp->arrays.insert({ Temp->Variables.size() - 1, *array_obj });
				}
			});

		V->AddSignature("PixelShader:", { Context_t::GlobalScope }, [V]()
			{
				if (Temp->CompilationConditions.at("ContainsPixelShader"))
					Temp->SignatureWritedFunctionErrors.push_back("Shader program can contain only one pixel shader");

				if (!Temp->CompilationConditions.at("ContainsVertexShader"))
					Temp->SignatureWritedFunctionErrors.push_back("Pixel shader must be definied after the vertex shader");

				Temp->CompilationConditions.at("ContainsPixelShader") = true;

				Temp->Deepness++;
				Temp->Context = Context_t::Shader;
				Temp->ShaderType = ShaderType_t::PixelShader;

				for (auto& ext : Temp->ExternVariables)
				{
					Temp->Variables.push_back({ ext.first, {ext.second, 1} });

					Compiling_Temp::Array* array_obj = nullptr;
					for (auto& ea : Temp->extern_arrays)
						if (ea.first == ext.first)
							array_obj = &ea.second;

					if (ext.second == V->FindTypeIdFromName({ (char*)"array", 5 }))
						Temp->arrays.insert({ Temp->Variables.size() - 1, *array_obj });
				}

				Temp->RequestedReturnType = V->FindTypeIdFromName({ (char*)"vec4", 4 });
			});

		V->AddSignature("GeometryShader:", { Context_t::GlobalScope }, [V]()
			{

				if (!Temp->CompilationConditions.at("ContainsVertexShader"))
					Temp->SignatureWritedFunctionErrors.push_back("Geometry shader must be definied after the vertex shader");

				if (Temp->CompilationConditions.find("ContainsGeometryShader") != Temp->CompilationConditions.end())
					Temp->SignatureWritedFunctionErrors.push_back("Shader program can contain only one geometry shader");
				else
					Temp->CompilationConditions.insert({"ContainsGeometryShader", true});

				if (Temp->geometry_shader_input_primitive_id == -1)
					Temp->SignatureWritedFunctionErrors.push_back(
						"Geometry shader cannot be created if input primitive is not specified");
				if (Temp->geometry_shader_output_primitive_id == -1)
					Temp->SignatureWritedFunctionErrors.push_back(
						"Geometry shader cannot be created if output primitive is not specified");
				if (Temp->geometry_shader_output_vertices_limit == -1)
					Temp->SignatureWritedFunctionErrors.push_back(
						"Geometry shader cannot be created if vertices limit is not specified");

				if (Temp->geometry_shader_input_primitive_id != -1
					&& Temp->geometry_shader_output_vertices_limit != -1
					&& Temp->geometry_shader_output_primitive_id != -1)
				{
					Temp->Deepness++;
					Temp->Context = Context_t::Shader;
					Temp->ShaderType = ShaderType_t::GeometryShader;
					Temp->RequestedReturnType = V->FindTypeIdFromName({ (char*)"vec4", 4 });

					int array_size;
					switch (Temp->geometry_shader_input_primitive_id)
					{
					case 0: array_size = 1; break;
					case 1: array_size = 2; break;
					case 2: array_size = 3; break;
					}

					utils::TextPointer name((char*)"Vertices", 8);

					Temp->Variables.push_back({ name, {V->FindTypeIdFromName({ (char*)"array", 5 }), 1} });
					Temp->arrays.insert({ 0, {V->FindTypeIdFromName({ (char*)"vec4", 4 }), array_size} });

					for (auto& ext : Temp->ExternVariables)
					{
						Temp->Variables.push_back({ ext.first, {ext.second, 1} });

						Compiling_Temp::Array* array_obj = nullptr;
						for (auto& ea : Temp->extern_arrays)
							if (ea.first == ext.first)
								array_obj = &ea.second;

						if (ext.second == V->FindTypeIdFromName({ (char*)"array", 5 }))
							Temp->arrays.insert({ Temp->Variables.size() - 1, *array_obj });
					}
				}
			});

		//return instruction
		V->AddSignature("return ?r", { Context_t::Shader, Context_t::CustomFunction }, 
			[V]()
			{
				switch (Temp->ShaderType)
				{
				case ShaderType_t::VertexShader: Temp->CompilationConditions.at("VertexReturn") = true; break;
				case ShaderType_t::PixelShader:  Temp->CompilationConditions.at("PixelReturn") = true;  break;
				}
			}
		);

		//Variable declaration without initialization
		V->AddSignature("?t ?n", { Context_t::Shader, Context_t::StructDeclaration, Context_t::CustomFunction, Context_t::Library }, []()
			{
				bool struct_declaration = Temp->Context == Context_t::StructDeclaration;

				if (struct_declaration)
				{
					for (auto& member : Temp->Structs.back().second.Members)
						if (member.first == Temp->NamesBuffor[0])
							goto var_dec_name_error;
					Temp->Structs.back().second.Members.push_back({ Temp->NamesBuffor[0], Temp->FieldsBuffor[0] });

					if (Temp->FileType == FileType::Library)
					{
						Temp->pass_to_binary_buffor.push_back(Temp->NamesBuffor.front().length);
						Temp->pass_to_binary_buffor.insert(
							Temp->pass_to_binary_buffor.end(),
							Temp->NamesBuffor.front().begin,
							Temp->NamesBuffor.front().begin + Temp->NamesBuffor.front().length
						);
					}
				}
				else if (Temp->Context == Context_t::Library)
				{
					Temp->SignatureWritedFunctionErrors.push_back("All variables definied in a library have to be initialized");
					return;
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

		auto declare_var = [V]()
		{
			if (!Temp->IsVarValiding(Temp->NamesBuffor[0]))
			{
				Temp->Variables.push_back({ Temp->NamesBuffor[0], {Temp->FieldsBuffor[0], Temp->Deepness} });

				if (Temp->FileType == FileType::Library)
				{
					Temp->pass_to_binary_buffor.push_back(Temp->NamesBuffor.front().length);
					Temp->pass_to_binary_buffor.insert(
						Temp->pass_to_binary_buffor.end(),
						Temp->NamesBuffor.front().begin,
						Temp->NamesBuffor.front().begin + Temp->NamesBuffor.front().length
					);
					Temp->pass_last_binary_index_to_declarations_positions = true;
				}
			}
			else
			{
				std::string error = "Variable ";
				for (int i = 0; i < Temp->NamesBuffor[0].length; i++)
					error += (*(Temp->NamesBuffor[0].begin + i));
				Temp->SignatureWritedFunctionErrors.push_back(error + " already exists");
			}
		};

		//Variable declaration with initialization by literal
		V->AddSignature("?t ?n = ?l", { Context_t::Shader, Context_t::CustomFunction, Context_t::Library }, declare_var);

		//Variable declaration with initialization by math expression
		V->AddSignature("?t ?n = ?e", { Context_t::Shader, Context_t::CustomFunction, Context_t::Library }, declare_var);

		//Array declaration without initialization
		V->AddSignature("?t ?n [ ?i ]", { Context_t::Shader, Context_t::CustomFunction }, [V]()
			{
				if (!Temp->IsVarValiding(Temp->NamesBuffor[0]))
				{
					Temp->Variables.push_back({ Temp->NamesBuffor[0],
						{V->FindTypeIdFromName({ (char*)"array", 5 }), Temp->Deepness} });

					auto ptr = &Temp->FieldsBuffor[1];
					std::vector<uint8_t> as_bin = { *(ptr + 3), *(ptr + 2), *(ptr + 1), *(ptr + 0) };
					int i;
					memcpy(&i, &(*as_bin.begin()), 4);

					std::string str = std::to_string(i);

					Temp->arrays.insert({ (int)Temp->Variables.size() - 1, {(int)Temp->FieldsBuffor[0], i} });
				}			
				else
				{
					std::string error = "Variable ";
					for (int i = 0; i < Temp->NamesBuffor[0].length; i++)
						error += (*(Temp->NamesBuffor[0].begin + i));
					Temp->SignatureWritedFunctionErrors.push_back(error + " already exists");
				}
			});

		//Array declaration with initialization
		V->AddSignature("?t ?n [ ?i ] = ?a", { Context_t::Shader, Context_t::CustomFunction }, [V]()
			{
				if (!Temp->IsVarValiding(Temp->NamesBuffor[0]))
				{
					Temp->Variables.push_back({ Temp->NamesBuffor[0],
						{V->FindTypeIdFromName({ (char*)"array", 5 }), Temp->Deepness} });

					auto ptr = &Temp->FieldsBuffor[1];
					std::vector<uint8_t> as_bin = { *(ptr + 3), *(ptr + 2), *(ptr + 1), *(ptr + 0) };
					int i;
					memcpy(&i, &(*as_bin.begin()), 4);

					std::string str = std::to_string(i);

					Temp->arrays.insert({ (int)Temp->Variables.size() - 1, {(int)Temp->FieldsBuffor[0], i} });
				}
				else
				{
					std::string error = "Variable ";
					for (int i = 0; i < Temp->NamesBuffor[0].length; i++)
						error += (*(Temp->NamesBuffor[0].begin + i));
					Temp->SignatureWritedFunctionErrors.push_back(error + " already exists");
				}
			});

		//Struct declaration
		V->AddSignature("struct ?n", { Context_t::GlobalScope, Context_t::Library }, []()
			{
				Temp->Context = Context_t::StructDeclaration;
				Temp->Deepness += 1;
				Temp->Structs.push_back({ Temp->NamesBuffor[0], (uint8_t)Temp->Structs.size() });

				if (Temp->FileType == FileType::Library)
				{
					Temp->pass_to_binary_buffor.push_back(Temp->NamesBuffor.front().length);
					Temp->pass_to_binary_buffor.insert(
						Temp->pass_to_binary_buffor.end(),
						Temp->NamesBuffor.front().begin,
						Temp->NamesBuffor.front().begin + Temp->NamesBuffor.front().length
					);
					Temp->pass_last_binary_index_to_declarations_positions = true;
				}
			});

		//Function declaration
		V->AddSignature("?t ?n ?f", { Context_t::GlobalScope, Context_t::Library }, []()
			{
				Temp->Context = Context_t::CustomFunction;
				Temp->Deepness += 1;

				Temp->RequestedReturnType = Temp->FieldsBuffor[0];

				for (auto& ext : Temp->ExternVariables)
					Temp->Variables.push_back({ ext.first, {ext.second, 1} });

				std::vector<int> Args_Types;

				//Add function parameters to variables list
				for (int i = 2; i < Temp->FieldsBuffor.size(); i++)
				{
					Temp->Variables.push_back({ Temp->NamesBuffor[i - 1], {Temp->FieldsBuffor[i], Temp->Deepness} });
					Args_Types.push_back(Temp->FieldsBuffor[i]);
				}

				if (Temp->FileType == FileType::Library)
				{
					Temp->pass_to_binary_buffor.push_back(Temp->NamesBuffor.front().length);
					Temp->pass_to_binary_buffor.insert(
						Temp->pass_to_binary_buffor.end(),
						Temp->NamesBuffor.front().begin, 
						Temp->NamesBuffor.front().begin + Temp->NamesBuffor.front().length
					);
					Temp->pass_last_binary_index_to_declarations_positions = true;
				}

				for (auto& func : Temp->FunctionsHeaders)
				{
					if (func.first == Temp->NamesBuffor.front() && Args_Types.size() == func.second.ArgumentsTypes.size())
					{
						bool all_equal = true;
						for (int i = 0; i < Args_Types.size(); i++)
						{
							if (Args_Types.at(i) != func.second.ArgumentsTypes.at(i))
							{
								all_equal = false;
								break;
							}
						}
						if (all_equal)
						{
							if (func.second.ReturnType != Temp->FieldsBuffor[0])
								Temp->SignatureWritedFunctionErrors.push_back("Function cannot be overloaded only by the returned type");
							else
								Temp->SignatureWritedFunctionErrors.push_back("Function with identical interface already exsist");
						}
					}
				}

				//Give compiler an information about function's args types and return type
				Temp->FunctionsHeaders.push_back({ Temp->NamesBuffor.front(), {Temp->FieldsBuffor[0], Args_Types} });
			});

		//Declare vertex layout
		V->AddSignature("using layout ?t", { Context_t::GlobalScope }, [V]()
			{
				if (Temp->CompilationConditions.at("VertexLayoutSpecified"))
					Temp->SignatureWritedFunctionErrors.push_back("using layout can be used only once per shader");
				else
				{
					Temp->CompilationConditions.at("VertexLayoutSpecified") = true;
					Temp->layout_type_id = Temp->FieldsBuffor.at(0);

					auto type_name = V->FindTypeNameFromId(Temp->FieldsBuffor[0]);
					std::string name_str;
					name_str.insert(name_str.end(), type_name.begin, type_name.begin + type_name.length);

					Temp->data_for_header.insert({ "vertex_layout", {HeaderEntryType::Value, {name_str}}});
				}
			});

		//Add uniform
		V->AddSignature("using extern ?t ?n", { Context_t::GlobalScope }, [V]()
			{
				if (!Temp->CompilationConditions.at("VertexLayoutSpecified"))
					Temp->SignatureWritedFunctionErrors.push_back("using extern cannot be used until vertex layout is specified");
				else if (!Temp->IsExternValiding(Temp->NamesBuffor[0]))
				{
					std::string name_str;
					name_str.insert(name_str.end(), Temp->NamesBuffor[0].begin, 
						Temp->NamesBuffor[0].begin + Temp->NamesBuffor[0].length);

					if (Temp->data_for_header.find("extern") == Temp->data_for_header.end())
						Temp->data_for_header.insert({ "extern", {HeaderEntryType::Array, {name_str}} });
					else
						Temp->data_for_header.at("extern").content.push_back(name_str);

					Temp->ExternVariables.push_back({ Temp->NamesBuffor[0], Temp->FieldsBuffor[0] });
				}
				else
				{
					std::string error = "Extern variable ";
					for (int i = 0; i < Temp->NamesBuffor[0].length; i++)
						error += (*(Temp->NamesBuffor[0].begin + i));
					Temp->SignatureWritedFunctionErrors.push_back(error + " already exists");
				}
			});

		//Add uniforms array
		V->AddSignature("using extern ?t ?n [ ?i ]", { Context_t::GlobalScope }, [V]()
			{
				if (!Temp->CompilationConditions.at("VertexLayoutSpecified"))
					Temp->SignatureWritedFunctionErrors.push_back("using extern cannot be used until vertex layout is specified");
				else if (!Temp->IsExternValiding(Temp->NamesBuffor[0]))
				{
					auto ptr = &Temp->FieldsBuffor[1];
					std::vector<uint8_t> as_bin = { *(ptr + 3), *(ptr + 2), *(ptr + 1), *(ptr + 0) };
					int i; memcpy(&i, &(*as_bin.begin()), 4);

					std::string name_str;
					name_str.insert(name_str.end(), Temp->NamesBuffor[0].begin,
						Temp->NamesBuffor[0].begin + Temp->NamesBuffor[0].length);

					if (Temp->data_for_header.find("array") == Temp->data_for_header.end())
						Temp->data_for_header.insert({ "array", {HeaderEntryType::Object, {name_str, std::to_string(i)}} });
					else
					{
						Temp->data_for_header.at("array").content.push_back(name_str);
						Temp->data_for_header.at("array").content.push_back(std::to_string(i));
					}

					Temp->ExternVariables.push_back({ Temp->NamesBuffor[0], Temp->FieldsBuffor[0] });

					Temp->ExternVariables.push_back({ Temp->NamesBuffor[0], V->FindTypeIdFromName({ (char*)"array", 5 })});
					Temp->extern_arrays.push_back({ Temp->NamesBuffor[0], {(int)Temp->FieldsBuffor[0], i} });
				}
				else
				{
					std::string error = "Extern variable ";
					for (int i = 0; i < Temp->NamesBuffor[0].length; i++)
						error += (*(Temp->NamesBuffor[0].begin + i));
					Temp->SignatureWritedFunctionErrors.push_back(error + " already exists");
				}
			});

		//Load texture
		V->AddSignature("using ?t ?n", { Context_t::GlobalScope }, [V]()
			{
				if (!Temp->CompilationConditions.at("VertexLayoutSpecified"))
					Temp->SignatureWritedFunctionErrors.push_back("using texture cannot be used until vertex layout is specified");
				else if (!Temp->IsExternValiding(Temp->NamesBuffor[0]))
				{
					if (Temp->FieldsBuffor[0] < V->FindTypeIdFromName({ (char*)"Buffor", 6 }))
						Temp->SignatureWritedFunctionErrors.push_back("invalid texture type");
					Temp->ExternVariables.push_back({ Temp->NamesBuffor[0], Temp->FieldsBuffor[0] });

					std::string name_str;
					name_str.insert(name_str.end(), Temp->NamesBuffor[0].begin,
						Temp->NamesBuffor[0].begin + Temp->NamesBuffor[0].length);

					if (Temp->data_for_header.find("texture") == Temp->data_for_header.end())
						Temp->data_for_header.insert({ "texture", {HeaderEntryType::Array, {name_str}} });
					else
						Temp->data_for_header.at("texture").content.push_back(name_str);
				}
				else
				{
					std::string error = "Extern variable ";
					for (int i = 0; i < Temp->NamesBuffor[0].length; i++)
						error += (*(Temp->NamesBuffor[0].begin + i));
					Temp->SignatureWritedFunctionErrors.push_back(error + " already exists");
				}
			});

		//Load library
		V->AddSignature("using library ?n", { Context_t::GlobalScope }, [V]()
			{
				std::string lib_name;
				lib_name.insert(lib_name.begin(), Temp->NamesBuffor[0].begin, Temp->NamesBuffor[0].begin + Temp->NamesBuffor[0].length);
				auto library_content = USL_Translator::USL_To_Binary::LEFC(0, lib_name);
				if (library_content.position == nullptr || library_content.size == 0)
					Temp->SignatureWritedFunctionErrors.push_back("Failed to load library " + lib_name);
				else
				{
					Temp->imported_libraries_names.push_back(lib_name);
					uint8_t* iterator = (uint8_t*)(library_content.position) + 2;
					uint16_t stuff_count;
					uint8_t s_count_parts[2] = { *iterator, *(iterator + 1) };
					memcpy(&stuff_count, &s_count_parts[0], 2);
					std::vector<uint16_t> positions;
					for (int i = 0; i < stuff_count; i++)
					{
						iterator += 2;
						uint16_t pos;
						uint8_t e_pos_parts[2] = { *iterator, *(iterator + 1) };
						memcpy(&pos, &e_pos_parts[0], 2);
						positions.push_back(pos);
					}

					auto get_lib_element_name = [&](int name_size, bool add_lib_name_prefix)
					{
						std::unique_ptr<std::string> function_name; 
						if (add_lib_name_prefix)
							function_name = std::make_unique<std::string>(lib_name + '.');
						else
							function_name = std::make_unique<std::string>("");

						++iterator;
						function_name->insert(function_name->end(), (char*)(iterator), (char*)(iterator)+name_size);

						return std::move(function_name);
					};

					for (auto& pos : positions)
					{
						iterator = (uint8_t*)(library_content.position) + pos + 4 + stuff_count * 2;
						if (*iterator == V->FindSignatureIdFromName("?t ?n ?f"))
						{
							int return_type_id = *(++iterator);
							int args_count = *(++iterator);
							std::vector<int> args;
							for (int i = 0; i < args_count; i++)
								args.push_back(*(++iterator));
				
							int name_size = *(++iterator);
							auto function_name = get_lib_element_name(name_size, true);
							Temp->ImportedStuffNames.push_back(std::move(function_name));

							int ptr_size = lib_name.size() + 1 + name_size;
							utils::TextPointer function_name_ptr{ &(Temp->ImportedStuffNames.back()->at(0)), ptr_size };

							Temp->FunctionsHeaders.push_back({ function_name_ptr, {return_type_id, args} });
						}
						else if (*iterator == V->FindSignatureIdFromName("struct ?n"))
						{
							int name_size = *(++iterator);
							auto struct_name = get_lib_element_name(name_size, true);
							Temp->ImportedStuffNames.push_back(std::move(struct_name));

							int ptr_size = lib_name.size() + 1 + name_size;
							utils::TextPointer struct_name_ptr{ &(Temp->ImportedStuffNames.back()->at(0)), ptr_size };

							iterator += name_size;

							Struct obj = {(uint8_t)Temp->Structs.size()};
							while (*iterator != 0 && iterator != ((uint8_t*)library_content.position) + library_content.size)
							{
								uint8_t member_type = *(++iterator);
								uint8_t member_name_size = *(++iterator);

								auto member_name = get_lib_element_name(member_name_size, false);
								Temp->ImportedStuffNames.push_back(std::move(member_name));

								utils::TextPointer member_name_ptr{ &(Temp->ImportedStuffNames.back()->at(0)), member_name_size };

								obj.Members.push_back({ member_name_ptr, (int)member_type });
							}
							Temp->Structs.push_back({ struct_name_ptr, obj });
						}
						else if (*iterator == V->FindSignatureIdFromName("?t ?n = ?l"))
						{
							uint8_t type = *(++iterator);
							int size = V->GetTypeSize(type);
							iterator += size;
							
							uint8_t name_size = *(++iterator);
							auto var_name = get_lib_element_name(name_size, true);
							Temp->ImportedStuffNames.push_back(std::move(var_name));

							utils::TextPointer var_name_ptr{ &(Temp->ImportedStuffNames.back()->at(0)), (int)lib_name.size() + 1 + name_size };
							Temp->ExternVariables.push_back({ var_name_ptr, type });
						}
						else if (*iterator == V->FindSignatureIdFromName("?t ?n = ?e"))
						{
							uint8_t type = *(++iterator);
							int exp_size = *(++iterator);
							iterator += exp_size;

							uint8_t name_size = *(++iterator);
							auto var_name = get_lib_element_name(name_size, true);
							Temp->ImportedStuffNames.push_back(std::move(var_name));

							utils::TextPointer var_name_ptr{ &(Temp->ImportedStuffNames.back()->at(0)), (int)lib_name.size() + 1 + name_size };
							Temp->ExternVariables.push_back({ var_name_ptr, type });
						}
					}

					Temp->pass_to_binary_buffor.push_back(lib_name.size());
					Temp->pass_to_binary_buffor.insert(Temp->pass_to_binary_buffor.end(), lib_name.begin(), lib_name.end());
				}
			});

		//Declare geometry shader vertices limit
		V->AddSignature("using geometry limit ?i", { Context_t::GlobalScope }, [V]()
			{
				auto ptr = &Temp->FieldsBuffor[0];
				std::vector<uint8_t> as_bin = { *(ptr + 3), *(ptr + 2), *(ptr + 1), *(ptr + 0) };
				int i; memcpy(&i, &(*as_bin.begin()), 4);
				Temp->geometry_shader_output_vertices_limit = i;
				if (i <= 0)
					Temp->SignatureWritedFunctionErrors.push_back("Vertices limit cannot be lower than 1");
			});

		//Declare geometry shader input primitive
		V->AddSignature("using input primitive ?n ?n", { Context_t::GlobalScope }, [V]()
			{
				if (Temp->NamesBuffor[0] == utils::TextPointer{ (char*)"point", 5 })
					Temp->geometry_shader_input_primitive_id = 0;
				else if (Temp->NamesBuffor[0] == utils::TextPointer{ (char*)"line", 4 })
					Temp->geometry_shader_input_primitive_id = 1;
				else if (Temp->NamesBuffor[0] == utils::TextPointer{ (char*)"triangle", 8 })
					Temp->geometry_shader_input_primitive_id = 2;
				else
					Temp->SignatureWritedFunctionErrors.push_back("No such primitive");
				Temp->pass_to_binary_buffor.push_back((uint8_t)Temp->geometry_shader_input_primitive_id);

				std::string name_str;
				name_str.insert(name_str.end(), Temp->NamesBuffor[0].begin,
					Temp->NamesBuffor[0].begin + Temp->NamesBuffor[0].length);

				Temp->data_for_header.insert({ "primitive", {HeaderEntryType::Value, {name_str}} });
			});

		//Declare geometry shader output primitive
		V->AddSignature("using output primitive ?n ?n", { Context_t::GlobalScope }, [V]()
			{
				if (Temp->NamesBuffor[0] == utils::TextPointer{ (char*)"point", 5 })
					Temp->geometry_shader_output_primitive_id = 0;
				else if (Temp->NamesBuffor[0] == utils::TextPointer{ (char*)"line", 4 } &&
					Temp->NamesBuffor[1] == utils::TextPointer{ (char*)"strip", 5 })
					Temp->geometry_shader_output_primitive_id = 1;
				else if (Temp->NamesBuffor[0] == utils::TextPointer{ (char*)"triangle", 8 } &&
					Temp->NamesBuffor[1] == utils::TextPointer{ (char*)"strip", 5 })
					Temp->geometry_shader_output_primitive_id = 2;
				else
					Temp->SignatureWritedFunctionErrors.push_back("No such primitive");
				Temp->pass_to_binary_buffor.push_back((uint8_t)Temp->geometry_shader_output_primitive_id);
			});

		//Finish drawing primitive
		V->AddSignature("FinishPrimitive", { Context_t::Shader }, [V]()
			{
				if (Temp->ShaderType != ShaderType_t::GeometryShader)
					Temp->SignatureWritedFunctionErrors.push_back("Invalid Context");
			});

		//Send value to another shader
		V->AddSignature("send ?t ?n = ?e", { Context_t::Shader }, [V]()
			{
				if (Temp->ShaderType != ShaderType_t::GeometryShader)
				{
					for (int i = 0; i < Temp->Sent.size(); i++)
						if (Temp->Sent[i].first == Temp->NamesBuffor[0])
						{
							std::string error = "Already sent value with name: ";
							for (int i = 0; i < Temp->NamesBuffor[0].length; i++)
								error += (*(Temp->NamesBuffor[0].begin + i));
							Temp->SignatureWritedFunctionErrors.push_back(error);
							break;
						}
					Temp->Sent.push_back({ Temp->NamesBuffor[0], {Temp->FieldsBuffor[0], Temp->ShaderType} });
				}
				else
				{
					bool add_var = true;
					uint8_t overwriten_sent_id = -1;
					for (int i = 0; i < Temp->Sent.size(); i++)
						if (Temp->Sent[i].first == Temp->NamesBuffor[0]
							&& Temp->Sent[i].second.second == ShaderType_t::GeometryShader)
						{
							overwriten_sent_id = i;
							add_var = false;
							break;
						}
							
					if (add_var)
						Temp->Sent.push_back({ Temp->NamesBuffor[0], {Temp->FieldsBuffor[0], Temp->ShaderType} });
					else
					{
						Temp->pass_to_binary_buffor.push_back(overwriten_sent_id);
						throw (int)1;
					}
				}
			});

		//Overwrite of send in geometry shader
		V->AddSignature("send ?t ?n = ?e", { Context_t::Shader }, [V]() {});

		//Catch value sent by other shader
		V->AddSignature("catch ?t ?n", { Context_t::Shader }, [V]()
			{
				for (int i = 0; i < Temp->Sent.size(); i++)
					if (Temp->Sent[i].first == Temp->NamesBuffor[0] 
						&& Temp->FieldsBuffor[0] == Temp->Sent[i].second.first)
					{
						if (Temp->Sent[i].second.second == ShaderType_t::VertexShader 
					&& Temp->ShaderType == ShaderType_t::PixelShader
					&& Temp->CompilationConditions.find("ContainsGeometryShader") != Temp->CompilationConditions.end())
							Temp->SignatureWritedFunctionErrors.push_back(
								"Cannot catch value sent in vertex shader in pixel shader if geometry shader exists");

						if (Temp->ShaderType == ShaderType_t::GeometryShader)
						{
							Temp->Variables.push_back({ Temp->NamesBuffor[0],
							{V->FindTypeIdFromName({ (char*)"array", 5 }), Temp->Deepness} });

							int array_size;
							switch (Temp->geometry_shader_input_primitive_id)
							{
							case 0: array_size = 1; break;
							case 1: array_size = 2; break;
							case 2: array_size = 3; break;
							}

							Temp->arrays.insert({ (int)Temp->Variables.size() - 1, {(int)Temp->FieldsBuffor[0], array_size
								} });
						}
						else
							Temp->Variables.push_back({ Temp->NamesBuffor[0], {Temp->FieldsBuffor[0], 1} });
						Temp->pass_to_binary_buffor.push_back(i);
					}
					else if (Temp->Sent[i].first == Temp->NamesBuffor[0])
					{
						Temp->SignatureWritedFunctionErrors.push_back(
							"The type specified in the catch statement does not match the type specified in the send statement");
					}
			});

		/*
			Built-in-functions
		*/

		//Distance function
		AddFunction("dst", "int",   { "int", "int" },   V);
		AddFunction("dst", "float", {"float", "float"}, V);
		AddFunction("dst", "vec2",  { "vec2", "vec2" },	V);
		AddFunction("dst", "vec3",  { "vec3", "vec3" }, V);
		AddFunction("dst", "vec4",  { "vec4", "vec4" },	V);

		AddFunction("round", "int", { "float" }, V);
		AddFunction("cell",  "int", { "float" }, V);
		AddFunction("floor", "int", { "float" }, V);

		AddFunction("sqrt", "float", { "float" }, V);

		AddFunction("sample", "vec4", { "texture_1d", "float" }, V);
		AddFunction("sample", "vec4", { "texture_2d", "vec2" },  V);
		AddFunction("sample", "vec4", { "texture_3d", "vec3" },  V);

		AddFunction("sample", "vec4", { "texture_cube",  "vec3" },		 V);
		AddFunction("sample", "vec4", { "texture_2d_ms", "vec3", "int"}, V);

		return V;
	}
}

#undef Temp