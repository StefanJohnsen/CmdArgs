#pragma once
#include <cmath>
#include <string>
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

  This file provides an easy-to-use command-line argument parser for small tools.
  It parses argv into:
    - source file + optional target file
    - optional flags

  It also performs file existence checks for:
    - Verifying that the source file exists before proceeding.
    - Verifying that the target directory exists before proceeding (if specified).

  Flags:
    - convert      : Enables or disables the conversion (default: true).
    - translate    : Enables or disables translation (default: true).
    - help         : Displays the help text, showing usage instructions.
    - version      : Displays the version information of the program.

  Flags state can be easily checked as boolean values:
    - if (convert)   { Conversion logic }
    - if (translate) { Translation logic }

  Extension validation:
    - source_ext / target_ext control which extensions are accepted.

  Requirements:
    - C++17 (uses std::filesystem and inline variables)

  License: MIT
  Copyright (c) 2024 FalconCoding
  Author: Stefan Falk Johnsen
*/

namespace cmd
{
    using namespace std;
    using namespace filesystem;

    struct cmd_flag
    {
        cmd_flag(string flag, bool on = false)
            : _flag(move(flag)), on(on) {
        }

        operator bool() const { return on; }
        operator string() const { return _flag; }

        void operator=(bool set) { on = set; }

        const string& name() const { return _flag; }
    private:
        const string _flag;
        bool on = false;
    };

    inline string text_version = "MyProgram version: 1.0.0";

    inline const vector<string> source_ext = { "txt", "csv", "json" };
    inline const vector<string> target_ext = { "csv", "json", "txt" };

    inline cmd_flag convert{ "convert", true };
    inline cmd_flag translate{ "translate", true };
    inline cmd_flag help{ "help" };
    inline cmd_flag version{ "version" };

    inline const std::vector<cmd_flag*> cmd_flags = { &convert, &translate, &help, &version };

    inline string join(const vector<string>& v, const char* delim)
    {
        string out;
        for (size_t i = 0; i < v.size(); ++i)
        {
            out += v[i];
            if (i + 1 < v.size())
                out += delim;
        }
        return out;
    }

    inline string text_help = []()
        {
            string s;

            s += "Usage: MyProgram [options] <source_file> [target_file]\n";
            s += "\n";
            s += "Options:\n";
            s += "  -convert       Convert the source file to the target format (default)\n";
            s += "  -translate     Enable translation (default)\n";
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

    static string defaultTargetExt()
    {
        return target_ext.empty() ? string() : target_ext.front();
    }

    inline string tolower(string txt)
    {
        transform(txt.begin(), txt.end(), txt.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return txt;
    }

    inline string getExtension(const path& file)
    {
        auto ext = tolower(file.extension().string());

        if (!ext.empty() && ext.front() == '.')
            ext.erase(ext.begin());

        return ext;
    }

    class CmdArgumentParser
    {
    public:
        CmdArgumentParser() = default;

        path source() const { return _source; }
        path target() const { return _target; }

        bool parse(int argc, char* argv[])
        {
            check();

            vector<string> flags;
            vector<string> files;

            for (int i = 1; i < argc; ++i)
            {
                string arg = argv[i] ? argv[i] : "";
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
            return find(list.begin(), list.end(), value) != list.end();
        }

        static bool err(const string& msg)
        {
            cerr << "Error: " << msg << "\n";
            return false;
        }

        [[noreturn]] static void info(const string& msg, int code)
        {
            cout << msg << "\n";
            exit(code);
        }

        void check()
        {
            cmd::source.clear();
            cmd::target.clear();

            for (auto* f : cmd_flags)
                *f = false;

            convert = true;
            translate = true;

            if (cmd::source_ext.empty() || cmd::target_ext.empty())
                throw runtime_error("Error: source_ext or target_ext is not defined.");
            if (cmd::source_ext == cmd::target_ext)
                throw runtime_error("Error: source_ext and target_ext cannot be the same.");
            if (!contains(cmd_flags, &help))
                throw runtime_error("Error: cmd_flags must contain 'help' flag.");
            if (!contains(cmd_flags, &version))
                throw runtime_error("Error: cmd_flags must contain 'version' flag.");
            if (cmd_flags.size() < 3)
                throw runtime_error("Error: cmd_flags is not defined.");
        }

        bool parseFlags(const vector<string>& flags, const size_t countFiles)
        {
            size_t countFlags = 0;

            for (const auto& flag : flags)
            {
                for (auto* f : cmd_flags)
                {
                    const string s = *f;

                    if (flag == "-" + s || flag == "--" + s)
                    {
                        *f = true;
                        ++countFlags;
                        goto next_flag;
                    }
                }

                return err(string("argument: Unknown flag ") + flag);

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

        bool parseFiles(const vector<string>& files)
        {
            if (files.empty())
                return err("No source file specified");

            if (files.size() > 2)
                return err("argument: Too many arguments");

            _source = files[0];

            if (is_directory(_source))
            {
                if (files.size() > 1)
                    return err("argument: Too many arguments");

                cmd::source = _source;
                cmd::target = _target;
                return true;
            }

            if (!exists(_source))
                return err(string("Could not find the source file ") + files[0]);

            const string extension = getExtension(_source);

            if (!contains(source_ext, extension))
                return err(string("Source file is not a valid extension: ") + _source.string());

            _target = _source;
            _target.replace_extension(defaultTargetExt());

            if (files.size() == 2)
            {
                _target = files[1];

                if (_target.parent_path().empty())
                    _target = _source.parent_path() / _target.filename();

                if (!is_directory(_target.parent_path()))
                    return err(string("Target file has unknown directory ") + _target.string());
            }

            if (tolower(_source.string()) == tolower(_target.string()))
                return err("Source and target files are the same");

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
