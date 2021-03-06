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

// Proxy for malloc that cannot fail
void * must_malloc( long size )
{
	void *mem = malloc( size );
	
	if ( !mem && size ) {
		log_err( "Malloc Failed." );
		exit( SYSTEM  );
	}

	return mem;
}

// Proxy for realloc that cannot fail
void * must_realloc( void *ptr, long size )
{
	void * mem = realloc( ptr, size );

	if ( !mem && size ) {
		log_err( "Realloc Failed." );
		exit( SYSTEM  );
	}

	return mem;
}

// Takes a string containing some digits and parses as double
// Moves jsonString pointer to after the number parsed ready for next token
JsonError parse_number( char **jsonString, double *d )
{

	// Check number and locate end
	char *end = *jsonString;
	if ( *end == '-' ) end++; // Negative sign
	while ( isdigit( *end ) ) end++;
	if ( *end == '.' ) {
		end++;
		while ( isdigit( *end ) ) end++;
	}

	// Skip white space
	while( isspace( *end ) ) end++;

	// Check valid terminating char
	if ( *end == COMMA || *end == CLOSE_BRACKET || *end == CLOSE_BRACE ) {

		sscanf( *jsonString, "%lf", d );

		// Advance string
		*jsonString = end;

		return SUCCESS;

	}

	return MALFORMED;

}

// Takes a string with characters surrouneded by double quotes. Returns the character contents. 
// Moves jsonString pointer after the characters parsed ready for next token
JsonError parse_string( char **jsonString, char **string )
{

	// Save start, find end
	char *start = *jsonString;
	char *end = start + 1;

	// Find closing quote
	while ( ( *end != QUOTE || *(end - 1) == '\\') && *end != '\0' ) end++;

	// If null terminator found before final quote, then string is malformed
	if ( *end == '\0' ) {
		return MALFORMED;
	}

	// Allocate and copy string contents
	*string = must_malloc( sizeof( char ) * (end - start) );
	strncpy( *string, start + 1, (end - 1) - start );
	(*string)[(end-1)-start] = '\0';

	// Advance jsonString
	*jsonString = end + 1;

	return SUCCESS;

}

// Takes a string hopefully containing "true" or "false". Returns the correct boolean value.. 
// Moves jsonString pointer after the boolean parsed ready for next token
JsonError parse_boolean( char **jsonString, bool *b )
{

	// Check chars and locate end
	char *end = *jsonString;
	while( isalpha( *end ) ) end++;

	// Check valid terminating char
	if ( *end == COMMA || *end == CLOSE_BRACKET || *end == CLOSE_BRACE ) {

		JsonError ret = MALFORMED;

		// Compare to true
		if ( !strncmp( *jsonString, "true", 4 ) ) {
			*b = true;
			ret = SUCCESS;
			*jsonString = end;
		// Compare to false
		} else if ( !strncmp( *jsonString, "false", 5 ) ) {
			*b = false;
			ret = SUCCESS;
			*jsonString = end;
		}

		return ret;

	}

	return MALFORMED;

}

// Takes a string containing and array. Recursively parses each member using parse functions. 
// Moves jsonString pointer after the array parsed ready for next token
JsonError parse_array( char **jsonString, Json ***array )
{
	// initialise array in memory
	int size = 1;
	int length = 0;
	char *end = *jsonString + 1;
	(*array) = must_malloc( sizeof( Json * ) * ( size + 1 ) );

	// Process each of the array members
	for ( EVER ) {

		// Skip whitespace
		while ( isspace( *end ) ) end++;

		// Check for end of array
		if ( *end == CLOSE_BRACKET ) {
			break;
		}

		// Check enough space for new member
		if ( length == size ) {
			size += size; // Double size in memory and reallocate
			(*array) = must_realloc( (*array), sizeof( Json * ) * ( size + 1 ) );
		}

		// Allocate new member
		(*array)[length] = must_malloc( sizeof( Json ) );
		(*array)[length]->name = NULL;
		(*array)[length]->type = NULL_TYPE;
		length++;

		// Determine type of member and parse

		// BOOLEAN TYPE
		if ( *end == 't' || *end == 'f' ) { 
			if ( parse_boolean( &end, &((*array)[length-1]->boolean) ) != SUCCESS )
				goto error;
			(*array)[length-1]->type = BOOLEAN;
		}
		
		// NUMBER TYPE
		else if ( isdigit( *end ) || *end == '.' || *end == '-' ) {
			if ( parse_number( &end, &((*array)[length-1]->number) ) != SUCCESS )
				goto error;
			(*array)[length-1]->type = NUMBER;
		}
		
		// STRING TYPE
		else if ( *end == QUOTE ) {
			if ( parse_string( &end, &((*array)[length-1]->string) ) != SUCCESS )
				goto error;
			(*array)[length-1]->type = STRING;
		}
		
		// ARRAY TYPE
		else if ( *end == OPEN_BRACKET ) {
			if ( parse_array( &end, &((*array)[length-1]->array) ) != SUCCESS )
				goto error;
			(*array)[length-1]->type = ARRAY;
		}
		
		// OBJECT TYPE
		else if ( *end == OPEN_BRACE ) {
			if ( parse_object( &end, &((*array)[length-1]->object) ) != SUCCESS )
				goto error;
			(*array)[length-1]->type = OBJECT;
		}
		
		// NULL TYPE
		else if ( strncmp( end, "null", 4 ) == 0 ) {
			(*array)[length-1]->type = NULL_TYPE;
			end += 4;
		}
		
		// UNKNOWN TYPE
		else {
			goto error;
		}

		// Skip whitespace
		while ( isspace( *end ) ) end++;
		
		// Check for end of array
		if ( *end == CLOSE_BRACKET ) {
			break;
		}
		
		// Check for comma indicating next member
		else if ( *end == COMMA ) {
			end++;
			continue;
		}
		
		// Illegal character inside array
		else {
			goto error;
		}

	}

	// Null terminate array
	(*array)[length] = NULL;
	*jsonString = end + 1;

	return SUCCESS;

	error:
	
		// Could parse array for some reason. Clean up all allocated memory.

		// clean up progress
		for ( int i = 0; i < length; i++ )
			free_json_object( &((*array)[i]) );

		// Clean up array
		free( (*array) );

		return MALFORMED;

}

