#include "script.h"
#include <iostream>
#include <regex>
#include <fstream>
#include <sstream>

#define AFFIRM_DATA(data) if(data == nullptr){return;}

FunctionLibrary::FunctionLibrary()
{
	functions = 
	{
		{"do", F_Do},
		{"function", F_Function},
		{"print", F_Print},
		{"input", F_Input},
		{"return_copy", F_ReturnCopy},
		{"return_ref", F_ReturnReference},
		{"set_copy", F_SetCopy},
		{"set_ref", F_SetReference},
		{"push_copy", F_AddElementsAsCopies},
		{"push_ref", F_AddElementsAsReferences},
		{"get_elem", F_GetElement},
		{"rem_elem", F_RemoveElement},
		{"has_key", F_HasKey},
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
		{"type_of", F_TypeOf},
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
}

void FunctionLibrary::F_Do(Function* self)
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

void FunctionLibrary::F_Function(Function* self)
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

void FunctionLibrary::Helper_PrintBool(Data* data)
{
	std::cout << std::boolalpha << *dynamic_cast<Value<Bool>*>(data)->valuePtr;
}

void FunctionLibrary::Helper_PrintInt(Data* data)
{
	std::cout << *dynamic_cast<Value<Int>*>(data)->valuePtr;
}

void FunctionLibrary::Helper_PrintFloat(Data* data)
{
	std::cout << *dynamic_cast<Value<Float>*>(data)->valuePtr;
}

void FunctionLibrary::Helper_PrintString(Data* data)
{
	std::cout << *dynamic_cast<Value<String>*>(data)->valuePtr;
}

void FunctionLibrary::Helper_PrintList(Data* data)
{
	List& l = *dynamic_cast<Value<List>*>(data)->valuePtr;
	std::cout << "[";
	for (int i = 0; i < l.size(); i++)
	{
		if (i > 0)
			std::cout << ", ";

		Helper_PrintAny(l[i]);
	}
	std::cout << "]";
}

void FunctionLibrary::Helper_PrintMap(Data* data)
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

void FunctionLibrary::Helper_PrintFunction(Data* data)
{
	std::cout << "function()";
}

void FunctionLibrary::Helper_PrintAny(Data* d)
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

void FunctionLibrary::F_Print(Function* self)
{
	for (auto& arg : self->arguments)
	{
		Data* res = arg->Evaluate();
		AFFIRM_DATA(res)

			Helper_PrintAny(res);
	}
}

void FunctionLibrary::F_Input(Function* self)
{
	for (auto& arg : self->arguments)
	{
		Data* data = arg->Evaluate();
		AFFIRM_DATA(data)

			if (data->type == DataType::Bool)
			{
				Value<Bool>* boolVal = dynamic_cast<Value<Bool>*>(data);
				std::cin >> *boolVal->valuePtr;
			}
			else if (data->type == DataType::Int)
			{
				Value<Int>* intVal = dynamic_cast<Value<Int>*>(data);
				std::cin >> *intVal->valuePtr;
			}
			else if (data->type == DataType::Float)
			{
				Value<Float>* floatVal = dynamic_cast<Value<Float>*>(data);
				std::cin >> *floatVal->valuePtr;
			}
			else if (data->type == DataType::String)
			{
				Value<String>* strVal = dynamic_cast<Value<String>*>(data);
				std::cin >> *strVal->valuePtr;
			}
			else
			{
				data->token.sourceCodePtr->PrintError(data->token, "expected bool, int, float or string");
			}
	}
}

void FunctionLibrary::F_ReturnCopy(Function* self)
{
	if (!self->CheckArgumens(1))
		return;

	Data* ret = self->arguments[0]->Evaluate();
	AFFIRM_DATA(ret)

		ret->CreateSameType(self->parent->returnValue);
	self->parent->returnValue->CopyOther(ret);
}

void FunctionLibrary::F_ReturnReference(Function* self)
{
	if (!self->CheckArgumens(1))
		return;

	Data* ret = self->arguments[0]->Evaluate();
	AFFIRM_DATA(ret)

		ret->CreateSameType(self->parent->returnValue);
	self->parent->returnValue->ReferenceOther(ret);
}

void FunctionLibrary::F_SetCopy(Function* self)
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

void FunctionLibrary::F_SetReference(Function* self)
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

void FunctionLibrary::F_AddElementsAsCopies(Function* self)
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
			copy->CopyOther(val);
			map->valuePtr->insert({ *keyStr->valuePtr, copy });
		}
	}
}

