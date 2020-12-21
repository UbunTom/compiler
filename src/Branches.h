#ifndef BRANCHES_H
#define BRANCHES_H

#include "Tree.h"
#include "Parserbase.h"
#include "ParseString.hpp"
#include <map>
#include <tuple>
#include "CodeGen.hpp"
#include "Allocation.hpp"
#include <algorithm>
#include "LabelAlloc.hpp"




class UnaryOperator: public Branch
{
	int character;
	
	public:
	UnaryOperator(int c):
		character(c)
	{}
	
	std::string format()
	{
		return std::string(" " ) + char(character);
	}

	void genCode(){}
};

enum ResultLocation
{
	REG,
	CONST,
	FLAGS,
	LITERAL,
	ADDRESS
};



class ExpressionResult
{
	public:
	ResultLocation loc;
	int64_t value;
	Flag flag;
	std::string str;
	
	ExpressionResult()
	{}

	ExpressionResult(ResultLocation b, int64_t v):
		loc(b), value(v)
	{}
	
	ExpressionResult(ResultLocation b, std::string v):
		loc(b), str(v)
	{}
	
	ExpressionResult(ResultLocation b, Flag f):
		loc(b), flag(f)
	{
		assert(loc == FLAGS);
	}
	
	void flagToReg()
	{
		assert(loc == FLAGS);
		int reg = RegAlloc::getScratch();
		CodeGen::push(new MoveBlock(reg, 0, true));
		CodeGen::push(new MoveBlock(reg, 1, true, flag));
		
		loc = REG;
		value = reg;
	}
	
	void constToReg()
	{
		assert(loc == CONST);
		int reg = RegAlloc::getScratch();
		CodeGen::push(new MoveBlock(reg, value, true));
		
		loc = REG;
		value = reg;
	}
	
	void literalToReg()
	{
		assert(loc == LITERAL);
		int reg = RegAlloc::getScratch();
		CodeGen::push(new TextBlock("    LDR r12, =" + str));
		
		loc = REG;
		value = reg;
	}
	
	void toReg()
	{
		if(loc == FLAGS) flagToReg();
		else if(loc == CONST) constToReg();
		else if(loc == LITERAL) literalToReg();
	}
};


bool checkResult(ExpressionResult& exp1, ResultLocation l1);
bool checkResult(ExpressionResult& exp1, ExpressionResult& exp2, ResultLocation l1, ResultLocation l2);
void orderResult(ExpressionResult& exp1, ExpressionResult& exp2, ResultLocation l1);

class _Expression: public Branch
{
public:
	void genCode(){
		assert(false);
	}

	virtual ExpressionResult execute() = 0;
	//Generates code for and returns a result
};

class AssignmentExpression: public _Expression
{
	int mode;
	_Expression* conditionalexp;
	
	_Expression* unaryexp;
	AssignmentOperator* op;
	_Expression* assignmentexp;

public:
	AssignmentExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(conditionalexp, _1);
	}
	
	AssignmentExpression(Branch* _1, Branch* _2, Branch* _3):
		mode(1)
	{
		dynamic_assign(unaryexp, _1);
		dynamic_assign(op, _2);
		dynamic_assign(assignmentexp, _3);
	}
	
	std::string format()
	{
		if(mode == 0)return conditionalexp->format();
		return unaryexp->format() + " " + op->format() + " " + assignmentexp->format();
	}

	ExpressionResult execute()
	{
		if(mode==0) return conditionalexp->execute();
		ExpressionResult lhs = unaryexp->execute();
		ExpressionResult rhs = assignmentexp->execute();

		if (!checkResult(lhs, REG)) throw SyntaxError("Left hand size of assignment is immutable");
		
		if(rhs.loc == LITERAL) rhs.literalToReg();

		if(checkResult(rhs, REG)){
			CodeGen::push(new MoveBlock(lhs.value, rhs.value, false));
		}
		else if(checkResult(rhs, CONST)){
			CodeGen::push(new MoveBlock(lhs.value, rhs.value, true));
		}
		else
		{
			CodeGen::push(new MoveBlock(lhs.value, 0, true));
			CodeGen::push(new MoveBlock(lhs.value, 1, true, rhs.flag));
		}

		return lhs;
	}
	
};


class Identifier: public _Expression
{
	std::string id;
public:
	Identifier(std::string s){
		id = s;
	}
	std::string format(){
		return id;
	}

	ExpressionResult execute()
	{
		ScopedVariable* s = dynamic_cast<ScopedVariable*>(getScope(id));
		assert(s != NULL);
		RegAlloc::bindReg(s);
		return ExpressionResult(REG, s->getARMReg());
	}
};

class Constant: public _Expression
{
	double dataf;
	int datai;
	bool intType;
public:
	Constant(std::string s, int t){
		intType = true;
		if(t == ParserBase::I_CONSTANT){
			datai = parseInt(s);
		}
		else if(t == ParserBase::F_CONSTANT)
		{
			dataf = parseFloat(s);
			intType = false;
			throw UnimplementedException("Float literals not supported");
		}
	}
	std::string format(){
		if(intType)return std::to_string((long long)datai);
		return std::to_string((long double)dataf);
	}

