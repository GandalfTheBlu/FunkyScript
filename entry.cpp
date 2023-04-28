#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <stdio.h>
#include <utility>
#include <fstream>
#include <sstream>
#include <regex>

void LogError(const std::string& message)
{
	printf("\033[0;31m[ERROR] %s\n", message.c_str());
	printf("\033[0;37m");
}

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
};

struct SourceCode
{
	std::string path;
	int row;
	int col;
	int index;
	std::string text;

	SourceCode();

	bool ReadFile(const std::string _path);

	void Reset();

	char CurrentChar();

	bool NextChar();

	void MoveAlong(int steps);

	void PrintError(const Token& token, const std::string& message);

	void PrintErrorAtCurrentIndex(const std::string& message);

	std::string Substring(int first, int last);

	bool BeginsWith(const std::string& str);
};

struct Data
{
	DataType type;
	bool isConst;
	Token token;

	Data(DataType _type, bool _isConst, const Token& _token)
	{
		type = _type;
		isConst = _isConst;
		token = _token;
	}

	virtual ~Data(){}

	virtual void Destroy() = 0;
	virtual void CreateSameType(Data*& inOutData) = 0;
	virtual void ReferenceOther(Data* data) = 0;
	virtual void CopyOther(Data* data) = 0;
	virtual Data* Evaluate() = 0;

	bool AffirmSameType(Data* other)
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

	bool AffirmSameType(DataType _type)
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
};

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
			delete users;
			delete valuePtr;
		}

		users = nullptr;
		valuePtr = nullptr;
	}

	void Init()
	{
		valuePtr = new T();
		users = new int(1);
	}

	virtual void CreateSameType(Data*& inOutData) override
	{
		inOutData = new Value<T>(type, false, token);
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

		if(users == nullptr)
			Init();

		*valuePtr = *value->valuePtr;
	}
};

typedef bool Bool;
typedef int Int;
typedef float Float;
typedef std::string String;

struct List
{
	std::vector<Data*> list;

	List(){}

	List(const List& other)
	{
		list.reserve(other.list.size());
		for (auto& elem : other.list)
		{
			Data* copy = nullptr;
			elem->CreateSameType(copy);
			copy->ReferenceOther(elem);
			list.push_back(copy);
		}
	}

	List& operator=(const List& other)
	{
		list.reserve(other.list.size());
		for (auto& elem : other.list)
		{
			Data* copy = nullptr;
			elem->CreateSameType(copy);
			copy->ReferenceOther(elem);
			list.push_back(copy);
		}
		return *this;
	}

	~List()
	{
		for (auto& e : list)
			delete e;
	}

	void push_back(Data* data)
	{
		list.push_back(data);
	}

	int size() const
	{
		return (int)list.size();
	}

	Data*& operator[](int i)
	{
		return list[i];
	}

	Data*& at(int i)
	{
		return list.at(i);
	}

	void erase(const std::vector<Data*>::const_iterator& itr)
	{
		list.erase(itr);
	}

	std::vector<Data*>::const_iterator begin()
	{
		return list.begin();
	}
};

struct Map
{
	std::unordered_map<std::string, Data*> map;

	Map(){}

	Map(const Map& other)
	{
		map.reserve(other.map.size());
		for (auto& elem : other.map)
		{
			Data* copy = nullptr;
			elem.second->CreateSameType(copy);
			copy->ReferenceOther(elem.second);
			map[elem.first] = copy;
		}
	}

	Map& operator=(const Map& other)
	{
		map.reserve(other.map.size());
		for (auto& elem : other.map)
		{
			Data* copy = nullptr;
			elem.second->CreateSameType(copy);
			copy->ReferenceOther(elem.second);
			map[elem.first] = copy;
		}
		return *this;
	}

	~Map()
	{
		for (auto& e : map)
			delete e.second;
	}

	void insert(const std::pair<std::string, Data*>& pair)
	{
		map.insert(pair);
	}

	int count(const std::string& key)
	{
		return map.count(key);
	}

	Data*& operator[](const std::string& key)
	{
		return map[key];
	}

	Data*& at(const std::string& key)
	{
		return map.at(key);
	}

	void erase(const std::string& key)
	{
		map.erase(key);
	}
};

struct Function
{
	Function* parent;
	Data* returnValue;
	std::vector<Data*> arguments;
	void (*function)(Function*);
	std::unordered_map<std::string, Data*> variables;
	std::vector<std::string> parameterNames;

	Function()
	{
		parent = nullptr;
		returnValue = nullptr;
		function = nullptr;
	}

	Function(void(*_function)(Function*))
	{
		parent = nullptr;
		returnValue = nullptr;
		function = _function;
	}

	Function(const Function& other)
	{
		parent = nullptr;
		returnValue = nullptr;
		for (auto& arg : other.arguments)
		{
			Data* arg_copy = nullptr;
			arg->CreateSameType(arg_copy);
			arg_copy->CopyOther(arg);
			AddArgument(arg_copy);
		}

		function = other.function;
		parameterNames = other.parameterNames;
	}

	Function& operator=(const Function& other)
	{
		parent = nullptr;
		returnValue = nullptr;
		for (auto& arg : other.arguments)
		{
			Data* arg_copy = nullptr;
			arg->CreateSameType(arg_copy);
			arg_copy->CopyOther(arg);
			AddArgument(arg_copy);
		}

		function = other.function;
		parameterNames = other.parameterNames;

		return *this;
	}

	~Function()
	{
		for (auto& arg : arguments)
			delete arg;

		delete returnValue;
	}


	bool GetVariableOwner(const std::string& name, Function*& outOwner)
	{
		if (variables.count(name) != 0)
		{
			outOwner = this;
			return true;
		}

		if (parent != nullptr)
			return parent->GetVariableOwner(name, outOwner);

		return false;
	}

	bool GetVariable(const std::string& name, Data*& outVar)
	{
		Function* owner = nullptr;
		if (!GetVariableOwner(name, owner))
			return false;
		
		outVar = owner->variables[name];
		return true;
	}

	bool AddVariable(const std::string& name, Data* var)
	{
		if (variables.count(name) != 0)
			return false;

		variables[name] = var;
	}

