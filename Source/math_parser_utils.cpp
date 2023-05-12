#include "math_parser_utils.h"
#include <cmath>
#include <unordered_map>

#define Temp Version::AccessTemp()

namespace math_parser
{
    std::pair<std::vector<char>, std::vector<utils::TextPointer>> GetOperatorsAndOperands(utils::TextPointer& exp);
    RawExpressionTree::RawExpressionTree(utils::TextPointer exp, Version* V, std::vector<std::string>& issues)
    {
        auto r = GetOperatorsAndOperands(exp);
        chars_buff = r.first;
        auto& operands = r.second;

        /*
            As vectors grow they changes they arrangement of elements
            Operator nodes are pointing to elements of this vector so those cannot change
            In order to achieve that we reserve operands.size() spaces, because in worst case
            every operand can be changed to vec2, vec3 or vec4 operator
        */
        chars_buff.reserve(operands.size());

        auto& operators = chars_buff;

        /*
            Catch invalid expressions
            Error if operators number >= operands number
            or operarators number == 0 && operands.size() == 0
        */
        if (operators.size() >= operands.size() || (!operators.size() && !operands.size()))
        {
            issues.push_back("Invalid expression");
            throw ((int)0);
        }
        //Operand length equals -1 or 0; occurs in invalid expressions like (* 2)
        if (operands[0].length == -1 || operands[0].length == 0)
        {
            issues.push_back("Invalid expression");
            throw ((int)0);
        }

        //We have only one operand so we don't need to do any magic with operators
        //Just make this operand root and we are done
        if (operators.size() == 0 || operands.size() == 0)
        {
            auto node = CreateNode();
            node->Content = operands[0];
            operands.erase(operands.begin());
        }
        //We have operators and operands, so
        else
        {
            /*
                Create tree using rules of operator precedence
                First we start with first operand and first operator
                Like:

                        /       <----\
                    operator         This places will be filled in next steps
                    /      \    <----/
                operand

                Then we do for all of the next operators:
                If previous operator precedence < current operator operators precedence
                    Current operator is placed as previous operator child node
                    Current operand is placed as current operator child node

                        /                               /
                        +      +   [3 *]  ---->         +
                      /   \                           /   \
                     2                               2     [*]
                                                          /   \
                                                        [3]
                Else
                    If previous node is on top:
                        Current operator is new tree's uppest node
                        Current operand is given to previous operator

                                                            /
                                                          [+]
                         /                               /   \
        on top  --->    +      +   [3 +]  ---->         +
                      /   \                           /   \
                     2                               2    [3]

                    Else
                        Current operator'll "climb" to the top of the tree
                        If it mets operator with < precedence it will insert itself
                        in between operator with < precedence and child node on which
                        Current operator already have climbed
                        If it doesn't it will keep going up and may become the uppest node

                        Current operand is given to the current operator


                            /                                       /
                           +                                        +
                         /   \              + (3 *)    ---->      /   \
                        2     : <-- last                         2    [*]    <-- inserted;
                            /   \                                    /   \       stoped on +
                           3    3                                  [3]    :      with lower predence
                                                                        /   \

                Last operand will be given to very lastly placed operator.
            */
            auto first_operand = CreateNode();
            first_operand->Content = operands[0];
            operands.erase(operands.begin());

            auto first_operator = CreateNode();
            first_operator->Content = utils::TextPointer(&operators[0], 1);
            first_operator->Own(first_operand);

            RawExpressionTree::Node* Previous = Uppest();

            for (int o = 1; o < operators.size(); o++)
            {
                //Previous operator's precedence
                int p_precedence;
                switch (operators[o - 1])
                {
                case '+': p_precedence = 0; break;
                case '-': p_precedence = 0; break;
                case '*': p_precedence = 1; break;
                case '/': p_precedence = 1; break;
                case '^': p_precedence = 2; break;
                case '.': p_precedence = 3; break;
                }

                //Current operator's precedence
                int c_precedence;
                switch (operators[o])
                {
                case '+': c_precedence = 0; break;
                case '-': c_precedence = 0; break;
                case '*': c_precedence = 1; break;
                case '/': c_precedence = 1; break;
                case '^': c_precedence = 2; break;
                case '.': c_precedence = 3; break;
                }

                //Previous operator's precedence >= current operator's precedence
                if (p_precedence >= c_precedence)
                {
                    //Give current operand to previous node
                    RawExpressionTree::Node* operand = CreateNode();
                    operand->Content = operands[0];
                    operands.erase(operands.begin());
                    Previous->Own(operand);

                    //Pointer to nodes above previous node
                    //Treat as current node location on the journey to the top
                    auto above = Previous->Upper;

                    //Already reached top of the tree - current node is new root.
                    if (above == nullptr)
                    {
                        auto uppest = Uppest();
                        AddUppest(&operators[o]);
                        Previous = Uppest();
                        continue;
                    }

                    while (true)
                    {
                        //Precedence of the operator under above pointer
                        int a_precedence = 0;
                        switch (above->Content.begin[0])
                        {
                        case '+': a_precedence = 0; break;
                        case '-': a_precedence = 0; break;
                        case '*': a_precedence = 1; break;
                        case '/': a_precedence = 1; break;
                        case '^': a_precedence = 2; break;
                        case '.': a_precedence = 3; break;
                        }

                        //Current precedence >= above precedence; keep going or
                        if (a_precedence >= p_precedence)
                        {
                            above = above->Upper;
                            //or if it's top make current node the uppest node
                            if (above == nullptr)
                            {
                                //Add new uppest node
                                auto u = Uppest();
                                AddUppest(&operators[o]);
                                Previous = Uppest();
                            }
                            break;
                        }
                        //Current precedence < above precedence; insert in here
                        else
                        {
                            //Insert new node
                            auto _new = CreateNode();
                            _new->Content = { &operators[o], 1 };
                            above->Own(_new);
                            _new->Own(_new->Upper->Owned[1]);
                            above->Owned.erase(above->Owned.begin() + 1);
                            Previous = _new;
                            break;
                        }
                    }
                }
                //previous operator's precedence < current operator operator's precedence
                else
                {
                    //Current operator is placed as previous operator child node
                    RawExpressionTree::Node* operato = CreateNode();
                    operato->Content = utils::TextPointer(&operators[o], 1);
                    Previous->Own(operato);

                    //Current operand is placed as current operator child node
                    RawExpressionTree::Node* operand = CreateNode();
                    operand->Content = operands[0];
                    operands.erase(operands.begin());
                    operato->Own(operand);

                    Previous = operato;
                }

                //Mark current operator predence as previous as we keep going
                p_precedence = c_precedence;
            }

            //Add remaining operand to last node
            if (operands.size() == 1)
            {
                RawExpressionTree::Node* operand = CreateNode();
                operand->Content = operands[0];
                Previous->Own(operand);
            }
        }

        auto contains_bracket = [](utils::TextPointer tp) -> bool
        {
            for (int i = 0; i < tp.length; i++)
                if (tp.begin[i] == '(')
                    return 1;
            return 0;
        };

        auto is_array_get = [](utils::TextPointer tp) -> bool
        {
            int deep = 0;
            int square_bracket_open_count = 0;
            for (int i = 0; i < tp.length; i++)
            {
                switch (tp.begin[i])
                {
                case '(': ++deep; break;
                case ')': --deep; break;
                case '[': if (deep == 0) ++square_bracket_open_count;
                }
            }
            return square_bracket_open_count;
        };

        auto GenSubexpForArg = [this, V, &issues](utils::TextPointer start) -> int
        {
            int i = 1;
            int last_comma = 1;
            int deep = 1;
            int commas = 0;

            bool content = false;

            while (deep != 0)
            {
                switch (start.begin[i])
                {
                case '(': deep++; break;
                case ')': deep--; break;
                case ',': if (deep == 1) commas++; break;
                case ' ': case '\n': break;
                default: content = true;
                }

                if (start.begin[i] == ',' && deep == 1 || deep == 0 && content)
                {
                    auto subexp = new RawExpressionTree(
                        { start.begin + last_comma + ((last_comma == 1) ? 0 : 1)
                        ,i - last_comma - (last_comma == 1 ? 0 : 1) }, V, issues);
                    subtrees.push_back(subexp);
                    last_comma = i;
                }
                i++;
            }
            if (content == false) return 0;
            return commas + 1;
        };

        //Find nested expressions (those in brackets), vectors constructors and functions calls 
        //and create tree for them, then merge those trees with main tree
        for (auto& node : All_Nodes)
        {
            if (contains_bracket(node->Content))
            {
                auto CountCommasAndCheckIfContainsAnything = [node]()
                {
                    int i = 0;
                    int commas = 0;
                    int deep = 0;
                    bool contains_something = false;

                    for (int i = 0; i < node->Content.length; i++)
                    {
                        switch (node->Content.begin[i])
                        {
                        case ',': if (deep == 1) commas++; break;
                        case '(': deep++;                  break;
                        case ')': deep--;                  break;
                        case ' ': break; case '\t': break;
                        default: contains_something = true;
                        }
                    }

                    return std::pair<int, bool>(commas, contains_something);
                };

                auto commasXcontains = CountCommasAndCheckIfContainsAnything();

                if (commasXcontains.second == false)
                {
                    issues.push_back("Empty brackets");
                    continue;
                }

                //Function call
                if (node->Content.begin[0] != '(')
                {
                    int i = 1;
                    while (node->Content.begin[i] != '(') ++i;
                    int args = GenSubexpForArg({ node->Content.begin + i, node->Content.length - i });
                    node->Content.length = i;

                    for (int j = subtrees.size() - args; j != subtrees.size(); j++)
                        node->Own(subtrees[j]->Uppest());
                }
                //Check if it is vector literal
                else if (V->FindTypeIdFromLiteral(node->Content) != -1)
                    continue;
                //If we have commas, then it is something like (2 * 2, 2 + 2) which is also vector
                else if (commasXcontains.first > 0)
                {
                    GenSubexpForArg(node->Content);

                    switch (commasXcontains.first)
                    {
                    case 1: chars_buff.push_back('\2'); node->Content.begin = &chars_buff.back();
                        node->Content.length = 1; break;
                    case 2: chars_buff.push_back('\3'); node->Content.begin = &chars_buff.back();
                        node->Content.length = 1; break;
                    case 3: chars_buff.push_back('\4'); node->Content.begin = &chars_buff.back();
                        node->Content.length = 1; break;
                    default:
                        issues.push_back("Too many arguments for vector constructor");
                        throw ((int)1);
                        break;
                    }

                    for (int i = commasXcontains.first + 1; i != 0; i--)
                    {
                        auto upp = subtrees[subtrees.size() - i]->Uppest();
                        node->Own(upp);
                    }
                }
                //Simple (2 + 2) etc.
                else
                {
                    auto subexp = new RawExpressionTree({ node->Content.begin + 1, node->Content.length }, V, issues);

                    subtrees.push_back(subexp);

                    auto sub_root = subexp->Uppest();
                    node->Content = sub_root->Content;
                    node->Owned = sub_root->Owned;
                    for (auto& own : node->Owned)
                        own->Upper = node;
                }
            }
            //a[1], b[2 / 2], c[i] etc.
            else if (is_array_get(node->Content))
            {
                int deep = 0;
                int i = 0;
                std::vector<int> open_pos;
                std::vector<int> close_pos;
                while (i < node->Content.length)
                {
                    switch (node->Content.begin[i])
                    {
                    case '(': ++deep; break;
                    case ')': --deep; break;
                    case '[': if (deep == 0) { open_pos.push_back(i); } break;
                    case ']': if (deep == 0) { close_pos.push_back(i); }
                    }
                    i++;
                }

                auto lhs = new RawExpressionTree({ node->Content.begin, open_pos.at(0) }, V, issues);
                subtrees.push_back(lhs);

                if (open_pos.size() != 2)
                {
                    auto rhs = new RawExpressionTree({ node->Content.begin + open_pos.at(0) + 1, i - close_pos.at(0) }, V, issues);
                    subtrees.push_back(rhs);

                    node->Own(lhs->Uppest());
                    node->Own(rhs->Uppest());
                
                    chars_buff.push_back('[');
                    node->Content.begin = &chars_buff.back();
                    node->Content.length = 1;
                }
                else
                {
                    auto first_id = new RawExpressionTree({ node->Content.begin + open_pos.at(0) + 1, 
                        close_pos.at(0) - open_pos.at(0) - 1 }, V, issues);
                    auto second_id = new RawExpressionTree({ node->Content.begin + open_pos.at(1) + 1, 
                        close_pos.at(1) - open_pos.at(1) - 1 }, V, issues);

                    node->Own(lhs->Uppest());
                    node->Own(first_id->Uppest());
                    node->Own(second_id->Uppest());

                    chars_buff.push_back(']');
                    node->Content.begin = &chars_buff.back();
                    node->Content.length = 1;
                }
            }
        }
    }


