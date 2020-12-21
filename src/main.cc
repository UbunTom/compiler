#include "Parser.h"
#include "Allocation.hpp"
#include "Branches.h"
#include <ostream>
#include <fstream>
#include <sstream>
#include <string.h>

TranslationUnit* topLevel;

int main(int argc, char **argv)
{
  if(argc!=5 || strcmp("-S", argv[1])!=0 || strcmp("-c", argv[2])!=0){
    std::cout << "Invalid arguments" << std::endl;
    return 1;  
  }
  
  std::ifstream input(argv[4]);
  
  if(!input){
	std::cout << "Invalid input file" << std::endl;
  }
  
  std::cin.rdbuf(input.rdbuf());
  
  
  Parser parser;

  parser.parse();
  
  if(TopBranch::get())
	TopBranch::get()->genCode();

  CodeGen::pushBegin(StringBin::createDirectives());
  
  std::ofstream outfile(argv[3]);
  
  CodeGen::format(outfile);
  
  return 0;
}