void FunctionLibrary::F_AddElementsAsReferences(Function* self)
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

			if (map->valuePtr->count(*keyStr->valuePtr) != 0)
			{
				delete map->valuePtr->at(*keyStr->valuePtr);
			}

			map->valuePtr->at(*keyStr->valuePtr) = copy;
		}
	}
}

void FunctionLibrary::F_GetElement(Function* self)
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
				i = (int)list->valuePtr->size() - 1;
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
		else if (first->type == DataType::Map)
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
				i = (int)str->valuePtr->size() - 1;
			else
				i = *index->valuePtr;

			if (i < 0 || i >= str->valuePtr->size())
			{
				second->token.sourceCodePtr->PrintError(second->token, "index out of range");
				return;
			}

			//Value<String>* val = new Value<String>(DataType::String, false, first->token);
			str->CreateSameType(self->returnValue);

			std::string s;
			s = str->valuePtr->at(i);

			dynamic_cast<Value<String>*>(self->returnValue)->SetValue(s);
		}
		else
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected list or map");
		}
}

void FunctionLibrary::F_RemoveElement(Function* self)
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
				i = (int)list->valuePtr->size() - 1;
			else
				i = *index->valuePtr;

			if (i < 0 || i >= list->valuePtr->size())
			{
				second->token.sourceCodePtr->PrintError(second->token, "index out of range");
				return;
			}

			Data* item = list->valuePtr->at(i);
			//delete item;
			FreeData(item);
			list->valuePtr->erase(list->valuePtr->begin() + i);
		}
		else if (first->type == DataType::Map)
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
			//delete item;
			FreeData(item);
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
				i = (int)str->valuePtr->size() - 1;
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
			first->token.sourceCodePtr->PrintError(first->token, "expected list, map or string");
		}
}

void FunctionLibrary::F_HasKey(Function* self)
{
	if (!self->CheckArgumens(2))
		return;

	Data* first = self->arguments[0]->Evaluate();
	AFFIRM_DATA(first)

		Data* second = self->arguments[1]->Evaluate();
	AFFIRM_DATA(second)

		if (!first->AffirmSameType(DataType::Map) || !second->AffirmSameType(DataType::String))
			return;

	Value<Map>* map = dynamic_cast<Value<Map>*>(first);
	Value<String>* str = dynamic_cast<Value<String>*>(second);
	bool contains = (map->valuePtr->count(*str->valuePtr) != 0);

	Value<Bool>* ret_val = Memory<Value<Bool>>().New(DataType::Bool, false, first->token);//new Value<Bool>(DataType::Bool, false, first->token);
	ret_val->SetValue(contains);
	self->returnValue = ret_val;
}

void FunctionLibrary::F_DefineFunction(Function* self)
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

	//Value<Function>* function_ref = new Value<Function>(DataType::Function, false, function->token);
	Data* function_ref = nullptr;
	function->CreateSameType(function_ref);
	function_ref->ReferenceOther(function);

	if (!self->parent->AddVariable(*name->valuePtr, function_ref))
	{
		first->token.sourceCodePtr->PrintError(first->token, "name is already defined");
		return;
	}

	for (int i = 1; i < self->arguments.size() - 1; i++)
	{
		Data* param = self->arguments[i]->Evaluate();
		AFFIRM_DATA(param)

			if (!param->AffirmSameType(DataType::String))
			{
				param->token.sourceCodePtr->PrintError(param->token, "expected parameter name");
				return;
			}

		Value<String>* param_str = dynamic_cast<Value<String>*>(param);

		dynamic_cast<Value<Function>*>(function_ref)->valuePtr->parameterNames.push_back(*param_str->valuePtr);
	}
}

