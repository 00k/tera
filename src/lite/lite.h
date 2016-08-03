// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef  TERA_LITE_H_
#define  TERA_LITE_H_

#include <string>

namespace tera {

class DB {
public:
    static void Init(const std::string& confpath);

    static bool NewDB(const TableDescriptor& desc, uint64_t id, DB** db);
    static bool NewDB(const TableDescriptor& desc, DB** db);

    DB() {}
    virtual ~DB() {}

    /// 返回一个新的RowMutation
    virtual RowMutation* NewRowMutation(const std::string& row_key) = 0;

    /// 返回一个新的RowReader
    virtual RowReader* NewRowReader(const std::string& row_key) = 0;

    /// 修改
    virtual void Put(RowMutation* row_mu) = 0;

    /// 读取
    virtual void Get(RowReader* row_reader) = 0;

    /// Scan, 流式读取, 返回一个数据流, 失败返回NULL
    virtual ResultStream* Scan(ErrorCode* err) = 0;

    virtual std::string GetName() = 0;

    /// 获取表格的最小最大key
    virtual bool GetStartEndKeys(std::string* start_key, std::string* end_key,
                                 ErrorCode* err) = 0;

    /// 获取表格描述符
    virtual bool GetDescriptor(TableDescriptor* desc, ErrorCode* err) = 0;

private:
    DB(const DB&);
    void operator=(const DB&);
};

}  // namespace tera

#endif  // TERA_LITE_H_
