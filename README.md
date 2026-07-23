# malscan — Rule-Based Malware Scanner

`malscan` is a lightweight, high-performance, YARA-style file scanner written in **C++**. It recursively loads detection rules from a nested directory structure, reads target files as raw binary (supporting any file format), and reports every rule whose signature is found.

It features a custom execution engine that parses plain-text rule definitions (supporting both raw strings and hex signatures) and utilizes efficient byte-searching algorithms to locate malware indicators, web shells, and obfuscation techniques.

## Features

- **Binary Agnostic**: Reads target files purely as raw bytes, meaning it can scan anything from `.txt` logs to compiled `.exe` or `.elf` binaries without crashing.
- **Advanced Threat Detection**: Includes over 60 signatures for detecting advanced persistent threats, process injection, rootkit patterns, keyloggers, worms, and DNS/DGA Command and Control (C2) communication.
- **Recursive Directory Traversal**: Supports deeply nested rule directories. You can organize your rules into logical categories (e.g., `webshell`, `advanced`, `ransomware`).
- **Flexible Pattern Matching**: Supports matching on exact ASCII strings or Hex byte arrays (e.g., `0F A2` for VM detection).
- **Offset Reporting**: Reports the exact hexadecimal byte offset where a malicious signature was found inside the target file.
- **Error Resilient**: Skips unreadable files, ignores non-rule files, and handles malformed signatures gracefully.

---

## Directory Layout

```text
malscan/
├── main.cpp                    # Core execution engine and scanner (C++11/17)
├── README.md                   # Project documentation
├── .gitignore                  # Git ignore file for binaries
└── rules/                      # Nested rules directory
    ├── advanced/
    │   ├── evasion.rule        # Anti-debugging, VM evasion (CPUID), and Obfuscation (Base64/XOR)
    │   └── indicators.rule     # Process injection, Credential theft, Rootkit hooks, and Worms
    ├── generic/
    │   └── headers.rule        # Standard file headers (ZIP, PE, etc.)
    ├── network/
    │   ├── c2.rule             # Command and control (C2), suspicious User-Agents, and reverse shells
    │   └── dns_c2.rule         # DNS TXT record polling, DGA patterns, and dynamic DNS providers
    ├── pe_tricks/
    │   ├── packers_ext.rule    # Additional PE packer signatures (PECompact, ASPack)
    │   └── pe.rule             # PE obfuscation, packing (UPX, Themida), and suspicious imports
    ├── ransomware/
    │   ├── ransomware.rule     # Ransomware behaviors (vssadmin, bcdedit, etc.)
    │   └── ransomware_ext.rule # Extended ransomware extensions (.LOCKED, .ENCRYPTED)
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
name = Evasion_VMDetection_CPUID
description = Uses CPUID instruction to detect hypervisor (VMware/VirtualBox) presence
hex = 0F A2

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
*(Note: Use `-std=c++17` if your compiler natively supports the `std::filesystem` header. Our source includes C++11 backward compatibility using `dirent.h` for older systems).*

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

Here is a live execution of `malscan` scanning itself using our expanded rule set (which contains 61 rules across various categories). It accurately detects its own PE (Portable Executable) header, standard Windows imports, and flags the `CPUID` hex sequence:

```text
================================================
  malscan - Rule-Based File Scanner
================================================
[*] Target   : malscan.exe
[*] File size: 80969 bytes
[*] Rules dir: rules
[*] Rules    : 61 loaded
------------------------------------------------
[MATCH] Evasion_VMDetection_CPUID | offset=0x28af | Uses CPUID instruction to detect hypervisor (VMware/VirtualBox) presence
[MATCH] MZ_Executable_Header | offset=0x0 | DOS/PE executable magic bytes at file start
[MATCH] PE_Suspicious_Import_LoadLibrary | offset=0x7f00 | Imports LoadLibraryA/W (often used for dynamic API resolution to hide intent)
------------------------------------------------
[RESULT] 3 rule(s) matched.
================================================
```

---

## Exit Codes Integration

The program returns standard exit codes making it highly suitable for CI/CD pipelines, automation scripts, or cron jobs:
- `0` : File is **clean** (no rule matches).
- `1` : **Error** (missing arguments, target file not found, or invalid rules directory).
- `2` : File is **flagged** (one or more malware rules matched).
