// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TERA_TABLETNODE_TABLET_WRITER_H_
#define TERA_TABLETNODE_TABLET_WRITER_H_

#include "common/event.h"
#include "common/mutex.h"
#include "common/thread.h"

#include "proto/status_code.pb.h"
#include "proto/tabletnode_rpc.pb.h"
#include "utils/counter.h"
#include "utils/rpc_timer_list.h"

namespace leveldb {
class WriteBatch;
}

namespace tera {
namespace io {

class TabletIO;

class TabletWriter {
public:
    struct WriteTask {
        const WriteTabletRequest* request;
        WriteTabletResponse* response;
        google::protobuf::Closure* done;
        const std::vector<int32_t>* index_list;
        Counter* done_counter;
        WriteRpcTimer* timer;

        WriteTask() :
            request(NULL), response(NULL), done(NULL), index_list(NULL),
            done_counter(NULL), timer(NULL) {}
    };

    typedef std::vector<WriteTask> WriteTaskBuffer;

public:
    TabletWriter(TabletIO* tablet_io);
    ~TabletWriter();
    void Write(const WriteTabletRequest* request,
               WriteTabletResponse* response,
               google::protobuf::Closure* done,
               const std::vector<int32_t>* index_list,
               Counter* done_counter, WriteRpcTimer* timer = NULL);
    /// ���Լ���һ��request�����ݴ�С
    static uint64_t CountRequestSize(const WriteTabletRequest& request,
                                     const std::vector<int32_t>& index_list,
                                     bool kv_only);
    /// ��һ��request��һ��leveldbbatch��ȥ, request��ԭ�ӵ�, batchҲ��, so ..
    bool BatchRequest(const WriteTabletRequest& request,
                      const std::vector<int32_t>& index_list,
                      leveldb::WriteBatch* batch,
                      bool kv_only);
    void Start();
    void Stop();

private:
    void DoWork();
    bool SwapActiveBuffer(bool force);
    /// �������, ִ�лص�
    bool FinishTask(const WriteTask& task, StatusCode status);
    /// ��bufferˢ������(leveldb), ��sync
    void FlushToDiskBatch(WriteTaskBuffer* task_buffer);

private:
    TabletIO* m_tablet;

    mutable Mutex m_task_mutex;
    mutable Mutex m_status_mutex;
    AutoResetEvent m_write_event;       ///< �����ݿ�д
    AutoResetEvent m_worker_done_event; ///< worker�˳�

    bool m_stopped;
    common::Thread m_thread;

    WriteTaskBuffer* m_active_buffer;   ///< ǰ̨buffer,����д����
    WriteTaskBuffer* m_sealed_buffer;   ///< ��̨buffer,�ȴ�ˢ������
    int64_t m_sync_timestamp;

    bool m_active_buffer_instant;      ///< active_buffer����instant����
    uint64_t m_active_buffer_size;      ///< active_buffer�����ݴ�С
    bool m_tablet_busy;                 ///< tablet����æµ״̬
};

} // namespace tabletnode
} // namespace tera

#endif // TERA_TABLETNODE_TABLET_WRITER_H_
