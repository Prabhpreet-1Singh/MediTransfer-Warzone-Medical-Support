#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

//For linux
#include <arpa/inet.h>

//For Windows
//#include <Winsock2.h>
//#include <WS2tcpip.h>

#define SIZE 2048

char bufferB[SIZE];
char bufferSend[SIZE] = {0};

int serverKey = 0;
int clientBKey = 89;


void Encode (int key1, int key2, char * buf, char * begin) {
	int counter = 1;

	while (*buf != '\0') {
		int  new_ascii = *buf;
	
		for(int j=1; j<=(key1*key2*counter); j++){
			new_ascii += 1;
			if (new_ascii > 127) {
				new_ascii = 32;
			}
		}
		
	
		char encrypted = new_ascii;
		*buf = encrypted;
		buf++;
		counter++;
	}
	//fwrite(begin+1, 1 , counter-3*sizeof(char) , current);
}
void read_password(char *password, int size) {
    struct termios old_flags, new_flags;
    tcgetattr(STDIN_FILENO, &old_flags);
    new_flags = old_flags;
    new_flags.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_flags);
    fgets(password, size, stdin);
    password[strcspn(password, "\n")] = '\0';  // remove trailing newline character
    tcsetattr(STDIN_FILENO, TCSANOW, &old_flags);
}

void Decode (int key1, int key2, char * buf, char * begin) {
	int counter = 1;

	while (*buf != '\0') {
		int  new_ascii = *buf;
		

		for(int j=1; j<=(key1*key2*counter); j++){
			new_ascii -= 1;
			if (new_ascii < 32) {
				new_ascii = 127;
			}
		}

		char decrypted = new_ascii;
		*buf = decrypted;
		buf++;
		counter++;
	}

	//fwrite(begin+1, 1 , counter-3*sizeof(char) , current);
}

//Receive data from a socket and write it to a file
void write_file(int sockfd) {
    int n;
    FILE *fp;
    char *filename = "messageFromServer.txt";

    fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Error in writing file.");
        exit(1);  
    }

    n = recv(sockfd, bufferB, SIZE, 0);
    char*begin = &bufferB[0];

    Decode(serverKey, clientBKey, bufferB, begin);
    fprintf(fp, "%s", bufferB);
    bzero(bufferB, SIZE);
    fclose(fp);

    return;
}

//Send data to a socket from a file
void send_file(int sockfd) {
    FILE *fp;
    char *filename ="messageFromClient.txt";

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Error in reading file.");
        exit(1);  
    }

    while (fgets(bufferSend, SIZE, fp) != NULL) {
        char*begin = &bufferSend[0];
        Encode(serverKey, clientBKey, bufferSend, begin);
        printf("Sent encrypted data: %s\n", bufferSend);
        if (send(sockfd, bufferSend, sizeof(bufferSend), 0) == -1) {
            perror("Error in sending data");
            exit(1);
        }
        bzero(bufferSend, SIZE);
    }
    fclose(fp);
}

int main() {
    char *ip = "10.35.70.36";
    int port = 33330;
    int e;
    int p;
    int t=1;
    char password[100];

    int sockfd;
    struct sockaddr_in server_addr;

    // //File variables for sending data
    // FILE *fp;
    // char *filename ="messageFromClient.txt";

    //Create the listening socket and check for errors
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error in socket.");
        exit(1);
    }
    printf("Server socket created.\n");

    //Set up the port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    
        printf("Enter password: \n");
    read_password(password, sizeof(password));
    unsigned char hash[SHA384_DIGEST_LENGTH];
    SHA384((unsigned char *) password, strlen(password), hash);
    char hashed_password[SHA384_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA384_DIGEST_LENGTH; i++) {
        sprintf(&hashed_password[i*2], "%02x", hash[i]);
    }

    while(t){
    if (strcmp(hashed_password, "0cccd75cf243a7ad2bc51e236670a21a4bc6f1c865e6ba85b4b0e0b941cff187e5cd98f63e2be979c29552c305b2a4e1") == 0) {
        printf("Correct Password!\n");
        t=0;
    } 
    else {
        printf("Wrong Password! Try again\n");
        main();
    }
    }

    //Bind the socket to the port and check for errors
    e = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (e == -1) {
        perror("Error in connecting");
        exit(1);
    }
    printf("Connected to server on socket: %i.\n", sockfd);
    sleep(0.5);

    int m;

    //Send the clientB key
    send(sockfd, &clientBKey, sizeof(clientBKey), 0);
    printf("Sent security key %i.\n", clientBKey);

    //Receive the server key
    m = recv(sockfd, &serverKey, sizeof(serverKey), 0);
    //sleep(0.5);
    printf("Received security key %i.\n", serverKey);

    //Client Interface
    int flag = 1;

    int sig = 999;

    //User interface messages
    printf("Enter command\n");
    printf("<1> data request\n<2> data transmission\n<3> disconnection\n");


    //Client loop to process commands
    while (flag == 1) {
        printf("> ");
        scanf(" %i", &sig);

        //Send the chosen command to the server
        send(sockfd, &sig, sizeof(sig), 0);

        //sleep(1);

        //Start the switch for the various possible commands chosen by the user
        switch (sig) {
            default:
                break;

            //Requesting data - client must receive incoming file from the server
            case 1:
                write_file(sockfd);
                printf("Data written in the text file.\n");

                break;

            //Sending data - client must send available file to the server
            case 2:
                //sleep(2);

                send_file(sockfd);
                printf("File data send successful.\n");

                break;

            //Client wants to disconnect - close client
            case 3:
                flag = 0;
                close(sockfd);
                printf("Disconnected from the server.\n");
                break;
        }
    }

    return 0;
}
