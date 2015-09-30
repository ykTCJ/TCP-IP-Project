#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>          
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <dirent.h>

#include "inet.h"

void error(const char *msg){
    perror(msg);
    exit(0);
}

//Creating file on client-site
void createFile(int sockfd){	
  	printf("Creating a file...");
	
	//Setting directory
	char content[256];
	char dir[256] = "/root/Desktop/yk/Project/";
	char hostname[256];
	gethostname(hostname, 255);
	char file[256] = "/Client/";
	printf("\nPath: %s", dir);
	
	//Create directory if it does not exist	
	struct stat st = {0};
	if(stat(dir, &st) == -1){
	  mkdir(dir, 0700);
	}
	
	//Create file name
	char filename[256];
	printf("\nPlease enter the file name: ");
	fgets(filename, 256, stdin);
	
	if(filename != NULL){
		strcat(dir, filename);
		printf("File location: %s", dir);
		//Create file
		FILE *fp;
		fp = fopen(dir, "w+");
		if(fp == NULL){
		  printf("\nERROR: File cannot be created");
		  perror("fopen");	
		  exit(0);	
		}
		//Client keyin file content
		else{	
		  printf("Please keyin the file content: ");
		  fgets(content, 256, stdin);
		  printf("Content: %s", content);
		 
		  fprintf(fp, "%s", content);
		  fclose(fp);
		  printf("\nFile created successfully");
		}
	}
	else{
		printf("\nERROR: Filename cannot be empty\n");		
		exit(0);
	}
}

//Client download file from Server
void downloadFile(int sockfd){	
	printf("Downloading file from Server... ");
	
	int n;
	int buflen;

	//Setting directory
	char revBuff[256];
	char dir[256] = "/root/Desktop/yk/Project/";
	char hostname[256];
	gethostname(hostname, 255);
	char file[256] = "/Client/";
	
	
	//Create directory if it does not exist
	struct stat st = {0};
	if(stat(dir, &st) == -1){
	  mkdir(dir, 0700);
	}

	//Getting available file from Server
	char tempo[256];
	bzero(tempo,256);
	n = read(sockfd, (char*)&buflen, sizeof(buflen));
	if (n < 0) error("ERROR reading from socket");
	buflen = htonl(buflen);
	n = read(sockfd,tempo,buflen);
	if (n < 0) error("ERROR reading from socket");
	printf("\nAvailable file: \n");
	printf("%s", tempo);

	//Need more enhance
	printf("Please keyin the file name that you want to download: ");
	char selectFile[256];
	bzero(selectFile,256);
	fgets(selectFile,255,stdin);
    	char input[256];
	
	//Sending file name that Client wants to download to Server
	int datalen = strlen(selectFile);
	int tmp = htonl(datalen);
	n = write(sockfd, (char*)&tmp, sizeof(tmp));
	if(n < 0) error("ERROR writing to socket");
	n = write(sockfd,selectFile,datalen);
	if(n < 0) error("ERROR writing to socket");
	
	char filename[256];
	printf("Save the file name as: ");
	fgets(filename, 256, stdin);

	if(filename != NULL){
		//Concatenate directory and filename
		strcat(dir, filename);	
		printf("File location: %s", dir);

		FILE *fr = fopen(dir, "ab");
		if(fr == NULL){
		  printf("File cannot be opened");
		  perror("fopen");
		  exit(0);
		}
		else{	//Receiving file from Server 
		  bzero(revBuff, 256);
		  int fr_block_sz = 0;
		  while((fr_block_sz = recv(sockfd, revBuff, 256, 0)) > 0){
		  	int write_sz = fwrite(revBuff, sizeof(char), fr_block_sz, fr);
			if(write_sz < fr_block_sz){
			  error("File write failed on server.\n");
			}
			bzero(revBuff, 256);
			if(fr_block_sz == 0 || fr_block_sz != 256){
			  break;			
			}
		  }
		  printf("\nFile downloaded successfully");
		  fclose(fr);
		}
	}
	else{
		printf("\nERROR: Filename cannot be empty\n");		
		exit(0);
	}
}

