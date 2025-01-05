// Copyright (c) Kuba Szczodrzyński 2025-1-4.

#include "include.h"

typedef void *(*pem_func_t)(BIO *, void *, void *, void *);

static void *tls_read_pem(char *buf, pem_func_t func) {
	BIO *bio = BIO_new_mem_buf(buf, -1);
	if (bio == NULL)
		return NULL;
	void *data = func(bio, NULL, NULL, NULL);
	BIO_free(bio);
	return data;
}

int SDL_main(int argc, char *argv[]) {
	MALLOC(SETTINGS, sizeof(*SETTINGS), return 1);
	settings_load();

	// load certificate
	char *cert = file_read_data(SETTINGS->tls_cert_file);
	if (cert == NULL)
		LT_ERR(F, return 1, "TLS certificate file '%s' cannot be read", SETTINGS->tls_cert_file);
	SETTINGS->tls_cert = tls_read_pem(cert, (pem_func_t)PEM_read_bio_X509);
	free(cert);
	if (SETTINGS->tls_cert == NULL)
		SSL_ERROR("TLS certificate parse", return 1);

	// load private key
	char *key = file_read_data(SETTINGS->tls_key_file);
	if (key == NULL)
		LT_ERR(F, return 1, "TLS private key file '%s' cannot be read", SETTINGS->tls_key_file);
	SETTINGS->tls_key = tls_read_pem(key, (pem_func_t)PEM_read_bio_RSAPrivateKey);
	free(key);
	if (SETTINGS->tls_key == NULL)
		SSL_ERROR("TLS private key parse", return 1);

	// extract port number from public server address
	char *port = strchr(SETTINGS->public_server_address, ':');
	if (port != NULL)
		SETTINGS->server_port = strtol(port + 1, NULL, 0);

	for (int i = 0; i < 8; i++)
		game_init();
	net_server_start(true);
	return 0;
}
