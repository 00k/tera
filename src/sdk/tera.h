// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef  TERA_TERA_H_
#define  TERA_TERA_H_

#include <stdint.h>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace tera {

/// ����������
class ErrorCode {
public:
    enum ErrorCodeType {
        kOK = 0,
        kNotFound,
        kBadParam,
        kSystem,
        kTimeout,
        kBusy,
        kNoQuota,
        kNoAuth,
        kUnknown,
        kNotImpl
    };
    ErrorCode();
    void SetFailed(ErrorCodeType err, const std::string& reason = "");
    std::string GetReason() const;
    ErrorCodeType GetType() const;

private:
    ErrorCodeType _err;
    std::string _reason;
};

/// ��tera������ת��Ϊ�ɶ��ַ���
const char* strerr(ErrorCode error_code);

enum CompressType {
    kNoneCompress = 1,
    kSnappyCompress = 2,
};

enum StoreType {
    kInDisk = 0,
    kInFlash = 1,
    kInMemory = 2,
};

/// RawKeyƴװģʽ��kReadable���ܽϸߣ���key�в�����'\0'��kBinary���ܵ�һЩ�����������ַ�
enum RawKeyType {
    kReadable = 0,
    kBinary = 1,
    kTTLKv = 2,
};

extern const int64_t kLatestTimestamp;
extern const int64_t kOldestTimestamp;

/// ACL
struct ACL {
    int32_t owner;  ///< �����û�id
    int32_t role;   ///<
    int64_t acl;    ///<
};

class ColumnFamilyDescriptor {
public:
    ColumnFamilyDescriptor() {}
    virtual ~ColumnFamilyDescriptor() {}
    virtual int32_t Id() const = 0;
    virtual const std::string& Name() const = 0;
    /// �����ĸ�lg, ���ɸ���
    virtual const std::string& LocalityGroup() const = 0;
    /// ��ʷ�汾����ʱ��, ������ʱΪ0�� ��ʾ���޴����ñ���
    virtual void SetTimeToLive(int32_t ttl) = 0;
    virtual int32_t TimeToLive() const = 0;
    /// ��TTL��,���洢�İ汾��, Ĭ��3, ��Ϊ0ʱ, �رն�汾
    virtual void SetMaxVersions(int32_t max_versions) = 0;
    virtual int32_t MaxVersions() const = 0;
    /// ���ٴ洢�İ汾��,��ʹ����TTL,Ҳ���ٱ���min_versions���汾
    virtual void SetMinVersions(int32_t min_versions) = 0;
    virtual int32_t MinVersions() const = 0;
    /// �洢�޶�, MBytes
    virtual void SetDiskQuota(int64_t quota) = 0;
    virtual int64_t DiskQuota() const = 0;
    /// ACL
    virtual void SetAcl(ACL acl) = 0;
    virtual ACL Acl() const = 0;

    virtual void SetType(const std::string& type) = 0;
    virtual const std::string& Type() const = 0;

private:
    ColumnFamilyDescriptor(const ColumnFamilyDescriptor&);
    void operator=(const ColumnFamilyDescriptor&);
};

/// �ֲ���Ⱥ������
class LocalityGroupDescriptor {
public:
    LocalityGroupDescriptor() {}
    virtual ~LocalityGroupDescriptor() {}
    /// Id read only
    virtual int32_t Id() const = 0;
    /// Name
    virtual const std::string& Name() const = 0;
    /// Compress type
    virtual void SetCompress(CompressType type) = 0;
    virtual CompressType Compress() const = 0;
    /// Block size
    virtual void SetBlockSize(int block_size) = 0;
    virtual int BlockSize() const = 0;
    /// Store type
    virtual void SetStore(StoreType type) = 0;
    virtual StoreType Store() const = 0;
    /// Bloomfilter
    virtual void SetUseBloomfilter(bool use_bloomfilter) = 0;
    virtual bool UseBloomfilter() const = 0;
    /// Memtable On Leveldb (disable/enable)
    virtual bool UseMemtableOnLeveldb() const = 0 ;
    virtual void SetUseMemtableOnLeveldb(bool use_mem_ldb) = 0;
    /// Memtable-LDB WriteBuffer Size
    virtual int32_t MemtableLdbWriteBufferSize() const = 0;
    virtual void SetMemtableLdbWriterBufferSize(int32_t buffer_size) = 0;
    /// Memtable-LDB Block Size
    virtual int32_t MemtableLdbBlockSize() const = 0;
    virtual void SetMemtableLdbBlockSize(int32_t block_size) = 0;

private:
    LocalityGroupDescriptor(const LocalityGroupDescriptor&);
    void operator=(const LocalityGroupDescriptor&);
};

