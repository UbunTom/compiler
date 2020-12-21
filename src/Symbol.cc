#include "Symbol.h"

void SymbolStore::push(std::string s)
{
	store.push_back(s);
}

std::string SymbolStore::pop()
{
	auto s = store.back();
	store.pop_back();
	return s;
}


void SymbolStore::printTop()
{
	std::cout << store.back() << std::endl;
}

std::vector<std::string> SymbolStore::store;
