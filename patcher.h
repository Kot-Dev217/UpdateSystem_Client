#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <dirent.h>
#include "zip.h"
#include <zlib.h>
#include <errno.h>

#define MAX_LINE 2048
#define MAX_LEN 256
#define CHUNK 16384

#define SERVER_IP "192.168.107.103"
#define PORT "5000"

#define VERSION_FILE "/var/opt/version.txt"
#define TMP_PATH "/tmp"
#define WORK_PATH "/work/IDFace"

struct MemoryStruct
{
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
char *checkServer(const char *url);
int isUpdatedVersion();
void patchUpdate();
void downloadFile(const char *url);
void extractZip(const char *zipFilePath);
void remove_newline(char *s);
void list_files(const char *path);
void copy_files(const char *src, const char *dst);
void deleteTemp();