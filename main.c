#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAXDATESIZE 20

int main(int argc, char* argv[]) {

	time_t rawtime;
	int dt=atoi(argv[3]), n=0;

	struct tm * timeinfo;
	char date[MAXDATESIZE];
	char destinyfolder[sizeof(argv[2])+14];
	char destinyfolder2[sizeof(argv[2])+MAXDATESIZE];
	pid_t pid;
	destinyfolder[sizeof(destinyfolder)-1]=0;
	destinyfolder2[sizeof(destinyfolder2)-1]=0;
	sprintf(destinyfolder,"%s\\%s",argv[2],"__bckpinfo__");
	printf("%s\n",destinyfolder);

	char entry[200];
	FILE * bckinfo;

	if( (bckinfo=fopen(destinyfolder,"w")) == NULL) {
		perror("Couldn't open __bckpinfo__");
		return -1;
	}

	date[MAXDATESIZE-1]=0;

	DIR *dir;
	struct dirent *dentry;
	struct stat stat_entry;

	if (argc != 4) {
		printf("Usage: %s <monitored_dir_path> <target_dir_path> <time_int>\n", argv[0]);
		return 1;
	}
	if ((dir = opendir(argv[1])) == NULL) {
		perror(argv[1]);
		return 2;
	}

	chdir(argv[1]);
	printf("Ficheiros regulares do directorio '%s'\n", argv[1]);

	while ((dentry = readdir(dir)) != NULL) {
		stat(dentry->d_name, &stat_entry);
		if (S_ISREG(stat_entry.st_mode)) {

			rawtime=stat_entry.st_mtime;
			timeinfo=localtime(&rawtime);
			sprintf(entry,"%s\n%s", dentry->d_name, asctime(timeinfo));
			fputs(entry,bckinfo);
			/*
			if((pid=fork()) < 0) {
				perror("Fork error.\n");
				return -1;
			} else if(pid==0) {
				execlp("cp","cp",dentry->d_name,argv[1],NULL);
			}*/
		}
	}

	while(n<5) {

		printf("Press enter to continue...\n");
		getchar();

		time(&rawtime);
		timeinfo=localtime(&rawtime);
		sprintf(destinyfolder2,"%s\\%d_%d_%d_%d_%d_%d",argv[2],1900+timeinfo->tm_year, 1+timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		printf("%s",destinyfolder2);

		if(mkdir(destinyfolder2)<0) {
			perror("Couldn't create folder.");
			return -1;
		}

		n++;
	}

	fclose(bckinfo);

	return 0;
}
