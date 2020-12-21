#ifndef CODEGEN_H
#define CODEGEN_H

#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <exception>

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



class MoveBlock: public RegisterResultBlock
{
	int regimm;
	bool usingImm;
	Flag condCode;

public:
	MoveBlock(int d, int _0, bool us, Flag co = NONE):
		RegisterResultBlock(d),regimm(_0), usingImm(us), condCode(co)
	{}
	


	std::string format()
	{
		std::string out = std::string("    MOV");
		out += flagToStr(condCode);
		
		out += " r" + std::to_string((long long)regD);
		if(usingImm) out += ", #";
		else out += ", r";
		out += std::to_string((long long)regimm);
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
	int reg1;
	int reg2imm;
	bool usingImm;

public:
	CMPBlock(int d, int _0, bool us):
		reg1(d),reg2imm(_0), usingImm(us)
	{}
	


	std::string format()
	{
		std::string out = std::string("    CMP r") + std::to_string((long long)reg1);
		if(usingImm) out += ", #";
		else out += ", r";
		out += std::to_string((long long)reg2imm);
		return out;
	}

};

class AndBlock: public RegisterResultBlock
{
	int reg1;
	int reg2imm;
	bool usingImm;
public:
	AndBlock(int d, int p1, int p2, bool us):
		RegisterResultBlock(d), reg1(p1),reg2imm(p2), usingImm(us)
	{}
	std::string format()
	{
		std::string out = std::string("    AND r") + std::to_string((long long)reg1);
		if(usingImm) out += ", #";
		else out += ", r";
		out += std::to_string((long long)reg2imm);
		return out;
	}
};

class OrBlock: public RegisterResultBlock
{
	int reg1;
	int reg2imm;
	bool usingImm;
public:
	OrBlock(int d, int p1, int p2, bool us):
		RegisterResultBlock(d), reg1(p1),reg2imm(p2), usingImm(us)
	{}
	std::string format()
	{
		std::string out = std::string("    OR r") + std::to_string((long long)reg1);
		if(usingImm) out += ", #";
		else out += ", r";
		out += std::to_string((long long)reg2imm);
		return out;
	}
};

class AddBlock: public RegisterResultBlock
{
	int reg0;
	int reg1imm;
	bool usingImm;

public:
	AddBlock(int d, int _0, int _1, bool us):
		RegisterResultBlock(d), reg0(_0), reg1imm(_1), usingImm(us)
	{}

	void setImm(int i){
		usingImm = true;
		reg1imm = i;
	}

	std::string format()
	{
		if(usingImm && reg1imm == 0 && regD == reg0)
		{
			throw CodeAbortException();
		}
		std::string out = std::string("    ADD r") + std::to_string((long long)regD) + ", r" + std::to_string((long long)reg0);
		if(usingImm) out += ", #";
		else out += ", r";
		out += std::to_string((long long)reg1imm);
		return out;
	}	
};

class SubBlock: public RegisterResultBlock
{
	int reg0;
	int reg1imm;
	bool usingImm;

public:
	SubBlock(int d, int _0, int _1, bool us):
		RegisterResultBlock(d), reg0(_0), reg1imm(_1), usingImm(us)
	{		
	}

	void setImm(int i){
		usingImm = true;
		reg1imm = i;
	}

	std::string format()
	{
		std::string out = std::string("    SUB r") + std::to_string((long long)regD) + ", r" + std::to_string((long long)reg0);
		if(usingImm) out += ", #";
		else out += ", r";
		out += std::to_string((long long)reg1imm);
		return out;
	}	
};

class RSBBlock: public RegisterResultBlock
{
	int reg0;
	int reg1imm;
	bool usingImm;

public:
	RSBBlock(int d, int _0, int _1, bool us):
		RegisterResultBlock(d), reg0(_0), reg1imm(_1), usingImm(us)
	{}

	void setImm(int i){
		usingImm = true;
		reg1imm = i;
	}

	std::string format()
	{
		std::string out = std::string("    RSB r") + std::to_string((long long)regD) + ", r" + std::to_string((long long)reg0);
		if(usingImm) out += ", #";
		else out += ", r";
		out += std::to_string((long long)reg1imm);
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

