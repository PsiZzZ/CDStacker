// License is GPLv3

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <deque>
#include <vector>
#include <utility>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

namespace po = boost::program_options;

using namespace std;

#define ROOT_CDSTACKER_DIR              ".config/CDStacker"
#define ROOT_CDSTACKER_FILE             "CDs"
#define ROOT_CDSTACKER_FILE_PATH        ROOT_CDSTACKER_DIR "/" ROOT_CDSTACKER_FILE
#define ROOT_CDSTACKER_TILDEFILE_PATH   ROOT_CDSTACKER_DIR "/~" ROOT_CDSTACKER_FILE

// Types
typedef string CD;

typedef deque<CD> stackT;

typedef deque<stackT> stacksT;

struct CD_Desc
{
	string name;
	string previous;
	string next;
	int num_in_stack;
	int stack_size;
	int percent_in_stack;
	string first_cd_in_stack;
};
// /Types

// Terminal colors
const char Red[] = {033, '[', '1', ';', '3', '1', 'm', 0};
const char Green[] = {033, '[', '1', ';', '3', '2', 'm', 0};
const char Yellow[] = {033, '[', '1', ';', '3', '3', 'm', 0};
const char Blue[] = {033, '[', '1', ';', '3', '4', 'm', 0};
const char Bold[] = {033, '[', '1', 'm', 0};
const char Reset[] = {033, '[', 'm', 017, 0};
// /Terminal colors


// Glob Vars

stacksT stacks;

bool match_case = 0;

// /Glob Vars

string lowercase(const string &str);

int addCD(int stackT, string CDName, int location);

list<CD_Desc> searchCD(string CDNameRegexp);

void RemoveCD(string StackName, string CDName);
void RemoveCDRegexp(string StackRegexp, string CDRegexp);
void AddCD(string StackName, string CDName);
void AddCDRegexp(string StackRegexp, string CDName);

void CleanStacks(void);

void Loadstacks(string filename);
void Savestacks(string filename);

//Returns n so that 2^n <= k < 2^(n+1)
int GetPower2Num(int k);

