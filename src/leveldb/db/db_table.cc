// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "db/db_table.h"

#include <algorithm>
#include <iostream>
#include <vector>
#include <stdio.h>

#include "db/db_impl.h"
#include "db/filename.h"
#include "db/lg_compact_thread.h"
#include "db/log_reader.h"
#include "db/memtable.h"
#include "db/memtable.h"
#include "db/version_edit.h"
#include "db/version_set.h"
#include "db/write_batch_internal.h"
#include "leveldb/compact_strategy.h"
#include "leveldb/iterator.h"
#include "leveldb/lg_coding.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"
#include "leveldb/table_utils.h"
#include "table/merger.h"
#include "util/string_ext.h"

namespace leveldb {

struct DBTable::RecordWriter {
    Status status;
    WriteBatch* batch;
    bool sync;
    bool done;
    port::CondVar cv;

    explicit RecordWriter(port::Mutex* mu) : cv(mu) {}
};

Options InitDefaultOptions(const Options& options, const std::string& dbname) {
    Options opt = options;
    Status s = opt.env->CreateDir(dbname);
    if (!s.ok()) {
        std::cerr << "[" << dbname << "] fail to create dir: "
            << s.ToString() << std::endl;
    }
    if (opt.info_log == NULL) {
        opt.env->RenameFile(InfoLogFileName(dbname), OldInfoLogFileName(dbname));
        s = opt.env->NewLogger(InfoLogFileName(dbname), &opt.info_log);
        if (!s.ok()) {
            // No place suitable for logging
            std::cerr << "[" << dbname << "] fail to init info log: "
                << s.ToString() << std::endl;
            opt.info_log = NULL;
        }
    }
    assert(s.ok());
    assert(opt.lg_info_list);
    if (opt.compact_strategy_factory == NULL) {
        opt.compact_strategy_factory = new DummyCompactStrategyFactory();
    }
    return opt;
}

Options InitOptionsLG(const Options& options, LG_info* lg_info) {
    Options opt = options;
    if (lg_info->env) {
        opt.env = lg_info->env;
    }
    opt.compression = lg_info->compression;
    opt.block_size = lg_info->block_size;
    opt.use_memtable_on_leveldb = lg_info->use_memtable_on_leveldb;
    opt.memtable_ldb_write_buffer_size = lg_info->memtable_ldb_write_buffer_size;
    opt.memtable_ldb_block_size = lg_info->memtable_ldb_block_size;
    opt.sst_size = lg_info->sst_size;
    return opt;
}

DBTable::DBTable(const Options& options, const std::string& dbname)
    : shutdown_phase_(0), shutting_down_(NULL), bg_cv_(&mutex_),
      bg_cv_timer_(&mutex_), bg_cv_sleeper_(&mutex_),
      options_(InitDefaultOptions(options, dbname)),
      dbname_(dbname), env_(options.env),
      created_own_lg_list_(options_.lg_info_list != options.lg_info_list),
      created_own_info_log_(options_.info_log != options.info_log),
      created_own_compact_strategy_(options_.compact_strategy_factory != options.compact_strategy_factory),
      commit_snapshot_(kMaxSequenceNumber), logfile_(NULL), log_(NULL), force_switch_log_(false),
      last_sequence_(0), current_log_size_(0),
      tmp_batch_(new WriteBatch),
      bg_schedule_gc_(false), bg_schedule_gc_id_(0),
      bg_schedule_gc_score_(0), force_clean_log_seq_(0) {
}

Status DBTable::Shutdown1() {
    assert(shutdown_phase_ == 0);
    shutdown_phase_ = 1;

    Log(options_.info_log, "[%s] shutdown1 start", dbname_.c_str());
    shutting_down_.Release_Store(this);

    Status s;
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        Status impl_s = lg->Shutdown1();
        if (!impl_s.ok()) {
            s = impl_s;
        }
    }

    Log(options_.info_log, "[%s] wait bg garbage clean finish", dbname_.c_str());
    mutex_.Lock();
    while (bg_schedule_gc_) {
        bg_cv_.Wait();
    }
    mutex_.Unlock();

    Log(options_.info_log, "[%s] fg garbage clean", dbname_.c_str());
    GarbageClean();

    Log(options_.info_log, "[%s] shutdown1 done", dbname_.c_str());
    return s;
}

