
#define SERVER_IP "192.168.1.100"
#define SERVER_PORT 8080
#define SALT_LENGTH 32


void init_db();
void close_db();
int register_user(const char* username, const char* password);
int verify_user(const char*username, const char* password);
char* generate_salt();
void hash_sha256(const char *input, unsigned char output[SHA256_DIGEST_LENGTH]);
char* generate_salted_password(const char* password, const char* salt);

