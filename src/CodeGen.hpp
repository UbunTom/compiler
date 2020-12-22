#ifndef CODEGEN_H
#define CODEGEN_H

#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <exception>

#include "ExpressionResultBase.hpp"

enum Flag
{
	EQ, NE, MI, PL, GT, LT, GE, LE, NONE, NEVER
};

Flag flagInvert(Flag);

class CodeAbortException: public std::exception
{};

class CodeBlock
{
public:
	virtual std::string format()=0;
	
	std::string flagToStr(Flag f)
	{
		switch(f){
			case EQ: return "EQ";
			case NE: return "NE";
			case MI: return "MI";
			case PL: return "PL";
			case GT: return "GT";
			case LT: return "LT";
			case GE: return "GE";
			case LE: return "LE";
			case NONE: return "";
			case NEVER: throw CodeAbortException();
			default : assert(false);
		}
	}
};

class RegisterResultBlock: public CodeBlock
{
	friend class CodeGen;
	protected:
	int regD;
	public:
	RegisterResultBlock(int r):
		regD(r)
	{}
	
	void rebindResult(int r){
		regD = r;
	}
};

class Imm{
	public:
	int value;
	Imm(int v): value(v)
	{}
};

/*
Class to coerce ExpressionResults into register values
*/
class Dest{
	int value;

	public:
	Dest(int i){
		value = i;
	}

	Dest(ExpressionResult expr){
		assert(expr->isRegisterable());
		value = expr->getRegisterable()->bind();
	}

	operator int(){
		return value;
	}
};

/*
Class to coerce ExpressionResults into register values
*/
class Src{
	public:

	int value;

	Src(){}

	Src(int i){
		value = i;
	}

	Src(RegExpressionResult expr){
		assert(expr->isRegisterable());
		value = expr->getRegisterable()->bind();
	}

	std::string render(){
		return "r" + std::to_string((long long)value);
	}
};

/*
Class to coerce ExpressionResults into register values.
Can also take values for immediate instructions.
*/
class FlexSrc: public Src{
	public:

	bool imm = false;

	FlexSrc(int i, bool _imm = false):
		Src(i), imm(_imm)
	{}

	FlexSrc(Imm i):
		Src(i.value), imm(true)
	{}

	FlexSrc(ExpressionResult expr){
		if(expr->isRegisterable()){
			value = expr->getRegisterable()->bind();
		}
		else{
			imm = true;
			value = expr->getValue();
		}
	}
	FlexSrc(RegExpressionResult expr){
		value = expr->getRegisterable()->bind();
	}

	std::string render(){
		if (!imm) return Src::render();
		return "#" + std::to_string((long long)value);
	}

	operator int(){
		return value;
	}
};

class MoveBlock: public RegisterResultBlock
{
	FlexSrc src;
	Flag condCode;

public:
	MoveBlock(Dest d, FlexSrc _src, Flag co = NONE):
		RegisterResultBlock(d), src(_src), condCode(co)
	{}
	


	std::string format()
	{
		std::string out = std::string("    MOV");
		out += flagToStr(condCode);
		
		out += " r" + std::to_string((long long)regD);
		out += ", ";
		out += src.render();
		return out;
	}

};

class BranchBlock: public CodeBlock
{
	std::string label;
	Flag condCode;

public:
	BranchBlock(std::string l, Flag co = NONE):
		label(l), condCode(co)
	{}
	


	std::string format()
	{
		std::string out = std::string("    B");
		out += flagToStr(condCode) + " ";
		
		out += label;
		return out;
	}

};

class CMPBlock: public CodeBlock
{
	Src op1;
	FlexSrc op2;

public:
	CMPBlock(Src a, FlexSrc b):
		op1(a),op2(b)
	{}

	std::string format()
	{
		std::string out = std::string("    CMP ") + op1.render();
		out += ", ";
		out += op2.render();
		return out;
	}

};

class AndBlock: public RegisterResultBlock
{
	Src op1;
	FlexSrc op2;
public:
	AndBlock(Dest d, Src p1, FlexSrc p2):
		RegisterResultBlock(d), op1(p1), op2(p2)
	{}
	std::string format()
	{
		std::string out = std::string("    AND ") + op1.render();
		out += ", ";
		out += op2.render();
		return out;
	}
};

