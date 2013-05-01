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
#include <ctype.h>

#define MAXDATESIZE 20 //yyyy_mm_dd_hh_mm_ss
#define MAXLINELENGTH 302 //256 max. folder name + 24 for date + 1 \0
#define MAXNAMESIZE 256
#define newbckpinfoLENGTH 12

int checkArguments(int argc) {
	if (argc != 3) {
		printf("Error");
		return -1;
	}
	return 0;
}

int getFileSize(FILE* bckpinfo) {
	fseek(bckpinfo, 0L, SEEK_END);
	int sz = ftell(bckpinfo);
	fseek(bckpinfo, 0L, SEEK_SET);
	return sz;
}

int chdirec(char* argv) {
	if (chdir(argv) != 0) {
		perror("Couldn't change dir");
		return -1;
	}
	return 0;
}

//vai buscar as datas e apresenta-as ao utilizador para ele escolher uma
int getDates(char * arg2, char buf[]) {
	struct dirent *de = NULL;
	DIR *d = NULL;
	int i = 1, c = 0;
	char tmp[5];

	d = opendir(arg2);
	if (d == NULL ) {
		perror("Couldn't open directory");
	}

	printf("Choose Date\n");

	while ((de = readdir(d)) != NULL ) {
		if (de->d_type == DT_DIR && (strcmp(de->d_name, ".") == 1)
				&& (strcmp(de->d_name, "..") == 1)) {
			printf("[%i] - %s\n", i, de->d_name);
			i++;
		}
	}

	int valid = 0;
	//verifica se o utilizador introduziu um numero
	while (valid == 0) {
		scanf("%s", &tmp);

		valid = 1;
		for (i = 0; i < strlen(tmp); ++i) {
			if (!isdigit(tmp[i])) {
				printf("Enter 5 digits or less\n");
				valid = 0;
				break;
			}
		}
	}
	c = atoi(tmp);

	i = 1;

	rewinddir(d);

	while (i <= c) {
		if ((de = readdir(d)) != NULL ) {
			if (de->d_type == DT_DIR && (strcmp(de->d_name, ".") == 1)
					&& (strcmp(de->d_name, "..") == 1)) {
				i++;
			}
		}
	}

	strcpy(buf, de->d_name);

	return 0;

}
int copyfile(char name[], char folder[], char basedir[], char dest[]) {
	char direc[MAXLINELENGTH];
	//junta ao directorio base as strings que foram encontradas em bckpinfo para obter o path do ficheiro, se a folder for 0 significa que o ficheiro esta no directorio base e por isso apenas se adiciona o nome a string
	if (strcmp(folder, "0\n") == 0) {
		sprintf(direc, "%s/%s", basedir, name);
		direc[strlen(direc) - 1] = 0;
	} else {
		sprintf(direc, "%s/%s", basedir, folder);
		direc[strlen(direc) - 1] = 0;
		sprintf(direc, "%s/%s", direc, name);
		direc[strlen(direc) - 1] = 0;
	}

	execlp("cp", "cp", direc, dest, NULL );
	return 0;
}

//le do bckpinfo a informação da ultima actualização de cada ficheiro na pasta e chama copyfile enviando-lhe a informação da localizaçao do ficheiro
void readBCKP(char basedir[], char direc[], char dest[]) {
	FILE* bckpinfo;
	int k = 0;
	char name[MAXNAMESIZE];
	char folder[MAXDATESIZE];
	char buf[MAXNAMESIZE];

	if ((bckpinfo = fopen("__bckpinfo__", "r")) == NULL ) {
		perror("Couldn't open __bckpinfo__");
		//return -1;
	}
	int sz = getFileSize(bckpinfo);

	while ((fgets(buf, sz, bckpinfo)) != NULL ) {
		strcpy(name, buf);
		k++;
		fgets(buf, sz, bckpinfo);
		strcpy(folder, buf);
		int pid;
		if ((pid = fork()) < 0) {
			perror("Fork error.\n");
		} else if (pid == 0)
			copyfile(name, folder, basedir, dest);

	}
	fclose(bckpinfo);
}

int main(int argc, char* argv[]) {
	if (checkArguments(argc) != 0)
		return -1;

	char date[MAXDATESIZE];
	getDates(argv[1], date);

	char direc[sizeof(argv[1]) + sizeof(date) + 2 + 1];
	sprintf(direc, "%s/%s", argv[1], date);
	//direc[strlen(direc) - 1] = 0;

	chdirec(direc);
	//printf("\n%s\n", direc);

	readBCKP(argv[1], direc, argv[2]);
	return EXIT_SUCCESS;
}
