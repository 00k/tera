// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sdk/lock_impl.h"

namespace tera {

RowLockImpl::RowLockImpl(TableImpl* table, const std::string& row_key, int64_t lock_id)
    : SdkTask(SdkTask::LOCK),
      _table(table),
      _row_key(row_key),
      _lock_id(lock_id),
      _lock_state(kUnlocked),
      _callback(NULL),
      _user_context(NULL),
      _timeout_ms(0),
      _retry_times(0),
      _finish(false),
      _finish_cond(&_mutex) {
}

RowLockImpl::~RowLockImpl() {
}

const std::string& RowLockImpl::RowKey() {
    return _row_key;
}

void RowLockImpl::SetCallBack(RowLockImpl::Callback callback) {
    _callback = callback;
}

RowLockImpl::Callback RowLockImpl::GetCallBack() {
    return _callback;
}

void RowLockImpl::SetContext(void* context) {
    _user_context = context;
}

void* RowLockImpl::GetContext() {
    return _user_context;
}

void RowLockImpl::SetTimeOut(int64_t timeout_ms) {
    _timeout_ms = timeout_ms;
}

int64_t RowLockImpl::TimeOut() {
    return _timeout_ms;
}

uint32_t RowLockImpl::RetryTimes() {
    return _retry_times;
}

void RowLockImpl::IncRetryTimes() {
    _retry_times++;
}

void RowLockImpl::SetError(ErrorCode::ErrorCodeType err, const std::string& reason) {
    _error_code.SetFailed(err, reason);
}

const ErrorCode& RowLockImpl::GetError() {
    return _error_code;
}

bool RowLockImpl::IsAsync() {
    return (_callback != NULL);
}

bool RowLockImpl::IsFinished() {
    MutexLock lock(&_mutex);
    return _finish;
}

void RowLockImpl::Wait() {
    MutexLock lock(&_mutex);
    while (!_finish) {
        _finish_cond.Wait();
    }
}

void RowLockImpl::RunCallback() {
    if (_callback) {
        _callback(this);
    } else {
        MutexLock lock(&_mutex);
        _finish = true;
        _finish_cond.Signal();
    }
}

void RowLockImpl::SetLockState(RowLockImpl::LockState lock_state) {
    _lock_state = lock_state;
}

RowLockImpl::LockState RowLockImpl::LockState() {
    return _lock_state;
}

int64_t RowLockImpl::LockId() {
    return _lock_id;
}

} // namespace tera

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
