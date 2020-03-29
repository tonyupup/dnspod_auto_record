#include "record.h"
#include <memory.h>
#include <string.h>

size_t writeData(void *buffer, size_t size, size_t nmemb, void **p)
{
    void *doc = malloc((size * nmemb + 1) * sizeof(char));
    memset(doc, '\0', (size * nmemb + 1) * sizeof(char));
    strncpy(doc, buffer, size * nmemb);
    *p = doc;
    return size * nmemb;
}
int httpRequest(CURL *curl, char **p)
{
    if (!curl)
    {
        fprintf(stderr, "curl init failed.\n");
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)p);
    // curl_easy_setopt(curl,CURLOPT_HEADERFUNCTION,header_set);
    curl_easy_setopt(curl, CURLOPT_POST, 1); //设置问非0表示本次操作为post
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1); //打印调试信息
    // curl_easy_setopt(curl, CURLOPT_HEADER, 0);  //将响应头信息和相应体一起传给write_data
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        switch (res)
        {

        case CURLE_UNSUPPORTED_PROTOCOL:
        {
            fprintf(stderr, "不支持的协议,由URL的头部指定\n");
            break;
        }
        case CURLE_COULDNT_CONNECT:
        {
            fprintf(stderr, "不能连接到remote主机或者代理\n");
            break;
        }
        case CURLE_HTTP_RETURNED_ERROR:
        {
            fprintf(stderr, "http返回错误\n");
            break;
        }
        case CURLE_READ_ERROR:
        {
            fprintf(stderr, "读本地文件错误\n");
            break;
        }
        default:
        {
            fprintf(stderr, "返回值:{%d\n", res);
            break;
        }
            return -1;
        }
    }
    return 0;
}
xmlNodePtr dfs(xmlNodePtr proot, const char *name)
{

    xmlNodePtr tmp = NULL;
    while (proot != NULL)
    {
        if (!xmlStrcmp(proot->name, BAD_CAST(name)))
            return proot;
        if (proot->children->type == XML_ELEMENT_NODE)
        {
            tmp = dfs(proot->children, name);
            if (tmp)
                return tmp;
        }
        proot = proot->next;
    }
}
Record *getItems(const char *lt, const char *domain)
{
    xmlDocPtr doc = NULL;
    xmlKeepBlanksDefault(0);
    char *p;
    char postfiled[200];
    memset(postfiled, 0, 200);
    sprintf(postfiled, "login_token=%s&domain=%s&format=xml", lt, domain);
    Record rlist;
    memset(&rlist, 0, sizeof(rlist));

    curl_easy_setopt(curl, CURLOPT_URL, URL_RECORD_LIST);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfiled);

    if (httpRequest(curl, &p))
        goto FAILED;
    printf("get content size :%d.\n", strlen(p));
    doc = xmlReadMemory(p, strlen(p), "in_memory.xml", "UTF-8", 0);
    xmlXPathContextPtr content = xmlXPathNewContext(doc);
    //
    if (!content)
        goto FAILED;
    xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST("//records/item"), content);

    if (!result || xmlXPathNodeSetIsEmpty(result->nodesetval))
    {
        fprintf(stderr, "result or nodeset is null\n");
        goto FAILED;
    }
    int nsize = result->nodesetval->nodeNr;

    for (int i = 0; i < nsize; i++)
    {
        Record *node = (Record *)malloc(sizeof(Record));
        memset(node, 0, sizeof(rlist));
        strcpy(node->rid, xmlNodeGetContent(dfs(result->nodesetval->nodeTab[i], "id")));
        strcpy(node->name, xmlNodeGetContent(dfs(result->nodesetval->nodeTab[i], "name")));
        strcpy(node->ip, xmlNodeGetContent(dfs(result->nodesetval->nodeTab[i], "value")));
        strcpy(node->type, xmlNodeGetContent(dfs(result->nodesetval->nodeTab[i], "type")));
        node->next = rlist.next;
        rlist.next = node;
    }
    xmlXPathFreeContext(content);
    xmlXPathFreeObject(result);
FAILED:

    xmlFreeDoc(doc);
    free((void *)p);
    return rlist.next;
}

