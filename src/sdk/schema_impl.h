// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef  TERA_SDK_SCHEMA_IMPL_H_
#define  TERA_SDK_SCHEMA_IMPL_H_

#include <string>

#include "proto/table_meta.pb.h"
#include "sdk/tera.h"

namespace tera {

/// ��������
class CFDescImpl : public ColumnFamilyDescriptor {
public:
    /// �������ֽ�����ʹ����ĸ�����ֺ��»��߹���, ���Ȳ�����256
    CFDescImpl(const std::string& cf_name, int32_t id, const std::string& lg_name);
    /// id
    int32_t Id() const;
    const std::string& Name() const;

    const std::string& LocalityGroup() const;

    /// ��ʷ�汾����ʱ��, ������ʱΪ0�� ��ʾ���޴����ñ���
    void SetTimeToLive(int32_t ttl);

    int32_t TimeToLive() const;

    /// ��TTL��,���洢�İ汾��
    void SetMaxVersions(int32_t max_versions);

    int32_t MaxVersions() const;

    /// ���ٴ洢�İ汾��,��ʹ����TTL,Ҳ���ٱ���min_versions���汾
    void SetMinVersions(int32_t min_versions);

    int32_t MinVersions() const;

    /// �洢�޶�, MBytes
    void SetDiskQuota(int64_t quota);

    int64_t DiskQuota() const;

    /// ACL
    void SetAcl(ACL acl);

    ACL Acl() const;

    void SetType(const std::string& type);

    const std::string& Type() const;

private:
    int32_t _id;
    std::string _name;
    std::string _lg_name;
    int32_t _max_versions;
    int32_t _min_versions;
    int32_t _ttl;
    int64_t _acl;
    int32_t _owner;
    int32_t _disk_quota;
    std::string _type;
};

/// �ֲ���Ⱥ������
class LGDescImpl : public LocalityGroupDescriptor {
public:
    /// �ֲ���Ⱥ�����ֽ�����ʹ����ĸ�����ֺ��»��߹���,���Ȳ�����256
    LGDescImpl(const std::string& lg_name, int32_t id);

    /// Id read only
    int32_t Id() const;

    /// Name read only
    const std::string& Name() const;

    /// Compress type
    void SetCompress(CompressType type);

    CompressType Compress() const;

    /// Block size
    void SetBlockSize(int block_size);

    int BlockSize() const;

    /// Store type
    void SetStore(StoreType type);

    StoreType Store() const;

    /// Bloomfilter
    void SetUseBloomfilter(bool use_bloomfilter);

    bool UseBloomfilter() const;

    /// Memtable On Leveldb (disable/enable)
    bool UseMemtableOnLeveldb() const;

    void SetUseMemtableOnLeveldb(bool use_mem_ldb);

    /// Memtable-LDB WriteBuffer Size
    int32_t MemtableLdbWriteBufferSize() const;

    void SetMemtableLdbWriterBufferSize(int32_t buffer_size);

    /// Memtable-LDB Block Size
    int32_t MemtableLdbBlockSize() const;

    void SetMemtableLdbBlockSize(int32_t block_size);

private:
    int32_t         _id;
    std::string     _name;
    CompressType    _compress_type;
    StoreType       _store_type;
    int             _block_size;
    bool            _use_bloomfilter;
    bool            _use_memtable_on_leveldb;
    int32_t         _memtable_ldb_write_buffer_size;
    int32_t         _memtable_ldb_block_size;
};

/// ��������.
class TableDescImpl {
public:
    /// ������ֽ�����ʹ����ĸ�����ֺ��»��߹���,���Ȳ�����256
    TableDescImpl(const std::string& tb_name, bool is_kv);
    ~TableDescImpl();
    std::string TableName() const;
    /// ����Ϊkv�����У���������ɺ��޷��ı�
    void SetKvOnly();
    /// ����һ��localitygroup
    LocalityGroupDescriptor* AddLocalityGroup(const std::string& lg_name);
    /// ��ȡĬ��localitygroup��������kv��
    LocalityGroupDescriptor* DefaultLocalityGroup();
    /// ɾ��һ��localitygroup
    bool RemoveLocalityGroup(const std::string& lg_name);
    /// ��ȡlocalitygroup
    const LocalityGroupDescriptor* LocalityGroup(int32_t id) const;
    const LocalityGroupDescriptor* LocalityGroup(const std::string& lg_name) const;
    /// ��ȡlocalitygroup����
    int32_t LocalityGroupNum() const;
    /// ����һ��columnfamily
    ColumnFamilyDescriptor* AddColumnFamily(const std::string& cf_name,
                                            const std::string& lg_name);
    ColumnFamilyDescriptor* DefaultColumnFamily();
    /// ɾ��һ��columnfamily
    void RemoveColumnFamily(const std::string& cf_name);
    /// ��ȡ���е�colmnfamily
    const ColumnFamilyDescriptor* ColumnFamily(int32_t id) const;
    const ColumnFamilyDescriptor* ColumnFamily(const std::string& cf_name) const;
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
    bool IsKv() const;

private:
    typedef std::map<std::string, LGDescImpl*> LGMap;
    typedef std::map<std::string, CFDescImpl*> CFMap;
    std::string     _name;
    bool            _kv_only;
    LGMap           _lg_map;
    std::vector<LGDescImpl*> _lgs;
    CFMap           _cf_map;
    std::vector<CFDescImpl*> _cfs;
    int32_t         _next_lg_id;
    int32_t         _next_cf_id;
    std::vector<uint64_t> _snapshots;
    static const std::string DEFAULT_LG_NAME;
    static const std::string DEFAULT_CF_NAME;
    RawKeyType      _raw_key_type;
    int64_t         _split_size;
    int64_t         _merge_size;
};

} // namespace tera

#endif  // TERA_SDK_SCHEMA_IMPL_H_
