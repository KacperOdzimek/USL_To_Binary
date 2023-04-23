#include "compiler_utils.h"

namespace utils
{
	TextPointer TextPointer::Get(char* start)
	{
		int spaces = 0;

		while (start[spaces] == ' ')
			++spaces;

		int i = spaces;

		while (true)
		{
			if (
				*(start + i) == '\n' ||
				*(start + i) == '\r' ||
				*(start + i) == '\0' ||

				*(start + i) == '(' ||
				*(start + i) == ')' ||
				*(start + i) == '[' ||
				*(start + i) == ']' ||
				*(start + i) == ',' ||

				*(start + i) == ' ')
				return TextPointer(start + spaces, (int)i - spaces);
			i++;
		}
		return TextPointer(start + spaces, (int)i - spaces);
	}

	bool TextPointer::operator == (const TextPointer other) const
	{
		if (other.length != length) return false;
		for (int i = 0; i < length; i++)
			if (other.begin[i] != begin[i])
				return false;
		return true;
	}

	bool TextPointer::operator != (const TextPointer other) const
	{
		return !(*this == other);
	};

#ifdef DEBUG
	void TextPointer::Print()
	{
		for (int i = 0; i < length; i++)
			std::cout << begin[i];
		std::cout << "\n";
	}
#endif

	int CountSpaces(const char* src)
	{
		int i = 0;
		int spaces = 0;
		while (src[i] != '\0' && src[i] != '\r' && src[i] != '\n')
		{
			if (src[i] == ' ')
				spaces++;
			i++;
		}
		return spaces;
	}

	bool Contains(std::vector<const char*> v, char* obj)
	{
		auto w = TextPointer::Get(obj);
		for (auto w_s : v)
		{
			auto i = TextPointer::Get((char*)w_s);
			if (w == i)
				return 1;
		}
		return 0;
	}

	int Find(std::vector<const char*> v, char* obj)
	{
		auto w = TextPointer::Get(obj);
		int j = 0;
		for (auto w_s : v)
		{
			auto i = TextPointer::Get((char*)w_s);
			if (w == i)
				return j;
			j++;
		}
		return -1;
	}
	
	bool IsInteger(utils::TextPointer& src)
	{
		if (src.length == 0) return 0;
		int i = 0;
		if (*src.begin == '-')
		{
			i++;
			while (src.begin[i] == ' ')
				i++;
		}

		for (; i != src.length; i++)
			if ((!(src.begin[i] >= '0' && src.begin[i] <= '9')))
				return 0;
		return 1;
	}

	bool IsFloat(utils::TextPointer& src)
	{
		if (src.length == 0) return 0;
		bool used_comma = 0;
		for (int i = 0; i != src.length; i++)
			if (!(src.begin[i] >= '0' && src.begin[i] <= '9'))
			{
				if (src.begin[i] == '.')
					if (used_comma)
						return 0;
					else
						used_comma = 1;
				else
					return 0;
			}
		return 1;
	}

	std::pair<std::vector<TextPointer>, int> ExtrudeArguments(const char* src, BracketsType BT)
	{
		int i = 0;
		char opening_bracket = ' ';
		char closing_bracket = ' ';

#define Case(BT, ob, cb) case utils::BracketsType::BT: opening_bracket = ob; closing_bracket = cb; break;
		switch (BT)
		{
		Case(parentheses, '(', ')')
		Case(square, '[', ']')
		Case(curly, '{', '}')
		Case(angle, '<', '>')
		}
#undef Case

		//Looping to start
		while (src[i] != opening_bracket)
		{
			++i;
			if (src[i] != ' ' && src[i] != opening_bracket)
				return { {}, 0 };
		}
		++i;

		int j = i;
		bool not_empty = false;
		while (src[j] != closing_bracket && j < 1000)
		{
			switch (src[j])
			{
			case ' ': case '\t':	   break;
			default: not_empty = true; break;
			}
			j++;
		}

		if (!not_empty)
			return { {}, j + 1};

		std::vector<TextPointer> result;

		int deep = 0;
		while (src[i - 1] != closing_bracket)
		{
			if (src[i] == ' ') { i++; continue; }

			int a = 1;
			int b = 0;

			while (!(src[i + b] == ',' && deep == 0) && src[i + b] != closing_bracket)
			{
				if (src[i + b] != ' ' && !(src[i + b] == ',' && deep == 0) && src[i + b] != closing_bracket)
				{
					a = b + 1;
				}
				switch (src[i + b])
				{
				case '(': case '{': case '[': case '<': deep++; break;
				case ')': case '}': case ']': case '>': deep--; break;
				}
				b++;
				if (b == 1000) return { {},0 };
			}

			result.push_back({ (char*)(src + i), a == 0 ? 1 : a });
			i += b + 1;
		}

		return { result, i };
	}
}