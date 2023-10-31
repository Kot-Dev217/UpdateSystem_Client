#include "patcher.h"

char sUpdatedVersion[MAX_LEN];
char sCurrentVersion[MAX_LEN];
char sDeviceType[6];
char patchDir[MAX_LEN];

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL)
    {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void remove_newline(char *s)
{
    while (*s)
    {
        if (*s == '\n')
        {
            *s = '\0';
        }
        s++;
    }
}

int isUpdatedVersion()
{
    FILE *fp = fopen(VERSION_FILE, "r");
    if (fp == NULL)
    {
        printf("Error: could not open file %s", VERSION_FILE);
        return 0;
    }

    size_t len = 0;
    ssize_t read;
    char *line = NULL;

    read = getline(&line, &len, fp);
    remove_newline(line);
    strcpy(sDeviceType, line);
    read = getline(&line, &len, fp);
    strcpy(sCurrentVersion, line);

    printf("device type: %s\n", sDeviceType);
    printf("current version: %s\n", sCurrentVersion);

    free(line);
    fclose(fp);

    char checkUrl[MAX_LINE];
    sprintf(checkUrl, "http://%s:%s/check", SERVER_IP, PORT);
    char *sCheckVersion = checkServer(checkUrl);

    if (!strcmp(sCheckVersion, ""))
    {
        return 0;
    }

    if (!strcmp(sCurrentVersion, sCheckVersion))
    {
        printf("Same version\n");
        return 0;
    }

    strcpy(sUpdatedVersion, sCheckVersion);

    return 1;
}

char *checkServer(const char *url)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return "";
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    printf("%lu bytes retrieved\n", (unsigned long)chunk.size);
    printf("%s\n", chunk.memory);

    // free(chunk.memory);

    return chunk.memory;
}

void downloadFile(const char *url)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;

    char outfilename[MAX_LEN] = "";
    strcpy(outfilename, sUpdatedVersion);
    strcat(outfilename, ".zip");

    curl = curl_easy_init();
    if (curl)
    {
        fp = fopen(outfilename, "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
    }
}

void extractZip(const char *zipfile)
{
    if (!fopen(zipfile, "r"))
        return;

    int errorp = 0;

    zip_t *arch = NULL;

    arch = zip_open(zipfile, 0, &errorp);

    struct zip_stat *finfo = NULL;

    finfo = calloc(256, sizeof(int));

    zip_stat_init(finfo);

    zip_file_t *fd = NULL;

    int index = 0;
    char *outp = NULL;
    char extractedFile[MAX_LEN];

    while (zip_stat_index(arch, index, 0, finfo) == 0)
    {
        fd = zip_fopen_index(arch, index, 0);
        outp = calloc(finfo->size + 1, sizeof(char));
        zip_fread(fd, outp, finfo->size);
        zip_fclose(fd);

        sprintf(extractedFile, "%s/%s", TMP_PATH, finfo->name);
        int len = strlen(finfo->name);

        printf("filename : %s\n", finfo->name);

        if (finfo->name[len - 1] == '/')
        {
            mkdir(extractedFile, 0777);
            index++;
        }
        else
        {
            FILE *fp = fopen(extractedFile, "w");
            if (fp == NULL)
            {
                printf("Error: Failed to open file\n");
                return;
            }
            fwrite(outp, sizeof(char), finfo->size, fp);
            fclose(fp);
            free(outp);
            index++;
        }
    }

    free(finfo);
    zip_close(arch);

    if (remove(zipfile) == 0)
    {
        printf("ZipFile deleted successfully.\n");
    }
    else
    {
        printf("Error deleting zip file.\n");
    }
}

void copy_files(const char *source_file, const char *destination_file)
{
    // Check if destination directory exists, create it if it doesn't
    char *dir = strdup(destination_file);
    char *last_slash = strrchr(dir, '/');
    if (last_slash != NULL)
    {
        *last_slash = '\0';
        struct stat st = {0};
        if (stat(dir, &st) == -1)
        {
            mkdir(dir, 0700);
        }
    }
    free(dir);

    // Copy file from source to destination
    FILE *src = fopen(source_file, "r");
    FILE *dest = fopen(destination_file, "w");
    if (src == NULL || dest == NULL)
    {
        printf("Error opening file\n");
        exit(1);
    }
    int c;
    while ((c = fgetc(src)) != EOF)
    {
        fputc(c, dest);
    }
    fclose(src);
    fclose(dest);
}

void list_files(const char *path)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(path);
    if (dir == NULL)
    {
        printf("Error opening directory\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }
            char new_path[1024];
            snprintf(new_path, sizeof(new_path), "%s/%s", path, entry->d_name);
            list_files(new_path);
        }
        else
        {
            char source_file[MAX_LINE];
            char destination_file[MAX_LINE];
            char current_file_path[MAX_LEN];

            sprintf(source_file, "%s/%s", path, entry->d_name);

            char *root_index = strstr(source_file, patchDir);
            int index = root_index - source_file;
            strncpy(current_file_path, source_file + index + strlen(patchDir) + 1, sizeof(current_file_path));

            sprintf(destination_file, "%s/%s/%s", WORK_PATH, sDeviceType, current_file_path);
            // printf("destination_file: %s\n", destination_file);
            copy_files(source_file, destination_file);
            // printf("%s-------copied to -------- %s\n", source_file, destination_file);
        }
    }

    closedir(dir);
}

void deleteTemp()
{
    char cmd1[MAX_LEN];
    char cmd2[MAX_LEN];
    sprintf(cmd1, "rm -rf %s/server", TMP_PATH);
    sprintf(cmd2, "rm -rf %s/client", TMP_PATH);

    int status = system(cmd1);
    if (status != 0)
    {
        printf("temp file deleting error!\n");
    }

    status = system(cmd2);
    if (status != 0)
    {
        printf("temp file deleting error!\n");
    }
}

void updateFiles()
{
    sprintf(patchDir, "%s/%s", TMP_PATH, sDeviceType);

    list_files(patchDir);
}

void patchUpdate()
{
    char url[MAX_LINE];
    sprintf(url, "http://%s:%s/download/", SERVER_IP, PORT);

    char fileName[MAX_LEN];
    strcpy(fileName, sUpdatedVersion);
    strcat(fileName, ".zip");
    strcat(url, fileName);

    downloadFile(url);

    extractZip(fileName);

    updateFiles();

    deleteTemp();
}

int main(void)
{
    int updateFlag = isUpdatedVersion();

    if (updateFlag == 1)
    {
        patchUpdate();
    }

    // printf("%s\n", sUpdatedVersion);

    return 0;
}