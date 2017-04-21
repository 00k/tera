// Copyright (c) 2017, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TERA_TSO_TSO_H_
#define TERA_TSO_TSO_H_

#include <stdint.h>

namespace tera {
namespace tso {

class TsoClientImpl;

class TimestampOracle {
public:
    TimestampOracle();
    virtual ~TimestampOracle();
    virtual int64_t GetTimestamp();

private:
    TsoClientImpl* impl_;
};

} // namespace tso
} // namespace tera

#endif // TERA_TSO_TSO_H_
