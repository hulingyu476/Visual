#pragma warning( disable : 4996)
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
//#include "hidapi.h"
//#include <windows.h>
//#include "afxdialogex.h"
//#include <afxpriv.h>
//#include <atlconv.h>

// Headers needed for sleeping.
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <iostream>


#include "HHT_JSON_Parse.h"
#include "cJSON.h" 


int read_file(const char* filename, char** content)
{
	// open in read binary mode
	FILE* file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "read file fail: %s\n", filename);
		return -1;
	}

	// get the length
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	// allocate content buffer
	*content = (char*)malloc((size_t)length + sizeof(""));

	// read the file into memory
	size_t read_chars = fread(*content, sizeof(char), (size_t)length, file);
	if ((long)read_chars != length) {
		fprintf(stderr, "length dismatch: %d, %d\n", read_chars, length);
		free(*content);
		return -1;
	}
	(*content)[read_chars] = '\0';

	fclose(file);
	return 0;
}

int test_cjson()
{
	const char* filename = "c:\\wemeet\\OSVersion.json";
	int ret = -1;

	char *json = NULL;
	if (read_file(filename, &json) != 0) return -1;;

	if ((json == NULL) || (json[0] == '\0') || (json[1] == '\0')) {
		fprintf(stderr, "file content is null\n");
		return -1;
	}

	cJSON* items = cJSON_Parse(json);
	if (items == NULL) {
		fprintf(stderr, "pasre json file fail\n");
		return -1;
	}
	
	int do_format = 0;
	if (json[1] == 'f') do_format = 1;

	char *printed_json = NULL;
	if (json[0] == 'b') {
		// buffered printing
		printed_json = cJSON_PrintBuffered(items, 1, do_format);
	}
	else {
		// unbuffered printing
		if (do_format) printed_json = cJSON_Print(items);
		else printed_json = cJSON_PrintUnformatted(items);
	}
	printf("do_format =%d\n", do_format);

	if (printed_json == NULL) {
		fprintf(stderr, "print json fail\n");
		return -1;
	}
	printf(" printed_json=%s\n", printed_json);

	
	cJSON*  item = cJSON_GetObjectItem(items, "TMRUpdateStatus");
	fprintf(stdout, "key: %s, value: %s\n", "TMRUpdateStatus", item->valuestring);
	
	char tmp[10] = "0";
	if (strncmp(item->valuestring, tmp, 1) == 0)
		ret = 0;
	else
		ret =1;


	if (items != NULL) cJSON_Delete(items);
	if (json != NULL) free(json);
	if (printed_json != NULL) free(printed_json);

	printf("return ret=%d\n", ret);

	return ret;
}


int isLauncherRoomsEnable() 
{
	return test_cjson();
}