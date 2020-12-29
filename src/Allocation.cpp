#include "Tree.h"
#include "Allocation.hpp"

void RegAlloc::begin()
{
	count = 0;
	for(int i=0; i<NREGS; i++)
	{
		lastUsed[i] = 0;
		regs[i] = NULL;
	}
}

int RegAlloc::getEmptyReg()
{
	int oldest=0;
	for(int i=0; i<NREGS; i++)
	{
		if(regs[i] == NULL) return i;

		if(lastUsed[i] < lastUsed[oldest]) oldest = i;
	}
	if(regs[oldest]!=NULL)regs[oldest]->store();
	
	regs[oldest]=NULL;

	count++;
	lastUsed[oldest] = count;
	

	return oldest;
}

void RegAlloc::bindReg(Registerable* s)
{
	for(size_t i=0; i<stack.size(); i++)
	{
		stack[i].second.insert(s);
	}
	
	if(s->inReg) return;
	int r = getEmptyReg();
	s->use(r);
	regs[r] = s;
}

void RegAlloc::bindReg(Registerable* s, int r)
{
	for(size_t i=0; i<stack.size(); i++)
	{
		stack[i].second.insert(s);
	}
	
	if(s->inReg && s->regLoc == r) return;
	
	if(regs[r] != NULL)
		regs[r]->store();
		
	if(s->inReg)
		CodeGen::push(new MoveBlock(r, s->regLoc));
	else
		s->use(r);
	
	regs[r] = s;
}

// Free up a register
void RegAlloc::freeReg(int r){
	regs[r] = NULL;
}

void RegAlloc::swap(int a, int b){
	Registerable* one = regs[a];
	if(one)one->use(b);
	Registerable* two = regs[b];
	if(two)two->use(a);
	
	regs[a] = two;
	regs[b] = one;
	
	std::swap(lastUsed[a], lastUsed[b]);
}

void RegAlloc::print()
{
	for(int i=0; i<NREGS; i++)
	{
		std::cout << i;
		if(regs[i]!=NULL){
			std::cout << regs[i]->getNameInfo() << " " << regs[i]->regLoc;
		}
		std::cout << std::endl;
	}
}

/*
Restore the internal state of the registers to the provided snapshot.
This function will not generate code to store/load stack state.
*/
void RegAlloc::restoreSnapshot(std::array<Registerable*, NREGS> snap){
	for(int i=0; i<NREGS; i++){
		if(regs[i] != nullptr){
			regs[i]->restore();
			regs[i] = nullptr;
		}
	}
	for(int i=0; i<NREGS; i++){
		if(snap[i] != nullptr){
			snap[i]->restore(i);
			regs[i] = snap[i];
		}
	}
}

/*
void RegAlloc::pushState()
{
	stack.push_back(std::make_pair(regs, std::set<ScopedVariable*>()));
}

void RegAlloc::popState(Branch* loop)
{
	auto popped = stack.back();
	std::array<ScopedVariable*, 8> checked = popped.first;
	std::set<ScopedVariable*> used = popped.second;
	stack.pop_back();
	
	for(int i=0; i<8; i++)
	{
		if(checked[i] == NULL)continue;
		if(checked[i]->inReg && checked[i]->regLoc == i) continue;	//Variable is still in the correct register
		if(used.find(checked[i]) == used.end()) continue;	//The variable was not used so can be ignored
		
		if(regs[i] == NULL)
		{
			checked[i]->use(i);
			regs[i] = checked[i];
			continue;
		}
		ScopedVariable* inTheWay = NULL;
		try{
			inTheWay = dynamic_cast<ScopedVariable*>(loop->getScope(regs[i]->name()));
		}
		catch (NotInScopeException e)
		{}
		if (inTheWay == regs[i])	//The variable in reg i is not temporary and can't be overwritten
		{
			regs[i]->store();
		}
		else
		{
			checked[i]->use(i);
			regs[i] = checked[i];
		}
		
	}
}
*/
void RegAlloc::store(int i){
	if(regs[i]!=NULL){
		regs[i]->store();
		regs[i] = NULL;
	}
}

void RegAlloc::storeAll(int from)
{
	for(int i=from; i<NREGS; i++)
	{
		store(i);
	}
}

void RegAlloc::loadAll(int from)
{
	for(int i=from; i<NREGS; i++)
	{
		if(regs[i]!=NULL)regs[i]->use(i);
	}
}

std::string StringBin::intToLabel(int label){
	std::string stLabel = ".literal_" + std::to_string((long long) label);
	return stLabel;
}

std::string StringBin::newLiteral(std::string text)
{
	text = text.substr(1,text.length()-2);

	decltype(stringMap)::iterator it;
	if ((it = stringMap.find(text)) == StringBin::stringMap.end()){
		int index = StringBin::stringMap.size();
		stringMap[text] = index;
	}

	int index = stringMap[text];
	
	return intToLabel(index);
}

TextBlock* StringBin::createDirectives(){

	std::string header = ".section .rodata\n";
	for (auto it = stringMap.begin(); it != stringMap.end(); it++)
	{
		int index = it->second;
		std::string stLabel = intToLabel(index);
		std::string text = it->first;
		std::string code = stLabel + ":\n.asciz \"" + text + "\"\n";
		header += code;
	}
	header += ".text\n";

	return new TextBlock(header);
}

decltype(RegAlloc::stack) RegAlloc::stack;
decltype(RegAlloc::regs) RegAlloc::regs;
int RegAlloc::lastUsed[NREGS];
int RegAlloc::count;
std::map<std::string, int> StringBin::stringMap;