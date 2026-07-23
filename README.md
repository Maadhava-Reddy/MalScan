# MalScan — Rule-Based Malware Scanner

MalScan is a lightweight, YARA-style file scanner written in C++. It recursively loads detection rules from a nested rules directory, reads a target file as raw binary data (so it works on any file type), and reports every rule whose signature is found inside it — along with the exact byte offset of the match.

---

## What it does

- Loads detection rules from `.rule` files stored anywhere under a `rules/` directory, no matter how deeply nested.
- Reads the target file purely as raw bytes, so it can scan text files, executables (`.exe`), Linux binaries (`.elf`), or anything else without needing to understand the file format.
- Searches the target's bytes for every pattern defined in every loaded rule (plain ASCII strings or hex byte sequences).
- Prints the rule name, description, and the exact hex offset where each match was found.
- Returns an exit code so it can be used in scripts, CI pipelines, or automated checks.

---

## Project structure

```
malscan/
├── main.cpp                    # Core execution engine and scanner
├── README.md                   # This file
├── .gitignore                  # Git ignore file for compiled binaries
└── rules/                      # Nested rules directory
    ├── advanced/
    │   ├── evasion.rule        # Anti-debugging, VM evasion (CPUID), obfuscation (Base64/XOR)
    │   └── indicators.rule     # Process injection, credential theft, rootkit hooks, worms
    ├── generic/
    │   └── headers.rule        # Standard file headers (ZIP, PE, etc.)
    ├── network/
    │   ├── c2.rule              # Command-and-control, suspicious user agents, reverse shells
    │   └── dns_c2.rule          # DNS TXT polling, DGA patterns, dynamic DNS providers
    ├── pe_tricks/
    │   ├── packers_ext.rule    # Additional PE packer signatures (PECompact, ASPack)
    │   └── pe.rule              # PE obfuscation, packing (UPX, Themida), suspicious imports
    ├── ransomware/
    │   ├── ransomware.rule      # Ransomware behaviors (vssadmin, bcdedit, etc.)
    │   └── ransomware_ext.rule # Extended ransomware extensions (.LOCKED, .ENCRYPTED)
    ├── webshell/
    │   └── webshell.rule        # Web shell backdoors (PHP eval, ASP, JSP, China Chopper)
    └── test/
        └── eicar/
            └── eicar.rule        # Industry-standard EICAR antivirus test string
```

Currently ships with **61 rules** across 6 categories.

---

## Rule format

Rules are plain text files with a `.rule` extension. A single file can contain multiple `[rule]` blocks. Each block needs a `name`, a `description`, and one or more `string=` / `hex=` patterns. If a rule has multiple patterns, it fires when **any one** of them is found (OR logic).

```ini
# Lines starting with # are comments

[rule]
name = Evasion_VMDetection_CPUID
description = Uses CPUID instruction to detect hypervisor (VMware/VirtualBox) presence
hex = 0F A2

[rule]
name = Webshell_PHP_Eval_Base64
description = PHP eval(base64_decode()) pattern used in web shells
string = eval(base64_decode(
```

To add detection coverage, drop a new `.rule` file anywhere under `rules/` (any subfolder depth works) and rerun the scanner — no code changes needed.

---

## How to build it

No external dependencies — just a C++ compiler.

**GCC / MinGW:**
```bash
g++ -std=c++11 -O2 -o malscan main.cpp
```

**Clang:**
```bash
clang++ -std=c++11 -O2 -o malscan main.cpp
```

**MSVC (Visual Studio, Windows):**
```cmd
cl /EHsc /O2 /Fe:malscan.exe main.cpp
```

---

## How to run it

```
malscan.exe <target_file> <rules_dir>
```

On Windows, if you're in PowerShell and the exe is in your current folder, prefix it with `.\`:
```
.\malscan.exe <target_file> rules
```

`malscan.exe` and `rules/` should sit side by side (siblings) in the same project folder. If you run the command from that folder, `rules` (relative path) is enough. If you run it from elsewhere, give the full path to both the target file and the rules folder.

### Exit codes
| Code | Meaning |
|---|---|
| `0` | Clean — no rules matched |
| `1` | Error — bad arguments, missing target file, or missing rules directory |
| `2` | Flagged — one or more rules matched |

This makes it easy to wire into a script:
```bash
malscan.exe samples/test_suspicious.bin rules
if %errorlevel%==2 echo File flagged!
```

---

## Verified test results

These were run on this project and confirmed working end to end.

**Clean file (`samples/clean_test.txt`) — expect no matches:**
```
================================================
  malscan - Rule-Based File Scanner
================================================
[*] Target   : samples/clean_test.txt
[*] File size: 74 bytes
[*] Rules dir: rules
[*] Rules    : 61 loaded
------------------------------------------------
------------------------------------------------
[CLEAN] No matches found.
================================================
```
Exit code: `0`

**EICAR test string (`samples/eicar_test.txt`) — the industry-standard, harmless AV test file:**
```
================================================
  malscan - Rule-Based File Scanner
================================================
[*] Target   : samples/eicar_test.txt
[*] File size: 67 bytes
[*] Rules dir: rules
[*] Rules    : 61 loaded
------------------------------------------------
[MATCH] EICAR_Test_File | offset=0x1b | Standard EICAR antivirus test string (not real malware)
------------------------------------------------
[RESULT] 1 rule(s) matched.
================================================
```
Exit code: `2`

**Synthetic multi-signature file (`samples/test_suspicious.bin`) — embeds a PE header, a UPX packer marker, a CPUID hex pattern, a ransomware shadow-copy-deletion string, and a PHP web shell string:**
```
================================================
  malscan - Rule-Based File Scanner
================================================
[*] Target   : samples/test_suspicious.bin
[*] File size: 142 bytes
[*] Rules dir: rules
[*] Rules    : 61 loaded
------------------------------------------------
[MATCH] Evasion_VMDetection_CPUID | offset=0x5d | Uses CPUID instruction to detect hypervisor (VMware/VirtualBox) presence
[MATCH] MZ_Executable_Header | offset=0x0 | DOS/PE executable magic bytes at file start
[MATCH] PE_UPX_Packed | offset=0x1e | UPX packer signature commonly used to compress malware
[MATCH] Ransomware_Shadow_Copy_Delete | offset=0x6b | Attempts to delete Windows Volume Shadow Copies (common ransomware behavior)
[MATCH] Webshell_PHP_System_Command | offset=0x32 | PHP system() call used for OS command execution in shells
------------------------------------------------
[RESULT] 5 rule(s) matched.
================================================
```
Exit code: `2`

All rule counts, offsets, and match names were cross-checked against the raw rule files (61 `[rule]` blocks counted directly in the `rules/` folder matches the `61 loaded` the program reports), confirming the recursive traversal, parser, and matching engine are all working correctly together.

---

## Known limitations

- Pattern logic within a rule is OR-only — there's no AND/NOT condition support yet.
- No regex matching, only exact string and exact/wildcard-free hex sequences.
- The whole target file is loaded into memory, so very large files (multi-GB) aren't handled efficiently.