int main (int ac, char** av)
{

	po::options_description desc("CDStacker options\nFor the add,remove* functions, no --stack means \"any stack\" or \"a new stack\"");
	desc.add_options()
		("help,h", "Show this message")
		("search,S", po::value<string>(), "search a CD in the database (regexp)")
		("case,C", "Match case")
		("use,U", "Mark the CD as used (see doc)")
		("stack,s", po::value<string>(), "Select a stack (for add, remove*, list) (regexp)")
		("add,A", po::value<string>(), "Add a cd in the given CD stack")
		("remove,R", po::value<string>(), "Remove CD(s) from the given CD stack")
		("list,L", "Lists the CD in the given CD stack")
		("list-stacks", "Lists stacks (i.e. first CD of each stack)")
		("edit", po::value<string>(), "Edit the CDs file with your editor (--edit gedit for example)")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(ac, av, desc), vm);
	po::notify(vm);


	if (vm.count("help"))
	{
	    cout << desc << "\n";
	    return 1;
	}

	if (vm.count("edit"))
	{
	    system((vm["edit"].as<string>()+" "+getenv("HOME")+ ("/" ROOT_CDSTACKER_FILE_PATH)).c_str());
	    return 1;
	}

	if (vm.count("case"))
		match_case = 1;
	else
		match_case = 0;


	Loadstacks((string)getenv("HOME")+ ("/" ROOT_CDSTACKER_FILE_PATH));
	Savestacks((string)getenv("HOME")+ ("/" ROOT_CDSTACKER_TILDEFILE_PATH));

	if (vm.count("search"))
	{
		list<CD_Desc> result = searchCD(vm["search"].as<string>() );

		cout << endl;

		for (list<CD_Desc>::iterator i = result.begin(); i != result.end(); i++)
		{
			cout
			     << Yellow <<"*  " << Reset << Green << (*i).name << Reset << endl
			     << "     Stack : (" << Bold << (*i).first_cd_in_stack << Reset << ")   (" << (*i).num_in_stack << "/" << (*i).stack_size << " = " << (*i).percent_in_stack  << "%)" << endl
			     << "     Previous : " << Bold << (*i).previous << Reset << endl
			     << "     Next : " << Bold << (*i).next << Reset << endl
			     << "     Howto find it (D:Divide the stack in 2, u:keep the up, d: keep the down) : \n       ";

			int n = GetPower2Num((*i).stack_size);
			int w = 0;

			for (int k = 1; k <= n; k++)
			{
				cout << "D,";

				if ( w+(*i).stack_size/(1<<k) >= (*i).num_in_stack )
				{
					cout << Yellow << "u" << Reset;
				}
				else
				{
					cout << Yellow << "d" << Reset;
					w+= (*i).stack_size/(1<<k);
				}

				if (k!=n) cout << " - ";
			}
			cout << '\n' << endl;

		}

		if (vm.count("use"))
		{
			if (result.size()!=1)
				cout << Red << "The result was not composed by only one CD (i.e. 0 or >=2 ;) ), --use (-U) will have no effect" << Reset << endl;
			else
			{
				list<CD_Desc>::iterator i = result.begin();
				//Remove it and place it on top, if it was not already
				if ((*i).previous == "(first)")
				{
					cout << Blue << "The CD was already the first in the stack, --use (-U) will have no effect" << Reset << endl;
					return 0;
				}
				RemoveCD((*i).first_cd_in_stack, (*i).name);
				AddCD((*i).first_cd_in_stack, (*i).name);
				Savestacks((string)getenv("HOME")+ ("/" ROOT_CDSTACKER_FILE_PATH));
				return 0;
			}
		}
		return 0;
	}
	if (vm.count("add"))
	{

		if (!vm.count("stack"))
		{
			AddCDRegexp((string)"",vm["add"].as<string>());
			Savestacks((string)getenv("HOME")+ ("/" ROOT_CDSTACKER_FILE_PATH));
			return 0;
		}
		else
		{
			boost::basic_regex<char> Sregex((match_case ? vm["stack"].as<string>() : lowercase(vm["stack"].as<string>())).c_str());

			int numstack=0;

			for (deque<stackT>::iterator stack = stacks.begin(); stack != stacks.end(); stack++)
			if ( boost::regex_match( (match_case ? *(*stack).begin() : lowercase(*(*stack).begin())).c_str(),Sregex ) )
			{
				numstack++;
			}

			if (numstack != 1)
			{
				cout << Red << "Error, if you want to add a CD, there should be only one stack corresponding to your regexp" << Reset << endl;
				return 1;
			}

			AddCDRegexp(vm["stack"].as<string>(),vm["add"].as<string>());
			Savestacks((string)getenv("HOME")+ ("/" ROOT_CDSTACKER_FILE_PATH));
			return 0;
		}
	}

	if (vm.count("remove"))
	{
		string stackregexp = ".*";
		if (vm.count("stack"))
			stackregexp = vm["stack"].as<string>();

		RemoveCDRegexp(stackregexp, vm["remove"].as<string>());
		CleanStacks();
		Savestacks((string)getenv("HOME")+ ("/" ROOT_CDSTACKER_FILE_PATH));
		return 0;

	}


	if (vm.count("list"))
	{
		if (vm.count("stack"))
		{
			boost::basic_regex<char> Sregex((match_case ? vm["stack"].as<string>() : lowercase(vm["stack"].as<string>())).c_str());

			for (deque<stackT>::iterator stack = stacks.begin(); stack != stacks.end(); stack++)
			if ( boost::regex_match( (match_case ? *(*stack).begin() : lowercase(*(*stack).begin())).c_str(),Sregex ) )
			{
				cout << '\n' << Yellow << *(*stack).begin() << Reset << " (" << (*stack).size() << ")" << endl;
				for (deque<CD>::iterator CD = (*stack).begin(); CD != (*stack).end(); CD++)
				{
					cout << Blue << "   * " << Reset << *CD << endl;
				}
				cout << endl;
			}
			else //Show that there is a stack but without developing it
				cout << Yellow << "* " << Reset << *(*stack).begin() << " (" << (*stack).size() << ")" << endl;
		}
		else
			for (deque<stackT>::iterator stack = stacks.begin(); stack != stacks.end(); stack++)
			{
				cout << Yellow << *(*stack).begin() << Reset << endl;
				for (deque<CD>::iterator CD = (*stack).begin(); CD != (*stack).end(); CD++)
				{
					cout << Blue << "   * " << Reset << *CD << endl;
				}
				cout << '\n' << endl;
			}

		return 0;

	}

	if (vm.count("list-stacks"))
	{

		for (deque<stackT>::iterator stack = stacks.begin(); stack != stacks.end(); stack++)
			cout << Yellow << "* " << Reset << *(*stack).begin() << " (" << (*stack).size() << ")" << endl;
		return 0;
	}

}