void FunctionLibrary::F_GetVariable(Function* self)
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
		for (int i = 0; i + 1 < self->arguments.size() && i < func->valuePtr->parameterNames.size(); i++)
		{
			Data* arg = self->arguments[i + 1]->Evaluate();
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

void FunctionLibrary::F_GetFunctionReference(Function* self)
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
	//self->returnValue = new Value<Function>(DataType::Function, false, func->token);
	func->CreateSameType(self->returnValue);
	self->returnValue->ReferenceOther(func);
}

void FunctionLibrary::F_FunctionReference(Function* self)
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
	func->CreateSameType(self->returnValue);
	self->returnValue->ReferenceOther(func);
	//Value<Function>* func_ref = new Value<Function>(DataType::Function, false, func->token);
	//func_ref->ReferenceOther(func);
	//self->returnValue = func_ref;

	for (int i = 0; i < self->arguments.size() - 1; i++)
	{
		Data* param = self->arguments[i]->Evaluate();
		AFFIRM_DATA(param)

			if (!param->AffirmSameType(DataType::String))
			{
				param->token.sourceCodePtr->PrintError(param->token, "expected parameter name");
				return;
			}

		Value<String>* param_str = dynamic_cast<Value<String>*>(param);
		dynamic_cast<Value<Function>*>(self->returnValue)->valuePtr->parameterNames.push_back(*param_str->valuePtr);
	}
}

void FunctionLibrary::F_EvaluateFunction(Function* self)
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

void FunctionLibrary::F_If(Function* self)
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

void FunctionLibrary::F_While(Function* self)
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

void FunctionLibrary::F_TypeOf(Function* self)
{
	if (!self->CheckArgumens(1))
		return;

	Data* data = self->arguments[0]->Evaluate();
	AFFIRM_DATA(data)

		static std::string typeNames[]
	{
		"bool",
		"int",
		"float",
		"string",
		"list",
		"map",
		"function"
	};

	std::string typeName;
	bool customType = false;

	if (data->type == DataType::Map)
	{
		Value<Map>* map = dynamic_cast<Value<Map>*>(data);
		if (map->valuePtr->count("__type__") != 0)
		{
			Data* val = map->valuePtr->at("__type__");
			if (val->type == DataType::String)
			{
				Value<String>* type_str = dynamic_cast<Value<String>*>(val);
				typeName = *type_str->valuePtr;
				customType = true;
			}
		}
	}

	if (!customType)
	{
		int index = (int)data->type;
		typeName = typeNames[index];
	}

	Value<String>* str_val = Memory<Value<String>>().New(DataType::String, false, data->token);//new Value<String>(DataType::String, false, data->token);
	str_val->SetValue(typeName);
	self->returnValue = str_val;
}

void FunctionLibrary::F_ToString(Function* self)
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
	else if (data->type == DataType::String)
	{
		Value<String>* s = dynamic_cast<Value<String>*>(data);
		str = *s->valuePtr;
	}
	else
	{
		data->token.sourceCodePtr->PrintError(data->token, "expected bool, int, float or string");
		return;
	}

	Value<String>* str_val = Memory<Value<String>>().New(DataType::String, false, data->token);//new Value<String>(DataType::String, false, data->token);
	str_val->SetValue(str);
	self->returnValue = str_val;
}

bool FunctionLibrary::Helper_IsInt(const std::string& str)
{
	for (int i = 0; i < str.size(); i++)
	{
		char c = str[i];
		if ((c == '-' && i != 0) || c < '0' || c > '9')
			return false;
	}
	return true;
}

bool FunctionLibrary::Helper_IsFloat(const std::string& str)
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

void FunctionLibrary::F_ToInt(Function* self)
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

	Value<Int>* int_val = Memory<Value<Int>>().New(DataType::Int, false, data->token);//new Value<Int>(DataType::Int, false, data->token);
	int_val->SetValue(i);
	self->returnValue = int_val;
}

