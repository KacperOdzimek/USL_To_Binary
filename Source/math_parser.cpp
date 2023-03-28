#include "math_parser.h"

#define Temp Version::AccessTemp()

bool math_parser::ParseMath(utils::TextPointer exp, int requested_type, Version* version, std::vector<std::string>& issues)
{
    //Part 1 - Check if amount of '(' is equal to amount of ')'
    int deep = 0;
    for (int i = 0; i < exp.length; i++)
        if (exp.begin[i] == '(')
            ++deep;
        else if (exp.begin[i] == ')')
            --deep;

    if (deep > 0)
        issues.push_back("Missing character ')' [" + std::to_string(deep) + " chars missing]");
    else if (deep < 0)
        issues.push_back("Missing character '(' [" + std::to_string(std::abs(deep)) + " chars missing]");

    //Part 2 - generate raw tree
    RawExpressionTree* RawTree;
    try
    {
        RawTree = new RawExpressionTree(exp, version, issues);
    }
    catch (int)
    {
        return true;
    }

    //Part 3 - create tree with real numbers instead of text
    ExpressionTree* Tree = new ExpressionTree(*RawTree, version, issues);
    delete RawTree;

    //Part 4 - simplify whole expression
    //4.1 - find operations on literal operands and turn it into single literal
    bool found_something = true;

    //This vector holds pointers to simplified results. As they are dynamicaly allocated, we have to delete all of them
    //along with ExpressionTree after it is converted to binary form.
    std::vector<Version::PrecomputationResult*> simplified_nodes_values;
    //Dont run if some issues occured as nodes values may be incorrect
    while (found_something && !issues.size())
    {
        found_something = false;
        for (auto n : Tree->All_Nodes)
        {
            if (n->Type == ExpressionTree::NodeType::Operator &&
                n->OwnedNodes[0]->Type == ExpressionTree::NodeType::Literal &&
                n->OwnedNodes[1]->Type == ExpressionTree::NodeType::Literal)
            {
                //Result is dynamicaly allocated object, so it needs to be deleted in due time
                auto result = version->Precompute(
                    n->OwnedNodes[0]->content.Literal.first,
                    n->OwnedNodes[1]->content.Literal.first,
                    n->content.OperatorType,
                    n->OwnedNodes[0]->content.Literal.second,
                    n->OwnedNodes[1]->content.Literal.second);

                if (result->success == 1)
                {
                    n->OwnedNodes.clear();
                    n->Type = ExpressionTree::NodeType::Literal;
                    n->content.Literal = decltype(n->content.Literal){result->new_type, result->value};
                    simplified_nodes_values.push_back(result);
                    found_something = true;
                }
                else
                {
                    //Error; No function to handle this
                    delete result;
                }
            }
            else if (n->Type == ExpressionTree::NodeType::Operator &&
                     n->content.OperatorType == OperatorType_T::get &&
                     n->OwnedNodes[0]->Type == ExpressionTree::NodeType::Literal)
            {
                auto result = version->get_component_from_vector_literal_function.second(
                    n->OwnedNodes[0]->content.Literal.second,
                    n->OwnedNodes[1]->content.Byte);

                Version::PrecomputationResult* as_precomp_result = new Version::PrecomputationResult {   
                    true,
                    version->get_component_from_vector_literal_function.first,
                    result 
                };

                n->OwnedNodes.clear();
                n->Type = ExpressionTree::NodeType::Literal;
                n->content.Literal = decltype(n->content.Literal){as_precomp_result->new_type, as_precomp_result->value};

                simplified_nodes_values.push_back(as_precomp_result);
                found_something = true;
            }
        }
    }    

    if (Tree->All_Nodes[0]->GetNodeDataTypeId(version) != requested_type &&
        !version->IsAllowedConversion(Tree->All_Nodes[0]->GetNodeDataTypeId(version), requested_type))
    {
        issues.push_back("Type returned by expression does not match requested type");
    }   
    else if (version->IsAllowedConversion(Tree->All_Nodes[0]->GetNodeDataTypeId(version), requested_type) &&
        Tree->All_Nodes[0]->Type == ExpressionTree::NodeType::Literal)
    {
        auto result = version->Convert(Tree->All_Nodes[0]->GetNodeDataTypeId(version), requested_type,
            Tree->All_Nodes[0]->content.Literal.second);

        Version::PrecomputationResult* as_precomp_result = new Version::PrecomputationResult{
                    true,
                    requested_type,
                    result
        };

        Tree->All_Nodes[0]->OwnedNodes.clear();
        Tree->All_Nodes[0]->content.Literal = 
            decltype(Tree->All_Nodes[0]->content.Literal){as_precomp_result->new_type, as_precomp_result->value};

        simplified_nodes_values.push_back(as_precomp_result);
    }


    //Part 5 - write to field buffor
    /*  
        This step turns 2d Tree into linear, compressed data series.
        First there is one-byte information about length (L) of expression in bytes.

        Then we have L nodes.

        Single node is 8 bit. It structure is described below:

            Condition       In Bits     Type         Rest of Bits

            MSB bit = 1     1xxxxxxx    Variable     7 bits for id
            i = 0           000xxxxx    Operator     5 bits for operator type
            i = 1           001xxxxx    Struct       5 bits for struct's member id
            i = 2           010xxxxx    Literal      5 bits for literal type
            i = 3           011xxxxx    Function     depends

            *MSB - most significant bit
            *i is 2 bit unsigned inteager created from the second and the third bit of the node.
            *x in In Bits section means "not affecting node type".
            *rest of bits in functions may be used in various ways (if ever used):
                *if described function is an dynamic one ROB will be used to store information
                    how many arguments does function have. If ROB is equal to 0 that means that
                    5 bit scale is too small and arguments count is saved is next byte.

        Order of nodes is very important. Very first node is expression root.         
        Next to it, there is it first child node. After it, there are root's child's child nodes ...
        There is a ownership chain that always end on childless nodes, like:
            *Variable
            *Literal
            *Struct
            
        Remember that converting algorithm always takes left side route!
        
        Therefore, with descripted above mechanism in mind, lets change this tree

            +
           / \
          *   3
         / \
        2   v

        into binary form:
        1. Reserve space for information about size:
                Bits: 00000000
                Binary: 0
        1. Lets take root of the tree. It is an basic mathematical operations
            so we create node, give it type of basic math operator (000) and looking at
            operators map add a type appendix (00000).
                Bits: 00000000 00000000
                Binary: 0 0
        2. We take left-sided node of the previous node. Like we did with previous node, we
            create new node, give it operator id (000), and add type information (00010).
                Bits: 00000000 00000000 00000010
                Binary: 0 0 2 
        3. We take next left-sided child of the previous node. Type of new node is literal
            (010) and type is int (00000). In addition to node, we also save the value. 
            Int is 4 byte long, so in total we pass 5 bytes to binary:
                Bits: 00000000 00000000 00000010 01000000 00000000 00000000 00000000 00000020
                Binary: 0 0 2 64 0 0 0 2
        4. We go back to upper node, because there is no child nodes to convert. Now we take 
            right-sided node. This time it is a variable (so 1 in place of MSB). As this is 
            theorethical situation and we don't know variable's id, let's assume it is equal
            to 0. If we go with that we now have:
                Bits: 00000000 00000000 00000010 01000000 00000000 00000000 00000000 00000020
                    10000000
                Binary: 0 0 2 64 0 0 0 2 128
        5. And once again, we need to go up. Now we take right-side child of the root. It is 
            int literal, so as in step 4, we create node (010 node type, 00000 data type), 
            and pass the value.
                Bits: 00000000 00000000 00000010 01000000 00000000 00000000 00000000 00000020
                    10000000 01000000 00000000 00000000 00000000 00000011
                Binary: 0 0 2 64 0 0 0 2 128 64 0 0 0 3
        6. Finaly, we can take size of vector holding all of these bytes and write it to first place.
                Bits: 00000000 00000000 00000010 01000000 00000000 00000000 00000000 00000020
                    10000000 01000000 00000000 00000000 00000000 00000011
                Binary: 13 0 2 64 0 0 0 2 128 64 0 0 0 0 3


        Other example:
            *
           / \
         max  sqrt
       / \  \    \
      a   *  c   3
         / \
        b   3

        1. Reserve space:
                Binary: 0
        2. *:
                Binary: 0 2 
        3. max - dynamical function so we use rest of bits as information about child count - 3
            fits into 5 bits so we definitely are going with that. Next to the node there is 
            function id - let's assume it's 0. (So now it is 01100011 - 96 from function and 
            3 arguments count in total 99)
                Binary: 0 2 99
        4. a - it's variable, say it's id is 0
                Binary: 0 2 99 128
        5. *:
                Binary: 0 2 99 128 2
        6. b, id = 1:
                Binary: 0 2 99 128 2 129
        7. 3, int
                Binary: 0 2 99 128 2 129 64 0 0 0 3
        8. c, id = 2
                Binary: 0 2 99 128 2 129 64 0 0 0 3 130
        9. sqrt, id = 1 (rest of bits not used)
                Binary: 0 2 99 0 128 2 129 64 0 0 0 3 130 96 1 
        10. 3, int
                Binary: 0 2 99 0 128 2 129 64 0 0 0 3 130 96 1 64 0 0 0 3
        11. count bytes
                Binary: 19 2 99 0 128 2 129 64 0 0 0 3 130 96 1 64 0 0 0 3

    */
    std::vector<uint8_t> Binary;

    ExpressionTree::ExpressionTree::Node* current_node;
    current_node = Tree->All_Nodes.at(0);

    //Path to current node. Values are id of child node at given level.
    std::vector<int> path;
    auto GoUp = [&path, &Tree, &current_node]()
    {
        int jump = 0;
        while (true)
        {
            //No nodes left. Finish task.
            if (path.size() == 0)
            {
                return false;
            }
            int last_node_id = path.back();
            last_node_id++;
            if (current_node->upper->OwnedNodes.size() > last_node_id)
            {
                path.back() += 1;
                ExpressionTree::Node* up = current_node->upper;
                for (int i = 0; i < jump; i++)
                    up = up->upper;
                current_node = up->OwnedNodes[last_node_id];
                return true;
            }
            else
            {
                path.erase(path.end() - 1);
                jump++;
            }
        }
    };

    auto Convert = [&current_node, &path, &Binary, GoUp, version]()
    {
        BinaryNode::Type type;
        switch (current_node->Type)
        {
        case ExpressionTree::NodeType::Variable:
            type = BinaryNode::Type::Variable; break;
        case ExpressionTree::NodeType::Function:
            type = BinaryNode::Type::Function; break;
        case ExpressionTree::NodeType::Literal:
            type = BinaryNode::Type::Literal; break;
        case ExpressionTree::NodeType::Operator:
            type = BinaryNode::Type::Operator; break;
        case ExpressionTree::NodeType::Byte:
            type = BinaryNode::Type::Byte; break;
        //StructMember
        default:
            type = BinaryNode::Type::StructMember; break;
        }

        BinaryNode bn(type);

        switch (type)
        {
        case math_parser::BinaryNode::Type::Variable:
        {
            bn.SetRest(current_node->content.Variable);
            Binary.push_back(bn);
            return GoUp(); break;
        }
        case math_parser::BinaryNode::Type::StructMember:
        {
            bn.SetRest(current_node->content.StructMember);
            Binary.push_back(bn);
            return GoUp(); break;
        }
        case math_parser::BinaryNode::Type::Literal:
        {
            bn.SetRest(current_node->content.Literal.first); 
            Binary.push_back(bn);
            //Pass literal value to binary
            for (uint8_t x : version->ConvertLiteralToBinary(current_node->content.Literal.first, current_node->content.Literal.second))
                Binary.push_back(x);
            return GoUp(); break;
        }
        //Operator and Function have child nodes, so we need to go deeper
        case math_parser::BinaryNode::Type::Operator:
        {
            bn.SetRest((uint8_t)current_node->content.OperatorType);
            //Go Deeper
            path.push_back(0);
            current_node = current_node->OwnedNodes[0];
            Binary.push_back(bn);
            return true;
        }
        case math_parser::BinaryNode::Type::Function:
        {
            //TODO: if dynamic write args count here
            bn.SetRest(0); break;
            //Go Deeper
            path.push_back(0);
            current_node = current_node->OwnedNodes[0];
            Binary.push_back(bn);
            return true;
        }
        case math_parser::BinaryNode::Type::Byte:
        {
            Binary.push_back(current_node->content.Byte);
            return GoUp(); break;
        }
        }
        return false;
    };
    
    while(Convert());

    delete Tree;
    for (auto x : simplified_nodes_values)
        delete x;

    //Part 6 - write to fields buffor
    Temp->FieldsBuffor.push_back(Binary.size());
    for (auto b : Binary)
    {
        Temp->FieldsBuffor.push_back(b);
    }

    return 1;
}

#undef Temp