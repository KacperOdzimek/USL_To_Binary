#include "USL_version.h"

#define Temp USL_Translator::USL_To_Binary::Temp

Compiling_Temp* Version::AccessTemp() noexcept
{
	return Temp;
}

bool Version::IsTypeValiding(const char* name)
{
    for (auto t : Types)
    {
        auto T = utils::TextPointer::Get((char*)name);
        if (utils::TextPointer::Get((char*)t->name) == T)
            return 1;
    }
    return 0;
}

unsigned int Version::TypeNameToTypeId(const char* name)
{
    int i = 0;
    auto T = utils::TextPointer::Get((char*)name);
    for (; i < Types.size(); i++)
    {			
        if (utils::TextPointer::Get((char*)Types[i]->name) == T)
            return i;
    }
    for (int j = 0; j < Temp->Structs.size(); j++)
    {
        if (Temp->Structs[j].first == T)
            return i + j;
    }
    return 0;
}

bool Version::verify_struct(Struct& s, const char* args_src)
{
    auto x = utils::ExtrudeArguments(args_src, utils::BracketsType::parentheses);
    auto args = x.first;
    if (args.size() != s.Members.size())
        return 0;

    for (int i = 0; i < s.Members.size(); i++)
    {
        if (!Types[s.Members[i].second]->verify(args[i]))
            return 0;
    }

    return 1;
}

void Version::structs_to_binary(Struct& s, const char* args_src)
{
    auto x = utils::ExtrudeArguments(args_src, utils::BracketsType::parentheses);
    for (int i = 0; i < s.Members.size(); i++)
    {
        for (uint8_t x : Types[s.Members[i].second]->to_binary(x.first[i]))
            Temp->FieldsBuffor.push_back(x);
    }
}

Version::MatchResult Version::Match(char* source, uint64_t size)
{
    std::vector<Version::IsMatchingResult> used_wrong;
    MatchResult result;

    for (auto s : Signatures)
    {
        auto IsMatchinRes = IsMatching(source, size, s);
        if (IsMatchinRes.all_conditions_met)
        {
            result.success = true;
            result.matching_signature = s;
            result.matching_but_wrong = used_wrong;
            return result;
        }          
        else if (IsMatchinRes.matching)
            used_wrong.push_back(IsMatchinRes);
    }
    
    result.success = false;
    result.matching_signature = nullptr;
    result.matching_but_wrong = used_wrong;

    return result;
}

int Version::GetBasicTypesNumber()
{
	return Types.size();
}

std::vector<uint8_t> Version::ConvertLiteralToBinary(int Type, utils::TextPointer src)
{
    return Types[Type]->to_binary(src);
}

Version::PrecomputationResult* Version::Precompute(int Type1, int Type2, OperatorType_T OP, utils::TextPointer Literal1, utils::TextPointer Literal2)
{
    for (auto& operation : precomputation_functions)
    {
        if (
            (
                (operation.Type1 == Type1 && operation.Type2 == Type2) ||
                (operation.Type1 == Type2 && operation.Type2 == Type1) 
             ) 
            &&
             OP == operation.operato)
        {
            auto r = operation.func(Literal1, Literal2);
            return new PrecomputationResult(true, r.first, { (char*)r.second.c_str(), r.second.size() });
        }
    }
    return new PrecomputationResult(false, 0, {});
}

int Version::GetOperationReturnType(int Type1, int Type2, OperatorType_T op)
{
    for (auto& operation : precomputation_functions)
    {
        if (
            (
                (operation.Type1 == Type1 && operation.Type2 == Type2) ||
                (operation.Type1 == Type2 && operation.Type2 == Type1)
                )
            &&
            op == operation.operato)

            return operation.ReturnType;
    }
    return -1;
}

bool Version::IsAllowedConversion(int source_type, int target_type)
{
    for (auto& conversion : TypeConversions)
        if (conversion.source_type == source_type && conversion.target_type == target_type)
            return true;
    return false;
}

std::string Version::Convert(int source_type, int target_type, utils::TextPointer source)
{
    for (auto& conversion : TypeConversions)
    {
        if (conversion.source_type == source_type && conversion.target_type == target_type)
            return conversion.func(source);
    }
    return "";
}

void Version::AddCondition(std::string Name, std::string ErrorText)
{
    CompilationConditions.insert({ Name, ErrorText });
}

std::string Version::GetConditionErrorText(std::string Name)
{
    return CompilationConditions.at(Name);
}