Status DBTable::Shutdown2() {
    assert(shutdown_phase_ == 1);
    shutdown_phase_ = 2;

    Log(options_.info_log, "[%s] shutdown2 start", dbname_.c_str());

    Status s;
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        Status impl_s = lg->Shutdown2();
        if (!impl_s.ok()) {
            s = impl_s;
        }
    }

    Log(options_.info_log, "[%s] stop async log", dbname_.c_str());
    if (log_) {
        log_->Stop(false);
    }

    if (s.ok() && options_.dump_mem_on_shutdown) {
        Log(options_.info_log, "[%s] gather all log file", dbname_.c_str());
        std::vector<uint64_t> logfiles;
        s = GatherLogFile(0, &logfiles);
        if (s.ok()) {
            Log(options_.info_log, "[%s] delete all log file", dbname_.c_str());
            s = DeleteLogFile(logfiles);
        }
    }

    Log(options_.info_log, "[%s] shutdown2 done", dbname_.c_str());
    return s;
}

DBTable::~DBTable() {
    assert(shutdown_phase_ >= 0 && shutdown_phase_ <= 2);
    // Shutdown1 must be called before delete.
    // Shutdown2 is both OK to be called or not.
    // But if Shutdown1 returns non-ok, Shutdown2 must NOT be called.
    if (shutdown_phase_ < 1) {
        Status s = Shutdown1();
        if (s.ok()) {
            Shutdown2();
        }
    }
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        delete it->second;
    }
    lg_list_.clear();

    if (created_own_lg_list_) {
        delete options_.lg_info_list;
    }

    if (created_own_compact_strategy_) {
        delete options_.compact_strategy_factory;
    }

    if (created_own_info_log_) {
        delete options_.info_log;
    }
    delete tmp_batch_;
}

Status DBTable::Init() {
    std::vector<VersionEdit> lg_edits;
    lg_edits.resize(options_.lg_info_list->size());
    Status s;
    Log(options_.info_log, "[%s] start Init()", dbname_.c_str());
    MutexLock lock(&mutex_);

    int32_t i = 0;
    uint64_t min_log_sequence = kMaxSequenceNumber;
    std::vector<uint64_t> snapshot_sequence = options_.snapshots_sequence;
    std::map<std::string, LG_info*>::iterator it = options_.lg_info_list->begin();
    for (; it != options_.lg_info_list->end() && s.ok(); ++it) {
        const std::string& lg_name = it->first;
        LG_info* lg_info = it->second;
        uint32_t id = lg_info->lg_id;
        DBImpl* lg = new DBImpl(InitOptionsLG(options_, lg_info),
                                dbname_ + "/" + Uint64ToString(id));
        lg_list_[lg_name] = lg;
        for (uint32_t j = 0; j < snapshot_sequence.size(); ++j) {
            lg->GetSnapshot(snapshot_sequence[j]);
        }

        // recover SST
        Log(options_.info_log, "[%s] start Recover lg %s, last_seq= %lu",
            dbname_.c_str(), lg_name.c_str(), lg->GetLastSequence());
        s = lg->Recover(&lg_edits[i]);
        Log(options_.info_log, "[%s] end Recover lg %s, last_seq= %lu",
            dbname_.c_str(), lg_name.c_str(), lg->GetLastSequence());
        if (s.ok()) {
            uint64_t last_seq = lg->GetLastSequence();

            Log(options_.info_log,
                "[%s] Recover lg %s last_log_seq= %lu", dbname_.c_str(), lg_name.c_str(), last_seq);
            if (min_log_sequence > last_seq) {
                min_log_sequence = last_seq;
            }
            if (last_sequence_ < last_seq) {
                last_sequence_ = last_seq;
            }
        } else {
            Log(options_.info_log, "[%s] fail to recover lg %s", dbname_.c_str(), lg_name.c_str());
            break;
        }
        i++;
    }

    std::vector<uint64_t> logfiles;
    if (s.ok()) {
        Log(options_.info_log, "[%s] start GatherLogFile", dbname_.c_str());
        // recover log files
        s = GatherLogFile(min_log_sequence + 1, &logfiles);
    }

    if (s.ok()) {
        for (uint32_t i = 0; i < logfiles.size(); ++i) {
            // If two log files have overlap sequence id, ignore records
            // from old log.
            uint64_t recover_limit = kMaxSequenceNumber;
            if (i < logfiles.size() - 1) {
                recover_limit = logfiles[i + 1];
            }
            s = RecoverLogFile(logfiles[i], recover_limit, &lg_edits);
            if (!s.ok()) {
                Log(options_.info_log, "[%s] Fail to RecoverLogFile %ld",
                    dbname_.c_str(), logfiles[i]);
            }
        }
    } else {
        Log(options_.info_log, "[%s] Fail to GatherLogFile", dbname_.c_str());
    }

    if (s.ok()) {
        i = 0;
        Log(options_.info_log, "[%s] start RecoverLogToLevel0Table", dbname_.c_str());
        std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
        for (; it != lg_list_.end(); ++it) {
            DBImpl* lg = it->second;
            s = lg->RecoverLastDumpToLevel0(&lg_edits[i]);

            // LogAndApply to lg's manifest
            if (s.ok()) {
                MutexLock lock(&lg->mutex_);
                s = lg->versions_->LogAndApply(&lg_edits[i], &lg->mutex_);
                if (s.ok()) {
                    lg->DeleteObsoleteFiles();
                    lg->MaybeScheduleCompaction();
                } else {
                    Log(options_.info_log, "[%s] Fail to modify manifest of lg %d",
                        dbname_.c_str(),
                        i);
                }
            } else {
                Log(options_.info_log, "[%s] Fail to dump log to level 0", dbname_.c_str());
            }
            i++;
        }
    }

    if (s.ok()) {
        Log(options_.info_log, "[%s] start DeleteLogFile", dbname_.c_str());
        s = DeleteLogFile(logfiles);
    }

    if (s.ok() && !options_.disable_wal) {
        std::string log_file_name = LogHexFileName(dbname_, last_sequence_ + 1);
        s = options_.env->NewWritableFile(log_file_name, &logfile_);
        if (s.ok()) {
            //Log(options_.info_log, "[%s] open logfile %s",
            //    dbname_.c_str(), log_file_name.c_str());
            log_ = new log::AsyncWriter(logfile_, options_.log_async_mode);
        } else {
            Log(options_.info_log, "[%s] fail to open logfile %s",
                dbname_.c_str(), log_file_name.c_str());
        }
    }

    if (s.ok()) {
        Log(options_.info_log, "[%s] custom compact strategy: %s, flush trigger %lu",
            dbname_.c_str(), options_.compact_strategy_factory->Name(),
            options_.flush_triggered_log_num);

        Log(options_.info_log, "[%s] Init() done, last_seq=%llu", dbname_.c_str(),
            static_cast<unsigned long long>(last_sequence_));
    }

    if (!s.ok()) {
        Log(options_.info_log, "[%s] fail to init table.", dbname_.c_str());
        std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
        for (; it != lg_list_.end(); ++it) {
            delete it->second;
        }
        lg_list_.clear();
    }
    return s;
}