	void AddArgument(Data* data)
	{
		if (data->type == DataType::Function)
		{
			dynamic_cast<Value<Function>*>(data)->valuePtr->parent = this;
		}
		arguments.push_back(data);
	}

	void Call()
	{
		delete returnValue;
		returnValue = nullptr;

		function(this);

		for (auto& v : variables)
			delete v.second;

		variables.clear();
	}

	bool CheckArgumens(int count)
	{
		if (arguments.size() < count)
		{
			LogError("too few arguments sent to function");
			return false;
		}
		return true;
	}
};


SourceCode::SourceCode()
{
	row = -1;
	col = -1;
	index = -1;
}

bool SourceCode::ReadFile(const std::string _path)
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
	for(int i = token.index; i >= 0 && text[i] != '\n'; rowStart = i--);

	int rowEnd = 0;
	for (int i = token.index; i < text.size() && text[i] != '\n'; rowEnd = i++);

	std::string rowSample = text.substr(rowStart, rowEnd-rowStart+1);
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

	for (int i = token.index+1; i <= rowEnd; i++)
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
	PrintError({row, col, index}, message);
}

std::string SourceCode::Substring(int first, int last)
{
	return text.substr(first, last-first+1);
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

std::unordered_map<std::string, void(*)(List&)> scriptFunctions;

#define SCRIPT_FUNCTION(name, body) scriptFunctions[name] = [](List& args)body;
#define AFFIRM_DATA(data) if(data == nullptr){return;}

struct FunctionLibrary
{
	static void F_Do(Function* self)
	{
		for (auto& arg : self->arguments)
		{
			arg->Evaluate();

			if (self->returnValue != nullptr && self->parent != nullptr)
			{
				self->returnValue->CreateSameType(self->parent->returnValue);
				self->parent->returnValue->ReferenceOther(self->returnValue);
				return;
			}
		}
	}

	static void F_Function(Function* self)
	{
		for (auto& arg : self->arguments)
		{
			arg->Evaluate();

			if (self->returnValue != nullptr)
			{
				return;
			}
		}
	}

	static void Helper_PrintBool(Data* data)
	{
		std::cout << std::boolalpha << *dynamic_cast<Value<Bool>*>(data)->valuePtr;
	}

	static void Helper_PrintInt(Data* data)
	{
		std::cout << *dynamic_cast<Value<Int>*>(data)->valuePtr;
	}

	static void Helper_PrintFloat(Data* data)
	{
		std::cout << *dynamic_cast<Value<Float>*>(data)->valuePtr;
	}

	static void Helper_PrintString(Data* data)
	{
		const String& s = *dynamic_cast<Value<String>*>(data)->valuePtr;
		
		for (int i = 0; i < s.size(); i++)
		{
			if (s[i] == '\\' && i+1 < s.size() && s[++i] == 'n')
				std::cout << std::endl;
			else 
				std::cout << s[i];
		}
	}

	static void Helper_PrintList(Data* data)
	{
		List& l = *dynamic_cast<Value<List>*>(data)->valuePtr;
		std::cout << "[";
		for (int i=0; i<l.size(); i++)
		{
			if(i > 0)
				std::cout << ", ";

			Helper_PrintAny(l[i]);
		}
		std::cout << "]";
	}

	static void Helper_PrintMap(Data* data)
	{
		Map& m = *dynamic_cast<Value<Map>*>(data)->valuePtr;
		std::cout << "[";
		int i = 0;
		for (auto& e : m.map)
		{
			if (i > 0)
				std::cout << ", ";

			std::cout << "{" << e.first << ": ";
			Helper_PrintAny(e.second);
			std::cout << "}";
			i++;
		}
		std::cout << "]";
	}

	static void Helper_PrintFunction(Data* data)
	{
		std::cout << "function()";
	}

	static void Helper_PrintAny(Data* d)
	{
		static void(*printFunctions[])(Data*) {
			Helper_PrintBool,
			Helper_PrintInt,
			Helper_PrintFloat,
			Helper_PrintString,
			Helper_PrintList,
			Helper_PrintMap,
			Helper_PrintFunction
		};

		printFunctions[(int)d->type](d);
	}

	static void F_Print(Function* self)
	{
		for (auto& arg : self->arguments)
		{
			Data* res = arg->Evaluate();
			AFFIRM_DATA(res)

			Helper_PrintAny(res);
		}
	}

	static void F_ReturnCopy(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* ret = self->arguments[0]->Evaluate();
		AFFIRM_DATA(ret)

		ret->CreateSameType(self->parent->returnValue);
		self->parent->returnValue->CopyOther(ret);
	}

	static void F_ReturnReference(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* ret = self->arguments[0]->Evaluate();
		AFFIRM_DATA(ret)

		ret->CreateSameType(self->parent->returnValue);
		self->parent->returnValue->ReferenceOther(ret);
	}

	static void F_SetCopy(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type != DataType::String)
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected variable name (string)");
			return;
		}

		Value<String>* name = dynamic_cast<Value<String>*>(first);
		Data* data = self->arguments[1]->Evaluate();
		AFFIRM_DATA(data)

		Data* current = nullptr;

		if (self->GetVariable(*name->valuePtr, current))
		{
			current->CopyOther(data);
		}
		else
		{
			data->CreateSameType(current);
			current->CopyOther(data);
			self->parent->AddVariable(*name->valuePtr, current);
		}
	}

	static void F_SetReference(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type != DataType::String)
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected variable name (string)");
			return;
		}

		Value<String>* name = dynamic_cast<Value<String>*>(first);
		Data* data = self->arguments[1]->Evaluate();
		AFFIRM_DATA(data)

		Data* current = nullptr;

		if (self->GetVariable(*name->valuePtr, current))
		{
			current->ReferenceOther(data);
		}
		else
		{
			data->CreateSameType(current);
			current->ReferenceOther(data);
			self->parent->AddVariable(*name->valuePtr, current);
		}
	}

	static void F_AddElementsAsCopies(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type != DataType::List && first->type != DataType::Map)
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected list or map");
			return;
		}
		if (first->isConst)
		{
			first->token.sourceCodePtr->PrintError(first->token, "tried to change const container");
			return;
		}

		if (first->type == DataType::List)
		{
			Value<List>* list = dynamic_cast<Value<List>*>(first);
			for (int i = 1; i < self->arguments.size(); i++)
			{
				Data* copy = nullptr;
				Data* val = self->arguments[i]->Evaluate();
				AFFIRM_DATA(val)

				val->CreateSameType(copy);
				copy->CopyOther(val);
				list->valuePtr->push_back(copy);
			}
		}
		else
		{
			Value<Map>* map = dynamic_cast<Value<Map>*>(first);
			for (int i = 1; i+1 < self->arguments.size(); i+=2)
			{
				Data* copy = nullptr;
				Data* key = self->arguments[i]->Evaluate();
				AFFIRM_DATA(key)

				if (key->type != DataType::String)
				{
					key->token.sourceCodePtr->PrintError(key->token, "expected string as key");
					return;
				}

				Value<String>* keyStr = dynamic_cast<Value<String>*>(key);
				Data* val = self->arguments[i+1]->Evaluate();
				AFFIRM_DATA(val)

				val->CreateSameType(copy);
				copy->CopyOther(val);
				map->valuePtr->insert({ *keyStr->valuePtr, copy });
			}
		}
	}

	static void F_AddElementsAsReferences(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type != DataType::List && first->type != DataType::Map)
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected list or map");
			return;
		}
		if (first->isConst)
		{
			first->token.sourceCodePtr->PrintError(first->token, "tried to change const container");
			return;
		}

		if (first->type == DataType::List)
		{
			Value<List>* list = dynamic_cast<Value<List>*>(first);
			for (int i = 1; i < self->arguments.size(); i++)
			{
				Data* copy = nullptr;
				Data* val = self->arguments[i]->Evaluate();
				AFFIRM_DATA(val)

				val->CreateSameType(copy);
				copy->ReferenceOther(val);
				list->valuePtr->push_back(copy);
			}
		}
		else
		{
			Value<Map>* map = dynamic_cast<Value<Map>*>(first);
			for (int i = 1; i + 1 < self->arguments.size(); i += 2)
			{
				Data* copy = nullptr;
				Data* key = self->arguments[i]->Evaluate();
				AFFIRM_DATA(key)

				if (key->type != DataType::String)
				{
					key->token.sourceCodePtr->PrintError(key->token, "expected string as key");
					return;
				}

				Value<String>* keyStr = dynamic_cast<Value<String>*>(key);
				Data* val = self->arguments[i + 1]->Evaluate();
				AFFIRM_DATA(val)

				val->CreateSameType(copy);
				copy->ReferenceOther(val);
				map->valuePtr->insert({ *keyStr->valuePtr, copy });
			}
		}
	}

	static void F_GetElement(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		Data* second = self->arguments[1]->Evaluate();
		AFFIRM_DATA(second)

		if (first->type == DataType::List)
		{
			if (!second->AffirmSameType(DataType::Int))
				return;
			
			Value<List>* list = dynamic_cast<Value<List>*>(first);
			Value<Int>* index = dynamic_cast<Value<Int>*>(second);
			
			int i = 0;
			if (*index->valuePtr == -1)
				i = list->valuePtr->size() - 1;
			else
				i = *index->valuePtr;

			if (i < 0 || i >= list->valuePtr->size())
			{
				second->token.sourceCodePtr->PrintError(second->token, "index out of range");
				return;
			}

			Data* item = list->valuePtr->at(i);
			item->CreateSameType(self->returnValue);
			self->returnValue->ReferenceOther(item);
		}
		else if(first->type == DataType::Map)
		{
			if (!second->AffirmSameType(DataType::String))
				return;

			Value<Map>* map = dynamic_cast<Value<Map>*>(first);
			Value<String>* key = dynamic_cast<Value<String>*>(second);

			std::string& k = *key->valuePtr;

			if (map->valuePtr->count(k) == 0)
			{
				second->token.sourceCodePtr->PrintError(second->token, "key not found");
				return;
			}

			Data* item = map->valuePtr->at(k);
			item->CreateSameType(self->returnValue);
			self->returnValue->ReferenceOther(item);
		}
		else if (first->type == DataType::String)
		{
			if (!second->AffirmSameType(DataType::Int))
				return;

			Value<String>* str = dynamic_cast<Value<String>*>(first);
			Value<Int>* index = dynamic_cast<Value<Int>*>(second);

			int i = 0;
			if (*index->valuePtr == -1)
				i = str->valuePtr->size() - 1;
			else
				i = *index->valuePtr;

			if (i < 0 || i >= str->valuePtr->size())
			{
				second->token.sourceCodePtr->PrintError(second->token, "index out of range");
				return;
			}

			Value<String>* val = new Value<String>(DataType::String, false, first->token);
			std::string s;
			s += str->valuePtr->at(i);
			val->SetValue(s);
			self->returnValue = val;
		}
		else
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected list or map");
		}
	}

	static void F_RemoveElement(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		Data* second = self->arguments[1]->Evaluate();
		AFFIRM_DATA(second)

		if (first->type == DataType::List)
		{
			if (!second->AffirmSameType(DataType::Int))
				return;

			Value<List>* list = dynamic_cast<Value<List>*>(first);
			Value<Int>* index = dynamic_cast<Value<Int>*>(second);

			int i = 0;
			if (*index->valuePtr == -1)
				i = list->valuePtr->size() - 1;

			if (i < 0 || i >= list->valuePtr->size())
			{
				second->token.sourceCodePtr->PrintError(second->token, "index out of range");
				return;
			}

			Data* item = list->valuePtr->at(i);
			delete item;
			list->valuePtr->erase(list->valuePtr->begin() + i);
		}
		else if(first->type == DataType::Map)
		{
			if (!second->AffirmSameType(DataType::String))
				return;

			Value<Map>* map = dynamic_cast<Value<Map>*>(first);
			Value<String>* key = dynamic_cast<Value<String>*>(second);

			std::string& k = *key->valuePtr;

			if (map->valuePtr->count(k) == 0)
			{
				second->token.sourceCodePtr->PrintError(second->token, "key not found");
				return;
			}

			Data* item = map->valuePtr->at(k);
			delete item;
			map->valuePtr->erase(k);
		}
		else if (first->type == DataType::String)
		{
			if (first->isConst)
			{
				first->token.sourceCodePtr->PrintError(first->token, "cannot change literal string");
				return;
			}

			if (!second->AffirmSameType(DataType::Int))
				return;

			Value<String>* str = dynamic_cast<Value<String>*>(first);
			Value<Int>* index = dynamic_cast<Value<Int>*>(second);

			int i = 0;
			if (*index->valuePtr == -1)
				i = str->valuePtr->size() - 1;
			else
				i = *index->valuePtr;

			if (i < 0 || i >= str->valuePtr->size())
			{
				second->token.sourceCodePtr->PrintError(second->token, "index out of range");
				return;
			}

			str->valuePtr->erase(i, 1);
		}
		else
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected list or map");
		}
	}

	static void F_DefineFunction(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type != DataType::String)
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected variable name (string)");
			return;
		}

		Value<String>* name = dynamic_cast<Value<String>*>(first);
		Data* function = self->arguments.back();
		
		if (!function->AffirmSameType(DataType::Function))
		{
			function->token.sourceCodePtr->PrintError(first->token, "expected function");
			return;
		}

		Value<Function>* function_ref = new Value<Function>(DataType::Function, false, function->token);
		function_ref->ReferenceOther(function);

		if (!self->parent->AddVariable(*name->valuePtr, function_ref))
		{
			first->token.sourceCodePtr->PrintError(first->token, "name is already defined");
			return;
		}
		
		for (int i = 1; i < self->arguments.size()-1; i++)
		{
			Data* param = self->arguments[i]->Evaluate();
			AFFIRM_DATA(param)

			if (!param->AffirmSameType(DataType::String))
			{
				param->token.sourceCodePtr->PrintError(param->token, "expected parameter name");
				return;
			}

			Value<String>* param_str = dynamic_cast<Value<String>*>(param);

			function_ref->valuePtr->parameterNames.push_back(*param_str->valuePtr);
		}
	}

	static void F_GetVariable(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type != DataType::String)
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected variable name (string)");
			return;
		}

		Value<String>* name = dynamic_cast<Value<String>*>(first);
		Data* var = nullptr;

		if (!self->GetVariable(*name->valuePtr, var))
		{
			first->token.sourceCodePtr->PrintError(first->token, "variable is not defined");
			return;
		}

		if (var->type == DataType::Function)
		{
			Value<Function>* func = dynamic_cast<Value<Function>*>(var);
			for (int i = 0; i+1 < self->arguments.size() && i < func->valuePtr->parameterNames.size(); i++)
			{
				Data* arg = self->arguments[i+1]->Evaluate();
				AFFIRM_DATA(arg)

				Data* argRef = nullptr;
				arg->CreateSameType(argRef);
				argRef->ReferenceOther(arg);
				func->valuePtr->AddVariable(func->valuePtr->parameterNames[i], argRef);
			}
		}

		Data* res = var->Evaluate();
		if (res == nullptr)
			return;

		res->CreateSameType(self->returnValue);
		self->returnValue->ReferenceOther(res);
	}

	static void F_GetFunctionReference(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type != DataType::String)
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected function name (string)");
			return;
		}

		Value<String>* name = dynamic_cast<Value<String>*>(first);
		Data* var = nullptr;

		if (!self->GetVariable(*name->valuePtr, var))
		{
			first->token.sourceCodePtr->PrintError(first->token, "function is not defined");
			return;
		}

		if (var->type != DataType::Function)
		{
			first->token.sourceCodePtr->PrintError(first->token, "the variable is not of type function");
			return;
		}

		Value<Function>* func = dynamic_cast<Value<Function>*>(var);
		self->returnValue = new Value<Function>(DataType::Function, false, func->token);
		self->returnValue->ReferenceOther(func);
	}

	static void F_FunctionReference(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* last = self->arguments.back();
		if (last->type != DataType::Function)
		{
			last->token.sourceCodePtr->PrintError(last->token, "the argument is not of type function");
			return;
		}

		Value<Function>* func = dynamic_cast<Value<Function>*>(last);
		Value<Function>* func_ref = new Value<Function>(DataType::Function, false, func->token);
		func_ref->ReferenceOther(func);
		self->returnValue = func_ref;

		for (int i = 0; i < self->arguments.size()-1; i++)
		{
			Data* param = self->arguments[i]->Evaluate();
			AFFIRM_DATA(param)

			if (!param->AffirmSameType(DataType::String))
			{
				param->token.sourceCodePtr->PrintError(param->token, "expected parameter name");
				return;
			}

			Value<String>* param_str = dynamic_cast<Value<String>*>(param);
			func_ref->valuePtr->parameterNames.push_back(*param_str->valuePtr);
		}
	}

	static void F_EvaluateFunction(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type != DataType::Function)
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected a function");
			return;
		}

		Value<Function>* func = dynamic_cast<Value<Function>*>(first);
		for (int i = 0; i + 1 < self->arguments.size() && i < func->valuePtr->parameterNames.size(); i++)
		{
			Data* arg = self->arguments[i + 1]->Evaluate();
			AFFIRM_DATA(arg)

			Data* argRef = nullptr;
			arg->CreateSameType(argRef);
			argRef->ReferenceOther(arg);
			func->valuePtr->AddVariable(func->valuePtr->parameterNames[i], argRef);
		}

		Data* res = first->Evaluate();
		if (res == nullptr)
			return;

		res->CreateSameType(self->returnValue);
		self->returnValue->ReferenceOther(res);
	}

	static void F_If(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		Data* condition = self->arguments[0]->Evaluate();
		AFFIRM_DATA(condition)

		if (condition->type != DataType::Bool)
		{
			condition->token.sourceCodePtr->PrintError(condition->token, "expected boolean");
			return;
		}

		Value<Bool>* val = dynamic_cast<Value<Bool>*>(condition);

		if (*val->valuePtr)
		{
			self->arguments[1]->Evaluate();
		}
		else if (self->arguments.size() == 3)
		{
			self->arguments[2]->Evaluate();
		}

		if (self->returnValue != nullptr && self->parent != nullptr)
		{
			self->returnValue->CreateSameType(self->parent->returnValue);
			self->parent->returnValue->ReferenceOther(self->returnValue);
			return;
		}
	}

	static void F_While(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		Data* condition = self->arguments[0]->Evaluate();
		AFFIRM_DATA(condition)

		if (condition->type != DataType::Bool)
		{
			condition->token.sourceCodePtr->PrintError(condition->token, "expected boolean");
			return;
		}

		Value<Bool>* val = dynamic_cast<Value<Bool>*>(condition);

		while (*val->valuePtr)
		{
			self->arguments[1]->Evaluate();

			if (self->returnValue != nullptr && self->parent != nullptr)
			{
				self->returnValue->CreateSameType(self->parent->returnValue);
				self->parent->returnValue->ReferenceOther(self->returnValue);
				return;
			}

			condition = self->arguments[0]->Evaluate();
			AFFIRM_DATA(condition)

			if (condition->type != DataType::Bool)
			{
				condition->token.sourceCodePtr->PrintError(condition->token, "expected boolean");
				return;
			}

			val = dynamic_cast<Value<Bool>*>(condition);
		}
	}

	static void F_ToString(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* data = self->arguments[0]->Evaluate();
		AFFIRM_DATA(data)

		std::string str;
		if (data->type == DataType::Bool)
		{
			Value<Bool>* b = dynamic_cast<Value<Bool>*>(data);
			str = *b->valuePtr ? "true" : "false";
		}
		else if (data->type == DataType::Int)
		{
			Value<Int>* i = dynamic_cast<Value<Int>*>(data);
			str = std::to_string(*i->valuePtr);
		}
		else if (data->type == DataType::Float)
		{
			Value<Float>* f = dynamic_cast<Value<Float>*>(data);
			str = std::to_string(*f->valuePtr);
		}
		else
		{
			data->token.sourceCodePtr->PrintError(data->token, "expected bool, int or float");
			return;
		}

		Value<String>* str_val = new Value<String>(DataType::String, false, data->token);
		str_val->SetValue(str);
		self->returnValue = str_val;
	}

	static bool Helper_IsInt(const std::string& str)
	{
		for (int i = 0; i < str.size(); i++)
		{
			char c = str[i];
			if ((c == '-' && i != 0) || c < '0' || c > '9')
				return false;
		}
		return true;
	}

	static bool Helper_IsFloat(const std::string& str)
	{
		bool hasDot = false;
		for (int i = 0; i < str.size(); i++)
		{
			char c = str[i];
			bool isDot = c == '.';
			if ((c == '-' && i != 0) || (!isDot && (c < '0' || c > '9')) || (hasDot && isDot))
				return false;

			hasDot |= isDot;
		}
		return true;
	}

	static void F_ToInt(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* data = self->arguments[0]->Evaluate();
		AFFIRM_DATA(data)

		int i;
		if (data->type == DataType::String)
		{
			Value<String>* s = dynamic_cast<Value<String>*>(data);
			if (!Helper_IsInt(*s->valuePtr))
			{
				data->token.sourceCodePtr->PrintError(data->token, "failed to convert string into int");
				return;
			}
			i = std::stoi(*s->valuePtr);
		}
		else if (data->type == DataType::Float)
		{
			Value<Float>* f = dynamic_cast<Value<Float>*>(data);
			i = (int)*f->valuePtr;
		}
		else
		{
			data->token.sourceCodePtr->PrintError(data->token, "expected string or float");
			return;
		}

		Value<Int>* int_val = new Value<Int>(DataType::Int, false, data->token);
		int_val->SetValue(i);
		self->returnValue = int_val;
	}

	static void F_ToFloat(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* data = self->arguments[0]->Evaluate();
		AFFIRM_DATA(data)

		float f;
		if (data->type == DataType::String)
		{
			Value<String>* s = dynamic_cast<Value<String>*>(data);
			if (!Helper_IsFloat(*s->valuePtr))
			{
				data->token.sourceCodePtr->PrintError(data->token, "failed to convert string into float");
				return;
			}
			f = std::stof(*s->valuePtr);
		}
		else if (data->type == DataType::Int)
		{
			Value<Int>* i = dynamic_cast<Value<Int>*>(data);
			f = (float)*i->valuePtr;
		}
		else
		{
			data->token.sourceCodePtr->PrintError(data->token, "expected string or int");
			return;
		}

		Value<Float>* float_val = new Value<Float>(DataType::Float, false, data->token);
		float_val->SetValue(f);
		self->returnValue = float_val;
	}

	static void F_Add(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		DataType t;
		Data* left = self->arguments[0]->Evaluate();
		AFFIRM_DATA(left)
		Data* right = self->arguments[1]->Evaluate();
		AFFIRM_DATA(right)

		if ((t = left->type) != right->type || (t != DataType::Int && t != DataType::Float && t != DataType::String))
		{
			left->token.sourceCodePtr->PrintError(left->token, "type mismatch");
			return;
		}

		if (t == DataType::Float)
		{
			Value<Float>* f_left = dynamic_cast<Value<Float>*>(left);
			Value<Float>* f_right = dynamic_cast<Value<Float>*>(right);

			Value<Float>* sum = new Value<Float>(DataType::Float, false, f_left->token);
			sum->SetValue(*f_left->valuePtr + *f_right->valuePtr);
			self->returnValue = sum;
		}
		else if (t == DataType::Int)
		{
			Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
			Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

			Value<Int>* sum = new Value<Int>(DataType::Int, false, i_left->token);
			sum->SetValue(*i_left->valuePtr + *i_right->valuePtr);
			self->returnValue = sum;
		}
		else if (t == DataType::String)
		{
			Value<String>* s_left = dynamic_cast<Value<String>*>(left);
			Value<String>* s_right = dynamic_cast<Value<String>*>(right);

			Value<String>* sum = new Value<String>(DataType::String, false, s_left->token);
			sum->SetValue(*s_left->valuePtr + *s_right->valuePtr);
			self->returnValue = sum;
		}
	}

	static void F_Sub(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		DataType t;
		Data* left = self->arguments[0]->Evaluate();
		AFFIRM_DATA(left)
		Data* right = self->arguments[1]->Evaluate();
		AFFIRM_DATA(right)

		if ((t = left->type) != right->type || (t != DataType::Int && t != DataType::Float))
		{
			left->token.sourceCodePtr->PrintError(left->token, "type mismatch");
			return;
		}

		if (t == DataType::Float)
		{
			Value<Float>* f_left = dynamic_cast<Value<Float>*>(left);
			Value<Float>* f_right = dynamic_cast<Value<Float>*>(right);

			Value<Float>* dif = new Value<Float>(DataType::Float, false, f_left->token);
			dif->SetValue(*f_left->valuePtr - *f_right->valuePtr);
			self->returnValue = dif;
		}
		else if (t == DataType::Int)
		{
			Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
			Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

			Value<Int>* dif = new Value<Int>(DataType::Int, false, i_left->token);
			dif->SetValue(*i_left->valuePtr - *i_right->valuePtr);
			self->returnValue = dif;
		}
	}

	static void F_Mult(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		DataType t;
		Data* left = self->arguments[0]->Evaluate();
		AFFIRM_DATA(left)
		Data* right = self->arguments[1]->Evaluate();
		AFFIRM_DATA(right)

		if ((t = left->type) != right->type || (t != DataType::Int && t != DataType::Float))
		{
			left->token.sourceCodePtr->PrintError(left->token, "type mismatch");
			return;
		}

		if (t == DataType::Float)
		{
			Value<Float>* f_left = dynamic_cast<Value<Float>*>(left);
			Value<Float>* f_right = dynamic_cast<Value<Float>*>(right);

			Value<Float>* prod = new Value<Float>(DataType::Float, false, f_left->token);
			prod->SetValue(*f_left->valuePtr * *f_right->valuePtr);
			self->returnValue = prod;
		}
		else if (t == DataType::Int)
		{
			Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
			Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

			Value<Int>* prod = new Value<Int>(DataType::Int, false, i_left->token);
			prod->SetValue(*i_left->valuePtr * *i_right->valuePtr);
			self->returnValue = prod;
		}
	}

	static void F_Div(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		DataType t;
		Data* left = self->arguments[0]->Evaluate();
		AFFIRM_DATA(left)
		Data* right = self->arguments[1]->Evaluate();
		AFFIRM_DATA(right)

		if ((t = left->type) != right->type || (t != DataType::Int && t != DataType::Float))
		{
			left->token.sourceCodePtr->PrintError(left->token, "type mismatch");
			return;
		}

		if (t == DataType::Float)
		{
			Value<Float>* f_left = dynamic_cast<Value<Float>*>(left);
			Value<Float>* f_right = dynamic_cast<Value<Float>*>(right);

			Value<Float>* quota = new Value<Float>(DataType::Float, false, f_left->token);
			quota->SetValue(*f_left->valuePtr / *f_right->valuePtr);
			self->returnValue = quota;
		}
		else if (t == DataType::Int)
		{
			Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
			Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

			Value<Int>* quota = new Value<Int>(DataType::Int, false, i_left->token);
			quota->SetValue(*i_left->valuePtr / *i_right->valuePtr);
			self->returnValue = quota;
		}
	}

	static void F_Less(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		DataType t;
		Data* left = self->arguments[0]->Evaluate();
		AFFIRM_DATA(left)
		Data* right = self->arguments[1]->Evaluate();
		AFFIRM_DATA(right)

		if ((t = left->type) != right->type || (t != DataType::Int && t != DataType::Float))
		{
			left->token.sourceCodePtr->PrintError(left->token, "type mismatch");
			return;
		}

		if (t == DataType::Float)
		{
			Value<Float>* f_left = dynamic_cast<Value<Float>*>(left);
			Value<Float>* f_right = dynamic_cast<Value<Float>*>(right);

			Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, f_left->token);
			comp->SetValue(*f_left->valuePtr < *f_right->valuePtr);
			self->returnValue = comp;
		}
		else if (t == DataType::Int)
		{
			Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
			Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

			Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, i_left->token);
			comp->SetValue(*i_left->valuePtr < *i_right->valuePtr);
			self->returnValue = comp;
		}
	}

	static void F_Equal(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		DataType t;
		Data* left = self->arguments[0]->Evaluate();
		AFFIRM_DATA(left)
		Data* right = self->arguments[1]->Evaluate();
		AFFIRM_DATA(right)

		if ((t = left->type) != right->type || (t != DataType::Int && t != DataType::Float && t != DataType::String))
		{
			left->token.sourceCodePtr->PrintError(left->token, "type mismatch");
			return;
		}

		if (t == DataType::Float)
		{
			Value<Float>* f_left = dynamic_cast<Value<Float>*>(left);
			Value<Float>* f_right = dynamic_cast<Value<Float>*>(right);

			Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, f_left->token);
			comp->SetValue(*f_left->valuePtr == *f_right->valuePtr);
			self->returnValue = comp;
		}
		else if (t == DataType::Int)
		{
			Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
			Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

			Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, i_left->token);
			comp->SetValue(*i_left->valuePtr == *i_right->valuePtr);
			self->returnValue = comp;
		}
		else if (t == DataType::String)
		{
			Value<String>* s_left = dynamic_cast<Value<String>*>(left);
			Value<String>* s_right = dynamic_cast<Value<String>*>(right);

			Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, s_left->token);
			comp->SetValue(*s_left->valuePtr == *s_right->valuePtr);
			self->returnValue = comp;
		}
	}

	static void F_And(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		DataType t;
		Data* left = self->arguments[0]->Evaluate();
		AFFIRM_DATA(left)
		Data* right = self->arguments[1]->Evaluate();
		AFFIRM_DATA(right)

		if ((t = left->type) != right->type || t != DataType::Bool)
		{
			left->token.sourceCodePtr->PrintError(left->token, "expected bool");
			return;
		}

		Value<Bool>* b_left = dynamic_cast<Value<Bool>*>(left);
		Value<Bool>* b_right = dynamic_cast<Value<Bool>*>(right);

		Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, b_left->token);
		comp->SetValue(*b_left->valuePtr && *b_right->valuePtr);
		self->returnValue = comp;
	}

	static void F_Or(Function* self)
	{
		if (!self->CheckArgumens(2))
			return;

		DataType t;
		Data* left = self->arguments[0]->Evaluate();
		AFFIRM_DATA(left)
		Data* right = self->arguments[1]->Evaluate();
		AFFIRM_DATA(right)

		if ((t = left->type) != right->type || t != DataType::Bool)
		{
			left->token.sourceCodePtr->PrintError(left->token, "expected bool");
			return;
		}

		Value<Bool>* b_left = dynamic_cast<Value<Bool>*>(left);
		Value<Bool>* b_right = dynamic_cast<Value<Bool>*>(right);

		Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, b_left->token);
		comp->SetValue(*b_left->valuePtr || *b_right->valuePtr);
		self->returnValue = comp;
	}

	static void F_Not(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type != DataType::Bool)
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected bool");
			return;
		}

		Value<Bool>* b = dynamic_cast<Value<Bool>*>(first);

		Value<Bool>* b_not = new Value<Bool>(DataType::Bool, false, b->token);
		b_not->SetValue(!*b->valuePtr);
		self->returnValue = b_not;
	}

	static void F_Count(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type == DataType::List)
		{
			Value<List>* list = dynamic_cast<Value<List>*>(first);
			Value<Int>* count = new Value<Int>(DataType::Int, false, first->token);
			count->SetValue((int)list->valuePtr->size());
			self->returnValue = count;
		}
		else if (first->type == DataType::String)
		{
			Value<String>* str = dynamic_cast<Value<String>*>(first);
			Value<Int>* count = new Value<Int>(DataType::Int, false, first->token);
			count->SetValue((int)str->valuePtr->size());
			self->returnValue = count;
		}
		else
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected list or string");
		}
	}

	static void F_Keys(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type != DataType::Map)
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected map");
			return;
		}

		Value<Map>* map = dynamic_cast<Value<Map>*>(first);
		Value<List>* list = new Value<List>(DataType::List, false, first->token);
		list->Init();

		for (auto& e : map->valuePtr->map)
		{
			Value<String>* key = new Value<String>(DataType::String, false, first->token);
			key->SetValue(e.first);
			list->valuePtr->push_back(key);
		}

		self->returnValue = list;
	}

	static void F_CallCPPFunction(Function* self)
	{
		if (!self->CheckArgumens(1))
			return;

		Data* first = self->arguments[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->type != DataType::String)
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected function name (string)");
			return;
		}

		Value<String>* name = dynamic_cast<Value<String>*>(first);
		if (scriptFunctions.count(*name->valuePtr) == 0)
		{
			first->token.sourceCodePtr->PrintError(first->token, "function not defined");
			return;
		}

		List args;
		for (int i = 1; i < self->arguments.size(); i++)
		{
			Data* argRef = nullptr;
			Data* arg = self->arguments[i]->Evaluate();
			AFFIRM_DATA(arg)

			arg->CreateSameType(argRef);
			argRef->ReferenceOther(arg);
			args.push_back(argRef);
		}

		scriptFunctions[*name->valuePtr](args);
	}

	std::unordered_map<std::string, void(*)(Function*)> functions
	{
		{"do", F_Do},
		{"function", F_Function},
		{"print", F_Print},
		{"return_copy", F_ReturnCopy},
		{"return_ref", F_ReturnReference},
		{"set_copy", F_SetCopy},
		{"set_ref", F_SetReference},
		{"push_copy", F_AddElementsAsCopies},
		{"push_ref", F_AddElementsAsReferences},
		{"get_elem", F_GetElement},
		{"rem_elem", F_RemoveElement},
		{"def", F_DefineFunction},
		{"get", F_GetVariable},
		{"ref_func", F_GetFunctionReference},
		{"lambda", F_FunctionReference},
		{"eval", F_EvaluateFunction},
		{"if", F_If},
		{"while", F_While},
		{"add", F_Add},
		{"sub", F_Sub},
		{"mult", F_Mult},
		{"div", F_Div},
		{"to_string", F_ToString},
		{"to_int", F_ToInt},
		{"to_float", F_ToFloat},
		{"less", F_Less},
		{"and", F_And},
		{"or", F_Or},
		{"not", F_Not},
		{"equal", F_Equal},
		{"count", F_Count},
		{"keys", F_Keys},
		{"call_cpp", F_CallCPPFunction}
	};
};

