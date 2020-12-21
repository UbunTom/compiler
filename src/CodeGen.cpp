#include "CodeGen.hpp"
#include "Allocation.hpp"

void CodeGen::push(CodeBlock* p)
{
	blocks.push_back(p);
}

void CodeGen::pushBegin(CodeBlock* p)
{
	blocks.insert(blocks.begin(),p);
}

bool CodeGen::rebindResultReg(int r)
{
	
	RegisterResultBlock* rr = dynamic_cast<RegisterResultBlock*>(blocks.back());
	if (rr == NULL) return false;
	int firstReg = rr->regD;
	if(RegAlloc::regs[rr->regD] != NULL) return false;

	rr->rebindResult(r);
	
	
	while(true)
	{
		RegisterResultBlock* rr = dynamic_cast<RegisterResultBlock*>(blocks.back());
		if (rr == NULL) return true;

		if(RegAlloc::regs[rr->regD] != NULL) return true;
		if(rr->regD != firstReg) return true;

		rr->rebindResult(r);
	
	}
}

Flag flagInvert(Flag f)
{
	switch(f){
		case EQ: return NE;
		case NE: return EQ;
		case GT: return LE;
		case LE: return GT;
		case GE: return LT;
		case LT: return GE;
		case NONE: return NEVER;
		default : assert(false);
	}
}
