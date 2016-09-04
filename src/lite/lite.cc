// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tera.h"
#include "lite/lite.h"

#include "io/tablet_io.h"
#include "io/tablet_writer.h"
#include "io/utils_leveldb.h"
#include "leveldb/cache.h"
#include "leveldb/db/filename.h"
#include "leveldb/db/table_cache.h"
#include "leveldb/env_flash.h"
#include "proto/tabletnode_rpc.pb.h"
#include "sdk/client_impl.h"
#include "sdk/mutate_impl.h"
#include "sdk/read_impl.h"
#include "sdk/scan_impl.h"
#include "sdk/sdk_utils.h"
#include "utils/config_utils.h"
#include "utils/utils_cmd.h"

DECLARE_string(tera_leveldb_env_type);
DECLARE_string(tera_leveldb_log_path);
DECLARE_int32(tera_tabletnode_block_cache_size);
DECLARE_int32(tera_tabletnode_table_cache_size);
DECLARE_int32(tera_tabletnode_compact_thread_num);
DECLARE_string(tera_tabletnode_cache_paths);
DECLARE_bool(tera_io_cache_path_vanish_allowed);
DECLARE_int32(tera_tabletnode_cache_update_thread_num);
DECLARE_bool(tera_tabletnode_cache_force_read_from_cache);
DECLARE_bool(tera_sync_log);

tera::Counter rand_read_delay;
tera::Counter row_read_delay;
tera::Counter range_error_counter;
tera::Counter read_pending_counter;
tera::Counter write_pending_counter;
tera::Counter scan_pending_counter;
tera::Counter compact_pending_counter;

namespace tera {

class DBResultStream : public ResultStream {
public:
    DBResultStream(io::TabletIO* io) : _io(io), _index(0), _context(new io::ScanContext) {
        _io->key_operator_->EncodeTeraKey("", "", "", kLatestTs, leveldb::TKT_VALUE,
                                          &_context->start_tera_key);
        _context->end_row_key = "";
        _context->scan_options.max_size = 10 << 20;

        leveldb::ReadOptions read_option(&_io->ldb_options_);
        read_option.verify_checksums = true;
        read_option.fill_cache = false;
        _context->it = _io->db_->NewIterator(read_option);
        _context->compact_strategy = _io->ldb_options_.compact_strategy_factory->NewInstance();
        _context->version_num = 1;
        _context->ret_code = kTabletNodeOk;
        _context->complete = false;
        _context->result = &_result;

        _context->it->Seek(_context->start_tera_key);
    }
    virtual ~DBResultStream() {
        delete _context->it;
        delete _context->compact_strategy;
        delete _context;
    }

    virtual bool LookUp(const std::string& row_key) {
        return false;
    }
    /// 是否到达结束标记
    virtual bool Done(ErrorCode* err = NULL) {
        while (_context->ret_code == kTabletNodeOk) {
            if (_index < _result.key_values_size()) {
                return false;
            }
            if (_context->complete) {
                return true;
            }
            _index = 0;
            _context->result->Clear();
            _io->ProcessScan(_context);
        }
        if (err != NULL) {
            err->SetFailed(ErrorCode::kSystem);
        }
        return true;
    }

    /// 迭代下一个cell
    virtual void Next() {
        _index++;
    }
    /// RowKey
    virtual std::string RowName() const {
        return _result.key_values(_index).key();
    }
    /// Column(格式为cf:qualifier)
    virtual std::string ColumnName() const {
        return Family() + ":" + Qualifier();
    }
    /// Column family
    virtual std::string Family() const {
        return _result.key_values(_index).column_family();
    }
    /// Qualifier
    virtual std::string Qualifier() const {
        return _result.key_values(_index).qualifier();
    }
    /// Cell对应的时间戳
    virtual int64_t Timestamp() const {
        return _result.key_values(_index).timestamp();
    }
    /// Value
    virtual std::string Value() const {
        return _result.key_values(_index).value();
    }
    virtual int64_t ValueInt64() const {
        return -1;
    }

private:
    io::TabletIO* _io;
    int32_t _index;
    RowResult _result;
    io::ScanContext* _context;
};

class DBImpl : public DB {
public:
    static void Init(const std::string& confpath) {
        utils::LoadFlagFile(confpath);
        ::google::InitGoogleLogging("teralite");
        utils::SetupLog("teralite");

        leveldb::Env::Default()->SetBackgroundThreads(FLAGS_tera_tabletnode_compact_thread_num);
        leveldb::Env::Default()->RenameFile(FLAGS_tera_leveldb_log_path,
                                            FLAGS_tera_leveldb_log_path + ".bak");
        leveldb::Status s =
            leveldb::Env::Default()->NewLogger(FLAGS_tera_leveldb_log_path, &_ldb_logger);
        assert(s.ok());
        leveldb::Env::Default()->SetLogger(_ldb_logger);

        _ldb_block_cache = leveldb::NewLRUCache(FLAGS_tera_tabletnode_block_cache_size << 20);
        _ldb_table_cache = new leveldb::TableCache(FLAGS_tera_tabletnode_table_cache_size);

        if (FLAGS_tera_leveldb_env_type != "local") {
            io::InitDfsEnv();
        }
        leveldb::FlashEnv* flash_env = (leveldb::FlashEnv*)io::LeveldbFlashEnv();
        flash_env->SetFlashPath(FLAGS_tera_tabletnode_cache_paths,
                                FLAGS_tera_io_cache_path_vanish_allowed);
        flash_env->SetUpdateFlashThreadNumber(FLAGS_tera_tabletnode_cache_update_thread_num);
        flash_env->SetIfForceReadFromCache(FLAGS_tera_tabletnode_cache_force_read_from_cache);
    }

