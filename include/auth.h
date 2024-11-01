#ifndef AUTH_H
#define AUTH_H

#include <openssl/sha.h>

#define DATABASE_PATH "sqlite.db"
#define SALT_LENGTH	  30

#define INIT_SQL                                                               \
	"CREATE TABLE IF NOT EXISTS users ("                                       \
	"uid INTEGER PRIMARY KEY AUTOINCREMENT, "                                  \
	"username TEXT UNIQUE NOT NULL, "                                          \
	"password BLOB NOT NULL, "                                                 \
	"salt TEXT NOT NULL);"

#define ALPHA_NUMERIC                                                          \
	"0123456789"                                                               \
	"!@#$%^&*"                                                                 \
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"                                               \
	"abcdefghijklmnopqrstuvwxyz"

#define PW_SELECT_SQL                                                          \
	"SELECT password, salt, uid FROM users WHERE username = ?;"

int init_db();
int close_db();

char *generate_salt();
char *generate_salted_password(const char *password, const char *salt);

int64_t register_user(const char *username, const char *password);
int64_t verify_user(const char *username, const char *password);

#endif // AUTH_H
