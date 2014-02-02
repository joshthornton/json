#include <stdio.h>
#include <stdlib.h>

#include "json.h"

int main( int argc, char *argv[] )
{

	Json *json;

	parse_json_string( "{ \"test\" : 1 }", &json );
	pretty_print_json( json, 0 );
	free_json_object( &json );

	parse_json_string( "{ \"test\" : { \"test\" : 1 } }", &json );
	pretty_print_json( json, 0 );
	free_json_object( &json );

	parse_json_string( "{ \"test\" : [ 1, 2, null, false, { \"foo\" : \"bar\" } ] }", &json );
	pretty_print_json( json, 0 );
	free_json_object( &json );

	parse_json_string( "{ \"test\" : \"inline \\\" quote\" }", &json );
	pretty_print_json( json, 0 );
	free_json_object( &json );

	parse_json_string( "{ \"test\" : [ 1, 2, null, false, { \"foo\" : \"bar\" } }", &json );

	return EXIT_SUCCESS;
}
