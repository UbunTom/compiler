all:
	bisonc++ grammar.y
	flexc++ grammar.l
	g++ --std=c++0x -o ../bin/compiler *cc *cpp
