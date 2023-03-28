#pragma once
#include "USL_Translator.h"

class CompilingTask
{
public:
	bool finished = true;
	USL_Translator::TranslationResult result;
	void Start(void* source, int size);
};