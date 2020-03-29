#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>
#include "record.h"
#include <unistd.h>

CURL *curl;
char *domain, *record_id, *login_token, sub_domain[50];
void help();

void init(int args, char **argv)
{

    domain = getenv("RECORD_DOMAIN");
    if (!domain)
    {
        fprintf(stderr, "The RECORD_DOMAIN not exists in Environment\n");
        exit(-1);
    }
    login_token = getenv("RECORD_LOGINTOKEN");
    if (!login_token)
    {
        fprintf(stderr, "The RECORD_LOGINTOKEN not exists in Environment\n");
        exit(-1);
    }
    if (args > 1)
        strcpy(sub_domain, argv[1]);
    else if (getenv("RECORD_SUBDOMAIN"))
        strcpy(sub_domain, getenv("RECORD_SUBDOMAIN"));
    if (!strlen(sub_domain))
    {

        fprintf(stdout, "The SUB_DOMAIN not exists in Param or Environment,Used hostname\n");
        help();
        gethostname(sub_domain, 50);
        for (int i = 0; i < strlen(sub_domain); i++)
        {
            if (sub_domain[i] >= 65 && sub_domain[i] <= 90)
                sub_domain[i] = sub_domain[i] + 32;
        }
    }
    record_id = getenv("RECORD_ID");
}

void help()
{
    fprintf(stdout, "Usage: autoRecord [OPTIONS] [SUB_DOMAIN] \n");
    fprintf(stdout, "\tDDNS of ipv4/6 for dnspod.cn\n");

    fprintf(stdout, "\tBefore using the program, RECORD_SUBDOMAIN and RECORD_DOMAIN must be provided as environment variables,if not provide SUB_DOMAIN ,the will used hostname \n");
    fprintf(stdout, "Example:\n\tRECORD_DOMAIN=xx.com RECORD_SUBDOMAIN=xxx autoRecord text\nOPTIONS:\n");
    fprintf(stdout, "\t-h Help\n");
}

int main(int args, char **argv)
{
    char **ip;
    char setnv[100];
    memset(setnv, 0, 100);

    if (args > 1 && !strcmp(argv[1], "-h"))
    {
        help();
        exit(0);
    }
    init(args, argv);
    curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "curl init failed.\n");
        return -1;
    }
    Record *r = getRecodeList("wlp2s0");
    sprintf(setnv, "%s:%s", r->name, r->type);
    Record *clits = getItems(login_token, domain);
    Record *tmp = clits;
    for (; tmp; tmp = tmp->next)
    {
        if (!strcmp(tmp->name, r->name) && !strcmp(tmp->type, r->type))
        {
            strcpy(r->rid, (char *)tmp->rid);
            break;
        }
    }
    if (!strlen(r->rid)) //not exist.
    {
        if (createRecode(r))
            exit(-1);
        else
        {
            memset(setnv, 0, 100);
            sprintf(setnv, "%s:%s", r->name, r->type);
            setenv(setnv, r->rid, 1);
            setenv(r->rid, r->ip, 1);
            printf("Create New Record of sub_domain:%s , type:%s ,value:%s.\n", r->name, r->type, r->ip);
        }
    }
    else //reord exist.
    {

        if (strcmp(r->ip, tmp->ip))
        {
            modifyRecode(r);
        }
        setenv(setnv, r->rid, 1);
        setenv(r->rid, r->ip, 1);
    }
    freeRecordList(clits);
    freeRecordList(r);

    int pid = fork();
    if (pid < 0)
        fprintf(stderr, "Fork Failed\n");
    else if (pid == 0)
    {
        //daemon

        for (;;)
        {
            sleep(600);
            r = getRecodeList("wlp2s0");
            sprintf(setnv, "%s:%s", r->name, r->type);
            char *rid = getenv(setnv);
            if (rid && getenv(rid) && strcmp(r->ip, getenv(rid)))
            {
                modifyRecode(r);
                setenv(getenv(rid), r->ip, 1);
            }
            freeRecordList(r);
        }
    }
    else
    {
        printf("sub program %d enable\n", pid);
        curl_easy_cleanup(curl);
        exit(0);
    }
}
