#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <openssl/md5.h>
#include <archive.h>
#include <archive_entry.h>

#define SIZE  65535
#define PNAME 4096
#define FNAME 1024

struct option {
	int hardlink;
	int compbackup;
	char* commitmsg;
};

struct option g_argoption;
int g_upperbackupsize;
struct archive *g_archive;
char* g_backupdate;

