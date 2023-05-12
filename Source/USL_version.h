#pragma once
#include "commons.h"
#include "compiling_temp.h"
#include "operator_type.h"
#include "math_parser.h"

#define Temp USL_Translator::USL_To_Binary::Temp

class Version
{
private:
	typedef std::function<void()> function;
	struct Signature;
	struct Type;
	int signatures_count = 0;

	std::vector<Signature*> Signatures{};
	std::vector<Type*> Types{};

public:
	~Version() { 
		for (Signature* s : Signatures) delete s;
		for (Type* t : Types) delete t;
	};

	static Compiling_Temp* AccessTemp() noexcept;

private:
	struct Type
	{
		friend Version;
		//Name of type like. int, float etc.
		const char* name;
		/*
			Function that checks if value, that pretends to by assigned
			to variable or arg. field of this type, is correct eg.
			for int a value 1 will be correct, but [1, 1, 1] won't
		*/
		std::function<bool(utils::TextPointer&) > verify;
		/*
			Function that converts value to binary
			results have to be writen directly to Temp::FieldsBuffer
		*/
		std::function<std::vector<uint8_t>(utils::TextPointer&)> to_binary;
		Type(const char* _name, std::function<bool(utils::TextPointer&)> _verify,
		std::function<std::vector<uint8_t>(utils::TextPointer&)> _to_binary) :
		name(_name), verify(_verify), to_binary(_to_binary) {};
	};
public:
	char vector_components_names[4];
	void SetVectorComponentsNames(char (&&_vector_components_names)[4])
	{
		for (int i = 0; i < 4; i++)
			vector_components_names[i] = _vector_components_names[i];
	}

	bool IsTypeValiding(const char* name);
	unsigned int TypeNameToTypeId(const char* name);
	//Return the id of matching type, else returns -1
	int FindTypeIdFromLiteral(utils::TextPointer literal)
	{
		for (int i = 0; i < Types.size(); i++)
			if (Types[i]->verify(literal))
				return i;
		return -1;
	}
	int FindTypeIdFromName(utils::TextPointer name)
	{
		for (int i = 0; i < Types.size(); i++)
			if (utils::TextPointer::Get((char*)Types[i]->name) == name)
				return i;
		return -1;
	}
	utils::TextPointer FindTypeNameFromId(int id)
	{
		if (id < Types.size())
		{
			auto* ptr = Types.at(id)->name;
			int size = 0;
			while (*(ptr + size) != '\0') ++size;
			return { (char*)ptr, size };
		}
		else
			return Temp->Structs.at(id - Types.size()).first;
	}
	
private:
	struct Signature
	{
	friend Version;

	private:
		const function				 Writed;
		const std::vector<Context_t> allowed_contexts;
	public:
		const char*			sig;
		const uint64_t		size;
		const unsigned int	id;

		template <uint64_t arr_size>
		Signature(const char (&_sig)[arr_size], const std::vector<Context_t> _allowed_contexts, const function _writed, Version* OwningVersion)
		: sig(_sig), Writed(_writed), size(arr_size - 1), id(OwningVersion->signatures_count + 1), allowed_contexts(_allowed_contexts)
		{ OwningVersion->Signatures.push_back(this); OwningVersion->signatures_count++; };
		//Inform translator about created variables, change of context etc.
		void LeaveTranslatorTips() { if (Writed != nullptr) return Writed(); }
	};

	//Check if given set of initializing args is correct
	bool verify_struct(Struct& s, const char* args_src);
	//Converts each of struct's members to binary form
	void structs_to_binary(Struct& s, const char* args_src);

public:
	struct IsMatchingResult
	{
		bool matching;
		bool all_conditions_met;
		std::vector<std::string> issues;
		IsMatchingResult(bool _matching, bool _all_conditions_met, std::vector<std::string> _issues) :
			matching(_matching), all_conditions_met(_all_conditions_met), issues(_issues) {};
	};

private:
	IsMatchingResult IsMatching(char* t, int s, Signature* sig);

public:
	struct MatchResult
	{
		bool success;
		Signature* matching_signature;
		std::vector<Version::IsMatchingResult> matching_but_wrong;
	};

	MatchResult Match(char* source, uint64_t size);

	template <uint64_t arr_size>
	void AddSignature(const char(&_sig)[arr_size], const std::vector<Context_t> _allowed_contexts, const function _writed)
	{
		new Signature( _sig, _allowed_contexts, _writed, this );
	}

	template <uint64_t arr_size>
	void AddType(
		const char(&_name)[arr_size], 
		std::function<bool(utils::TextPointer&)> _verify,
		std::function<std::vector<uint8_t>(utils::TextPointer&)> _to_binary)
		{
    		auto T = new Type(_name, _verify, _to_binary);
    		Types.push_back(T);
		}

private:
	//Function that handle basic mathematical operations on basic types literals
	struct PrecomputationFunction
	{
		//returned type id, literal, right arg, left arg
		using function = std::function<std::pair<int, std::string>(utils::TextPointer&, utils::TextPointer&)>;
		const int Type1;
		const int Type2;
		const int ReturnType;
		bool commutative_types;
		const OperatorType_T operato;
		const function func;
		PrecomputationFunction(int _Type1, int _Type2, int _ReturnType,
			OperatorType_T _operato,
			function _func, bool _commutative_types)
			: Type1(_Type1), Type2(_Type2), ReturnType(_ReturnType),
			operato(_operato), func(_func), commutative_types(_commutative_types) {};
	};