int GetPower2Num(int k)
{
	int n = 0;
	while ( (1 << n) <= k)
	{
		n++;
	}
	return n-1;
}

list<CD_Desc> searchCD(string CDNameRegexp)
{
	cout << "Searching regexp '" << Blue << CDNameRegexp << Reset << "' ..." << endl;

	boost::basic_regex<char> regex((match_case ? CDNameRegexp : lowercase(CDNameRegexp)).c_str());

	int cd_num;
	list<CD_Desc>  l;
	CD_Desc CDDescTemp;
	cout << '[';cout.flush();
	//for each stack
	for (stacksT::iterator stack = stacks.begin(); stack != stacks.end(); stack++)
	{
		if (stack!=stacks.begin())
			cout << "],[";

		cout.flush();
		cd_num = 0;
		//for each CD in each stack
		for (stackT::iterator cd = (*stack).begin(); cd != (*stack).end(); cd++)
		{
			cd_num++;
			//Does the regexp correspond ?
			if (boost::regex_match( (match_case ? *cd : lowercase(*cd)).c_str(),regex ))
			{
				CDDescTemp.name = (*cd);
				//Check what the previous and next CD are to find it faster
				if (cd_num != 1)
				{
					CDDescTemp.previous = (*(--cd));
					cd++;
					if (cd_num != (*stack).size())
					{
						CDDescTemp.next = (*(++cd));
						cd--;
					}
					else
					{
						CDDescTemp.next = "(last)";
					}
				}
				else
				{
					CDDescTemp.previous = "(first)";
					if (cd_num != (*stack).size())
					{
						CDDescTemp.next = (*(++cd));
						cd--;
					}
					else
						CDDescTemp.next = "(last)";
				}

				CDDescTemp.num_in_stack = cd_num;
				CDDescTemp.percent_in_stack = (100*cd_num)/(*stack).size();
				CDDescTemp.stack_size = (*stack).size();
				CDDescTemp.first_cd_in_stack = (*(*stack).begin());
				l.push_back(CDDescTemp);

				cout << Red << "*" << Reset; cout.flush();
			}
			else
			{
				cout << Bold <<"." << Reset; cout.flush();
			}
		}

	}
	cout << ']';cout.flush();
	cout << endl;

	return l;
}

void CleanStacks(void)
{
	deque<stackT>::iterator stack = stacks.begin();
	while (stack != stacks.end())
	{
		if ((*stack).size() == 0)
		{
			stacks.erase(stack);
			stack = stacks.begin();
		}
		else
			stack++;
	}
}

void RemoveCD(string StackName, string CDName)
{

	for (deque<stackT>::iterator stack = stacks.begin(); stack != stacks.end(); stack++)
	if ( *(*stack).begin() == StackName )
	{
		for (deque<CD>::iterator CD = (*stack).begin(); CD != (*stack).end(); CD++)
		if (*CD == CDName)
		{
			(*stack).erase(CD);
			return;
		}

		cout << Red << "Could not find the right CD in this stack (" << *(*stack).begin() << ')' << Reset << endl;
		return;
	}

	cout << Red << "Could not find the right stack" << Reset << endl;

}

void AddCD(string StackName, string CDName)
{
	if (StackName == "")
	{
		deque<CD> newstack;
		newstack.push_front(CDName);
		stacks.push_front(newstack);
	}
	else
	{
		for (deque<stackT>::iterator stack = stacks.begin(); stack != stacks.end(); stack++)
		{
			if ( *(*stack).begin() == StackName )
			{
				(*stack).push_front(CDName);
				return;
			}

		}
		cout << Red << "Could not find the right stack" << Reset << endl;
	}
}