	ExpressionResult execute()
	{
		return ExpressionResult(CONST, datai);
	}
};

class String: public _Expression
{
	std::string id;
public:
	String(std::string s){
		id = s;

	}
	std::string format(){return id;}

	ExpressionResult execute()
	{
		return ExpressionResult(LITERAL, StringBin::newLiteral(id));
	}
};



class ExpressionList: public _Expression
{
	_Expression* first;
	
	_Expression* next;
	
public:
	ExpressionList(Branch* _1, Branch* _2)
	{
		dynamic_assign(first, _1);
		dynamic_assign(next, _2);
		
	}
	std::string format()
	{
		return first->format() + ", " + next->format();
	
	}
	ExpressionResult execute()
	{
		first->execute();
		return  next->execute();
	}
};

class ArgumentExpressionList: public _Expression
{
	_Expression* first;
	
	ArgumentExpressionList* next;
	
public:
	ArgumentExpressionList(Branch* _1)
	{
		dynamic_assign(first, _1);
		next = NULL;
	}
	
	void extend(Branch* _1)
	{
		if(!next)dynamic_assign(next, _1);
		else next->extend(_1);
	}
	
	std::string format()
	{
		return first->format() + ", " + (next?next->format():"");
	
	}
	ExpressionResult execute()
	{
		executeEval(0);
		return ExpressionResult();
	}
	
	int executeEval(int i){
		ExpressionResult eval = first->execute();
		eval.toReg();
		
		CodeGen::push(new StackPushPop({eval.value}, true));
		
		int ret = i;
		if(next){
			ret = next->executeEval(i+1);
		}
		
		CodeGen::push(new StackPushPop({i}, false));
		
		return ret;
		
	}
	
};

class FunctionCall: public _Expression
{
	Identifier* iden;
	ArgumentExpressionList* ael;

public:
	FunctionCall(Branch* _1){
		dynamic_assign(iden, _1);
		ael = NULL;
	}
	
	FunctionCall(Branch* _1, Branch* _2){
		dynamic_assign(iden, _1);
		dynamic_assign(ael, _2);
	}
	
	std::string format()
	{
		return iden->format() + "(" + (ael?ael->format():"");
	}
	
	ExpressionResult execute()
	{
		if(ael){
			ael->execute();
		}
		
		RegAlloc::storeAll();
		CodeGen::push(new BBlock(iden->format(), true));
		RegAlloc::loadAll();
		
		return ExpressionResult(REG, 0);
	}
};

class BracketedExpression: public _Expression
{
	_Expression* exp;
public:
	BracketedExpression(Branch* _1){
		dynamic_assign(exp, _1);
	}
	std::string format(){return "(" + exp->format() + ")";}

	ExpressionResult execute()
	{
		return exp->execute();
	}
};


class PostfixExpression: public _Expression
{
	_Expression* postexp;

public:
	PostfixExpression(Branch* _1)
	{
		dynamic_assign(postexp, _1);
	}

	
	std::string format()
	{	
		return postexp->format();
	}

	ExpressionResult execute()
	{
		return postexp->execute();
	}
};

class ConditionalExpression: public _Expression
{
	int mode;
	_Expression* logicalorexp;
	//optional
	_Expression* exp;
	_Expression* condexp;
	
public:
	ConditionalExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(logicalorexp, _1);
	}
	
	ConditionalExpression(Branch* _1, Branch* _2, Branch* _3):
		mode(1)
	{
		dynamic_assign(logicalorexp, _1);
		dynamic_assign(exp, _2);
		dynamic_assign(condexp, _3);
	}
	
	std::string format(){
		if(mode == 0) return logicalorexp->format();
		else return logicalorexp->format() + " ? " + exp->format() + " : " + condexp->format();
	}

	ExpressionResult execute(){
		if (mode==0) return logicalorexp->execute();
		throw UnimplementedException("conditional");
	}
};

class LogicalOrExpression: public _Expression
{
	int mode;
	_Expression* logicalandexp;
	//Optional
	_Expression* logicalorexp;
	
public:
	LogicalOrExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(logicalandexp, _1);
	}
	LogicalOrExpression(Branch* _1, Branch* _2):
		mode(1)
	{
		dynamic_assign(logicalorexp, _1);
		dynamic_assign(logicalandexp, _2);
	}
	
	std::string format(){
		if(mode == 0) return logicalandexp->format();
		else return logicalorexp->format() + " || " + logicalandexp->format();
	}

	ExpressionResult execute(){
		if (mode==0) return logicalandexp->execute();
		throw UnimplementedException("logicalor");
	}
};

class LogicalAndExpression: public _Expression
{
	int mode;
	_Expression* inclusiveorexp;
	//Optional
	_Expression* logicalandexp;
	
public:
	LogicalAndExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(inclusiveorexp, _1);
	}
	LogicalAndExpression(Branch* _1, Branch* _2):
		mode(1)
	{
		dynamic_assign(logicalandexp, _1);
		dynamic_assign(inclusiveorexp, _2);
	}
	
	std::string format(){
		if(mode == 0) return inclusiveorexp->format();
		else return logicalandexp->format() + " && " + inclusiveorexp->format();
	}

	ExpressionResult execute(){
		if (mode==0) return inclusiveorexp->execute();
		throw UnimplementedException("logicaland");
	}
};

