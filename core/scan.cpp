#include "scan.h"
#include <iostream>
#include <sqlite3.h>
#include "hash.h"
std::string Scan::scan(const std::string& path) {
    sqlite3* db;
    Hash hash;
    if (sqlite3_open("D:/Softwares/Proj/masha/core/malware_hashes.db", &db) != SQLITE_OK) { /*db open error*/
    return "Failed to open database";
    }
    sqlite3_stmt* stmt;
    std::string sha256  = hash.hash(path);
    const char* query = "SELECT 1 FROM malware_hashes WHERE sha256 = ""?"" LIMIT 1;";

    if(sqlite3_prepare(db,query,-1,&stmt,nullptr) !=SQLITE_OK){ /*statement preparation error*/
        std::cerr << "Failed to prepare statement" << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return "DB closed";
    }
    
    if (sqlite3_bind_text(stmt, 1, sha256.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) { /*bind error*/
        std::cerr << "Failed to bind hash\n";
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return "Binding error";
    }
    
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {  /*checking for hash if SQLITE_ROW exists it means theres some value*/
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return "Not found";
    }
    sqlite3_finalize(stmt);
    return "Malware found";

    
}
