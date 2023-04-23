#include "compiling_task.h"

#include "commons.h"
#include "compiler_utils.h"
#include "USL_version.h"
#include "USL_features.h"

#define Temp USL_Translator::USL_To_Binary::Temp

void CompilingTask::Start(void* source, int size)
{
	Temp = new Compiling_Temp;

	char* begin = (char*)source;

	std::vector<std::string> prompt;

	Version* CurrentVersion = CreateVersion(0);

	//Create compilation conditions
	for (auto& condition : CurrentVersion->GetConditionsNames())
	{
		Temp->CompilationConditions.insert({ condition, false });
	}

	std::vector<uint8_t> binary;

	uint64_t signature_start = 0;
	uint64_t signature_end = 0;

	//Current line of code
	int line = 0;

	auto FinishSig = [&prompt, &signature_start, &signature_end, &begin, &binary, &CurrentVersion, &line]() mutable {
		if ((signature_end - signature_start) != 2)
		{
			auto result = CurrentVersion->Match(&begin[signature_start], signature_end - signature_start);
			signature_start = signature_end;
			if (result.success)
			{
				binary.push_back(result.matching_signature->id);
				int sig_pos = binary.size() - 1;
				//Add information about fields' identyficators
				for (uint8_t b : Temp->FieldsBuffor)
					binary.push_back(b);

				Temp->SignatureWritedFunctionErrors.clear();
				try
				{
					result.matching_signature->LeaveTranslatorTips();
				}
				catch (int offset)
				{
					binary[sig_pos] += offset;
				}	

				for (auto& v : Temp->pass_to_binary_buffor)
					binary.push_back(v);
				Temp->pass_to_binary_buffor.clear();

				if (Temp->SignatureWritedFunctionErrors.size())
					for (auto issue : Temp->SignatureWritedFunctionErrors)
						prompt.push_back(std::string("Error at line ") + std::to_string(line) + ": " + issue + ".");

				return;
			}
			else
			{
				for (auto matching : result.matching_but_wrong)
					for (auto issue : matching.issues)
						prompt.push_back(std::string("Error at line ") + std::to_string(line) + ": " + issue + ".");

				if (result.matching_but_wrong.size() == 0)
					prompt.push_back(std::string("Error at line ") + std::to_string(line) + ": " + "Incorrect line.");

				return;
			}
		}
		else
		{
			signature_start = signature_end;
		}
	};

	auto LineContainsOnlyWhiteChars = [&signature_start, &signature_end, &begin]()
	{
		int i = signature_start;
		bool does = true;
		while (i != signature_end)
		{
			auto& chr = begin[i];
			if (chr != '\0' && chr != '\r' && chr != '\n' &&
				chr != '\t' && chr != ' ')
			{
				does = false;
				break;
			}
			i++;
		}
		return does;
	};

	/*
		Main loop
	*/
	Temp->Context = Context_t::GlobalScope;
	int local_deepness = 0;
	int space_counter = 0;
	bool non_white_char_occured = false;
	for (size_t i = 0; i < size + 1; i++)
	{
		signature_end++;
		if (begin[i] == '\t' && !non_white_char_occured)
		{
			local_deepness++;
			signature_start++;
		}
		else if (begin[i] == ' ' && !non_white_char_occured)
		{
			space_counter++;
			if (space_counter == 4)
			{
				space_counter = 0;
				local_deepness++;
			}
			signature_start++;
		}
		else if (begin[i] == '\n' || i == size)
		{
			if (!non_white_char_occured)
			{
				local_deepness = 0;
				non_white_char_occured = false;
				space_counter = 0;
				continue;
			}		

			line++;
			//line is empty; bypass
			if ((signature_end - signature_start) == 2)
			{
				signature_start = signature_end;
				local_deepness = 0;
				non_white_char_occured = false;
				space_counter = 0;
				continue;
			}
			//line contains only white characters
			else if (LineContainsOnlyWhiteChars())
			{
				signature_start = signature_end;
				continue;
			}
			else if (local_deepness == Temp->Deepness) FinishSig();
			//line breaks block of nested code
			else if ((unsigned int)local_deepness < Temp->Deepness)
			{
				if (local_deepness == 0)
					Temp->Context = Context_t::GlobalScope;

				//Remove variables from leaved scope
				{
					int i = Temp->Variables.size() - 1;
					while (Temp->Variables.size() != 0 && Temp->Variables[i].second.second > local_deepness)
					{
						i--; Temp->Variables.erase(Temp->Variables.end() - (Temp->Variables.size() - i - 1));
					}
				}

				//Put block ends signs ( 0 )
				{
					int i = Temp->Deepness - local_deepness;
					while (i != 0) { i--; binary.push_back(0); }
				}

				Temp->Deepness = local_deepness;
				FinishSig();
			}
			//line is nested without reason. Raise error.
			else prompt.push_back(std::string("Error at line ") + std::to_string(line) + ": " + "Invalid syntax (nested block).");

			local_deepness = 0;
			non_white_char_occured = false;
			space_counter = 0;
		}
		else if (begin[i] != '\t' && begin[i] != ' ')
			non_white_char_occured = true;
	}

	//Check if compilation conditions are met
	for (auto& condition : Temp->CompilationConditions)
	{
		if (!condition.second)
		{
			//Error; Not met condition
			prompt.push_back(std::string("Error: ") + CurrentVersion->GetConditionErrorText(condition.first));
		}
	}

	result.success = !prompt.size();
	result.prompt = prompt;
	result.data = binary;

	delete CurrentVersion;
	delete Temp;
}