std::vector<std::string> Version::GetConditionsNames()
{
    std::vector<std::string> vector;
    for (auto& condition : CompilationConditions)
        vector.push_back(condition.first);
    return vector;
}

Version::IsMatchingResult Version::IsMatching(char* t, uint64_t s, Signature* sig)
{
    bool context_matching = true;
    std::vector<std::string> issues;

    if (std::find(sig->allowed_contexts.begin(), sig->allowed_contexts.end(), Temp->Context) == sig->allowed_contexts.end())
    {
        context_matching = false;
        issues.push_back("Inncorect context");
    }

    //Clear buffors before writing to it
    Temp->FieldsBuffor.clear();
    Temp->NamesBuffor.clear();

    //s_i - iterate on signature
    int s_i = 0;
    //c_i - iterate on code
    int c_i = 0;

    //removed due to math expressions
    //if number of spaces are diffrent it isn't matching
    //bool result = utils::CountSpaces(t) == utils::CountSpaces(sig->sig);

    bool result = true;
    //Position of first type mentioned in signature in FieldsBuffor
    int first_type_id = -1;

    while (!(s_i >= sig->size) && result == true)
    {	
        auto s_w = utils::TextPointer::Get((char*)(sig->sig + s_i));
        auto c_w = utils::TextPointer::Get(t + c_i);

        //Check arguments fields
        if (s_w.begin[0] == '?')
        {
#define EndCase s_i += 3; c_i += c_w.length + 1; break; }
            switch (s_w.begin[1])
            {
            //Type
            case 't': {
                result = IsTypeValiding(c_w.begin) || Temp->IsStructValiding(c_w);
                if (result) 
                    Temp->FieldsBuffor.push_back(TypeNameToTypeId(c_w.begin));
                if (first_type_id == -1) first_type_id = Temp->FieldsBuffor.size() - 1;
                EndCase
            //Name
            case 'n': {
                if (c_w.begin[c_w.length - 1] == ':')
                    Temp->NamesBuffor.push_back(utils::TextPointer(c_w.begin, c_w.length - 1));
                else
                    Temp->NamesBuffor.push_back(c_w);
                EndCase
            //Variable of type, passed in first type field
            case 'v': {
                if (first_type_id == -1)
                    throw "No type specified! (variable request)";
                utils::TextPointer key = utils::TextPointer::Get(c_w.begin); 
                auto V = Temp->GetVar(key);
                result = V != nullptr && V->second.first == Temp->FieldsBuffor[first_type_id];
                EndCase
            //Literal of type, passed in the first type field
            //Literals must be at end of expression
            case 'l': {
                if (first_type_id == -1)
                    throw "No type specified! (literal request)";
                //Basic, build-in type
                bool is_basic_type = Temp->FieldsBuffor[first_type_id] < Types.size();
                if (is_basic_type)
                    result = Types[Temp->FieldsBuffor[first_type_id]]->verify(c_w);
                //User declared struct
                else
                    result = verify_struct(Temp->Structs[Temp->FieldsBuffor[first_type_id] - Types.size()].second, c_w.begin);
                if (result && is_basic_type)
                    for (uint8_t x : Types[Temp->FieldsBuffor[first_type_id]]->to_binary(c_w))
                        Temp->FieldsBuffor.push_back(x);
                else if (result)
                {
                    structs_to_binary(Temp->Structs[Temp->FieldsBuffor[first_type_id] - Types.size()].second, c_w.begin);
                    int i = 0;
                    while (c_w.begin[i] != ')')
                        i++;
                    c_i += i - c_w.length;
                }
                if (result)
                {
                    int spaces_after = 0;
                    int j = c_w.length;
                    while (c_w.begin[j] != '\n' && c_w.begin[j] != '\0' && c_w.begin[j] != '\r')
                    {
                        if (c_w.begin[j] == ' ')
                            spaces_after++;
                        j++;
                    }

                    if (c_i + c_w.length + 1 == s - spaces_after)
                        goto l_end;

                    else if (*c_w.begin == '(')
                    {
                        int i = 0;
                        int deep = 1;
                        while (c_w.begin[i] != '\n' && c_w.begin[i] != '\0' 
                            && c_w.begin[i] != '\r' && deep != 0)
                        {
                            i++;
                            switch (c_w.begin[i])
                            {
                            case '(': deep++; break;
                            case ')': deep--; break;
                            }
                        }
                        result = (s - 2 == c_i + i || s - 3 == c_i + i);
                    }
                    else
                        result = 0;
                }

                l_end:
                if (result)
                    c_i = s - c_w.length - 1;
                EndCase
            //Function arguments in function declaration
            //e.g. int a, int b
            case 'f': {
                //at this point c_w's begin is seted to first letter
                //after '(' character
                
                auto x = utils::ExtrudeArguments(c_w.begin - 1, utils::BracketsType::parentheses);

                if (x.second != 0)
                {
                    auto args = x.first;

                    for (auto& a : args)
                    {
                        //a.Print();
                        auto type = utils::TextPointer::Get(a.begin);
                        result = IsTypeValiding(type.begin);
                        if (result)
                        {
                            Temp->FieldsBuffor.push_back(TypeNameToTypeId(type.begin));
                            auto name = utils::TextPointer::Get(type.begin + type.length + 1);
                            Temp->NamesBuffor.push_back(name);
                        }
                        else break;
                    }

                    c_i += x.second - c_w.length;
                }
                else
                {
                    result = 0;
                }

                EndCase

            //Expression returning type, passed in first type field
            case 'e': {
                /*
                    first, we need to find expression bounds
                    code below do it by following this pattern:
                    1. expression starts from an operand
                    2. iterate throught operand
                            (switch operand var to true)
                    3. (optional) iterate throught spaces separating operand from operators
                            (switch operand var to false)
                    4.1 if we found an operator (+, -, *, /, ^) set operator_detected to true
                    4.2 if we found operand that was not preceded by an operator leave the loop
                    4.3 if we found operand that was preceded by an operator consume operator_detected
                            and start iterating throught the operand

                    extra rules:
                    1. ',' normally causes program to break the loop, but if vector_literal is true
                            end operand and set operator_detected to true
                    2. '(' openes vector literal
                    3. ')' closes vector literal
                */

                char* iterator = c_w.begin - 1;
                bool bracket = false;
                bool operator_detected = true;
                bool operand;

                while (true)
                {
                    iterator++;
                    switch (*iterator)
                    {
                    case '+': case '-': case '*': case '/': case '^':
                        operator_detected = true;
                        operand = false;
                        break;

                    case '(':
                        bracket = true;
                        break;

                    case ')':
                        bracket = false;
                        break;

                    case '.':
                    {
                        char* i = iterator + 1;
                        while (*i == ' ')
                            i++;
                        if (!(*i >= '0' && *i <= '9'))
                        {
                            operator_detected = true;
                            operand = false;
                        }
                        break;
                    }

                    case ',':
                        if (!bracket)
                            goto leave_expression;
                        else
                        {
                            operator_detected = true;
                            operand = false;
                        }
                        break;

                    case ' ': 
                        operand = false;
                        break;
                    default:
                        if (!operator_detected && !operand)
                            goto leave_expression;
                        else
                            operator_detected = false;
                        operand = true;
                        break;
                    case '\n': case '\0':
                        goto leave_expression;
                    }
                }

                leave_expression:

                int length = iterator - c_w.begin;

                result = math_parser::ParseMath({ c_w.begin, length }, Temp->FieldsBuffor[first_type_id], this, issues);
                c_i = iterator - t - c_w.length - 1;
                EndCase


            //Expressions
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9': {
                int type_id = std::atoi(s_w.begin + 1);

                //first word after expression symbol
                auto x = utils::TextPointer::Get((char*)(sig->sig + s_i + s_w.length + 1));
                int expr_size = 0;
                if (x.length != 0)
                {
                    // 0 + 1 end
                    // ?0 end
                    // this loop watch for keyword ending expression

                    int i = 0;
                    do
                    {
                        if (*(c_w.begin + expr_size + i) != *(x.begin + i))
                        {
                            expr_size = expr_size + i + 1;
                            i = 0;
                        }
                        else
                        {
                            i++;
                            if (x.length == i)
                                //Break loop
                                goto leave;
                        }
                    } while (true);
                }
                else
                    expr_size = s - c_i - 1;

                //At this point we have size of our expression
                leave:

                EndCase

#undef EndCase	
            }
        }
        //Check keywords like "for" etc.
        else if (s_w.begin[0] == ':' && *(c_w.begin - 1) == ':')
        {
            result = true;
            break;
        }
        else
        {
            if (s_w == c_w)
            {
                s_i += s_w.length + 1;
                c_i += c_w.length + 1;
            }
            else
            {
                result = false;
                break;
            }
        }
    }

    //Check if we have iterated to end of code
    bool iterated_to_end = result && (c_i == s || c_i == s - 1 || c_i == s + 1);
    return { iterated_to_end, iterated_to_end && context_matching && !issues.size(), issues };
}