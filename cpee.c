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

#define SIZE  65535
#define PNAME 4096
#define FNAME 1024

struct option {
	int hardlink;
};

struct option g_argoption;
int g_upperbackupsize;

void show_errno(){
	fprintf(stderr,"errorno: %d\n",errno);
	exit(-1);
}

int is_file_exist(char* path){
	struct stat s;
	if( stat(path,&s) == -1 ){
		switch(errno){
			case ENOENT:
				return 0;
			default:
				fprintf(stderr,"is_file_exist: stat() failed.\n");
				show_errno();
		}
	}else{
		if( !(S_ISREG(s.st_mode)) ){
			fprintf(stderr,"is_file_exist: st_mode is not S_IFREG.\n");
			show_errno();
		}
	}

	return 1;
}

void copy_file_to_file(char* from, char* to){
	struct stat s;
	char buf[SIZE];
	int fd_from, fd_to, size;
	if( is_file_exist(to) ){
		printf("file exists.\n");
		exit(-1);
	}else{
		if( stat(from,&s) == -1 )
			show_errno();

		if(g_upperbackupsize < s.st_size)
			return;

		if((fd_to=open(to,O_CREAT|O_WRONLY,s.st_mode)) != -1){
			if((fd_from=open(from,O_RDONLY)) != -1){
				while(1){
					size = read(fd_from,buf,SIZE);
					switch(size){
						case -1:
							printf("copy_file_to_file : %s, %s\n",from,to);
							show_errno();
						case 0:
							close(fd_to);
							close(fd_from);
							break;
						default:
							if(write(fd_to,buf,size) == -1){
								printf("copy_file_to_file : %s, %s\n",from,to);
								show_errno();
							}
							break;
					}
					if(size==0) break;
				}
			}else{
				printf("copy_file_to_file : %s, %s\n",from,to);
				show_errno();
			}
		}else{
			printf("copy_file_to_file : %s, %s\n",from,to);
			show_errno();
		}

	}
}

void copy_file_to_link(char* from, char* to){
	link(from,to);
}

void get_file_name(char* path, char* file_name){
	char* token;
	char* saveptr;
	char copy[FNAME] = {"\0"};
	memcpy(copy,path,FNAME);
	token = strtok_r(copy,"/",&saveptr);
	while(token!=NULL){
		sprintf(file_name,"%s",token);
		token = strtok_r(NULL,"/",&saveptr);
	}
}

void copy_file_to_dir(char* from, char* to){
	char to_full[FNAME] = {"\0"};
	char from_file[FNAME] = {"\0"};
	get_file_name(from,from_file);
	sprintf(to_full,"%s/%s",to,from_file);
	if(g_argoption.hardlink){
		copy_file_to_link(from,to_full);
	}else{
		copy_file_to_file(from,to_full);
	}
}

void copy_dir_to_dir(char* from, char* to){
	DIR *dir;
	if((dir=opendir(from))==NULL)
		show_errno();

	struct dirent *dp;
	char next_from[FNAME] = {'\0'};
	char next_to[FNAME] = {'\0'};
	struct stat s;
	for(dp=readdir(dir);dp!=NULL;dp=readdir(dir)){
		sprintf(next_from,"%s/%s",from,dp->d_name);
		if(dp->d_type==DT_REG){
			copy_file_to_dir(next_from,to);
		}
		if(dp->d_type==DT_DIR){
			if(strncmp(dp->d_name,"..",2)==0 || strncmp(dp->d_name,".",1)==0)
				continue;

			sprintf(next_to,"%s/%s",to,dp->d_name);
			if(stat(next_from,&s) == 0){
				mkdir(next_to,s.st_mode);
			}else{
				show_errno();
			}
			copy_dir_to_dir(next_from,next_to);
		}
	}
}

void copy_to_dir(int argc, char* argv[]){
	int i;
	char from[FNAME] = {'\0'};
	struct stat s;
	for(i=1;i<argc-1;i++){
		sprintf(from,"%s",argv[i]);
		if( stat(argv[i],&s) == 0 ){
			if( S_ISREG(s.st_mode) )
				copy_file_to_dir(argv[i],argv[argc-1]);

			if( S_ISDIR(s.st_mode) )
				copy_dir_to_dir(argv[i],argv[argc-1]);
		}else{
			show_errno();
		}
	}
}

int is_last_arg_dir(int argc, char* argv[]){
	struct stat s;
	if( stat(argv[argc-1],&s) == 0 ){
		if( S_ISDIR(s.st_mode) )
			return 1;
	}
	return 0;
}