    DBImpl(const TableDescriptor& desc)
        : _io("", "") {
        TableDescToSchema(desc, &_schema);
        _name = _schema.name();
    }

    virtual ~DBImpl() {
        CHECK(Close());
    }

    bool Open(uint64_t id) {
        std::vector<uint64_t> parent_tablets;
        std::map<uint64_t, uint64_t> snapshots;
        std::map<uint64_t, uint64_t> rollbacks;

        StatusCode status = kTableOk;
        std::string path = leveldb::GetTabletPathFromNum(_schema.name(), id);
        return _io.Load(_schema, path, parent_tablets, snapshots, rollbacks,
                        _ldb_logger, _ldb_block_cache, _ldb_table_cache, &status);
    }

    bool Close() {
        StatusCode status = kTableOk;
        return _io.Unload(&status);
    }

    /// 返回一个新的RowMutation
    virtual RowMutation* NewRowMutation(const std::string& row_key) {
        RowMutationImpl* row_mu = new RowMutationImpl(NULL, row_key);
        return row_mu;
    }

    /// 返回一个新的RowReader
    virtual RowReader* NewRowReader(const std::string& row_key) {
        RowReaderImpl* row_rd = new RowReaderImpl(NULL, row_key);
        return row_rd;
    }

    /// 修改
    virtual void Put(RowMutation* row_mutation) {
        RowMutationImpl* mu_impl = static_cast<RowMutationImpl*>(row_mutation);

        // convert RowMutation to RowMutationSequence
        RowMutationSequence mu_buf;
        mu_buf.set_row_key(mu_impl->RowKey());
        for (uint32_t j = 0; j < mu_impl->MutationNum(); j++) {
            const RowMutation::Mutation& mu_impl_mu = mu_impl->GetMutation(j);
            tera::Mutation* mu_seq_mu = mu_buf.add_mutation_sequence();
            SerializeMutation(mu_impl_mu, mu_seq_mu);
        }

        // put RowMutationSequence to TabletIO
        std::vector<const RowMutationSequence*> row_mu_buf_vec ;
        row_mu_buf_vec.push_back(&mu_buf);
        StatusCode status = kTableOk;
        _io.WriteMany(row_mu_buf_vec, true, false, &status);

        if (status == kTableOk) {
            mu_impl->SetError(ErrorCode::kOK);
        } else {
            mu_impl->SetError(ErrorCode::kSystem, StatusCodeToString(status));
        }
        mu_impl->RunCallback();
    }

    /// 读取
    virtual void Get(RowReader* row_reader) {
        RowReaderImpl* rd_impl = static_cast<RowReaderImpl*>(row_reader);
        RowReaderInfo rd_info;
        rd_impl->ToProtoBuf(&rd_info);

        StatusCode status = kTableOk;
        RowResult result;
        _io.ReadCells(rd_info, &result, rd_impl->GetSnapshot(), &status);

        if (status == kTableOk) {
            rd_impl->SetResult(result);
            rd_impl->SetError(ErrorCode::kOK);
        } else if (status == kKeyNotExist) {
            rd_impl->SetError(ErrorCode::kNotFound, "not found");
        } else if (status == kSnapshotNotExist) {
            rd_impl->SetError(ErrorCode::kNotFound, "snapshot not found");
        } else {
            rd_impl->SetError(ErrorCode::kSystem, StatusCodeToString(status));
        }
        rd_impl->RunCallback();
    }

    /// Scan, 流式读取, 返回一个数据流, 失败返回NULL
    virtual ResultStream* Scan(ErrorCode* err) {
        return new DBResultStream(&_io);
    }

    virtual std::string GetName() {
        return _name;
    }

    /// 获取表格的最小最大key
    virtual bool GetStartEndKeys(std::string* start_key, std::string* end_key,
                                 ErrorCode* err) {
        return false;
    }

    /// 获取表格描述符
    virtual bool GetDescriptor(TableDescriptor* desc, ErrorCode* err) {
        return false;
    }

private:
    io::TabletIO _io;
    TableSchema _schema;
    std::string _name;

private:
    static leveldb::Logger* _ldb_logger;
    static leveldb::Cache* _ldb_block_cache;
    static leveldb::TableCache* _ldb_table_cache;

private:
    DBImpl(const DBImpl&);
    void operator=(const DBImpl&);
};

leveldb::Logger* DBImpl::_ldb_logger = NULL;
leveldb::Cache* DBImpl::_ldb_block_cache = NULL;
leveldb::TableCache* DBImpl::_ldb_table_cache = NULL;

void DB::Init(const std::string& confpath) {
    DBImpl::Init(confpath);
}

bool DB::NewDB(const TableDescriptor& desc, uint64_t id, DB** db) {
    DBImpl* db_impl = new DBImpl(desc);
    if (!db_impl->Open(id)) {
        delete db_impl;
        return false;
    }
    *db = db_impl;
    return true;
}

bool DB::NewDB(const TableDescriptor& desc, DB** db) {
    return NewDB(desc, 1, db);
}

} // namespace tera