class InclusiveOrExpression: public _Expression
{
	int mode;
	_Expression* exclusiveorexp;
	//Optional
	_Expression* inclusiveorexp;
	
public:
	InclusiveOrExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(exclusiveorexp, _1);
	}
	InclusiveOrExpression(Branch* _1, Branch* _2):
		mode(1)
	{
		dynamic_assign(inclusiveorexp, _1);
		dynamic_assign(exclusiveorexp, _2);
	}
	
	std::string format(){
		if(mode == 0) return exclusiveorexp->format();
		else return inclusiveorexp->format() + " | " + exclusiveorexp->format();
	}

	ExpressionResult execute(){
		if (mode==0) return exclusiveorexp->execute();
		
		ExpressionResult lhs = exclusiveorexp->execute();
		ExpressionResult rhs = inclusiveorexp->execute();
		
		if(checkResult(lhs, FLAGS))lhs.flagToReg();
		if(checkResult(rhs, FLAGS))rhs.flagToReg();
		
		if(checkResult(lhs, rhs, REG, REG))
		{
			int res = RegAlloc::getScratch();
			CodeGen::push(new OrBlock(res, lhs.value, rhs.value, false));
			return ExpressionResult(REG, res);
		}
		else if(checkResult(lhs, rhs, CONST, REG))
		{
			int res = RegAlloc::getScratch();
			orderResult(lhs, rhs, REG);
			CodeGen::push(new OrBlock(res, lhs.value, rhs.value, true));
			return ExpressionResult(REG, res);
		}
		else
		{
			return ExpressionResult(CONST, lhs.value && rhs.value);
		}
	}
};

class ExclusiveOrExpression: public _Expression
{
	int mode;
	_Expression* andexp;
	//Optional
	_Expression* exclusiveorexp;
	
public:
	ExclusiveOrExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(andexp, _1);
	}
	ExclusiveOrExpression(Branch* _1, Branch* _2):
		mode(1)
	{
		dynamic_assign(exclusiveorexp, _1);
		dynamic_assign(andexp, _2);
	}
	
	std::string format(){
		if(mode == 0) return andexp->format();
		else return exclusiveorexp->format() + " ^ " + andexp->format();
	}

	ExpressionResult execute(){
		if (mode==0) return andexp->execute();
		throw UnimplementedException("exclusiveor");
	}
};

class AndExpression: public _Expression
{
	int mode;
	_Expression* equalityexp;
	//Optional
	_Expression* andexp;
	
public:
	AndExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(equalityexp, _1);
	}
	AndExpression(Branch* _1, Branch* _2):
		mode(1)
	{
		dynamic_assign(andexp, _1);
		dynamic_assign(equalityexp, _2);
	}
	
	std::string format(){
		if(mode == 0) return equalityexp->format();
		else return andexp->format() + " & " + equalityexp->format();
	}

	ExpressionResult execute(){
		if (mode==0) return equalityexp->execute();
		
		ExpressionResult lhs = equalityexp->execute();
		ExpressionResult rhs = andexp->execute();
		
		if(checkResult(lhs, FLAGS))lhs.flagToReg();
		if(checkResult(rhs, FLAGS))rhs.flagToReg();
		
		if(checkResult(lhs, rhs, REG, REG))
		{
			int res = RegAlloc::getScratch();
			CodeGen::push(new AndBlock(res, lhs.value, rhs.value, false));
			return ExpressionResult(REG, res);
		}
		else if(checkResult(lhs, rhs, CONST, REG))
		{
			int res = RegAlloc::getScratch();
			orderResult(lhs, rhs, REG);
			CodeGen::push(new AndBlock(res, lhs.value, rhs.value, true));
			return ExpressionResult(REG, res);
		}
		else
		{
			return ExpressionResult(CONST, lhs.value && rhs.value);
		}
		
	}
};

class EqualityExpression: public _Expression
{
	int mode;
	_Expression* relationalexp;
	//Optional
	_Expression* equalityexp;
	int op;
	
public:
	EqualityExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(relationalexp, _1);
	}
	
	EqualityExpression(Branch* _1, int _2, Branch* _3):
		mode(1)
	{
		dynamic_assign(equalityexp, _1);
		op = _2;
		dynamic_assign(relationalexp, _3);
	}
	
	std::string format(){
		if(mode == 0) return relationalexp->format();
		else{
			 std::string ret = equalityexp->format();
			 if(op == ParserBase::EQ_OP) ret += " == ";
			 else ret += " != ";
			 ret += relationalexp->format();
			 return ret;
		}
	}

	ExpressionResult execute(){
		if (mode==0) return relationalexp->execute();
		
		ExpressionResult lhs = relationalexp->execute();
		ExpressionResult rhs = equalityexp->execute();
		
		if(checkResult(lhs, FLAGS))lhs.flagToReg();
		if(checkResult(rhs, FLAGS))rhs.flagToReg();
		
		if(checkResult(lhs, rhs, REG, REG))
		{
			CodeGen::push(new CMPBlock(lhs.value, rhs.value, false));
			return ExpressionResult(FLAGS, (op == ParserBase::EQ_OP) ? EQ : NE);
		}
		else if(checkResult(lhs, rhs, CONST, REG))
		{
			orderResult(lhs, rhs, REG);
			CodeGen::push(new CMPBlock(lhs.value, rhs.value, true));
			return ExpressionResult(FLAGS, (op == ParserBase::EQ_OP) ? EQ : NE);
		}
		else
		{
			return ExpressionResult(CONST, lhs.value == rhs.value);
		}
		
	}
};