void get_backup_dir(char timestamp[], char* to_path){
	char* backdir;
	if((backdir=getenv("CPEEBACKUPDIR"))==NULL)
		show_errno();
	sprintf(to_path,"%s/%s/",backdir,timestamp);
}

void get_backup_size(){
	char* backsize;
	if((backsize=getenv("CPEEBACKUPSIZE"))==NULL){
		g_upperbackupsize = 1024*1024; // 1MB
	}else{
		g_upperbackupsize = atoi(backsize);
	}
}

void get_date(char date[]){
	time_t timer;
	struct tm* local;
	timer = time(NULL);
	local = localtime(&timer);

	sprintf(date,"%d%02d%02d%02d%02d%02d",
			local->tm_year+1900,local->tm_mon+1,local->tm_mday,
			local->tm_hour,local->tm_min,local->tm_sec);

}

void register_hash(char* from){
	MD5_CTX c;
	MD5_Init(&c);
	MD5_Update(&c,from,strlen(from));
	unsigned char md[MD5_DIGEST_LENGTH];
	MD5_Final(md,&c);

	char mdstr[33];
	int i;
	for(i=0;i<16;i++)
		sprintf(&mdstr[i*2],"%02x",(unsigned int)md[i]);

	printf("MD5 = %s\n",mdstr);
}

void cpee_to_dir(int argc, char* argv[]){
	char date[FNAME];
	get_date(date);

	char to_path[PNAME];
	get_backup_dir(date,to_path);
	mkdir(to_path,0777); // umask works here

	get_backup_size();

	copy_to_dir(argc,argv);

	sprintf(argv[argc-1],"%s/",to_path);
	copy_to_dir(argc,argv);

	register_hash(argv[1]);
}

void cpee_file_to_file(char* from, char* to){
	char date[FNAME];
	get_date(date);

	char to_path[PNAME];
	get_backup_dir(date,to_path);
	mkdir(to_path,0777); // umask works here

	get_backup_size();

	char to_file[PNAME];
	if(g_argoption.hardlink){
		copy_file_to_link(from,to);
	}else{
		copy_file_to_file(from,to);
	}

	char from_file[FNAME] = {"\0"};
	get_file_name(from,from_file);
	sprintf(to_file,"%s/%s",to_path,from_file);
	copy_file_to_file(from,to_file);

	register_hash(from);
}

void copy_from_backup(char* date){
	char to_path[PNAME];
	get_backup_dir(date,to_path);

	struct stat s;
	if( stat(to_path,&s) == 0 ){
		if( !S_ISDIR(s.st_mode) )
			show_errno();
	}else{
		printf("no backups of the date %s.\n",date);
		exit(-1);
	}

	copy_dir_to_dir(to_path,".");
}

void show_backups(){
	char* backdir;
	if((backdir=getenv("CPEEBACKUPDIR"))==NULL)
		show_errno();

	struct dirent **nl;
	int n;

	n = scandir(backdir,&nl,NULL,alphasort);
	if(n<0)
		perror("scandir");
	else {
		while(n--){
			if(strncmp(nl[n]->d_name,"..",2)==0 || strncmp(nl[n]->d_name,".",1)==0)
				continue;
			printf("%s\n",nl[n]->d_name);
			free(nl[n]);
		}
		free(nl);
	}
}


void init_option(){
	g_argoption.hardlink = 0;
}

void shift_arguments(int idx, int argc, char* argv[]){
	int i;
	for(i=idx;i<argc-1;i++)
		argv[i] = argv[i+1];

}

int main(int argc, char* argv[]){

	init_option();
	int i;
	for(i=1;i<argc;i++){
		switch(argv[i][0]){
			case '-':
				if(strncmp(argv[i],"-l",2)==0){
					g_argoption.hardlink = 1;
				}
				shift_arguments(i,argc,argv);
				break;
			default:
				break;
		}
	}

	switch(argc){
		case 0:
			exit(-1);
		case 1:
			show_backups();
			break;
		case 2:
			copy_from_backup(argv[1]);
			break;
		case 3:
			if( is_last_arg_dir(argc,argv) ) {
				cpee_to_dir(argc,argv);
			}else{
				cpee_file_to_file(argv[1],argv[2]);
			}
			break;
		default:
			if( is_last_arg_dir(argc,argv) ) {
				cpee_to_dir(argc,argv);
			}else{
				printf("the last argument must be directory if you give more than 3 args.\n");
				exit(-1);
			}
		break;
	}

	return 0;
}
