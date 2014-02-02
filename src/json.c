#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dbg.h"
#include "json.h"

// Structural Characters
#define QUOTE			'"'
#define COMMA			','
#define OPEN_BRACKET		'['
#define CLOSE_BRACKET		']'
#define OPEN_BRACE		'{'
#define CLOSE_BRACE		'}'
#define PAIR			':'

// MACROS
#define EVER ;;

// Function prototypes
JsonError parse_object( char **jsonString, Json ***object );

void * must_malloc( long size )
{
	void *mem = malloc( size );
	
	if ( !mem && size ) {
		log_err( "Malloc Failed." );
		exit( SYSTEM  );
	}

	return mem;
}

void * must_realloc( void *ptr, long size )
{
	void * mem = realloc( ptr, size );

	if ( !mem && size ) {
		log_err( "Realloc Failed." );
		exit( SYSTEM  );
	}

	return mem;
}

JsonError parse_number( char **jsonString, double *d )
{

	// Check number and locate end
	char *end = *jsonString;
	while ( isdigit( *end ) ) end++;
	if ( *end == '.' ) {
		end++;
		while ( isdigit( *end ) ) end++;
	}
	while( isspace( *end ) ) end++;

	char endChar = *end;
	if ( endChar == COMMA || endChar == CLOSE_BRACKET || endChar == CLOSE_BRACE ) {

		*end = '\0';
		sscanf( *jsonString, "%lf", d );
		*end = endChar;

		// Advance string
		*jsonString = end;

		return SUCCESS;

	}

	return MALFORMED;

}

JsonError parse_string( char **jsonString, char **string )
{

	char *start = *jsonString;
	char *end = start + 1;

	// Find closing quote
	while ( ( *end != QUOTE || *(end - 1) == '\\') && *end != '\0' ) end++;

	if ( *end == '\0' ) {
		return MALFORMED;
	}

	*string = must_malloc( sizeof( char ) * (end - start) );
	strncpy( *string, start + 1, (end - 1) - start );
	(*string)[(end-1)-start] = '\0';

	*jsonString = end + 1;

	return SUCCESS;

}

JsonError parse_boolean( char **jsonString, bool *b )
{

	// Check chars and locate end
	char *end = *jsonString;
	while( isalpha( *end ) ) end++;

	char endChar = *end;
	if ( endChar == COMMA || endChar == CLOSE_BRACKET || endChar == CLOSE_BRACE ) {

		JsonError ret = MALFORMED;

		*end = '\0';

		if ( !strcmp( *jsonString, "true" ) ) {
			*b = true;
			ret = SUCCESS;
			*jsonString = end;
		} else if ( !strcmp( *jsonString, "false" ) ) {
			*b = false;
			ret = SUCCESS;
			*jsonString = end;
		}
		*end = endChar;

		return ret;

	}

	return MALFORMED;

}

JsonError parse_array( char **jsonString, Json ***array )
{
	int size = 1;
	int length = 0;
	char *end = *jsonString + 1;
	(*array) = must_malloc( sizeof( Json * ) * ( size + 1 ) );

	for ( EVER ) {

		// Skip whitespace
		while ( isspace( *end ) ) end++;

		// Check end of (*array)
		if ( *end == CLOSE_BRACKET ) {
			break;
		}

		// Check enough space for new member
		if ( length == size ) {
			size += size;
			(*array) = must_realloc( (*array), sizeof( Json * ) * ( size + 1 ) );
		}

		// Allocate new member
		(*array)[length] = must_malloc( sizeof( Json ) );
		(*array)[length]->name = NULL;
		(*array)[length]->type = NULL_TYPE;
		length++;

		if ( *end == 't' || *end == 'f' ) { // Boolean
			if ( parse_boolean( &end, &((*array)[length-1]->boolean) ) != SUCCESS )
				goto error;
			(*array)[length-1]->type = BOOLEAN;
		} else if ( isdigit( *end ) || *end == '.' ) { // number 
			if ( parse_number( &end, &((*array)[length-1]->number) ) != SUCCESS )
				goto error;
			(*array)[length-1]->type = NUMBER;
		} else if ( *end == QUOTE ) { // string
			if ( parse_string( &end, &((*array)[length-1]->string) ) != SUCCESS )
				goto error;
			(*array)[length-1]->type = STRING;
		} else if ( *end == OPEN_BRACKET ) { // array
			if ( parse_array( &end, &((*array)[length-1]->array) ) != SUCCESS )
				goto error;
			(*array)[length-1]->type = ARRAY;
		} else if ( *end == OPEN_BRACE ) { // object
			if ( parse_object( &end, &((*array)[length-1]->object) ) != SUCCESS )
				goto error;
			(*array)[length-1]->type = OBJECT;
		} else if ( strncmp( end, "null", 4 ) == 0 ) { // null 
			(*array)[length-1]->type = NULL_TYPE;
			end += 4;
		} else {
			goto error;
		}

		// Skip whitespace
		while ( isspace( *end ) ) end++;
		
		// Check end of array
		if ( *end == CLOSE_BRACKET ) {
			break;
		} else if ( *end == COMMA ) {
			end++;
			continue;
		} else {
			goto error;
		}

	}

	// Null terminate array
	(*array)[length] = NULL;
	*jsonString = end + 1;

	return SUCCESS;

	error:

		fprintf( stderr, "Encountered error at: %s\n", end );

		// clean up progress
		for ( int i = 0; i < length; i++ )
			free_json_object( &((*array)[i]) );

		// Clean up array
		free( (*array) );

		return MALFORMED;

}

