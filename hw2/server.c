/* Name: Juan Chavez
 * Date: 11/22/2014
 * Class: CS165 Fall2014
 * Assignment: HW2; server
 * */
#include <stdlib.h> 
#include <string>
#include <time.h>
//openssl libraries
#include <openssl/ssl.h>//secure socket layer 
#include <openssl/bio.h>//basic input output for secure sockets
#include <openssl/err.h>//error handling
#include <openssl/rsa.h>//rsa cryptosystem
#include <openssl/pem.h>//used to read .pem files(used for rsa keys)

using namespace std;

//Load keys from files
	
BIO *public_bio = BIO_new_file("public_key.pem", "r");//to read file
RSA *public_key = PEM_read_bio_RSA_PUBKEY(public_bio, 
													NULL, NULL, NULL);
int public_key_size = RSA_size(public_key);
		
BIO *private_bio = BIO_new_file("private_key.pem", "r");
RSA *private_key = PEM_read_bio_RSAPrivateKey(private_bio, 
													NULL, NULL, NULL);
int private_key_size = RSA_size(private_key);

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
	
	int d = RSA_private_decrypt(length, (unsigned char*) buffer,
	(unsigned char*) decrypted, private_key, RSA_PKCS1_PADDING);
}

void Write_Item(char buffer[], SSL *ssl)
{
	char sign[private_key_size];
	bzero(sign, sizeof(sign));
	
	RSA_private_encrypt(sizeof(buffer), (unsigned char*) buffer, 
	(unsigned char*) sign, private_key,RSA_PKCS1_PADDING); 
	
	SSL_write(ssl, sign, sizeof(sign));	
}

void Read_File(SSL *ssl)
{	
	int length;
	
	char buffer[public_key_size];
	bzero(buffer, sizeof(buffer));
	
	while((length = SSL_read(ssl, buffer, sizeof(buffer))) > 0)
	{
		char decrypted[public_key_size];
		bzero(decrypted, sizeof(decrypted));
		
		int d = RSA_private_decrypt(length, (unsigned char*) buffer,
		(unsigned char*) decrypted, private_key, RSA_PKCS1_PADDING);
		
		//BIO_write(rbio, decrypted, d);
		printf("%s", decrypted);
		bzero(buffer, sizeof(buffer));
	}
}

void Write_File(char filename[], SSL *ssl)
{	
	printf("filename: %s\n", filename);
	BIO *sbio = BIO_new_file(filename, "r");
	printf("created bio\n");
	int length;
	
	char buffer[60];
	bzero(buffer, sizeof(buffer));
	
	while(((length = BIO_read(sbio, buffer, sizeof(buffer)) > 0)))
	{
		char sign[private_key_size];
	
		RSA_private_encrypt(sizeof(buffer), (unsigned char*) buffer, 
		(unsigned char*) sign, private_key,RSA_PKCS1_PADDING); 
		
		SSL_write(ssl, sign, sizeof(sign));
	}
}

