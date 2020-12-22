
#include "ExpressionResultBase.hpp"


class FlagsResult: public ExpressionResultHolder{
	Flag flag;

	public:
	FlagsResult(Flag f):
		ExpressionResultHolder(FLAGS),
		flag(f)
	{}

    Flag getFlag(){return flag;}
	int getValue(){throw CompilerError("Flags cannot be used directly");}

	RegExpressionResult toRegisterable();
};

class LiteralResult: public ExpressionResultHolder{
	std::string literal;

	public:
	LiteralResult(std::string l):
		ExpressionResultHolder(LITERAL),
		literal(l)
	{}

	int getValue(){throw CompilerError("Literal cannot be used directly");}

	RegExpressionResult toRegisterable();
};

class ConstResult: public ExpressionResultHolder{
	int value;

	public:
	ConstResult(int v):
		ExpressionResultHolder(CONST),
		value(v)
	{}

	int getValue(){return value;}

	RegExpressionResult toRegisterable();
};

class TempResult: public RegisterableResult{
	std::shared_ptr<TemporaryValue> temp;

	public:
	TempResult(std::shared_ptr<TemporaryValue>&& _tval):
		temp(std::move(_tval))
	{}

	TempResult(std::shared_ptr<TemporaryValue>& _tval):
		temp(_tval)
	{}

	int getValue(){
		return temp->bind();
	}

    Registerable* getRegisterable(){
        return temp.get();
    }

	RegExpressionResult toRegisterable();
};

class VarResult: public RegisterableResult{
	ScopedVariable* var;

	public:

	VarResult(ScopedVariable* _var):
		var(_var)
	{}

	int getValue(){
		return var->bind();
	}

    Registerable* getRegisterable(){
        return var;
    }

	RegExpressionResult toRegisterable();
};