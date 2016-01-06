// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sdk/lock_impl.h"

namespace tera {

RowLock::RowLock() {}
RowLock::~RowLock() {}

RowLockImpl::RowLockImpl(TableImpl* table, const std::string& row_key, const std::string& lock_id)
    : SdkTask(SdkTask::LOCK),
      _table(table),
      _row_key(row_key),
      _lock_id(lock_id),
      _lock_state(kUnlocked) {
}

RowLockImpl::~RowLockImpl() {
}

const std::string& RowLockImpl::RowKey() {
    return _row_key;
}

void RowLockImpl::SetLockState(enum RowLockImpl::LockState lock_state) {
    _lock_state = lock_state;
}

enum RowLockImpl::LockState RowLockImpl::LockState() {
    return _lock_state;
}

const std::string& RowLockImpl::LockId() {
    return _lock_id;
}

} // namespace tera

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
