#include "cpee.h"

void show_errno(){
	fprintf(stderr,"errorno: %d\n",errno);
	exit(-1);
}

void recursive_write_archive(struct archive* a, const char *to_path){

	// copy_dir_to_dir
	DIR *dir;
	if((dir=opendir(to_path))==NULL)
		show_errno();

	struct dirent *dp;
	char next_from[FNAME] = {'\0'};
	char next_to[FNAME] = {'\0'};

	struct archive_entry *entry;
	char buff[8192];
	int len;
	int fd;

	struct stat s;

	for(dp=readdir(dir);dp!=NULL;dp=readdir(dir)){
		sprintf(next_from,"%s/%s",to_path,dp->d_name);
		if(dp->d_type==DT_REG){
			stat(next_from, &s);
			entry = archive_entry_new(); // Note 2
			archive_entry_set_pathname(entry, next_from);
			archive_entry_copy_stat(entry,&s);
			archive_write_header(a, entry);
			fd = open(next_from, O_RDONLY);
			len = read(fd, buff, sizeof(buff));
			while ( len > 0 ) {
			    archive_write_data(a, buff, len);
			    len = read(fd, buff, sizeof(buff));
			}
			close(fd);
			archive_entry_free(entry);
		}
		if(dp->d_type==DT_DIR){
			if(strncmp(dp->d_name,"..",2)==0 || strncmp(dp->d_name,".",1)==0)
				continue;

			sprintf(next_to,"%s/%s",to_path,dp->d_name);
			if(stat(next_from,&s) == 0){
				recursive_write_archive(a,next_to);
			}else{
				show_errno();
			}
		}
	}

}

void write_archive(char *outname, char *to_path)
{
	struct archive *a;
	struct stat st;

	a = archive_write_new();
	archive_write_add_filter_gzip(a);
	archive_write_set_format_pax_restricted(a); // Note 1
	archive_write_open_filename(a, outname);

	recursive_write_archive(a, to_path);

	archive_write_close(a); // Note 4
	archive_write_free(a); // Note 5
}

void comp_backup(char* to_path){
	char archive[FNAME];
	sprintf(archive,"%s.tar.gz",to_path);
	write_archive(archive,to_path);
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

void get_date(char date[]){
	time_t timer;
	struct tm* local;
	timer = time(NULL);
	local = localtime(&timer);

	sprintf(date,"%d%02d%02d%02d%02d%02d",
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
	char date[FNAME];
	get_date(date);

	char to_path[PNAME];
	get_backup_dir(date,to_path);
	mkdir(to_path,0777); // umask works here

	get_backup_size();

	copy_to_dir(argc,argv);

	sprintf(argv[argc-1],"%s/",to_path);
	copy_to_dir(argc,argv);

	if(g_argoption.compbackup)
		comp_backup(to_path);

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

	if(g_argoption.compbackup)
		comp_backup(to_path);

	register_hash(from);
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
	g_argoption.compbackup = 0;
	g_argoption.commitmessage = NULL;
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
					g_argoption.commitmessage = argv[i];
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

