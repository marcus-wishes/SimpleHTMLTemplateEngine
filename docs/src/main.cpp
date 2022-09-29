#include <map>
#include <string>
#include "../SimpleTemplateEngine.h"
#include <iostream>

#include <functional>



int main(int argc, const char* argv[])
{
	///this is the c++ parts of the pangrams example:

	//lets create a dictionary containing the new content of the variables.
	std::map<std::string, std::string> vars;
	vars["title"] = "Pangrams Example";
	vars["most known pangram"] = "The quick brown fox jumps over the lazy dog.";
	vars["animal1"] = "fox";
	vars["animal2"] = "dog";

	//next, the content for the loops
	std::map<std::string, std::vector<std::string> > loop;
	loop["pangramlist"] = std::vector<std::string>();
	loop["pangramlist"].push_back("Sphinx of black quartz judge my vow.");
	loop["pangramlist"].push_back("A quick movement of the enemy will jeopardize six gunboats.");
	loop["pangramlist"].push_back("Five quacking Zephyrs jolt my wax bed.");
	loop["pangramlist"].push_back("Heavy boxes perform waltzes and jigs.");

	loop["empty"] = std::vector < std::string >();
	loop["empty"].push_back("");
	loop["empty"].push_back("");
	loop["empty"].push_back("");

	//and for the switch example
	std::map<std::string, std::string> swith;
	swith["choosePangram"] = "first";

	//the if/else example
	std::map<std::string, bool> ifElse;
	ifElse["isPangram"] = true;

	//finally and the most complicated... we create a custom function which returns just the content unchanged (basically this means that just the syntax elements like section markings, name and parameters will be removed.
	//return value = the new length of the content
	//content      = a unique pointer reference to the "whole" content which is currently being parsed. but the only content which should be handled is beginning at "begin"
	//customName   = identifier of the custom function (its name)
	//begin		   = the size from where the content in the pointer is to be handled in this custom function
	//length	   = the length of the content to be handled.  
	//begin and length basically mark the section between the syntactical elements °° and °°/ marking it.
	//params	   = the dictionary containing the parameter... every custom parameter can be set in the correct syntax °°customFunctionIdentifier [param1=value1; param2=value2]...
	std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)>  customFunction = [](
		std::unique_ptr<std::string>& wholeContent, 
		std::string customName,
		unsigned int begin, 
		unsigned int length, 
		unsigned indentations, 
		std::map<std::string, std::string> parameters)
	{
		//lets add the {{title}} and let it be parsed:
		std::string title = "{{title}}: ";
		wholeContent->insert(begin, title);
		length += title.length();
		length = SimpleTemp::parse(wholeContent, begin, length);
		//and some not very useful operations
		std::string sectionContent = "Content handled by " + customName + ": " + wholeContent->substr(begin, length);
		wholeContent->replace(begin, length, sectionContent);
		return sectionContent.length();
	};
	//after creating the function we need the custom function dictionary:
	std::map< std::string, std::function<unsigned int(std::unique_ptr<std::string>&, std::string, unsigned int, unsigned int, unsigned int, std::map<std::string, std::string>)> > customFuncDic;
	customFuncDic["customExampleFunction"] = customFunction;

	//a test for the Utils::Escape method:
	std::string escapeTest = "Please escape these characters: <, >, &!";
	std::cout << "EscapeTest: " << std::endl << escapeTest << std::endl;
	SimpleTemp::Utils::Escape(escapeTest);
	std::cout << "EscapeTest Result: " << std::endl << escapeTest << std::endl;


	//now we create a template instance, and set the data dictionaries, to be sure (altough in this example not necessary) we let them be escaped.
	SimpleTemp::Template html;
	html.SetIfElseDictionary(ifElse);
	html.SetLoopDictionary(loop, true);
	html.SetSwitchDictionary(swith);
	html.SetVariableDictionary(vars, true);
	html.SetCustomFunctionDictionary(customFuncDic);
	try{
		html.FillTemplateFile("D:/MasterProject/VolumeShop/QT5BRanch/src/plugins/editors/plugin_editor_statechangerecorder/src/SimpleTemplateEngine/doc/pangrams.tmpl", "D:/MasterProject/VolumeShop/QT5BRanch/src/plugins/editors/plugin_editor_statechangerecorder/src/SimpleTemplateEngine/doc/pangrams.html");
	}
	catch (std::string s)
	{
		std::cerr << s << std::endl;
		return -1;
	}
	return 0;
}



int mainREP(int argc, const char* argv[])
{
	///this is the c++ parts of the pangrams example:

	//lets create a dictionary containing the new content of the variables.
	std::map<std::string, std::string> vars;
	vars["title"] = "Example Examination Title";

	//next, the content for the loops
	std::map<std::string, std::vector<std::string> > loop;
	loop["StateNodes"] = std::vector<std::string>();
	for (unsigned int i = 0; i < 100; ++i)
	{
		loop["StateNodes"].push_back("Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit  << >> && amet.");
	}


	//now we create a template instance, and set the data dictionaries
	SimpleTemp::Template report;
	report.SetLoopDictionary(loop, true);
	report.SetVariableDictionary(vars, true);

	try{
		report.FillTemplateFile("D:/MasterProject/VolumeShop/QT5BRanch/src/plugins/editors/plugin_editor_statechangerecorder/templates/report.html", "D:/MasterProject/VolumeShop/QT5BRanch/src/plugins/editors/plugin_editor_statechangerecorder/templates/reportFinished.html");
	}
	catch (std::string s)
	{
		std::cerr << s << std::endl;
		return -1;
	}
	return 0;
}



//TODO:
// - escape the strings and literals:
// https://github.com/mrtazz/plustache/blob/master/src/template.cpp
// - remove necessity for indentations but use counter

