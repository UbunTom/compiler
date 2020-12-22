#include "Branches.h"

bool checkResult(ExpressionResult& exp1, ResultLocation l1)
{
	return (exp1->loc == l1);
}

bool checkResult(ExpressionResult& exp1, ExpressionResult& exp2, ResultLocation l1, ResultLocation l2)
{
	return ((exp1->loc == l1) && (exp2->loc == l2)) || ((exp1->loc == l2) && (exp2->loc == l1));
}

void orderResult(ExpressionResult& exp1, ExpressionResult& exp2, ResultLocation l1)
{
	if(exp1->loc == l1)return;
	std::swap(exp1, exp2);
}

TranslationUnit* TopBranch::tu;
