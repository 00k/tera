// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tera/sdk/read_impl.h"
#include "tera/sdk/tera.h"

namespace tera {

RowReader::RowReader() {}

RowReader::~RowReader() {}

#if 0
/// ��ȡ����
RowReader::RowReader(Table* table, const std::string& row_key) {
    _impl = new RowReaderImpl(this, table, row_key);
}

/// ���ö�ȡ�ض��汾
void RowReader::SetTimestamp(int64_t ts) {
    _impl->SetTimestamp(ts);
}

/// ���ó�ʱʱ��(ֻӰ�쵱ǰ����,��Ӱ��Table::SetReadTimeout���õ�Ĭ�϶���ʱ)
void RowReader::SetTimeOut(int64_t timeout_ms) {
    _impl->SetTimeOut(timeout_ms);
}

void RowReader::SetCallBack(Callback callback) {
    _impl->SetCallBack(callback);
}

/// �����첽����
void RowReader::SetAsync() {
    _impl->SetAsync();
}

/// �첽�����Ƿ����
bool RowReader::IsFinished() const {
    return _impl->IsFinished();
}

/// ��ý��������
ErrorCode RowReader::GetError() {
    return _impl->GetError();
}

/// �Ƿ񵽴�������
bool RowReader::Done() {
    return _impl->Done();
}

/// ������һ��cell
void RowReader::Next() {
    _impl->Next();
}

/// ��ȡ�Ľ��
std::string RowReader::Value() {
    return _impl->Value();
}

/// Timestamp
int64_t RowReader::Timestamp() {
    return _impl->Timestamp();
}

/// Row
std::string RowReader::RowName() {
    return _impl->RowName();
}

/// Column cf:qualifier
std::string RowReader::ColumnName() {
    return _impl->ColumnName();
}

/// Column family
std::string RowReader::Family() {
    return _impl->Family();
}

/// Qualifier
std::string RowReader::Qualifier() {
    return _impl->Qualifier();
}

void RowReader::ToMap(Map* rowmap) {
    _impl->ToMap(rowmap);
}
#endif

} // namespace tera

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
