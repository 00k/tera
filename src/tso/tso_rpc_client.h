// Copyright (c) 2017, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TERA_TSO_TSO_RPC_CLIENT_H_
#define TERA_TSO_TSO_RPC_CLIENT_H_

#include <string>

#include <gflags/gflags.h>

#include "proto/rpc_client.h"
#include "proto/tso.pb.h"

DECLARE_int32(tera_rpc_timeout_period);

namespace tera {
namespace tso {

class TsoRpcClient : public RpcClient<TimestampOracleServer::Stub> {
public:
    TsoRpcClient(const std::string& server_addr = "",
                 int32_t rpc_timeout = FLAGS_tera_rpc_timeout_period);

    virtual ~TsoRpcClient();

    virtual bool GetTimestamp(const GetTimestampRequest* request,
                              GetTimestampResponse* response);

private:
    int32_t rpc_timeout_;
};

} // namespace tso
} // namespace tera

#endif // TERA_TSO_TSO_RPC_CLIENT_H_