    ExpressionTree::ExpressionTree(RawExpressionTree& RET, Version* version, std::vector<std::string>& issues)
    {
        {
			auto Proccess = [this, &version, &issues](RawExpressionTree::Node* n, auto& SelfRef, Node* Upper) -> std::pair<bool, Node*>
			{
				auto processed = new Node;
                All_Nodes.push_back(processed);
                
                if (Upper != nullptr && Upper->Type == NodeType::Operator && 
                    Upper->content.OperatorType == OperatorType_T::get && Upper->OwnedNodes.size() != 0)
                {
                    int get_target_type_id = Upper->OwnedNodes[0]->GetNodeDataTypeId(version, issues);
                    //Vector
                    if (get_target_type_id < version->GetBasicTypesNumber() && get_target_type_id != -1)
                    {
                        processed->Type = NodeType::Byte;

                        if (n->Content.length != 1)
                        {
                            std::string eror_text = "Vector object does not contains component ";
                            for (int i = 0; i < n->Content.length; i++)
                                eror_text.push_back(n->Content.begin[i]);
                            issues.push_back(eror_text);
                        }
                        else
                        {
                            //find requested compound id
                            int vec_size = 0;
                            if (get_target_type_id == version->FindTypeIdFromName({ (char*)"vec2", 4 })) vec_size = 2;
                            else if (get_target_type_id == version->FindTypeIdFromName({ (char*)"vec3", 4 })) vec_size = 3;
                            else if (get_target_type_id == version->FindTypeIdFromName({ (char*)"vec4", 4 })) vec_size = 4;
                            int id = -1;
                            for (int i = 0; i < vec_size; i++)
                                if (n->Content.begin[0] == version->vector_components_names[i])
                                    id = i;
                            if (id == -1)
                                issues.push_back("Vector object of size " + std::to_string(vec_size) +
                                    " does not contains component " + n->Content.begin[0]);
                            processed->content.Byte = (uint8_t)id;
                        }
                    }
                    //Struct
                    else if (get_target_type_id != -1)
                    {
                        processed->Type = NodeType::Byte;
                        auto& struc = Temp->Structs.at(
                            Upper->OwnedNodes[0]->GetNodeDataTypeId(version, issues) - version->GetBasicTypesNumber());
                        for (int i = 0; i < struc.second.Members.size(); i++)
                            if (struc.second.Members[i].first == n->Content)
                            {
                                processed->content.Byte = (uint8_t)i;
                                break;
                            }
                    }
                }
                //Operator
				else if (n->Content.length == 1)
				{
					switch (n->Content.begin[0])
					{
						case '+': processed->content.OperatorType = OperatorType_T::add;        break;
						case '-': processed->content.OperatorType = OperatorType_T::sub;        break;
						case '*': processed->content.OperatorType = OperatorType_T::mul;        break;
						case '/': processed->content.OperatorType = OperatorType_T::div;        break;
						case '^': processed->content.OperatorType = OperatorType_T::pow;        break;

                        case '.': processed->content.OperatorType = OperatorType_T::get;        break;
                        case '[': processed->content.OperatorType = OperatorType_T::array_get;  break;
                        case ']': processed->content.OperatorType = OperatorType_T::matrix_get; break;

                        case '\2': processed->content.OperatorType = OperatorType_T::vec2;      break;
                        case '\3': processed->content.OperatorType = OperatorType_T::vec3;      break;
                        case '\4': processed->content.OperatorType = OperatorType_T::vec4;      break;

						default: goto other_cases;
					}
					processed->Type = NodeType::Operator;
				}
				else
				{
					other_cases:
					//Check if it is an instance of an type
					int TypeId = version->FindTypeIdFromLiteral(n->Content);
					if (TypeId != -1)
					{
						processed->Type = ExpressionTree::NodeType::Literal;
						processed->content.Literal = { TypeId, n->Content };
					}
					//Check if it is an variable
					else if (Temp->IsVarValiding(n->Content) && n->Owned.size() == 0)
					{
						processed->Type = ExpressionTree::NodeType::Variable;
						processed->content.Variable = Temp->GetVarId(n->Content);
					}
                    else
                    {
                        //Check if it is a function call
                        if (n->Owned.size() != 0 || *(n->Content.begin + n->Content.length) == '(')
                        {
                            std::vector<int> a_types;
                            //We need to know function args types, so lets convert child nodes first
                            bool success = true;
                            for (auto o : n->Owned)
                            {
                                auto r = SelfRef(o, SelfRef, processed);
                                if (!r.first)
                                    success = false;
                                processed->OwnedNodes.push_back(r.second);
                                r.second->upper = processed;
                            }

                            for (auto& arg : processed->OwnedNodes)
                                a_types.push_back(arg->GetNodeDataTypeId(version, issues));

                            int func_id = Temp->GetFunctionId(n->Content, a_types, version);
                            
                            if (func_id >= 0)
                            {
                                processed->Type = NodeType::Function;
                                processed->content.FunctionId = func_id;
                                return { true, processed }; //Return as child nodes were converted
                            }
                            switch (func_id)
                            {
                            case -1:
                                goto Unrecognised_symbol; break;
                            case -2:
                            {
                                std::string str = "type/s of arguments doesn't match any overload of the function: ";
                                str.insert(str.end(), n->Content.begin, n->Content.begin + n->Content.length);
                                issues.push_back(str);
                                break;
                            }
                            case -3:
                            {
                                std::string str = "argument set too big for a function: ";
                                str.insert(str.end(), n->Content.begin, n->Content.begin + n->Content.length);
                                issues.push_back(str);
                                break;
                            }
                            case -4:
                            {
                                std::string str = "argument set too little for a function: ";
                                str.insert(str.end(), n->Content.begin, n->Content.begin + n->Content.length);
                                issues.push_back(str);
                                break;
                            }
                            }      
                        }
                        else
                        {
                            Unrecognised_symbol:
                            std::string symbol(n->Content.begin);
                            issues.push_back("Unrecognised symbol: " + symbol.substr(0, n->Content.length));
                        }
                    }
				}

				for (auto o : n->Owned)
				{
                    auto r = SelfRef(o, SelfRef, processed);
                    if (!r.first)
                    {
                        return {false, processed};
                    }
                    processed->OwnedNodes.push_back(r.second);
					r.second->upper = processed;
                }

				return {true, processed};
			};

			Proccess(RET.Uppest(), Proccess, nullptr);
		}
    }

