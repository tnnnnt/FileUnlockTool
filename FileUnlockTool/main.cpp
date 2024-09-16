#include "argparse/argparse.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

const std::string version = "2024.9.16";
const std::string program_path = (std::filesystem::canonical(std::filesystem::current_path() / ".")).string();
const std::string sevenzip_path = "\"" + program_path + "/7z\"";
static bool building_brute_force_password(const std::string& iterative_chars, std::string password, const size_t& len, const std::string& file) {
	if (password.size() == len) {
		std::string cmd = "\"" + sevenzip_path + " x -o\"" + file + "~\" -p\"" + password + "\" \"" + file + "\"\"";
		int exit_code = system(cmd.c_str());
		if (!exit_code) {
			std::cout << file + " success!\npassword is \"" + password + "\"" << std::endl;
			return true;
		}
		else if (std::filesystem::exists(file + "~")) {
			std::filesystem::remove_all(file + "~");
			return false;
		}
	}
	for (const auto& c : iterative_chars) {
		if (building_brute_force_password(iterative_chars, password + c, len, file)) {
			return true;
		}
	}
	return false;
}
int main(int argc, char* argv[]) {
	argparse::ArgumentParser program("FileUnlockTool", version);
	program.add_argument("-i", "--iterative_char_range")
		.help("Character sequences used for brute force cracking")
		.default_value("");
	program.add_argument("-min", "--minimum_length")
		.help("The minimum length for brute force or brute force password cracking")
		.default_value(1)
		.scan<'u', size_t>();
	program.add_argument("-max", "--maximum_length")
		.help("The maximum length for brute force or brute force password cracking")
		.default_value(8)
		.scan<'u', size_t>();
	program.add_argument("-p", "--password_file_path")
		.help("The file path for storing passwords")
		.default_value("");
	program.add_argument("files")
		.help("The path of the compressed file that needs to be decompressed")
		.remaining();
	try {
		program.parse_args(argc, argv);
	}
	catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
		return 1;
	}
	const std::string iterative_char_range = program.get<std::string>("-i");
	const size_t minimum_length = program.get<size_t>("-min");
	const size_t maximum_length = program.get<size_t>("-max");
	const std::string password_file_path = program.get<std::string>("-p");
	const std::vector<std::string> files = program.get<std::vector<std::string>>("files");

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
	std::set<char>set_iterative_chars;
	std::string iterative_chars = "";
	for (const auto& c : iterative_char_range) {
		set_iterative_chars.insert(c);
	}
	for (const auto& it : set_iterative_chars) {
		iterative_chars += it;
	}
	for (const auto& file : files) {
		bool flag = false;
		for (const auto& password : passwords) {
			std::string cmd = "\"" + sevenzip_path + " x -o\"" + file + "~\" -p\"" + password + "\" \"" + file + "\"\"";
			int exit_code = system(cmd.c_str());
			if (!exit_code) {
				std::cout << file + " success!\npassword is \"" + password + "\"" << std::endl;
				flag = true;
				break;
			}
			else if (std::filesystem::exists(file + "~")) {
				std::filesystem::remove_all(file + "~");
			}
		}
		if (!flag) {
			if (minimum_length <= maximum_length && iterative_char_range != "") {
				for (size_t len = minimum_length; len <= maximum_length; ++len) {
					if (building_brute_force_password(iterative_chars, "", len, file)) {
						flag = true;
						break;
					}
				}
			}
			if (!flag) {
				std::cout << file + " fail!" << std::endl;
			}
		}
	}
	return 0;
}
