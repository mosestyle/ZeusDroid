// Test driver: converts an .eng file with the native converter.
#include "platform/eengconverter.h"
#include <cstdio>
int main(int argc, char** argv) {
    if(argc != 3) { std::fprintf(stderr, "usage: %s in.eng out.xml\n", argv[0]); return 1; }
    std::string err;
    if(!eEngConverter::convertFile(argv[1], argv[2], err)) {
        std::fprintf(stderr, "error: %s\n", err.c_str());
        return 2;
    }
    return 0;
}
