#include <dchain/dchain.h>
#include <nihdb/nihdb.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <regex>
#include <sstream>

#define DBFILE "/.passwords"

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

std::string changePassword(nihdb::dataBase* datb, std::string oldpwd, std::vector<std::string> items)
{
	modStty(false, false);
	std::cout << "\nPasswords will be invisible\n";
	std::string temp1, temp2;

	bool btemp = false;
	while (!btemp) {
		std::cout << "-->Enter new password: ";
		std::getline(std::cin, temp1);
		putchar('\n');
		std::cout << "-->Confirm password: ";
		std::getline(std::cin, temp2);
		putchar('\n');

		btemp = (temp1 == temp2);

		if (temp1.empty()) {
			std::cout << "Password field cannot be empty.\n\n";
			btemp = false;
			continue;
		}

		if (!btemp)
			std::cout << "Passwords do not match.\n\n";
	}
	datb->ChangeVarValue("meta", "passwd", dchain::strEncrypt(temp1, temp1));

	for (int i = 0; i < items.size(); i++)
		datb->ChangeVarValue(items[i], "passwd", dchain::strEncrypt(dchain::strDecrypt(datb->ReturnVar(items[i], "passwd"), oldpwd), temp1));
	datb->ApplyChanges();

	return temp1;
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
		cmd("clear", false),
		cmd("list", false),
		cmd("xclip", false),
		cmd("passwd", false),
		cmd("generate", false)
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
	bool argCand;
	if (!arg) {
		temp = "(";
		temp += command;
		temp += ")(.*)";
		std::regex rgx(temp);
		for (int i = 0; i < sizeof(cmds)/sizeof(*cmds); i++) {
			if (std::regex_match(cmds[i].name, rgx)) {
				cand.push_back(cmds[i].name);
				argCand = cmds[i].items;
			}
		}
	}
	else
	{
		temp = "(";
		temp += command.substr(pos + 1, command.length() - pos);
		temp += ")(.*)";
		std::regex rgx(temp);
		temp = command.substr(0, pos);
		bool takesArg = false;

		for (int i = 0; i < sizeof(cmds)/sizeof(*cmds); i++) {
			if (cmds[i].name == temp) {
				takesArg = cmds[i].items;
				break;
			}
		}
		if (!takesArg)
			return command;

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
		else {
			if (argCand)
				return cand[0] + " ";
			else
				return cand[0];
		}
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
		<< "list            List all items in database\n"
		<< "xclip           Toggle automatic copying of passord to clipboard on command show\n"
		<< "passwd          Change password for database\n"
		<< "generate        Generate password\n"

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
	datb->CreateVar(items, "username", temp);

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

void showItem(std::string item, std::string passwd, nihdb::dataBase* datb)
{
	modStty(true, false);
	std::string temp = datb->ReturnVar(item, "username");
	if (temp.empty()) {
		std::cout << "\nItem \"" << item << "\" not found.\n";
		return;
	}

	std::cout << "\nUsername: " << temp << '\n';
	temp = dchain::strDecrypt(datb->ReturnVar(item, "passwd"), passwd);
	std::cout << "Password: " << temp << '\n';
	if (datb->ReturnVar("meta", "xclip") == "true") {
		temp.insert(0, "/usr/bin/echo -n \'");
		temp += "\' | xclip -selection clipboard";
		system(temp.c_str());
		std::cout << "Password copied to clipboard\n";
	}

}

std::vector<std::string> removeItem(std::string item, std::vector<std::string> items, nihdb::dataBase* datb)
{
	modStty(false, true);
	bool btemp = false;
	for (int i = 0; i < items.size(); i++) {
		if (item == items[i]) {
			btemp = true;
			break;
		}
	}

	if (!btemp) {
		std::cout << "\n\rItem \"" << item << "\" not found.\n";
		return items;
	}
	std::cout << "\n\rAre you sure you want to delete \"" << item << "\"? [N/y]: ";
	char c = std::cin.get();
	if ( c != '\r')
		putchar(c);
	std::cout << "\n\r";

	if ( ( c == '\r') || (c == 'n') || (c == 'Y') ) {
		return items;
	}

	datb->DeleteSection(item);
	std::cout << "Item \"" << item << "\" has been removed from the database\n";

	for (std::vector<std::string>::iterator itr = items.begin(); itr != items.end(); itr++) {
		if (*itr == item) {
			items.erase(itr);
			break;
		}
	}
	std::string temp;
	for (int i = 0; i < items.size(); i++)
		temp += items[i] + " ";

	if (temp.empty())
		temp = "empty";
	else
		temp.pop_back();

	datb->ChangeVarValue("meta", "items", temp);
	datb->ApplyChanges();
	return items;
}

void modItem(std::string item, std::string password, nihdb::dataBase* datb)
{
	modStty(true, false);
	std::string temp = datb->ReturnVar(item, "username");

	if (temp.empty()) {
		std::cout << "\nItem \"" << item << "\" not found.\n";
		return;
	}
	std::cout << "\n\nLeaving the field empty will leave the field unmodified.\n\n";
	std::string input;

	std::cout << "Username (" << temp << "): ";
	std::getline(std::cin, input);

	if (!input.empty())
		datb->ChangeVarValue(item, "username", input);

	temp = dchain::strDecrypt(datb->ReturnVar(item, "passwd"), password);
	std::cout << "Password (" << temp << "): ";
	std::getline(std::cin, input);

	if (!input.empty())
		datb->ChangeVarValue(item, "passwd", dchain::strEncrypt(input, password));

	datb->ApplyChanges();
	std::cout << "Item changes saved...\n";
	return;
}

void generate()
{
	modStty(true, false);
	std::string temp;
	std::cout << "\nEnter password length: ";
	std::stringstream ss;
	int length;
	std::getline(std::cin, temp);
	ss << temp;
	ss >> length;
	std::srand(std::clock());
	temp.clear();
	char c;
	for (int i = 0; i < length; i++) {
		 c = char((random() % 94) + 33);
		 if (c == '\'')
			 i--;
		 else
			 temp += c;
	}

	std::cout << "Generated password: " << temp << '\n';
	temp.insert(0, "/usr/bin/echo -n \'");
	temp += "\' | xclip -selection clipboard";
	system(temp.c_str());
	std::cout << "Password copied to clipboard\n";
	return;
}

int startCLI(nihdb::dataBase* datb, std::string password)
{
	std::cout << "bassoon command line interface initialized.\n\nType \"help\" to get a list of commands.\nCAUTION! All passwords will now be plaintext!\n\n";

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
	if (!temp.empty())
		items.push_back(command);
	bool bexit = false;
	bool esc = false;
	bool arrow = false;
	std::vector<std::string> cmdHist;
	int histpos = 0;

	while (!bexit)
	{
		cmdHist.push_back(std::string());
		histpos = cmdHist.size() - 1;
		modStty(false, true);
		command.clear();
		char c = '\0';
		std::cout << "\r> ";

		while (c != '\r')
		{
			cmdHist[histpos] = command;
			c = std::cin.get();

			if (c == 27) {
				esc = true;
				continue;
			}

			if (esc && !arrow) {
				if (c == 91) {
					arrow = true;
					continue;
				}
				else {
					esc = false;
					arrow = false;
				}
			}

			if (esc && arrow) {
				if (c == 65) {
					if (histpos > 0) {
						histpos -= 1;
						command = cmdHist[histpos];
					}
				}
				if (c == 66) {
					if (histpos < cmdHist.size() - 1) {
						histpos += 1;
						command = cmdHist[histpos];
					}
				}
				esc = false;
				arrow = false;
				std::cout << "\33[2K\r> " << command;
				continue;
			}



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
			while (true && (command.length() != 0)) {
				if (command[0] == ' ') {
					command.erase(0, 1);
					break;
				}
				command.erase(0, 1);
			}
			showItem(command, password, datb);
			continue;
		}

		if (std::regex_match(command, std::regex("(remove)(.*)"))) {
			while (true && (command.length() != 0)) {
				if (command[0] == ' ') {
					command.erase(0, 1);
					break;
				}
				command.erase(0, 1);
			}
			items = removeItem(command, items, datb);
			continue;
		}

		if (std::regex_match(command, std::regex("(modify)(.*)"))) {
			while (true && (command.length() != 0)) {
				if (command[0] == ' ') {
					command.erase(0, 1);
					break;
				}
				command.erase(0, 1);
			}
			modItem(command, password, datb);
			continue;
		}

		if (command == "list") {
			if (items.empty()) {
				std::cout << "\n\r";
				continue;
			}
			for (int i = 0; i < items.size(); i++) {
				std::cout << "\n\r" << items[i];
			}
			std::cout << "\n\r";
			continue;
		}

		if (command == "xclip") {
			if (datb->ReturnVar("meta", "xclip") == "true")
				datb->ChangeVarValue("meta", "xclip", "false");
			else
				datb->ChangeVarValue("meta", "xclip", "true");

			std::cout << "\r\nAutomatic copying of password to clipboard now set to [" << datb->ReturnVar("meta", "xclip") << "]\n\r";
			datb->ApplyChanges();
			continue;
		}

		if (command == "passwd") {
			password = changePassword(datb, password, items);
			std::cout << "\n\rPassword changed successfully\n\r";
			continue;
		}

		if (command == "generate") {
			generate();
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
			std::ofstream touch(filepath.c_str());
			touch.close();
			nihdb::dataBase tempdb(filepath);
			tempdb.AddComment("bassoon password database");
			tempdb.AddComment("All sensitive data is encrypted with dchain encryption library");
			tempdb.AddComment("Please do not modify this file manually as this could corrupt the database");
			tempdb.CreateSection("meta");
			tempdb.CreateVar("meta", "passwd");
			tempdb.CreateVar("meta", "items");
			tempdb.CreateVar("meta", "xclip", "false");
			changePassword(&tempdb, "", std::vector<std::string>());

			std::cout << "Database created successfully! Please log in to continue...\n\n";
		}
		else{
			modStty(true, false);
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
