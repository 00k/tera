// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sdk/mutate_impl.h"
#include "sdk/tera.h"

namespace tera {

RowMutation::RowMutation() {}

RowMutation::~RowMutation() {}

#if 0
enum MutationType {
    kNone,
    kPut,
    kDeleteColumn,
    kDeleteColumns,
    kDeleteFamily,
    kDeleteRow
};


class Mutation {
public:
    virtual void Serialize(tera::Mutation* mutation) = 0;

protected:
    Mutation() {}
    ~Mutation() {}
};

class PutMutaion : public Mutation {
public:
    void Put(const std::string& family, const std::string& qualifier,
             int64_t timestamp, const std::string& value) {
        type = kPut;
        m_family = family;
        m_qualifier = qualifier;
        m_timestamp = timestamp;
        m_value = value;
    }

private:

};

class DeleteColumnMutaion : public Mutation {
public:
    DeleteColumnMutaion(const std::string& family, const std::string& qualifier,
                        int64_t timestamp) {
        type = kDeleteColumns;
        m_family = family;
        m_qualifier = qualifier;
        m_timestamp = timestamp;
    }

private:
    std::string m_family;
    std::string m_qualifier;
    int64 m_timestamp;
};

class DeleteColumnsMutaion : public Mutation {
public:
    DeleteColumnsMutaion(const std::string& family, const std::string& qualifier,
                        int64_t ts_start, int64_t ts_end) {
        type = kDeleteColumns;
        m_family = family;
        m_ts_start = ts_start;
        m_ts_end = ts_end;
    }

private:
    std::string m_family;
    std::string m_qualifier;
    int64 m_ts_start;
    int64 m_ts_end;
};

class DeleteFamilyMutaion : public Mutation {
public:
    DeleteFamilyMutaion(const std::string& family, int64_t ts_start,
                        int64_t ts_end) {
        type = kDeleteFamily;
        m_family = family;
        m_ts_start = ts_start;
        m_ts_end = ts_end;
    }

private:
    std::string m_family;
    int64 m_ts_start;
    int64 m_ts_end;
};

class DeleteRowMutaion : public Mutation {
public:
    void DeleteRow(int64_t ts_start, int64_t ts_end) {
        type = kDeleteRow;
        m_ts_start = ts_start;
        m_ts_end = ts_end;
    }

private:
    int64 m_ts_start;
    int64 m_ts_end;
};


/// �޸Ĳ���
RowMutation::RowMutation(Table* table, const std::string& row_key) {
    _impl = new RowMutationImpl(this, table, row_key);
}

RowMutation::~RowMutation() {
    delete _impl;
}

/// ���ã�����ǰ�������
void RowMutation::Reset(Table* table, const std::string& row_key) {
    _impl->Reset(table, row_key);
}

/// �޸�һ����
void RowMutation::Put(const std::string& family, const std::string& qualifier,
                      const std::string& value) {
    _impl->Put(family, qualifier, value);
}

/// �޸�һ���е��ض��汾
void RowMutation::Put(const std::string& family, const std::string& qualifier,
         int64_t timestamp, const std::string& value) {
    _impl->Put(family, qualifier, timestamp, value);
}

/// �޸�Ĭ����
void RowMutation::Put(const std::string& value) {
    _impl->Put(value);
}

/// �޸�Ĭ���е��ض��汾
void RowMutation::Put(int64_t timestamp, const std::string& value) {
    _impl->Put(timestamp, value);
}

/// ɾ��һ���е����°汾
void RowMutation::DeleteColumn(const std::string& family,
                               const std::string& qualifier) {
    _impl->DeleteColumn(family, qualifier);
}

/// ɾ��һ���е�ָ���汾
void RowMutation::DeleteColumn(const std::string& family,
                               const std::string& qualifier,
                               int64_t timestamp) {
    _impl->DeleteColumn(family, qualifier, timestamp);
}

/// ɾ��һ���е�ȫ���汾
void RowMutation::DeleteColumns(const std::string& family,
                                const std::string& qualifier) {
    _impl->DeleteColumns(family, qualifier);
}

/// ɾ��һ���е�ָ����Χ�汾
void RowMutation::DeleteColumns(const std::string& family,
                                const std::string& qualifier,
                                int64_t ts_end, int64_t ts_start) {
    _impl->DeleteColumns(family, qualifier, ts_end, ts_start);
}

/// ɾ��һ������������е�ȫ���汾
void RowMutation::DeleteFamily(const std::string& family) {
    _impl->DeleteFamily(family);
}

/// ɾ��һ������������е�ָ����Χ�汾
void RowMutation::DeleteFamily(const std::string& family, int64_t ts_end,
                  int64_t ts_start) {
    _impl->DeleteFamily(family, ts_end, ts_start);
}

/// ɾ�����е�ȫ������
void RowMutation::DeleteRow() {
    _impl->DeleteRow();
}

/// ɾ�����е�ָ����Χ�汾
void RowMutation::DeleteRow(int64_t ts_end, int64_t ts_start) {
    _impl->DeleteRow(ts_end, ts_start);
}

/// �޸���ס����, �����ṩ����
void RowMutation::SetLock(RowLock* rowlock) {
    _impl->SetLock(rowlock);
}

/// ���ó�ʱʱ��(ֻӰ�쵱ǰ����,��Ӱ��Table::SetWriteTimeout���õ�Ĭ��д��ʱ)
void RowMutation::SetTimeOut(int64_t timeout_ms) {
    _impl->SetTimeOut(timeout_ms);
}

int64_t RowMutation::TimeOut() {
    return _impl->TimeOut();
}

/// �����첽�ص�, �������첽����
void RowMutation::SetCallBack(Callback callback) {
    _impl->SetCallBack(callback);
}

RowMutation::Callback RowMutation::GetCallBack() {
    return _impl->GetCallBack();
}

/// ��ý��������
ErrorCode RowMutation::GetError() {
    return _impl->GetError();
}

/// �Ƿ��첽����
bool RowMutation::IsAsync() {
    return _impl->IsAsync();
}

/// �첽�����Ƿ����
bool RowMutation::IsFinished() const {
    return _impl->IsFinished();
}

/// ����row_key
const std::string& RowMutation::RowKey() {
    return _impl->RowKey();
}

/// mutation����
uint32_t RowMutation::MutationNum() {
    return _impl->MutationNum();
}

/// ����mutation
//const Mutation& Mutation(uint32_t index);
/// ���Դ���
uint32_t RowMutation::RetryTimes() {
    return _impl->RetryTimes();
}

RowMutationImpl* RowMutation::GetImpl() {
    return _impl;
}

#endif

} // namespace tera

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
