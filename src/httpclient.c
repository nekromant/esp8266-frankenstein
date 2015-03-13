/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Martin d'Allens <martin.dallens@gmail.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

// FIXME: sprintf->snprintf everywhere.
// FIXME: support null characters in responses.

#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "httpclient.h"


// Debug output.
#if 0
#define PRINTF(...) os_printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

// Internal state.
typedef struct {
	char * path;
	int port;
	char * post_data;
	char * hostname;
	char * buffer;
	int buffer_size;
	http_callback user_callback;
} request_args;

static char * esp_strdup(const char * str)
{
	char * new_str = (char *)os_malloc(os_strlen(str) + 1); // 1 for null character
	if (new_str == NULL)
		return NULL;
	os_strcpy(new_str, str);
	return new_str;
}

static void ICACHE_FLASH_ATTR receive_callback(void * arg, char * buf, unsigned short len)
{
	struct espconn * conn = (struct espconn *)arg;
	request_args * req = (request_args *)conn->reverse;

	if (req->buffer == NULL) {
		return;
	}

	// Let's do the equivalent of a realloc().
	const int new_size = req->buffer_size + len;
	char * new_buffer;
	if (new_size > BUFFER_SIZE_MAX || NULL == (new_buffer = (char *)os_malloc(new_size))) {
		os_printf("Response too long %d\n", new_size);
		os_free(req->buffer);
		req->buffer = NULL;
		// TODO: espconn_disconnect(conn) without crashing.
		return;
	}

	os_memcpy(new_buffer, req->buffer, req->buffer_size);
	os_memcpy(new_buffer + req->buffer_size - 1 /*overwrite the null character*/, buf, len); // Append new data.
	new_buffer[new_size - 1] = '\0'; // Make sure there is an end of string.

	os_free(req->buffer);
	req->buffer = new_buffer;
	req->buffer_size = new_size;
}

static void ICACHE_FLASH_ATTR sent_callback(void * arg)
{
	struct espconn * conn = (struct espconn *)arg;
	request_args * req = (request_args *)conn->reverse;

	if (req->post_data == NULL) {
		PRINTF("All sent\n");
	}
	else {
		// The headers were sent, now send the contents.
		PRINTF("Sending request body\n");
		espconn_sent(conn, (uint8_t *)req->post_data, strlen(req->post_data));
		os_free(req->post_data);
		req->post_data = NULL;
	}
}

static void ICACHE_FLASH_ATTR connect_callback(void * arg)
{
	PRINTF("Connected\n");
	struct espconn * conn = (struct espconn *)arg;
	request_args * req = (request_args *)conn->reverse;

	espconn_regist_recvcb(conn, receive_callback);
	espconn_regist_sentcb(conn, sent_callback);

	const char * method = "GET";
	char post_headers[128] = "";

	if (req->post_data != NULL) { // If there is data this is a POST request.
		method = "POST";
		os_sprintf(post_headers,
				   "Content-Type: application/x-www-form-urlencoded\r\n"
				   "Content-Length: %d\r\n", strlen(req->post_data));
	}

	char buf[2048];
	int len = os_sprintf(buf,
						 "%s %s HTTP/1.1\r\n"
						 "Host: %s:%d\r\n"
						 "Connection: close\r\n"
						 "User-Agent: ESP8266\r\n"
						 "%s"
						 "\r\n",
						 method, req->path, req->hostname, req->port, post_headers);

	espconn_sent(conn, (uint8_t *)buf, len);
	PRINTF("Sending request header\n");
}

static void ICACHE_FLASH_ATTR disconnect_callback(void * arg)
{
	PRINTF("Disconnected\n");
	struct espconn *conn = (struct espconn *)arg;

	if(conn == NULL) {
		return;
	}

	if(conn->proto.tcp != NULL) {
		os_free(conn->proto.tcp);
	}
	if(conn->reverse != NULL) {
		request_args * req = (request_args *)conn->reverse;
		if (req->buffer != NULL) {
			// FIXME: make sure this is not a partial response, using the Content-Length header.

			const char * version = "HTTP/1.1 ";
			if (os_strncmp(req->buffer, version, strlen(version)) != 0) {
				os_printf("Invalid version in %s\n", req->buffer);
				return;
			}
			int http_status = atoi(req->buffer + strlen(version));

			char * body = (char *)os_strstr(req->buffer, "\r\n\r\n") + 4;

			if (req->user_callback != NULL) { // Callback is optional.
				req->user_callback(body, http_status, req->buffer);
			}
			os_free(req->buffer);
		}
		os_free(req->hostname);
		os_free(req->path);
		os_free(req);
	}
	os_free(conn);
}

