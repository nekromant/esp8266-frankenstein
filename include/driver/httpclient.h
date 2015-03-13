#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#define BUFFER_SIZE_MAX  5000

/*
 * "full_response" is a string containing all response headers and the response body.
 * "response_body and "http_status" are extracted from "full_response" for convenience.
 *
 * A successful request corresponds to an HTTP status code of 200 (OK).
 * More info at http://en.wikipedia.org/wiki/List_of_HTTP_status_codes
 */
typedef void (* http_callback)(char * response_body, int http_status, char * full_response);

/*
 * Download a web page from its URL.
 * Try:
 * http_get("http://wtfismyip.com/text", http_callback_example);
 */
void ICACHE_FLASH_ATTR http_get(const char * url, http_callback user_callback);

/*
 * Post data to a web form.
 * The data should be encoded as application/x-www-form-urlencoded.
 * Try:
 * http_post("http://httpbin.org/post", "first_word=hello&second_word=world", http_callback_example);
 */
void ICACHE_FLASH_ATTR http_post(const char * url, const char * post_data, http_callback user_callback);

/*
 * Call this function to skip URL parsing if the arguments are already in separate variables.
 */
void ICACHE_FLASH_ATTR http_raw_request(const char * hostname, int port, const char * path, const char * post_data, http_callback user_callback);

/*
 * Output on the UART.
 */
void http_callback_example(char * response, int http_status, char * full_response);

#endif
