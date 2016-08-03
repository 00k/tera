// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "sdk/tera.h"
#include "lite/lite.h"

int main() {
    tera::DB::Init("tera.flag");

    tera::TableDescriptor desc("test_lite");
    desc.AddLocalityGroup("lg0");
    desc.AddLocalityGroup("lg1");
    desc.AddColumnFamily("cf0", "lg0");
    desc.AddColumnFamily("cf1", "lg1");

    tera::DB* db = NULL;
    if (!tera::DB::NewDB(desc, &db)) {
        std::cout << "open table fail" << std::endl;
        return -1;
    }
    std::cout << "open table success" << std::endl;

    tera::RowMutation* mutation = db->NewRowMutation("row1");
    mutation->Put("cf0", "cq0", "value0");
    mutation->Put("cf1", "cq1", "value1");
    db->Put(mutation);
    tera::ErrorCode err = mutation->GetError();
    if (err.GetType() == tera::ErrorCode::kOK) {
        std::cout << "put success" << std::endl;
    } else {
        std::cout << "put fail: " << err.GetReason() << std::endl;
    }
    delete mutation;
    mutation = db->NewRowMutation("row2");
    mutation->Put("cf0", "cq2", "value2");
    mutation->Put("cf1", "cq3", "value3");
    db->Put(mutation);
    err = mutation->GetError();
    if (err.GetType() == tera::ErrorCode::kOK) {
        std::cout << "put success" << std::endl;
    } else {
        std::cout << "put fail: " << err.GetReason() << std::endl;
    }
    delete mutation;

    tera::RowReader* reader = db->NewRowReader("row1");
    db->Get(reader);
    err = reader->GetError();
    if (err.GetType() == tera::ErrorCode::kOK) {
        std::cout << "get success" << std::endl;
        while (!reader->Done()) {
            std::cout << "  " << reader->Family() << ":" << reader->Qualifier()
                      << "  " << reader->Value() << std::endl;
            reader->Next();
        }
    } else {
        std::cout << "get fail: " << err.GetReason() << std::endl;
    }
    delete reader;

    tera::ResultStream* result = db->Scan(&err);
    while (!result->Done()) {
        std::cout << result->RowName() << "  "
                  << result->Family() << ":" << result->Qualifier()
                  << "  " << result->Value() << std::endl;
        result->Next();
    }
    delete result;

    delete db;
    return 0;
}
