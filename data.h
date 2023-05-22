#pragma once

enum class DataType
{
	Bool,
	Int,
	Float,
	String,
	List,
	Map,
	Function
};

struct Token
{
	int row;
	int col;
	int index;
	struct SourceCode* sourceCodePtr;

	Token();
	Token(int _row, int _col, int _index, struct SourceCode* _sourceCodePtr);
};

struct Data
{
	DataType type;
	bool isConst;
	Token token;

	Data();
	Data(DataType _type, bool _isConst, const Token& _token);

	virtual ~Data();

	virtual void Destroy() = 0;
	virtual void CreateSameType(Data*& inOutData) = 0;
	virtual void ReferenceOther(Data* data) = 0;
	virtual void CopyOther(Data* data) = 0;
	virtual Data* Evaluate() = 0;

	bool AffirmSameType(Data* other);
	bool AffirmSameType(DataType _type);
};