	bool BinaryNode::SetRest(uint8_t _rest)
	{
		switch (type)
		{
		case Type::Variable:
			if (_rest > 127)
				return false;
			else
			{
				rest = _rest;
				return true;
			}
		default:
			if (_rest > 31)
				return false;
			else
			{
				rest = _rest;
				return true;
			}
		}
	}

    int ExpressionTree::Node::GetNodeDataTypeId(Version* version, std::vector<std::string>& issues)
    {
        switch (Type)
        {
        case math_parser::ExpressionTree::NodeType::Variable:
            return Temp->Variables[content.Variable].second.first;
            break;
        case math_parser::ExpressionTree::NodeType::Literal:
            return content.Literal.first;
            break;
        case math_parser::ExpressionTree::NodeType::Operator:
        {
            int type_lhs = OwnedNodes[0]->GetNodeDataTypeId(version, issues);
            int type_rhs = OwnedNodes[1]->GetNodeDataTypeId(version, issues);

            if (content.OperatorType == OperatorType_T::get)
            {
                //Vector
                if (type_lhs == version->FindTypeIdFromName({ (char*)"vec2", 4 }) ||
                    type_lhs == version->FindTypeIdFromName({ (char*)"vec3", 4 }) ||
                    type_lhs == version->FindTypeIdFromName({ (char*)"vec4", 4 }))
                {
                    return version->FindTypeIdFromName({ (char*)"float", 5 });
                }
                //Struct
                else if (type_lhs >= version->GetBasicTypesNumber())
                {
                    auto& struc = Temp->Structs.at(type_lhs - version->GetBasicTypesNumber());
                    return struc.second.Members[OwnedNodes[1]->content.Byte].second;
                }
            }
            //Get from array
            else if (content.OperatorType == OperatorType_T::array_get)
            {
                if (type_lhs >= version->FindTypeIdFromName({ (char*)"mat2x2", 6 }) &&
                    type_lhs <= version->FindTypeIdFromName({ (char*)"mat4x4", 6 }))
                {
                    std::string vec_name = "vec";
                    auto name = version->FindTypeNameFromId(type_lhs);
                    vec_name += *(name.begin + 3);
                    return version->FindTypeIdFromName({(char*)vec_name.c_str(), 4});
                }
            gt_then_it_is_array:
                if (OwnedNodes[0]->Type == NodeType::Variable && Temp->arrays.find(OwnedNodes[0]->content.Variable) != Temp->arrays.end())
                    return Temp->arrays.at(OwnedNodes[0]->content.Variable).type;
                return -1;
            }
            else if (content.OperatorType == OperatorType_T::matrix_get)
            {
                //Not an matrix
                if (!(type_lhs >= version->FindTypeIdFromName({ (char*)"mat2x2", 6 }) &&
                    type_lhs <= version->FindTypeIdFromName({ (char*)"mat4x4", 6 })))
                {
                    goto gt_then_it_is_array;
                }
                return version->FindTypeIdFromName({ (char*)"float", 5 });
            }
            else if (content.OperatorType == OperatorType_T::vec2)
                return version->FindTypeIdFromName({ (char*)"vec2", 4 });
            else if (content.OperatorType == OperatorType_T::vec3)
            {
                int int_id = version->FindTypeIdFromName({ (char*)"int", 3 });
                int float_id = version->FindTypeIdFromName({ (char*)"float", 5 });

                int third_arg = OwnedNodes[2]->GetNodeDataTypeId(version, issues);

                for (auto& arg : std::vector<int>{ type_lhs, type_rhs, third_arg })
                    if (arg != int_id && arg != float_id)
                        return -1;

                return version->FindTypeIdFromName({ (char*)"vec3", 4 });
            }
            else if (content.OperatorType == OperatorType_T::vec4)
            {
                int int_id     = version->FindTypeIdFromName({ (char*)"int", 3 });
                int float_id   = version->FindTypeIdFromName({ (char*)"float", 5 });

                int third_arg  = OwnedNodes[2]->GetNodeDataTypeId(version, issues);
                int fourth_arg = OwnedNodes[3]->GetNodeDataTypeId(version, issues);

                for (auto& arg : std::vector<int>{ type_lhs, type_rhs, third_arg, fourth_arg })
                    if (arg != int_id && arg != float_id)
                        return -1;

                return version->FindTypeIdFromName({ (char*)"vec4", 4 });
            }   
            else
            {
                if (type_lhs == -1 || type_rhs == -1)
                    return -1;
                else
                {
                    int return_type = version->GetOperationReturnType(type_lhs, type_rhs, content.OperatorType);
                    if (return_type == -1)
                    {
                        std::string error = "Forbidden operation: ";
                        auto name_1 = version->FindTypeNameFromId(type_lhs);
                        auto name_2 = version->FindTypeNameFromId(type_rhs);
                        error.insert(error.end(), name_1.begin, name_1.begin + name_1.length);
                        error.push_back(' ');
                        switch (content.OperatorType)
                        {
                        case OperatorType_T::add:
                            error.push_back('+'); break;
                        case OperatorType_T::sub:
                            error.push_back('-'); break;
                        case OperatorType_T::mul:
                            error.push_back('*'); break;
                        case OperatorType_T::div:
                            error.push_back('/'); break;
                        case OperatorType_T::pow:
                            error.push_back('^'); break;
                        }
                        error.push_back(' ');
                        error.insert(error.end(), name_2.begin, name_2.begin + name_2.length);
                        issues.push_back(error);
                    }

                    return return_type;
                }
            }
            break;
        }
        case math_parser::ExpressionTree::NodeType::Function:
            return Temp->FunctionsHeaders.at(content.FunctionId).second.ReturnType;
            break;
        }

        return -1;
    }