Status DBTable::Put(const WriteOptions& options,
                    const Slice& key, const Slice& value) {
    return DB::Put(options, key, value);
}

Status DBTable::Delete(const WriteOptions& options, const Slice& key) {
    return DB::Delete(options, key);
}

bool DBTable::BusyWrite() {
    MutexLock l(&mutex_);
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        if (lg->BusyWrite()) {
            return true;
        }
    }
    return false;
}

Status DBTable::Write(const WriteOptions& options, WriteBatch* my_batch) {
    RecordWriter w(&mutex_);
    w.batch = my_batch;
    w.sync = options.sync;
    w.done = false;

    MutexLock l(&mutex_);
    writers_.push_back(&w);
    while (!w.done && &w != writers_.front()) {
        w.cv.Wait();
    }
    if (w.done) {
        return w.status;
    }

    // DB with fatal error is unwritable.
    Status s = fatal_error_;

    RecordWriter* last_writer = &w;
    WriteBatch* updates = NULL;
    if (s.ok()) {
        updates = GroupWriteBatch(&last_writer);
        WriteBatchInternal::SetSequence(updates, last_sequence_ + 1);
    }

    if (s.ok() && !options_.disable_wal && !options.disable_wal) {
        if (force_switch_log_ || current_log_size_ > options_.log_file_size) {
            mutex_.Unlock();
            if (SwitchLog(false) == 2) {
                s = Status::IOError(dbname_ + ": fail to open log: ", s.ToString());
            } else {
                force_switch_log_ = false;
            }
            mutex_.Lock();
        }
    }

    // dump to log
    if (s.ok() && !options_.disable_wal && !options.disable_wal) {
        mutex_.Unlock();

        Slice slice = WriteBatchInternal::Contents(updates);
        uint32_t wait_sec = options_.write_log_time_out;
        for (; ; wait_sec <<= 1) {
            // write a record into log
            log_->AddRecord(slice);
            s = log_->WaitDone(wait_sec);
            if (s.IsTimeOut()) {
                Log(options_.info_log, "[%s] AddRecord time out %lu",
                    dbname_.c_str(), current_log_size_);
                int ret = SwitchLog(true);
                if (ret == 0) {
                    continue;
                } else if (ret == 1) {
                    s = log_->WaitDone(-1);
                    if (!s.ok()) {
                        break;
                    }
                } else {
                    s = Status::IOError(dbname_ + ": fail to open log: ", s.ToString());
                    break;
                }
            }
            // do sync if needed
            if (!s.ok()) {
                s = Status::IOError(dbname_ + ": fail to write log: ", s.ToString());
                force_switch_log_ = true;
            } else {
                log_->Sync(options.sync);
                s = log_->WaitDone(wait_sec);
                if (s.IsTimeOut()) {
                    Log(options_.info_log, "[%s] Sync time out %lu",
                        dbname_.c_str(), current_log_size_);
                    int ret = SwitchLog(true);
                    if (ret == 0) {
                        continue;
                    } else if (ret == 1) {
                        s = log_->WaitDone(-1);
                        if (s.ok()) {
                            continue;
                        }
                    } else {
                        s = Status::IOError(dbname_ + ": fail to open log: ", s.ToString());
                        break;
                    }
                }
                if (!s.ok()) {
                    s = Status::IOError(dbname_ + ": fail to sync log: ", s.ToString());
                    force_switch_log_ = true;
                }
            }
            break;
        }
        mutex_.Lock();
    }
    if (s.ok()) {
        std::map<std::string, WriteBatch> lg_updates;
        // kv version may not create snapshot
        std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
        for (; it != lg_list_.end(); ++it) {
            const std::string& lg_name = it->first;
            DBImpl* lg = it->second;
            lg_updates[lg_name].Clear();
            lg->GetSnapshot(last_sequence_);
        }
        commit_snapshot_ = last_sequence_;
        if (lg_list_.size() > 1) {
            updates->SeperateLocalityGroup(&lg_updates);
        }
        mutex_.Unlock();
        it = lg_list_.begin();
        for (; it != lg_list_.end(); ++it) {
            const std::string& lg_name = it->first;
            DBImpl* lg = it->second;

            WriteBatch* wb = NULL;
            if (lg_list_.size() > 1) {
                wb = &lg_updates[lg_name];
            } else {
                wb = updates;
            }
            Status lg_s = lg->Write(WriteOptions(), wb);
            if (!lg_s.ok()) {
                // 这种情况下内存处于不一致状态
                Log(options_.info_log, "[%s] [Fatal] Write to lg %s fail",
                    dbname_.c_str(), lg_name.c_str());
                s = lg_s;
                fatal_error_ = lg_s;
                break;
            }
        }
        mutex_.Lock();
        if (s.ok()) {
            std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
            for (; it != lg_list_.end(); ++it) {
                DBImpl* lg = it->second;
                lg->AddBoundLogSize(updates->DataSize());
                lg->ReleaseSnapshot(commit_snapshot_);
            }
            commit_snapshot_ = last_sequence_ + WriteBatchInternal::Count(updates);
        }
    }

    // Update last_sequence
    if (updates) {
        last_sequence_ += WriteBatchInternal::Count(updates);
        current_log_size_ += WriteBatchInternal::ByteSize(updates);
    }
    if (updates == tmp_batch_) tmp_batch_->Clear();

    while (true) {
        RecordWriter* ready = writers_.front();
        writers_.pop_front();
        if (ready != &w) {
            ready->status = s;
            ready->done = true;
            ready->cv.Signal();
        }
        if (ready == last_writer) break;
    }

    if (!writers_.empty()) {
        writers_.front()->cv.Signal();
    }

    return s;
}

