#pragma once
#include "commons.h"

#define DEBUG

namespace utils
{
	int  CountSpaces(const char* src);
	bool Contains(std::vector<const char*> v, char* obj);
	int  Find	(std::vector<const char*> v, char* obj);
	enum class BracketsType { parentheses, square, curly, angle };

	/*
		Example call:
		src = [ int a, int b]
		BT = BracketsType.square
		result = pair{vector{  TextPointer(int a, 5),  TextPointer(int b, 5) }, 15}
	*/
	struct TextPointer;
	//Args, Number of chars
	std::pair<std::vector<TextPointer>, int> ExtrudeArguments(const char* src, BracketsType BT);

	/*
		Used instead of string. It doesn't copy anything, just work on given pointer
	*/
	struct TextPointer
	{
		char* begin = nullptr;
		int length = 0;
	 TextPointer() {};
	 TextPointer(char* _begin, int _length) : begin(_begin), length(_length) {};
		bool operator == (TextPointer& other) const;
		bool operator != (TextPointer& other) const;
		static TextPointer Get(char* start);
#ifdef DEBUG
		void Print();
#endif
	};

	bool IsInteger(utils::TextPointer& src);
	bool IsFloat(utils::TextPointer& src);
}