class TableDescImpl;

class TableDescriptor {
public:
    /// ������ֽ�����ʹ����ĸ�����ֺ��»��߹���,���Ȳ�����256��Ĭ���Ƿ�kv��
    TableDescriptor(const std::string& tb_name, bool is_kv = false);

    ~TableDescriptor();

    std::string TableName() const;

    /// ����һ��localitygroup, ���ֽ�����ʹ����ĸ�����ֺ��»��߹���,���Ȳ�����256
    LocalityGroupDescriptor* AddLocalityGroup(const std::string& lg_name);
    LocalityGroupDescriptor* DefaultLocalityGroup();
    /// ɾ��һ��localitygroup, ������cf�������lgʱ, ��ɾ��ʧ��.
    bool RemoveLocalityGroup(const std::string& lg_name);
    /// ��ȡid��Ӧ��localitygroup
    const LocalityGroupDescriptor* LocalityGroup(int32_t id) const;
    const LocalityGroupDescriptor* LocalityGroup(const std::string& lg_name) const;
    /// LG����
    int32_t LocalityGroupNum() const;

    /// ����һ��columnfamily, ���ֽ�����ʹ����ĸ�����ֺ��»��߹���,���Ȳ�����256
    ColumnFamilyDescriptor* AddColumnFamily(const std::string& cf_name,
                                            const std::string& lg_name = "lg0");
    /// ɾ��һ��columnfamily
    void RemoveColumnFamily(const std::string& cf_name);
    /// ��ȡid��Ӧ��CF
    const ColumnFamilyDescriptor* ColumnFamily(int32_t id) const;
    const ColumnFamilyDescriptor* ColumnFamily(const std::string& cf_name) const;
    ColumnFamilyDescriptor* DefaultColumnFamily();
    /// CF����
    int32_t ColumnFamilyNum() const;

    /// Raw Key Mode
    void SetRawKey(RawKeyType type);
    RawKeyType RawKey() const;

    void SetSplitSize(int64_t size);
    int64_t SplitSize() const;

    void SetMergeSize(int64_t size);
    int64_t MergeSize() const;

    /// ����snapshot
    int32_t AddSnapshot(uint64_t snapshot);
    /// ��ȡsnapshot
    uint64_t Snapshot(int32_t id) const;
    /// Snapshot����
    int32_t SnapshotNum() const;
    /// �Ƿ�Ϊkv��
    void SetKvOnly();
    bool IsKv() const;

private:
    TableDescriptor(const TableDescriptor&);
    void operator=(const TableDescriptor&);
    TableDescImpl* _impl;
};

/// �ӱ�����ȡ�Ľ����
class ResultStream {
public:
    virtual bool LookUp(const std::string& row_key) = 0;
    /// �Ƿ񵽴�������
    virtual bool Done() = 0;
    /// ������һ��cell
    virtual void Next() = 0;
    /// RowKey
    virtual std::string RowName() const = 0;
    /// Column(��ʽΪcf:qualifier)
    virtual std::string ColumnName() const = 0;
    /// Column family
    virtual std::string Family() const = 0;
    /// Qualifier
    virtual std::string Qualifier() const = 0;
    /// Cell��Ӧ��ʱ���
    virtual int64_t Timestamp() const = 0;
    /// Value
    virtual std::string Value() const = 0;
    ResultStream() {}
    virtual ~ResultStream() {}

private:
    ResultStream(const ResultStream&);
    void operator=(const ResultStream&);
};

