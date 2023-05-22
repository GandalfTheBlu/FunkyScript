#include "script.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

class Time
{
private:
	std::chrono::steady_clock::time_point startTime;

	Time()
	{
		startTime = std::chrono::high_resolution_clock::now();
	}

public:
	static Time& Instance()
	{
		static Time instance;
		return instance;
	}

	float SecondsSinceStart()
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<float>(currentTime - startTime).count();
	}
};


int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cout << "\n[ERROR] incorrect arguments sent to program, expected path to script-folder" << std::endl;
		return 1;
	}

	Script::scriptFunctions["run"] = [](List& args)
	{
		if (args.size() != 1)
			return;

		Data* first = args[0]->Evaluate();
		if (first == nullptr)
			return;

		if (first->AffirmSameType(DataType::String))
		{
			std::string& path = *dynamic_cast<Value<String>*>(first)->valuePtr;
			Script script;
			if (script.LoadScript(path))
			{
				script.Run();
			}
		}
	};

	Script::scriptFunctions["read_txt"] = [](List& args)
	{
		if (args.size() != 2)
			return;

		Data* first = args[0]->Evaluate();
		if (first == nullptr)
			return;

		Data* second = args[1]->Evaluate();
		if (second == nullptr)
			return;

		if (first->AffirmSameType(DataType::String) && second->AffirmSameType(DataType::String))
		{
			std::string path = Script::workingDirectory + *dynamic_cast<Value<String>*>(first)->valuePtr;
			std::string& text = *dynamic_cast<Value<String>*>(second)->valuePtr;
			std::ifstream file;
			file.open(path);

			if (file.is_open())
			{
				std::stringstream stringStream;
				stringStream << file.rdbuf();
				text = stringStream.str();
				file.close();

				return;
			}

			std::cout << ("[ERROR] failed to open file '" + path + "'") << std::endl;
		}
	};

	Script::scriptFunctions["seconds_now"] = [](List& args)
	{
		if (args.size() != 1)
			return;

		Data* first = args[0]->Evaluate();
		if (first == nullptr || !first->AffirmSameType(DataType::Float))
			return;

		Value<Float>* seconds = dynamic_cast<Value<Float>*>(first);
		seconds->SetValue(Time::Instance().SecondsSinceStart());
	};

	Script::workingDirectory = argv[1];

	Script s;
	if (s.LoadScript("main.funky"))
	{
		s.Run();
	}

	return 0;
}