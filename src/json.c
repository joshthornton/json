#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dbg.h"
#include "json.h"

// Structural Characters
#define QUOTE			'"'
#define COMMA			','
#define OPEN_BRACKET	'['
#define CLOSE_BRACKET	']'
#define OPEN_BRACE		'{'
#define CLOSE_BRACE		'}'
#define PAIR			':'

void * must_malloc( long size )
{
	void *mem = malloc( size );
	
	if ( !mem && size ) {
		log_err( "Malloc Failed." );
		exit( SYSTEM  );
	}

	return mem;
}

JsonError parse_number( char **jsonString, double *d )
{

	// Check number and locate end
	char *end = *jsonString;
	while ( (*end >= '0' && *end <= '9') ) end++;
	if ( *end == '.' ) {
		end++;
		while ( (*end >= '0' && *end <= '9') ) end++;
	}
	while( isspace( *end++ ) );

	char endChar = *end;
	if ( endChar == COMMA || endChar == CLOSE_BRACKET || endChar == CLOSE_BRACE ) {

		*end = '\0';
		sscanf( *jsonString, "%lf", d );
		*end = endChar;

		// Advance string
		*jsonString = ++end;

		return SUCCESS;

	}

	return MALFORMED;

}

JsonError parse_json_string( char *jsonString, Json **jsonObject )
{

	(*jsonObject) = must_malloc( sizeof( Json ) );
	(*jsonObject)->type = OBJECT;
	(*jsonObject)->object = must_malloc( sizeof( Json * ) * 2 );
	(*jsonObject)->object[0] = must_malloc( sizeof( Json ) );
	(*jsonObject)->object[0]->name  = must_malloc( sizeof( char * ) * 5 );
	strcpy( (*jsonObject)->object[0]->name, "test" );
	(*jsonObject)->object[0]->type = NUMBER;
	(*jsonObject)->object[0]->number = 1; 
	(*jsonObject)->object[1] = NULL; 

	return SUCCESS;
}

JsonError free_json_object( Json **jsonObject )
{
	int i;

	switch ( (*jsonObject)->type ) {
	case NUMBER:
		if ( (*jsonObject)->name ) {
			free( (*jsonObject)->name );
		}
		free( *jsonObject );
		break;
	case STRING:
		if ( (*jsonObject)->name ) {
			free( (*jsonObject)->name );
		}
		free( (*jsonObject)->string );
		free( *jsonObject );
		break;
	case BOOLEAN:
		if ( (*jsonObject)->name ) {
			free( (*jsonObject)->name );
		}
		free( *jsonObject );
		break;
	case ARRAY:
		if ( (*jsonObject)->name ) {
			free( (*jsonObject)->name );
		}

		// free each member
		i = 0;
		while( (*jsonObject)->array[i] )
			free_json_object( &((*jsonObject)->array[i++]) );
		
		free( (*jsonObject)->array );
		free( *jsonObject );
		break;
	case OBJECT:
		if ( (*jsonObject)->name ) {
			free( (*jsonObject)->name );
		}

		// free each member
		i = 0;
		while( (*jsonObject)->object[i] )
			free_json_object( &((*jsonObject)->object[i++]) );
		
		free( (*jsonObject)->object );
		free( *jsonObject );
		break;
	default:
		if ( (*jsonObject)->name ) {
			free( (*jsonObject)->name );
		}
		free( *jsonObject );
		break;
		break;
	}

	*jsonObject = NULL;

	return SUCCESS;
}