void AddCDRegexp(string StackRegexp, string CDName)
{
	if (StackRegexp == "")
	{
		deque<CD> newstack;
		newstack.push_front(CDName);
		stacks.push_front(newstack);
		cout << Yellow << "* " << Reset << "Creating a new stack (" << CDName << ")" << endl;
	}
	else
	{
		boost::basic_regex<char> Sregex((match_case ? StackRegexp : lowercase(StackRegexp)).c_str());
		for (deque<stackT>::iterator stack = stacks.begin(); stack != stacks.end(); stack++)
		{
			if ( boost::regex_match( (match_case ? *(*stack).begin() : lowercase(*(*stack).begin())).c_str(),Sregex ) )
			{
				cout << Yellow << "* " << Reset << "Adding '" << Blue << CDName << Reset << "' to (" << *(*stack).begin() <<')'<< endl;
				(*stack).push_front(CDName);
				return;
			}
		}
	}
}

void RemoveCDRegexp(string StackRegexp, string CDRegexp)
{
	boost::basic_regex<char> regex((match_case ? CDRegexp : lowercase(CDRegexp)).c_str());
	boost::basic_regex<char> Sregex((match_case ? StackRegexp : lowercase(StackRegexp)).c_str());

	for (deque<stackT>::iterator stack = stacks.begin(); stack != stacks.end(); stack++)
	if (boost::regex_match( (match_case ? *(*stack).begin() : lowercase(*(*stack).begin())).c_str(),Sregex ))
	{
		deque<CD>::iterator CD = (*stack).begin();
		while (CD != (*stack).end())
		{
			if (boost::regex_match( (match_case ? *CD : lowercase(*CD)).c_str(),regex ))
			{
				cout << Yellow << "* " << Reset << "Removing (" << *(*stack).begin() << ")/'" << Red << *CD << Reset << '\'' << endl;
				(*stack).erase(CD);
				CD=(*stack).begin();
			}
			else
				CD++;
		}
	}
}

void Loadstacks(string filename)
{
	ifstream stacksfile;
	stacksfile.open(filename.c_str());
	if (!stacksfile.is_open())
	{
		cout << Red << "The CDs file could not be opened, if it does not exist, you should to create " << filename << Reset << endl;
		cout << "\nFormat :\nLeave first and last line empty, write 1 CD name by line, separate de stacks by an empty line" << endl;
		exit(0);
	}

	char line[512];
	stacksfile.getline(line,512);

	stackT *temp_stack;

	temp_stack = new stackT;
	while (!stacksfile.eof())
	{
		stacksfile.getline(line,512);
		if ((string) line == (string) "")
		{
			if (temp_stack->size()!=0)
			{
				stacks.push_back(*temp_stack);
				delete temp_stack;
				temp_stack = new stackT;
			}
		}
		else
		{
			temp_stack->push_back((string) line);
		}

	}
	if (temp_stack->size()!=0)
	{
		stacks.push_back(*temp_stack);
		delete temp_stack;
	}
	stacksfile.close();
}

void Savestacks(string filename)
{
	ofstream stacksfile;
	stacksfile.open(filename.c_str());
	if (!stacksfile.is_open())
	{
		cout << Red << "The CDs file (" << filename.c_str() << ") could not be opened" << Reset << endl;
		exit(0);
	}
	//Dirty UTF8 =)
	stacksfile << (char)0xEF << (char)0xBB << (char)0xBF << endl;

	for (stacksT::iterator stack = stacks.begin(); stack != stacks.end(); stack++)
	{
		for (stackT::iterator cd = (*stack).begin(); cd != (*stack).end(); cd++)
		{
			stacksfile << *cd << endl;
		}
		stacksfile << endl;
	}

	stacksfile.close();
}

string lowercase(const string &str)
{
	string out = str;
	for (unsigned int i=0; i<out.size(); i++)
	{
		out[i] = (char)tolower((int)out[i]);
	}
	return out;
}

