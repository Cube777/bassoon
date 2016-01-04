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
	return "";
}

int startCLI(nihdb::dataBase* datb, std::string password)
{
	modStty(false, true);
	std::cout << "bassoon command line interface initialized.\n\n\rType \"help\" to get a list of commands.\n\n\r";

	std::string command;
	std::vector<std::string> items;
	bool bexit = false;

	while (!bexit)
	{
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
