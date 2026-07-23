#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <sstream>

struct Rule {
    std::string name;
    std::string description;
    std::vector<std::vector<uint8_t>> patterns;
};

static std::string trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static bool endsWith(const std::string &s, const std::string &suf) {
    return s.size() >= suf.size() &&
           s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

static std::vector<uint8_t> strBytes(const std::string &s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

static std::vector<uint8_t> parseHex(const std::string &s) {
    std::vector<uint8_t> v;
    std::istringstream is(s);
    std::string t;
    while (is >> t) {
        if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) t = t.substr(2);
        if (t.empty()) continue;
        v.push_back((uint8_t)std::strtol(t.c_str(), nullptr, 16));
    }
    return v;
}

static void loadRuleFile(const std::string &path, std::vector<Rule> &out) {
    std::ifstream f(path);
    if (!f) return;
    std::string line;
    Rule cur;
    bool in = false;
    auto push = [&]() {
        if (in && !cur.name.empty() && !cur.patterns.empty())
            out.push_back(cur);
    };
    while (std::getline(f, line)) {
        std::string l = trim(line);
        if (l.empty() || l[0] == '#') continue;
        if (l == "[rule]" || l == "[Rule]") {
            push(); cur = Rule(); in = true; continue;
        }
        size_t eq = l.find('=');
        if (eq == std::string::npos) continue;
        std::string k   = trim(l.substr(0, eq));
        std::string val = trim(l.substr(eq + 1));
        if (k == "name")             cur.name = val;
        else if (k == "description") cur.description = val;
        else if (k == "string")      cur.patterns.push_back(strBytes(val));
        else if (k == "hex")         cur.patterns.push_back(parseHex(val));
    }
    push();
}

static void walkDir(const std::string &dir, std::vector<Rule> &out) {
    DIR *d = opendir(dir.c_str());
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != nullptr) {
        std::string name = ent->d_name;
        if (name == "." || name == "..") continue;
        std::string full = dir + "/" + name;
        struct stat st;
        if (stat(full.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode))
            walkDir(full, out);
        else if (S_ISREG(st.st_mode) && endsWith(name, ".rule"))
            loadRuleFile(full, out);
    }
    closedir(d);
}

static std::vector<uint8_t> readFile(const std::string &p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<uint8_t>(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>()
    );
}

static std::ptrdiff_t findOffset(
    const std::vector<uint8_t> &data,
    const std::vector<uint8_t> &pat)
{
    if (pat.empty() || pat.size() > data.size()) return -1;
    auto it = std::search(data.begin(), data.end(), pat.begin(), pat.end());
    return (it != data.end()) ? std::distance(data.begin(), it) : -1;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <target_file> <rules_dir>\n";
        std::cerr << "Exit codes: 0=clean  1=error  2=match(es) found\n";
        return 1;
    }

    std::string target   = argv[1];
    std::string rulesDir = argv[2];

    struct stat ts;
    if (stat(target.c_str(), &ts) != 0) {
        std::cerr << "[ERROR] Target not found: " << target << "\n";
        return 1;
    }
    struct stat rd;
    if (stat(rulesDir.c_str(), &rd) != 0 || !S_ISDIR(rd.st_mode)) {
        std::cerr << "[ERROR] Rules directory not found: " << rulesDir << "\n";
        return 1;
    }

    std::vector<Rule> rules;
    walkDir(rulesDir, rules);
    auto data = readFile(target);

    std::cout << "================================================\n";
    std::cout << "  malscan - Rule-Based File Scanner\n";
    std::cout << "================================================\n";
    std::cout << "[*] Target   : " << target << "\n";
    std::cout << "[*] File size: " << data.size() << " bytes\n";
    std::cout << "[*] Rules dir: " << rulesDir << "\n";
    std::cout << "[*] Rules    : " << rules.size() << " loaded\n";
    std::cout << "------------------------------------------------\n";

    int hits = 0;
    for (const auto &r : rules) {
        for (const auto &pat : r.patterns) {
            std::ptrdiff_t off = findOffset(data, pat);
            if (off >= 0) {
                std::cout << "[MATCH] " << r.name
                          << " | offset=0x" << std::hex << off << std::dec
                          << " | " << r.description << "\n";
                ++hits;
                break;
            }
        }
    }

    std::cout << "------------------------------------------------\n";
    if (!hits)
        std::cout << "[CLEAN] No matches found.\n";
    else
        std::cout << "[RESULT] " << hits << " rule(s) matched.\n";
    std::cout << "================================================\n";

    return hits ? 2 : 0;
}
