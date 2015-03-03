// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef  TERA_MUTATE_IMPL_H_
#define  TERA_MUTATE_IMPL_H_

#include <string>
#include <vector>

#include "common/mutex.h"
#include "tera/utils/timer.h"

#include "tera/proto/tabletnode_rpc.pb.h"
#include "tera/sdk/sdk_task.h"
#include "tera/sdk/tera.h"
#include "tera/types.h"

namespace tera {

class TableImpl;

class RowMutationImpl : public RowMutation, public SdkTask {
public:
    RowMutationImpl(TableImpl* table, const std::string& row_key);
    ~RowMutationImpl();

    /// ����
    void Reset(const std::string& row_key);

    /// �޸�һ����
    void Put(const std::string& family, const std::string& qualifier,
             const std::string& value);
    /// ��TTL���޸�һ����
    void Put(const std::string& family, const std::string& qualifier,
             const std::string& value, int32_t ttl);

    /// �޸�һ���е��ض��汾
    void Put(const std::string& family, const std::string& qualifier,
             int64_t timestamp, const std::string& value);

    /// ��TTL���޸�һ���е��ض��汾
    virtual void Put(const std::string& family, const std::string& qualifier,
                     int64_t timestamp, const std::string& value, int32_t ttl);

    /// �޸�Ĭ����
    void Put(const std::string& value);

    /// ��TTL���޸�Ĭ����
    virtual void Put(const std::string& value, int32_t ttl);

    /// �޸�Ĭ���е��ض��汾
    void Put(int64_t timestamp, const std::string& value);

    /// ԭ�Ӽ�һ��Cell
    void Add(const std::string& family, const std::string& qualifier, const int64_t delta);

    //  ԭ�Ӳ�������������ڲ���Put�ɹ�
    void PutIfAbsent(const std::string& family, const std::string& qualifier,
                     const std::string& value);

    /// ԭ�Ӳ�����׷�����ݵ�һ��Cell
    void Append(const std::string& family, const std::string& qualifier,
                const std::string& value);

    /// ɾ��һ���е����°汾
    void DeleteColumn(const std::string& family, const std::string& qualifier);

    /// ɾ��һ���е�ָ���汾
    void DeleteColumn(const std::string& family, const std::string& qualifier,
                      int64_t timestamp);

    /// ɾ��һ���е�ȫ���汾
    void DeleteColumns(const std::string& family, const std::string& qualifier);
    /// ɾ��һ���е�ָ����Χ�汾
    void DeleteColumns(const std::string& family, const std::string& qualifier,
                       int64_t timestamp);

    /// ɾ��һ������������е�ȫ���汾
    void DeleteFamily(const std::string& family);

    /// ɾ��һ������������е�ָ����Χ�汾
    void DeleteFamily(const std::string& family, int64_t timestamp);

    /// ɾ�����е�ȫ������
    void DeleteRow();

    /// ɾ�����е�ָ����Χ�汾
    void DeleteRow(int64_t timestamp);

    /// �޸���ס����, �����ṩ����
    void SetLock(RowLock* rowlock);

    /// ���ó�ʱʱ��(ֻӰ�쵱ǰ����,��Ӱ��Table::SetWriteTimeout���õ�Ĭ��д��ʱ)
    void SetTimeOut(int64_t timeout_ms);

    int64_t TimeOut();

    /// �����첽�ص�, �������첽����
    void SetCallBack(RowMutation::Callback callback);

    RowMutation::Callback GetCallBack();

    /// �����û������ģ����ڻص������л�ȡ
    void SetContext(void* context);

    void* GetContext();

    /// ��ý��������
    const ErrorCode& GetError();

    /// �����첽����
    bool IsAsync();

    /// �첽�����Ƿ����
    bool IsFinished() const;

    /// ����row_key
    const std::string& RowKey();

    /// mutation����
    uint32_t MutationNum();

    /// mutation�ܴ�С
    uint32_t Size();

    /// ����mutation
    const RowMutation::Mutation& GetMutation(uint32_t index);

    /// ���Դ���
    uint32_t RetryTimes();

public:
    /// ���½ӿڽ��ڲ�ʹ�ã������Ÿ��û�

    /// ���Լ�����һ
    void IncRetryTimes();

    /// ���ô�����
    void SetError(ErrorCode::ErrorCodeType err , const std::string& reason = "");

    /// �ȴ�������ʱ��abs_time_ms�Ǿ���ʱ��
    bool Wait(int64_t abs_time_ms = 0);

    /// ִ���첽�ص�
    void RunCallback();

    /// ��������
    void Ref();

    /// �ͷ�����
    void Unref();

protected:
    /// ����һ������
    RowMutation::Mutation& AddMutation();

private:
    TableImpl* _table;
    std::string _row_key;
    std::vector<RowMutation::Mutation> _mu_seq;

    mutable Mutex _mutex;
    int32_t _refs;

    RowMutation::Callback _callback;
    void* _user_context;
    int64_t _timeout_ms;
    uint32_t _retry_times;

    bool _finish;
    ErrorCode _error_code;
    mutable Mutex _finish_mutex;
    common::CondVar _finish_cond;
};

void SerializeMutation(const RowMutation::Mutation& src, tera::Mutation* dst);

} // namespace tera

#endif  //TERA_MUTATE_IMPL_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