void FunctionLibrary::F_ToFloat(Function* self)
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

	Value<Float>* float_val = Memory<Value<Float>>().New(DataType::Float, false, data->token);//new Value<Float>(DataType::Float, false, data->token);
	float_val->SetValue(f);
	self->returnValue = float_val;
}

void FunctionLibrary::F_Add(Function* self)
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

		//Value<Float>* sum = new Value<Float>(DataType::Float, false, f_left->token);
		Data* sum = nullptr;
		f_left->CreateSameType(sum);
		dynamic_cast<Value<Float>*>(sum)->SetValue(*f_left->valuePtr + *f_right->valuePtr);
		self->returnValue = sum;
	}
	else if (t == DataType::Int)
	{
		Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
		Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

		/*Value<Int>* sum = new Value<Int>(DataType::Int, false, i_left->token);
		sum->SetValue(*i_left->valuePtr + *i_right->valuePtr);
		self->returnValue = sum;*/
		Data* sum = nullptr;
		i_left->CreateSameType(sum);
		dynamic_cast<Value<Int>*>(sum)->SetValue(*i_left->valuePtr + *i_right->valuePtr);
		self->returnValue = sum;
	}
	else if (t == DataType::String)
	{
		Value<String>* s_left = dynamic_cast<Value<String>*>(left);
		Value<String>* s_right = dynamic_cast<Value<String>*>(right);

		/*Value<String>* sum = new Value<String>(DataType::String, false, s_left->token);
		sum->SetValue(*s_left->valuePtr + *s_right->valuePtr);
		self->returnValue = sum;*/
		Data* sum = nullptr;
		s_left->CreateSameType(sum);
		dynamic_cast<Value<String>*>(sum)->SetValue(*s_left->valuePtr + *s_right->valuePtr);
		self->returnValue = sum;
	}
}

void FunctionLibrary::F_Sub(Function* self)
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

		Data* diff = nullptr;
		f_left->CreateSameType(diff);
		dynamic_cast<Value<Float>*>(diff)->SetValue(*f_left->valuePtr - *f_right->valuePtr);
		self->returnValue = diff;
	}
	else if (t == DataType::Int)
	{
		Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
		Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

		Data* diff = nullptr;
		i_left->CreateSameType(diff);
		dynamic_cast<Value<Int>*>(diff)->SetValue(*i_left->valuePtr - *i_right->valuePtr);
		self->returnValue = diff;
	}
}

void FunctionLibrary::F_Mult(Function* self)
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

		Data* prod = nullptr;
		f_left->CreateSameType(prod);
		dynamic_cast<Value<Float>*>(prod)->SetValue(*f_left->valuePtr * *f_right->valuePtr);
		self->returnValue = prod;
	}
	else if (t == DataType::Int)
	{
		Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
		Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

		Data* prod = nullptr;
		i_left->CreateSameType(prod);
		dynamic_cast<Value<Int>*>(prod)->SetValue(*i_left->valuePtr * *i_right->valuePtr);
		self->returnValue = prod;
	}
}

void FunctionLibrary::F_Div(Function* self)
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

		Data* quota = nullptr;
		f_left->CreateSameType(quota);
		dynamic_cast<Value<Float>*>(quota)->SetValue(*f_left->valuePtr / *f_right->valuePtr);
		self->returnValue = quota;
	}
	else if (t == DataType::Int)
	{
		Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
		Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

		Data* quota = nullptr;
		i_left->CreateSameType(quota);
		dynamic_cast<Value<Int>*>(quota)->SetValue(*i_left->valuePtr / *i_right->valuePtr);
		self->returnValue = quota;
	}
}

void FunctionLibrary::F_Less(Function* self)
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

		//Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, f_left->token);
		Value<Bool>* comp = Memory<Value<Bool>>().New(DataType::Bool, false, f_left->token);
		comp->SetValue(*f_left->valuePtr < *f_right->valuePtr);
		self->returnValue = comp;
	}
	else if (t == DataType::Int)
	{
		Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
		Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

		//Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, i_left->token);
		Value<Bool>* comp = Memory<Value<Bool>>().New(DataType::Bool, false, i_left->token);
		comp->SetValue(*i_left->valuePtr < *i_right->valuePtr);
		self->returnValue = comp;
	}
}

