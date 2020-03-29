
#ifndef _RECORD_H_
#define _RECORD_H_

#include <libxml/globals.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <curl/curl.h>
#include <libxml/xpath.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#define URL_RECORD_LIST "https://dnsapi.cn/Record.List"
#define URL_RECORD_CREATE "https://dnsapi.cn/Record.Create"
#define URL_RECORD_MODIFY "https://dnsapi.cn/Record.Modify"

xmlNodePtr dfs(xmlNodePtr, const char *);
int httpRequest(CURL *curl, char **);
size_t writeData(void *buffer, size_t size, size_t nmemb, void **p);

extern CURL *curl;
extern char *domain, *login_token, *record_id;
extern char sub_domain[50];

typedef struct Record
{
    char rid[15];
    char ip[50];
    char type[5];
    char name[50];
    struct Record *next;
} Record;
Record *getItems(const char *lt, const char *domain);
//if used getRecodeList ,must use freeRecordList();
Record *getRecodeList(char *interface);
void freeRecordList(Record *r);
int createRecode(Record *);
int deleteRecode(Record *);
int modifyRecode(Record *);
#endif