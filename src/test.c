#include <stdio.h>
#include <stdlib.h>

#include "json.h"

int main( int argc, char *argv[] )
{

	Json *json;

	if ( parse_json_string( "{ \"test\" : 1 }", &json ) ) {
		fprintf( stderr, "Error\n" );
		exit( -1 );
	}

	printf( "{ %s : %lf }\n", json->object[0]->name, json->object[0]->number );

	if ( free_json_object( &json ) ) {
		fprintf( stderr, "Error\n" );
		exit( -1 );
	}

	return EXIT_SUCCESS;
}
