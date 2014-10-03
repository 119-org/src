#ifndef _LIBSKT_LOG_H_
#define _LIBSKT_LOG_H_

#include <stdio.h>

/*from /usr/include/sys/syslog.h */
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */

void skt_log(int level, const char *fmt, ...);

#endif