class RelationalExpression: public _Expression
{
	int mode;
	_Expression* shiftexp;
	//Optional
	_Expression* relationalexp;
	int op;
	
public:
	
	RelationalExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(shiftexp, _1);
	}
	
	RelationalExpression(Branch* _1, int _2, Branch* _3):
		mode(1)
	{
		dynamic_assign(relationalexp, _1);
		op = _2;
		dynamic_assign(shiftexp, _3);
	}
	
	std::string format(){
		if(mode == 0) return shiftexp->format();
		else{
			std::string ret = relationalexp->format();
			switch(op){
				 case '<': ret += " < "; break;
				 case '>': ret += " > "; break;
				 case ParserBase::LE_OP: ret += " <= "; break;
				 case ParserBase::GE_OP: ret += " >= "; break;
			}
			ret += shiftexp->format();
			return ret;
		}
	}

	ExpressionResult execute(){
		if (mode==0) return shiftexp->execute();
		
		
		ExpressionResult lhs = shiftexp->execute();
		ExpressionResult rhs = relationalexp->execute();
		
		lhs.toReg();
		rhs.toReg();
		
		CodeGen::push(new CMPBlock(lhs.value, rhs.value, false));
		Flag fl;
		switch(op){
			case '<': fl = LT; break;
			case '>': fl = GT; break;
			case ParserBase::LE_OP: fl = LE; break;
			case ParserBase::GE_OP: fl = GE; break;
		}
			
		return ExpressionResult(FLAGS, fl);
		
	}
};

class ShiftExpression: public _Expression
{
	int mode;
	_Expression* addexp;
	//Optional
	_Expression* shiftexp;
	int op;
	
public:
	
	ShiftExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(addexp, _1);
	}
	
	ShiftExpression(Branch* _1, int _2, Branch* _3):
		mode(1)
	{
		dynamic_assign(shiftexp, _1);
		op = _2;
		dynamic_assign(addexp, _3);
	}
	
	std::string format(){
		if(mode == 0) return addexp->format();
		else{
			 std::string ret = shiftexp->format();
			 if(op == ParserBase::LEFT_OP) ret += " << ";
			 else ret += " >> ";
			 ret += addexp->format();
			 return ret;
		}
	}

	ExpressionResult execute(){
		if (mode==0) return addexp->execute();
		throw UnimplementedException("shift");
	}
};

class AddExpression: public _Expression
{
	int mode;
	_Expression* multexp;
	//Optional
	_Expression* addexp;
	int op;
	
public:
	
	AddExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(multexp, _1);
	}
	
	AddExpression(Branch* _1, int _2, Branch* _3):
		mode(1)
	{
		dynamic_assign(addexp, _1);
		op = _2;
		dynamic_assign(multexp, _3);
	}
	
	std::string format(){
		if(mode == 0) return multexp->format();
		else{
			 std::string ret = addexp->format();
			 if(op == '+') ret += " + ";
			 else ret += " - ";
			 ret += multexp->format();
			 return ret;
		}
	}

	ExpressionResult execute()
	{
		if (mode==0) return addexp->execute();
		
		ExpressionResult lhs = multexp->execute();
		ExpressionResult rhs = addexp->execute();
		
		if(checkResult(lhs, FLAGS))lhs.flagToReg();
		if(checkResult(rhs, FLAGS))rhs.flagToReg();
		
		if(checkResult(lhs, rhs, REG, REG))
		{
			int res = RegAlloc::getScratch();
			if(op == '+') CodeGen::push(new AddBlock(res, lhs.value, rhs.value, false));
			else CodeGen::push(new SubBlock(res, lhs.value, rhs.value, false));
			return ExpressionResult(REG, res);
		}
		else if(checkResult(lhs, rhs, CONST, CONST))
		{
			if(op == '+') return ExpressionResult(CONST, lhs.value + rhs.value);
			else return ExpressionResult(CONST, lhs.value - rhs.value);
		}
		else if(checkResult(lhs, rhs, REG, CONST))
		{
			int res = RegAlloc::getScratch();
			if(op == '+'){
				orderResult(lhs, rhs, REG);
				CodeGen::push(new AddBlock(res, lhs.value, rhs.value, true));
				return ExpressionResult(REG, res);
			}
			
			if(lhs.loc == REG){
				CodeGen::push(new SubBlock(res, lhs.value, -rhs.value, true));
				return ExpressionResult(REG, res);
			}
			
			CodeGen::push(new RSBBlock(res, lhs.value, rhs.value, true));
			return ExpressionResult(REG, res);
		}
		
		assert(false);
	}
};

