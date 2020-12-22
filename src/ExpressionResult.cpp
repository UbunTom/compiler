#include "CodeGen.hpp"
#include "ExpressionResultTypes.hpp"


Registerable* ExpressionResultHolder::getRegisterable(){
    return toRegisterable()->getRegisterable();
};



RegExpressionResult FlagsResult::toRegisterable(){
    auto tempval = TemporaryValue::create();
    int reg = tempval->getReg();
    CodeGen::push(new MoveBlock(reg, FlexSrc(0, true)));
    CodeGen::push(new MoveBlock(reg, FlexSrc(1, true), flag));
    return std::make_shared<TempResult>(tempval);
}

RegExpressionResult LiteralResult::toRegisterable(){
    auto tempval = TemporaryValue::create();
    int reg = tempval->getReg();
    CodeGen::push(new TextBlock("    LDR r" + std::to_string(reg) + ", =" + literal));
    return std::make_shared<TempResult>(tempval);
}

RegExpressionResult ConstResult::toRegisterable(){
    auto tempval = TemporaryValue::create();
    int reg = tempval->getReg();
    CodeGen::push(new MoveBlock(reg, FlexSrc(value, true)));
    return std::make_shared<TempResult>(tempval);
}

RegExpressionResult TempResult::toRegisterable(){
    return std::make_shared<TempResult>(temp);
}

RegExpressionResult VarResult::toRegisterable(){
    return std::make_shared<VarResult>(var);
}