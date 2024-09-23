#include "argparse.hpp"
#include "json.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <windows.h>
static void print(const std::string& s) {
	std::cout << s << std::endl;
}
static void print(const int& a) {
	std::cout << a << std::endl;
}
static std::string get_one_file_from_zip(const std::string& sevenzip_path, const std::string& zip_file) {
	const std::string command = "\"" + sevenzip_path + " l -p1 \"" + zip_file + "\"\"";
	std::string output = "";
	char buffer[128];
	// �򿪹ܵ�
	FILE* pipe = _popen(command.c_str(), "r");
	if (!pipe) {
		std::cerr << "popen() failed!" << std::endl;
		return "";
	}
	// ��ȡ����������buffer
	while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
		output += buffer;
	}
	// �رչܵ�
	_pclose(pipe);
	std::stringstream output_stream(output);
	std::string line;
	while (getline(output_stream, line)) {
		if (line.length() > 53 && (line.substr(20, 5) == "....A" || line.substr(20, 5) == ".....")) {
			return "\"" + line.substr(53) + "\"";
		}
	}
	return "";
}
static bool try_password(const std::string& sevenzip_path, const std::string& file, const std::string& password, const std::string& one_file) {
	std::string cmd = "\"" + sevenzip_path + " x -o\"" + file + "~\" -p\"" + password + "\" -y \"" + file + "\" " + one_file + " > NUL 2>&1\"";
	print(cmd);
	int exit_code = system(cmd.c_str());
	if (!exit_code) {
		print(file + " success!\npassword is \"" + password + "\"");
		return true;
	}
	return false;
}
static bool building_brute_password(const std::string& sevenzip_path, const std::string& chars, const std::string password, const size_t& len, const std::string& file, const std::string& one_file, std::string& final_password) {
	if (password.size() == len) {
		final_password = password;
		return try_password(sevenzip_path, file, password, one_file);
	}
	for (const auto& c : chars) {
		if (building_brute_password(sevenzip_path, chars, password + c, len, file, one_file, final_password)) {
			return true;
		}
	}
	return false;
}
static bool decompress_zip(const std::string& sevenzip_path, const std::string& password, const std::string& file, const bool& flag_save_zip) {
	std::string cmd = "\"" + sevenzip_path + " x -o\"" + file + "~\" -p\"" + password + "\" -y \"" + file + "\" > NUL 2>&1\"";
	print(cmd);
	int exit_code = system(cmd.c_str());
	if (!exit_code) {
		print("decompress success!");
		if (!flag_save_zip) {
			std::filesystem::remove(file);
		}
		return true;
	}
	if (std::filesystem::exists(file + "~")) {
		std::filesystem::remove_all(file + "~");
	}
	std::cerr << "decompress failed!" << std::endl;
	std::cerr << file + " fail!" << std::endl;
	return false;
}
int main(int argc, char* argv[]) {
	const std::string version = "2024.9.22";
	const std::string program_path = std::filesystem::absolute(argv[0]).parent_path().string();
	const std::string sevenzip_path = "\"" + program_path + "/7z\"";
	const std::string key_json_path = program_path + "/key.json";
	argparse::ArgumentParser program("FileUnlockTool", version);
	program.add_argument("-b", "--brute")
		.help("Brute matching.")
		.flag();
	program.add_argument("-e", "--exact")
		.help("Exact matching.")
		.flag();
	program.add_argument("-f", "--file_name_matching")
		.help("File name matching.")
		.flag();
	program.add_argument("-s", "--save_zip")
		.help("Save the original compressed file after successful decompression.")
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
	const bool& flag_file_name_matching = program.get<bool>("-f");
	const bool& flag_save_zip = program.get<bool>("-s");
	const std::string& file = program.get<std::string>("file");
	if (!std::filesystem::exists(file)) {
		std::cerr << file + " not found!" << std::endl;
		return -114514;
	}
	const std::string& one_file = get_one_file_from_zip(sevenzip_path, file);
	std::ifstream key_json_file(key_json_path);
	nlohmann::json key_json_data = nlohmann::json::parse(key_json_file);
	if (flag_file_name_matching) {
		const std::string& file_name = std::filesystem::path(file).filename().string();
		const size_t& len = file_name.size();
		for (size_t i = 0; i < len; ++i) {
			for (size_t j = 1; j < len - i + 1; ++j) {
				const std::string& password = file_name.substr(i, j);
				if (try_password(sevenzip_path, file, password, one_file)) {
					if (decompress_zip(sevenzip_path, password, file, flag_save_zip)) {
						return 0;
					}
					return -114514;
				}
			}
		}
	}
	if (flag_exact) {
		const std::vector<std::string>& exact_passwords = key_json_data["exact"];
		for (const auto& password : exact_passwords) {
			if (try_password(sevenzip_path, file, password, one_file)) {
				if (decompress_zip(sevenzip_path, password, file, flag_save_zip)) {
					return 0;
				}
				return -114514;
			}
		}
	}
	if (flag_brute) {
		const size_t& min_len = key_json_data["brute"]["min_len"];
		const size_t& max_len = key_json_data["brute"]["max_len"];
		const std::string& chars = key_json_data["brute"]["chars"];
		std::string password = "";
		if (min_len <= max_len && chars != "") {
			for (size_t len = min_len; len <= max_len; ++len) {
				if (building_brute_password(sevenzip_path, chars, "", len, file, one_file, password)) {
					if (decompress_zip(sevenzip_path, password, file, flag_save_zip)) {
						return 0;
					}
					return -114514;
				}
			}
		}
	}
	std::cerr << file + " fail!" << std::endl;
	return -114514;
}