class MultExpression: public _Expression
{
	int mode;
	_Expression* castexp;
	//Optional
	_Expression* multexp;
	int op;
	
public:	
	MultExpression(Branch* _1):
		mode(0)
	{
		dynamic_assign(castexp, _1);
	}
	
	MultExpression(Branch* _1, int _2, Branch* _3):
		mode(1)
	{
		dynamic_assign(multexp, _1);
		op = _2;
		dynamic_assign(castexp, _3);
	}
	
	std::string format(){
		if(mode == 0) return castexp->format();
		else{
			std::string ret = multexp->format();
			switch(op){
				 case '*': ret += " * "; break;
				 case '/': ret += " / "; break;
				 case '&': ret += " & "; break;
			}
			ret += castexp->format();
			return ret;
		}
	}

	ExpressionResult execute(){
		if (mode==0) return castexp->execute();
		throw UnimplementedException("mult");
	}
};

class CastExpression: public _Expression
{
	_Expression* unaryexp;

public:
	CastExpression(Branch* _1){
		dynamic_assign(unaryexp, _1);
	}
	
	std::string format(){
		return unaryexp->format();
	}

	ExpressionResult execute(){
		return unaryexp->execute();
	}
};

class UnaryExpression: public _Expression
{
	_Expression* postfixexp;

public:
	UnaryExpression(Branch* _1)
	{
		dynamic_assign(postfixexp, _1);
		
	}
	
	std::string format(){
		return postfixexp->format();
	}

	ExpressionResult execute(){
		return postfixexp->execute();
	}
};

class _Statement: public Branch
{};

class ReturnType: public _Statement
{
	_Expression* exp;
	
public:
	ReturnType(){
		exp = NULL;
	}
	
	ReturnType(Branch* _1){
		dynamic_assign(exp, _1);
	}
	
	std::string format()
	{
		return "return " + exp->format() + ";";
	}
	
	void genCode()
	{
		if(exp){
			ExpressionResult expr = exp->execute();
			expr.toReg();
			CodeGen::push(new MoveBlock(0, expr.value, false));
		}
		CodeGen::push(new BBlock("." + LoopLabelJump::getReturn(), false));
	}
};

class LoopFlowControl: public _Statement
{
	int contOrRet;
	
public:
	LoopFlowControl(int cor):
		contOrRet(cor)
	{}
	
	void genCode()
	{
		if (contOrRet==0)CodeGen::push(new BBlock(LoopLabelJump::getContinue(), false));
		else CodeGen::push(new BBlock(LoopLabelJump::getBreak(), false));
	}
	
	std::string format()
	{
		if (contOrRet==0)return "continue;";
		else return "break;";
	}

};

class DeclarationSpec: public Branch
{
	
};

class TypeSpec: public DeclarationSpec
{
	int type;
	
public:
	TypeSpec(int t):
		type(t)
	{}
	
	std::string format()
	{
		std::string put = " ";
		switch(type){
			case ParserBase::INT: put += "int"; break;
		}
		put += " ";
		return put;
	}

	void genCode(){}

};

class Pointer: public Branch
{
	public:
		Pointer()
		{}
	
};

class DirectDeclarator: public Branch
{
	protected:
	std::string identifier;
	
	public:
		std::string getName(){return identifier;}
};

class DirectDeclaratorBase: public DirectDeclarator
//The base case for a direct declarator
{
	
public:
	DirectDeclaratorBase(std::string i)
	{
		identifier = i;
	}
	
	std::string format()
	{
		return identifier;
	}

	void genCode(){}
};

class Declarator: public Branch
{
	Pointer* ptr;
	DirectDeclarator* dd;
	
	public:
	Declarator(Branch* b)
	{
		dynamic_assign(dd, b);
		ptr = NULL;
		scopeSearch.push_back(dd);
	}
	
	Declarator(Branch* p, Branch* b)
	{
		dynamic_assign(ptr, p);
		dynamic_assign(dd, b);
	}
	
	std::pair<Pointer*, DirectDeclarator*> getData()
	{
		return std::make_pair(ptr, dd);
	}
	
	std::string format()
	{
		return ((ptr!=NULL) ? ptr->format() : std::string("")) + dd->format();
	}

	void genCode(){
		if(ptr)ptr->genCode();
		dd->genCode();
	}
	
};



class InitDeclarator: public Branch
{
	Declarator* decl;
	_Expression* init;
public:
	InitDeclarator* next;
	
	InitDeclarator(Branch* _1){
		dynamic_assign(decl, _1);
		assert(dynamic_cast<DirectDeclaratorBase*>(std::get<1>(decl->getData())));
		init = NULL;
		next = NULL;
	}
	
