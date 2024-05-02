
/* generate a DCE 1.1 v1 UUID from system environment */
char *uuid_v1(void)
{
    uuid_t *uuid;
    char *str;

    uuid_create(&uuid);
    uuid_make(uuid, UUID_MAKE_V1);
    str = NULL;
    uuid_export(uuid, UUID_FMT_STR, &str, NULL);
    uuid_destroy(uuid);
    return str;
}


int main(int *argc, char *argv[])
