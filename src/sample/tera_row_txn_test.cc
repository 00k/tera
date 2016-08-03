// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tera.h"
#include "utils/counter.h"

tera::Counter counter;
tera::Counter get_error_counter;
tera::Counter put_error_counter;
tera::Counter commit_error_counter;

std::string table_name = "test_row_txn";
tera::Client* client = NULL;
tera::Table* table = NULL;

/// 创建一个表格
int CreateTable() {
    // 创建一个表格的描述
    tera::TableDescriptor table_desc(table_name);
    table_desc.EnableTxn();

    // 创建LocalityGroup
    tera::LocalityGroupDescriptor* lgd0 = table_desc.AddLocalityGroup("lg0");
    lgd0->SetStore(tera::kInFlash);

    tera::LocalityGroupDescriptor* lgd1 = table_desc.AddLocalityGroup("lg1");
    lgd1->SetStore(tera::kInFlash);

    // 创建ColumnFamily
    tera::ColumnFamilyDescriptor* cfd1 = table_desc.AddColumnFamily("sum1", "lg0");
    cfd1->SetMaxVersions(5);
    tera::ColumnFamilyDescriptor* cfd2 = table_desc.AddColumnFamily("sum2", "lg1");
    cfd2->SetMaxVersions(5);
    tera::ColumnFamilyDescriptor* cfd3 = table_desc.AddColumnFamily("sum3", "lg1");
    cfd3->SetMaxVersions(5);

    tera::ErrorCode error_code;
    if (!client->CreateTable(table_desc, &error_code)) {
        printf("Create Table fail: %s\n", tera::strerr(error_code));
    }
    return 0;
}

void Start(tera::Transaction* txn, const std::string& key);
void GetCallback1(tera::RowReader*);
void GetCallback2(tera::RowReader*);
void PutCallback1(tera::RowMutation*);
void PutCallback2(tera::RowMutation*);
void CommitCallback(tera::Transaction*);

struct TxnContext {
    uint64_t id;
    std::string key;
};

void StartTxn(const std::string& key, uint64_t id) {
    tera::Transaction* txn = table->StartRowTransaction(key);
    txn->SetCommitCallback(CommitCallback);
    TxnContext* txn_context = new TxnContext;
    txn_context->id = id;
    txn_context->key = key;
    txn->SetContext(txn_context);
    Start(txn, key);
}

/// 修改一个表的内容
void Add(uint64_t sum) {
    tera::ErrorCode error_code;

    std::stringstream ss;
    ss << sum;
    std::string key = ss.str();

    for (uint64_t i = 0; i < sum; i++) {
        counter.Inc();
        StartTxn(key, i);
    }

    while (counter.Get() > 0) {
        std::cout << "wait for txn ..." << std::endl;
        usleep(1000000);
    }
    std::cout << "total: " << sum << " get fail: " << get_error_counter.Get()
              << " put fail: " << put_error_counter.Get()
              << " commit fail: " << commit_error_counter.Get()
              << std::endl;
}

void Start(tera::Transaction* txn, const std::string& key) {
    tera::RowReader* reader = table->NewRowReader(key);
    reader->AddColumnFamily("sum1");
    reader->SetCallBack(GetCallback1);
    reader->SetContext(txn);
    txn->Get(reader);
}

void GetCallback1(tera::RowReader* reader) {
    tera::Transaction* txn = reader->GetTransaction();
    TxnContext* ctx = (TxnContext*)txn->GetContext();
    uint64_t id = ctx->id;

    if (reader->GetError().GetType() != tera::ErrorCode::kOK
        && reader->GetError().GetType() != tera::ErrorCode::kNotFound) {
        std::cout << "txn " << id << " get1: " << reader->GetError().ToString() << std::endl;
        delete reader;
        delete ctx;
        delete txn;
        get_error_counter.Inc();
        counter.Dec();
        return;
    }
    std::string key = reader->RowName();
    uint64_t sum1 = 0;
    if (reader->GetError().GetType() == tera::ErrorCode::kOK) {
        sum1 = *(uint64_t*)reader->Value().data();
    }
    delete reader;
    std::cout << "txn " << id << " get1: " << sum1 << " put: " << sum1 + 1 << std::endl;

    sum1 += 1;
    char new_value_buf[8];
    memcpy(new_value_buf, &sum1, 8);
    std::string new_value(new_value_buf, 8);
    tera::RowMutation* mutation = table->NewRowMutation(key);
    mutation->Put("sum1", "", new_value);
    mutation->SetCallBack(PutCallback1);
    mutation->SetContext(txn);
    txn->ApplyMutation(mutation);
}

void PutCallback1(tera::RowMutation* mutation) {
    tera::Transaction* txn = mutation->GetTransaction();
    TxnContext* ctx = (TxnContext*)txn->GetContext();
    uint64_t id = ctx->id;
    std::cout << "txn " << id << " put1: " << mutation->GetError().ToString() << std::endl;

    if (mutation->GetError().GetType() != tera::ErrorCode::kOK) {
        delete mutation;
        delete ctx;
        delete txn;
        put_error_counter.Inc();
        counter.Dec();
        return;
    }
    delete mutation;

    sleep(id);
    table->CommitRowTransaction(txn);
}

void CommitCallback(tera::Transaction* txn) {
    TxnContext* txn_context = (TxnContext*)txn->GetContext();
    uint64_t id = txn_context->id;
    std::string key = txn_context->key;
    std::cout << "txn " << id << " commit: " << txn->GetError().ToString() << std::endl;

    tera::ErrorCode ec = txn->GetError();
    delete txn_context;
    delete txn;

    if (ec.GetType() != tera::ErrorCode::kOK) {
        commit_error_counter.Inc();
        StartTxn(key, id);
    } else {
        counter.Dec();
    }
}

void ShowResult(uint64_t sum) {
    std::stringstream ss;
    ss << sum;
    std::string key = ss.str();

    tera::RowReader* reader = table->NewRowReader(key);
    table->Get(reader);
    if (reader->GetError().GetType() != tera::ErrorCode::kOK) {
        std::cout << "fail to get result: " << reader->GetError().ToString() << std::endl;
        delete reader;
        return;
    }

    std::cout << "result of " << key << " : " << std::endl;
    while (!reader->Done()) {
        uint64_t sum = *(uint64_t*)reader->Value().data();
        std::cout << "      " << reader->ColumnName()
                  << " " << reader->Timestamp()
                  << " " << sum
                  << std::endl;
        reader->Next();
    }
    delete reader;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " num" << std::endl;
        return 1;
    }

    char* endptr = NULL;
    uint64_t sum = strtoll(argv[1], &endptr, 10);
    if (sum == 0 || *endptr != '\0') {
        std::cout << "num " << argv[1] << " is invalid" << std::endl;
        return 1;
    }

    tera::ErrorCode error_code;
    // 根据配置创建一个client
    client = tera::Client::NewClient("./tera.flag", "test_row_txn", &error_code);
    if (client == NULL) {
        std::cout << "create tera client fail: " << error_code.ToString() << std::endl;
        return 1;
    }

    CreateTable();

    table = client->OpenTable(table_name, &error_code);
    if (table == NULL) {
        std::cout << "open table fail: " << error_code.ToString() << std::endl;
        return 1;
    }

    Add(sum);
    ShowResult(sum);

    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