	InitDeclarator(Branch* _1, Branch* _2){
		dynamic_assign(decl, _1);
		assert(dynamic_cast<DirectDeclaratorBase*>(std::get<1>(decl->getData())));
		dynamic_assign(init, _2);
		next = NULL;
	}
	
	std::string format()
	{
		std::string out = decl->format();
		if(init){
			out += " = " + init->format();
		}
		if(next) out += ", " + next->format();
		return out;
	}
	
	Declarator* getDecl(){
		return decl;
	}
	
	void extend(Branch* id)
	{
		if(next == NULL) dynamic_assign(next, id);
		else
		{
			next->extend(id);
			scopeSearch.push_back(id);	
		}
	}

	void genCode()	
	{
		std::string name = std::get<1>(decl->getData())->getName();
		
		if(init){
		
			AssignmentExpression* assign;
			dynamic_assign(assign, new AssignmentExpression(new Identifier(name), new AssignmentOperator('='), init));
			assign->execute();
		}
	
	}
	
};

class Declaration: public Branch
{
	TypeSpec* type;
	InitDeclarator* idl;
public:
	Declaration(Branch* _1, Branch* _2){
		dynamic_assign(type, _1);
		dynamic_assign(idl, _2);
		
		scopeSearch.push_back(idl);
	}
	

	std::string format()
	{
		return type->format() + " " + idl->format() + ';';
	}

	void genCode(){

		InitDeclarator* idecl = idl;
		while(idecl!=NULL)
		{
			
			Declarator* d = idecl->getDecl();
			idecl->addScope(new ScopedVariable(d, type));
			
			idecl->genCode();
			
			idecl = idecl->next;
		}
	}
};




class ParameterDecl: public Branch
{
	TypeSpec* type;
	Declarator* decl;
	

public:
	ParameterDecl(Branch* _1, Branch* _2){
		
		dynamic_assign(decl, _2);
		dynamic_assign(type, _1);
		
		
	}

	std::string format()
	{
		return type->format() + " " + decl->format();
	}

	void genCode(){}
	
	void genCode(int i){
		ScopedVariable* sv = new ScopedVariable(decl, type);
		addScope(sv);
		RegAlloc::bindReg(sv, i);
	}

};

class ParameterList: public Branch
{
	std::vector<ParameterDecl*> list;

public:
	ParameterList(Branch* _1, Branch* _2)
	{
		ParameterList* ptl;
		dynamic_assign(ptl, _1);
		list = std::move(ptl->list);
		delete ptl;

		ParameterDecl* pd;
		dynamic_assign(pd, _2);
		list.push_back(pd);
		
		for(size_t i=0; i<list.size(); i++){
			list[i]->setParent(this);
		}
	}
	
	ParameterList(Branch* _1)
	{
		ParameterDecl* pd;
		dynamic_assign(pd, _1);
		list.push_back(pd);
		
		for(size_t i=0; i<list.size(); i++){
			list[i]->setParent(this);
		}
	}		
	
	std::string format()
	{
		std::string out = "";
		for(size_t i=0; i<list.size(); i++)
		{
			out += list[i]->format();
		}
		return out;
	}

	void genCode()
	{

		for(size_t i=0; i<list.size(); i++)
		{
			list[i]->genCode(i);
			scopeSearch.push_back(list[i]);
		}
	}
};

class DirectDeclaratorFunc: public DirectDeclarator
{
	ParameterList* ptl;

public:
	DirectDeclaratorFunc(Branch* _1, Branch* _2)
	{
		DirectDeclaratorBase* ddb;
		dynamic_assign(ddb, _1);
		identifier = ddb->getName();
		delete ddb;

		dynamic_assign(ptl, _2);
		
		scopeSearch.push_back(ptl);
	}

	DirectDeclaratorFunc(Branch* _1)
	{
		DirectDeclaratorBase* ddb;
		dynamic_assign(ddb, _1);
		identifier = ddb->getName();
		delete ddb;

		ptl = NULL;
	}
	
	std::string format()
	{
		std::string out = identifier + "(";
		if(ptl)out += ptl->format();
		out += ")";
		return out;
	}

	void genCode()
	{
		if(ptl)ptl->genCode();
	}
};



class IfStatement: public _Statement
{
	_Expression* cmp;
	_Statement* then;
	_Statement* other;
	
public:
	IfStatement(Branch* _1, Branch* _2)
	{
		dynamic_assign(cmp, _1);
		dynamic_assign(then, _2);
		other = NULL;
	}
	
	IfStatement(Branch* _1, Branch* _2, Branch* _3)
	{
		dynamic_assign(cmp, _1);
		dynamic_assign(then, _2);
		dynamic_assign(other, _3);
	}
	
	std::string format()
	{
		std::string out = "if(" + cmp->format() + ")\n{\n" + then->format() + "\n}";
		if(other != NULL) out += "else\n{\n" + other->format() + "\n}\n";
		return out;
	}
	