// REQUIRES: Writer list must be non-empty
// REQUIRES: First writer must have a non-NULL batch
WriteBatch* DBTable::GroupWriteBatch(RecordWriter** last_writer) {
    assert(!writers_.empty());
    RecordWriter* first = writers_.front();
    WriteBatch* result = first->batch;
    assert(result != NULL);

    size_t size = WriteBatchInternal::ByteSize(first->batch);

    // Allow the group to grow up to a maximum size, but if the
    // original write is small, limit the growth so we do not slow
    // down the small write too much.
    size_t max_size = 1 << 20;
    if (size <= (128<<10)) {
        max_size = size + (128<<10);
    }

    *last_writer = first;
    std::deque<RecordWriter*>::iterator iter = writers_.begin();
    ++iter;  // Advance past "first"
    for (; iter != writers_.end(); ++iter) {
      RecordWriter* w = *iter;
      if (w->sync && !first->sync) {
        // Do not include a sync write into a batch handled by a non-sync write.
        break;
      }

      if (w->batch != NULL) {
        size += WriteBatchInternal::ByteSize(w->batch);
        if (size > max_size) {
          // Do not make batch too big
          break;
        }

        // Append to *reuslt
        if (result == first->batch) {
          // Switch to temporary batch instead of disturbing caller's batch
          result = tmp_batch_;
          assert(WriteBatchInternal::Count(result) == 0);
          WriteBatchInternal::Append(result, first->batch);
        }
        WriteBatchInternal::Append(result, w->batch);
      } else {
        assert(0);
      }
      *last_writer = w;
    }
    return result;
}

Status DBTable::Get(const ReadOptions& options,
                    const Slice& key, std::string* value) {
    std::string lg_id = 0;
    Slice real_key = key;
    if (!GetFixed32LGId(&real_key, &lg_id)) {
        lg_id.clear();
        real_key = key;
    }
    std::map<std::string, DBImpl*>::iterator it = lg_list_.find(lg_id);
    if (it == lg_list_.end()) {
        return Status::InvalidArgument("lg invalid: " + lg_id);
    }
    DBImpl* lg = it->second;
    ReadOptions new_options = options;
    MutexLock lock(&mutex_);
    if (options.snapshot != kMaxSequenceNumber) {
        new_options.snapshot = options.snapshot;
    } else if (commit_snapshot_ != kMaxSequenceNumber) {
        new_options.snapshot = commit_snapshot_;
    }
    mutex_.Unlock();
    Status s = lg->Get(new_options, real_key, value);
    mutex_.Lock();
    return s;
}

Iterator* DBTable::NewIterator(const ReadOptions& options) {
    std::vector<Iterator*> list;
    ReadOptions new_options = options;
    mutex_.Lock();
    if (options.snapshot != kMaxSequenceNumber) {
        new_options.snapshot = options.snapshot;
    } else if (commit_snapshot_ != kMaxSequenceNumber) {
        new_options.snapshot = commit_snapshot_;
    }
    mutex_.Unlock();
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        const std::string& lg_name = it->first;
        DBImpl* lg = it->second;
        if (options.target_lgs) {
            std::set<std::string>::const_iterator found_it =
                options.target_lgs->find(lg_name);
            if (found_it == options.target_lgs->end()) {
                continue;
            }
        }
        list.push_back(lg->NewIterator(new_options));
    }
    return NewMergingIterator(options_.comparator, &list[0], list.size());
}

const uint64_t DBTable::GetSnapshot(uint64_t last_sequence) {
    MutexLock lock(&mutex_);
    uint64_t seq = last_sequence == kMaxSequenceNumber ? last_sequence_ : last_sequence;
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        lg->GetSnapshot(seq);
    }
    return seq;
}

void DBTable::ReleaseSnapshot(uint64_t sequence_number) {
    MutexLock lock(&mutex_);
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        lg->ReleaseSnapshot(sequence_number);
    }
}

bool DBTable::GetProperty(const Slice& property, std::string* value) {
    bool ret = true;
    std::string ret_string;
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        const std::string& lg_name = it->first;
        DBImpl* lg = it->second;
        std::string lg_value;
        bool lg_ret = lg->GetProperty(property, &lg_value);
        if (lg_ret) {
            if (lg_list_.size() > 1) {
                ret_string.append(lg_name + ": {\n");
            }
            ret_string.append(lg_value);
            if (lg_list_.size() > 1) {
                ret_string.append("\n}\n");
            }
        }
    }
    *value = ret_string;
    return ret;
}

void DBTable::GetApproximateSizes(const Range* range, int n,
                                  uint64_t* sizes) {
    for (int j = 0; j < n; ++j) {
        sizes[j] = 0;
    }

    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        uint64_t lg_sizes[config::kNumLevels] = {0};
        lg->GetApproximateSizes(range, n, lg_sizes);
        for (int j = 0; j < n; ++j) {
            sizes[j] += lg_sizes[j];
        }
    }
}

void DBTable::CompactRange(const Slice* begin, const Slice* end) {
    std::vector<LGCompactThread*> lg_threads;
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        const std::string& lg_name = it->first;
        DBImpl* lg = it->second;
        LGCompactThread* thread = new LGCompactThread(lg_name, lg, begin, end);
        lg_threads.push_back(thread);
        thread->Start();
    }
    for (uint32_t i = 0; i < lg_threads.size(); ++i) {
        lg_threads[i]->Join();
        delete lg_threads[i];
    }
}

Status DBTable::GatherLogFile(uint64_t begin_num,
                              std::vector<uint64_t>* logfiles) {
    std::vector<std::string> files;
    Status s = env_->GetChildren(dbname_, &files);
    if (!s.ok()) {
        Log(options_.info_log, "[%s] GatherLogFile fail", dbname_.c_str());
        return s;
    }
    uint64_t number = 0;
    uint64_t last_number = 0;
    FileType type;
    for (uint32_t i = 0; i < files.size(); ++i) {
        type = kUnknown;
        number = 0;
        if (ParseFileName(files[i], &number, &type)
            && type == kLogFile && number >= begin_num) {
            logfiles->push_back(number);
        } else if (type == kLogFile && number > 0) {
            last_number = number;
        }
    }
    std::sort(logfiles->begin(), logfiles->end());
    uint64_t first_log_num = logfiles->size() ? (*logfiles)[0] : 0;
    Log(options_.info_log, "[%s] begin_seq= %lu, first log num= %lu, last num=%lu, log_num=%lu\n",
        dbname_.c_str(), begin_num, first_log_num, last_number, logfiles->size());
    if (last_number > 0 && first_log_num > begin_num) {
        logfiles->push_back(last_number);
    }
    std::sort(logfiles->begin(), logfiles->end());
    return s;
}