class ScanDescImpl;
class ScanDescriptor {
public:
    /// ͨ����ʼ�й���
    ScanDescriptor(const std::string& rowkey);
    ~ScanDescriptor();
    /// ����scan�Ľ�����(�������ڷ��ؽ����), �Ǳ�Ҫ
    void SetEnd(const std::string& rowkey);
    /// ����Ҫ��ȡ��cf, ������Ӷ��
    void AddColumnFamily(const std::string& cf);
    /// ����Ҫ��ȡ��column(cf:qualifier)
    void AddColumn(const std::string& cf, const std::string& qualifier);
    /// ������෵�صİ汾��
    void SetMaxVersions(int32_t versions);
    /// ���÷��ذ汾��ʱ�䷶Χ
    void SetTimeRange(int64_t ts_end, int64_t ts_start);
    /// ���ù��˱��ʽ����֧��AND��
    bool SetFilterString(const std::string& filter_string);
    typedef bool (*ValueConverter)(const std::string& in,
                                   const std::string& type,
                                   std::string* out);
    /// �����Զ�������ת����������������͹���ʱʹ�ã�
    void SetValueConverter(ValueConverter converter);

    void SetSnapshot(uint64_t snapshot_id);
    /// ����Ԥ����buffer��С, Ĭ��64K
    void SetBufferSize(int64_t buf_size);

    /// ����async, ȱʡtrue
    void SetAsync(bool async);

    /// �жϵ�ǰscan�Ƿ���async
    bool IsAsync() const;

    ScanDescImpl* GetImpl() const;

private:
    ScanDescriptor(const ScanDescriptor&);
    void operator=(const ScanDescriptor&);
    ScanDescImpl* _impl;
};

class RowLock {
};

class Table;
/// �޸Ĳ���
class RowMutation {
public:
    enum Type {
        kPut,
        kDeleteColumn,
        kDeleteColumns,
        kDeleteFamily,
        kDeleteRow,
        kAdd,
        kPutIfAbsent,
        kAppend
    };
    struct Mutation {
        Type type;
        std::string family;
        std::string qualifier;
        std::string value;
        int64_t timestamp;
        int32_t ttl;
    };

    RowMutation();
    virtual ~RowMutation();

    virtual const std::string& RowKey() = 0;

    virtual void Reset(const std::string& row_key) = 0;

    /// �޸�ָ����
    virtual void Put(const std::string& family, const std::string& qualifier,
                     const std::string& value) = 0;
    /// ��TTL���޸�һ����
    virtual void Put(const std::string& family, const std::string& qualifier,
             const std::string& value, int32_t ttl) = 0;
    // ԭ�Ӽ�һ��Cell
    virtual void Add(const std::string& family, const std::string& qualifier,
                     const int64_t delta) = 0;

    // ԭ�Ӳ�������������ڲ���Put�ɹ�
    virtual void PutIfAbsent(const std::string& family,
                             const std::string& qualifier,
                             const std::string& value) = 0;

    /// ԭ�Ӳ�����׷�����ݵ�һ��Cell
    virtual void Append(const std::string& family, const std::string& qualifier,
                        const std::string& value) = 0;

    /// �޸�һ���е��ض��汾
    virtual void Put(const std::string& family, const std::string& qualifier,
                     int64_t timestamp, const std::string& value) = 0;
    /// ��TTL���޸�һ���е��ض��汾
    virtual void Put(const std::string& family, const std::string& qualifier,
                     int64_t timestamp, const std::string& value, int32_t ttl) = 0;
    /// �޸�Ĭ����
    virtual void Put(const std::string& value) = 0;

    /// ��TTL���޸�Ĭ����
    virtual void Put(const std::string& value, int32_t ttl) = 0;

    /// �޸�Ĭ���е��ض��汾
    virtual void Put(int64_t timestamp, const std::string& value) = 0;

    /// ɾ��һ���е����°汾
    virtual void DeleteColumn(const std::string& family,
                              const std::string& qualifier) = 0;
    /// ɾ��һ���е�ָ���汾
    virtual void DeleteColumn(const std::string& family,
                              const std::string& qualifier,
                              int64_t timestamp) = 0;
    /// ɾ��һ���е�ȫ���汾
    virtual void DeleteColumns(const std::string& family,
                               const std::string& qualifier) = 0;
    /// ɾ��һ���е�ָ����Χ�汾
    virtual void DeleteColumns(const std::string& family,
                               const std::string& qualifier,
                               int64_t timestamp) = 0;
    /// ɾ��һ������������е�ȫ���汾
    virtual void DeleteFamily(const std::string& family) = 0;
    /// ɾ��һ������������е�ָ����Χ�汾
    virtual void DeleteFamily(const std::string& family, int64_t timestamp) = 0;
    /// ɾ�����е�ȫ������
    virtual void DeleteRow() = 0;
    /// ɾ�����е�ָ����Χ�汾
    virtual void DeleteRow(int64_t timestamp) = 0;