int createRecode(Record *r)
{
    char *p;
    xmlDocPtr doc = NULL;
    xmlXPathContextPtr contex = NULL;
    xmlXPathObjectPtr result = NULL, result2 = NULL;
    xmlKeepBlanksDefault(0);
    char postfiled[200];

    sprintf(postfiled,
            "login_token=%s&domain=%s&format=xml&sub_domain=%s&record_type=%s&record_line=默认&value=%s",
            login_token, domain, r->name, r->type, r->ip);
    curl_easy_setopt(curl, CURLOPT_URL, URL_RECORD_CREATE);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfiled);

    int i = httpRequest(curl, &p);
    if (i < 0)
    {
        fprintf(stderr, "Some Error happed.\n");
        goto FAILED;
    }
    doc = xmlReadMemory(p, strlen(p), "in_memory.xml", "UTF-8", 0);
    contex = xmlXPathNewContext(doc);
    free(p);
    if (!contex)
        goto FAILED;
    result = xmlXPathEvalExpression(BAD_CAST("//status/code"), contex);

    if (!result || xmlXPathNodeSetIsEmpty(result->nodesetval))
    {
        fprintf(stdout, "result or nodeset is null\n");
        goto FAILED;
    }
    xmlChar *status = xmlXPathCastNodeSetToString(result->nodesetval);
    if (strcmp(status, "1"))
    {
        switch (atoi(status))
        {

        case -15:
        {

            fprintf(stderr, "域名已被封禁\n");
            break;
        }
        case 6:
        {

            fprintf(stderr, "域名ID错误\n");
            break;
        }
        case 7:
        {

            fprintf(stderr, "非域名所有者\n");
            break;
        }
        case 8:
        {

            fprintf(stderr, "域名无效\n");
            break;
        }
        case 17:
        {

            fprintf(stderr, "记录的值不正确\n");
            break;
        }
        case 21:
        {

            fprintf(stderr, "域名被锁定\n");
            break;
        }
        case 22:
        {

            fprintf(stderr, "子域名不合法\n");
            break;
        }
        case 23:
        {

            fprintf(stderr, "子域名级数超出限制\n");
            break;
        }
        case 24:
        {

            fprintf(stderr, "泛解析子域名错误\n");
            break;
        }
        case 500025:
        {

            fprintf(stderr, "A记录负载均衡超出限制\n");
            break;
        }
        case 500026:
        {

            fprintf(stderr, "CNAME记录负载均衡超出限制\n");
            break;
        }
        case 26:
        {

            fprintf(stderr, "记录线路错误\n");
            break;
        }
        case 27:
        {

            fprintf(stderr, "记录类型错误\n");
            break;
        }
        case 30:
        {

            fprintf(stderr, "MX 值错误，1-20\n");
            break;
        }
        case 31:
        {

            fprintf(stderr, "没有添加默认线路的记录、存在冲突的记录(A记录、CNAME记录、URL记录不能共存)\n");
            break;
        }
        case 32:
        {

            fprintf(stderr, "记录的TTL值超出了限制、NS记录超出限制\n");
            break;
        }
        case 33:
        {

            fprintf(stderr, "AAAA 记录数超出限制\n");
            break;
        }
        case 34:
        {

            fprintf(stderr, "记录值非法\n");
            break;
        }
        case 37:
        {

            fprintf(stderr, "SRV记录超出限制\n");
            break;
        }
        case 82:
        {

            fprintf(stderr, "不能添加黑名单中的IP\n");
            break;
        }
        case 104:
        {

            fprintf(stderr, "记录已存在无需添加\n");
            break;
        }
        case 110:
        {

            fprintf(stderr, "域名没有备案（显性URL和隐形URL类型）\n");
            break;
        }
        }
        goto FAILED;
    }
    result2 = xmlXPathEvalExpression(BAD_CAST("//record/id"), contex);

    xmlChar *recodeId = xmlXPathCastNodeSetToString(result->nodesetval);

    strcpy(r->rid, recodeId);
    xmlFree(recodeId);
    xmlXPathFreeNodeSetList(result2);
    xmlFreeDoc(doc);
    xmlFree(status);
    xmlXPathFreeContext(contex);
    xmlXPathFreeNodeSetList(result);

    return 0;
