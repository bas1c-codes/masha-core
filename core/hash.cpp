#include "hash.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <openssl/sha.h>
#include <sstream>
std::string Hash::hash(const std::string& path) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256; // made a sha256 type variable
    SHA256_Init(&sha256); // started it
    char stream[16384]; // stream for storing raw bytes of file
    std::ifstream file(path,std::ios::binary);
    if(!file.is_open()){
        return "File not found";
    }
    
    while (file.read(stream,sizeof(stream)) || file.gcount() >0 )
    {
        SHA256_Update(&sha256,stream,file.gcount()); // updated the hash to sha256 variable
    }
    if (file.bad()) {
        // I/O error happened
        return "I/O error while reading file";
    }

    if (file.fail() && !file.eof()) {
        // Non-recoverable error other than EOF
        return "Non-EOF read error";
    }
    SHA256_Final(hash,&sha256); // finalized the hash and copied it to hash variable
    std::stringstream ss;
    for(int i=0;i<SHA256_DIGEST_LENGTH;i++){
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);   // converting raw hash to hexadecimal
    }
    return ss.str();
}
//Authorized
