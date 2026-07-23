# malscan — Rule-Based Malware Scanner

`malscan` is a lightweight, high-performance, YARA-style file scanner written in **C++**. It recursively loads detection rules from a nested directory structure, reads target files as raw binary (supporting any file format), and reports every rule whose signature is found.

It features a custom execution engine that parses plain-text rule definitions (supporting both raw strings and hex signatures) and utilizes efficient byte-searching algorithms to locate malware indicators, web shells, and obfuscation techniques.

## Features

- **Binary Agnostic**: Reads target files purely as raw bytes, meaning it can scan anything from `.txt` logs to compiled `.exe` or `.elf` binaries without crashing.
- **Recursive Directory Traversal**: Supports deeply nested rule directories. You can organize your rules into logical categories (e.g., `webshells`, `pe_tricks`).
- **Flexible Pattern Matching**: Supports matching on exact ASCII strings or Hex byte arrays (e.g., `4D 5A 90 00`).
- **Offset Reporting**: Reports the exact hexadecimal byte offset where a malicious signature was found inside the target file.
- **Error Resilient**: Skips unreadable files, ignores non-rule files, and handles malformed signatures gracefully.

---

## Directory Layout

```text
malscan/
├── main.cpp                    # Core execution engine and scanner
├── README.md                   # Project documentation
├── .gitignore                  # Git ignore file for binaries
└── rules/                      # Nested rules directory
    ├── generic/
    │   └── headers.rule        # Standard file headers (ZIP, PE, etc.)
    ├── network/
    │   └── c2.rule             # Command and control (C2) and reverse shell indicators
    ├── pe_tricks/
    │   └── pe.rule             # PE obfuscation, packing (UPX, Themida), and suspicious imports
    ├── ransomware/
    │   └── ransomware.rule     # Ransomware behaviors (vssadmin, bcdedit, etc.)
    ├── webshell/
    │   └── webshell.rule       # Web shell backdoors (PHP eval, ASP, JSP, China Chopper)
    └── test/
        └── eicar/
            └── eicar.rule      # Industry-standard EICAR test string
```

---

## Rule Format

Rules are stored in plain text files with a `.rule` extension. A single file can contain multiple `[rule]` blocks. 

The scanner applies **OR semantics** — if a rule contains multiple `string` or `hex` patterns, the rule fires if *any* of them match.

### Example Rule File:
```ini
# Lines starting with # are ignored comments.

[rule]
name = MZ_Executable_Header
description = DOS/PE executable magic bytes at file start
hex = 4D 5A

[rule]
name = Webshell_PHP_Eval_Base64
description = PHP eval(base64_decode()) pattern used in web shells
string = eval(base64_decode(
```

---

## Build Instructions

`malscan` is written in standard C++ and has zero external dependencies.

**Using GCC / MinGW:**
```bash
g++ -std=c++11 -O2 -o malscan main.cpp
```
*(Note: Use `-std=c++17` if your compiler natively supports the `std::filesystem` header, or compile the compatible POSIX version if using older MinGW).*

**Using Clang:**
```bash
clang++ -std=c++11 -O2 -o malscan main.cpp
```

**Using MSVC (Visual Studio):**
```cmd
cl /EHsc /O2 /Fe:malscan.exe main.cpp
```

---

## Usage & Execution

```bash
Usage: ./malscan <target_file> <rules_dir>
Exit codes: 0=clean  1=error  2=match(es) found
```

### Example Output

Here is a live execution of `malscan` scanning itself using our expanded rule set (which contains 36 rules across various categories). It accurately detects its own PE (Portable Executable) header and standard Windows imports:

```text
================================================
  malscan - Rule-Based File Scanner
================================================
[*] Target   : malscan.exe
[*] File size: 79602 bytes
[*] Rules dir: rules
[*] Rules    : 36 loaded
------------------------------------------------
[MATCH] MZ_Executable_Header | offset=0x0 | DOS/PE executable magic bytes at file start
[MATCH] PE_Suspicious_Import_LoadLibrary | offset=0x7d10 | Imports LoadLibraryA/W (often used for dynamic API resolution to hide intent)
------------------------------------------------
[RESULT] 2 rule(s) matched.
================================================
```

---

## Exit Codes Integration

The program returns standard exit codes making it highly suitable for CI/CD pipelines, automation scripts, or cron jobs:
- `0` : File is **clean** (no rule matches).
- `1` : **Error** (missing arguments, target file not found, or invalid rules directory).
- `2` : File is **flagged** (one or more malware rules matched).
