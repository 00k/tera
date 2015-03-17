// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "io/coding.h"
#include "sdk/mutate_impl.h"
#include "utils/timer.h"

namespace tera {

RowMutationImpl::RowMutationImpl(TableImpl* table, const std::string& row_key)
    : SdkTask(SdkTask::MUTATION),
      _table(table),
      _row_key(row_key),
      _refs(0),
      _callback(NULL),
      _timeout_ms(0),
      _retry_times(0),
      _finish(false),
      _finish_cond(&_mutex) {
}

RowMutationImpl::~RowMutationImpl() {
}

/// ���ã�����ǰ�������
void RowMutationImpl::Reset(const std::string& row_key) {
    _row_key = row_key;
    _mu_seq.clear();
    _callback = NULL;
    _timeout_ms = 0;
    _retry_times = 0;
    _finish = false;
    _error_code.SetFailed(ErrorCode::kOK);
}

// ԭ�Ӽ�һ��Cell
void RowMutationImpl::Add(const std::string& family, const std::string& qualifier,
                          const int64_t delta) {
    char delta_buf[sizeof(int64_t)];
    RowMutation::Mutation& mutation = AddMutation();
    mutation.type = RowMutation::kAdd;
    mutation.family = family;
    mutation.qualifier = qualifier;
    mutation.timestamp = get_micros();//Ϊ�˱���retry������ظ��ӣ������Դ�ʱ���
    io::EncodeBigEndian(delta_buf, delta);
    mutation.value.assign(delta_buf, sizeof(delta_buf));
}

// ԭ�Ӳ�������������ڲ���Put�ɹ�
void RowMutationImpl::PutIfAbsent(const std::string& family, const std::string& qualifier,
                                  const std::string& value) {
    RowMutation::Mutation& mutation = AddMutation();
    mutation.type = RowMutation::kPutIfAbsent;
    mutation.family = family;
    mutation.qualifier = qualifier;
    mutation.timestamp = kLatestTimestamp;
    mutation.value = value;
}

void RowMutationImpl::Append(const std::string& family, const std::string& qualifier,
                             const std::string& value) {
    RowMutation::Mutation& mutation = AddMutation();
    mutation.type = RowMutation::kAppend;
    mutation.family = family;
    mutation.qualifier = qualifier;
    mutation.timestamp = get_micros();
    mutation.value = value;
}

/// �޸�һ����
void RowMutationImpl::Put(const std::string& family, const std::string& qualifier,
                          const std::string& value) {
    Put(family, qualifier, kLatestTimestamp, value);
}

/// ��TTL�޸�һ����
void RowMutationImpl::Put(const std::string& family, const std::string& qualifier,
                          const std::string& value, int32_t ttl) {
    RowMutation::Mutation& mutation = AddMutation();
    mutation.type = RowMutation::kPut;
    mutation.family = family;
    mutation.qualifier = qualifier;
    mutation.timestamp = kLatestTimestamp;
    mutation.value = value;
    mutation.ttl = ttl;
}

/// �޸�һ���е��ض��汾
void RowMutationImpl::Put(const std::string& family, const std::string& qualifier,
                          int64_t timestamp, const std::string& value) {
    RowMutation::Mutation& mutation = AddMutation();
    mutation.type = RowMutation::kPut;
    mutation.family = family;
    mutation.qualifier = qualifier;
    mutation.timestamp = timestamp;
    mutation.value = value;
    mutation.ttl = -1;
    /*
    mutation.set_type(kPut);
    mutation.set_family(family);
    mutation.set_qualifier(qualifier);
    mutation.set_timestamp(timestamp);
    mutation.set_value(value);
    */
}

/// ��TTL���޸�һ���е��ض��汾
void RowMutationImpl::Put(const std::string& family, const std::string& qualifier,
                 int64_t timestamp, const std::string& value, int32_t ttl) {
    RowMutation::Mutation& mutation = AddMutation();
    mutation.type = RowMutation::kPut;
    mutation.family = family;
    mutation.qualifier = qualifier;
    mutation.timestamp = timestamp;
    mutation.value = value;
    mutation.ttl = ttl;
}

/// �޸�Ĭ����
void RowMutationImpl::Put(const std::string& value) {
    Put("", "", kLatestTimestamp, value);
}

/// ��TTL���޸�Ĭ����
void RowMutationImpl::Put(const std::string& value, int32_t ttl) {
    Put("", "", kLatestTimestamp, value, ttl);
}

/// �޸�Ĭ���е��ض��汾
void RowMutationImpl::Put(int64_t timestamp, const std::string& value) {
    Put("", "", timestamp, value);
}

/// ɾ��һ���е����°汾
void RowMutationImpl::DeleteColumn(const std::string& family,
                                   const std::string& qualifier) {
    DeleteColumn(family, qualifier, kLatestTimestamp);
}

/// ɾ��һ���е�ָ���汾
void RowMutationImpl::DeleteColumn(const std::string& family,
                                   const std::string& qualifier,
                                   int64_t timestamp) {
    RowMutation::Mutation& mutation = AddMutation();
    mutation.type = RowMutation::kDeleteColumn;
    mutation.family = family;
    mutation.qualifier = qualifier;
    mutation.timestamp = timestamp;

    /*
    mutation.set_type(kDeleteColumn);
    mutation.set_family(family);
    mutation.set_qualifier(qualifier);
    mutation.set_timestamp(timestamp);
    */
}

/// ɾ��һ���е�ȫ���汾
void RowMutationImpl::DeleteColumns(const std::string& family,
                                    const std::string& qualifier) {
    DeleteColumns(family, qualifier, kLatestTimestamp);
}

/// ɾ��һ���е�ָ����Χ�汾
void RowMutationImpl::DeleteColumns(const std::string& family,
                                    const std::string& qualifier,
                                    int64_t timestamp) {
    RowMutation::Mutation& mutation = AddMutation();
    mutation.type = RowMutation::kDeleteColumns;
    mutation.family = family;
    mutation.qualifier = qualifier;
    mutation.timestamp = timestamp;

    /*
    mutation.set_type(kDeleteColumns);
    mutation.set_family(family);
    mutation.set_qualifier(qualifier);
    mutation.set_ts_end(ts_end);
    mutation.set_ts_start(ts_start);
    */
}

/// ɾ��һ������������е�ȫ���汾
void RowMutationImpl::DeleteFamily(const std::string& family) {
    DeleteFamily(family, kLatestTimestamp);
}

/// ɾ��һ������������е�ָ����Χ�汾
void RowMutationImpl::DeleteFamily(const std::string& family,
                                   int64_t timestamp) {
    RowMutation::Mutation& mutation = AddMutation();
    mutation.type = RowMutation::kDeleteFamily;
    mutation.family = family;
    mutation.timestamp = timestamp;
    /*
    mutation.set_type(kDeleteFamily);
    mutation.set_family(family);
    mutation.set_ts_end(ts_end);
    mutation.set_ts_start(ts_start);
    */
}

/// ɾ�����е�ȫ������
void RowMutationImpl::DeleteRow() {
    DeleteRow(kLatestTimestamp);
}

/// ɾ�����е�ָ����Χ�汾
void RowMutationImpl::DeleteRow(int64_t timestamp) {
    RowMutation::Mutation& mutation = AddMutation();
    mutation.type = RowMutation::kDeleteRow;
    mutation.timestamp = timestamp;
    /*
    mutation.set_type(kDeleteRow);
    mutation.set_ts_end(ts_end);
    mutation.set_ts_start(ts_start);
    */
}

/// �޸���ס����, �����ṩ����
void RowMutationImpl::SetLock(RowLock* rowlock) {
}

/// ���ó�ʱʱ��(ֻӰ�쵱ǰ����,��Ӱ��Table::SetWriteTimeout���õ�Ĭ��д��ʱ)
void RowMutationImpl::SetTimeOut(int64_t timeout_ms) {
    _timeout_ms = timeout_ms;
}

int64_t RowMutationImpl::TimeOut() {
    return _timeout_ms;
}

/// �����첽�ص�, �������첽����
void RowMutationImpl::SetCallBack(RowMutation::Callback callback) {
    _callback = callback;
}

/// ��ûص�����
RowMutation::Callback RowMutationImpl::GetCallBack() {
    return _callback;
}

/// �����û������ģ����ڻص������л�ȡ
void RowMutationImpl::SetContext(void* context) {
    _user_context = context;
}

/// ����û�������
void* RowMutationImpl::GetContext() {
    return _user_context;
}

/// ��ý��������
const ErrorCode& RowMutationImpl::GetError() {
    return _error_code;
}

/// �Ƿ��첽����
bool RowMutationImpl::IsAsync() {
    return (_callback != NULL);
}

/// �첽�����Ƿ����
bool RowMutationImpl::IsFinished() const {
    MutexLock lock(&_finish_mutex);
    return _finish;
}

/// ����row_key
const std::string& RowMutationImpl::RowKey() {
    return _row_key;
}

/// mutation����
uint32_t RowMutationImpl::MutationNum() {
    return _mu_seq.size();
}

/// mutation�ܴ�С
uint32_t RowMutationImpl::Size() {
    uint32_t total_size = 0;
    for (int i = 0; i < _mu_seq.size(); ++i) {
        total_size +=
            + _row_key.size()
            + _mu_seq[i].family.size()
            + _mu_seq[i].qualifier.size()
            + _mu_seq[i].value.size()
            + sizeof(_mu_seq[i].timestamp);
    }
    return total_size;
}

/// ����mutation
const RowMutation::Mutation& RowMutationImpl::GetMutation(uint32_t index) {
    return _mu_seq[index];
}

/// ���Դ���
uint32_t RowMutationImpl::RetryTimes() {
    return _retry_times;
}

/// ���Լ�����һ
void RowMutationImpl::IncRetryTimes() {
    _retry_times++;
}

/// ���ô�����
void RowMutationImpl::SetError(ErrorCode::ErrorCodeType err,
                               const std::string& reason) {
    _error_code.SetFailed(err, reason);
}

/// �ȴ�����
bool RowMutationImpl::Wait(int64_t abs_time_ms) {
    int64_t timeout = -1;
    if (abs_time_ms > 0) {
        timeout = abs_time_ms - GetTimeStampInMs();
        if (timeout < 0) {
            timeout = 0;
        }
    }

    MutexLock lock(&_finish_mutex);
    while (!_finish && timeout != 0) {
        _finish_cond.TimeWait(timeout);
        if (abs_time_ms > 0) {
            timeout = abs_time_ms - GetTimeStampInMs();
            if (timeout < 0) {
                timeout = 0;
            }
        }
    }

    return _finish;
}

void RowMutationImpl::RunCallback() {
    if (_callback) {
        _callback(this);
    } else {
        MutexLock lock(&_finish_mutex);
        _finish = true;
        _finish_cond.Signal();
    }
}

void RowMutationImpl::Ref() {
    MutexLock lock(&_mutex);
    ++_refs;
}

void RowMutationImpl::Unref() {
    MutexLock lock(&_mutex);
    --_refs;
    CHECK(_refs >= 0);
    if (_refs == 0) {
        delete this;
    }
}

RowMutation::Mutation& RowMutationImpl::AddMutation() {
    _mu_seq.resize(_mu_seq.size() + 1);
    return _mu_seq.back();
}

void SerializeMutation(const RowMutation::Mutation& src, tera::Mutation* dst) {
    switch (src.type) {
        case RowMutation::kPut:
            dst->set_type(tera::kPut);
            dst->set_family(src.family);
            dst->set_qualifier(src.qualifier);
            dst->set_timestamp(src.timestamp);
            dst->set_value(src.value);
            dst->set_ttl(src.ttl);
            break;
        case RowMutation::kAdd:
            dst->set_type(tera::kAdd);
            dst->set_family(src.family);
            dst->set_qualifier(src.qualifier);
            dst->set_timestamp(src.timestamp);
            dst->set_value(src.value);
            break;
        case RowMutation::kPutIfAbsent:
            dst->set_type(tera::kPutIfAbsent);
            dst->set_family(src.family);
            dst->set_qualifier(src.qualifier);
            dst->set_timestamp(src.timestamp);
            dst->set_value(src.value);
            break;
        case RowMutation::kAppend:
            dst->set_type(tera::kAppend);
            dst->set_family(src.family);
            dst->set_qualifier(src.qualifier);
            dst->set_timestamp(src.timestamp);
            dst->set_value(src.value);
            break;
        case RowMutation::kDeleteColumn:
            dst->set_type(tera::kDeleteColumn);
            dst->set_family(src.family);
            dst->set_qualifier(src.qualifier);
            dst->set_timestamp(src.timestamp);
            break;
        case RowMutation::kDeleteColumns:
            dst->set_type(tera::kDeleteColumns);
            dst->set_family(src.family);
            dst->set_qualifier(src.qualifier);
            dst->set_timestamp(src.timestamp);
            break;
        case RowMutation::kDeleteFamily:
            dst->set_type(tera::kDeleteFamily);
            dst->set_family(src.family);
            dst->set_timestamp(src.timestamp);
            break;
        case RowMutation::kDeleteRow:
            dst->set_type(tera::kDeleteRow);
            dst->set_timestamp(src.timestamp);
            break;
        default:
            assert(false);
            break;
    }
}

} // namespace tera

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
