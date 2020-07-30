/*
 * sebastr.c
 *
 *  Created on: Oct 10, 2018
 *      Author: seba
 */

#include <sebastr.h>
#include <stdbool.h>
#include <string.h>

char* strfix(char* string, const char C) {
	if (!string || !C)
		return NULL;
	char tmp[MAX_STRING] = "";
	int tmp_pos = 0;
	bool flag = false, start = false;
	for (int pos = 0; pos < strlen(string); pos++) {
		if (string[pos] == C) {
			flag = true;
			continue;
		}
		if (flag && start) {
			tmp[tmp_pos++] = C;
			flag = false;
		}
		tmp[tmp_pos++] = string[pos];
		start = true;
		flag = false;
	}
	//string = calloc(strlen(tmp), sizeof(char)); // NO ES NECESARIO, salvo 'string' sea literal
	strcpy(string, tmp);
	return string;
}

char* strext(const char* string, char* result, const char* start,
		const char* end) {
	if (!string || !result)
		return NULL;
	if (!start)
		start = "";
	if (!end)
		end = "";
	memset(result, '\0', MAX_STRING);
	char* tmp;
	if ((tmp = strstr(string, start)) == NULL)
		return NULL;
	int pos1 = tmp - string + strlen(start);
	int pos2 = strlen(string) - strlen(end);
	if (end[0] != '\0') {
		if ((tmp = strstr(string + pos1, end)) == NULL)
			return NULL;
		pos2 = tmp - string;
	}
	strncpy(result, string + pos1, pos2 - pos1);
	return result;
}

char* strsub(char* string, const char* minus) {
	if (!isValidString(string) || !isValidString(minus))
		return NULL;
	char tmp1[MAX_STRING], tmp2[MAX_STRING];
	if (!strext(string, tmp1, "", minus))
		return NULL;
	if (!strext(string, tmp2, minus, ""))
		return NULL;
	sprintf(string, "%s%s", tmp1, tmp2);
	return string;
}

char* strrep(char* string, const char* replace, const char* replacement) {
	if (!isValidString(string) || !isValidString(replace) || !replacement)
		return NULL;
	char tmp1[MAX_STRING], tmp2[MAX_STRING];
	if (!strext(string, tmp1, "", replace))
		return NULL;
	if (!strext(string, tmp2, replace, ""))
		return NULL;
	sprintf(string, "%s%s%s", tmp1, replacement, tmp2);
	return string;
}

int isValidString(const char* const string) {
	if (!string)
		return false;
	if (!strcmp(string, ""))
		return false;
	return true;
}