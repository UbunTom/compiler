#ifndef TREE_H
#define TREE_H

#include "Symbol.h"
#include <vector>
#include "Parserbase.h"
#include "Exception.h"
#include <map>
#include <memory>

/*
Base class for objects that can be accessed within a scope
*/
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

/*
Base class for objects that can be stored in registers
*/
class Registerable
{

	public:
	bool used;
	bool inReg;	//Is currently in a rregister
	int regLoc;	//Register location

	int stackAllocated = false;
	int stackLocation;	//stack location

	bool unregistered = false;



	Registerable():
		used(false),
		inReg(false)
	{}

	Registerable(int loc):
		used(true),
		inReg(true),
		regLoc(loc)
	{}

	void restore();
	void restore(int);
	void use(int);
	void store();
	void unregister();

	int bind();
	
	virtual int getSize() = 0;
	virtual std::string getNameInfo() = 0;
	
	int getReg(){return regLoc;}
	int getStackLocation();
};

class TemporaryValue: public Registerable{

	public:
	
	TemporaryValue();
	TemporaryValue(int reg);
	~TemporaryValue();
	int getSize(){return 4;}
	std::string getNameInfo(){return "<temp>";}

	template<class... Args>
	static std::shared_ptr<TemporaryValue> create(Args&&... args){
		return std::shared_ptr<TemporaryValue>(new TemporaryValue(std::forward<Args>(args)...));
	}
};

class ScopedVariable: public Registerable, public Scoped
{
	TypeSpec* type;
	Pointer* ptr;

	public:
	int getSize();
	
	ScopedVariable(Declarator* d, TypeSpec* t);
	ScopedVariable(Declarator* d, TypeSpec* t, int);
	std::string typeString();

	std::string getNameInfo(){return name();}

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
