#ifndef SYMBOL_H
#define SYMBOL_H

#include <iostream>
#include <string>
#include <vector>
#include <cassert>



class SymbolStore
//FIFO store for symbols
{
	static std::vector<std::string> store;
	
	public:
	static void push(std::string);
	static std::string pop();
	static void printTop();
	
	static void pop_into(std::string& t)
	{
		t = store.back();
		store.pop_back();
	}
};

#endif
