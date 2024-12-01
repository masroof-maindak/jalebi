#ifndef AUTH_H
#define AUTH_H

#include <openssl/sha.h>

#include "bool.h"
#include "utils.h"

#define DATABASE_PATH "sqlite.db"
#define SALT_LENGTH	  30

#define ALPHA_NUMERIC                                                          \
	"0123456789"                                                               \
	"!@#$%^&*"                                                                 \
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"                                               \
	"abcdefghijklmnopqrstuvwxyz"

#define INIT_SQL                                                               \
	"CREATE TABLE IF NOT EXISTS users ("                                       \
	"uid INTEGER PRIMARY KEY AUTOINCREMENT, "                                  \
	"username TEXT UNIQUE NOT NULL, "                                          \
	"password BLOB NOT NULL, "                                                 \
	"salt TEXT NOT NULL);"

#define PW_SELECT_SQL                                                          \
	"SELECT password, salt, uid FROM users WHERE username = ?;"

#define INSERT_USER_SQL                                                        \
	"INSERT INTO users (username, password, salt) VALUES (?, ?, ?);"

int init_db();
bool close_db();

void generate_rand_salt(char salt[SALT_LENGTH + 1]);
bool conc_salt_and_pw(char pwSalt[PW_MAX_LEN + SALT_LENGTH + 1],
					  const char pw[PW_MAX_LEN],
					  const unsigned char salt[SALT_LENGTH]);

int64_t register_user(const char *username, const char *password);
int64_t verify_user(const char *username, const char *password);

#endif // AUTH_H