    /// �޸���ס����, �����ṩ����
    virtual void SetLock(RowLock* rowlock) = 0;
    /// ���ó�ʱʱ��(ֻӰ�쵱ǰ����,��Ӱ��Table::SetWriteTimeout���õ�Ĭ��д��ʱ)
    virtual void SetTimeOut(int64_t timeout_ms) = 0;
    /// ���س�ʱʱ��
    virtual int64_t TimeOut() = 0;
    /// �����첽�ص�, �������첽����
    typedef void (*Callback)(RowMutation* param);
    virtual void SetCallBack(Callback callback) = 0;
    // �����첽�ص�����
    virtual Callback GetCallBack() = 0;
    /// �����û������ģ����ڻص������л�ȡ
    virtual void SetContext(void* context) = 0;
    /// ����û�������
    virtual void* GetContext() = 0;
    /// ��ý��������
    virtual const ErrorCode& GetError() = 0;
    /// �Ƿ��첽����
    virtual bool IsAsync() = 0;
    /// �첽�����Ƿ����
    virtual bool IsFinished() const = 0;
    /// mutation����
    virtual uint32_t MutationNum() = 0;
    virtual uint32_t Size() = 0;
    virtual uint32_t RetryTimes() = 0;
    virtual const RowMutation::Mutation& GetMutation(uint32_t index) = 0;

private:
    RowMutation(const RowMutation&);
    void operator=(const RowMutation&);
};

class RowReaderImpl;
/// ��ȡ����
class RowReader {
public:
    /// �����û���ȡ�е����󣬸�ʽΪmap<family, set<qualifier> >����map��СΪ0����ʾ��ȡ����
    typedef std::map<std::string, std::set<std::string> >ReadColumnList;

    RowReader();
    virtual ~RowReader();
    /// ����row key
    virtual const std::string& RowName() = 0;
    /// ���ö�ȡ�ض��汾
    virtual void SetTimestamp(int64_t ts) = 0;
    /// ���ض�ȡʱ���
    virtual int64_t GetTimestamp() = 0;
    /// ���ÿ���id
    virtual void SetSnapshot(uint64_t snapshot_id) = 0;
    /// ȡ������id
    virtual uint64_t GetSnapshot() = 0;
    /// ��ȡ����family
    virtual void AddColumnFamily(const std::string& family) = 0;
    virtual void AddColumn(const std::string& cf_name,
                           const std::string& qualifier) = 0;

    virtual void SetTimeRange(int64_t ts_start, int64_t ts_end) = 0;
    virtual void SetMaxVersions(uint32_t max_version) = 0;
    virtual void SetTimeOut(int64_t timeout_ms) = 0;
    /// �����첽�ص�, �������첽����
    typedef void (*Callback)(RowReader* param);
    virtual void SetCallBack(Callback callback) = 0;
    /// �����û������ģ����ڻص������л�ȡ
    virtual void SetContext(void* context) = 0;
    virtual void* GetContext() = 0;
    /// �����첽����
    virtual void SetAsync() = 0;
    /// �첽�����Ƿ����
    virtual bool IsFinished() const = 0;

    /// ��ý��������
    virtual ErrorCode GetError() = 0;
    /// �Ƿ񵽴�������
    virtual bool Done() = 0;
    /// ������һ��cell
    virtual void Next() = 0;
    /// ��ȡ�Ľ��
    virtual std::string Value() = 0;
    virtual std::string Family() = 0;
    virtual std::string ColumnName() = 0;
    virtual std::string Qualifier() = 0;
    virtual int64_t Timestamp() = 0;