Status DBTable::RecoverLogFile(uint64_t log_number, uint64_t recover_limit,
                               std::vector<VersionEdit>* edit_list) {
    struct LogReporter : public log::Reader::Reporter {
        Env* env;
        Logger* info_log;
        const char* fname;
        Status* status;  // NULL if options_.paranoid_checks==false
        virtual void Corruption(size_t bytes, const Status& s) {
            Log(info_log, "%s%s: dropping %d bytes; %s",
                (this->status == NULL ? "(ignoring error) " : ""),
                fname, static_cast<int>(bytes), s.ToString().c_str());
            if (this->status != NULL && this->status->ok()) *this->status = s;
        }
    };

    mutex_.AssertHeld();

    // Open the log file
    std::string fname = LogHexFileName(dbname_, log_number);
    SequentialFile* file;
    Status status = env_->NewSequentialFile(fname, &file);
    if (!status.ok()) {
        MaybeIgnoreError(&status);
        return status;
    }

    // Create the log reader.
    LogReporter reporter;
    reporter.env = env_;
    reporter.info_log = options_.info_log;
    reporter.fname = fname.c_str();
    reporter.status = (options_.paranoid_checks ? &status : NULL);
    log::Reader reader(file, &reporter, true/*checksum*/,
                     0/*initial_offset*/);
    Log(options_.info_log, "[%s] Recovering log #%lx, sequence limit %lu",
        dbname_.c_str(), log_number, recover_limit);

    // Read all the records and add to a memtable
    std::string scratch;
    Slice record;
    WriteBatch batch;
    while (reader.ReadRecord(&record, &scratch) && status.ok()) {
        if (record.size() < 12) {
            reporter.Corruption(record.size(),
                                Status::Corruption("log record too small"));
            continue;
        }
        WriteBatchInternal::SetContents(&batch, record);
        uint64_t first_seq = WriteBatchInternal::Sequence(&batch);
        uint64_t last_seq = first_seq + WriteBatchInternal::Count(&batch) - 1;
        //Log(options_.info_log, "[%s] batch_seq= %lu, last_seq= %lu, count=%d",
        //    dbname_.c_str(), batch_seq, last_sequence_, WriteBatchInternal::Count(&batch));
        if (last_seq >= recover_limit) {
            Log(options_.info_log, "[%s] exceed limit %lu, ignore %lu ~ %lu",
                        dbname_.c_str(), recover_limit, first_seq, last_seq);
            continue;
        }

        if (last_seq > last_sequence_) {
            last_sequence_ = last_seq;
        }

        std::map<std::string, WriteBatch> lg_updates;
        if (lg_list_.size() > 1) {
            std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
            for (; it != lg_list_.end(); ++it) {
                lg_updates[it->first].Clear();
            }
            status = batch.SeperateLocalityGroup(&lg_updates);
            if (!status.ok()) {
                return status;
            }
        }

        if (status.ok()) {
            int32_t i = 0;
            std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
            for (; it != lg_list_.end(); ++it) {
                const std::string& lg_name = it->first;
                DBImpl* lg = it->second;
                if (last_seq <= lg->GetLastSequence()) {
                    continue;
                }
                WriteBatch* wb = NULL;
                if (lg_list_.size() > 1) {
                    wb = &lg_updates[lg_name];
                } else {
                    wb = &batch;
                }
                uint64_t first = WriteBatchInternal::Sequence(wb);
                uint64_t last = first + WriteBatchInternal::Count(wb) - 1;
                // Log(options_.info_log, "[%s] recover log batch first= %lu, last= %lu\n",
                //     dbname_.c_str(), first, last);

                Status lg_s = lg->RecoverInsertMem(wb, &(*edit_list)[i]);
                if (!lg_s.ok()) {
                    Log(options_.info_log, "[%s] recover log fail batch first= %lu, last= %lu\n",
                        dbname_.c_str(), first, last);
                    status = lg_s;
                }
                i++;
            }
        }
    }
    delete file;

    return status;
}

void DBTable::MaybeIgnoreError(Status* s) const {
    if (s->ok() || options_.paranoid_checks) {
        // No change needed
    } else {
        Log(options_.info_log, "[%s] Ignoring error %s",
            dbname_.c_str(), s->ToString().c_str());
        *s = Status::OK();
    }
}

