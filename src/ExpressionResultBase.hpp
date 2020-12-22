#ifndef EXPRESSIONRESULTBASE_H
#define EXPRESSIONRESULTBASE_H

#include "Tree.h"

enum ResultLocation
{
	REG,
	CONST,
	FLAGS,
	LITERAL,
	ADDRESS,
};


class ExpressionResultHolder;
class RegisterableResult;
typedef std::shared_ptr<ExpressionResultHolder> ExpressionResult;
typedef std::shared_ptr<RegisterableResult> RegExpressionResult;


class ExpressionResultHolder
{
	public:
	ExpressionResultHolder(ResultLocation _loc):loc(_loc){}
	ResultLocation loc;
	// 
	virtual int getValue() = 0;

    bool isRegisterable(){
        return (loc == REG);
    }

	// Convert the result to a temp value
	virtual RegExpressionResult toRegisterable() = 0;
	virtual Registerable* getRegisterable();
};


class RegisterableResult: public ExpressionResultHolder{
    protected:
    RegisterableResult():
        ExpressionResultHolder(REG)
    {}
};


#endif