	void genCode()
	{
		std::string notLabel = LabelAlloc::allocate();
		std::string afterLabel;
		if(!other)afterLabel = notLabel;
		else afterLabel = LabelAlloc::allocate();
		
		
		
		ExpressionResult rx = cmp->execute();
		if(checkResult(rx, REG))
		{
			CodeGen::push(new CMPBlock(rx.value, 0, true));
			CodeGen::push(new BranchBlock(notLabel, NE));
		}
		else if(checkResult(rx, CONST))
		{
			if(rx.value != 0)
				then->genCode();
			else if(other)
				other->genCode();
				
			return;
		}
		else{
			CodeGen::push(new BranchBlock(notLabel, flagInvert(rx.flag)));
			
		}
		
		StackStore::begin();
		then->genCode();
		StackStore::end();
		
		if(other)
		{
			CodeGen::push(new LabelBlock(notLabel));
			StackStore::begin();
			other->genCode();
			StackStore::end();
		}
		
		CodeGen::push(new LabelBlock(afterLabel));		
	}
};



class BlockItem: public Branch
{
	bool stmtmode;
	_Statement* stmt;
	Declaration* decl;
	
	BlockItem* next;

public:
	BlockItem(Branch* _1){
		if(dynamic_cast<_Statement*>(_1))
		{
			dynamic_assign(stmt, _1);
			stmtmode = true;
		}
		else
		{
			dynamic_assign(decl, _1);
			stmtmode = false;
		}
		
		next = NULL;

		scopeSearch.push_back(_1);
	}
	
	void extend(Branch* ext)
	{
		if(next==NULL) dynamic_assign(next, ext);
		else next->extend(ext);

		//scopeSearch.insert(scopeSearch.begin(), ext);
	}
	
	
	std::string format(){
		std::string out = "";
		if (stmtmode) out+=stmt->format();
		else out+=decl->format();
		
		if(next) out += next->format();
		
		return out;
	}

	void genCode()
	{
		if(stmtmode) stmt->genCode();
		else decl->genCode();

		if(next) next->genCode();
	}
};



class ExpressionStatement: public _Statement
{
	
	
	public:
	
	bool empty;
	_Expression* exp;
	
	ExpressionStatement():
		empty(true)
	{}
	
	ExpressionStatement(Branch* b):
		empty(false)
	{
		dynamic_assign(exp, b);
	}
	
	std::string format()
	{
		std::string out = "";
		if(!empty) out += exp->format();
		out += ";";
		return out;
	}

	void genCode()
	{
		if(!empty)exp->execute();
	}
	
};


class CompoundStatement: public _Statement
{
	bool empty;
	BlockItem* bil;
	
public:
	CompoundStatement():
		empty(true)
	{}
	
	CompoundStatement(Branch* _bil):
		empty(false)
	{
		dynamic_assign(bil, _bil);
	}
	
	std::string format()
	{
		if(!empty) return bil->format();
		return std::string("");
	}

	void genCode()
	{
		StackStore::begin();
		if(!empty)bil->genCode();
		StackStore::end();
	}		
};

class ForLoop: public _Statement
{
	//for(
	Declaration* decl;
	ExpressionStatement* declstmt;
	//;
	ExpressionStatement* expstmt;
	//;
	_Expression* exp;
	//){
	_Statement* stmt;
	//}
	
public:
	ForLoop(Branch* _1, Branch* _2, Branch* _3){
		setFirstParam(_1);
		dynamic_assign(expstmt, _2);
		exp = NULL;
		dynamic_assign(stmt, _3);
	}
	
	ForLoop(Branch* _1, Branch* _2, Branch* _3, Branch* _4){
		setFirstParam(_1);
		dynamic_assign(expstmt, _2);
		dynamic_assign(exp, _3);
		dynamic_assign(stmt, _4);
	}
	
	void setFirstParam(Branch* b)
	{
		if(dynamic_cast<Declaration*>(b))
		{
			dynamic_assign(decl, b);
			declstmt = NULL;
			scopeSearch.push_back(decl);
		}
		else
		{
			dynamic_assign(declstmt, b);
			decl = NULL;
		}
	}
	
	std::string format()
	{
		std::string out = "for(";
		if(decl)out += decl->format();
		else out += declstmt->format();
		
		out += ";" + expstmt->format();
		out += ";" + stmt->format();
		
		out += "\n{\n" + stmt->format() + "\n}\n";
		
		return out;
	}
	
	void genCode()
	{
		std::string loopLabel = LabelAlloc::allocate();
		std::string compLabel = LabelAlloc::allocate();
		std::string afterLabel = LabelAlloc::allocate();
		
		LoopLabelJump::push(afterLabel, compLabel);
		
		StackStore::begin();
		
		if(decl)decl->genCode();
		else declstmt->genCode();
		
		CodeGen::push(new BBlock(compLabel, false));
		
		//RegAlloc::pushState();
		CodeGen::push(new LabelBlock(loopLabel));
		
		exp->execute();
		
		stmt->genCode();
		
		
		//RegAlloc::popState(this);
		
		CodeGen::push(new LabelBlock(compLabel));
		
		if(expstmt->empty == true)
			CodeGen::push(new BranchBlock(loopLabel));
		else{
			ExpressionResult expr = expstmt->exp->execute();
			expr.toReg();
			CodeGen::push(new CMPBlock(expr.value, 0, true));
			CodeGen::push(new BranchBlock(loopLabel, NE));
		}
		
		CodeGen::push(new LabelBlock(afterLabel));
	
		StackStore::end();
		
		LoopLabelJump::pop();
	}
};

