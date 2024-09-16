#include "argparse/argparse.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
	const std::string version = "2024.9.16";
	const std::string program_path = (std::filesystem::canonical(std::filesystem::current_path() / ".")).string();
	const std::string sevenzip_path = "\"" + program_path + "/7z\"";
	argparse::ArgumentParser program("FileUnlockTool", version);
	program.add_argument("-p", "--password_file_path")
		.default_value("")
		.help("The file path for storing passwords");
	program.add_argument("files")
		.remaining()
		.help("The path of the compressed file that needs to be decompressed");
	try {
		program.parse_args(argc, argv);
	}
	catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
		return 1;
	}
	auto password_file_path = program.get<std::string>("-p");
	auto files = program.get<std::vector<std::string>>("files");

	std::string line;
	std::vector<std::string>passwords{ "" };
	if (password_file_path != "") {
		std::ifstream password_file(password_file_path);
		if (!password_file.is_open()) {
			std::cerr << "error to open " << password_file_path << std::endl;
		}
		else {
			while (std::getline(password_file, line)) {
				passwords.push_back(line);
			}
			password_file.close();
		}
	}
	for (const auto& file : files) {
		bool flag = false;
		for (const auto& password : passwords) {
			std::string cmd = "\"" + sevenzip_path + " x -o\"" + file + "~\" -p\"" + password + "\" \"" + file + "\"\"";
			int exitCode = system(cmd.c_str());
			if (!exitCode) {
				std::cout << file + " success!\npassword is \"" + password + "\"" << std::endl;
				flag = true;
				break;
			}
			else if (std::filesystem::exists(file + "~")) {
				std::filesystem::remove_all(file + "~");
			}
		}
		if (!flag) {
			std::cout << file + " fail!" << std::endl;
		}
	}
	return 0;
}
