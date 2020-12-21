#include <string>
#include <vector>

class LabelAlloc
{
	static int labelCount;
	
public:
	static std::string allocate(bool mode = true)
	{
		return (mode?std::string(".L"):"") + std::to_string((long long)labelCount++);
	}
	
};

class LoopLabelJump
{
	static std::string _return;
	static std::vector<std::string> _break;
	static std::vector<std::string> _continue;
	
public:
	static void push(std::string b, std::string c)
	{
		_break.push_back(b);
		_continue.push_back(c);
	}
	static void pop()
	{
		_break.pop_back();
		_continue.pop_back();
	}
	
	static std::string getBreak(){return _break.back();}
	static std::string getContinue(){return _continue.back();}
	
	static void setReturn(std::string s){_return = s;}
	static std::string getReturn(){return _return;}
	
};