    std::pair<std::vector<char>, std::vector<utils::TextPointer>> GetOperatorsAndOperands(utils::TextPointer& exp)
    {
        int i = 0;
        int bound = exp.length + 1;
        if (bound == 0)
            bound = 1;
        int j = 0;
        int deepness = 0;
        while (j < exp.length)
        {
            switch (exp.begin[j])
            {
            case '(': deepness++; break;
            case ')': deepness--; break;
            }
            if (deepness < 0)
            {
                bound = j - 1;
                break;
            }
            j++;
        }

        //Part 1 - get operators and operands
        auto GetArg = [&exp, &i, bound]()
        {
            //Pass spaces before arg
            while (exp.begin[i] == ' ' && i < bound)
                i++;
            //Save arg begin
            int start = i;

            bool negative = exp.begin[i] == '-';
            bool non_special_chars_occurred = false;
            bool vector = 0;
            int deep = 0;

            //Iterate through arg
            do
            {
                switch (exp.begin[i])
                {
                case ' ': case '+': case '-': case '*': case '/': case '^': case ':':
                {
                    if (!negative || (non_special_chars_occurred) && deep == 0)
                        goto end; break;
                }
                case '.':
                {
                    int j = i + 1;
                    bool function = false;
                    while (j < bound)
                    {
                        switch (exp.begin[j])
                        {
                        case ' ': case '+': case '-': case '*': case '/': case '^': case ':': goto finish_is_function;
                        case '(': function = true; goto finish_is_function; break;
                        }
                        j++;
                    }
                finish_is_function:
                    if (function)
                    {
                        i++;
                    }
                    else
                    {
                        utils::TextPointer total = { exp.begin + start, i - start };
                        if (!utils::IsInteger(total))
                            if (!negative || (non_special_chars_occurred))
                                goto end; break;
                    }
                    break;
                }
                case '\r': case '\n': case '\0':
                    goto end; break;
                case '(':
                    deep = 1;
                    while (deep != 0 && i <= bound)
                    {
                        i++;
                        switch (exp.begin[i])
                        {
                        case '(': deep += 1; break;
                        case ')': deep -= 1; break;
                        }
                    }
                    break;
                case '[':
                {
                    if (i != 0)
                        while (exp.begin[i] != ']' && i != bound - 1)
                            i++;
                    break;
                }
                default:
                {
                    non_special_chars_occurred = true;
                    break;
                }
                } 
                i++;
            } while (i < bound);
        end:
            int end = (uint64_t)i - start - (bound == 1 ? 1 : 0);
            return utils::TextPointer(exp.begin + start, end == bound - start ? end - 1 : end);
        };

        auto GetOperator = [&exp, &i, bound]()
        {
            i--;
            //Wait for operator
            while (i < bound)
            {
                i++;
                switch (exp.begin[i])
                {
                case '+': case '-': case '*': case '/': case '^': case '.':
                    goto end; break;
                default:
                    break;
                }
            }

        end:
            i++;
            return exp.begin[i - 1];
        };

        std::vector<char> operators;
        std::vector<utils::TextPointer> operands;

        int state = 0;
        while (i < bound)
        {
            if (exp.begin[i] == '\r' || exp.begin[i] == '\n' || exp.begin[i] == '\0')
            {
                i++; continue;
            }
            switch (state)
            {
            case 0: operands.push_back(GetArg());		break;
            case 1:
                while (exp.begin[i] == ' ')
                    i++;
                break;
            case 2: operators.push_back(GetOperator()); break;
            }
            state++;
            if (state == 3)
                state = 0;
        }

        return { operators, operands };
    }
}