static void ICACHE_FLASH_ATTR dns_callback(const char * hostname, ip_addr_t * addr, void * arg)
{
	request_args * req = (request_args *)arg;

	if (addr == NULL) {
		os_printf("DNS failed %s\n", hostname);
	}
	else {
		PRINTF("DNS found %s " IPSTR "\n", hostname, IP2STR(addr));

		struct espconn * conn = (struct espconn *)os_malloc(sizeof(struct espconn));
		conn->type = ESPCONN_TCP;
		conn->state = ESPCONN_NONE;
		conn->proto.tcp = (esp_tcp *)os_malloc(sizeof(esp_tcp));
		conn->proto.tcp->local_port = espconn_port();
		conn->proto.tcp->remote_port = req->port;
		conn->reverse = req;

		os_memcpy(conn->proto.tcp->remote_ip, addr, 4);

		espconn_regist_connectcb(conn, connect_callback);
		espconn_regist_disconcb(conn, disconnect_callback);

		// TODO: consider using espconn_regist_reconcb (for timeouts?)
		// cf esp8266_sdk_v0.9.1/examples/at/user/at_ipCmd.c  (TCP ARQ retransmission?)

		espconn_connect(conn);
	}
}

void ICACHE_FLASH_ATTR http_raw_request(const char * hostname, int port, const char * path, const char * post_data, http_callback user_callback)
{
	PRINTF("DNS request\n");

	request_args * req = (request_args *)os_malloc(sizeof(request_args));
	req->hostname = esp_strdup(hostname);
	req->path = esp_strdup(path);
	req->port = port;
	req->post_data = esp_strdup(post_data);
	req->buffer_size = 1;
	req->buffer = (char *)os_malloc(1);
	req->buffer[0] = '\0'; // Empty string.
	req->user_callback = user_callback;

	ip_addr_t addr;
	err_t error = espconn_gethostbyname((struct espconn *)req, // It seems we don't need a real espconn pointer here.
										hostname, &addr, dns_callback);

	if (error == ESPCONN_INPROGRESS) {
		PRINTF("DNS pending\n");
	}
	else if (error == ESPCONN_OK) {
		// Already in the local names table (or hostname was an IP address), execute the callback ourselves.
		dns_callback(hostname, &addr, req);
	}
	else if (error == ESPCONN_ARG) {
		os_printf("DNS error %s\n", hostname);
	}
	else {
		os_printf("DNS error code %d\n", error);
	}
}

/*
 * Parse an URL of the form http://host:port/path
 * <host> can be a hostname or an IP address
 * <port> is optional
 */
void ICACHE_FLASH_ATTR http_post(const char * url, const char * post_data, http_callback user_callback)
{
	// FIXME: handle HTTP auth with http://user:pass@host/
	// FIXME: make https work.
	// FIXME: get rid of the #anchor part if present.

	char hostname[128] = "";
	int port = 80;

	if (os_strncmp(url, "http://", strlen("http://")) != 0) {
		os_printf("URL is not HTTP %s\n", url);
		return;
	}
	url += strlen("http://"); // Get rid of the protocol.

	char * path = os_strchr(url, '/');
	if (path == NULL) {
		path = os_strchr(url, '\0'); // Pointer to end of string.
	}

	char * colon = os_strchr(url, ':');
	if (colon > path) {
		colon = NULL; // Limit the search to characters before the path.
	}

	if (colon == NULL) { // The port is not present.
		os_memcpy(hostname, url, path - url);
		hostname[path - url] = '\0';
	}
	else {
		port = atoi(colon + 1);
		if (port == 0) {
			os_printf("Port error %s\n", url);
			return;
		}

		os_memcpy(hostname, url, colon - url);
		hostname[colon - url] = '\0';
	}


	if (path[0] == '\0') { // Empty path is not allowed.
		path = "/";
	}

	PRINTF("hostname=%s\n", hostname);
	PRINTF("port=%d\n", port);
	PRINTF("path=%s\n", path);
	http_raw_request(hostname, port, path, post_data, user_callback);
}

void ICACHE_FLASH_ATTR http_get(const char * url, http_callback user_callback)
{
	http_post(url, NULL, user_callback);
}

void http_callback_example(char * response, int http_status, char * full_response)
{
	os_printf("http_status=%d\n", http_status);
	os_printf("strlen(response)=%d\n", strlen(response));
	os_printf("strlen(full_response)=%d\n", strlen(full_response));
	os_printf("response=%s\n(end)\n", response);
}

void http_test()
{
	/* Test cases:
	http_get("https://google.com"); // Should fail.
	http_get("http://google.com/search?q=1");
	http_get("http://google.com");
	http_get("http://portquiz.net:8080/");
	http_raw_request("google.com", 80, "/search?q=2", NULL);
	http_get("http://173.194.45.65"); // Fails if not online yet. FIXME: we should wait for DHCP to have finished before connecting.
	*/

	http_get("http://wtfismyip.com/text", http_callback_example);
	http_post("http://httpbin.org/post", "first_word=hello&second_word=world", http_callback_example);
}
