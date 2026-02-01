#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <cstdlib>

/*
  CmdArgs.h

  Header-only helpers for simple console tools.

  This file provides an easy-to-use command-line argument parser for small tools.
  It parses argv into:
    - source file or source directory
    - optional target file or target directory
    - optional flags

  It also performs file system checks for:
    - Verifying that the source path exists before proceeding.
      - If the source is a file, it must exist.
      - If the source is a directory, it must exist and parsing succeeds without requiring a target.
    - Verifying that the target directory exists before proceeding (if specified).
      - If the target is a file, its parent directory must exist.
      - If the target is a directory, it must exist.

  Flags:
    - convert      : Enables or disables conversion (default = true).
    - translate    : Enables or disables translation (default = true).
    - help         : Displays the help text, showing usage instructions.
    - version      : Displays the version information of the program.

  Flags state can be easily checked as boolean values:
    - if (convert)   { Conversion  logic }
    - if (translate) { Translation logic }

  Extension validation:
    - source_ext / target_ext control which extensions are accepted.
    - The target extension is validated after the final target path is resolved.

  Requirements:
    - C++17 (uses std::filesystem and inline variables)

  OS Support:
    - Windows
    - Linux

  License: MIT
  Author: Stefan Falk Johnsen
  Copyright (c) 2024 FalconCoding
*/

namespace cmd
{
    struct cmd_flag
    {
        cmd_flag(std::string flag, bool on = false)
            : _flag(std::move(flag)), on(on), defaultOn(on) {
        }

        void clear() { on = defaultOn; }

        operator bool() const { return on; }
        operator std::string() const { return _flag; }

        void operator=(bool set) { on = set; }

        const std::string& name() const { return _flag; }
    private:
        const std::string _flag;
        bool on = false;
        bool defaultOn = false;
    };

    std::string join(const std::vector<std::string>& v, const char* delim);

    //-[ CmdArgs Setup for Your Program ]----------------------------------------------------------------------

    // Set your program name and version here
    inline std::string text_version = "MyProgram version: 1.0.0";

    // Set your accepted source and target extensions here (first is default)
    inline const std::vector<std::string> source_ext = { "txt", "csv", "json" };
    inline const std::vector<std::string> target_ext = { "csv", "json", "txt" };

    // Set your command line flags here (true means you enable the flag by default)
    inline cmd_flag convert{ "convert", true };
    inline cmd_flag translate{ "translate"};
    inline cmd_flag help{ "help" };
    inline cmd_flag version{ "version" };

    // Register your flags here (help and version must be included)
    inline const std::vector<cmd_flag*> cmd_flags = { &convert, &translate, &help, &version };

    // Define your help text here
    inline std::string text_help = []()
        {
            std::string s;

            s += "\nUsage: MyProgram [options] <source_path> [target_path]\n\n";
            s += "Options:\n\n";
            s += "  -convert       Convert the source to the target format (default)\n";
            s += "  -translate     Enable translation (must be specified)\n";
            s += "  -help          Show this help message\n";
            s += "  -version       Show version information\n";
            s += "\n";
            s += "File extensions:\n\n";
            s += "  Source: ";
            s += join(source_ext, ", ");
            s += "\n";
            s += "  Target: ";
            s += join(target_ext, ", ");
            s += "\n\n";
            s += "Notes:\n\n";
            s += "  Paths are resolved relative to the current working directory.\n";
            s += "  If target_path is given without a directory, it is placed next to the source file.\n";
            s += "  If target_path is a directory, output is placed in that directory.\n";
            s += "  If target_path is omitted, the output name is derived from the source file.\n";
            s += "  Example:  input.txt  ->  input.csv\n";
            return s;
        }();

    //-[ CmdArgs Global Functions ]------------------------------------------------------------

    // Here are the global variables where the parsed source and target paths are stored
    // after parsing (if you prefer global parsing).
    inline std::filesystem::path source;
    inline std::filesystem::path target;

    // Function to get the default extension (the first in the list)
    static std::string defaultExt(const std::vector<std::string>& list)
    {
        return list.empty() ? std::string() : list.front();
    }

    // Returns the default source extension (from the source_ext list)
    static std::string defaultSourceExt()
    {
        return defaultExt(source_ext);
    }

    // Returns the default target extension (from the target_ext list)
    static std::string defaultTargetExt()
    {
        return defaultExt(target_ext);
    }

    //-----------------------------------------------------------------------------------------

