#include "cpee.h"

void copy_file_to_file(char* from, char* to){
	struct stat s;
	char buf[SIZE];
	int fd_from, fd_to, size;
	if( is_file_exist(to) ){
		printf("file exists. %s %s\n",from,to);
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


