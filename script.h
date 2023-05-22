#pragma once
#include "source_code.h"
#include "value.h"
#include "value_types.h"

struct FunctionLibrary
{
	std::unordered_map<std::string, void(*)(Function*)> functions;

	FunctionLibrary();

	static void F_Do(Function* self);
	static void F_Function(Function* self);
	static void Helper_PrintBool(Data* data);
	static void Helper_PrintInt(Data* data);
	static void Helper_PrintFloat(Data* data);
	static void Helper_PrintString(Data* data);
	static void Helper_PrintList(Data* data);
	static void Helper_PrintMap(Data* data);
	static void Helper_PrintFunction(Data* data);
	static void Helper_PrintAny(Data* d);
	static void F_Print(Function* self);
	static void F_Input(Function* self);
	static void F_ReturnCopy(Function* self);
	static void F_ReturnReference(Function* self);
	static void F_SetCopy(Function* self);
	static void F_SetReference(Function* self);
	static void F_AddElementsAsCopies(Function* self);
	static void F_AddElementsAsReferences(Function* self);
	static void F_GetElement(Function* self);
	static void F_RemoveElement(Function* self);
	static void F_HasKey(Function* self);
	static void F_DefineFunction(Function* self);
	static void F_GetVariable(Function* self);
	static void F_GetFunctionReference(Function* self);
	static void F_FunctionReference(Function* self);
	static void F_EvaluateFunction(Function* self);
	static void F_If(Function* self);
	static void F_While(Function* self);
	static void F_TypeOf(Function* self);
	static void F_ToString(Function* self);
	static bool Helper_IsInt(const std::string& str);
	static bool Helper_IsFloat(const std::string& str);
	static void F_ToInt(Function* self);
	static void F_ToFloat(Function* self);
	static void F_Add(Function* self);
	static void F_Sub(Function* self);
	static void F_Mult(Function* self);
	static void F_Div(Function* self);
	static void F_Less(Function* self);
	static void F_Equal(Function* self);
	static void F_And(Function* self);
	static void F_Or(Function* self);
	static void F_Not(Function* self);
	static void F_Count(Function* self);
	static void F_Keys(Function* self);
	static void F_CallCPPFunction(Function* self);
};

struct Script
{
	static std::string workingDirectory;
	static std::unordered_map<std::string, void(*)(List&)> scriptFunctions;

	SourceCode sourceCode;
	int nextHiddenStringIndex = 0;
	std::unordered_map<std::string, std::string> hiddenStrings;
	FunctionLibrary functionLibrary;
	Value<Function>* rootFunction;

	Script();

	bool IsDigit(char c);

	bool IsWhitespace(char c);

	bool IsDelimiter(char c);

	bool ApplyIncludes();

	void HideStrings();

	void RemoveComments();

	bool ApplyMacros();

	void ShowStrings();

	bool ApplyPreprocessing();

	bool RecursiveParse(Data*& outData, int maxIndex = -1);

	bool LoadScript(const std::string& path);

	void Run();
};