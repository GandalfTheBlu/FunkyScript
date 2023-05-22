#include "source_code.h"
#include "data.h"
#include <fstream>
#include <sstream>
#include <stdio.h>

void LogError(const std::string& message)
{
	printf("[ERROR][SourceCode] %s\n", message.c_str());
}

SourceCode::SourceCode()
{
	row = -1;
	col = -1;
	index = -1;
}

bool SourceCode::ReadFile(const std::string& _path)
{
	path = _path;
	row = 1;
	col = 1;
	index = 0;

	std::ifstream file;
	file.open(path);

	if (file.is_open())
	{
		std::stringstream stringStream;
		stringStream << file.rdbuf();
		text = stringStream.str();
		file.close();
		return true;
	}

	LogError("failed to read source code at '" + path + "'");
	return false;
}

void SourceCode::Reset()
{
	row = 1;
	col = 1;
	index = 0;
}

char SourceCode::CurrentChar()
{
	return text[index];
}

bool SourceCode::NextChar()
{
	if (index + 1 >= text.size())
		return false;

	col++;
	if (CurrentChar() == '\n')
	{
		col = 1;
		row++;
	}
	index++;

	return true;
}

void SourceCode::MoveAlong(int steps)
{
	for (int i = 0; i < steps && NextChar(); i++);
}

void SourceCode::PrintError(const Token& token, const std::string& message)
{
	int rowStart = 0;
	for (int i = token.index; i >= 0 && text[i] != '\n'; rowStart = i--);

	int rowEnd = 0;
	for (int i = token.index; i < text.size() && text[i] != '\n'; rowEnd = i++);

	std::string rowSample = text.substr(rowStart, rowEnd - rowStart + 1);
	std::string str = "in '" + path + "' on line " + std::to_string(token.row) + " col " + std::to_string(token.col)
		+ "\n" + message + ":\n" + rowSample + "\n";

	for (int i = rowStart; i < token.index; i++)
	{
		if (text[i] == '\t')
			str += "--------";
		else
			str += "-";
	}

	str += "^";

	for (int i = token.index + 1; i <= rowEnd; i++)
	{
		if (text[i] == '\t')
			str += "---";
		else
			str += "-";
	}

	LogError(str);
}

void SourceCode::PrintErrorAtCurrentIndex(const std::string& message)
{
	PrintError(Token(row, col, index, nullptr), message);
}

std::string SourceCode::Substring(int first, int last)
{
	return text.substr(first, last - first + 1);
}

bool SourceCode::BeginsWith(const std::string& str)
{
	if (str.size() + index > text.size())
		return false;

	for (int i = 0; i < str.size(); i++)
		if (str[i] != text[index + i])
			return false;

	return true;
}