void FunctionLibrary::F_Equal(Function* self)
{
	if (!self->CheckArgumens(2))
		return;

	DataType t;
	Data* left = self->arguments[0]->Evaluate();
	AFFIRM_DATA(left)
		Data* right = self->arguments[1]->Evaluate();
	AFFIRM_DATA(right)

		if ((t = left->type) != right->type || (t != DataType::Bool && t != DataType::Int && t != DataType::Float && t != DataType::String))
		{
			left->token.sourceCodePtr->PrintError(left->token, "type mismatch");
			return;
		}

	if (t == DataType::Bool)
	{
		Value<Bool>* b_left = dynamic_cast<Value<Bool>*>(left);
		Value<Bool>* b_right = dynamic_cast<Value<Bool>*>(right);

		//Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, b_left->token);
		Value<Bool>* comp = Memory<Value<Bool>>().New(DataType::Bool, false, b_left->token);
		comp->SetValue(*b_left->valuePtr == *b_right->valuePtr);
		self->returnValue = comp;
	}
	if (t == DataType::Float)
	{
		Value<Float>* f_left = dynamic_cast<Value<Float>*>(left);
		Value<Float>* f_right = dynamic_cast<Value<Float>*>(right);

		//Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, f_left->token);
		Value<Bool>* comp = Memory<Value<Bool>>().New(DataType::Bool, false, f_left->token);
		comp->SetValue(*f_left->valuePtr == *f_right->valuePtr);
		self->returnValue = comp;
	}
	else if (t == DataType::Int)
	{
		Value<Int>* i_left = dynamic_cast<Value<Int>*>(left);
		Value<Int>* i_right = dynamic_cast<Value<Int>*>(right);

		//Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, i_left->token);
		Value<Bool>* comp = Memory<Value<Bool>>().New(DataType::Bool, false, i_left->token);
		comp->SetValue(*i_left->valuePtr == *i_right->valuePtr);
		self->returnValue = comp;
	}
	else if (t == DataType::String)
	{
		Value<String>* s_left = dynamic_cast<Value<String>*>(left);
		Value<String>* s_right = dynamic_cast<Value<String>*>(right);

		//Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, s_left->token);
		Value<Bool>* comp = Memory<Value<Bool>>().New(DataType::Bool, false, s_left->token);
		comp->SetValue(*s_left->valuePtr == *s_right->valuePtr);
		self->returnValue = comp;
	}
}

void FunctionLibrary::F_And(Function* self)
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

	//Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, b_left->token);
	Value<Bool>* comp = Memory<Value<Bool>>().New(DataType::Bool, false, b_left->token);
	comp->SetValue(*b_left->valuePtr && *b_right->valuePtr);
	self->returnValue = comp;
}

void FunctionLibrary::F_Or(Function* self)
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

	//Value<Bool>* comp = new Value<Bool>(DataType::Bool, false, b_left->token);
	Value<Bool>* comp = Memory<Value<Bool>>().New(DataType::Bool, false, b_left->token);
	comp->SetValue(*b_left->valuePtr || *b_right->valuePtr);
	self->returnValue = comp;
}

void FunctionLibrary::F_Not(Function* self)
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

	//Value<Bool>* b_not = new Value<Bool>(DataType::Bool, false, b->token);
	Value<Bool>* b_not = Memory<Value<Bool>>().New(DataType::Bool, false, b->token);
	b_not->SetValue(!*b->valuePtr);
	self->returnValue = b_not;
}

void FunctionLibrary::F_Count(Function* self)
{
	if (!self->CheckArgumens(1))
		return;

	Data* first = self->arguments[0]->Evaluate();
	AFFIRM_DATA(first)

		if (first->type == DataType::List)
		{
			Value<List>* list = dynamic_cast<Value<List>*>(first);
			//Value<Int>* count = new Value<Int>(DataType::Int, false, first->token);
			Value<Int>* count = Memory<Value<Int>>().New(DataType::Int, false, first->token);
			count->SetValue((int)list->valuePtr->size());
			self->returnValue = count;
		}
		else if (first->type == DataType::String)
		{
			Value<String>* str = dynamic_cast<Value<String>*>(first);
			//Value<Int>* count = new Value<Int>(DataType::Int, false, first->token);
			Value<Int>* count = Memory<Value<Int>>().New(DataType::Int, false, first->token);
			count->SetValue((int)str->valuePtr->size());
			self->returnValue = count;
		}
		else
		{
			first->token.sourceCodePtr->PrintError(first->token, "expected list or string");
		}
}

