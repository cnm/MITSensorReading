/*
 * spotter.h
 *
 *  Created on: Jun 10, 2011
 *      Author: novais
 */

#ifndef SPOTTER_H_
#define SPOTTER_H_

#define ENABLE_DEBUG 1

/*
#define SPOTTER_LOG_FILE "spotterlog"

#define logger(...) openlog(SPOTTER_LOG_FILE, LOG_CONS, LOG_DAEMON); syslog(LOG_INFO, __VA_ARGS__); closelog();

#ifdef ENABLE_DEBUG
	#define debugger(...) printf(__VA_ARGS__);
	//#define debugger(...) openlog(DEBUG_FILE, LOG_PID|LOG_CONS, LOG_USER); syslog(LOG_INFO, __VA_ARGS__); closelog();
#else
	#define debugger(...)
#endif
*/

typedef enum plugin_type{SYNC, ASYNC} PluginType;

typedef struct plugin{
	char * location;
	PluginType type;
}Plugin;



#endif /* SPOTTER_H_ */