int main(int argc, char**argv)
{
	
	//____________________________________________________Initialization
	
	SSL_library_init();//Loads ssl encryption and hash algorithms
	SSL_load_error_strings();
	ERR_load_crypto_strings();	
	
	srand(time(NULL));/*Important: initialize PRNG with a distinct 
	enough seed before generating any randomized parameters*/		
	
	//_________________________________________________________Arguments
	
	//ERROR CHECK: Make sure there are two arguments passed
	if(argc != 2)
	{
		printf("Error: Must be in format 'server --port=(portnum)\n");
		exit(-1); 
	}	
	
	//Check proper input
	
	//First argument: --port=####
	char *user_input;
	user_input = strndup(argv[1], strlen(argv[1]));
	
	//Split the string at the '='
	char *tk;
	tk = strtok(user_input, "=");//has --port
	
	//ERROR CHECK: Check to see string was of the form '--port'
	if((tk == NULL) || strcmp(tk, "--port"))
	{
		printf("Error: Incorrect formatting\n");
		exit(-1);
	}
	
	/*Get the second token after the '=' (this should be just the port
	 * number)*/
	tk = strtok(NULL, "=");//has ####(i.e. the portnumber)

	char *port_number;
	port_number = strndup(tk, strlen(tk));/*copy tokenized value to
	port_number for clarity(because it is what it currently holds)*/
	
	//________________________________________________________________DH
	
	//Generate a dh for SSL
	
	DH *dh;
	int prime_len = 69;//length, in bits, of the generated safe prime
	int generator = 2;//a small number usually 2 or 5
	int dh_error = 0;//used to check for errors in generated data
	
	//Allocate DH to store function data
	
	dh = DH_generate_parameters(prime_len, generator, NULL, NULL);
	
	/*ERROR CHECK: check generated parameters(i.e that p is safe and 
	g is suitable)*/
	DH_check(dh, &dh_error);
	if(dh_error)
	{
		printf("Error: Unsuitable DH parameters\n");
		exit(-1);
	}	
	
	//___________________________________________________________Context
	
	SSL_CTX *CTX;//context structure
	CTX = SSL_CTX_new(SSLv3_server_method());//set SSL protocol (server)
	
	//ERROR CHECK:
	if(!CTX)
	{
		printf("Error: Failed to CTX_new\n");
		exit(-1);
	}	
	
	SSL_CTX_set_verify(CTX, SSL_VERIFY_NONE, NULL);/*set verify to none
	because not looking for certificate*/	
	
	SSL_CTX_set_tmp_dh(CTX, dh);/*sets DH parameters to be used to be dh
	key will be inherited by all ssl objects created from ctx*/	
	
	//Set the CTX cipher list as 'ADH' for anonymous Diffie-Hellman
	bool list_success = SSL_CTX_set_cipher_list(CTX, "ADH");
	
	//ERROR CHECK:
	if(!list_success)
	{
		printf("Error: cipher list failed to initialize\n");
		exit(-1);
	}
	
	//_______________________________________________________________BIO
	
	//create a 'host:port' string
	char host_port[50] = "*:";
	strcat(host_port, port_number); 

	//create the server BIO
	BIO *sbio = BIO_new_accept(host_port);/*create a new accept BIO 
	with port hostname_port*/
	
	//ERROR CHECK:
	if(!sbio)
	{
		printf("Error: Failed to create server socket");
		exit(-1);
	}
	
	int b = BIO_do_accept(sbio);//create socket and bind address to it
	
	//ERROR CHECK:
	if(b <= 0)
	{
		printf("Error: failed to bind server socket");
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
	
	SSL_set_bio(ssl, sbio, sbio);/*SSL engine inherits the behavior
	of rbio and wbio*/
	
	//____________________________________________________________Accept
	
	int accept_error = SSL_accept(ssl);//start accepting client reqs
	
	//ERROR CHECK::
	if(accept_error <= 0)
	{
		printf("Error: failed to accept\n");
		exit(-1);
	}
	
	printf("Connected to client\n");
	
	//_________________________________________________________Challenge	
		
	char decrypted[private_key_size];
	bzero(decrypted, sizeof(decrypted));
	Read_Item(decrypted, ssl);
	
	printf("Challenge obtained: %s\n", decrypted);
	
	//Hash Challenge
	char digest[SHA_DIGEST_LENGTH]; 
	SHA_CTX stx;
	SHA1_Init(&stx);
	SHA1_Update(&stx, decrypted, strlen(decrypted));
	SHA1_Final((unsigned char*)digest, &stx);
	int h_length = 2*SHA_DIGEST_LENGTH+1;
	
	char hash[h_length];
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		sprintf(&hash[i*2], "%02x", (unsigned int)digest[i]);
	}
	
	printf("SHA1 hash: %s\n", hash);
	
	//Send hash size
	char size[5];
	sprintf(size, "%d", sizeof(hash));
	Write_Item(size, ssl);
	
	//Send hash in segments
	char *h_ptr;
	for(int i = 0; i < 41; i += 8)
	{
		h_ptr = &hash[i];
		Write_Item(h_ptr, ssl);
	}
	
	//_____________________________________________________________Files
	
	//Obtain request

	char req[private_key_size];
	bzero(req, private_key_size);
	Read_Item(req, ssl);
		
	//Client wants to send
	
	if(!strcmp(req, "--send"))
	{
		printf("Receiving file\n");
		Read_File(ssl);
	}
	
	//Client wants to receive
	
	else
	{
		printf("Sending file\n");
		Write_File("test.txt", ssl);
	}
		
	//Free after done
	SSL_CTX_free(CTX);
	SSL_free(ssl);
	
	return 0;
}
