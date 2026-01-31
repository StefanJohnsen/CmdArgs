#pragma once
#include <cmath>
#include <string>
#include <ranges>
#include <array>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <vector>
#include <utility>

/*
  CmdArgs.h
 
  Header-only helpers for simple console tools.
 
  This file provides an easy-to-use command-line argument parser for simple tools.
  It supports parsing command-line arguments (argv) into source/target files and flags.
  The following flags are available for controlling the program's behavior:
 
  Flags:
    - convert      : Enables or disables the conversion of the source file to the target format (default: true).
    - compress     : Enables or disables compression during conversion.
    - async        : Enables or disables asynchronous execution.
    - scale        : Enables or disables scaling during conversion.
    - help         : Displays the help text, showing usage instructions.
    - version      : Displays the version information of the program.
   
  Flags state can be easily checked as boolean values:
    - if (convert)  { Conversion logic  }
    - if (compress) { Compression logic  }
    - if (async)    { Asynchronous logic  }
 
  Requirements:
    - C++20 (uses std::ranges)
 
  License: MIT
  Copyright (c) 2024 FalconCoding
  Author: Stefan Falk Johnsen
*/
namespace cmd
{
	using namespace std::filesystem;

	/*
	  cmd_flag: Helper structure for handling command-line flags.

	  This structure represents a command-line flag (e.g., `-convert`, `-async`), storing the flag's name and whether it is enabled or disabled.

	  Features:
	  - Supports setting and checking the state of the flag.
	  - Allows flag names to be used both as strings and boolean values.
	*/

	struct cmd_flag
	{
		cmd_flag(std::string flag, bool on = false)
			: _flag(std::move(flag)), on(on) {
		}

		operator bool() const { return on; }
		operator std::string() const { return _flag; }

		void operator=(bool set) { on = set; }

		const std::string& name() const { return _name; }
	private:
		const std::string _flag;
		const std::string _name;
		bool on = false;
	};

	inline std::string text_version = "MyProgram version: 1.0.0";

	inline const std::vector<std::string> source_ext = { "rvm", "obj" };
	inline const std::vector<std::string> target_ext = { "obj", "gltf", "ciff", "dat", "txt" };

	inline cmd_flag convert{ "convert", true };
	inline cmd_flag compress{ "compress" };
	inline cmd_flag async{ "async" };
	inline cmd_flag scale{ "scale" };
	inline cmd_flag help{ "help" };
	inline cmd_flag version{ "version" };

	inline const std::array cmd_flags = { &convert, &compress, &async, &scale, &help, &version };

	/*
	  join: Helper function to join a list of strings with a delimiter.

	  Arguments:
	  - v: The vector of strings to join.
	  - delim: The delimiter string to insert between the elements.

	  Returns:
	  - A single string with all elements joined by the delimiter.
	*/
	inline std::string join(const std::vector<std::string>& v, const char* delim)
	{
		std::string out;
		for (size_t i = 0; i < v.size(); ++i)
		{
			out += v[i];
			if (i + 1 < v.size())
				out += delim;
		}
		return out;
	}

	/*
	  text_help: Displays the help text with usage instructions for the program.

	  Features:
	  - Describes the command-line options supported by the program.
	  - Lists the valid file extensions for source and target files.
	  - Provides notes on how to use the program when target files are omitted.
	*/

	inline std::string text_help = []()
		{
			std::string s;

			s += "Usage: MyProgram [options] <source_file> [target_file]\n";
			s += "\n";
			s += "Options:\n";
			s += "  -convert       Convert the source file to the target format (default)\n";
			s += "  -compress      Compress output (if supported)\n";
			s += "  -async         Run conversion asynchronously\n";
			s += "  -scale         Apply scaling (if supported)\n";
			s += "  -help          Show this help message\n";
			s += "  -version       Show version information\n";
			s += "\n";
			s += "File extensions:\n";
			s += "  Source: ";
			s += join(source_ext, ", ");
			s += "\n";
			s += "  Target: ";
			s += join(target_ext, ", ");
			s += "\n";
			s += "\n";
			s += "Notes:\n";
			s += "  If target_file is omitted, output defaults to source name with the default target extension.\n";

			return s;
		}();

	inline path source;
	inline path target;

	// Get default extension for source file
	static std::string defaultSourceExt()
	{
		return source_ext.empty() ? std::string() : source_ext.front();
	}

	// Get default extension for target file
	static std::string defaultTargetExt()
	{
		return target_ext.empty() ? std::string() : target_ext.front();
	}

