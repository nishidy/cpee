#include "cpee.h"

void archive_create(){
	struct stat st;

	g_archive = archive_write_new();
	archive_write_add_filter_gzip(g_archive);
	archive_write_set_format_pax_restricted(g_archive); // Note 1
}

void archive_register_filename(char* outname){
	archive_write_open_filename(g_archive, outname);
}

void archive_add(char* from, char* to){
	struct archive_entry *entry;
	char buff[8192];
	int len;
	int fd;
	struct stat s;

	if(stat(from, &s)==-1)
		show_errno();

	entry = archive_entry_new(); // Note 2
	archive_entry_set_pathname(entry, to);
	archive_entry_copy_stat(entry,&s);
	archive_write_header(g_archive, entry);
	fd = open(from, O_RDONLY);
	len = read(fd, buff, sizeof(buff));
	while ( len > 0 ) {
	    archive_write_data(g_archive, buff, len);
	    len = read(fd, buff, sizeof(buff));
	}
	close(fd);
	archive_entry_free(entry);
}

void archive_close(){
	archive_write_close(g_archive); // Note 4
	archive_write_free(g_archive); // Note 5
}

