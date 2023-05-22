#pragma once
#include "data.h"
#include <vector>
#include <string>
#include <unordered_map>

void FreeData(Data* data);

struct List
{
	std::vector<Data*> list;

	List();

	List(const List& other);

	List& operator=(const List& other);

	~List();

	void push_back(Data* data);

	int size() const;

	Data*& operator[](int i);

	Data*& at(int i);

	void erase(const std::vector<Data*>::const_iterator& itr);

	std::vector<Data*>::const_iterator begin();
};

struct Map
{
	std::unordered_map<std::string, Data*> map;

	Map();

	Map(const Map& other);

	Map& operator=(const Map& other);

	~Map();

	void insert(const std::pair<std::string, Data*>& pair);

	int count(const std::string& key);

	Data*& operator[](const std::string& key);

	Data*& at(const std::string& key);

	void erase(const std::string& key);
};

struct Function
{
	Function* parent;
	Data* returnValue;
	std::vector<Data*> arguments;
	void (*function)(Function*);
	std::unordered_map<std::string, Data*> variables;
	std::vector<std::string> parameterNames;

	Function();

	Function(void(*_function)(Function*));

	Function(const Function& other);

	Function& operator=(const Function& other);

	~Function();

	bool GetVariableOwner(const std::string& name, Function*& outOwner);

	bool GetVariable(const std::string& name, Data*& outVar);

	bool AddVariable(const std::string& name, Data* var);

	void AddArgument(Data* data);

	void Call();

	bool CheckArgumens(int count);
};