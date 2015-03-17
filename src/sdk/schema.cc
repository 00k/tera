// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sdk/schema_impl.h"
#include "sdk/tera.h"

namespace tera {

TableDescriptor::TableDescriptor(const std::string& tb_name, bool is_kv) {
    _impl = new TableDescImpl(tb_name, is_kv);
}
TableDescriptor::~TableDescriptor() {
    delete _impl;
    _impl = NULL;
}

/*
TableDescriptor::TableDescriptor(const TableDescriptor& desc) {
    _impl = new TableDescImpl(desc._impl->TableName());
    *_impl = *desc._impl;
}

TableDescriptor& TableDescriptor::operator=(const TableDescriptor& desc) {
    delete _impl;
    _impl = new TableDescImpl(desc._impl->TableName());
    *_impl = *desc._impl;
    return *this;
}
*/

std::string TableDescriptor::TableName() const {
    return _impl->TableName();
}

/// ����һ��localitygroup, ���ֽ�����ʹ����ĸ�����ֺ��»��߹���,���Ȳ�����256
LocalityGroupDescriptor* TableDescriptor::AddLocalityGroup(const std::string& lg_name) {
    return _impl->AddLocalityGroup(lg_name);
}
LocalityGroupDescriptor* TableDescriptor::DefaultLocalityGroup() {
    return _impl->DefaultLocalityGroup();
}
/// ɾ��һ��localitygroup,
bool TableDescriptor::RemoveLocalityGroup(const std::string& lg_name) {
    return _impl->RemoveLocalityGroup(lg_name);
}
/// ��ȡlocalitygroup
const LocalityGroupDescriptor* TableDescriptor::LocalityGroup(int32_t id) const {
    return _impl->LocalityGroup(id);
}
const LocalityGroupDescriptor* TableDescriptor::LocalityGroup(const std::string& name) const {
    return _impl->LocalityGroup(name);
}

/// LG����
int32_t TableDescriptor::LocalityGroupNum() const {
    return _impl->LocalityGroupNum();
}

/// ����һ��columnfamily, ���ֽ�����ʹ����ĸ�����ֺ��»��߹���,���Ȳ�����256
ColumnFamilyDescriptor* TableDescriptor::AddColumnFamily(const std::string& cf_name,
            const std::string& lg_name) {
    return _impl->AddColumnFamily(cf_name, lg_name);
}
ColumnFamilyDescriptor* TableDescriptor::DefaultColumnFamily() {
    return _impl->DefaultColumnFamily();
}
/// ɾ��һ��columnfamily
void TableDescriptor::RemoveColumnFamily(const std::string& cf_name) {
    return _impl->RemoveColumnFamily(cf_name);
}
/// ��ȡ���е�colmnfamily
const ColumnFamilyDescriptor* TableDescriptor::ColumnFamily(int32_t id) const {
    return _impl->ColumnFamily(id);
}

const ColumnFamilyDescriptor* TableDescriptor::ColumnFamily(const std::string& cf_name) const {
    return _impl->ColumnFamily(cf_name);
}

/// CF����
int32_t TableDescriptor::ColumnFamilyNum() const {
    return _impl->ColumnFamilyNum();
}

void TableDescriptor::SetRawKey(RawKeyType type) {
    _impl->SetRawKey(type);
}

RawKeyType TableDescriptor::RawKey() const {
    return _impl->RawKey();
}

void TableDescriptor::SetSplitSize(int64_t size) {
    _impl->SetSplitSize(size);
}

int64_t TableDescriptor::SplitSize() const {
    return _impl->SplitSize();
}

void TableDescriptor::SetMergeSize(int64_t size) {
    _impl->SetMergeSize(size);
}

int64_t TableDescriptor::MergeSize() const {
    return _impl->MergeSize();
}

int32_t TableDescriptor::AddSnapshot(uint64_t snapshot) {
    return _impl->AddSnapshot(snapshot);
}

uint64_t TableDescriptor::Snapshot(int32_t id) const {
    return _impl->Snapshot(id);
}
/// Snapshot����
int32_t TableDescriptor::SnapshotNum() const {
    return _impl->SnapshotNum();
}
/// �Ƿ�Ϊkv��
void TableDescriptor::SetKvOnly() {
    _impl->SetKvOnly();
}

bool TableDescriptor::IsKv() const {
    return _impl->IsKv();
}

} // namespace tera

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
