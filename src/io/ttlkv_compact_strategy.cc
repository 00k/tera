// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "io/ttlkv_compact_strategy.h"
#include "leveldb/slice.h"
#include "io/io_utils.h"

namespace tera {
namespace io {

using namespace leveldb;

KvCompactStrategy::KvCompactStrategy(const TableSchema& schema) :
        schema_(schema), raw_key_operator_(GetRawKeyOperatorFromSchema(schema_)) {
    VLOG(11) << "KvCompactStrategy construct";
}

KvCompactStrategy::~KvCompactStrategy() {
}

const char* KvCompactStrategy::Name() const {
    return "tera.TTLKvCompactStrategy";
}

bool KvCompactStrategy::Drop(const leveldb::Slice& tera_key, uint64_t n) {
    // If expire timestamp + schema's TTL <= time(NULL), Then Drop.
    // Desc: ��ǰTTL���������Ϊ�������û�ָ����key��03:10�ֹ��ڣ�ͬʱSchema��TTLΪ+300(�Ӻ�5����),
    // ��ô���key����03:15�ֹ���.
    //
    // ����������, ���ϣ��һ��key��ǰ����, ֻ��Ҫ�޸�schema��TTLΪ��ֵ������-300(��ǰ5����), ��ô���key��
    // ��03:05�ֹ���.
    //
    // ����, �����û�����������������ڵ�key, ������ô����schema��TTL����������κ����á�

    leveldb::Slice row_key;
    int64_t expire_timestamp;
    raw_key_operator_->ExtractTeraKey(tera_key, &row_key, NULL, NULL,
            &expire_timestamp, NULL);

    int64_t now = get_micros() / 1000000;

    int64_t final_expire_timestamp = expire_timestamp
            + schema_.column_families(0).time_to_live();
    if (final_expire_timestamp <= 0 /*����,��������*/
    || final_expire_timestamp > now) {
        VLOG(11) << "[KvCompactStrategy-Not-Drop] row_key:[" << row_key.ToString()
                           << "] expire_timestamp:[" << expire_timestamp
                           << "] now:[" << now << "] time_to_live=["
                           << schema_.column_families(0).time_to_live() << "]";
        return false;
    }
    VLOG(11) << "[KvCompactStrategy-Drop] row_key:[" << row_key.ToString()
                       << "] expire_timestamp:[" << expire_timestamp
                       << "] now:[" << now << "] time_to_live=["
                       << schema_.column_families(0).time_to_live() << "]";
    return true;
}

bool KvCompactStrategy::ScanDrop(const leveldb::Slice& tera_key, uint64_t n) {
    return Drop(tera_key, n); // used in scan.
}

bool KvCompactStrategy::ScanMergedValue(Iterator* it, std::string* merged_value) {
    return false;
}

bool KvCompactStrategy::MergeAtomicOPs(Iterator* it, std::string* merged_value,
                                std::string* merged_key) {
    return false;
}

KvCompactStrategyFactory::KvCompactStrategyFactory(const TableSchema& schema) :
        schema_(schema) {
}

KvCompactStrategy* KvCompactStrategyFactory::NewInstance() {
    return new KvCompactStrategy(schema_);
}

} // namespace io
} // namespace tera
