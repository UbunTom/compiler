#include "Tree.h"
#include "Branches.h"
#include <tuple>
#include "CodeGen.hpp"

ScopedVariable::ScopedVariable(Declarator* d, TypeSpec* t)
{
	type = t;
	auto ptr_dd = d->getData();
	ptr = std::get<0>(ptr_dd);
	auto ddb = dynamic_cast<DirectDeclaratorBase*>(std::get<1>(ptr_dd));
	assert(ddb != NULL);
	
	identifier = ddb->format();
}

ScopedVariable::ScopedVariable(Declarator* d, TypeSpec* t, int i):
	Registerable(i)
{
	
	type = t;
	auto ptr_dd = d->getData();
	ptr = std::get<0>(ptr_dd);
	auto ddb = dynamic_cast<DirectDeclaratorBase*>(std::get<1>(ptr_dd));
	assert(ddb != NULL);
	
	identifier = ddb->format();
}

int Registerable::getStackLocation(){
	if(!stackAllocated){
		int allocSize = getSize();
		stackLocation = StackStore::allocate(allocSize);
		stackAllocated = true;
	}
	return stackLocation;
}

int Registerable::bind(){
	RegAlloc::bindReg(this);
	return regLoc;
}

void Registerable::use(int r)
{
	assert(!unregistered);
	regLoc = r;
	
	if(!used)
	{
		used = true;
	}
	else if(!inReg)
	{
		CodeGen::push(new StackOp(false, r, getStackLocation()));
	}

	inReg = true;

}

void Registerable::store()
{
	inReg = false;
	CodeGen::push(new StackOp(true, getReg(), getStackLocation()));
}

void Registerable::unregister()
{
	unregistered = true;
	if(unregistered) return;
	inReg = false;
	RegAlloc::freeReg(regLoc);
}

TemporaryValue::TemporaryValue()
{
	RegAlloc::bindReg(this);
}
TemporaryValue::TemporaryValue(int reg)
{
	RegAlloc::bindReg(this, reg);
}
TemporaryValue::~TemporaryValue(){
	if (inReg){
		RegAlloc::freeReg(regLoc);
	}
}


int ScopedVariable::getSize()
{
	return 4;
}

std::string ScopedVariable::typeString()
{
	return type->format() + (ptr ? ptr->format() : "");

}	
