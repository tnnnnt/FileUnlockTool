#include "argparse.hpp"
#include "json.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <windows.h>
TCHAR executablePath[MAX_PATH];
const std::string program_path = (GetModuleFileName(NULL, executablePath, MAX_PATH), std::filesystem::path(executablePath).parent_path()).generic_string();
const std::string sevenzip_path = "\"" + program_path + "/7z\"";
const std::string key_json_path = program_path + "/key.json";
const std::string version = "2024.9.17";
static void print(const std::string& s) {
	std::cout << s << std::endl;
}
static void print(const int& a) {
	std::cout << a << std::endl;
}
static bool try_password(const std::string& file, const std::string& password) {
	std::string cmd = "\"" + sevenzip_path + " x -o\"" + file + "~\" -p\"" + password + "\" \"" + file + "\" > NUL 2>&1\"";
	print(cmd);
	int exit_code = system(cmd.c_str());
	if (!exit_code) {
		print(file + " success!\npassword is \"" + password + "\"");
		return true;
	}
	else if (std::filesystem::exists(file + "~")) {
		std::filesystem::remove_all(file + "~");
		return false;
	}
}
static bool building_brute_password(const std::string& chars, const std::string password, const size_t& len, const std::string& file) {
	if (password.size() == len) {
		return try_password(file, password);
	}
	for (const auto& c : chars) {
		if (building_brute_password(chars, password + c, len, file)) {
			return true;
		}
	}
	return false;
}
int main(int argc, char* argv[]) {
	argparse::ArgumentParser program("FileUnlockTool", version);
	program.add_argument("-b", "--brute")
		.help("Brute matching.")
		.flag();
	program.add_argument("-e", "--exact")
		.help("Exact matching.")
		.flag();
	program.add_argument("-f", "--fuzzy")
		.help("Fuzzy matching.")
		.flag();
	program.add_argument("file")
		.help("The path of the compressed file that needs to be decompressed.");
	try {
		program.parse_args(argc, argv);
	}
	catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
		return -114514;
	}
	const bool& flag_brute = program.get<bool>("-b");
	const bool& flag_exact = program.get<bool>("-e");
	const bool& flag_fuzzy = program.get<bool>("-f");
	const std::string& file = program.get<std::string>("file");
	if (!std::filesystem::exists(file)) {
		std::cerr << file + " not found!" << std::endl;
		return -114514;
	}
	std::ifstream key_json_file(key_json_path);
	nlohmann::json key_json_data = nlohmann::json::parse(key_json_file);
	if (flag_exact) {
		const std::vector<std::string>& exact_passwords = key_json_data["exact"];
		for (const auto& password : exact_passwords) {
			if (try_password(file, password)) {
				return 0;
			}
		}
	}
	if (flag_brute) {
		const size_t& min_len = key_json_data["brute"]["min_len"];
		const size_t& max_len = key_json_data["brute"]["max_len"];
		const std::string& chars = key_json_data["brute"]["chars"];
		if (min_len <= max_len && chars != "") {
			for (size_t len = min_len; len <= max_len; ++len) {
				if (building_brute_password(chars, "", len, file)) {
					return 0;
				}
			}
		}
	}
	//if (flag_fuzzy) {
	//	const size_t& min_len = key_json_data["fuzzy"]["min_len"];
	//	const size_t& max_len = key_json_data["fuzzy"]["max_len"];
	//	const std::vector<std::string>& chars = key_json_data["fuzzy"]["chars"];
	//}
	print(file + " fail!");
	return -114514;
}