void FunctionLibrary::F_Keys(Function* self)
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
	//Value<List>* list = new Value<List>(DataType::List, false, first->token);
	Value<List>* list = Memory<Value<List>>().New(DataType::List, false, first->token);
	list->Init();

	for (auto& e : map->valuePtr->map)
	{
		//Value<String>* key = new Value<String>(DataType::String, false, first->token);
		Value<String>* key = Memory<Value<String>>().New(DataType::String, false, first->token);
		key->SetValue(e.first);
		list->valuePtr->push_back(key);
	}

	self->returnValue = list;
}

void FunctionLibrary::F_CallCPPFunction(Function* self)
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
	if (Script::scriptFunctions.count(*name->valuePtr) == 0)
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

	Script::scriptFunctions[*name->valuePtr](args);
}


Script::Script()
{
	rootFunction = nullptr;
}

std::string Script::workingDirectory;
std::unordered_map<std::string, void(*)(List&)> Script::scriptFunctions;

bool Script::IsDigit(char c)
{
	return c >= '0' && c <= '9';
}

bool Script::IsWhitespace(char c)
{
	return c == ' ' || c == '\n' || c == '\t';
}

bool Script::IsDelimiter(char c)
{
	return IsWhitespace(c) || c == '(' || c == ')';
}

bool Script::ApplyIncludes()
{
	while (true)
	{
		if (sourceCode.BeginsWith("/*"))
		{
			for (; sourceCode.NextChar() && !sourceCode.BeginsWith("*/"););

			sourceCode.NextChar();
		}
		else if (sourceCode.BeginsWith("//"))
		{
			for (; sourceCode.NextChar() && sourceCode.CurrentChar() != '\n';);
		}
		else if (sourceCode.CurrentChar() == '"')
		{
			for (; sourceCode.NextChar() && sourceCode.CurrentChar() != '"';);
		}
		else if (sourceCode.BeginsWith("#include"))
		{
			size_t includeStartIndex = sourceCode.index;
			sourceCode.MoveAlong(8);

			std::string includeString = workingDirectory;
			char c;
			int quotationMarks = 0;
			for (; sourceCode.NextChar();)
			{
				if ((c = sourceCode.CurrentChar()) == '"')
				{
					if (++quotationMarks == 2)
						break;
				}
				else if (quotationMarks == 1)
					includeString += c;
			}

			if (quotationMarks != 2)
			{
				sourceCode.PrintErrorAtCurrentIndex("missing '\"' after #include");
				return false;
			}

			SourceCode includeCode;
			if (!includeCode.ReadFile(includeString))
			{
				sourceCode.PrintErrorAtCurrentIndex("failed to include file");
				return false;
			}

			sourceCode.text.erase(includeStartIndex, sourceCode.index - includeStartIndex + 2);
			sourceCode.text.insert(sourceCode.text.begin() + includeStartIndex, includeCode.text.begin(), includeCode.text.end());
			sourceCode.index = includeStartIndex;
			continue;
		}

		if (!sourceCode.NextChar())
			break;
	}

	sourceCode.Reset();
	return true;
}

