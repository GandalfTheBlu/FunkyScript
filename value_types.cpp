#include "value_types.h"
#include "value.h"
#include "memory_pool.h"

void FreeData(Data* data)
{
	if (data == nullptr)
		return;

	switch (data->type)
	{
	case DataType::Bool:
		Memory<Value<Bool>>().Delete(dynamic_cast<Value<Bool>*>(data));
		break;
	case DataType::Int:
		Memory<Value<Int>>().Delete(dynamic_cast<Value<Int>*>(data));
		break;
	case DataType::Float:
		Memory<Value<Float>>().Delete(dynamic_cast<Value<Float>*>(data));
		break;
	case DataType::String:
		Memory<Value<String>>().Delete(dynamic_cast<Value<String>*>(data));
		break;
	case DataType::List:
		Memory<Value<List>>().Delete(dynamic_cast<Value<List>*>(data));
		break;
	case DataType::Map:
		Memory<Value<Map>>().Delete(dynamic_cast<Value<Map>*>(data));
		break;
	case DataType::Function:
		Memory<Value<Function>>().Delete(dynamic_cast<Value<Function>*>(data));
		break;
	}
}

List::List(){}

List::List(const List& other)
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

List& List::operator=(const List& other)
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

List::~List()
{
	for (auto& e : list)
		FreeData(e);//delete e;
}

void List::push_back(Data* data)
{
	list.push_back(data);
}

int List::size() const
{
	return (int)list.size();
}

Data*& List::operator[](int i)
{
	return list[i];
}

Data*& List::at(int i)
{
	return list.at(i);
}

void List::erase(const std::vector<Data*>::const_iterator& itr)
{
	list.erase(itr);
}

std::vector<Data*>::const_iterator List::begin()
{
	return list.begin();
}

Map::Map() {}

Map::Map(const Map& other)
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

Map& Map::operator=(const Map& other)
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

Map::~Map()
{
	for (auto& e : map)
		FreeData(e.second);//delete e.second;
}

void Map::insert(const std::pair<std::string, Data*>& pair)
{
	map.insert(pair);
}

int Map::count(const std::string& key)
{
	return map.count(key);
}

Data*& Map::operator[](const std::string& key)
{
	return map[key];
}

Data*& Map::at(const std::string& key)
{
	return map.at(key);
}

void Map::erase(const std::string& key)
{
	map.erase(key);
}

Function::Function()
{
	parent = nullptr;
	returnValue = nullptr;
	function = nullptr;
}

Function::Function(void(*_function)(Function*))
{
	parent = nullptr;
	returnValue = nullptr;
	function = _function;
}

Function::Function(const Function& other)
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

Function& Function::operator=(const Function& other)
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

Function::~Function()
{
	for (auto& arg : arguments)
		FreeData(arg);//delete arg;

	FreeData(returnValue);//delete returnValue;
}

bool Function::GetVariableOwner(const std::string& name, Function*& outOwner)
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

bool Function::GetVariable(const std::string& name, Data*& outVar)
{
	Function* owner = nullptr;
	if (!GetVariableOwner(name, owner))
		return false;

	outVar = owner->variables[name];
	return true;
}

bool Function::AddVariable(const std::string& name, Data* var)
{
	if (variables.count(name) != 0)
		return false;

	variables[name] = var;
}

void Function::AddArgument(Data* data)
{
	if (data->type == DataType::Function)
	{
		dynamic_cast<Value<Function>*>(data)->valuePtr->parent = this;
	}
	arguments.push_back(data);
}

void Function::Call()
{
	FreeData(returnValue);//delete returnValue;
	returnValue = nullptr;

	function(this);

	for (auto& v : variables)
		FreeData(v.second);//delete v.second;

	variables.clear();
}

bool Function::CheckArgumens(int count)
{
	if (arguments.size() < count)
	{
		printf("[ERROR][Function] too few arguments sent to function\n");
		return false;
	}
	return true;
}