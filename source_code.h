#pragma once
#include <string>

struct SourceCode
{
	std::string path;
	int row;
	int col;
	int index;
	std::string text;

	SourceCode();

	bool ReadFile(const std::string& _path);

	void Reset();

	char CurrentChar();

	bool NextChar();

	void MoveAlong(int steps);

	void PrintError(const struct Token& token, const std::string& message);

	void PrintErrorAtCurrentIndex(const std::string& message);

	std::string Substring(int first, int last);

	bool BeginsWith(const std::string& str);
};