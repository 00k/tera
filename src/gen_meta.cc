#include <iostream>
#include <fstream>
#include "leveldb/db/filename.h"
#include "proto/kv_helper.h"
#include "proto/tabletnode_client.h"
#include "proto/tabletnode_rpc.pb.h"
#include "sdk/sdk_utils.h"
#include "sdk/sdk_zk.h"
#include "utils/config_utils.h"
#include "utils/timer.h"

using namespace tera;

bool WriteMetaTable(const std::string& meta_addr, const std::string& key,
                    const std::string& value);

int main(int argc, char** argv) {
    if (argc < 3 || (argc > 3 && strcmp(argv[3], "dryrun") != 0)) {
        std::cout << "Usage: " << argv[0]
                  << " <schema_file> <tablet_list_file> [dryrun]" << std::endl;
        return -1;
    }

    bool dryrun = false;
    if (argc > 3) {
        dryrun = true;
    }

    utils::LoadFlagFile("tera.flag");

    ErrorCode err;
    TableDescriptor table_desc;
    if (!ParseTableSchemaFile(argv[1], &table_desc, &err)) {
        std::cout << "fail to parse table schema from file " << argv[1] << std::endl;
        return -1;
    }
    TableSchema schema;
    TableDescToSchema(table_desc, &schema);

    std::string table_name = table_desc.TableName();

    sdk::ClusterFinder* finder = sdk::NewClusterFinder();
    std::string meta_addr = finder->RootTableAddr();
    if (meta_addr.empty()) {
        std::cout << "fail to get meta table addr" << std::endl;
        return -1;
    }

    int32_t id = 1;
    std::string start, end;
    bool last = false;
    std::ifstream list_file(argv[2], std::ios_base::in);
    const int lsize = 10240;
    char line[lsize];
    list_file.getline(line, lsize);
    while (list_file.good() || !last) {
        if (list_file.good()) {
            char* space = strpbrk(line, " \t");
            assert(space == NULL);
            end = line;
            assert(end > start);
        } else {
            last = true;
        }

        if (dryrun) {
            std::cout << id << " " << start << " ~ " << end << std::endl;
        }

        if (!dryrun) {
            TabletMeta tablet_meta;
            tablet_meta.set_table_name(table_name);
            tablet_meta.set_path(leveldb::GetTabletPathFromNum(table_name, id));
            tablet_meta.mutable_key_range()->set_key_start(start);
            tablet_meta.mutable_key_range()->set_key_end(end);
            tablet_meta.set_status(kTabletDisable);
            std::string tablet_key, tablet_value;
            tera::MakeMetaTableKeyValue(tablet_meta, &tablet_key, &tablet_value);
            bool r = WriteMetaTable(meta_addr, tablet_key, tablet_value);
            assert(r);
        }

        id++;
        start = end;
        end.clear();

        if (list_file.good()) {
            list_file.getline(line, lsize);
        }
    }
    std::cout << "write " << id - 1 << " tablet records" << std::endl;

    if (!dryrun) {
        TableMeta table_meta;
        table_meta.set_table_name(table_name);
        table_meta.set_status(tera::kTableDisable);
        table_meta.mutable_schema()->CopyFrom(schema);
        table_meta.set_create_time(get_millis());
        std::string table_key, table_value;
        MakeMetaTableKeyValue(table_meta, &table_key, &table_value);
        bool r = WriteMetaTable(meta_addr, table_key, table_value);
        assert(r);
    }

    std::cout << "write table record" << std::endl;
    return 0;
}

bool WriteMetaTable(const std::string& meta_addr, const std::string& key,
                    const std::string& value) {
    tabletnode::TabletNodeClient client(meta_addr);
    WriteTabletRequest request;
    WriteTabletResponse response;
    request.set_sequence_id(0);
    request.set_tablet_name("meta_table");
    request.set_is_sync(true);
    request.set_is_instant(true);
    RowMutationSequence* mu_seq = request.add_row_list();
    mu_seq->set_row_key(key);
    Mutation* mutation = mu_seq->add_mutation_sequence();
    mutation->set_type(tera::kPut);
    mutation->set_value(value);
    if (!client.WriteTablet(&request, &response)) {
        LOG(WARNING) << "fail to write meta table: "
            << StatusCodeToString(kRPCError);
        return false;
    }
    StatusCode status = response.status();
    if (status == kTabletNodeOk) {
        status = response.row_status_list(0);
    }
    if (status != kTabletNodeOk) {
        LOG(WARNING) << "fail to write meta table: "
            << StatusCodeToString(status);
        return false;
    }
    return true;
}

