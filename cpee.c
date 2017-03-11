#include "cpee.h"

void show_errno(){
	fprintf(stderr,"errorno: %d\n",errno);
	exit(-1);
}

void commit_message(char* to_path, int argc, char** argv){
	char commitfile[FNAME] = {'\0'};
	sprintf(commitfile,"%s/.commit",to_path);
	int fd = open(commitfile,O_CREAT|O_WRONLY,0400);

	int i=0;
	char arg[FNAME];
	while(i<argc){
		memset(arg,'\0',FNAME);
		sprintf(arg,"arg[%d] : ",i);
		write(fd,arg,strlen(arg));
		write(fd,argv[i],strlen(argv[i]));
		write(fd,"\n",1);
		i++;
	}

	write(fd,"\n",1);
	write(fd,"commit : ",9);
	write(fd,"\n",1);
	write(fd,g_argoption.commitmsg,strlen(g_argoption.commitmsg));
	write(fd,"\n",1);

	close(fd);
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

void remove_top_dir(char* path, char* removed_path){
	char* token;
	char* saveptr;
	char copy[FNAME] = {"\0"};
	memcpy(copy,path,FNAME);
	token = strtok_r(copy,"/",&saveptr);
	if(token!=NULL){
		token=token+strlen(token)+1;
		sprintf(removed_path,"%s",token);
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

void get_backup_dir(char* timestamp, char* to_path){
	char* backdir;
	if((backdir=getenv("CPEEBACKUPDIR"))==NULL)
		show_errno();
	sprintf(to_path,"%s/%s",backdir,timestamp);
}

void get_backup_size(){
	char* backsize;
	if((backsize=getenv("CPEEBACKUPSIZE"))==NULL){
		g_upperbackupsize = 1024*1024; // 1MB
	}else{
		g_upperbackupsize = atoi(backsize);
	}
}

char* return_backupdate(){
	return g_backupdate;
}

void set_backupdate(){
	time_t timer;
	struct tm* local;
	timer = time(NULL);
	local = localtime(&timer);

	g_backupdate = (char*)calloc(16,sizeof(char));

	sprintf(g_backupdate,"%d%02d%02d%02d%02d%02d",
			local->tm_year+1900,local->tm_mon+1,local->tm_mday,
			local->tm_hour,local->tm_min,local->tm_sec);

}

int is_compbackup_option_on(){
	if(getenv("CPEECOMPBACKUP")==NULL)
		return 0;
	return 1;
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
	set_backupdate();

	if(g_argoption.compbackup)
		archive_create();

	char to_path[PNAME];
	get_backup_dir(return_backupdate(),to_path);
	mkdir(to_path,0777); // umask works here

	get_backup_size();

	char archive[FNAME] = {'\0'};
	char file[FNAME] = {'\0'};
	if(g_argoption.compbackup){
		get_file_name(to_path,file);
		sprintf(archive,"%s/%s.tar.gz",to_path,file);
		printf("%s\n",archive);
		archive_register_filename(archive);
	}

	copy_to_dir(argc,argv,argv[argc-1]);

	if(!g_argoption.compbackup)
		copy_to_dir(argc,argv,to_path);

	if(g_argoption.compbackup)
		archive_close();

	if(g_argoption.commitmsg)
		commit_message(to_path,argc-2,++argv);

	register_hash(argv[1]);
}

void cpee_file_to_file(char* from, char* to){
	set_backupdate();

	if(g_argoption.compbackup)
		archive_create();

	char to_path[PNAME];
	get_backup_dir(return_backupdate(),to_path);
	mkdir(to_path,0777); // umask works here

	get_backup_size();

	char archive[FNAME] = {'\0'};
	char file[FNAME] = {'\0'};
	if(g_argoption.compbackup){
		get_file_name(to_path,file);
		sprintf(archive,"%s/%s.tar.gz",to_path,file);
		archive_register_filename(archive);
	}

	char to_file[PNAME];
	if(g_argoption.hardlink){
		copy_file_to_link(from,to);
	}else{
		copy_file_to_file(from,to);
	}

	char from_file[FNAME] = {"\0"};
	if(!g_argoption.compbackup){
		get_file_name(from,from_file);
		sprintf(to_file,"%s/%s",to_path,from_file);
		copy_file_to_file(from,to_file);
	}

	if(g_argoption.compbackup)
		archive_close();

	if(g_argoption.commitmsg)
		commit_message(to_path,1,&to);

	register_hash(from);
}

void show_backups(){
	char* backdir;
	if((backdir=getenv("CPEEBACKUPDIR"))==NULL)
		show_errno();

	struct dirent **nl;
	int n;

	char commitfile[PNAME] = {'\0'};
	char* buf = NULL;
	FILE* fd;
	int size;
	size_t k=0;

	n = scandir(backdir,&nl,NULL,alphasort);
	if(n<0)
		perror("scandir");
	else {
		while(n--){
			if(strncmp(nl[n]->d_name,"..",2)==0 || strncmp(nl[n]->d_name,".",1)==0)
				continue;
			printf("%s : ",nl[n]->d_name);

			sprintf(commitfile,"%s/%s/.commit",backdir,nl[n]->d_name);
			if((fd = fopen(commitfile,"r"))==NULL){
				printf("\n");
			}else{
				while((size = getline(&buf,&k,fd))!=-1){
					if(strncmp(buf,"commit : ",9)==0)
						break;
				}
				if(size==-1)
					printf("\n");

				if(getline(&buf,&k,fd)!=-1)
					printf("%s",buf);
			}
			free(nl[n]);
		}
		free(nl);
	}
}

void init_option(){
	g_argoption.hardlink = 0;
	g_argoption.compbackup = 0;
	g_argoption.commitmsg = NULL;
}

void shift_arguments(int idx, int* argc, char* argv[]){
	int i;
	for(i=idx;i<(*argc)-1;i++)
		argv[i] = argv[i+1];
	(*argc)--;
}

int main(int argc, char* argv[]){

	init_option();

	int i=1;
	while(i<argc){
		switch(argv[i][0]){
			case '-':
				if(strncmp(argv[i],"-l",2)==0)
					g_argoption.hardlink = 1;

				if(strncmp(argv[i],"-c",2)==0)
					g_argoption.compbackup = 1;

				if(strncmp(argv[i],"-m",2)==0){
					shift_arguments(i,&argc,argv);
					g_argoption.commitmsg = argv[i];
				}

				shift_arguments(i,&argc,argv);
				break;
			default:
				i++;
				break;
		}
	}

	if(is_compbackup_option_on())
		g_argoption.compbackup = 1;

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

