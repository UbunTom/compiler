#ifndef TREE_H
#define TREE_H

#include "Symbol.h"
#include <vector>
#include "Parserbase.h"
#include "Exception.h"
#include <map>

class Scoped
{
	protected:
	std::string identifier;

	public:
		std::string name(){
			return identifier;
		}
		virtual std::string typeString()=0;
};

class TypeSpec;
class Pointer;
class Declarator;

class ScopedVariable: public Scoped
{
	TypeSpec* type;
	Pointer* ptr;

	public:
	bool used;
	bool inReg;	//Is currently in a rregister
	int regLoc;	//Register location
	int location;	//stack location

	int getSize();
	
	ScopedVariable(Declarator* d, TypeSpec* t);
	ScopedVariable(Declarator* d, TypeSpec* t, int);
	std::string typeString();

	void use(int);
	void store();
	
	int getArrayReg(){return regLoc;}
	int getARMReg(){return regLoc+2;}
};

class ScopedFunction: public Scoped
{
	public:
	std::string label;

	ScopedFunction(std::string i)
	{
		identifier = i;
		label = ".F" + label;
	}
	std::string typeString(){return "Function";}
};

class Branch
{
	protected:
	Branch* parent;
	std::map<std::string, Scoped*> vars;
	bool pulled;
	bool pullSS;
	
	public:
	std::vector<Branch*> scopeSearch;	//Scopes to search before searching parent

	Branch():
		parent(NULL),
		pulled(false),
		pullSS(false)
	{}


	virtual std::string format()=0;
	virtual void genCode() = 0;
	virtual void setParent(Branch* p){
		parent = p;
	}
	
	template<typename T>
	void dynamic_assign(T& to, Branch* from)
	{
		assert(from != NULL);
		T casted = dynamic_cast<T>(from);
		assert(casted != NULL);
		to = casted;
		
		to->setParent(this);
	}
	
	Scoped* getScope(std::string s)
	{
		if(vars.find(s) != vars.end()){
			return vars[s];
		}
		for(size_t i=0; i<scopeSearch.size(); i++)
		{
			Scoped* sc = scopeSearch[i]->searchScope(s);
			if (sc!=NULL) {
				//addScope(sc);
				return sc;
			}
		}
		if(parent != NULL){
			Scoped* sc = parent->getScope(s);
			//addScope(sc);
			return sc;
		}
		//pullScope();
		if(vars.find(s) != vars.end()){
			return vars[s];
		}
		throw NotInScopeException(s);
	}
	
	Scoped* searchScope(std::string s)
	{
		if(vars.find(s) != vars.end()){
			return vars[s];
		}
		for(size_t i=0; i<scopeSearch.size(); i++)
		{
			Scoped* sc = scopeSearch[i]->searchScope(s);
			if (sc!=NULL) return sc;
		}
		return NULL;
	}
	
	void addScope(Scoped* s)
	{
		if(vars.find(s->name()) != vars.end()){
			throw VariableRedefinedError(s->name(), s->typeString());
		}
		vars[s->name()] = s;
	}
/*
	std::map<std::string, Scoped*>& pullScope()
	{
		if(!pulled)
		{
			if(parent!=NULL)
			{
				auto parentScope = parent->pullScope();
				vars.insert(parentScope.begin(), parentScope.end());
			}
			pullScopeSearch();
			pulled = true;
		}
		return vars;
	}
	
	std::map<std::string, Scoped*>& pullScopeSearch()
	{
		if(!pullSS)
		{
			for(size_t i=0; i<scopeSearch.size(); i++)
			{
				auto parentScope = scopeSearch[i]->pullScopeSearch();
				vars.insert(parentScope.begin(), parentScope.end());
			}
			pullSS = true;
		}
		return vars;
	}
	*/
	virtual ~Branch(){}
};


class AssignmentOperator: public Branch
{
	int type;
	
	std::string getSymbol(){
		switch (type){
			case '=': return "=";
			case ParserBase::MUL_ASSIGN: return "*=";
			case ParserBase::DIV_ASSIGN: return "/=";
			case ParserBase::MOD_ASSIGN:return "%=";
			case ParserBase::ADD_ASSIGN: return "+=";
			case ParserBase::SUB_ASSIGN: return "-=";
			case ParserBase::LEFT_ASSIGN: return "<<=";
			case ParserBase::RIGHT_ASSIGN: return ">>=";
			case ParserBase::AND_ASSIGN: return "&=";
			case ParserBase::XOR_ASSIGN: return "^=";
			case ParserBase::OR_ASSIGN: return "|=";
		}
		return "No symbol!";
	}
		
	
public:
	AssignmentOperator(int i)
	{
		type = i;
	}

	
	std::string format()
	{
		return getSymbol();
	}

	void genCode(){}
};


#endif
