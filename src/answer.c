#include <stdlib.h>

#include "../include/answer.h"

#include <uuid/uuid.h>

/**
 * @brief allocates and creates a 36 byte uuid; must be freed.
 * TODO: CHECK
 */
char *generate_uuid_string() {
	uuid_t binuuid;
	uuid_generate_random(binuuid);

	char *uuid = malloc(UUID_STR_LEN);
	if (uuid == NULL)
		return NULL;

	uuid_unparse(binuuid, uuid);
	return uuid;
}
