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
    RowLockImpl(TableImpl* table, const std::string& row_key, const std::string& lock_id);
    virtual ~RowLockImpl();

    /// 返回row_key
    virtual const std::string& RowKey();

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
    const std::string& LockId();

    /// 设置状态
    void SetLockState(LockState lock_state);
    /// 获得状态
    enum LockState LockState();

private:
    TableImpl* _table;
    const std::string _row_key;
    const std::string _lock_id;
    enum LockState _lock_state;
};

} // namespace tera

#endif  // TERA_SDK_LOCK_IMPL_H_