void Script::HideStrings()
{
	while (true)
	{
		if (sourceCode.BeginsWith("/*"))
		{
			for (; sourceCode.NextChar() && !sourceCode.BeginsWith("*/"););

			sourceCode.NextChar();
		}
		else if (sourceCode.BeginsWith("//"))
		{
			for (; sourceCode.NextChar() && sourceCode.CurrentChar() != '\n';);
		}
		else if (sourceCode.BeginsWith("#macro"))
		{
			for (; sourceCode.NextChar() && sourceCode.CurrentChar() != '\n';);
		}
		else if (sourceCode.CurrentChar() == '"')
		{
			int startIndex = sourceCode.index;
			std::string str = "\"";
			for (; sourceCode.NextChar() && sourceCode.CurrentChar() != '"';)
			{
				str += sourceCode.CurrentChar();
			}
			str += "\"";
			int endIndex = sourceCode.index;
			std::string key = "@" + std::to_string(nextHiddenStringIndex++) + "@";
			hiddenStrings[key] = str;

			sourceCode.text.erase(startIndex, endIndex - startIndex + 1);
			sourceCode.text.insert(sourceCode.text.begin() + startIndex, key.begin(), key.end());
			sourceCode.index = startIndex + key.size();
		}

		if (!sourceCode.NextChar())
			break;
	}

	sourceCode.Reset();
}

void Script::RemoveComments()
{
	std::regex commentsRgx("(?:\\/\\/.*)|(?:\\/\\*[\\S\\s]*?\\*\\/)");
	sourceCode.text = std::regex_replace(sourceCode.text, commentsRgx, "", std::regex_constants::format_default);
}

bool Script::ApplyMacros()
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

			std::regex rgxName("(\\$name)");
			std::regex rgxAny("(\\$any)");
			regexStr = std::regex_replace(regexStr, rgxName, "[a-zA-Z_][a-zA-Z0-9_]*", std::regex_constants::format_default);
			regexStr = std::regex_replace(regexStr, rgxAny, "[\\S\\s]*?", std::regex_constants::format_default);

			std::string expansionStr;
			for (; sourceCode.NextChar() && (c = sourceCode.CurrentChar()) != '\n';)
				expansionStr += c;

			std::string restOfText = sourceCode.Substring(sourceCode.index + 1, sourceCode.text.size() - 1);
			std::string result;

			try
			{
				std::regex rgx(regexStr);
				result = std::regex_replace(restOfText, rgx, expansionStr, std::regex_constants::format_default);
			}
			catch (const std::regex_error& e) {
				sourceCode.PrintErrorAtCurrentIndex("regex: " + regexStr + " " + e.what());
				return false;
			}

			sourceCode.text.resize(macroStartIndex);
			sourceCode.text.append(result.begin(), result.end());
			sourceCode.index = macroStartIndex;
			continue;
		}

		if (!sourceCode.NextChar())
			break;
	}

	sourceCode.Reset();
	return true;
}

void Script::ShowStrings()
{
	for (auto& str : hiddenStrings)
	{
		std::regex strRgx("(" + str.first + ")");
		sourceCode.text = std::regex_replace(sourceCode.text, strRgx, str.second, std::regex_constants::format_default);
	}
}

bool Script::ApplyPreprocessing()
{
	bool logExpanded = sourceCode.BeginsWith("#log_expanded");
	std::string logFilename;
	if (logExpanded)
	{
		sourceCode.MoveAlong(13);
		char c;
		for (; (c = sourceCode.CurrentChar()) != '"' && c != '\n' && sourceCode.NextChar(););

		if (c == '"')
		{
			sourceCode.NextChar();
			for (; (c = sourceCode.CurrentChar()) != '"' && sourceCode.NextChar();)
				logFilename += c;

			for (; sourceCode.CurrentChar() != '\n' && sourceCode.NextChar(););
		}

		sourceCode.text.erase(0, sourceCode.index + 1);
		sourceCode.Reset();
	}

	if (!ApplyIncludes())
		return false;

	HideStrings();
	RemoveComments();

	if (!ApplyMacros())
		return false;

	ShowStrings();

	if (logExpanded)
	{
		if (logFilename.size() == 0)
			printf("[INFO] preprocessed code:\n%s\n", sourceCode.text.c_str());
		else
		{
			logFilename = workingDirectory + logFilename;
			std::ofstream logFile;
			logFile.open(logFilename);
			logFile << sourceCode.text;
			logFile.close();
			printf("[INFO] preprocessed code logged to %s\n", logFilename.c_str());
		}
	}

	std::regex newlineRgx("(\\\\n)");
	sourceCode.text = std::regex_replace(sourceCode.text, newlineRgx, "\n", std::regex_constants::format_default);
	std::regex tabRgx("(\\\\t)");
	sourceCode.text = std::regex_replace(sourceCode.text, tabRgx, "\t", std::regex_constants::format_default);

	sourceCode.Reset();
	return true;
}

