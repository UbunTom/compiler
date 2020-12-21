#include "Tree.h"
#include "Branches.h"
#include <tuple>
#include "CodeGen.hpp"

ScopedVariable::ScopedVariable(Declarator* d, TypeSpec* t):
	used(false),
	inReg(false)
{
	type = t;
	auto ptr_dd = d->getData();
	ptr = std::get<0>(ptr_dd);
	auto ddb = dynamic_cast<DirectDeclaratorBase*>(std::get<1>(ptr_dd));
	assert(ddb != NULL);
	
	identifier = ddb->format();

	//Allocate a stack position
	location = StackStore::allocate(getSize());
	
}

ScopedVariable::ScopedVariable(Declarator* d, TypeSpec* t, int i):
	used(true),
	inReg(true)
{
	regLoc = i;
	
	type = t;
	auto ptr_dd = d->getData();
	ptr = std::get<0>(ptr_dd);
	auto ddb = dynamic_cast<DirectDeclaratorBase*>(std::get<1>(ptr_dd));
	assert(ddb != NULL);
	
	identifier = ddb->format();

	//Allocate a stack position
	location = StackStore::allocate(getSize());
	
	
}

void ScopedVariable::use(int r)
{
	regLoc = r;
	
	if(!used)
	{
		used = true;
	}
	else if(!inReg)
	{
		CodeGen::push(new StackOp(false, r, location));
	}

	inReg = true;

}

void ScopedVariable::store()
{
	inReg = false;
	CodeGen::push(new StackOp(true, regLoc, location));

}

int ScopedVariable::getSize()
{
	return 4;
}

std::string ScopedVariable::typeString()
{
	return type->format() + (ptr ? ptr->format() : "");

}	
