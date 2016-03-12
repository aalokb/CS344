/**********************************************************
 * Name: Marta Wegner
 * File Name: otp_dec.c
 * Date: 3/7/16
 * Description: connects to otp_dec_d and asks it to
 * perform decyption.
 * syntax: otp_dec [ciphertext] [key] [port]
 * When plain text is received, outputs it to stdout
 * If receives key or ciphertext files with bad char, or key
 * file shorter than the ciphertext, it exits with an error
 * If port not found error reported to screen
 **********************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<netdb.h>

int main(int argc, char **argv) {
   int i;

   //Check for correct num of args
   if (argc < 4) {
	printf("Must specifiy ciphertext, key, and port number\n");
	exit(1);
   }

   //Get port num from args
   int portNum = atoi(argv[3]);

   //open cipher text file and key for reading
   int fdCipher = open(argv[1], O_RDONLY);
   int fdKey = open(argv[2], O_RDONLY);

   //check that there was not an error opening
   if (fdCipher == -1 || fdKey == -1) {
	printf("error opening files\n");
	exit(1);
   }

   //Get size of cipher text
   int cLen = lseek(fdCipher, 0, SEEK_END);

   //Get size of key text
   int kLen = lseek(fdKey, 0, SEEK_END);

   //Verify key file is larger than cipher text
   if (kLen < cLen) { //compare key to plain
	printf("Key too short\n");
	exit(1);
   }

   //Create string to hold cipher text
   char *cipherText = malloc(sizeof(char) * cLen); 

   //Set file point to begining of file
   lseek(fdCipher, 0, SEEK_SET);

   //Read cipher text into string
   if (read(fdCipher, cipherText, cLen) == -1) {//read
	//If error reading
	printf("read cipher text dec\n");
	exit(1);
   }

   //Null terminate the string
   cipherText[cLen - 1] = '\0';

   //Check that chars in cipher text are valid
   for (i = 0; i < cLen - 1; i++) {
	if(isalpha(cipherText[i]) || isspace(cipherText[i])) {
	   //If letter or space do nothing
	}
	else { //not letter of space
	   //print error
	   printf("Cipher text invalid char\n");
	   exit(1);
	}
   }

   //Create string to hold key text
   char *keyText = malloc(sizeof(char) * kLen); 

   //Set file point to begining of file
   lseek(fdKey, 0, SEEK_SET);

   //Read key text into string
   if (read(fdKey, keyText, kLen) == -1) {//read
	//If error reading
	printf("read key text enc\n");
	exit(1);
   }

   //Null terminate the string
   keyText[kLen - 1] = '\0';
 
   //Check that chars in plain text are valid
   for (i = 0; i < kLen - 1; i++) {
	if(isalpha(keyText[i]) || isspace(keyText[i])) {
	   //If letter or space do nothing
	}
	else { //not letter of space
	   //print error
	   printf("key text invalid char\n");
	   exit(1);
	}
   }

   //Client
   //create socket
   int socketfd;

   if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {//create
	//Id error creating
	printf("socket error\n");
	exit(1);
   }

   //Setting up address
   struct hostent * server_ip_address;
   server_ip_address = gethostbyname("localhost");

   if(server_ip_address == NULL) {
	printf("could not resolve host name\n");
	exit(1);
   }
 
   struct sockaddr_in server;

   //clear socket structure
   memset((char *)&server, 0, sizeof(server));


   server.sin_family = AF_INET;
   server.sin_port = htons(portNum);
   memcpy(&server.sin_addr, server_ip_address->h_addr, 
	  server_ip_address->h_length);


   //Connect socket
   if(connect(socketfd, (struct sockaddr*) &server, 
			 sizeof(server)) == -1) {
	printf("connect\n");
	exit(2);
   }

   //confirm connection
   int r;
   int conNum;
   //Receive confirmation number
   if((r = recv(socketfd, &conNum, sizeof(conNum), 0)) == -1) {
	//If error receiving
	printf("recv enc\n");
	exit(1);
   } 
   else if(r == 0) {
	printf("recv enc 0\n");
	exit(1);
   }

   //Check that confirmation number is correct
   int confirm = ntohl(conNum);

   //If number recieved is not correct
   if (confirm != 0) {
	printf("could not contact otp_dec_d on port %d\n",
		portNum);
	exit(1);
   }

   //Successful connection to otp_enc_d
   //send cipher text file size
   int cLenSend = htonl(cLen); //convert

   if(send(socketfd, &cLenSend, sizeof(cLenSend), 0) == -1) {
	printf("cipher text file send\n");
	exit(1);
   }

   //send key text file size
   int kLenSend = htonl(kLen); //convert

   if(send(socketfd, &kLenSend, sizeof(kLenSend), 0) == -1) {
	printf("key text file send\n");
	exit(1);
   }

   //Send cipher text
   if(send(socketfd, cipherText, cLen, 0) < cLen){
	printf("cipher text send\n");
	exit(1);
   }

   //Send key text
   if(send(socketfd, keyText, kLen, 0) < kLen){
	perror("key text send");
	exit(1);
   }

   //Receive back decrypted text
   //allocate memory for plain text
   char *plainText = malloc(sizeof(char) * cLen);
   char buffer[1042];

   //Clear plaintext 
   memset(plainText, '\0', cLen);

   //Receive plain text
   int len = 0;
   r = 0;
   while(len < cLen) { //while the whole file has not 
 		       //been received
 	r = recv(socketfd, buffer, cLen, 0);//receive
	len += r;//add len received to total len received

	if (r <= len) {//If total len not yet received
	   if(r == -1) {
		//Error receiving data
		printf("recv plain text file -1\n");
		break;
	   }	   
	   else if(r == 0) {
		//end of data
		if(len < cLen) {
		   printf("recv plain text file <\n");
		   break;
		}
	   }
	   else {
		//concat string
		strncat(plainText,buffer,r);
	   }	   
	}
   }

   plainText[cLen] = '\n';

   //Print plain text
   printf("%s", plainText);

   //Free memory
   free(plainText);
   free(keyText);
   free(cipherText);

   return 0;
}