//Client send file to Server
void sendFile(int sockfd){	

	printf("Sending file to Server...");
	char buff[256];
	int n;
	
	//Setting directory
	char dir[256] = "/root/Desktop/yk/Project/";
	char hostname[256];
	gethostname(hostname, 255);
	char file[256] = "/Client/";
    printf("\nPath: %s", dir);
	
	//Create directory if it does not exist
	struct stat st = {0};
	if(stat(dir, &st) == -1){
	  mkdir(dir, 0700);
	}
	
	//Printing files that is available from the directory
	printf("\nAvailable file: \n");
	DIR *directory;
	struct dirent *ent;
	if((directory = opendir(dir)) != NULL){
	  while((ent = readdir(directory)) != NULL){
		printf("%s", ent->d_name);
	  }
	  closedir(directory);
	}
	else{
	  perror("ERROR");
	  exit(0);
	}
	
	//Selecting file to be sent to Server
	char tempo[256];
	printf("\nPlease keyin the file name that you wish to send: ");
	fgets(tempo, 256, stdin);
	char filename[256];
	strcpy(filename, tempo); 
	
	if(filename != NULL){

		//Sending the file name to Server
		int datalen = strlen(tempo);
		int tmp = htonl(datalen);
		n = write(sockfd, (char*)&tmp, sizeof(tmp));
		if(n < 0) error("ERROR writing to socket");
		n = write(sockfd,tempo,datalen);
		if (n < 0) error("ERROR writing to socket");
	
		char split[2] = "\n";
	 	strtok(tempo, split);

		strcat(dir, filename);
		printf("Sending %s to Server... ", tempo);
		printf("\nDir: %s", dir);
	
		FILE *fs = fopen(dir, "rb");
		if(fs == NULL){
		  printf("\nERROR: File not found.\n");
		  perror("fopen");
		  exit(0);
		}
		else{	//Sending file to Server
		  bzero(buff, 256);
		  int fs_block_sz;
		  while((fs_block_sz = fread(buff, sizeof(char), 256, fs)) > 0){
		    if(send(sockfd, buff, fs_block_sz, 0) < 0){
			fprintf(stderr, "ERROR: Failed to send file. %d", errno);
			break;
		    }
		    bzero(buff, 256);
		  }
		  printf("\nFile sent successfully\n");
		  fclose(fs);
		}
	}
	else{
		printf("\nERROR: Filename cannot be empty\n");		
		exit(0);
	}
		
}

//Client Delete file
void deleteFile(int sockfd){	
	printf("Deleting a file...");
	
	//Setting directory
	char content[256];
	char dir[256] = "/root/Desktop/yk/Project/";
	char hostname[256];
	gethostname(hostname, 255);
	char file[256] = "/Client/";
	printf("\nPath: %s", dir);
	
	//Create directory if it does not exist	
	struct stat st = {0};
	if(stat(dir, &st) == -1){
	  mkdir(dir, 0700);
	}

	//Printing files that is available from the directory
	printf("\nAvailable file(s): \n");
	DIR *directory;
	struct dirent *ent;
	if((directory = opendir(dir)) != NULL){
	  while((ent = readdir(directory)) != NULL){
		printf("%s", ent->d_name);
	  }
	  closedir(directory);
	}
	else{
	  perror("ERROR");
	  exit(0);
	}

	//Getting the file name to be deleted
	char filename[256];
	printf("\nPlease enter the file name that you wish to delete: ");
	fgets(filename, 256, stdin);

	
	if(filename != NULL){

		strcat(dir, filename);
		FILE *fp;
		
	//Check if the file is available or not
		fp = fopen(dir, "r");
		if(fp == NULL){
		  printf("\nERROR: File cannot be created\n");
		  perror("fopen");	
		  exit(0);	
		}
		else{
		  int status = remove(dir);
		  if(status == 0){
			printf("\nFile deleted successfully");
			fclose(fp);
		  }else{
			printf("\nERROR: unable to delete the file\n");
			exit(0);
		  }
		}
	}
}

//Connecting to Server - SOCKET

//int main(int argc, char *argv[])
int main(int argc, char *argv[])	
{
    // 
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if(argc <= 1) {
	fprintf(stderr,"Error input. \nExample of Correct Usage: %s hostname\n", argv[0]);
       	exit(0);
    }
    //portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    //Testing
    serv_addr.sin_port = htons(SERV_TCP_PORT);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

//Getting the Client's choice
    	printf("\n\nYou are connected to the Server... ");

    	int count = 0;

    	while(count == 0){	
	int proceed = 0;
	printf("\n\n1. Create a file \n2. Download a file \n3. Send a file \n4. Delete a file \n5. Exit");
	printf("\n\nPlease choose one option: ");
	bzero(buffer,256);
	fgets(buffer,255,stdin);
    	char input[256];
	strcpy(input, buffer);
	
//Sending Client's choice to Server
	int datalen = strlen(buffer);
	int tmp = htonl(datalen);
	n = write(sockfd, (char*)&tmp, sizeof(tmp));
	if(n < 0) error("ERROR writing to socket");
	n = write(sockfd,buffer,datalen);
	if (n < 0) error("ERROR writing to socket");
	

	if((strcmp(input, "1\n")) == 0){	//Create file on client-site
	   createFile(sockfd);
	   count = 0;
	}
	else if((strcmp(input, "2\n")) == 0){	//Client download file from Server
	   downloadFile(sockfd);
	   count = 0;
	}
	else if((strcmp(input, "3\n")) == 0){	//Client send file to Server
	   sendFile(sockfd);
	   count = 0;
	}
	else if((strcmp(input, "4\n")) == 0){	//Delete file on client-site
	   deleteFile(sockfd);
	   count = 0;
	}
	else if((strcmp(input, "5\n")) == 0){	//Client disconnect from Server
	   count = 1;
	   proceed = 1;
	}
	else{
	   printf("\nInvalid input, please try again.\n");
	   exit(1);	
	   count = 0;
	}
    }
	
    close(sockfd);
    printf("\nYou have disconnected from the Server. Goodbye\n\n");
    return 0;
}
