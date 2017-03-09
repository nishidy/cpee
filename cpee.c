#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

#define SIZE 65535
#define FNAME 1024

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

		if((fd_to=open(to,O_CREAT|O_WRONLY,s.st_mode)) != -1){
			if((fd_from=open(from,O_RDONLY)) != -1){
				while(1){
					size = read(fd_from,buf,SIZE);
					switch(size){
						case -1:
							printf("copy_file_to_file\n");
							show_errno();
						case 0:
							close(fd_to);
							close(fd_from);
							break;
						default:
							if(write(fd_to,buf,size) == -1){
								printf("copy_file_to_file!\n");
								show_errno();
							}
							break;
					}
					if(size==0) break;
				}
			}else{
				printf("copy_file_to_file\n");
				show_errno();
			}
		}else{
			printf("copy_file_to_file\n");
			show_errno();
		}

	}
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
	copy_file_to_file(from,to_full);
}

void copy_files_to_dir(int argc, char* argv[]){
	int i;
	char from[FNAME] = {'\0'};
	for(i=1;i<argc-1;i++){
		sprintf(from,"%s",argv[i]);
		if(is_file_exist(from)){
			copy_file_to_dir(from,argv[argc-1]);
		}
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


int main(int argc, char* argv[]){

	switch(argc){
		case 0:
		case 1:
		case 2:
			printf("not enough arguments.\n");
			exit(-1);
			break;
		case 3:
			if( is_last_arg_dir(argc,argv) ) {
				printf("aaa\n");
				copy_to_dir(argc,argv);
			}else{
				copy_file_to_file(argv[1],argv[2]);
			}
			break;
		default:
			if( is_last_arg_dir(argc,argv) ) {
				copy_to_dir(argc,argv);
			}else{
				printf("the last argument must be directory if you give more than 3 args.\n");
				exit(-1);
			}
		break;
	}

	return 0;
}

