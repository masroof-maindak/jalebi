#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sqlite3.h>

#include "../include/auth.h"

static sqlite3 *db;

int init_db() {
	int rc;
	if ((rc = sqlite3_open(DATABASE_PATH, &db))) {
		fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
		return -1;
	}

	char *err = NULL;
	rc		  = sqlite3_exec(db, INIT_SQL, 0, 0, &err);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQLite error: %s\n", err);
		sqlite3_free(err);
		return -2;
	}

	return 0;
}

int close_db() {
	int rc;
	if ((rc = sqlite3_close(db)) != SQLITE_OK) {
		fprintf(stderr, "Couldn't close DB: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	return 0;
}

/**
 * @brief Generates a random salt
 */
char *get_salt() {
	char *salt;
	if ((salt = malloc(SALT_LENGTH + 1)) == NULL) {
		perror("malloc()");
		return NULL;
	}

	srand(time(NULL));

	for (int i = 0; i < SALT_LENGTH; i++) {
		int idx = rand() % (sizeof(ALPHA_NUMERIC) - 1);
		salt[i] = ALPHA_NUMERIC[idx];
	}

	salt[SALT_LENGTH] = '\0';
	return salt;
}

/**
 * @brief Concatenates the password and salt
 */
char *get_salted_pw(const char *pw, const unsigned char *salt) {
	size_t len = strlen(pw) + SALT_LENGTH + 1;
	char *saltedPw;

	if ((saltedPw = malloc(len)) == NULL) {
		perror("malloc()");
		return NULL;
	}

	if (snprintf(saltedPw, len, "%s%s", pw, salt) < 0) {
		free(saltedPw);
		return NULL;
	}

	return saltedPw;
}

/**
 * @brief Registers a user in the database
 * @return uid on success, negative on failure
 */
int64_t register_user(const char *un, const char *pw) {
	int64_t uid = -1;
	char *pwSalt, *salt = get_salt();
	unsigned char pwHash[SHA256_DIGEST_LENGTH];
	sqlite3_stmt *stmt;
	const char *insertSql =
		"INSERT INTO users (username, password, salt) VALUES (?, ?, ?);";

	if (salt == NULL)
		return -1;

	if ((pwSalt = get_salted_pw(pw, (const unsigned char *)salt)) == NULL) {
		free(salt);
		return -2;
	}

	SHA256((const unsigned char *)pwSalt, strlen(pwSalt), pwHash);

	if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, NULL) != SQLITE_OK) {
		fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
		uid = -3;
		goto cleanup;
	}

	sqlite3_bind_text(stmt, 1, un, -1, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, pwHash, SHA256_DIGEST_LENGTH, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, salt, -1, SQLITE_STATIC);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
		uid = -4;
		goto cleanup;
	}

	uid = sqlite3_last_insert_rowid(db);
cleanup:
	sqlite3_finalize(stmt);
	free(pwSalt);
	free(salt);
	return uid;
}

/**
 * @brief Verifies a user's credentials
 * @return uid on success, negative on failure
 */
int64_t verify_user(const char *username, const char *pw) {
	sqlite3_stmt *stmt;

	if (sqlite3_prepare_v2(db, PW_SELECT_SQL, -1, &stmt, NULL) != SQLITE_OK) {
		fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

	/* User not found or execution error */
	if (sqlite3_step(stmt) != SQLITE_ROW) {
		sqlite3_finalize(stmt);
		return -2;
	}

	const unsigned char *dbPwHash = sqlite3_column_blob(stmt, 0);
	const unsigned char *dbSalt	  = sqlite3_column_text(stmt, 1);
	const int64_t uid			  = sqlite3_column_int(stmt, 2);
	char *pwSalt				  = get_salted_pw(pw, dbSalt);

	if (pwSalt == NULL) {
		sqlite3_finalize(stmt);
		return -3;
	}

	unsigned char pwHash[SHA256_DIGEST_LENGTH];
	SHA256((const unsigned char *)pwSalt, strlen(pwSalt), pwHash);

	/* Password mismatch */
	if (memcmp(dbPwHash, pwHash, SHA256_DIGEST_LENGTH) != 0) {
		sqlite3_finalize(stmt);
		free(pwSalt);
		return -4;
	}

	sqlite3_finalize(stmt);
	free(pwSalt);
	return uid;
}
