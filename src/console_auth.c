#include <stdlib.h>

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/uart.h"
#include "microrl.h"
#include "console.h"
#include "env.h"
#include <base64.h>

#define MAX_AUTH_TOKEN_LEN 64

struct auth_state {
	char	buf[MAX_AUTH_TOKEN_LEN];
	int	pos;
	int	state;
};


void auth_reset(struct auth_state *s)
{
	s->pos = 0;
	s->state = 0;
	memset(s->buf, 0x0, MAX_AUTH_TOKEN_LEN);
	char *hostname = env_get("hostname");
	console_printf("\n\n\n%s login\n\n login: ", hostname);
}

void auth_success(struct auth_state *s)
{
	console_set_charhandler(NULL, NULL);
	console_printf("\n\nWelcome to frankenstein's interactive shell!");
	console_lock(0);
	os_free(s);
}

void auth_console_handler(void *arg, char c)
{
	struct auth_state *s = arg;

	if ((c == '\n') || (c == '\r')) {
		if (!s->state) {
			if (strcmp(s->buf, "root") == 0) {
				s->state++;
				s->pos = 0;
				console_printf("\n password: ");
			} else {
				auth_reset(s);
			}
		} else {
			char *pwd = env_get("passwd");
			if (!pwd) {
				console_printf("\n [!] No root password setup, use 'passwd' command to set one!\n");
				auth_success(s);
				return;
			}

			char *tmp = alloca(b64e_size(strlen(s->buf) + 1));
			b64_encode(s->buf, strlen(s->buf), tmp);
			if (strcmp(tmp, pwd) == 0) {
				auth_success(s);
				return;
			}
			console_printf("\nIncorrect password\n");
			auth_reset(s);
		}
	} else {
		if (!s->state)
			console_printf("%c", c);
		else
			console_printf("*");
		if (s->pos >= MAX_AUTH_TOKEN_LEN) {
			auth_reset(s);
			return;
		}
		s->buf[s->pos++] = c;
	}
}

void console_auth_start()
{
	struct auth_state *state = os_malloc(sizeof(*state));
	auth_reset(state);
	console_set_charhandler(auth_console_handler, state);
	console_lock(1);
}


static int  do_passwd(int argc, const char *const *argv)
{
	char *tmp = alloca(b64e_size(strlen(argv[1])+1));
	b64_encode(argv[1], strlen(argv[1]), tmp);
	env_insert("passwd", tmp);
	env_save();
}

CONSOLE_CMD(passwd, 2, -1,
	    do_passwd, NULL, NULL,
	    "Set up a root password"
	    HELPSTR_NEWLINE "passwd topsecret"
	    );


static int  do_logout(int argc, const char *const *argv)
{
	console_auth_start();
	return 0;
}

CONSOLE_CMD(logout, 1, -1,
	    do_logout, NULL, NULL,
	    "Logout from this shell"
	    HELPSTR_NEWLINE "logout"
	    );
