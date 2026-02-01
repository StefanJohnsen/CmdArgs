# CmdArgs - Command-Line Argument Parser

CmdArgs is a header-only utility that provides an easy-to-use command-line argument parser designed for small tools and utilities. This solution simplifies the management of source and target file paths, flags, and file extensions. It is a self-contained parser that requires no third-party dependencies, relying only on standard C++17 headers. 

This parser was created after repeatedly having to write similar argument parsers for various tools. I grew tired of doing this manually each time, so I decided to create a small helper parser to simplify the process. I use it regularly and thought that others might benefit from it as well. Feel free to use it and happy coding!

### Compatibility and Dependencies
- C++17 Standard and above
- Standard Template Library (STL)

### OS Support
- Windows
- Linux
- macOS

# Usage

Copy CmdArgs.h to your project and include the file.

CmdArgs.h is defined with demo values (these should be updated and tailored to your project).

The following demo legal extensions are defined:

```cpp
std::vector<std::string> source_ext = { "txt", "csv", "json" };
std::vector<std::string> target_ext = { "csv", "json", "txt" };
```

The following demo flags are defined:

```cpp
cmd_flag convert{ "convert", true };    <- ex where convert is on
cmd_flag translate{ "translate" };
cmd_flag help{ "help" };
cmd_flag version{ "version" };
```

***There are also demo version text and help text that need to be adapted.***

# Example code

```cpp
#include <iostream>
#include "CmdArgs.h"

int main(int argc, char* argv[])
{
    // Parse the command line arguments
    if (!cmd::parse(argc, argv)) return 1;

    // Access parsed arguments
    auto source = cmd::source;
    auto target = cmd::target;

    // Display parsed arguments
    std::cout << "Source: " << source.string() << std::endl;
    std::cout << "Target: " << target.string() << std::endl;

    // Check flags
    if (cmd::convert)
        std::cout << "Convert is enabled" << std::endl;
    if (cmd::translate)
        std::cout << "Translate is enabled" << std::endl;
    if (cmd::help)
        std::cout << "Help is enabled" << std::endl;
    if (cmd::version)
        std::cout << "Version is enabled" << std::endl;

    return 0;
}
```

# Run code (Simulating Command-Line Arguments)

Assume you are in the directory C:\App\MyProgram.exe. Here are a few test scenarios:

### Test: Basic Conversion Command

```bash
C:\App>MyProgram.exe source.txt target.csv
Source: C:\App\source.txt
Target: C:\App\target.csv
Convert is enabled
```

***PS: Convert is enabled by default and does not need to be specified as shown above.****

### Test: Basic Conversion Command using no target (default target ext is csv)

```bash
C:\App>MyProgram.exe source.txt
Source: C:\App\source.txt
Target: C:\App\source.csv      <- same name as source.txt, but with .csv extension
Convert is enabled
```

### Test: Basic Conversion Command using full path

```bash
C:\App>MyProgram.exe C:\App\source.txt C:\temp\target.csv
Source: C:\App\source.txt
Target: C:\temp\target.csv
Convert is enabled
```

### Test: Basic Conversion Command using target directory only

```bash
C:\App>MyProgram.exe C:\App\source.txt C:\temp
Source: C:\App\source.txt
Target: C:\temp\source.csv
Convert is enabled
```

### Test: -translate enabled

```bash
C:\App>MyProgram.exe -translate source.json target.csv
Source: C:\App\source.json
Target: C:\App\target.csv
Convert is enabled
Translate is enabled
```

### Test: Invalid Source File Extension

```bash
C:\App>MyProgram.exe source.jpg target.csv
Error: Source file is not a valid extension: C:\App\source.jpg
```

### Test: Non existing Source File

```bash
C:\App>MyProgram.exe source.cpp
Error: Could not find the source file C:\App\source.cpp
```

### Test: -version enabled

```bash
C:\App>MyProgram.exe -version
MyProgram version: 1.0.0
```

### Test: -help enabled

```bash
C:\App>MyProgram.exe -help
Usage: MyProgram [options] <source_path> [target_path]

Options:

  -convert       Convert the source to the target format (default)
  -translate     Enable translation (must be specified)
  -help          Show this help message
  -version       Show version information

File extensions:

  Source: txt, csv, json
  Target: csv, json, txt

Notes:

  Paths are resolved relative to the current working directory.
  If target_path is given without a directory, it is placed next to the source file.
  If target_path is a directory, output is placed in that directory.
  If target_path is omitted, the output name is derived from the source file.
  Example:  input.txt  ->  input.csv
```

## License
This software is released under the MIT License terms.
