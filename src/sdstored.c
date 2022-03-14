#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util/sv.h"

void usage(int argc, char** argv){
    (void) argc;

    char buf[1024];
    snprintf(buf, sizeof(buf), "USAGE: %s [conf_file] [bin_path]\n", argv[0]);
    sv_write(sv_from_cstr(buf), STDOUT_FILENO);
}

int main(int argc, char** argv){
    if(argc != 3){
        usage(argc, argv);
        exit(-1);
    }
    SV conf = sv_slurp_file(argv[1]);
    
    sv_write(conf, STDOUT_FILENO);
    return 0;
}