    virtual uint32_t GetReadColumnNum() = 0;
    virtual const ReadColumnList& GetReadColumnList() = 0;
    /// �����ת�浽һ��std::map��, ��ʽΪ: map<column, map<timestamp, value>>
    typedef std::map< std::string, std::map<int64_t, std::string> > Map;
    virtual void ToMap(Map* rowmap) = 0;

private:
    RowReader(const RowReader&);
    void operator=(const RowReader&);
};

struct TableInfo {
    TableDescriptor* table_desc;
    std::string status;
};

/// tablet�ֲ���Ϣ
struct TabletInfo {
    std::string table_name;     ///< ����
    std::string path;           ///< ·��
    std::string server_addr;    ///< tabletserver��ַ, ip:port��ʽ
    std::string start_key;      ///< ��ʼrowkey(����)
    std::string end_key;        ///< ��ֹrowkey(������)
    int64_t data_size;          ///< ������, Ϊ����ֵ
    std::string status;
};

/// ��ӿ�
class Table {
public:
    Table() {}
    virtual ~Table() {}
    /// ����һ���µ�RowMutation
    virtual RowMutation* NewRowMutation(const std::string& row_key) = 0;
    /// ����һ���µ�RowReader
    virtual RowReader* NewRowReader(const std::string& row_key) = 0;
    /// �ύһ���޸Ĳ���, ͬ�����������Ƿ�ɹ�, �첽������Զ����true
    virtual void ApplyMutation(RowMutation* row_mu) = 0;
    /// �ύһ���޸Ĳ���, ����֮�䲻��֤ԭ����, ͨ��RowMutation��GetError��ȡ���Եķ�����
    virtual void ApplyMutation(const std::vector<RowMutation*>& row_mu_list) = 0;
    /// �޸�ָ����, ����Ϊkv���ά���ʹ��ʱ�ı�ݽӿ�
    virtual bool Put(const std::string& row_key, const std::string& family,
                     const std::string& qualifier, const std::string& value,
                     ErrorCode* err) = 0;
    /// ��TTL�޸�ָ����, ����Ϊkv���ά���ʹ��ʱ�ı�ݽӿ�
    virtual bool Put(const std::string& row_key, const std::string& family,
                     const std::string& qualifier, const std::string& value,
                     int32_t ttl, ErrorCode* err) = 0;
    /// ��TTL�޸�ָ���е�ָ���汾, ����Ϊkv���ά���ʹ��ʱ�ı�ݽӿ�
    virtual bool Put(const std::string& row_key, const std::string& family,
                     const std::string& qualifier, const std::string& value,
                     int64_t timestamp, int32_t ttl, ErrorCode* err) = 0;
    /// ԭ�Ӽ�һ��Cell
    virtual bool Add(const std::string& row_key, const std::string& family,
                     const std::string& qualifier, int64_t delta,
                     ErrorCode* err) = 0;

    /// ԭ�Ӳ�������������ڲ���Put�ɹ�
    virtual bool PutIfAbsent(const std::string& row_key,
                             const std::string& family,
                             const std::string& qualifier,
                             const std::string& value,
                             ErrorCode* err) = 0;

    /// ԭ�Ӳ�����׷�����ݵ�һ��Cell
    virtual bool Append(const std::string& row_key, const std::string& family,
                        const std::string& qualifier, const std::string& value,
                        ErrorCode* err) = 0;
    /// ��ȡһ��ָ����
    virtual void Get(RowReader* row_reader) = 0;
    /// ��ȡ����
    virtual void Get(const std::vector<RowReader*>& row_readers) = 0;
    /// ��ȡָ��cell, ����Ϊkv���ά���ʹ��ʱ�ı�ݽӿ�
    virtual bool Get(const std::string& row_key, const std::string& family,
                     const std::string& qualifier, std::string* value,
                     ErrorCode* err) = 0;

    virtual bool IsPutFinished() = 0;
    virtual bool IsGetFinished() = 0;

    /// Scan, ��ʽ��ȡ, ����һ��������, ʧ�ܷ���NULL
    virtual ResultStream* Scan(const ScanDescriptor& desc, ErrorCode* err) = 0;

    virtual const std::string GetName() = 0;