FAILED:
    free(p);
    xmlFree(recodeId);
    xmlXPathFreeNodeSetList(result2);
    xmlFreeDoc(doc);
    xmlFree(status);
    xmlXPathFreeContext(contex);
    xmlXPathFreeNodeSetList(result);
    return -1;
}
int deleteRecode(Record *r)
{
    return 0;
}
int modifyRecode(Record *r)
{
    char *p;
    xmlDocPtr doc = NULL;
    xmlXPathContextPtr contex = NULL;
    xmlXPathObjectPtr result = NULL;
    xmlKeepBlanksDefault(0);
    char postfiled[300];
    if (!r->rid)
        return -1;
    sprintf(postfiled,
            "login_token=%s&domain=%s&format=xml&sub_domain=%s&record_id=%s&record_type=%s&record_line=默认&value=%s",
            login_token, domain, r->name, r->rid, r->type, r->ip);
    curl_easy_setopt(curl, CURLOPT_URL, URL_RECORD_MODIFY);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfiled);

    int i = httpRequest(curl, &p);
    if (i < 0)
    {
        fprintf(stderr, "Some Error happed.\n");
        free(p);
        return -1;
    }
    doc = xmlReadMemory(p, strlen(p), "in_memory.xml", "UTF-8", 0);
    contex = xmlXPathNewContext(doc);
    free(p);
    if (!contex)
        goto FAILED;
    result = xmlXPathEvalExpression(BAD_CAST("//status/code"), contex);

    if (!result || xmlXPathNodeSetIsEmpty(result->nodesetval))
    {
        fprintf(stdout, "result or nodeset is null\n");
        goto FAILED;
    }

    char *status = xmlXPathCastNodeSetToString(result->nodesetval);
    if (strcmp(status, "1"))
    {
        switch (atoi(status))
        {
        case -15:
            fprintf(stderr, "域名已被封禁\n");
        case 6:
            fprintf(stderr, "域名ID错误\n");
        case 7:
            fprintf(stderr, "非域名所有者\n");
        case 8:
            fprintf(stderr, "域名无效\n");
        case 13:
            fprintf(stderr, "当前域名有误，请返回重新操作\n");
        case 17:
            fprintf(stderr, "记录的值不正确\n");
        case 21:
            fprintf(stderr, "域名被锁定\n");
        default:
            break;
        }
        goto FAILED;
    }
    xmlFreeDoc(doc);
    xmlXPathFreeContext(contex);
    xmlXPathFreeNodeSetList(result);
    return 0;
FAILED:
    xmlFreeDoc(doc);
    xmlXPathFreeContext(contex);
    xmlXPathFreeNodeSetList(result);
    return -1;
}

Record *getRecodeList(char *interface)
{
    Record r;
    memset(&r, 0, sizeof(Record));
    struct ifaddrs *ifaddr, *ifa;
    int cnt = 0;
    char ip_str[64];

    if (-1 == getifaddrs(&ifaddr))
    {
        perror("getifaddrs");
        return r.next;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    { //&& !strcmp(ifa->ifa_name, interface)
        if (ifa->ifa_name && ifa->ifa_addr)
        {
            memset(ip_str, '\0', 64);
            if (ifa->ifa_addr->sa_family == AF_INET)
            {

                struct sockaddr_in *s6 = (struct sockaddr_in *)(ifa->ifa_addr);
                inet_ntop(AF_INET, &(s6->sin_addr), ip_str, sizeof(ip_str));
                if (!strncmp(ip_str, "192.168", 7) || !strncmp(ip_str, "127.0.0.1", 9) || !strncmp(ip_str, "10.", 3) || !strncmp(ip_str, "172.16", 6))
                    continue;
                else
                {
                    Record *pr = (Record *)malloc(sizeof(Record));
                    memset(pr, 0, sizeof(Record));
                    strcpy(pr->ip, ip_str);
                    strcpy(pr->type, "A");
                    pr->next = r.next;
                    r.next = pr;
                }
            }
            else if (ifa->ifa_addr->sa_family == AF_INET6)
            {
                struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);
                inet_ntop(AF_INET6, &(s6->sin6_addr), ip_str, sizeof(ip_str));
                if (!strncmp(ip_str, "fe80::", 6) || !strncmp(ip_str, "::1", 3))
                    continue;
                else
                {
                    Record *pr = (Record *)malloc(sizeof(Record));
                    memset(pr, 0, sizeof(Record));
                    strcpy(pr->ip, ip_str);
                    strcpy(pr->type, "AAAA");
                    pr->next = r.next;
                    r.next = pr;
                }
            }
            else
                continue;
            if (r.next && record_id)
                strcpy(r.next->rid, record_id);
            if (r.next && sub_domain)
                strcpy(r.next->name, sub_domain);

            printf("%s [%d] %s\n", interface, cnt, ip_str);
            cnt++;
        }
    }
    freeifaddrs(ifaddr);
    return r.next;
}

void freeRecordList(Record *r)
{
    while (r)
    {
        Record *temp = r;
        r = r->next;
        free(temp);
    }
}