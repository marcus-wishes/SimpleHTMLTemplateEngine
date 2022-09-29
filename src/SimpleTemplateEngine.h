#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>


namespace SimpleTemp{

	//future plans--- exchange the hardcoded elements with PaserFunction instances
	/*struct ParserFunction
	{
		std::string m_RegexEl;
		std::string m_TxtEl;
		std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)>	m_Function;
	};*/





	class Template
	{
	public:
		Template();
		virtual ~Template();

		
		void												FillTemplateFile(std::string templateFile, std::string outFile);
		std::unique_ptr<std::string>&						CreateString(std::unique_ptr<std::string> content);
		void												SetTemplateFile(std::string templateFile);

		void												SetVariableDictionary(std::map<std::string, std::string> dic, bool needsToBeEscaped);
		std::map<std::string, std::string>					GetVariableDictionary();

		void												SetLoopDictionary(std::map<std::string, std::vector<std::string> > dic, bool needsToBeEscaped);
		std::map<std::string, std::vector<std::string> >	GetLoopDictionary();

		void												SetIfElseDictionary(std::map<std::string, bool> dic);
		std::map<std::string, bool>							GetIfElseDictionary();

		void												SetSwitchDictionary(std::map<std::string, std::string> dic);
		std::map<std::string, std::string>					GetSwitchDictionary();

		void												SetCustomFunctionDictionary(std::map<std::string, std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)> > dic);
		std::map<std::string, std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)> > getCustomFunctionDictionary();



	private:
		std::string																			m_FindPattern;

		std::pair<unsigned int, int>														handleElement(std::unique_ptr<std::string>& toParse, unsigned int begin, std::string element, std::string regex, std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)> eleFunc); //returns the new elements length and the length change compare to the original element length
		unsigned int																		walkElements( std::unique_ptr<std::string>& toParse, unsigned int begin, unsigned int length); //returns the new length of the currently walked part
		std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)>	m_LoopFunc;  //the parser functions also return the enw length of the content which was parsed and replaced
		std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)>	m_IfElseFunc;
		std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)>	m_SwitchFunc;
		std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)>	m_CustomFunc;

		//specific syntax elements settings:
		std::string																			m_VarOpenElement = "{{";
		std::string																			m_VarOpenRegex = "\\{\\{";
		std::string																			m_VarCloseElement = "}}";
		std::string																			m_VarCloseRegex = "\\}\\}";
		std::map<std::string, std::string>													m_VariableDic;


		std::string																			m_SectionClosingRegex = "\\/";
		std::string																			m_SectionClosingElement = "/";

		std::string																			m_LoopElement = "**";
		std::string																			m_LoopRegex   = "\\*\\*";
		std::map<std::string, std::vector<std::string> >									m_LoopDic;	
		unsigned int																		loopF(std::unique_ptr<std::string>& toParse, std::string loopName, unsigned int begin, unsigned int loopLength, unsigned int indentations, std::map<std::string, std::string> const &paras); //changes the string and returns the new length of the element after the changes
		int																					m_InNestedLoopNr;

		std::string																			m_IfElseElement = "<<";
		std::string																			m_IfElseRegex   = "\\<\\<";
		std::map<std::string, bool>															m_IfElseDic;
		unsigned int																		ifElseF(std::unique_ptr<std::string>& toParse, std::string ifElseName, unsigned int begin, unsigned int ifElseLength, unsigned int indentations, std::map<std::string, std::string> const &paras);

		std::string																			m_SwitchElement = ">>";
		std::string																			m_SwitchRegex   = "\\>\\>";
		std::map<std::string, std::string>													m_SwitchDic;
		unsigned int																		switchF(std::unique_ptr<std::string>& toParse, std::string switchName, unsigned int begin, unsigned int switchLength, unsigned int indentations, std::map<std::string, std::string> const &paras);

		std::string																			m_CustomFuncElement = "!!";
		std::string																			m_CustomFuncRegex   = "\\!\\!";
		std::map<std::string, std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)> > m_CustomFunctionDic;
		unsigned int																		customF(std::unique_ptr<std::string>& toParse, std::string customName, unsigned int begin, unsigned int custLength, unsigned int indentations, std::map<std::string, std::string> const &paras);

		unsigned int																		m_ElementLength;
	};




	//publishes actually the walkElement function "outside" so it can be used in custom function lambdas.
	extern std::function<unsigned int(std::unique_ptr<std::string>&, unsigned int, unsigned int)> parse;



	namespace Utils
	{
		std::vector<std::string>				 StringSplit(std::string const &s, const char delimiter);
		const std::map<std::string, std::string> escapeLut = {
			{ "<", "&lt"  },
			{ ">", "&gt"  },
			{ "&", "&amp" }
		};
		void									 Escape(std::string &str);
	}

}

