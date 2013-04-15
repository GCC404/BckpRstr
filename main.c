#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define MAXDATESIZE 20    //yyyy_mm_dd_hh_mm_ss
#define MAXLINELENGTH 302 //256 max. folder name + 24 for date + 1 \0
#define newbckpinfoLENGTH 12

int main(int argc, char* argv[]) {

	if (argc != 4) {
		printf("Usage: %s <monitored_monitoreddir_path> <target_monitoreddir_path> <time_int>\n", argv[0]);
		return -1;
	}

	int dt=atoi(argv[3]), oldnumfiles=0;

	//Used for reading time
	struct tm * timeinfo;
	time_t rawtime;

	char bckpinfopath[sizeof(argv[2])+newbckpinfoLENGTH+MAXDATESIZE+2+1];
	bckpinfopath[sizeof(bckpinfopath)-1]=0;
	sprintf(bckpinfopath,"%s//%s",argv[2],"__bckpinfo__");

	FILE * bckpinfo;

	if( (bckpinfo=fopen(bckpinfopath,"w")) == NULL) {
		perror("Couldn't open __bckpinfo__ for writting");
		return -1;
	}

	//Blocks SIGCHLD, in order to allow uninterrupted sleep()
	sigset_t signmask;
	sigemptyset(&signmask);
	sigaddset(&signmask,SIGCHLD);
	sigprocmask(SIG_SETMASK,&signmask,NULL);


	DIR *monitoreddir;
	struct dirent *dentry;
	struct stat stat_entry;
	pid_t pid;

	//Will contain two lines: filename and last modification date
	char entry[MAXLINELENGTH];

	if ((monitoreddir = opendir(argv[1])) == NULL) {
		perror(argv[1]);
		return -1;
	}

	chdir(argv[1]);

	//Reads monitored directory, searching for regular files
	while ((dentry = readdir(monitoreddir)) != NULL) {
		stat(dentry->d_name, &stat_entry);
		if (S_ISREG(stat_entry.st_mode)) {

			//Writes filename and last modification date to __newbckpinfo__
			rawtime=stat_entry.st_mtime;
			timeinfo=localtime(&rawtime);
			sprintf(entry,"%s\n%s", dentry->d_name, asctime(timeinfo));
			fputs(entry,bckpinfo);

			oldnumfiles++;

			//Launches a different process to copy the file to target folder
			if((pid=fork()) < 0) {
				perror("Fork error.\n");
				return -1;
			} else if(pid==0) execlp("cp","cp",dentry->d_name,argv[2],NULL);

		}
	}

	fclose(bckpinfo);

	int s=dt, madecopy=0, newnumfiles=0;
	char newfolder[sizeof(argv[2])+MAXDATESIZE];
	newfolder[sizeof(newfolder)-1]=0;

	time_t foldertime;

	while(1) {

		while(s>0)
			s=sleep(s);

		//Creates a new folder in the target directory with the current time as name
		time(&rawtime);
		timeinfo=localtime(&rawtime);
		sprintf(newfolder,"%s/%d_%d_%d_%d_%d_%d",argv[2],1900+timeinfo->tm_year, 1+timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

		if(mkdir(newfolder,0770)<0) {
			perror("Couldn't create folder.");
			return -1;
		}

		time(&foldertime);

		//Alters path to read in the newly created folder
		sprintf(bckpinfopath,"%s//%s",newfolder,"__bckpinfo__");
		printf("bckpinfopath %s\n",bckpinfopath);

		if( (bckpinfo=fopen(bckpinfopath,"w")) == NULL) {
			perror("Couldn't open __bckpinfo__ for writting");
			return -1;
		}

		sprintf(newfolder,"%s/%d_%d_%d_%d_%d_%d",argv[2],1900+timeinfo->tm_year, 1+timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

		//Reads monitored directory, searching for regular files
		rewinddir(monitoreddir);
		while ((dentry = readdir(monitoreddir)) != NULL) {
			stat(dentry->d_name, &stat_entry);
			if (S_ISREG(stat_entry.st_mode)) {

				//Writes filename and last modification date to __bckpinfo__
				rawtime=stat_entry.st_mtime;
				timeinfo=localtime(&rawtime);
				sprintf(entry,"%s\n%s", dentry->d_name, asctime(timeinfo));
				fputs(entry,bckpinfo);
				newnumfiles++;

				if(rawtime>=foldertime-dt) {
					madecopy=1;
					//Launches a different process to copy the file to target folder
					if((pid=fork()) < 0) {
						perror("Fork error.\n");
						return -1;
					} else if(pid==0) execlp("cp","cp",dentry->d_name,newfolder,NULL);
				}
			}
		}

		fclose(bckpinfo);

		//Checks if changes were made,
		//if not, deletes the folder
		if(!(newnumfiles<oldnumfiles || madecopy)) {
			printf("Apagar\n");
			if((pid=fork()) < 0) {
				perror("Fork error.\n");
				return -1;
			} else if(pid==0) execlp("rm","rm","-rf",newfolder,NULL);
		}

		oldnumfiles=newnumfiles;
		newnumfiles=0;
		madecopy=0;
		s=dt;
	}

	return 0;
}
