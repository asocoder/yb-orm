#include <cstring>
#include <cstdlib>
#include <iostream>
#include "orm/schema.h"
#include "orm/schema_config.h"
#include "orm/code_gen.h"

using namespace std;
using namespace Yb;

#define ORM_LOG(x) cerr << "yborm_gen: " << x << "\n";

void usage()
{
    cerr << "Usage:\n"
        << "    yborm_gen --domain config.xml output_path [include_prefix]\n"
        << "    yborm_gen --ddl config.xml dialect_name [output.sql]\n\n";
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc < 4 || argc > 5)
        usage();
    try {
        bool gen_domain = true;
        string config, output_path, include_prefix, dialect_name;
        config = argv[2];
        if (!strcmp(argv[1], "--ddl")) {
            gen_domain = false;
            dialect_name = argv[3];
            if (argc == 5)
                output_path = argv[4];
        }
        else {
            output_path = argv[3];
            if (argc == 5)
                include_prefix = argv[4];
            else
                include_prefix = "domain/";
        }
        Schema r;
        load_schema(WIDEN(config), r);
        ORM_LOG("table count: " << r.tbl_count());
        ORM_LOG("generation started...");
        if (gen_domain)
            generate_domain(r, output_path, include_prefix);
        else
            generate_ddl(r, output_path, dialect_name);
        ORM_LOG("generation successfully finished");
    }
    catch (exception &e) {
        string error = string("Exception: ") + e.what();
        ORM_LOG(error);
        return 1;
    }
    catch (...) {
        ORM_LOG("Unknown exception");
        return 1;
    }
    return 0;
}
// vim:ts=4:sts=4:sw=4:et:
