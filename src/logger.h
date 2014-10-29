#ifndef LOGGER_H_
#define LOGGER_H_

/* Solution thanks to:
 * http://stackoverflow.com/a/23446001/552214
 */

/** Logs an error message
 */
void log_error(const char* message, ...);

/** Logs an informational message
 */
void log_info(const char* message, ...);

/** Logs a debugging message
 */
void log_debug(const char* message, ...);

#endif