	std::vector<PrecomputationFunction> precomputation_functions;

public:
	void AllowPrecomputation(const char* _first_type,
						const char* _second_type, const char* _return_type,
						OperatorType_T _operator, bool commutative_types,
						PrecomputationFunction::function _func)
	{
		int type1 = FindTypeIdFromName(utils::TextPointer::Get((char*)_first_type));
		int type2 = FindTypeIdFromName(utils::TextPointer::Get((char*)_second_type));
		int ret   = FindTypeIdFromName(utils::TextPointer::Get((char*)_return_type));
		PrecomputationFunction op = PrecomputationFunction{
			type1, type2, ret,
			_operator, _func, commutative_types
		};
		precomputation_functions.push_back(op);
	}

	//Mathemathical operation that is allowed but doesn't have precomputation function
	struct AllowedOperation
	{
		int type_1;
		int type_2;
		int return_type;
		bool commutative_types;
		OperatorType_T operato;
		AllowedOperation(int _type_1, int _type_2, int _r_type, OperatorType_T _operato, bool _commutative_types)
			: type_1(_type_1), type_2(_type_2), return_type(_r_type), operato(_operato), commutative_types(_commutative_types) {};
	};
	std::vector<AllowedOperation> allowed_operations;
	void AllowOperation(const char* _first_type,
		const char* _second_type, const char* _return_type, 
		OperatorType_T operato, bool commutative_types)
	{
		int type1 = FindTypeIdFromName(utils::TextPointer::Get((char*)_first_type));
		int type2 = FindTypeIdFromName(utils::TextPointer::Get((char*)_second_type));
		int ret   = FindTypeIdFromName(utils::TextPointer::Get((char*)_return_type));
		AllowedOperation ap = AllowedOperation{
			type1, type2, ret, operato, commutative_types
		};
		allowed_operations.push_back(ap);
	}

	//return - new literal; args - vector && component id
	using GetVectorComponetFunctionType = std::function<std::string(utils::TextPointer&, uint8_t)>;

	//Return type / function
	std::pair<int, GetVectorComponetFunctionType> get_component_from_vector_literal_function;

	void SetGetComponentFromVectorFunction(const char* _return_type,
									GetVectorComponetFunctionType _func)
	{	
		int ret = FindTypeIdFromName(utils::TextPointer::Get((char*)_return_type));
		get_component_from_vector_literal_function = std::pair<int, GetVectorComponetFunctionType>{ret, _func};
	}

	struct PrecomputationResult
	{
		const bool success;
		const int new_type;
		utils::TextPointer value;
	private:
		const std::string value_container;
	public:
		PrecomputationResult(bool _success, int _new_type, std::string _value)
			: success(_success), new_type(_new_type), value_container(_value)
		{
			value = { (char*)value_container.c_str(), (int)value_container.size() };
		}
	};

	PrecomputationResult* Precompute(int Type1, int Type2, OperatorType_T OP, utils::TextPointer Literal1, utils::TextPointer Literal2);
	int GetOperationReturnType(int Type1, int Type2, OperatorType_T op);

	int GetBasicTypesNumber();

	std::vector<uint8_t> ConvertLiteralToBinary(int Type, utils::TextPointer src);

	/*
		List of conditions, that file must meet to get compilated correctly.
		It's content is constant and it's depends on version.
		List is meant to define such a conditions like 
		"file have to contains VertexShader", 
		"VertexShader have to return something"
		and someone. 
		When compilation starts, there will be created map of conditions
		in Temp. It will be based on this list.
		Key is name of condition
		Value is error, that will appear, when condition is not met
	*/
private: 
	std::map<std::string, std::string> CompilationConditions;
public:
	void AddCondition(std::string Name, std::string ErrorText);
	std::string GetConditionErrorText(std::string Name);
	std::vector<std::string> GetConditionsNames();

private:
	struct TypeConversion
	{
		using ConversionFunction = std::function<std::string(utils::TextPointer)>;
		int source_type;
		int target_type;
		ConversionFunction func;
		TypeConversion(int _source_type, int _target_type, ConversionFunction _func) :
			source_type(_source_type), target_type(_target_type), func(_func) {};
	};
	std::vector<TypeConversion> TypeConversions;
public:
	void AllowConversion(const char* _from_type,
		const char* _to_type, TypeConversion::ConversionFunction _func)
	{
		int type1 = FindTypeIdFromName(utils::TextPointer::Get((char*)_from_type));
		int type2 = FindTypeIdFromName(utils::TextPointer::Get((char*)_to_type));
		TypeConversion TC = TypeConversion{
			type1, type2, _func
		};
		TypeConversions.push_back(TC);
	}

	bool IsAllowedConversion(int source_type, int target_type);
	std::string Convert(int source_type, int target_type, utils::TextPointer source);

private:
	std::vector<int> ShadersReturnTypes;
public:
	void SetShadersReturnTypes(std::vector<int> _ShadersReturnTypes)
	{
		ShadersReturnTypes = _ShadersReturnTypes;
	}

	std::vector<std::pair<utils::TextPointer, FunctionHeader>> built_in_functions;
};

#undef Temp