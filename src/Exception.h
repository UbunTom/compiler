#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>

class UnimplementedException: public std::runtime_error
{
	
	public:
	UnimplementedException(std::string s):
		std::runtime_error("Unimplemented parse: " + s)
	{}

};


class NotInScopeException: public std::runtime_error
{
	public:
	NotInScopeException(std::string s):
		std::runtime_error("The variable " + s + " is not in scope")
	{}
};

class VariableRedefinedError: public std::runtime_error
{
	
	public:
	VariableRedefinedError(std::string s, std::string t):
		std::runtime_error("The variable " + s + " has already been defined in scope as " + t)
	{}

};

class SyntaxError: public std::runtime_error
{
	
	public:
	SyntaxError(std::string s):
		std::runtime_error("Syntax error" + s)
	{}

};

class CompilerError: public std::runtime_error
{
	public:
	CompilerError(std::string s):
		std::runtime_error("Compiler error: " + s)
	{}

};

#endif
