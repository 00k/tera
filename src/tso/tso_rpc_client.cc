// Copyright (c) 2017, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tso/tso_rpc_client.h"

#include "common/base/closure.h"

namespace tera {
namespace tso {

TsoRpcClient::TsoRpcClient(const std::string& server_addr,
                           int32_t rpc_timeout)
    : RpcClient<TimestampOracleServer::Stub>(server_addr),
      rpc_timeout_(rpc_timeout) {}

TsoRpcClient::~TsoRpcClient() {}

bool TsoRpcClient::GetTimestamp(const GetTimestampRequest* request,
                                GetTimestampResponse* response) {
    return SendMessageWithRetry(&TimestampOracleServer::Stub::GetTimestamp, request, response,
                                (Closure<void, GetTimestampRequest*, GetTimestampResponse*, bool, int>*)NULL,
                                "GetTimestamp", rpc_timeout_);
}

} // namespace tso
} // namespace tera