class OrBlock: public RegisterResultBlock
{
	Src op1;
	FlexSrc op2;
public:
	OrBlock(int d, Src p1, FlexSrc p2):
		RegisterResultBlock(d), op1(p1), op2(p2)
	{}
	std::string format()
	{
		std::string out = std::string("    OR ") + op1.render();
		out += ", ";
		out += op2.render();
		return out;
	}
};

class AddBlock: public RegisterResultBlock
{
	Src op1;
	FlexSrc op2;

public:
	AddBlock(Dest d, Src _0, FlexSrc _1):
		RegisterResultBlock(d), op1(_0), op2(_1)
	{}

	void setOp2(FlexSrc i){
		op2 = i;
	}

	std::string format()
	{
		if(op2.imm && op2.value == 0 && regD == op1.value)
		{
			throw CodeAbortException();
		}
		std::string out = std::string("    ADD r") + std::to_string((long long)regD) + ", " + op1.render();
		out += ", ";
		out += op2.render();
		return out;
	}	
};

class SubBlock: public RegisterResultBlock
{
	Src op1;
	FlexSrc op2;

public:
	SubBlock(Dest d, Src _0, FlexSrc _1):
		RegisterResultBlock(d), op1(_0), op2(_1)
	{}

	void setOp2(FlexSrc i){
		op2 = i;
	}

	std::string format()
	{
		std::string out = std::string("    SUB r") + std::to_string((long long)regD) + ", " + op1.render();
		out += ", ";
		out += op2.render();
		return out;
	}	
};

class RSBBlock: public RegisterResultBlock
{
	Src op1;
	FlexSrc op2;

public:
	RSBBlock(Dest d, Src _0, FlexSrc _1):
		RegisterResultBlock(d), op1(_0), op2(_1)
	{}

	void setOp2(FlexSrc i){
		op2 = i;
	}

	std::string format()
	{
		std::string out = std::string("    RSB r") + std::to_string((long long)regD) + ", " + op1.render();
		out += ", ";
		out += op2.render();
		return out;
	}	
};

class StackPushPop: public CodeBlock
{
	bool push;
	std::vector<int64_t> loc;
	
	public:
	StackPushPop(std::vector<int64_t> l, bool p):
		push(p), loc(l)
	{}
	
	std::string format()
	{
		if (loc.size()==0)throw CodeAbortException();
		
		std::string out = "";
		if(push) out += std::string("    STMFD sp!, {r");
		else out += std::string("    LDMFD sp!, {r");
		
		out += std::to_string((long long)loc[0]);
		
		for(size_t i=1; i<loc.size(); i++)
		{
			out += ", r" + std::to_string((long long)loc[i]);
		}
		out += "}";
		return out;
	}
};

class StackOp: public CodeBlock
{
	bool push;
	int reg;
	int offset;

public:
	StackOp(bool p, int r, int o):
		push(p), reg(r), offset(o)
	{}

	std::string format()
	{
		std::string out = "";
		if(push) out += std::string("    STR r");
		else out += std::string("    LDR r");
		out += std::to_string((long long)reg) + ", [fp, #" + std::to_string((long long)offset) + "]";
		return out;
	}

};

class LabelBlock: public CodeBlock
{
	std::string label;

public:
	LabelBlock(int i){
		label = std::to_string((long long)i);
	}
	
	LabelBlock(std::string s):
		label(s)
	{}
	
	std::string format()
	{
		return label + ":";
	}

};

class BBlock: public CodeBlock
{
	std::string label;
	bool link;
	
public:
	
	BBlock(std::string s, bool l):
		link(l)
	{
		label = s;
	}
	
	BBlock(int s, bool l):
		link(l)
	{
		label = "r" + std::to_string((long long) s);
	}
	
	std::string format()
	{
		return std::string("    B") + (link?"L ":" ") + label;
	}


};

class TextBlock: public CodeBlock
{
	std::string label;
public:
	TextBlock(std::string s):
		label(s)
	{}
	
	std::string format()
	{
		return label;
	}

};

class CodeGen
{
	static std::vector<CodeBlock*> blocks;
	
	public:
		static void push(CodeBlock* p);
		static void pushBegin(CodeBlock* p);

		static void format(std::ostream& output){
			
			for(size_t i=0; i<blocks.size(); i++){
				try{
					output << blocks[i]->format() << "\n";
				}
				catch (CodeAbortException e)
				{}
			}
		}

		static bool rebindResultReg(int r);
};

#endif