JsonError parse_object( char **jsonString, Json ***object )
{
	int size = 1;
	int length = 0;
	char *end = *jsonString + 1;
	(*object) = must_malloc( sizeof( Json * ) * ( size + 1 ) );

	for ( EVER ) {

		// Skip whitespace
		while ( isspace( *end ) ) end++;

		// Check end of object
		if ( *end == CLOSE_BRACE ) {
			break;
		}

		// Check enough space for new member
		if ( length == size ) {
			size += size;
			(*object) = must_realloc( (*object), sizeof( Json * ) * ( size + 1 ) );
		}

		// Allocate new member
		(*object)[length] = must_malloc( sizeof( Json ) );
		(*object)[length]->name = NULL;
		(*object)[length]->type = NULL_TYPE;
		length++;

		// Get name
		if ( *end != QUOTE || parse_string( &end, &((*object)[length-1]->name) ) != SUCCESS )
			goto error;

		// Skip whitespace
		while ( isspace( *end ) ) end++;

		// Get pair separator
		if ( *end++ != PAIR )
			goto error;

		// Skip whitespace
		while ( isspace( *end ) ) end++;

		if ( *end == 't' || *end == 'f' ) { // Boolean
			if ( parse_boolean( &end, &((*object)[length-1]->boolean) ) != SUCCESS )
				goto error;
			(*object)[length-1]->type = BOOLEAN;
		} else if ( isdigit( *end ) || *end == '.' ) { // number 
			if ( parse_number( &end, &((*object)[length-1]->number) ) != SUCCESS )
				goto error;
			(*object)[length-1]->type = NUMBER;
		} else if ( *end == QUOTE ) { // string
			if ( parse_string( &end, &((*object)[length-1]->string) ) != SUCCESS )
				goto error;
			(*object)[length-1]->type = STRING;
		} else if ( *end == OPEN_BRACKET ) { // array
			if ( parse_array( &end, &((*object)[length-1]->array) ) != SUCCESS )
				goto error;
			(*object)[length-1]->type = ARRAY;
		} else if ( *end == OPEN_BRACE ) { // object
			if ( parse_object( &end, &((*object)[length-1]->object) ) != SUCCESS )
				goto error;
			(*object)[length-1]->type = OBJECT;
		} else if ( strncmp( end, "null", 4 ) == 0 ) { // null 
			(*object)[length-1]->type = NULL_TYPE;
			end += 4;
		} else {
			goto error;
		}

		// Skip whitespace
		while ( isspace( *end ) ) end++;
		
		// Check end of object
		if ( *end == CLOSE_BRACE ) {
			break;
		} else if ( *end == COMMA ) {
			end++;
			continue;
		} else {
			goto error;
		}

	}

	// Null terminate object
	(*object)[length] = NULL;
	*jsonString = end + 1;

	return SUCCESS;

	error:

		fprintf( stderr, "Encountered error at: %s\n", end );

		// clean up progress
		for ( int i = 0; i < length; i++ )
			free_json_object( &(*(object)[i]) );

		// Clean up object
		free( (*object) );

		return MALFORMED;

}

JsonError parse_json_string( char *jsonString, Json **jsonObject )
{

	*jsonObject = must_malloc( sizeof( Json ) );
	(*jsonObject)->type = OBJECT;
	(*jsonObject)->name = NULL;

	char *cpy = must_malloc( sizeof( char ) * (strlen(jsonString) + 1 ) );
	strcpy( cpy, jsonString );
	char *cpycpy = cpy;
	JsonError ret = parse_object( &cpy, &((*jsonObject)->object) );
	free( cpycpy );

	return ret;

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
	}

	*jsonObject = NULL;

	return SUCCESS;
}

JsonError pretty_print_json( Json *json, int depth )
{
	int i;
	bool first;
	for( i = 0; i < depth; i++ ) printf( "\t" );
	if ( json->name ) printf( "%s : ", json->name );
	switch ( json->type ) {
	case NUMBER:
		printf( "%lf", json->number );
		break;
	case STRING:
		printf( "\"%s\"", json->string );
		break;
	case BOOLEAN:
		printf( "%s", json->boolean ? "true" : "false" );
		break;
	case ARRAY:
		printf( "[\n" );
		i = 0;
		first = true;
		while ( json->array[i] ) {
			if ( !first ) {
				printf( ",\n" );
			} else {
				first = false;
			}
			pretty_print_json( json->array[i], depth + 1 );
			i++;
		}
		printf( "\n" );
		for( i = 0; i < depth; i++ ) printf( "\t" );
		printf( "]" );
		break;
	case OBJECT:
		printf( "{\n" );
		i = 0;
		first = true;
		while ( json->object[i] ) {
			if ( !first ) {
				printf( ",\n" );
			} else {
				first = false;
			}
			pretty_print_json( json->array[i], depth + 1 );
			i++;
		}
		printf( "\n" );
		for( i = 0; i < depth; i++ ) printf( "\t" );
		printf( "}" );
		break;
	default:
		printf( "null" );
		break;
	}

	if ( depth == 0 ) {
		printf( "\n" );
	}

	return SUCCESS;
}
