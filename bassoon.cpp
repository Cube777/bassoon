#include <dchain/dchain.h>
#include <nihdb/nihdb.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <regex>

#define DBFILE "/.password"

struct cmd {
	cmd(std::string nm, bool use){
		name = nm;
		items = use;
	}
	std::string name;
	bool items;
};

void modStty(bool echo, bool raw)
{
	if (echo)
		system("/usr/bin/stty echo");
	else
		system("/usr/bin/stty -echo");

	if (raw)
		system("/usr/bin/stty raw");
	else
		system("/usr/bin/stty -raw");
}

std::string tabComplete(std::string command, std::vector<std::string> items)
{
	if (command.empty())
		return "";

	cmd cmds[] = {
		cmd("help", false),
		cmd("exit", false),
		cmd("show", true),
		cmd("modify", true),
		cmd("remove", true),
		cmd("new", false),
		cmd("clear", false)
	};

	bool arg = false;
	int pos = 0;

	for (; pos < command.length(); pos++) {
		if (command[pos] == ' ') {
			arg = true;
			break;
		}
	}

	std::string temp;
	std::vector<std::string> cand;
	if (!arg) {
		temp = "(";
		temp += command;
		temp += ")(.*)";
		std::regex rgx(temp);
		for (int i = 0; i < sizeof(cmds)/sizeof(*cmds); i++) {
			if (std::regex_match(cmds[i].name, rgx))
				cand.push_back(cmds[i].name);
		}
	}
	else
	{
		temp = "(";
		temp += command.substr(pos + 1, command.length() - pos);
		temp += ")(.*)";
		std::regex rgx(temp);

		for (int i = 0; i < items.size(); i++) {
			if (std::regex_match(items[i], rgx))
				cand.push_back(items[i]);
		}
	}
	if (cand.size() == 0)
		return command;

	if (cand.size() == 1)
	{
		if (arg)
			return command.substr(0, pos + 1) + cand[0];
		else
			return cand[0];
	}

	std::cout << "\r> " << command.substr(0, pos + 1) << "{ ";
	for (int i = 0; i < cand.size(); i++)
		std::cout << cand[i] << "; ";
	std::cout << "}\n\n";

	bool btemp = true;
	temp.clear();
	char c;
	pos = 0;

	while (true) {
		if (pos >= cand[0].length())
			break;
		else
			c = cand[0][pos];

		for (int i = 0; i < cand.size(); i++) {
			if (pos >= cand[i].length()) {
				btemp = false;
				break;
			}
			btemp = (c == cand[i][pos]) && btemp;
			if (!btemp)
				break;
		}

		if (!btemp)
			break;

		temp += c;
		pos++;
	}

	if (!arg)
		return temp;
	else {
		int i = 0;
		while (true) {
			if (command[i++] == ' ')
				break;
		}
		temp.insert(0, command.substr(0, i));
		return temp;
	}
}

void printHelp()
{
	modStty(true,false);

	std::cout << "\n\nUsage: command (argument/item)\n\n"
		<< "exit            Close bassoon\n"
		<< "show (item)     Show item details\n"
		<< "modify (item)   Modify item\n"
		<< "remove (item)   Remove item\n"
		<< "new             Create a new item\n"
		<< "clear           Clear screen\n"

		<< '\n';
	modStty(false,true);
}

std::string newItem(nihdb::dataBase* datb, std::string passwd)
{
	modStty(true, false);
	bool btemp = false;
	std::string temp, items;

	while (!btemp) {
outer:
		std::cout << "\nEnter item name: ";
		temp.clear();
		std::getline(std::cin, temp);

		if (temp == "meta") {
			std::cout << "Name \"meta\" cannot be used as it is used internally inside bassoon\n\n";
			continue;
		}

		for (int i = 0; i < temp.size(); i++) {
			if (temp[i] == ' ') {
				std::cout << "Name cannot contain spaces\n\n";
				goto outer;
			}
		}

		btemp = datb->CreateSection(temp);

		if (!btemp) {
			std::cout << "Name \"" << temp << "\" is already in use, please use another\n";
			continue;
		}

		items = datb->ReturnVar("meta", "items");
		if (items == "empty") {
			items.clear();
			items = temp;
		} else {
			items += " ";
			items += temp;
		}

		datb->ChangeVarValue("meta", "items", items);
		items = temp;
	}

	btemp = false;
	while (!btemp) {
		std::cout << "Enter username for this item: ";
		std::getline(std::cin, temp);
		if (temp.empty()) {
			std::cout << "Username field cannot be empty\n";
			continue;
		}
		btemp = true;
	}
	datb->CreateVar(items, "username", dchain::strEncrypt(temp, passwd));

	btemp = false;
	while (!btemp) {
		std::cout << "Enter password for this item: ";
		std::getline(std::cin, temp);
		if (temp.empty()) {
			std::cout << "Password field cannot be empty\n";
			continue;
		}
		btemp = true;
	}
	datb->CreateVar(items, "passwd", dchain::strEncrypt(temp, passwd));

	datb->ApplyChanges();
	std::cout << "Item \"" << items << "\" successfully created!\n\n";
	return items;
}

