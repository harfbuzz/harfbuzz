// SPDX-License-Identifier: MIT
// Copyright 2011, SIL International, All rights reserved.

// JSON debug logging very basic test harness
// Author: Tim Eves
#include "inc/json.h"

using namespace graphite2;

int main(int argc, char * argv[])
{
	json	jo(argc == 1 ? stdout : fopen(argv[1], "w"));
	jo << json::array << "a string"
		<< 0.54
		<< 123
		<< false
		<< json::null
		<< json::close;

	jo << json::object
			<< "empty object" << json::object << json::close;

	jo << "primitive types" << json::object
			<< "string"		<< "a string"
			<< "number" 	<< 0.54
			<< "integer" 	<< 123
			<< "boolean" 	<< false
			<< "null" 		<< json::null
			<< json::close;

	jo << "complex object" << json::object
			<< "firstName" 	<< "John"
			<< "lastName" 	<< "Smith"
			<< "age"		<< 25
			<< "address"	<< json::flat << json::object
				<< "streetAddress" 	<< "21 2nd Street"
				<< "city"			<< "New York"
				<< "state"			<< "NY"
				<< "postalCode" 	<< "10021"
				<< json::close
			<< "phoneNmuber" << json::array
				<< json::flat << json::object
					<< "type" 	<< "home"
					<< "number" << "212 555-1234"
					<< json::close
				<< json::object
					<< "type" 	<< "fax"
					<< "number"	<< "646 555-4567";

	return 0;
}

