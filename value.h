#pragma once
#include "data.h"
#include "source_code.h"
#include "memory_pool.h"

typedef bool Bool;
typedef int Int;
typedef float Float;
typedef std::string String;
struct List;
struct Map;
struct Function;

template<typename T>
struct Value : public Data
{
	T* valuePtr;
	int* users;

	Value(DataType _type, bool _isConst, const Token& _token) :
		Data(_type, _isConst, _token)
	{
		valuePtr = nullptr;
		users = nullptr;
	}

	virtual void Destroy() override
	{
		if (users == nullptr)
			return;

		if (--(*users) == 0)
		{
			//delete users;
			//delete valuePtr;
			Memory<int>().Delete(users);
			Memory<T>().Delete(valuePtr);
		}

		users = nullptr;
		valuePtr = nullptr;
	}

	void Init()
	{
		//valuePtr = new T();
		//users = new int(1);
		valuePtr = Memory<T>().New();
		users = Memory<int>().New();
		*users = 1;
	}

	virtual void CreateSameType(Data*& inOutData) override
	{
		inOutData = Memory<Value<T>>().New(type, false, token);
		// inOutData = new Value<T>(type, false, token);
	}

	virtual Data* Evaluate() override
	{
		if (type == DataType::Function)
		{
			Value<Function>* self = dynamic_cast<Value<Function>*>(this);
			self->valuePtr->Call();
			return self->valuePtr->returnValue;
		}

		return this;
	}

	void SetValue(const T& value)
	{
		if (users == nullptr)
			Init();

		*valuePtr = value;
	}

	virtual ~Value()
	{
		Destroy();
	}

	virtual void ReferenceOther(Data* data) override
	{
		if (isConst)
		{
			token.sourceCodePtr->PrintError(token, "trying to change a constant variable");
			return;
		}

		if (!AffirmSameType(data))
			return;

		Value<T>* value = dynamic_cast<Value<T>*>(data);

		Destroy();
		valuePtr = value->valuePtr;
		users = value->users;
		isConst = value->isConst;
		(*users)++;
	}

	virtual void CopyOther(Data* data) override
	{
		if (isConst)
		{
			token.sourceCodePtr->PrintError(token, "trying to change a constant variable");
			return;
		}

		if (!AffirmSameType(data))
			return;

		Value<T>* value = dynamic_cast<Value<T>*>(data);

		if (users == nullptr)
			Init();

		*valuePtr = *value->valuePtr;
	}
};