struct Script
{
	SourceCode sourceCode;
	FunctionLibrary functionLibrary;
	Value<Function>* rootFunction;

	Script()
	{
		rootFunction = nullptr;
	}

	bool IsDigit(char c)
	{
		return c >= '0' && c <= '9';
	}

	bool IsWhitespace(char c)
	{
		return c == ' ' || c == '\n' || c == '\t';
	}

	bool IsDelimiter(char c)
	{
		return IsWhitespace(c) || c == '(' || c == ')';
	}

	bool ApplyMacros()
	{
		while (true)
		{
			if (sourceCode.BeginsWith("#macro"))
			{
				size_t macroStartIndex = sourceCode.index;
				sourceCode.MoveAlong(6);

				std::string regexStr;
				bool goNext = true;
				char c;
				for (; (goNext = sourceCode.NextChar()) && !IsWhitespace(c = sourceCode.CurrentChar());)
					regexStr += c;

				if (!goNext)
				{
					sourceCode.PrintErrorAtCurrentIndex("incomplete macro");
					return false;
				}

				std::string expansionStr;
				goNext = true;
				for (; (goNext = sourceCode.NextChar()) && !IsWhitespace(c = sourceCode.CurrentChar());)
					expansionStr += c;

				if (!goNext)
				{
					sourceCode.PrintErrorAtCurrentIndex("incomplete macro");
					return false;
				}

				std::regex rgx(regexStr);

				std::string restOfText = sourceCode.Substring(sourceCode.index, sourceCode.text.size()-1);

				std::string result = std::regex_replace(restOfText, rgx, expansionStr, std::regex_constants::format_default);
				sourceCode.text.resize(macroStartIndex);
				sourceCode.text.append(result.begin(), result.end());
				sourceCode.index = macroStartIndex;
			}


			if (!sourceCode.NextChar())
				break;
		}


		sourceCode.Reset();
		return true;
	}