Status DBTable::DeleteLogFile(const std::vector<uint64_t>& log_numbers) {
    Status s;
    for (uint32_t i = 0; i < log_numbers.size() && s.ok(); ++i) {
        uint64_t log_number = log_numbers[i];
        Log(options_.info_log, "[%s] Delete type=%s #%llu",
            dbname_.c_str(), FileTypeToString(kLogFile),
            static_cast<unsigned long long>(log_number));
        std::string fname = LogHexFileName(dbname_, log_number);
        s = env_->DeleteFile(fname);
        // The last log file must be deleted before write a new log
        // in case of record sequence_id overlap;
        // Fail to delete other log files may be accepted.
        if (i < log_numbers.size() - 1) {
            MaybeIgnoreError(&s);
        }
        if (!s.ok()) {
            Log(options_.info_log, "[%s] fail to delete logfile %llu: %s",
                dbname_.c_str(), static_cast<unsigned long long>(log_number),
                s.ToString().data());
        }
    }
    return s;
}

void DBTable::DeleteObsoleteFiles(uint64_t seq_no) {
    std::vector<std::string> filenames;
    env_->GetChildren(dbname_, &filenames);
    std::sort(filenames.begin(), filenames.end());
    Log(options_.info_log, "[%s] will delete obsolete file num: %u [seq < %llu]",
        dbname_.c_str(), static_cast<uint32_t>(filenames.size()),
        static_cast<unsigned long long>(seq_no));
    uint64_t number;
    FileType type;
    std::string last_file;
    for (size_t i = 0; i < filenames.size(); ++i) {
        bool deleted = false;
        if (ParseFileName(filenames[i], &number, &type)
            && type == kLogFile && number < seq_no) {
            deleted = true;
        }
        if (deleted) {
            Log(options_.info_log, "[%s] Delete type=%s #%llu",
                dbname_.c_str(), FileTypeToString(type),
                static_cast<unsigned long long>(number));
            if (!last_file.empty()) {
//                 ArchiveFile(dbname_ + "/" + last_file);
                env_->DeleteFile(dbname_ + "/" + last_file);
            }
            last_file = filenames[i];
        }
    }
}

void DBTable::ArchiveFile(const std::string& fname) {
    const char* slash = strrchr(fname.c_str(), '/');
    std::string new_dir;
    if (slash != NULL) {
        new_dir.assign(fname.data(), slash - fname.data());
    }
    new_dir.append("/lost");
    env_->CreateDir(new_dir);  // Ignore error
    std::string new_file = new_dir;
    new_file.append("/");
    new_file.append((slash == NULL) ? fname.c_str() : slash + 1);
    Status s = env_->RenameFile(fname, new_file);
    Log(options_.info_log, "[%s] Archiving %s: %s\n",
        dbname_.c_str(),
        fname.c_str(), s.ToString().c_str());
}

// tera-specific
bool DBTable::FindSplitKey(const std::string& start_key,
                           const std::string& end_key,
                           double ratio,
                           std::string* split_key) {
    DBImpl* largest_lg = NULL;
    uint64_t largest_lg_size = 0;
    MutexLock l(&mutex_);
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        uint64_t size = lg->GetScopeSize(start_key, end_key);
        if (largest_lg_size < size) {
            largest_lg_size = size;
            largest_lg = lg;
        }
    }
    if (largest_lg == NULL) {
        return false;
    }
    return largest_lg->FindSplitKey(start_key, end_key, ratio, split_key);
}

uint64_t DBTable::GetScopeSize(const std::string& start_key,
                               const std::string& end_key,
                               std::vector<uint64_t>* lgsize) {
    uint64_t size = 0;
    if (lgsize != NULL) {
        lgsize->clear();
    }
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        uint64_t lsize = lg->GetScopeSize(start_key, end_key);
        size += lsize;
        if (lgsize != NULL) {
            lgsize->push_back(lsize);
        }
    }
    return size;
}

bool DBTable::MinorCompact() {
    bool ok = true;
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        bool ret = lg->MinorCompact();
        ok = (ok && ret);
    }
    MutexLock lock(&mutex_);
    ScheduleGarbageClean(20.0);
    return ok;
}

void DBTable::CompactMissFiles(const Slice* begin, const Slice* end) {
    std::vector<LGCompactThread*> lg_threads;
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        const std::string& lg_name = it->first;
        DBImpl* lg = it->second;
        LGCompactThread* thread = new LGCompactThread(lg_name, lg, begin, end, true);
        lg_threads.push_back(thread);
        thread->Start();
    }
    for (uint32_t i = 0; i < lg_threads.size(); ++i) {
        lg_threads[i]->Join();
        delete lg_threads[i];
    }
}

void DBTable::AddInheritedLiveFiles(std::vector<std::set<uint64_t> >* live) {
    size_t lg_num = lg_list_.size();
    assert(live && live->size() == lg_num);
    {
        MutexLock l(&mutex_);
        std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
        for (; it != lg_list_.end(); ++it) {
            DBImpl* lg = it->second;
            lg->AddInheritedLiveFiles(live);
        }
    }
    //Log(options_.info_log, "[%s] finish collect inherited sst fils",
    //    dbname_.c_str());
}

