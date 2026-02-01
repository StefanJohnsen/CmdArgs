# CmdArgs - Command-Line Argument Parser

CmdArgs is a header-only utility that provides an easy-to-use command-line argument parser for small tools and utilities. This solution simplifies handling source and target file paths, flags, and file extensions. The parser is self-contained and requires no third-party dependencies, only standard C++17 headers.

### Compatibility and Dependencies
- C++17 Standard and above
- Standard Template Library (STL)

### OS Support
- Windows
- Linux
- macOS

## Usage

Copy CmdArgs.h to your project and include the file.

CmdArgs.h is defined with demo values (these should be updated and tailored to your project).

The following demo legal extensions are defined:

```cpp
std::vector<std::string> source_ext = { "txt", "csv", "json" };
std::vector<std::string> target_ext = { "csv", "json", "txt" };
```

The following demo flags are defined:

```cpp
cmd_flag convert{ "convert", true };
cmd_flag translate{ "translate" };
cmd_flag help{ "help" };
cmd_flag version{ "version" };
```

***There are also demo version text and help text that need to be adapted.***

# Example

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

Sample Tests (Simulating Command-Line Arguments)

Assume you are in the directory C:\App\MyProgram.exe. Here are a few test scenarios:

### Test: Basic Conversion Command

```bash
C:\App>MyProgram.exe source.txt target.csv
Source: C:\App\source.txt
Target: C:\App\target.csv
Convert is enabled
```
***PS: Convert er default on, og trengs ikke å spesifiseres som vist over**** 

### Test: Basic Conversion Command using default (default target is csv)

```bash
C:\App>MyProgram.exe source.txt
Source: C:\App\source.txt
Target: C:\App\target.csv
Convert is enabled
```

### Test: Translation enabled

```bash
C:\App> MyProgram.exe -translate source.json target.csv
Source: data.json
Target: result.csv
Convert is enabled
Translate is enabled
```

### Test: Invalid Source File Extension

```bash
C:\App> MyProgram.exe source.jpg target.csv
Error: Source file is not a valid extension: file.rvm
```