JsonError parse_object( char **jsonString, Json ***object )
{

	// Initialise memory for object array
	int size = 1;
	int length = 0;
	char *end = *jsonString + 1;
	(*object) = must_malloc( sizeof( Json * ) * ( size + 1 ) );

	// Parse all of the object members
	for ( EVER ) {

		// Skip whitespace
		while ( isspace( *end ) ) end++;

		// Check for end of object
		if ( *end == CLOSE_BRACE ) {
			break;
		}

		// Check enough space for new member
		if ( length == size ) {
			size += size; // Double size and reallocate if necessary
			(*object) = must_realloc( (*object), sizeof( Json * ) * ( size + 1 ) );
		}

		// Allocate new member
		(*object)[length] = must_malloc( sizeof( Json ) );
		(*object)[length]->name = NULL;
		(*object)[length]->type = NULL_TYPE;
		length++;

		// Get name (JS Objects have "name" : "value" pairs)s
		if ( *end != QUOTE || parse_string( &end, &((*object)[length-1]->name) ) != SUCCESS )
			goto error;

		// Skip whitespace
		while ( isspace( *end ) ) end++;

		// Get pair separator
		if ( *end++ != PAIR )
			goto error;

		// Skip whitespace
		while ( isspace( *end ) ) end++;

		// BOOLEAN TYPE
		if ( *end == 't' || *end == 'f' ) {
			if ( parse_boolean( &end, &((*object)[length-1]->boolean) ) != SUCCESS )
				goto error;
			(*object)[length-1]->type = BOOLEAN;
		}
		
		// NUMBER TYPE
		else if ( isdigit( *end ) || *end == '.' ) {
			if ( parse_number( &end, &((*object)[length-1]->number) ) != SUCCESS )
				goto error;
			(*object)[length-1]->type = NUMBER;
		}
		
		// STRING TYPE
		else if ( *end == QUOTE ) {
			if ( parse_string( &end, &((*object)[length-1]->string) ) != SUCCESS )
				goto error;
			(*object)[length-1]->type = STRING;
		}
		
		// ARRAY TYPE
		else if ( *end == OPEN_BRACKET ) {
			if ( parse_array( &end, &((*object)[length-1]->array) ) != SUCCESS )
				goto error;
			(*object)[length-1]->type = ARRAY;
		}
		
		// OBJECT TYPE
		else if ( *end == OPEN_BRACE ) {
			if ( parse_object( &end, &((*object)[length-1]->object) ) != SUCCESS )
				goto error;
			(*object)[length-1]->type = OBJECT;
		}
		
		// NULL TYPE
		else if ( strncmp( end, "null", 4 ) == 0 ) {
			(*object)[length-1]->type = NULL_TYPE;
			end += 4;
		}
		
		// UNKNOWN TYPE
		else {
			goto error;
		}

		// Skip whitespace
		while ( isspace( *end ) ) end++;
		
		// Check for end of object
		if ( *end == CLOSE_BRACE ) {
			break;
		}
		
		// Check for COMMA indicating next member
		else if ( *end == COMMA ) {
			end++;
			continue;
		}
		
		// Illegal character inside object
		else {
			goto error;
		}

	}

	// Null terminate object
	(*object)[length] = NULL;
	*jsonString = end + 1;

	return SUCCESS;

	error:

		// Encountered an error during parsing, free all memory

		// clean up progress
		for ( int i = 0; i < length; i++ )
			free_json_object( &(*(object)[i]) );

		// Clean up object
		free( (*object) );

		return MALFORMED;

}

JsonError parse_json_string( char *jsonString, Json **jsonObject )
{

	// Initialise root object
	*jsonObject = must_malloc( sizeof( Json ) );
	(*jsonObject)->type = OBJECT;
	(*jsonObject)->name = NULL;

	// Parse root object members
	JsonError ret = parse_object( &jsonString, &((*jsonObject)->object) );

	// Memory safety
	if ( ret != SUCCESS ) {
		free( *jsonObject );
		*jsonObject = NULL;
	}

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
	if ( json->name ) printf( "\"%s\" : ", json->name );
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
