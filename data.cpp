#include "data.h"
#include "source_code.h"
#include <string>

Token::Token() :
	row(0),
	col(0),
	index(0),
	sourceCodePtr(nullptr)
{}

Token::Token(int _row, int _col, int _index, struct SourceCode* _sourceCodePtr) :
	row(_row),
	col(_col),
	index(_index),
	sourceCodePtr(_sourceCodePtr)
{}

Data::Data()
{
	type = DataType::Bool;
	isConst = false;
	token = {};
}

Data::Data(DataType _type, bool _isConst, const Token& _token)
{
	type = _type;
	isConst = _isConst;
	token = _token;
}

Data::~Data() {}

bool Data::AffirmSameType(Data* other)
{
	if (other->type == type)
		return true;

	static std::string names[]
	{
		"bool",
		"int",
		"float",
		"string",
		"list",
		"map",
		"function"
	};

	token.sourceCodePtr->PrintError(token, "type mismatch between " + names[(int)other->type] + " and " + names[(int)type]);
	return false;
}

bool Data::AffirmSameType(DataType _type)
{
	if (_type == type)
		return true;

	static std::string names[]
	{
		"bool",
		"int",
		"float",
		"string",
		"list",
		"map",
		"function"
	};

	token.sourceCodePtr->PrintError(token, "type mismatch between " + names[(int)_type] + " and " + names[(int)type]);
	return false;
}