    virtual bool Flush() = 0;
    /// �����޸�, ָ��Cell��ֵΪvalueʱ, ��ִ��row_mu
    virtual bool CheckAndApply(const std::string& rowkey, const std::string& cf_c,
                               const std::string& value, const RowMutation& row_mu,
                               ErrorCode* err) = 0;
    /// ������++����
    virtual int64_t IncrementColumnValue(const std::string& row, const std::string& family,
                              const std::string& qualifier, int64_t amount,
                              ErrorCode* err) = 0;
    /// ���ñ��д����Ĭ�ϳ�ʱ
    virtual void SetWriteTimeout(int64_t timeout_ms) = 0;
    virtual void SetReadTimeout(int64_t timeout_ms) = 0;

    /// ��������
    virtual bool LockRow(const std::string& rowkey, RowLock* lock, ErrorCode* err) = 0;

    /// ��ȡ������С���key
    virtual bool GetStartEndKeys(std::string* start_key, std::string* end_key,
                                 ErrorCode* err) = 0;

    /// ��ȡ���ֲ���Ϣ
    virtual bool GetTabletLocation(std::vector<TabletInfo>* tablets,
                                   ErrorCode* err) = 0;
    /// ��ȡ���������
    virtual bool GetDescriptor(TableDescriptor* desc, ErrorCode* err) = 0;

    virtual void SetMaxMutationPendingNum(uint64_t max_pending_num) = 0;
    virtual void SetMaxReaderPendingNum(uint64_t max_pending_num) = 0;

private:
    Table(const Table&);
    void operator=(const Table&);
};

class Client {
public:
    static Client* NewClient(const std::string& confpath,
                             const std::string& log_prefix,
                             ErrorCode* err = NULL);

    static Client* NewClient(const std::string& confpath,
                             ErrorCode* err = NULL);

    static Client* NewClient();

    /// �������
    virtual bool CreateTable(const TableDescriptor& desc, ErrorCode* err) = 0;
    virtual bool CreateTable(const TableDescriptor& desc,
                             const std::vector<std::string>& tablet_delim,
                             ErrorCode* err) = 0;
    /// ���±��Schema
    virtual bool UpdateTable(const TableDescriptor& desc, ErrorCode* err) = 0;
    /// ɾ�����
    virtual bool DeleteTable(std::string name, ErrorCode* err) = 0;
    /// ֹͣ������
    virtual bool DisableTable(std::string name, ErrorCode* err) = 0;
    /// �ָ�������
    virtual bool EnableTable(std::string name, ErrorCode* err) = 0;
    /// �򿪱��, ʧ�ܷ���NULL
    virtual Table* OpenTable(const std::string& table_name, ErrorCode* err) = 0;
    /// ��ȡ���ֲ���Ϣ
    virtual bool GetTabletLocation(const std::string& table_name,
                                   std::vector<TabletInfo>* tablets,
                                   ErrorCode* err) = 0;
    /// ��ȡ���Schema
    virtual TableDescriptor* GetTableDescriptor(const std::string& table_name,
                                                ErrorCode* err) = 0;

    virtual bool List(std::vector<TableInfo>* table_list,
                      ErrorCode* err) = 0;

    virtual bool List(const std::string& table_name,
                      TableInfo* table_info,
                      std::vector<TabletInfo>* tablet_list,
                      ErrorCode* err) = 0;

    virtual bool IsTableExist(const std::string& table_name, ErrorCode* err) = 0;

    virtual bool IsTableEnabled(const std::string& table_name, ErrorCode* err) = 0;

    virtual bool IsTableEmpty(const std::string& table_name, ErrorCode* err) = 0;

    virtual bool GetSnapshot(const std::string& name, uint64_t* snapshot, ErrorCode* err) = 0;
    virtual bool DelSnapshot(const std::string& name, uint64_t snapshot, ErrorCode* err) = 0;

    virtual bool CmdCtrl(const std::string& command,
                         const std::vector<std::string>& arg_list,
                         bool* bool_result,
                         std::string* str_result,
                         ErrorCode* err) = 0;

    Client() {}
    virtual ~Client() {}

private:
    Client(const Client&);
    void operator=(const Client&);
};
} // namespace tera
#endif  // TERA_TERA_H_
