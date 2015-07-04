/* Name: Juan Chavez
 * Date: 11/22/2014
 * Class: CS165 Fall2014
 * Assignment: HW2; client
 * */
#include <stdlib.h> 
#include <string>
#include <time.h>
#include <sstream>
//openssl libraries
#include <openssl/ssl.h>//secure socket layer 
#include <openssl/bio.h>//basic input output for secure sockets
#include <openssl/err.h>//error handling
#include <openssl/rsa.h>//rsa cryptosystem
#include <openssl/pem.h>//used to read .pem files(used for rsa keys)

using namespace std;

//Load public key from file

BIO *public_bio = BIO_new_file("public_key.pem", "r");//to read file
RSA *public_key = PEM_read_bio_RSA_PUBKEY(public_bio, 
													NULL, NULL, NULL);	
int public_key_size = RSA_size(public_key);

void SSL_Errors()
{
	char buff[600];//create a buffer (at least 120 bytes long)
	unsigned long err;/*datatype get_error and error_string use for the
	error code*/
	
	err = ERR_get_error();
	while(err != 0)//will return 0 when no more errors
	{
		ERR_error_string_n(err, buff, 600);
		printf("%s\n", buff);
		err = ERR_get_error();
	}
	
	return;
}

void Read_Item(char decrypted[], SSL *ssl)
{	
	int length = 0;
	char buffer[public_key_size];
	bzero(buffer, sizeof(buffer));//clear buffer
	
	length = SSL_read(ssl, buffer, sizeof(buffer));//read challenge	
	
	int d = RSA_public_decrypt(length, (unsigned char*) buffer,
	(unsigned char*) decrypted, public_key, RSA_PKCS1_PADDING);
}

void Write_Item(char buffer[], SSL *ssl)
{
	char sign[public_key_size];
	bzero(sign, sizeof(sign));
	
	RSA_public_encrypt(sizeof(buffer), (unsigned char*) buffer, 
	(unsigned char*) sign, public_key, RSA_PKCS1_PADDING); 
	
	SSL_write(ssl, sign, sizeof(sign));	
}

void Read_File(SSL *ssl)
{
	printf("Reading file\n");
	int length;
	
	char buffer[public_key_size];
	bzero(buffer, sizeof(buffer));
	
	while((length = SSL_read(ssl, buffer, sizeof(buffer))) > 0)
	{
		char decrypted[public_key_size];
		bzero(decrypted, sizeof(decrypted));
		
		int d = RSA_public_decrypt(length, (unsigned char*) buffer,
		(unsigned char*) decrypted, public_key, RSA_PKCS1_PADDING);
		
		printf("%s", decrypted);
	}
	printf("Done with reading file\n");
}

void Write_File(char filename[], SSL *ssl)
{
	printf("Writing file\n");
	
	printf("filename: %s\n", filename);
	BIO *sbio = BIO_new_file(filename, "r");
	printf("created bio\n");
	int length;
	
	char buffer[60];
	bzero(buffer, sizeof(buffer));
	
	while(((length = BIO_read(sbio, buffer, sizeof(buffer)) > 0)))
	{
		char sign[public_key_size];
	
		RSA_public_encrypt(sizeof(buffer), (unsigned char*) buffer, 
		(unsigned char*) sign, public_key,RSA_PKCS1_PADDING); 
		
		SSL_write(ssl, sign, sizeof(sign));
	}
	printf("Done with writing file\n");
}