// end of tera-specific

// for unit test
Status DBTable::TEST_CompactMemTable() {
    Status s;
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        s = lg->TEST_CompactMemTable();
        if (!s.ok()) {
            return s;
        }
    }
    return Status::OK();
}

void DBTable::TEST_CompactRange(int level, const Slice* begin, const Slice* end) {
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        lg->TEST_CompactRange(level, begin, end);
    }
}

Iterator* DBTable::TEST_NewInternalIterator() {
    std::vector<Iterator*> list;
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        list.push_back(lg->TEST_NewInternalIterator());
    }
    return NewMergingIterator(options_.comparator, &list[0], list.size());
}

int64_t DBTable::TEST_MaxNextLevelOverlappingBytes() {
    return 0;
}

int DBTable::SwitchLog(bool blocked_switch) {
    if (!blocked_switch ||
        log::AsyncWriter::BlockLogNum() < options_.max_block_log_number) {
        if (current_log_size_ == 0) {
            last_sequence_++;
        }
        WritableFile* logfile = NULL;
        std::string log_file_name = LogHexFileName(dbname_, last_sequence_ + 1);
        Status s = env_->NewWritableFile(log_file_name, &logfile);
        if (s.ok()) {
            log_->Stop(blocked_switch);
            logfile_ = logfile;
            log_ = new log::AsyncWriter(logfile, options_.log_async_mode);
            current_log_size_ = 0;

            // protect bg thread cv
            mutex_.Lock();
            ScheduleGarbageClean(10.0);
            mutex_.Unlock();

            if (blocked_switch) {
                // if we switched log because it was blocked
                log::AsyncWriter::BlockLogNumInc();
                Log(options_.info_log, "[%s] SwitchLog", dbname_.c_str());
            }
            return 0;   // success
        } else {
            Log(options_.info_log, "[%s] fail to open logfile %s. SwitchLog failed",
                    dbname_.c_str(), log_file_name.c_str());
            if (!blocked_switch) {
                return 2;   // wanted to switch log but failed
            }
        }
    }
    return 1;   // cannot switch log right now
}

void DBTable::ScheduleGarbageClean(double score) {
    mutex_.AssertHeld();
    if (shutting_down_.Acquire_Load()) {
        return;
    }

    if (bg_schedule_gc_ && score <= bg_schedule_gc_score_) {
        return;
    } else if (bg_schedule_gc_) {
        Log(options_.info_log, "[%s] ReSchedule Garbage clean[%ld] score= %.2f",
            dbname_.c_str(), bg_schedule_gc_id_, score);
        env_->ReSchedule(bg_schedule_gc_id_, score);
        bg_schedule_gc_score_ = score;
    } else {
        Log(options_.info_log, "[%s] Schedule Garbage clean[%ld] score= %.2f",
            dbname_.c_str(), bg_schedule_gc_id_, score);
        bg_schedule_gc_id_ = env_->Schedule(&DBTable::GarbageCleanWrapper, this, score);
        bg_schedule_gc_score_ = score;
        bg_schedule_gc_ = true;
    }
}

void DBTable::GarbageCleanWrapper(void* db) {
    DBTable* db_table = reinterpret_cast<DBTable*>(db);
    db_table->BackgroundGarbageClean();
}

void DBTable::BackgroundGarbageClean() {
    if (!shutting_down_.Acquire_Load()) {
        GarbageClean();
    }
    MutexLock lock(&mutex_);
    bg_schedule_gc_ = false;
    bg_cv_.Signal();
}

void DBTable::GarbageClean() {
    uint64_t min_last_seq = -1U;
    bool found = false;
    std::map<std::string, DBImpl*>::iterator it = lg_list_.begin();
    for (; it != lg_list_.end(); ++it) {
        DBImpl* lg = it->second;
        uint64_t last_seq = lg->GetLastVerSequence();
        if (last_seq < min_last_seq) {
            min_last_seq = last_seq;
            found = true;
        }
    }
    if (force_clean_log_seq_ > min_last_seq) {
        Log(options_.info_log, "[%s] force_clean_log_seq_= %lu, min_last_seq= %lu",
            dbname_.c_str(), force_clean_log_seq_, min_last_seq);
        min_last_seq = force_clean_log_seq_;
        found = true;
    }

    if (found && min_last_seq > 0) {
        Log(options_.info_log, "[%s] delete obsolete file, seq_no below: %lu",
            dbname_.c_str(), min_last_seq);
        DeleteObsoleteFiles(min_last_seq);
    }
}

} // namespace leveldb
