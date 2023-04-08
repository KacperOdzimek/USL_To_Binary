#include "../Include/USL_To_Binary.h"

#include "compiling_task.h"

const char* USL_Translator::USL_To_Binary::src_type_n    = "USL_Shader";
const char* USL_Translator::USL_To_Binary::target_type_n = "USL_Binary";

Compiling_Temp* USL_Translator::USL_To_Binary::Temp      = nullptr;

USL_Translator::TranslationResult USL_Translator::USL_To_Binary::Translate(Data InData)
{
	auto Task = new CompilingTask;
	Task->Start(InData.position, InData.size);
	return Task->result;
}