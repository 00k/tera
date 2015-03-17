// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef  TERA_READ_IMPL_H_
#define  TERA_READ_IMPL_H_

#include <string>
#include <vector>

#include "common/mutex.h"

#include "proto/tabletnode_rpc.pb.h"
#include "sdk/sdk_task.h"
#include "sdk/tera.h"
#include "types.h"
#include "utils/timer.h"



namespace tera {

class TableImpl;

class RowReaderImpl : public RowReader, public SdkTask {
public:
    RowReaderImpl(Table* table, const std::string& row_key);
    ~RowReaderImpl();
    /// ���ö�ȡ�ض��汾
    void SetTimestamp(int64_t ts);
    /// ���ض�ȡʱ���
    int64_t GetTimestamp();

    void SetSnapshot(uint64_t snapshot_id) { _snapshot_id = snapshot_id; }

    uint64_t GetSnapshot() { return _snapshot_id; }

    /// ���ö�ȡCF
    void AddColumnFamily(const std::string& cf_name);
    /// ���ö�ȡColumn(CF:Qualifier)
    void AddColumn(const std::string& cf_name, const std::string& qualifier);
    /// ���ö�ȡtime_range
    void SetTimeRange(int64_t ts_start, int64_t ts_end);
    /// ����time_range
    void GetTimeRange(int64_t* ts_start, int64_t* ts_end = NULL);
    /// ���ö�ȡmax_version
    void SetMaxVersions(uint32_t max_version);
    /// ����max_version
    uint32_t GetMaxVersions();
    /// ���ó�ʱʱ��(ֻӰ�쵱ǰ����,��Ӱ��Table::SetReadTimeout���õ�Ĭ�϶���ʱ)
    void SetTimeOut(int64_t timeout_ms);
    /// �����첽�ص�, �������첽����
    void SetCallBack(RowReader::Callback callback);
    /// �����û������ģ����ڻص������л�ȡ
    void SetContext(void* context);
    void* GetContext();
    /// �����첽����
    void SetAsync();
    /// �첽�����Ƿ����
    bool IsFinished() const;
    /// ��ȡ����ʱʱ��
    int64_t TimeOut();
    /// ���ô�����
    void SetError(ErrorCode::ErrorCodeType err , const std::string& reason = "");
    /// ��ý��������
    ErrorCode GetError();
    /// �Ƿ񵽴�������
    bool Done();
    /// ������һ��cell
    void Next();
    /// Row
    const std::string& RowName();
    /// ��ȡ�Ľ��
    std::string Value();
    /// Timestamp
    int64_t Timestamp();
    /// Column cf:qualifier
    std::string ColumnName();
    /// Column family
    std::string Family();
    /// Qualifier
    std::string Qualifier();
    /// �����ת�浽һ��std::map��, ��ʽΪ: map<column, map<timestamp, value>>
    typedef std::map< std::string, std::map<int64_t, std::string> > Map;
    void ToMap(Map* rowmap);

    void SetResult(const RowResult& result);

    void IncRetryTimes();

    uint32_t RetryTimes();

    bool IsAsync();

    bool Wait(int64_t abs_time_ms = 0);
    /// ִ���첽�ص�
    void RunCallback();
    /// Get����
    uint32_t GetReadColumnNum();
    /// ����Get����
    const ReadColumnList& GetReadColumnList();
    /// ���л�
    void ToProtoBuf(RowReaderInfo* info);

private:
    std::string _row_key;
    RowReader::Callback _callback;
    void* _user_context;

    bool _finish;
    ErrorCode _error_code;
    mutable Mutex _finish_mutex;
    common::CondVar _finish_cond;

    typedef std::set<std::string> QualifierSet;
    typedef std::map<std::string, QualifierSet> FamilyMap;
    FamilyMap _family_map;
    int64_t _ts_start;
    int64_t _ts_end;
    uint32_t _max_version;
    uint64_t _snapshot_id;

    int64_t _timeout_ms;
    uint32_t _retry_times;
    int32_t _result_pos;
    RowResult _result;
};

} // namespace tera

#endif  //TERA_READ_IMPL_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
