#pragma once
#include "commons.h"
#include "compiler_utils.h"

namespace utils
{
	struct TextPointer;
}

/*
	Context describes, whats kinds of signatures we can use in given block of code
	like. for example what in Global Scope and what in Pixel Shader
*/
enum class Context_t
{
	GlobalScope, Shader, CustomFunction, StructDeclaration
};

enum class ShaderType_t : uint8_t
{
	VertexShader, PixelShader, GeometryShader
};

struct Struct
{
	uint8_t id;
	//Name; type id
	std::vector<std::pair<utils::TextPointer, int>> Members;
	Struct(uint8_t _id) : id(_id) {};
};

struct FunctionHeader
{
	int Return_Type;
	std::vector<int> ArgumentsTypes;
	FunctionHeader(int _Return_Type, std::vector<int> _ArgumentsTypes) :
		Return_Type(_Return_Type), ArgumentsTypes(_ArgumentsTypes) {};
};

/*
	This object contains all temporary data, that compiling process need.
	If you have to modify compiler and add some temporary variables - do it here
*/
struct Compiling_Temp
{
	/*
		Store argument fields identyficators
		If we found, that signature is matching
		it is appended to binary vector after signature id
	*/
	std::vector<uint8_t> FieldsBuffor{};
	/*
		Store names between signature's IsMatching function
		and writed function
	*/
	std::vector<utils::TextPointer> NamesBuffor{};
	/*
		List of all variables
		TextPointer is name
		pair first value is type
		pair second is deepness
	*/
	std::vector<std::pair<utils::TextPointer, std::pair<int, int>>> Variables{};
	std::vector<std::pair<utils::TextPointer, int>> ExternVariables;

	struct Array
	{
		int type;
		int size;
		Array(int _type, int _size) : type(_type), size(_size) {};
	};

	//Variable Id, Data
	std::map<int, Array> arrays;
	//Extern name, Data
	std::vector<std::pair<utils::TextPointer, Array>> extern_arrays;

	int geometry_shader_input_primitive_id  = -1;
	int geometry_shader_output_primitive_id = -1;
	int geometry_shader_output_vertices_limit = -1;

	//Name, Type, Sender
	std::vector<std::pair<utils::TextPointer, std::pair<int, ShaderType_t>>> Sent;
	/*
		Store information about, how deep nested is currency compiled
		block of code. It is used to track in which scope variable were created
		and when we have to delete it. Global, default level is 0
	*/
	unsigned int Deepness = 0;
	/*
		Store layout of user declared structs.
		Pair first is struct code name
	*/
	std::vector<std::pair<utils::TextPointer, Struct>> Structs;
	/*
		Id of type that represents layout of input
		Set by "using layout".
	*/
	int layout_type_id = 0;
	/*
		Store informations about functions headers
	*/
	std::vector<std::pair<utils::TextPointer, FunctionHeader>> FunctionsHeaders;
	/*
		If something is wrong running signature's writed function
		you can push error text into this vector
		it will be threated like regular error
	*/
	std::vector<std::string> SignatureWritedFunctionErrors;
	Context_t Context = Context_t::GlobalScope;
	ShaderType_t ShaderType = ShaderType_t::VertexShader;
	/*
		See Version::compilation_conditions
		key is condition name
		value is whatever condition were met
	*/
	std::map<std::string, bool> CompilationConditions;

	int RequestedReturnType = -1;

	//Can be used by signatures to pass external bytes to binary
	std::vector<uint8_t> pass_to_binary_buffor;

	bool IsVarValiding(utils::TextPointer& var);
	bool IsExternValiding(utils::TextPointer& ext);
	bool IsStructValiding(utils::TextPointer& struc);
	//-1 if not exists, -2 if exists but args types doesn't match
	//-3 if args set is too big, -4 if too small
	int GetFunctionId(utils::TextPointer& func, std::vector<int> args_types);
	std::pair<utils::TextPointer, std::pair<int, int>>* GetVar(utils::TextPointer& var);
	int GetVarId(utils::TextPointer& var);
};