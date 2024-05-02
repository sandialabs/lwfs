
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "support/ezxml/ezxml.h"
#include "support/logger/logger.h"

#include "config_parser.h"



int
main(int argc, char **argv) {

        char *docname;
	struct lwfs_config config; 

	memset(&config, 0, sizeof(struct lwfs_config));
                
        if (argc <= 1) {
                printf("Usage: %s docname\n", argv[0]);
                return(0);
        }

        docname = argv[1];
        parse_lwfs_config_file(docname, &config);

        return (1);
}

