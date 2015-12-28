// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef  TERA_SDK_LOCK_IMPL_H_
#define  TERA_SDK_LOCK_IMPL_H_

#include <string>

#include "common/mutex.h"
#include "sdk/sdk_task.h"
#include "sdk/tera.h"

namespace tera {

class TableImpl;

class RowLockImpl : public RowLock, public SdkTask {
public:
    RowLockImpl(TableImpl* table, const std::string& row_key, int64_t lock_id);
    virtual ~RowLockImpl();

    /// 返回row_key
    virtual const std::string& RowKey() = 0;

    /// 设置异步回调, 操作会异步返回
    typedef void (*Callback)(RowLock* row_lock) = 0;
    virtual void SetCallBack(Callback callback) = 0;
    /// 获得回调函数
    virtual Callback GetCallBack() = 0;

    /// 设置用户上下文，可在回调函数中获取
    virtual void SetContext(void* context) = 0;
    /// 获得用户上下文
    virtual void* GetContext() = 0;

    /// 设置超时时间
    virtual void SetTimeOut(int64_t timeout_ms) = 0;
    /// 获得超时时间
    virtual int64_t TimeOut() = 0;

    /// 获得错误码
    virtual const ErrorCode& GetError() = 0;

    /// 等待结束
    virtual void Wait() = 0;

public:
    enum LockState {
        kUnlocked = 1,
        kSharedLocked = 2,
        kExclusiveLocked = 3
    };
    enum LockAction {
        kTrySharedLock = 1,
        kTryExclusiveLock = 2,
        kTryUnlock = 3
    };

    /// 获得ID
    int64_t LockId();

    /// 设置状态
    void SetLockState(LockState lock_state);
    /// 获得状态
    LockState LockState();

    /// 重试计数加一
    void IncRetryTimes();
    /// 获得重试次数
    uint32_t RetryTimes();

    /// 设置结果错误码
    void SetError(ErrorCode::ErrorCodeType err, const std::string& reason);

    /// 是否异步操作
    bool IsAsync();

    /// 异步操作是否完成
    bool IsFinished();

    /// 调用用户回调函数
    void RunCallback();

private:
    TableImpl* _table;
    std::string _row_key;
    int64_t _lock_id;
    LockState _lock_state;

    RowMutation::Callback _callback;
    void* _user_context;
    int64_t _timeout_ms;
    uint32_t _retry_times;

    // protected by mutex
    mutable Mutex _mutex;
    bool _finish;
    ErrorCode _error_code;
    common::CondVar _finish_cond;
};

} // namespace tera

#endif  // TERA_SDK_LOCK_IMPL_H_
