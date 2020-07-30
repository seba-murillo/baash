/*
 * sebastr.h
 *
 *  Created on: Oct 20, 2018
 *      Author: seba
 */
#ifndef SEBASTR_H_
#define SEBASTR_H_

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_STRING 1024

char* strfix(char* string, const char C);
char* strext(const char *string, char *result, const char* start,
		const char* end);
char* strsub(char* string, const char* minus);
char* strrep(char* string, const char* replace, const char* replacement);
int isValidString(const char* const string);

#endif /* SEBASTR_H_ */