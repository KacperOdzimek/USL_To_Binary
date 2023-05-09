#pragma once
#include "USL_Translator.h"

struct Compiling_Temp;
class Version;

namespace USL_Translator
{
	class USL_To_Binary : public TranslatorBase
	{
	friend Version;
	public:
		TranslationResult Translate(Data InData, LoadExternalFileCallback LLC);
		virtual const char* src_type()    { return src_type_n; };
		virtual const char* target_type() { return target_type_n; };
	private:
		static const char* src_type_n;
		static const char* target_type_n;
	public:
		static Compiling_Temp* Temp;
		static LoadExternalFileCallback LEFC;
	};
}
