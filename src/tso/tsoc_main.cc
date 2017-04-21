// Copyright (c) 2017, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <map>
#include <string>

#include <tso/tso.h>

namespace tera {
namespace tso {

static TimestampOracle* tso = NULL;
TimestampOracle* GetTsoInstance() {
    if (tso == NULL) {
        tso = new TimestampOracle;
    }
    return tso;
}

int Get(int argc, char** argv) {
    if (argc > 2) {
        std::cerr << "too many arguments for GET cmd" << std::endl;
        return -1;
    }
    int64_t ts = GetTsoInstance()->GetTimestamp();
    std::cout << ts << std::endl;
    return 0;
}

void PrintHelp(char* program) {
    std::cout << program << " "
              << "help\n"
              << "version\n"
              << "get\n"
              << std::endl;
}

void PrintVersion() {

}

typedef std::map<std::string, int(*)(int, char** argv)> CommandTable;
static CommandTable command_table;

static void InitializeCommandTable(){
    command_table["get"] = Get;
}

int main(int argc, char** argv) {
    if (argc == 1) {
        PrintHelp(argv[0]);
        return 0;
    }
    std::string cmd = argv[1];
    if (cmd == "help") {
        PrintHelp(argv[0]);
        return 0;
    }
    if (cmd == "version") {
        PrintVersion();
        return 0;
    }
    InitializeCommandTable();
    if (command_table.find(cmd) == command_table.end()) {
        PrintHelp(argv[0]);
        return -1;
    }
    return command_table[cmd](argc, argv);
}

} // namespace tso
} // namespace tera