class WhileLoop: public _Statement
{
	//while(
	_Expression* exp;
	//){
	_Statement* stmt;
	//}
	
public:
	WhileLoop(Branch* _1, Branch* _2){
		dynamic_assign(exp, _1);
		dynamic_assign(stmt, _2);
	}

	
	std::string format()
	{
		std::string out = "while(";
		out += exp->format();
		out += ")\n{\n" + stmt->format() + "\n}\n";
		
		return out;
	}
	
	void genCode()
	{
		std::string loopLabel = LabelAlloc::allocate();
		std::string compLabel = LabelAlloc::allocate();
		std::string afterLabel = LabelAlloc::allocate();;
		
		LoopLabelJump::push(afterLabel, compLabel);
		
		StackStore::begin();
		
		CodeGen::push(new BBlock(compLabel, false));
		
		//RegAlloc::pushState();
		CodeGen::push(new LabelBlock(loopLabel));
		
		stmt->genCode();
		
		//RegAlloc::popState(this);
		
		CodeGen::push(new LabelBlock(compLabel));
		
		ExpressionResult expr = exp->execute();
		expr.toReg();
		CodeGen::push(new CMPBlock(expr.value, 0, true));
		CodeGen::push(new BranchBlock(loopLabel, NE));
		
		CodeGen::push(new LabelBlock(afterLabel));
	
		StackStore::end();
		
		LoopLabelJump::pop();
	}
};

class FuncDef: public Branch
{
		DeclarationSpec* declspec;	//Probably nothing more than a TypeSpec
		Declarator* decl;			//Function name
		CompoundStatement*	cmpstmt;	//Body of the function
	
	public:
	FuncDef(Branch* _1, Branch* _2, Branch* _3)
	{
		
		dynamic_assign(declspec, _1);
		dynamic_assign(decl, _2);
		dynamic_assign(cmpstmt, _3);
		
		DirectDeclaratorFunc* ddf = dynamic_cast<DirectDeclaratorFunc*>(std::get<1>(decl->getData()));
		assert(ddf != NULL);
		
		scopeSearch.push_back(decl);
		
	}
	
	std::string getIdentifier()
	{
		DirectDeclaratorFunc* ddf = dynamic_cast<DirectDeclaratorFunc*>(std::get<1>(decl->getData()));
		
		return ddf->getName();
	}
	
	std::string format()
	{
		return declspec->format() + " " + decl->format() + "\n{" + cmpstmt->format() + "\n}";
	}

	void genCode()
	{
		DirectDeclaratorFunc* ddf = dynamic_cast<DirectDeclaratorFunc*>(std::get<1>(decl->getData()));
		
		std::string fname = ddf->getName();
		
		CodeGen::push(new TextBlock("    .global " + fname));

		if(fname!="main")CodeGen::push(new LabelBlock(ddf->getName()));
		else CodeGen::push(new TextBlock(ddf->getName() + ":"));
		
		CodeGen::push(new StackPushPop({11,14}, true));
		CodeGen::push(new TextBlock("    add fp, sp, #0"));
		
		LoopLabelJump::setReturn(getIdentifier() + "return");
		
		
		StackStore::beginFunc();
		RegAlloc::begin();

		declspec->genCode();
		decl->genCode();
		
		
		
		cmpstmt->genCode();
		
		CodeGen::push(new LabelBlock("."+getIdentifier() + "return"));
		
		
		StackStore::endFunc();
		CodeGen::push(new TextBlock("    add sp, fp, #0"));
		CodeGen::push(new StackPushPop({11,15}, false));
	}

};

class TranslationUnit: public Branch
{
	FuncDef* funcdef;
	Declaration* decl;
	
	TranslationUnit* next;
	
	public:
	TranslationUnit(Branch* _1)
	{
		if(dynamic_cast<FuncDef*>(_1))
		{
			dynamic_assign(funcdef, _1);
			addScope(new ScopedFunction(funcdef->getIdentifier()));
			decl = NULL;
		}
		else
		{
			dynamic_assign(decl, _1);
			scopeSearch.push_back(decl);
			funcdef = NULL;
		}
		next = NULL;
	}
	
	void extend(Branch* _1)
	{
		if(!next)dynamic_assign(next, _1);
		else next->extend(_1);
	}

	std::string format()
	{
		std::string out;
		if(funcdef)out += funcdef->format();
		else out+= decl->format();

		if(next)out += next->format();
		return out;
	}

	
	void genCode()
	{
		if(decl)decl->genCode();
		else funcdef->genCode();
		
		if(next) next->genCode();
	}
	
};

class TopBranch
{
	static TranslationUnit* tu;

	public:
		static void set(TranslationUnit* t){tu = t;}
		static TranslationUnit* get(){return tu;}
};

#endif
