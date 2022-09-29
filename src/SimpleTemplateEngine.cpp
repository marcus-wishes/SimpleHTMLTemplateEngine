#include "SimpleTemplateEngine.h"
#include <sstream>
#include <fstream>
#include <regex>

using namespace SimpleTemp;
typedef std::unique_ptr<std::string> StringUPtr;
typedef std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)> ParserFunc;
typedef unsigned int uint;


std::function<unsigned int(std::unique_ptr<std::string>&, unsigned int, unsigned int)>  SimpleTemp::parse;// = std::function<unsigned int(std::unique_ptr<std::string>&, unsigned int, unsigned int)>();

//hack to avoid ambiguous regex_constants when mixing with QRegExp and to not recreate the std::regex used in walkElements
std::regex findPattern;

Template::Template()
	: m_InNestedLoopNr(-1)
{
	m_LoopFunc   = std::bind(&Template::loopF,   this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
	m_IfElseFunc = std::bind(&Template::ifElseF, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
	m_SwitchFunc = std::bind(&Template::switchF, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
	m_CustomFunc = std::bind(&Template::customF, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
}



Template::~Template()
{
}


void Template::FillTemplateFile(std::string templateFile, std::string outFile)
{
	//load file
	std::ifstream isf;
	isf.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		isf.open(templateFile);
	}
	catch (std::exception e)
	{
		throw "Error reading the template file: " + templateFile;
	}
	StringUPtr toParse(new std::string());
	isf.seekg(0, std::ios::end);
	toParse->reserve(isf.tellg());
	isf.seekg(0, std::ios::beg);
	toParse->assign((std::istreambuf_iterator<char>(isf)), std::istreambuf_iterator<char>());

	//check that all syntax elements are from the same size
	m_ElementLength = m_VarOpenElement.length();
	if (m_VarCloseElement.length() != m_ElementLength
		|| m_LoopElement.length() != m_ElementLength
		|| m_SwitchElement.length() != m_ElementLength
		|| m_IfElseElement.length() != m_ElementLength
		|| m_CustomFuncElement.length() != m_ElementLength)
		throw std::string("Syntax element lengths are not the same. This needs to be corrected in the SimpleTemplateEngine header file. All syntax elements need to be of the same length (only the \"element\" versions, not the regex ones).");

	//create the element finding regex expression
	//m_FindPattern = std::regex("("+m_VarOpenRegex+")|(\\t*" + m_LoopRegex + ")|(\\t*" + m_IfElseRegex + ")|(\\t*" + m_SwitchRegex + ")|(\\t*" + m_CustomFuncRegex + ")");
	m_FindPattern = std::string("(" + m_VarOpenRegex + ")|(\\t*" + m_LoopRegex + ")|(\\t*" + m_IfElseRegex + ")|(\\t*" + m_SwitchRegex + ")|(\\t*" + m_CustomFuncRegex + ")");
	findPattern = std::regex(m_FindPattern);

	

	//set the parse function to be accessed in custom functions
	SimpleTemp::parse = std::bind(&Template::walkElements, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

	//start parsing
	walkElements(toParse, 0, toParse->length());

	//write output file
	std::ofstream of;
	of.open(outFile, std::ofstream::out | std::ofstream::trunc);
	of << *toParse;
	of.flush();
	of.close();
}

StringUPtr& Template::CreateString(StringUPtr cont)
{
	StringUPtr toParse = std::move(cont);
	walkElements(toParse, 0, toParse->length());
	return toParse;
}




void Template::SetVariableDictionary(std::map<std::string, std::string> dic, bool needsToBeEscaped)
{
	if (needsToBeEscaped)
		for (std::map<std::string, std::string>::iterator it = dic.begin(); it != dic.end(); ++it)
			Utils::Escape(it->second);
	m_VariableDic = dic;
}


std::map<std::string, std::string> Template::GetVariableDictionary()
{
	return m_VariableDic;
}


std::map<std::string, std::vector<std::string> > Template::GetLoopDictionary()
{
	return m_LoopDic;
}



void Template::SetLoopDictionary(std::map<std::string, std::vector<std::string> > dic, bool needsToBeEscaped)
{
	if (needsToBeEscaped)
		for (std::map<std::string, std::vector<std::string>>::iterator it = dic.begin(); it != dic.end(); ++it)
			for (std::vector<std::string>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
				Utils::Escape(*it2);
	m_LoopDic = dic;
}

std::map<std::string, bool> Template::GetIfElseDictionary()
{
	return m_IfElseDic;
}


void Template::SetIfElseDictionary(std::map<std::string, bool> dic)
{
	m_IfElseDic = dic;
}


std::map<std::string, std::string> Template::GetSwitchDictionary()
{
	return m_SwitchDic;
}


void Template::SetSwitchDictionary(std::map<std::string, std::string> dic)
{
	m_SwitchDic = dic;
}


void Template::SetCustomFunctionDictionary(std::map<std::string, ParserFunc> dic)
{
	m_CustomFunctionDic = dic;
}


std::map<std::string, ParserFunc> Template::getCustomFunctionDictionary()
{
	return m_CustomFunctionDic;
}



uint Template::walkElements(StringUPtr& toParse, uint begin, uint length)
{
	std::sregex_iterator it(toParse->begin() + begin, toParse->begin() + begin + length, findPattern), end;
	std::string elefound = "";
	std::pair<uint, int> lengthParas(0, 0); //first == accumulated new length sum, second  == accumulated length change
	uint currPos = 0;

	while (it != end)
	{
		elefound = (*it)[0];
		uint elepos = begin + currPos + it->position();
		std::string found = elefound.substr(elefound.length() - m_ElementLength, m_ElementLength);
		if (found == m_VarOpenElement)
		{
			int endpos = toParse->find("}}", elepos);
			if (endpos == std::string::npos)
				throw "Variable: No closing "+m_VarCloseElement+" of a variable declaration found.";
			int oldLength = endpos + 2 - elepos;
			std::string var = toParse->substr(elepos + m_ElementLength, endpos - elepos - m_ElementLength);
			bool indic = m_VariableDic.find(var) != m_VariableDic.end();
			if (indic == false)
				throw "Variable " + var + ": Variable not found in the variable dictionary.";
			std::string val = m_VariableDic[var];
			toParse->replace(elepos, oldLength, val);
			lengthParas.first  = val.length();
			lengthParas.second = val.length() - oldLength;
		}
		else if (found == m_LoopElement)
		{
			++m_InNestedLoopNr;
			lengthParas = handleElement( toParse, elepos, m_LoopElement, m_LoopRegex, m_LoopFunc);
			--m_InNestedLoopNr;
		}
		else if (found == m_IfElseElement)
			lengthParas = handleElement( toParse, elepos, m_IfElseElement, m_IfElseRegex, m_IfElseFunc);
		else if (found == m_SwitchElement)
			lengthParas = handleElement(toParse, elepos, m_SwitchElement, m_SwitchRegex, m_SwitchFunc);
		else if (found == m_CustomFuncElement)
			lengthParas = handleElement(toParse, elepos, m_CustomFuncElement, m_CustomFuncRegex, m_CustomFunc);

		length += lengthParas.second; //changes the length
		currPos += it->position() + lengthParas.first;
		it = std::sregex_iterator(toParse->begin() + begin + currPos, toParse->begin() + begin + length, findPattern);
	}

	if (elefound == "")
		return  length;
	else
		return length;
}


                                                                                                  
std::pair<uint, int> Template::handleElement(StringUPtr& toParse, uint begin, std::string element, std::string regex, ParserFunc eleFunc)
{
	//check if the found element is a closing one which should not be
	char c = toParse->at(begin + element.length());
	if (c == m_SectionClosingElement.at(0))
		throw "Found just a closing of an " + element + " section, missing its opening!";

	//find element end
	uint indentation = 0;
	c = toParse->at(begin + indentation);
	while (toParse->at(begin + indentation) == '\t'){
		c = toParse->at(begin + indentation);
		++indentation;
	}	
	std::regex endpatr = std::regex("\\n\\t{0," + std::to_string(indentation) + "}" + regex + m_SectionClosingRegex);
	std::sregex_iterator it = std::sregex_iterator(toParse->begin() + begin, toParse->end(), endpatr), end;
	if (it == end)
		throw "Did not find the closing of an " + element + " element.";


	uint endpos = begin + it->position();
	//remove the line of the endpos
	++endpos;
	int endlinepos = toParse->find('\n', endpos);
	if (endlinepos == std::string::npos)
		endlinepos = toParse->length();
	else
		++endlinepos;
	toParse->erase(endpos, endlinepos - endpos);

	//this is the original length of the element:
	uint oldLength = endlinepos - begin;

	//get the element name
	std::string eleName = toParse->substr(begin, toParse->find('\n', begin) - begin);
	int lengthToErase = eleName.length() + 1;
	toParse->erase(begin, lengthToErase);
	endpos -= (lengthToErase);
	uint p = eleName.find(element) + element.length();
	eleName = eleName.substr(p , eleName.length() - p);

	//parse and extract eventual parameters:
	int paraBegin = eleName.find("[");
	std::map<std::string, std::string> eleParas;
	if (paraBegin != std::string::npos)
	{
		int i = 1;
		while (isspace(eleName[paraBegin - i - 1]) != 0)
			++i;
		int paramEnd = eleName.find("]", paraBegin);
		if (paramEnd == std::string::npos)
			throw "Could not find the end of parameters for " + eleName+".";
		std::string eleParams = eleName.substr(paraBegin + 1, eleName.length() - (paraBegin + 1) - 1);
		eleName = eleName.substr(0, paraBegin - i);
		std::vector<std::string> paramsStr = Utils::StringSplit(eleParams, ';');
		for each (std::string s in paramsStr)
		{
			std::vector<std::string> pair = Utils::StringSplit(s, '=');
			eleParas[pair[0]] = pair[1];
		}
	}


	//the content of the element inside the element closure
	uint eleLength = endpos - begin;

	//error check:
	if (eleName == "")
		throw "No element name found for an " + element + " element.";

	//THIS element specific handling:
	uint newEleLength = eleFunc(toParse, eleName, begin, eleLength, indentation, eleParas);
	return std::pair<uint, int>(newEleLength, newEleLength - oldLength);
}


uint Template::loopF(StringUPtr& toParse, std::string loopName, uint begin, uint length, uint indentations, std::map<std::string, std::string> const &paras)
{
	//check if the parameter is set that the inner part of the loop needs repeatedly be parsed
	bool repeatatlyParseInner = false;
	std::map<std::string, std::string>::const_iterator par = paras.find("repeatParsing");
	if (par != paras.cend())
		repeatatlyParseInner = (par->second == "true") ? true : false;

	//check if the loopname has a parameter value which overwrites the one in the data dictionary
	std::vector<std::string> loopVariables;
	par = paras.find(loopName);
	if (par != paras.cend())
	{
		//create the new list
		std::string list = par->second;
		loopVariables = Utils::StringSplit(list, ',');
	}
	else 
	{
		if (m_LoopDic.find(loopName) == m_LoopDic.end())
			throw "Loop " + loopName + ": loop not found in loop dictionary.";
		loopVariables = m_LoopDic[loopName];
	}

	//for additional loop lists:
	std::vector<std::string> addLoopLists;
	par = paras.find("additionalLists");
	if (par != paras.cend())
	{
		std::string list = par->second;
		addLoopLists = Utils::StringSplit(list, ',');
	}
	std::map<std::string, std::vector<std::string> > addLoopListValues;
	for each (std::string add in addLoopLists)
	{
		par = paras.find(add);
		if (par != paras.cend())
		{
			std::vector<std::string> values = Utils::StringSplit(par->second, ',');
			addLoopListValues[add] = values;
		}
	}

	//if the parameter to repeatedly parse the loop contenten is not set we parse it now
	if (repeatatlyParseInner == false)
		length = walkElements(toParse, begin, length);

	//remove the original content from the string
	std::string acc = std::to_string(m_InNestedLoopNr);
	std::unique_ptr<std::string> content(new std::string(toParse->substr(begin, length)));
	toParse->erase(begin, length);

	//are there any replacements to do in the first place?
	uint offset = begin + length;
	uint l = toParse->length();
	std::string regpat = "\\#\\#(counter" + acc + "|value" + acc;
	for each(std::string list in addLoopLists)
		regpat += "|" + list;
	regpat += ")\\#\\#";
	std::regex accessor = std::regex(regpat);
	std::sregex_iterator it(content->begin(), content->end(), accessor), end;
	bool hasAccessors = false;
	if (it != end)
		hasAccessors = true;

	//no accessor found, we can just repeat the content
	if (hasAccessors == false)
	{
		for (unsigned int i = 0; i < loopVariables.size(); ++i)
			toParse->insert(begin, *content);
		return content->length() * loopVariables.size();
	}
	else
	{
		uint newLength = 0;
		for (int i = loopVariables.size() - 1; i >= 0; --i)
		{
			std::unique_ptr<std::string> s(new std::string(*content));
			std::sregex_iterator it(s->begin(), s->end(), accessor), end;
			unsigned int pos = 0;
			while (it != end)
			{
				std::string a = (*it)[0];
				std::string acc = (*it)[1];
				if (acc == "counter" + std::to_string(m_InNestedLoopNr))
					a = std::to_string(i);
				else if (acc == "value" + std::to_string(m_InNestedLoopNr))
					a = loopVariables[i];
				else if (addLoopListValues.find(acc) != addLoopListValues.end())
				{
					if (addLoopListValues[acc].size() > i)
						a = addLoopListValues[acc][i];
				}
				else if (m_LoopDic.find(acc) != m_LoopDic.end())
				{
					if (m_LoopDic[acc].size() > i)
						a = m_LoopDic[acc][i];
				}
				
				s->replace(pos + it->position(), it->length(), a);
				pos += it->position() + a.length();
				it = std::sregex_iterator(s->begin() + pos, s->end(), accessor);
			}
			//if the parameter is set we parse now every loop iteration the inner part
			if (repeatatlyParseInner == true)
				walkElements(s, 0, s->length());
			newLength += s->length();
			toParse->insert(begin, *s);
		}
		return newLength;
	}
}


uint Template::ifElseF(StringUPtr& toParse, std::string ifElseName, uint begin, uint length, uint indentations, std::map<std::string, std::string> const &paras)
{
	//check if the ifElseName is overwritten or defaulted
	bool which = false;
	std::map<std::string, std::string>::const_iterator par = paras.find(ifElseName);
	if (par != paras.cend())
	{
		if (par->second == "true")
			which = true;
		else if (par->second == "false")
			which = false;
		else
			throw "Wrong default parameter set for " + ifElseName + ", can only be true or false.";
	}
	else
	{
		std::map<std::string, bool>::iterator it = m_IfElseDic.find(ifElseName);
		if (it == m_IfElseDic.end())
			throw "If/Else " + ifElseName + ": Did not find in the dictionary.";
		else
			which = it->second;
	}

	//the parsing
	std::regex sepr = std::regex("\\t{0," + std::to_string(indentations) + "}\\|\\|");
	std::sregex_iterator it = std::sregex_iterator(toParse->begin() + begin, toParse->begin() + begin + length, sepr), end;
	if (it == end)
		throw "If/Else: Could not find the if/else separator || for " + ifElseName + ".";
	std::pair<uint, uint> ifpart(begin, it->position());
	std::pair<uint, uint> elsepart(begin + it->position() + it->length() + 1, length - (it->position() + it->length() + 1));
		
	if (ifElseName == "")
		throw std::string("If/Else: no if if/else name provided.");
	std::string newcont = "";
	if (which == true)
	{
		uint newcontentlength = walkElements(toParse, ifpart.first, ifpart.second);
		newcont = toParse->substr(ifpart.first, newcontentlength);
		length += (newcontentlength - ifpart.second);
	}	
	else
	{
		int newcontentlength = walkElements(toParse, elsepart.first, elsepart.second);
		newcont = toParse->substr(elsepart.first, newcontentlength);
		length += (newcontentlength - elsepart.second);
	}	
	toParse->replace(begin, length, newcont);
	return newcont.length();
}


uint Template::switchF(StringUPtr& toParse, std::string switchName, uint begin, uint length, uint indentations, std::map<std::string, std::string> const &paras)
{
	//check if the ifElseName is overwritten or defaulted
	std::string which = "default";
	std::map<std::string, std::string>::const_iterator par = paras.find(switchName);
	if (par != paras.cend())
		which = par->second;
	else
	{
		std::map<std::string, std::string>::iterator it = m_SwitchDic.find(switchName);
		if (it != m_SwitchDic.end())
			which = it->second;
	}

	// start the parsing
	std::regex sepr = std::regex("\\t{0," + std::to_string(indentations) + "}\\|\\|(.+)");
	std::sregex_iterator it = std::sregex_iterator(toParse->begin() + begin, toParse->begin() + begin + length, sepr), end;
	if (it == end)
		throw "Switch " + switchName + ": Could not find the case separator.";
	//extract the cases
	std::map<std::string, std::pair<uint, uint> > cases;
	unsigned int pos = begin;
	while (it != end)
	{
		std::string casename = (*it)[1];
		uint casestart = pos + it->position() + it->length() + 1;
		it = std::sregex_iterator(toParse->begin() + casestart, toParse->begin() + begin + length, sepr);
		uint caselength = 0;
		if (it == end)
			caselength = length - (casestart - begin);
		else
		{
			caselength = it->position();
			pos = casestart;
		}
		cases[casename] = std::pair<uint, uint>(casestart, caselength);
	}

	//the final replacment to the chosen case
	std::map<std::string, std::pair<uint, uint> >::iterator newContIt = cases.find(which);
	if (newContIt == cases.end())
		throw "Did not find a case " + which + " in the " + switchName + " switch section.";
	
	int newcontentlength = walkElements(toParse, newContIt->second.first, newContIt->second.second);
	std::string newcont = toParse->substr(cases[which].first, newcontentlength);
	length += (newcontentlength - cases[which].second);
	toParse->replace(begin, length, newcont);
	return newcont.length();
}


uint Template::customF(StringUPtr& toParse, std::string customName, uint begin, uint length, uint indentations, std::map<std::string, std::string> const &paras)
{
	std::map<std::string, ParserFunc >::iterator it = m_CustomFunctionDic.find(customName);
	if (it == m_CustomFunctionDic.end())
		throw "Custom function " + customName + " not found.";
	//just calles the custom function
	return it->second(toParse, customName, begin, length, indentations, paras);
}








std::vector<std::string> Utils::StringSplit(std::string const &s, const char delimiter)
{
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> ret;
	while (std::getline(ss, item, delimiter))
	{
		while (isspace(item[0]) != 0)
			item = item.substr(1);

		while (isspace(item[item.length() - 1]) != 0)
			item = item.substr(0, item.length() - 1);

		ret.push_back(item);
	}
	return ret;
}



void Utils::Escape(std::string& str)
{
	//chars to escape: <, >, &
	std::regex search = std::regex("\\<|\\>|\\&");
	int pos = 0;
	std::sregex_iterator it = std::sregex_iterator(str.begin(), str.end(), search), end;
	while (it != end)
	{
		std::string found = (*it)[0];
		std::string rep = escapeLut.at(found);
		str.replace(pos + it->position(), it->length(), rep);
		pos += it->position() + rep.length();
		it = std::sregex_iterator(str.begin() + pos, str.end(), search);
	}
}






