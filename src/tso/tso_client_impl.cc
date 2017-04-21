// Copyright (c) 2017, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tso/tso_client_impl.h"

#include <gflags/gflags.h>

#include "tso/tso_rpc_client.h"

DEFINE_string(tera_tso_host, "127,0,0,1", "host of timestamp oracle");
DEFINE_string(tera_tso_port, "50000", "port of timestamp oracle");

DECLARE_string(tera_tso_host);
DECLARE_string(tera_tso_port);

namespace tera {
namespace tso {

common::Counter TsoClientImpl::last_sequence_;

int64_t TsoClientImpl::GetTimestamp() {
    TsoRpcClient rpc_client(FLAGS_tera_tso_host + ":" + FLAGS_tera_tso_port);
    GetTimestampRequest request;
    request.set_sequence_id(last_sequence_.Inc());
    request.set_number(1);
    GetTimestampResponse response;
    rpc_client.GetTimestamp(&request, &response);

    if (response.status() != kTabletNodeOk) {
        return -1;
    }
    if (response.number() != 1) {
        return -2;
    }
    return response.start_timestamp();
}

} // namespace tso
} // namespace tera
