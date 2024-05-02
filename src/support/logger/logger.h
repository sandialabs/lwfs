/*-------------------------------------------------------------------------*/
/**  @file logger.h
 *   
 *   @brief Method prototypes for the logger API.
 *   
 *   The logger API is a simple API for logging events to a file 
 *   (or to stderr or stdio) 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 1014 $.
 *   $Date: 2006-10-09 15:59:10 -0600 (Mon, 09 Oct 2006) $.
 *
 */
 
#include <stdio.h>
#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdio.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FALSE 
#	define FALSE (0)
#endif 

#ifndef TRUE 
#	define TRUE (1)
#endif 

/**
 * @brief Boolean function that returns TRUE if we are logging 
 *        error statements.
 */
#define logging_fatal(level) \
	(((level == LOG_UNDEFINED) && (default_log_level >= LOG_FATAL)) \
	|| ((level != LOG_UNDEFINED) && (level >= LOG_FATAL)))

/**
 * @brief Inline function that outputs a FATAL 
 * message to the log file.
 *
 * @param level   The log level to use.
 * @param args    A formatted message (like printf).
 */
#define log_fatal(level, args...) if (logging_fatal(level)) \
		log_output("FATAL",__FUNCTION__,__FILE__,__LINE__, ## args)

/**
 * @brief Boolean function that returns TRUE if we are logging 
 *        error statements.
 */
#define logging_error(level) \
	(((level == LOG_UNDEFINED) && (default_log_level >= LOG_ERROR)) \
	|| ((level != LOG_UNDEFINED) && (level >= LOG_ERROR)))

/**
 * @brief Inline function that outputs an ERROR 
 * message to the log file.
 *
 * @param level   The log level to use.
 * @param args    A formatted message (like printf).
 */
#define log_error(level, args...) if (logging_error(level)) \
		log_output("ERROR",__FUNCTION__,__FILE__,__LINE__, ## args)

/**
 * @brief Boolean function that returns TRUE if we are logging 
 *        warning statements.
 */
#define logging_warn(level) \
	(((level == LOG_UNDEFINED) && (default_log_level >= LOG_WARN)) \
	|| ((level != LOG_UNDEFINED) && (level >= LOG_WARN)))

/**
 * @brief Inline function that outputs a WARN
 * message to the log file.
 *
 * @param level   The log level to use.
 * @param args    A formatted message (like printf).
 */
#define log_warn(level, args...) if (logging_warn(level)) \
		log_output("WARN",__FUNCTION__,__FILE__,__LINE__, ## args)

/**
 * @brief Boolean function that returns TRUE if we are logging 
 *        info statements.
 */
#define logging_info(level) \
	(((level == LOG_UNDEFINED) && (default_log_level >= LOG_INFO)) \
	|| ((level != LOG_UNDEFINED) && (level >= LOG_INFO)))

/**
 * @brief Inline function that outputs an INFO 
 * message to the log file.
 *
 * @param level   The log level to use.
 * @param args    A formatted message (like printf).
 */
#define log_info(level, args...) if (logging_info(level)) \
		log_output("INFO",__FUNCTION__,__FILE__,__LINE__, ## args)

/**
 * @brief Boolean function that returns TRUE if we are logging 
 *        debug statements.
 */
#define logging_debug(level) \
	(((level == LOG_UNDEFINED) && (default_log_level >= LOG_DEBUG)) \
	|| ((level != LOG_UNDEFINED) && (level >= LOG_DEBUG)))
		 
/**
 * @brief Inline function that outputs a DEBUG 
 * message to the log file.
 *
 * @param level   The log level to use.
 * @param args    A formatted message (like printf).
 */
#define log_debug(level, args...) if (logging_debug(level)) \
		log_output("DEBUG",__FUNCTION__,__FILE__,__LINE__, ## args)

enum log_level {
	LOG_UNDEFINED = -1,
	LOG_OFF = 0,
	LOG_FATAL = 1,
	LOG_ERROR = 2,
	LOG_WARN = 3,
	LOG_INFO = 4,
	LOG_DEBUG = 5,
	LOG_ALL = 6,
};
typedef enum log_level log_level;

extern log_level default_log_level; 

extern pthread_mutex_t logger_mutex;

/* the functions */

#if defined(__STDC__) || defined(__cplusplus)


extern int logger_init(const log_level debug_level, const char *file);
extern int logger_not_initialized(void);
extern void logger_set_file(FILE *); 
extern FILE *logger_get_file(void); 
extern void logger_set_default_level(const log_level); 
extern log_level logger_get_default_level(void); 
extern void logger_mutex_lock(void); 
extern void logger_mutex_unlock(void); 

void log_output(const char *prefix, 
		const char *func_name, 
		const char *file_name, 
		const int line_no, 
		const char *msg, ...); 

#endif


#ifdef __cplusplus
}
#endif

#endif /* !LOGGER_H_ */
