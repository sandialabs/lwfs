
#include <stdlib.h>
#include "logger.h"
#include "cmdline.h"
#include "logger_opts.h"

int main(int argc, char *argv[]) {

	int level=LOG_ALL; 
	struct gengetopt_args_info args_info; 
	
	load_default_logger_opts(&args_info);

	if (cmdline_parser(argc, argv, &args_info) != 0)
		exit(1);

	print_logger_opts(stdout, &args_info, "");
	
	logger_init(args_info.verbose_arg, args_info.logfile_arg);

	log_debug(level, "print number=%d", 10);
	log_info(level, "print number=%d", 11);
	log_warn(level, "warn msg");
	log_error(level, "error msg");
	log_fatal(level, "fatal msg");

	return 0;
}
