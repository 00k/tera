// Copyright (c) 2017, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TERA_TSO_TSO_CLIENT_IMPL_H_
#define TERA_TSO_TSO_CLIENT_IMPL_H_

#include "common/counter.h"

#include "tso/tso.h"

namespace tera {
namespace tso {

class TsoClientImpl : public TimestampOracle {
public:
    virtual int64_t GetTimestamp();
    TsoClientImpl() {}
    ~TsoClientImpl() {}

private:
    static common::Counter last_sequence_;
};

} // namespace tso
} // namespace tera

#endif // TERA_TSO_TSO_CLIENT_IMPL_H_
