#include <openssl/sha.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/auth.h"

static sqlite3 *db;
const char alphanum[] = "0123456789"
                        "!@#$%^&*"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "abcdefghijklmnopqrstuvwxyz";

char *generate_salt() {
    char *salt; 
    if ((salt = malloc(SALT_LENGTH + 1) )== NULL) {
        fprintf(stderr, "malloc()\n");
        return NULL;
    }

    srand(time(NULL));

    for (int i = 0; i < SALT_LENGTH; i++) {
        int index = rand() % (sizeof(alphanum) - 1);
        salt[i] = alphanum[index];
    }
    salt[SALT_LENGTH] = '\0';

    return salt;
}

void hash_sha256(const char *input, unsigned char output[SHA256_DIGEST_LENGTH]) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input, strlen(input));
    SHA256_Final(output, &sha256);
}

void init_db() {
    int rc;
    rc = sqlite3_open("auth.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    } else {
        printf("Database opened successfully.\n");
    }

    const char *sql = "CREATE TABLE IF NOT EXISTS users ("
                      "userId INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "username TEXT UNIQUE NOT NULL, "
                      "password BLOB NOT NULL, "
                      "salt TEXT NOT NULL);";
    char *err_msg = 0;
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        printf("Table 'users' created or already exists.\n");
    }
}

void close_db() {
    int rc = sqlite3_close(db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't close database: %s\n", sqlite3_errmsg(db));
    }
}

char *generate_salted_password(const char *password, const char *salt) {
    size_t len = strlen(password) + SALT_LENGTH + 1;
    char *salted_password; 


    if ((salted_password = malloc(len) )== NULL) {
        fprintf(stderr, "malloc()\n");
        return NULL;
    }

    strcpy(salted_password, password);
    strcat(salted_password, salt);
    return salted_password;
}

int register_user(const char *username, const char *password) {
    char *salt = generate_salt();
    unsigned char hash[SHA256_DIGEST_LENGTH];

    if (salt == NULL) {
        return -1; 
    }

    char *salted_password = generate_salted_password(password, salt);
    if (salted_password == NULL) {
        free(salt); 
        return -2; 
    }

    hash_sha256(salted_password, hash);

    
    const char *insert_sql = "INSERT INTO users (username, password, salt) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        free(salted_password); 
        free(salt); 
        return -3;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, hash, SHA256_DIGEST_LENGTH, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, salt, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        free(salted_password); 
        free(salt); 
        return -4;
    }

    sqlite3_finalize(stmt); 
    free(salted_password);   
    free(salt);              
    return 0; // Registration successful
}

int verify_user(const char *username, const char *password) {
    const char *select_sql = "SELECT password, salt FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, select_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return -2; // User not found or execution error
    }

    const unsigned char *stored_hashed_password = sqlite3_column_blob(stmt, 0);
    const char *stored_salt = (const char *)sqlite3_column_text(stmt, 1);

    char *salted_password = generate_salted_password(password, stored_salt);
    if (salted_password == NULL) {
        sqlite3_finalize(stmt);
        return -3; 
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    hash_sha256(salted_password, hash);

    if (memcmp(stored_hashed_password, hash, SHA256_DIGEST_LENGTH) != 0) {
        sqlite3_finalize(stmt); 
        free(salted_password);   
        return -4; 
    }

    sqlite3_finalize(stmt); 
    free(salted_password);   
    return 0; 
}