int main(int argc, char**argv)
{
    //____________________________________________________Initialization
    
	SSL_library_init();//loads ssl encryption and hash algorithms
	SSL_load_error_strings();
	ERR_load_crypto_strings();
	srand(time(NULL));//Initialize PRNG with a relatively secure seed
	
	//_________________________________________________________Arguments
	
	//ERROR CHECK: Make sure there are five arguments passed	
	if(argc != 5)
	{
		printf("Error: Must be in format client --serverAddress=");
		printf("000.111.222.333 --port=1234 --send/--receive ./file");
		exit(-1); 
	}	
	
	//Second Argument
	
	char *server_address;
	server_address = strndup(argv[1], strlen(argv[1]));
	
	//Split the string at the '='
	char *tk;
	tk = strtok(server_address, "=");
	
	/*ERROR CHECK: Check to see whether the string was of the form
	'--serverAddress'*/
	if(tk == NULL || strcmp(tk, "--serverAddress"))
	{
		printf("Error: Incorrect formatting\n");
		exit(-1);
	}
	
	//Get the second token after the '=' (this should be server address)
	tk = strtok(NULL, "=");	
	
	//Copy server address number to explicit parameter for clarity
	char *server_address_num;
	server_address_num = strndup(tk, strlen(tk));
	
	//Third Argument
	
	char *port;
	port = strndup(argv[2], strlen(argv[2]));
	
	//Split the string at the '='
	tk = strtok(port, "=");
	
	//ERROR CHECK:Check whether the string was of the form --port=1234
	if(tk == NULL || strcmp(tk, "--port"))
	{
		printf("Error: Incorrect formatting of '--port'\n");
		exit(-1);
	}
	
	/*Get the second token after the '=' (this should be just the port
	 * number)*/
	tk = strtok(NULL, "=");	
	
	//Copy port number to explicit parameter for clarity
	char *port_num;
	port_num = strndup(tk, strlen(tk));
	
	//Fourth Argument
	
	char *file_op;//file operation: send or receive
	file_op = strndup(argv[3], strlen(argv[3]));
	
	//ERROR CHECK: Check whether string is --send or --receive
	if(strcmp(file_op, "--send") && strcmp(file_op, "--receive"))
	{
		printf("Error: Incorrect formatting of '--send/receive'\n");
		exit(-1);
	}
	
	//Fifth Argument
	
	//TODO: check for './file' formatting
	char *file;
	file = strndup(argv[4], strlen(argv[4]));
	
	//___________________________________________________________Context
		
	SSL_CTX *CTX;
	CTX = SSL_CTX_new(SSLv3_client_method());
	
	//Error Check:
	if(!CTX)
	{
		printf("Error: Failed to CTX_new\n");
		exit(-1);
	}
	
	SSL_CTX_set_verify(CTX, SSL_VERIFY_NONE, NULL);//disable certificate
	
	bool list_success = SSL_CTX_set_cipher_list(CTX, "ADH");
	
	//ERROR CHECK:
	if(!list_success)
	{
		printf("Error: cipher list failed to initialize\n");
		exit(-1);
	}
	
	//_______________________________________________________________BIO
	
	//Create 'host:port' string
	char host_port[50] = "localhost:";
	strcat(host_port, port_num);
	
	//Create client BIO
	BIO *cbio = BIO_new_connect(host_port);
	
	//Create socket and bind address to it
	int b = BIO_do_connect(cbio);
	//ERROR CHECK:
	if(b <= 0)
	{
		printf("Error: failed to bind client socket");
		exit(-1);
	}
	
	//_______________________________________________________________SSL
	
	SSL *ssl;//set up SSL structure
	ssl = SSL_new(CTX);//create a new SSL structure for a connection
	
	//ERROR CHECK:
	if(ssl == NULL)
	{
		printf("Error: failed to create SSL structure using context\n");
		exit(-1);
	}
	SSL_set_bio(ssl, cbio, cbio);/*SSL engine inherits the behavior
	of rbio and wbio*/	
	
	//___________________________________________________________Connect
	
	//Connect to server
	
	int connect_error = SSL_connect(ssl);
	
	//ERROR CHECK:
	if(connect_error < 0)
	{
		printf("Error: failed to connect\n");
		exit(-1);
	}
	
	printf("Connected to server\n");
	
	//____________________________________________________Send Challenge
	
	//Generate random number
	
	int random_number = rand() % 100;
	
	//Convert this number into c-string
	char challenge[5];
	sprintf(challenge, "%d", random_number);//convert to string
	printf("Chosen rand num: %s\n", challenge);
	
	char digest[SHA_DIGEST_LENGTH]; 
	SHA_CTX stx;
	SHA1_Init(&stx);
	SHA1_Update(&stx, challenge, strlen(challenge));
	SHA1_Final((unsigned char*)digest, &stx);
	
	//Hash challenge
	char hash[2*SHA_DIGEST_LENGTH+1];
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		sprintf(&hash[i*2], "%02x", (unsigned int)digest[i]);
	}		
	
	printf("Hashed challenge: %s\n", hash);
	
	Write_Item(challenge, ssl);
	
	printf("Sent Encrypted challenge to server\n");
	
	//_______________________________________________________Signed Hash
	

	char size[5];//store the size of the hash
	char responce[public_key_size];
	//Get the hash size
	Read_Item(size, ssl);
	int s = atoi((const char*)size);
	
	//Get the hash
	char auth[s];
	bzero(auth, sizeof(auth));
	char *a_ptr;
	for(int i = 0; i < s; i += 8)
	{
		a_ptr = &auth[i];
		Read_Item(a_ptr, ssl);
	}
	printf("Received hash from server: %s\n", auth);
	//Read_Item(responce, ssl);
	printf("Comparing the two hashes: ");
		
	//Compare the two
	if(!strcmp(auth, responce))
	{
		printf("Not verified, exiting\n");
		
		SSL_CTX_free(CTX);
		SSL_free(ssl);	
		exit(-1);	
	} 	
	
	printf("Verified\n");	
	
	//_____________________________________________________________Files
	
	printf("Sending the request %s to server\n", file_op);
	Write_Item(file_op, ssl);
			
	if(!strcmp(file_op, "--send"))
	{
		printf("Sending %s to server\n", file);
		Write_File(file, ssl);    
	}
	
	else
	{
		printf("Receiving %s from server\n", file);
		Read_File(ssl);	
	}
	
	//Free after done
	SSL_CTX_free(CTX);
	SSL_free(ssl);	
	return 0;
}