	bool RecursiveParse(Data*& outData, int maxIndex = -1)
	{
		Token unknownToken;
		std::string unknown;
		int lastUnknownCharIndex = 0;

		while (maxIndex == -1 || sourceCode.index <= maxIndex)
		{
			char c = sourceCode.CurrentChar();
			if (IsWhitespace(c));
			else if (sourceCode.BeginsWith("/*"))
			{
				for (; sourceCode.NextChar() && !sourceCode.BeginsWith("*/"););
				sourceCode.MoveAlong(2);
			}
			else if (sourceCode.BeginsWith("//"))
			{
				for (; sourceCode.NextChar() && sourceCode.CurrentChar() != '\n';);
				sourceCode.NextChar();
			}
			else if (c == '"')
			{
				int first = sourceCode.index;
				for (; sourceCode.NextChar() && sourceCode.CurrentChar() != '"';);
				int last = sourceCode.index;

				Value<String>* val = new Value<String>(DataType::String, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
				val->SetValue(sourceCode.Substring(first + 1, last - 1));
				sourceCode.NextChar();

				outData = val;
				return true;
			}
			else if (c == '-' || IsDigit(c))
			{
				bool isFloat = false;
				std::string num;
				num += c;
				for (; sourceCode.NextChar(); num += c)
				{
					c = sourceCode.CurrentChar();
					if (c == '.')
					{
						if (isFloat)
						{
							sourceCode.PrintErrorAtCurrentIndex("unexpected '.'");
							return false;
						}
						isFloat = true;
					}
					else if (!IsDigit(c))
					{
						if (!IsDelimiter(c))
						{
							std::string err("unexpected '");
							err += c;
							err += "'";
							sourceCode.PrintErrorAtCurrentIndex(err);
							return false;
						}
						break;
					}
				}

				if (isFloat)
				{
					Value<Float>* val = new Value<Float>(DataType::Float, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
					val->SetValue(std::stof(num));
					outData = val;
					return true;
				}
				else
				{
					Value<Int>* val = new Value<Int>(DataType::Int, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
					val->SetValue(std::stoi(num));
					outData = val;
					return true;
				}
			}
			else if (sourceCode.BeginsWith("true"))
			{
				Value<Bool>* val = new Value<Bool>(DataType::Bool, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
				val->SetValue(true);
				sourceCode.MoveAlong(4);
				outData = val;
				return true;
			}
			else if (sourceCode.BeginsWith("false"))
			{
				Value<Bool>* val = new Value<Bool>(DataType::Bool, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
				val->SetValue(false);
				sourceCode.MoveAlong(5);
				outData = val;
				return true;
			}
			else if (sourceCode.BeginsWith("list"))
			{
				Value<List>* val = new Value<List>(DataType::List, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
				val->SetValue({});
				sourceCode.MoveAlong(4);
				outData = val;
				return true;
			}
			else if (sourceCode.BeginsWith("map"))
			{
				Value<Map>* val = new Value<Map>(DataType::Map, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
				val->SetValue({});
				sourceCode.MoveAlong(3);
				outData = val;
				return true;
			}
			else if (unknown.size() > 0 && IsDelimiter(c))
			{
				if (functionLibrary.functions.count(unknown) == 0)
				{
					sourceCode.PrintError(unknownToken, "undefined '" + unknown + "'");
					return false;
				}

				Value<Function>* val = new Value<Function>(DataType::Function, true, unknownToken);
				val->SetValue(functionLibrary.functions[unknown]);

				for (; c != '(' && sourceCode.NextChar() && IsWhitespace(c = sourceCode.CurrentChar()););
				if (c != '(')
				{
					std::string err = "unexpected '";
					err += c;
					err += "'";
					sourceCode.PrintErrorAtCurrentIndex(err);
					return false;
				}

				int opened = 0;
				int closingIndex = sourceCode.index;
				for (int i=sourceCode.index; i<sourceCode.text.size(); i++, closingIndex++)
				{
					if ((c = sourceCode.text[i]) == '(')
						opened++;
					else if (c == ')' && --opened == 0)
						break;
				}
				if (opened == 1)
				{
					sourceCode.PrintErrorAtCurrentIndex("expected ')' missing");
					return false;
				}
				
				if (sourceCode.index + 1 < closingIndex)
				{
					Data* arg = nullptr;
					sourceCode.NextChar();
					for (; RecursiveParse(arg, closingIndex-1) && arg != nullptr;)
					{
						val->valuePtr->AddArgument(arg);
						arg = nullptr;
					}
				}
				
				sourceCode.NextChar();

				outData = val;
				return true;
			}
			else
			{
				if (unknown.size() == 0)
					unknownToken = { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode };
				else if (sourceCode.index > lastUnknownCharIndex+1)
				{
					sourceCode.PrintError(unknownToken, "unexpected " + unknown);
					return false;
				}

				lastUnknownCharIndex = sourceCode.index;
				unknown += c;
			}


			if (!sourceCode.NextChar())
				break;
		}

		return true;
	}

	bool LoadScript(const std::string& path)
	{
		if (!sourceCode.ReadFile(path))
			return false;

		if (!ApplyMacros())
			return false;

		Data* res = nullptr;
		if (!RecursiveParse(res) || !res->AffirmSameType(DataType::Function))
			return false;

		rootFunction = dynamic_cast<Value<Function>*>(res);
		return true;
	}

	void Run()
	{
		rootFunction->valuePtr->Call();
	}
};

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cout << "\n[ERROR] incorrect arguments sent to program, expected script path" << std::endl;
		return 1;
	}

	SCRIPT_FUNCTION("run", {
		if (args.size() != 1)
			return;

		Data* first = args[0]->Evaluate();
		AFFIRM_DATA(first)

		if (first->AffirmSameType(DataType::String))
		{
			std::string& path = *dynamic_cast<Value<String>*>(first)->valuePtr;
			Script script;
			if (script.LoadScript(path))
			{
				script.Run();
			}
		}
	})

	Script s;
	if (s.LoadScript(argv[1]))
	{
		s.Run();
	}

	return 0;
}