	inline std::string tolower(std::string txt)
	{
		std::ranges::transform(txt, txt.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return txt;
	}

	std::string getExtension(const path& file)
	{
		auto ext = tolower(file.extension().string());

		if (!ext.empty() && ext.front() == '.')
			ext.erase(ext.begin());

		return ext;
	}

	/*
	  CmdArgumentParser: A class for parsing command-line arguments.

	  This class handles parsing the source file, target file, and optional flags. It validates the flags and files, and ensures that required arguments are provided. The parsed source and target paths are made available to other parts of the program.

	  Methods:
	  - parse(int argc, char* argv[]): Parses command-line arguments and returns true if successful.
	  - source(): Returns the parsed source file path.
	  - target(): Returns the parsed target file path.
	*/

	class CmdArgumentParser
	{
	public:
		CmdArgumentParser() = default;

		path source() const { return _source; }
		path target() const { return _target; }

		// Parse the command-line arguments (argc, argv)
		bool parse(int argc, char* argv[])
		{
			check();

			std::vector<std::string> flags;
			std::vector<std::string> files;

			for (int i = 1; i < argc; ++i)
			{
				std::string arg = argv[i] ? argv[i] : "";

				if (arg.empty()) continue;

				if (arg.front() == '-')
					flags.emplace_back(arg);
				else
					files.emplace_back(arg);
			}

			if (!parseFlags(flags, files.size()))
				return false;

			return parseFiles(files);
		}

	private:

		template <class T, class V>
		static bool contains(const T& list, const V& value)
		{
			return std::find(list.begin(), list.end(), value) != list.end();
		}

		void check()
		{
			cmd::source.clear();
			cmd::target.clear();

			for (auto* f : cmd_flags)
				*f = false;

			convert = true;

			// Validate source and target extensions
			if (cmd::source_ext.empty() || cmd::target_ext.empty())
				throw std::runtime_error("Error: source_ext or target_ext is not defined.");
			if (cmd::source_ext == cmd::target_ext)
				throw std::runtime_error("Error: source_ext and target_ext cannot be the same.");

			if (!contains(cmd_flags, &help))
				throw std::runtime_error("Error: cmd_flags must contain 'help' flag.");
			if (!contains(cmd_flags, &version))
				throw std::runtime_error("Error: cmd_flags must contain 'version' flag.");
			if (cmd_flags.size() < 3)
				throw std::runtime_error("Error: cmd_flags is not defined.");
		}

		// Evaluate and validate flags passed in the command-line arguments.
		bool parseFlags(const std::vector<std::string>& flags, const size_t countFiles)
		{
			size_t countFlags = 0;

			for (const auto& flag : flags)
			{
				bool found = false;

				for (auto* cmd_flag : cmd_flags)
				{
					const std::string s_flag = *cmd_flag;

					if (flag == "-" + s_flag || flag == "--" + s_flag)
					{
						*cmd_flag = true;
						found = true;
						countFlags++;
						break;
					}
				}

				if (found) continue;

				std::cout << "Error argument: Unknown flag " << flag << std::endl;
				return false;
			}

			if (help || version)
			{
				if (countFlags > 1 || countFiles != 0)
				{
					std::cout << "Error argument: Too many arguments" << std::endl;
					exit(1);
				}

				if (help)
				{
					std::cout << text_help << std::endl;
					exit(0);
				}
				if (version)
				{
					std::cout << text_version << std::endl;
					exit(0);
				}

				return false;
			}

			return true;
		}

		bool parseFiles(const std::vector<std::string>& files)
		{
			if (files.empty())
			{
				std::cout << "Error: No source file specified" << std::endl;
				return false;
			}

			if (files.size() > 2)
			{
				std::cout << "Error argument: Too many arguments" << std::endl;
				return false;
			}

			_source = files[0];

			// Declare 'extension' early so a `goto` does not bypass its initialization
			std::string extension;

			if (is_directory(_source))
			{
				if (files.size() > 1)
				{
					std::cout << "Error argument: Too many arguments" << std::endl;
					return false;
				}

				goto exit;
			}

			if (!exists(_source))
			{
				std::cout << "Error: Could not find the source file " << files[0] << std::endl;
				return false;
			}

			extension = getExtension(_source);

			if (!contains(source_ext, extension))
			{
				std::cout << "Error: Source file is not a valid extension: " << _source.string() << std::endl;
				return false;
			}

			_target = _source;
			_target.replace_extension(defaultTargetExt());

			if (files.size() == 2)
			{
				_target = files[1];

				if (_target.parent_path().empty())
					_target = _source.parent_path() / _target.filename();

				if (!is_directory(_target.parent_path()))
				{
					std::cout << "Error: Target file has unknown directory " << _target.string() << std::endl;
					return false;
				}
			}

			if (tolower(_source.string()) == tolower(_target.string()))
			{
				std::cout << "Error: Source and target files are the same" << std::endl;
				return false;
			}

		exit:

			cmd::source = _source;
			cmd::target = _target;

			return true;
		}

		path _source, _target;
	};

	static bool parse(int argc, char* argv[])
	{
		CmdArgumentParser command;
		return command.parse(argc, argv);
	}
}