int startCLI(nihdb::dataBase* datb, std::string password)
{
	std::cout << "bassoon command line interface initialized.\n\nType \"help\" to get a list of commands.\n\n";

	std::string temp, command;
	temp = datb->ReturnVar("meta", "items");
	if (temp == "empty")
		temp.clear();
	std::vector<std::string> items;
	for (int i = 0; i < temp.length(); i++){
		if (temp[i] == ' ') {
			items.push_back(command);
			command.clear();
			continue;
		}
		command += temp[i];
	}
	items.push_back(command);
	bool bexit = false;

	while (!bexit)
	{
		modStty(false, true);
		command.clear();
		char c = '\0';
		std::cout << "\r> ";

		while (c != '\r')
		{
			c = std::cin.get();

			if (c == '\r')
				continue;

			if (c == '\t') {
				command = tabComplete(command, items);
				std::cout << "\33[2K\r> " << command;
				continue;
			}

			if (c == 127) {
				if (command.length() != 0)
					command.pop_back();
				std::cout << "\33[2K\r> " << command;
				continue;
			}

			command += c;
			putchar(c);
		}

		if (command.empty()) {
			putchar('\n');
			continue;
		}

		if (command == "exit") {
			bexit = true;
			continue;
		}

		if (command == "help") {
			printHelp();
			continue;
		}

		if (command == "clear") {
			system("clear");
			continue;
		}

		if (command == "new") {
			items.push_back(newItem(datb, password));
			continue;
		}

		if (std::regex_match(command, std::regex("(show)(.*)"))) {
			continue;
		}

		std::cout << "\n\rCommand \"" << command << "\" not found\n";
	}

	return 0;
}

int main()
{
	std::string filepath = getenv("HOME");
	filepath += DBFILE;

	std::ifstream test(filepath.c_str());
	std::string password, temp;

	if (!test.is_open())
	{
		std::cout << "Error, password database not found. Would you like to create one? [Y/n]: ";
		modStty(false, true);
		char c = std::cin.get();

		if (c != '\r')
			std::cout << c << "\n\r";
		else
			std::cout << "\n\r";

		if ( (c == '\r') || (c == 'y') || (c == 'Y') )
		{
			modStty(false, false);
			std::cout << "\nPasswords will be invisible\n";

			bool btemp = false;
			while (!btemp) {
				std::cout << "-->Enter new password: ";
				std::getline(std::cin, temp);
				putchar('\n');
				std::cout << "-->Confirm password: ";
				std::getline(std::cin, password);
				putchar('\n');

				btemp = (temp == password);

				if (!btemp)
					std::cout << "Passwords do not match!\n\n";
			}

			std::ofstream touch(filepath.c_str());
			touch.close();
			nihdb::dataBase tempdb(filepath);
			tempdb.AddComment("bassoon password database");
			tempdb.AddComment("All sensitive data is encrypted with dchain encryption library");
			tempdb.AddComment("Please do not modify this file manually as this could corrupt the database");
			tempdb.CreateSection("meta");
			tempdb.CreateVar("meta", "passwd", dchain::strEncrypt(password, password));
			tempdb.CreateVar("meta", "items");
			tempdb.CreateVar("meta", "xclip", "false");

			tempdb.ApplyChanges();
			std::cout << "Database created successfully! Please log in to continue...\n\n";
		}
		else{
			modStty(true, false);
			std::cout << "asd";
			return 1;
		}
	}

	nihdb::dataBase* datb = new nihdb::dataBase(filepath);
	modStty(false, false);

	int count = 1;
	bool btemp = false;

	while (!btemp)
	{
		std::cout << "-->Enter password: ";
		std::getline(std::cin, password);
		putchar('\n');
		btemp = ( password == dchain::strDecrypt(datb->ReturnVar("meta", "passwd"), password));

		if (!btemp) {
			std::this_thread::sleep_for(std::chrono::seconds(3));
			if (count < 3)
				std::cout << "Incorrect password, please try again\n";
			else {
				std::cout << "Too many incorrect password attempts...\n";
				return 1;
			}
			count++;
		}
	}

	std::cout << "Initializing command line interface...\n";
	int status =  startCLI(datb, password);
	modStty(true, false);
	putchar('\n');
	return status;
}
