// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <signal.h>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "version.h"
#include "common/base/scoped_ptr.h"

#include "master/master_entry.h"
#include "tabletnode/tabletnode_entry.h"
#include "tera_entry.h"
#include "utils/utils_cmd.h"

DECLARE_string(tera_role);

bool g_quit = false;

static void SignalIntHandler(int sig) {
    LOG(INFO) << "receive interrupt signal from user, will stop";
    g_quit = true;
}

tera::TeraEntry* SwitchTeraEntry() {
    const std::string& server_name = FLAGS_tera_role;

    if (server_name == "master") {
        return new tera::master::MasterEntry();
    } else if (server_name == "tabletnode") {
        return new tera::tabletnode::TabletNodeEntry();
    }
    LOG(ERROR) << "FLAGS_tera_role should be one of ("
        << "master | tabletnode"
        << "), not : " << FLAGS_tera_role;
    return NULL;
}

int main(int argc, char** argv) {
    ::gflags::ParseCommandLineFlags(&argc, &argv, true);
    ::google::InitGoogleLogging(argv[0]);
    tera::utils::SetupLog(FLAGS_tera_role);

    if (argc > 1) {
        std::string ext_cmd = argv[1];
        if (ext_cmd == "version") {
            PrintSystemVersion();
            return 0;
        }
    }

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);

    scoped_ptr<tera::TeraEntry> entry(SwitchTeraEntry());
    if (entry.get() == NULL) {
        return -1;
    }

    if (!entry->Start()) {
        return -1;
    }

    while (!g_quit) {
        if (!entry->Run()) {
            LOG(ERROR) << "Server run error ,and then exit now ";
            break;
        }
        // jvm����ע���, ʱ��׼����������
        signal(SIGINT, SignalIntHandler);
        signal(SIGTERM, SignalIntHandler);
        // signal(SIGSEGV, SIG_DFL); // ���������Ļ�SIG_DFL, jvm��core, ��֪��Ϊɶ...
    }

    if (!entry->Shutdown()) {
        return -1;
    }

    return 0;
}