bool Script::RecursiveParse(Data*& outData, int maxIndex)
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
			sourceCode.NextChar();
		}
		else if (sourceCode.BeginsWith("//"))
		{
			for (; sourceCode.NextChar() && sourceCode.CurrentChar() != '\n';);
		}
		else if (c == '"')
		{
			int first = sourceCode.index;
			for (; sourceCode.NextChar() && sourceCode.CurrentChar() != '"';);
			int last = sourceCode.index;

			//Value<String>* val = new Value<String>(DataType::String, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
			Value<String>* val = Memory<Value<String>>().New(DataType::String, true, Token(sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode));
			val->SetValue(sourceCode.Substring(first + 1, last - 1));
			sourceCode.NextChar();

			outData = val;
			return true;
		}
		else if (c == '-' || (IsDigit(c) && unknown.size() == 0))
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
				//Value<Float>* val = new Value<Float>(DataType::Float, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
				Value<Float>* val = Memory<Value<Float>>().New(DataType::Float, true, Token(sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode));
				val->SetValue(std::stof(num));
				outData = val;
				return true;
			}
			else
			{
				//Value<Int>* val = new Value<Int>(DataType::Int, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
				Value<Int>* val = Memory<Value<Int>>().New(DataType::Int, true, Token(sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode));
				val->SetValue(std::stoi(num));
				outData = val;
				return true;
			}
		}
		else if (sourceCode.BeginsWith("true"))
		{
			//Value<Bool>* val = new Value<Bool>(DataType::Bool, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
			Value<Bool>* val = Memory<Value<Bool>>().New(DataType::Bool, true, Token(sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode));
			val->SetValue(true);
			sourceCode.MoveAlong(4);
			outData = val;
			return true;
		}
		else if (sourceCode.BeginsWith("false"))
		{
			//Value<Bool>* val = new Value<Bool>(DataType::Bool, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
			Value<Bool>* val = Memory<Value<Bool>>().New(DataType::Bool, true, Token(sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode));
			val->SetValue(false);
			sourceCode.MoveAlong(5);
			outData = val;
			return true;
		}
		else if (sourceCode.BeginsWith("list"))
		{
			//Value<List>* val = new Value<List>(DataType::List, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
			Value<List>* val = Memory<Value<List>>().New(DataType::List, true, Token(sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode));
			val->SetValue({});
			sourceCode.MoveAlong(4);
			outData = val;
			return true;
		}
		else if (sourceCode.BeginsWith("map"))
		{
			//Value<Map>* val = new Value<Map>(DataType::Map, true, { sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode });
			Value<Map>* val = Memory<Value<Map>>().New(DataType::Map, true, Token(sourceCode.row, sourceCode.col, sourceCode.index, &sourceCode));
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

			//Value<Function>* val = new Value<Function>(DataType::Function, true, unknownToken);
			Value<Function>* val = Memory<Value<Function>>().New(DataType::Function, true, unknownToken);
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
			for (int i = sourceCode.index; i < sourceCode.text.size(); i++, closingIndex++)
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
				for (; RecursiveParse(arg, closingIndex - 1) && arg != nullptr;)
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
			else if (sourceCode.index > lastUnknownCharIndex + 1)
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

bool Script::LoadScript(const std::string& path)
{
	if (!sourceCode.ReadFile(workingDirectory + path))
		return false;

	if (!ApplyPreprocessing())
		return false;

	Data* res = nullptr;
	if (!RecursiveParse(res) || !res->AffirmSameType(DataType::Function))
		return false;

	rootFunction = dynamic_cast<Value<Function>*>(res);
	return true;
}

void Script::Run()
{
	rootFunction->valuePtr->Call();
}
