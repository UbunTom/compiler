#include "LabelAlloc.hpp"

int LabelAlloc::labelCount = 1;

std::vector<std::string> LoopLabelJump::_break;
std::vector<std::string> LoopLabelJump::_continue;
std::string LoopLabelJump::_return;
