#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define MAXDATESIZE 20    //yyyy_mm_dd_hh_mm_ss
#define MAXLINELENGTH 302 //256 max. folder name + 24 for date + 1 \0
#define bckpinfoLENGTH 12

static int unfinishedChilds=0, argv2size=0;

int checkArguments(int argc, char* argv0, char* argv3) {
	if (argc != 4) {
		printf("Usage: %s <monitored_monitoreddir_path> <target_monitoreddir_path> <time_int>\n", argv0);
		return -1;
	}

	int i;
	for(i=0; i<strlen(argv3); i++)
		if( ((int)argv3[i]<48) || ((int)argv3[i]>57) ) {
			printf("Last argument must be fully numeric\n");
			return -1;
		}

	return 0;
}

int chdirec(char* argv) {
	if (chdir(argv) != 0) {
		perror("Couldn't change dir");
		return -1;
	}

	return 0;
}

void sigchld_handler(int coise) {
	unfinishedChilds--;
}

void sigusr1_handler(int coise) {
	if(unfinishedChilds)
		printf("%d processes still in course.\n",unfinishedChilds);
	else exit(0);
}

void install_signal_handlers() {
	struct sigaction action;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	action.sa_handler = sigchld_handler;
	sigaction(SIGCHLD,&action,NULL);

	action.sa_handler = sigusr1_handler;
	sigaction(SIGUSR1,&action,NULL);
}

//Knowing when the program started running
//and file's last modification
//calculates where file's last version
//is saved
int calcdif(const time_t rawtime,const time_t initime,const int dt) {
	int dif;
	dif=rawtime-initime;
	if(dif<0)
		dif=0;

	while(dif%dt!=0)
		dif++;

	dif=(initime+dif);

	return dif;
}

//Checks if the file exists
int checkifexists(const char* filename, const char* argv2,const struct tm *timeinfo) {
	FILE *file;
	char path[MAXLINELENGTH+argv2size];

	sprintf(path,"%s/%d_%d_%d_%d_%d_%d/%s",argv2,1900+timeinfo->tm_year, 1+timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, filename);

	if ((file = fopen(path,"r"))!=NULL) {
		fclose(file);
		return 1;
	}

	return 0;
}

int main(int argc, char* argv[]) {

	if(checkArguments(argc,argv[0],argv[3])!=0)
		return -1;

	int dt=atoi(argv[3]), oldnumfiles=0;

	if(dt<3) {
		write(STDERR_FILENO,"Please choose a bigger dt\n",27);
		return -1;
	}

	argv2size=sizeof(argv[2]); //used in checkifexists(...)

	//Installs signal handlers
	install_signal_handlers();

	//Used for reading time
	struct tm *timeinfo;
	time_t rawtime, foldertime, initime;

	//Stores target path of the several
	//__bckpinfo__, changes with time
	char bckpinfopath[sizeof(argv[2])+bckpinfoLENGTH+MAXDATESIZE+2+1];
	bckpinfopath[sizeof(bckpinfopath)-1]=0;
	sprintf(bckpinfopath,"%s//%s",argv[2],"__bckpinfo__");

	FILE * bckpinfo;

	if( (bckpinfo=fopen(bckpinfopath,"w")) == NULL) {
		perror("Couldn't open __bckpinfo__ for writting");
		return -1;
	}

	//User for reading folder
	DIR *monitoreddir;
	struct dirent *dentry;
	struct stat stat_entry;
	pid_t pid;

	//Will contain two lines: filename and last modification date,
	//to be written on __bckpinfo__
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

			while(waitpid(-1, NULL, WNOHANG)!=-1);

			//Writes filename and last modification date to __newbckpinfo__
			rawtime=stat_entry.st_mtime;
			timeinfo=localtime(&rawtime);
			sprintf(entry,"%s\n%s", dentry->d_name, asctime(timeinfo));
			fputs(entry,bckpinfo);

			oldnumfiles++;
			unfinishedChilds++;

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

	time_t dif=0, current;

	time(&initime);

	while(1) {
		//Sleeps for dt time
		while(s>0)
			s=sleep(s);

		while(waitpid(-1, NULL, WNOHANG)!=-1);

		//Creates a new folder in the target directory with the current time as name
		time(&rawtime);
		timeinfo=localtime(&rawtime);
		sprintf(newfolder,"%s/%d_%d_%d_%d_%d_%d",argv[2],1900+timeinfo->tm_year, 1+timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

		if(mkdir(newfolder,0770)<0) {
			perror("Couldn't create folder.");
			return -1;
		}

		//Alters path to read in the newly created folder
		sprintf(bckpinfopath,"%s//%s",newfolder,"__bckpinfo__");

		if( (bckpinfo=fopen(bckpinfopath,"w")) == NULL) {
			perror("Couldn't open __bckpinfo__ for writting");
			return -1;
		}

		sprintf(newfolder,"%s/%d_%d_%d_%d_%d_%d",argv[2],1900+timeinfo->tm_year, 1+timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

		time(&foldertime);
		//Reads monitored directory, searching for regular files
		rewinddir(monitoreddir);
		while ((dentry = readdir(monitoreddir)) != NULL) {
			stat(dentry->d_name, &stat_entry);
			if (S_ISREG(stat_entry.st_mode)) {

				//Writes filename and last modification date to __bckpinfo__

				rawtime=stat_entry.st_mtime;
				time(&current);

				//Gets late modification path
				dif=calcdif(rawtime,initime,dt);
				timeinfo=localtime(&dif);

				//Sometime there can be an off-by-one error,
				//this should prevent it
				if(dif!=initime && dif!=current) {
					if(!checkifexists(dentry->d_name,argv[2],timeinfo)) {
						dif++;
						timeinfo=localtime(&dif);
					}
				}

				sprintf(entry,"%s\n%d_%d_%d_%d_%d_%d\n", dentry->d_name,1900+timeinfo->tm_year, 1+timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
				fputs(entry,bckpinfo);
				newnumfiles++;


				//If file was changed since the last
				//check, copies it
				if(rawtime>=foldertime-dt) {
					unfinishedChilds++;
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
			unfinishedChilds++;
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
