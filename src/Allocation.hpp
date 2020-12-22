#ifndef ALLOCATION_H
#define ALLOCATION_H

#include <cmath>
#include "CodeGen.hpp"
#include <cassert>
#include <vector>
#include <array>
#include <set>
#include <map>

/*
0 - 3	pass params. Overwritten by subroutines
4 - 11 	preserved. Store and restore in subroutine
12	intra-procedure scrtch
13	stack pointer
14	link reg
15	pc
*/

/* Function call:
First 4 words are passed via r0-3
(Copied to r4-7)
Anything extra must be passed by stack
*/

class StackScope
{
	int framePos;
	int allocAmt;
	SubBlock* incStack;
public:
	
	StackScope(int framePos){
		allocAmt = 4;
		incStack = new SubBlock(13,13,0,true);
		CodeGen::push(incStack);
	}

	int allocate(int size)
	{
		int s = 4*ceil(size/4.0);
		int ret = allocAmt;
		allocAmt+=s;
		return -ret;
	}

	~StackScope(){
		incStack->setImm(allocAmt);
		CodeGen::push(new AddBlock(13,13,allocAmt,true));
	}
	
	int getEnd()
	{
		return framePos + allocAmt;
	}
	

};

class StackStore
{
	static std::vector<StackScope*> scopes;
	
	public:
	static void beginFunc(){
		scopes.push_back(new StackScope(0));
	}
	
	static void begin(){
		//scopes.push_back(new StackScope(scopes.back()->getEnd()));
	}
	
	static int allocate(int size)
	{
		return scopes.back()->allocate(size);
	}
	
	static void end(){
		/*delete scopes.back();
		scopes.pop_back();
		assert(scopes.size() != 0);*/
	}
	
	static void endFunc(){
		delete scopes.back();
		scopes.pop_back();
		
		assert(scopes.size() == 0);
	}
	

};

class ScopedVariable;
class Branch;
class RegAlloc
{
	static std::vector<std::pair<std::array<ScopedVariable*, 8>, std::set<ScopedVariable*> > > stack;
	friend class CodeGen;
	protected:
	static std::array<ScopedVariable*, 8> regs;
	static int lastUsed[8];
	static int count;
	
	public:
	static void swap(int,int);
	static void begin();
	static int getEmptyReg();
	static void bindReg(ScopedVariable* s, int r);
	static void bindReg(ScopedVariable* s);
	static void freeReg(int i);
	static int getScratch(){return 12;}
	
	static void print();
	
	static void pushState();
	static void popState(Branch*);
	
	static void store(int);
	static void storeAll(int = 0);
	static void loadAll(int = 0);

};


class StringBin
{
	static std::map<std::string, int> stringMap;
	static std::string intToLabel(int);
public:
	static std::string newLiteral(std::string);
	static TextBlock* createDirectives();

	
};


#endif