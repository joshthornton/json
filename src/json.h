#ifndef _JSON_H_
#define _JSON_H_

#include <stdbool.h>

typedef enum {
	SUCCESS		=	0,
	SYSTEM		=	1,
	MALFORMED	=	2
} JsonError;

typedef enum {
	/* NULL	=	0,*/
	NUMBER	=	1,
	STRING 	=	2,
	BOOLEAN	=	4,
	ARRAY		=	8,
	OBJECT	=	16
} JsonType;

typedef struct Json_t {
	JsonType type;
	char *name;
	union {
		double			number;
		char				*string;
		bool				boolean;
		struct Json_t	**array;
		struct Json_t	**object;
	};
} Json;

/*
 * Takes a well formed jsonString and a pointer to a jsonObject.
 * Parses the json object and allocates jsonObject.
 * Returns 0 on success, error otherwise.
 */
JsonError parse_json_string( char *jsonString, Json **jsonObject );

/*
 * Correctly traverses a JSON object and frees all memory.
 */
JsonError free_json_object( Json **jsonObject );

#endif
