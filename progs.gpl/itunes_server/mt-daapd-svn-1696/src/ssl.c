
#include <dirent.h>
#include <time.h>
#include	"ssl.h"


/******************************* Definitions **********************************/
#define DEFAULT_CERT_FILE	"/web/server.pem"
#define DEFAULT_KEY_FILE	"/web/cakey.pem"
#define DEFAULT_CA_FILE		"/web/cacert.pem"
#define DEFAULT_CA_PATH		"/web/"
#define SSL_PORT			443


/*************************** Forward Declarations *****************************/

static int		websSSLSetCertStuff(SSL_CTX *ctx, 
									char *cert_file, 
									char *key_file);
static int		websSSLVerifyCallback(int ok, X509_STORE_CTX *ctx);
static RSA		*websSSLTempRSACallback(SSL *s, int is_export, int keylength);

static int		websSSLReadEvent (webs_t wp);
static int		websSSLAccept(int sid, char *ipaddr, int port, int listenSid);
static void		websSSLSocketEvent(int sid, int mask, int data);


/*********************************** Locals ***********************************/

static int		sslListenSock = -1;			/* Listen socket */
static SSL_CTX	*sslctx = NULL;


/******************************************************************************/
/*
 *	Start up the SSL Context for the application, and start a listen on the
 *	SSL port (usually 443, and defined by SSL_PORT)
 *	Return 0 on success, -1 on failure.
 */
int websSSLOpen()
{
	char		*certFile, *keyFile, *CApath, *CAfile;
	SSL_METHOD	*meth;
	
/*
 *	Install and initialize the SSL library
 */
	apps_startup();
	printf("ssl.c: SSL: Initializing SSL\n"); 
	
	SSL_load_error_strings();

	SSLeay_add_ssl_algorithms();


/*
 *	Important!  Enable both SSL versions 2 and 3
 */
	meth = SSLv23_server_method();
	sslctx = SSL_CTX_new(meth);
	
	if (sslctx == NULL) {
		printf("SSL: Unable to create SSL context!\n"); 
		return -1;
	}

/*
 *	Adjust some SSL Context variables
 */
	SSL_CTX_set_quiet_shutdown(sslctx, 1);
	SSL_CTX_set_options(sslctx, 0);
	SSL_CTX_sess_set_cache_size(sslctx, 128);

/*
 *	Set the certificate verification locations
 */
	CApath = DEFAULT_CA_PATH;
	CAfile = DEFAULT_CA_FILE;
	if ((!SSL_CTX_load_verify_locations(sslctx, CAfile, CApath)) ||
		(!SSL_CTX_set_default_verify_paths(sslctx))) {
		printf("SSL: Unable to set cert verification locations!\n"); 
		websSSLClose();
		return -1;
	}

/*
 *  Setting up certificates for the SSL server.
 *	Set the certificate and key files for the SSL context.
 */
	certFile = DEFAULT_CERT_FILE;
	keyFile = NULL;
	if (websSSLSetCertStuff(sslctx, certFile, keyFile) != 0) {
		websSSLClose();
		return -1;
	}

/*
 *	Set the RSA callback for the SSL context
 */
	SSL_CTX_set_tmp_rsa_callback(sslctx, websSSLTempRSACallback);

/*
 *	Set the verification callback for the SSL context
 */
	SSL_CTX_set_verify(sslctx, SSL_VERIFY_NONE, websSSLVerifyCallback);

/*
 *	Set the certificate authority list for the client
 */
	SSL_CTX_set_client_CA_list(sslctx, SSL_load_client_CA_file(CAfile));

/*
 *	Open the socket
 */
	sslListenSock = socketOpenConnection(NULL, SSL_PORT, 
		websSSLAccept, SOCKET_BLOCK);

	if (sslListenSock < 0) {
		trace(2, T("SSL: Unable to open SSL socket on port <%d>!\n"), 
			SSL_PORT);
		return -1;
	}

	return 0;
}

/******************************************************************************/
/*
 *	Stops the SSL
 */
void websSSLClose()
{
	printf("SSL: Closing SSL\n"); 

	if (sslctx != NULL) {
		SSL_CTX_free(sslctx);
		sslctx = NULL;
	}

	if (sslListenSock != -1) {
		socketCloseConnection(sslListenSock);
		sslListenSock = -1;
	}
}

/******************************************************************************/
/*
 *	Set the SSL certificate and key for the SSL context
 */

int websSSLSetCertStuff(SSL_CTX *ctx, char *certFile, char *keyFile)
{

	if (certFile != NULL) {
		if (SSL_CTX_use_certificate_file(ctx, certFile, 
			SSL_FILETYPE_PEM) <= 0) {
			printf("SSL: Unable to set certificate file <%s>\n", certFile); 
			return -1;
		}

		if (keyFile == NULL) {
			keyFile = certFile;
		}

		if (SSL_CTX_use_PrivateKey_file(ctx, keyFile, SSL_FILETYPE_PEM) <= 0) {
			printf("SSL: Unable to set private key file <%s>\n", keyFile); 
			return -1;
		}
		
/******************************************************************************/
/*		
 *		Now we know that a key and cert have been set against
 *		the SSL context 
 */
		if (!SSL_CTX_check_private_key(ctx)) {
			printf("SSL: Check of private key file <%s> FAILED!\n",keyFile); 
			return -1;
		}
	}

	return 0;
}

/******************************************************************************/
/*
 *	the Temporary RSA callback
 */

static RSA *websSSLTempRSACallback(SSL *ssl, int isExport, int keyLength)
{
	static RSA *rsaTemp = NULL;

	if (rsaTemp == NULL)
		rsaTemp = RSA_generate_key(keyLength, RSA_F4, NULL, NULL);

	return rsaTemp;
}

/******************************************************************************/
/*
 *	SSL Verification Callback
 */

static int sslVerifyDepth = 0;
static int sslVerifyError = X509_V_OK;

int websSSLVerifyCallback(int ok, X509_STORE_CTX *ctx)
{
	char	buf[256];
	X509	*errCert;
	int		err;
	int		depth;

	errCert =	X509_STORE_CTX_get_current_cert(ctx);
	err =		X509_STORE_CTX_get_error(ctx);
	depth =		X509_STORE_CTX_get_error_depth(ctx);

	X509_NAME_oneline(X509_get_subject_name(errCert), buf, 256);

	if (!ok) {
		if (sslVerifyDepth >= depth)	{
			ok = 1;
			sslVerifyError = X509_V_OK;
		} else {
			ok=0;
			sslVerifyError = X509_V_ERR_CERT_CHAIN_TOO_LONG;
		}
	}

	switch (err)	{
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
		X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert), buf, 256);
		break;

	case X509_V_ERR_CERT_NOT_YET_VALID:
	case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
	case X509_V_ERR_CERT_HAS_EXPIRED:
	case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
		break;
	}

	return ok;
}