    inline std::string tolower(std::string txt)
    {
        std::transform(txt.begin(), txt.end(), txt.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return txt;
    }

    inline std::string getExtension(const std::filesystem::path& file)
    {
        auto ext = tolower(file.extension().string());

        if (!ext.empty() && ext.front() == '.')
            ext.erase(ext.begin());

        return ext;
    }

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

    //-----------------------------------------------------------------------------------------
    // Command-line Argument Parser Class
    //
    // This class allows you to parse command-line arguments directly, 
    // providing an alternative to using global parsing. If you prefer 
    // to work with the parsed arguments in a more encapsulated manner, 
    // you can use this class.
    //
    // Usage example:
    // 
    //    CmdArgumentParser parser;
    // 
    //    if (!parser.parse(argc, argv)) return 1;
    // 
    //    auto source = cmd::source;  // Parsed source path (from global)
    //    auto target = cmd::target;  // Parsed target path (from global)
    //
    // After parsing, you can check the flags using the global flag variables:
    // 
    //    if (cmd::convert)   { ... }   // Checks if '-convert' was specified (default: true)
    //    if (cmd::translate) { ... }   // Checks if '-translate' was specified
    //
    // The flags are globally accessible, even after parsing.
    //-----------------------------------------------------------------------------------------

    class CmdArgumentParser
    {
    public:
        CmdArgumentParser() = default;

        std::filesystem::path source() const { return _source; }
        std::filesystem::path target() const { return _target; }

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

        static bool err(const std::string& msg)
        {
            std::cerr << "Error: " << msg << std::endl;
            return false;
        }

        static void info(const std::string& msg, int code)
        {
            std::cout << msg << std::endl;
            std::exit(code);
        }

        void check()
        {
            cmd::source.clear();
            cmd::target.clear();

            for (auto* f : cmd_flags) f->clear();

            if (cmd::source_ext.empty() || cmd::target_ext.empty())
                throw std::runtime_error("Error: source_ext or target_ext is not defined.");
            if (!contains(cmd_flags, &help))
                throw std::runtime_error("Error: cmd_flags must contain 'help' flag.");
            if (!contains(cmd_flags, &version))
                throw std::runtime_error("Error: cmd_flags must contain 'version' flag.");
            if (cmd_flags.size() < 3)
                throw std::runtime_error("Error: cmd_flags is not defined.");
        }

        bool parseFlags(const std::vector<std::string>& flags, const size_t countFiles)
        {
            size_t countFlags = 0;

            for (const auto& flag : flags)
            {
                for (auto* f : cmd_flags)
                {
                    const std::string s = *f;

                    if (flag == "-" + s || flag == "--" + s)
                    {
                        *f = true;
                        ++countFlags;
                        goto next_flag;
                    }
                }

                return err("argument: Unknown flag " + flag);

            next_flag:
                ;
            }

            if (help || version)
            {
                if (countFlags > 1 || countFiles != 0)
                    return err("argument: Too many arguments");

                if (help)
                    info(text_help, 0);

                info(text_version, 0);
            }

            return true;
        }

        bool parseFiles(const std::vector<std::string>& files)
        {
            if (files.empty())
                return err("No source file specified");

            if (files.size() > 2)
                return err("argument: Too many arguments");

            _source = files[0];

            // Source directory mode: accept existing directory, no target required.
            if (std::filesystem::is_directory(_source))
            {
                if (files.size() > 1)
                    return err("argument: Too many arguments");

                cmd::source = _source;
                cmd::target.clear();
                return true;
            }

            // If source has no parent path, treat it as relative to the current working directory.
            if (_source.parent_path().empty())
                _source = std::filesystem::current_path() / _source;

            if (!std::filesystem::exists(_source))
                return err("Could not find the source file " + files[0]);

            if (!contains(source_ext, getExtension(_source)))
                return err("Source file is not a valid extension: " + _source.string());

            // Default target is source with default extension.
            _target = _source;
            _target.replace_extension(defaultTargetExt());

            if (files.size() == 2)
            {
                _target = files[1];

                // If target has no parent path, treat it as relative to the source directory.
                if (_target.parent_path().empty())
                    _target = _source.parent_path() / _target;

                // If target has a parent path, ensure the parent is a directory.
                if (!_target.parent_path().empty() && !std::filesystem::is_directory(_target.parent_path()))
                {
                    if (_target.has_extension())
                        return err("Target file has unknown directory " + _target.string());

                    return err("Target directory does not exist " + _target.string());
                }

                // If target has no extension, interpret it as a directory.
                if (!_target.has_extension())
                {
                    if (!std::filesystem::is_directory(_target))
                        return err("Target directory does not exist " + _target.string());

                    _target = _target / _source.filename();
                    _target.replace_extension(defaultTargetExt());
                }
            }

            if (tolower(_source.string()) == tolower(_target.string()))
                return err("Source and target files are the same");

            if (!contains(target_ext, getExtension(_target)))
                return err("Target file is not a valid extension: " + _target.string());

            cmd::source = _source;
            cmd::target = _target;

            return true;
        }

        std::filesystem::path _source, _target;
    };

    //---------------------------------------------------------------------------------------------------------
    // Command-line Argument Parser for Global Parsing
    //
    // Usage example:
    // 
    // If you prefer using global parsing (recommended), follow this approach:
    //
    //    if (!cmd::parse(argc, argv)) return 1;  // Parse the arguments
    //
    //    auto source = cmd::source;  // Parsed source file or directory
    //    auto target = cmd::target;  // Parsed target file or directory
    //
    // To check flags, you can use the following pattern:
    //
    //    if (cmd::convert)   { ... }   // Checks if '-convert' was specified (default: true)
    //    if (cmd::translate) { ... }   // Checks if '-translate' was specified
    //
    // Additionally, you can retrieve the default extensions like this:
    //
    //    auto defaultSourceExt = cmd::defaultSourceExt();  // Default source extension (first in source_ext)
    //    auto defaultTargetExt = cmd::defaultTargetExt();  // Default target extension (first in target_ext)
    //
    //---------------------------------------------------------------------------------------------------------        

    static bool parse(int argc, char* argv[])
    {
        CmdArgumentParser command;
        return command.parse(argc, argv);
    }
}
