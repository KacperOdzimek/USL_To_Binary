#pragma once
#include "commons.h"
#include "USL_version.h"
#include "operator_type.h"

#define Temp Version::AccessTemp()

namespace math_parser
{
	struct RawExpressionTree
	{
	private:
		//Allows to keep chars out of the constructor's scope
		std::vector<char> chars_buff;
		std::vector<RawExpressionTree*> subtrees;
	public:
		struct Node;
		std::vector<Node*> All_Nodes;

		//May throw exception!
		RawExpressionTree(utils::TextPointer exp, Version* V, std::vector<std::string>& issues);

		~RawExpressionTree()
		{
			for (auto& n : All_Nodes)
				delete n;
			for (auto& subtree : subtrees)
				delete subtree;
		}

		struct Node
		{
			Node* Upper = nullptr;
			utils::TextPointer Content;
			std::vector<Node*> Owned;
			void Own(Node* n)
			{
				Owned.push_back(n);
				n->Upper = this;
			}	
			bool IsOperator()
			{
				return (
					Content.begin[0] == '+' ||
					Content.begin[0] == '-' ||
					Content.begin[0] == '*' ||
					Content.begin[0] == '/' ||
					Content.begin[0] == '^'
				);
			}
		};

		Node* CreateNode()
		{
			auto n = new RawExpressionTree::Node;
			All_Nodes.push_back(n);
			return n;
		}

		void AddUppest(utils::TextPointer content)
		{
			auto c = All_Nodes[0];
			while (c->Upper != nullptr)
				c = c->Upper;

			auto n = CreateNode();
			n->Content = content;
			n->Owned.push_back(c);
			c->Upper = n;
		}

		void AddUppest(char* content)
		{
			auto c = utils::TextPointer(content, 1);
			AddUppest(c);
		}

		Node* Uppest()
		{
			auto c = All_Nodes[0];
			while (c->Upper != nullptr)
				c = c->Upper;
			return c;
		}
	};

	struct ExpressionTree
	{
		enum class NodeType
		{
			Operator, Variable, Literal, Function, StructMember, Byte
		};

		struct Node
		{		
			NodeType Type;
			union content_u
			{
				utils::TextPointer FunctionName;
				//type id, literal
				std::pair<int, utils::TextPointer> Literal;
				//variable id
				int Variable;
				//member id
				int StructMember;
				OperatorType_T OperatorType;
				uint8_t Byte;
                content_u() {OperatorType = OperatorType_T::add;};
			};
			content_u content;
			std::vector<Node*> OwnedNodes;
			Node* upper = nullptr;
			Node()
			{
				content.OperatorType = OperatorType_T::add;
				OwnedNodes = {};
			}
			int GetNodeDataTypeId(Version* version);
		};
	
		std::vector<Node*> All_Nodes;

		ExpressionTree(RawExpressionTree& RET, Version* version, std::vector<std::string>& issues);
		~ExpressionTree()
		{
			for (auto n : All_Nodes)
				delete n;
		}
	};

	struct BinaryNode
	{
		enum class Type
		{
			Variable, Operator, StructMember, Literal, Function, Byte
		};
		const Type type;
		uint8_t rest = 0;
		BinaryNode(Type _type) : type(_type) {};
		bool SetRest(uint8_t _rest);
		operator uint8_t() const
		{
			switch (type)
			{
			case Type::Variable:
				return rest + 128;
			case Type::Operator:
				return rest;
			case Type::StructMember:
				return rest + 32;
			case Type::Literal:
				return rest + 64;
			case Type::Function:
				return rest + 96;
			}
			return 0;
		};
	};
};

#undef Temp