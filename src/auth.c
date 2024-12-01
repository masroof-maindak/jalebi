#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sqlite3.h>

#include "../include/auth.h"
#include "../include/utils.h"

static sqlite3 *db;

int init_db() {
	int rc;
	if ((rc = sqlite3_open(DATABASE_PATH, &db))) {
		fprintf(stderr, "sqlite3_error() in init_db(): %s\n",
				sqlite3_errmsg(db));
		return -1;
	}

	char *err = NULL;
	rc		  = sqlite3_exec(db, INIT_SQL, 0, 0, &err);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "sqlite3_exec() in init_db(): %s\n", err);
		sqlite3_free(err);
		return -2;
	}

	return 0;
}

bool close_db() {
	int rc;
	if ((rc = sqlite3_close(db)) != SQLITE_OK) {
		fprintf(stderr, "sqlite3_close() in close_db(): %s\n",
				sqlite3_errmsg(db));
		return false;
	}
	return true;
}

/**
 * @brief Generates a random salt
 */
void generate_rand_salt(char salt[SALT_LENGTH + 1]) {
	srand(time(NULL));
	for (int i = 0; i < SALT_LENGTH; i++)
		salt[i] = ALPHA_NUMERIC[rand() % (sizeof(ALPHA_NUMERIC) - 1)];
	salt[SALT_LENGTH] = '\0';
}

bool conc_salt_and_pw(char pwSalt[PW_MAX_LEN + SALT_LENGTH + 1],
					  const char pw[PW_MAX_LEN],
					  const unsigned char salt[SALT_LENGTH]) {
	return snprintf(pwSalt, PW_MAX_LEN + SALT_LENGTH + 1, "%s%s", pw, salt) < 0
			   ? false
			   : true;
}

/**
 * @brief Registers a user in the database
 * @return uid on success, negative on failure
 */
int64_t register_user(const char *un, const char *pw) {
	int64_t uid = -1;
	char pwSalt[PW_MAX_LEN + SALT_LENGTH + 1], salt[SALT_LENGTH + 1];
	unsigned char pwHash[SHA256_DIGEST_LENGTH];
	sqlite3_stmt *stmt;

	generate_rand_salt(salt);
	conc_salt_and_pw(pwSalt, pw, (const unsigned char *)salt);
	SHA256((const unsigned char *)pwSalt, strlen(pwSalt), pwHash);

	if (sqlite3_prepare_v2(db, INSERT_USER_SQL, -1, &stmt, NULL) != SQLITE_OK) {
		fprintf(stderr, "sqlite3_prepare_v2() in register_user(): %s\n",
				sqlite3_errmsg(db));
		uid = -2;
		goto cleanup;
	}

	sqlite3_bind_text(stmt, 1, un, -1, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, pwHash, SHA256_DIGEST_LENGTH, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, salt, -1, SQLITE_STATIC);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		fprintf(stderr, "sqlite3_step() in register_user(): %s\n",
				sqlite3_errmsg(db));
		uid = -3;
		goto cleanup;
	}

	uid = sqlite3_last_insert_rowid(db);
cleanup:
	sqlite3_finalize(stmt);
	return uid;
}

/**
 * @brief Verifies a user's credentials
 * @return uid on success, negative on failure
 */
int64_t verify_user(const char *username, const char *pw) {
	sqlite3_stmt *stmt;
	char pwSalt[PW_MAX_LEN + SALT_LENGTH + 1];

	if (sqlite3_prepare_v2(db, PW_SELECT_SQL, -1, &stmt, NULL) != SQLITE_OK) {
		fprintf(stderr, "sqlite3_prepare_v2() in verify_user(): %s\n",
				sqlite3_errmsg(db));
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

	if (!conc_salt_and_pw(pwSalt, pw, dbSalt)) {
		sqlite3_finalize(stmt);
		return -3;
	}

	unsigned char pwHash[SHA256_DIGEST_LENGTH];
	SHA256((const unsigned char *)pwSalt, strlen(pwSalt), pwHash);

	/* Password mismatch */
	if (memcmp(dbPwHash, pwHash, SHA256_DIGEST_LENGTH) != 0) {
		sqlite3_finalize(stmt);
		return -4;
	}

	sqlite3_finalize(stmt);
	return uid;
}
