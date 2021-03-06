#ifndef _DBG_H_
#define _DBG_H_

/* http://c.learncodethehardway.org/book/ex20.html */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define clean_errno() ( errno == 0 ? "None" : strerror( errno ) )

#define log_err( M, ... ) fprintf( stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__ ) 

#define log_warn( M, ... ) fprintf( stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__ )

#define log_info( M, ... ) fprintf( stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__ )

#endif
