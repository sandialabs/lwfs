#include <sys/types.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"

#define	DATABASE "access.db"

struct db_key {
	char name[64]; 
	int id;
};

struct db_data {
	char data[128]; 
};

int
main()
{
    DB *dbp;
    DBC *dbc; 
    DBT key, data;
    int ret=0, t_ret;
    struct db_key key1; 
    struct db_data data1; 
    struct db_data data2; 
    struct db_data result; 

    if (access(DATABASE, F_OK) == 0) {
	remove(DATABASE);
    }

    /* Create the database handle and open the underlying database. */
    if ((ret = db_create(&dbp, NULL, 0)) != 0) {
	fprintf(stderr, "db_create: %s\n", db_strerror(ret));
	exit (1);
    }

    /* this database allows duplicates */
    if ((ret = dbp->set_flags(dbp, DB_DUP | DB_DUPSORT)) != 0) {
	dbp->err(dbp, ret, "%s", DATABASE);
	goto err;
    }

    if ((ret = dbp->open(dbp, NULL, DATABASE, NULL, DB_HASH, DB_CREATE, 0664)) != 0) {
	dbp->err(dbp, ret, "%s", DATABASE);
	goto err;
    }

    /* initialize the key and data types */
    memset(&key1, 0, sizeof(struct db_key));
    sprintf(key1.name, "ron's key");
    key1.id = 1;

    memset(&data1, 0, sizeof(struct db_data));
    sprintf(data1.data, "I am Sam...");

    memset(&data2, 0, sizeof(struct db_data));
    sprintf(data2.data, "Sam I am");


    /* Initialize key/data structures. */
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = &key1;
    key.size = sizeof(key1); 
    data.data = &data1;
    data.size = sizeof(data1);

    /* Store the first data item */
    if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) == 0)
	printf("db: %s: key stored.\n", (char *)key.data);
    else {
	dbp->err(dbp, ret, "DB->put");
	goto err;
    }

    /* Initialize key/data structures. */
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = &key1;
    key.size = sizeof(key1); 
    data.data = &data2;
    data.size = sizeof(data2);

    /* Store the second data item */
    if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) == 0)
	printf("db: %s: key stored.\n", (char *)key.data);
    else {
	dbp->err(dbp, ret, "DB->put");
	goto err;
    }




    /* get a cursor for the database */
    if ((ret = dbp->cursor(dbp, NULL, &dbc, 0)) != 0) {
	dbp->err(dbp, ret, "DB->cursor");
	goto err;
    }

    /* initialize the key */
    memset(&key, 0, sizeof(DBT));
    key.data = &key1; 
    key.size = sizeof(key1);

    /* initialize the data */
    memset(&result, 0, sizeof(result));
    memset(&data, 0, sizeof(DBT));
    data.data = &result;
    data.ulen = sizeof(result);
    data.flags = DB_DBT_USERMEM;

    while ((ret = dbc->c_get(dbc, &key, &data, DB_NEXT)) == 0) {

	/* print result */
	fprintf(stdout, "found \"%s\"\n", result.data);

	/* reset the data */
	memset(&result, 0, sizeof(result));
	data.data = &result; 
    }

    if (ret != DB_NOTFOUND) {
	dbp->err(dbp, ret, "DBC->c_get");
    }
    ret = 0;

    dbc->c_close(dbc);


err:	
    if ((t_ret = dbp->close(dbp, 0)) != 0 && ret == 0) {
	fprintf(stderr, "could not close dbp, rc=%d\n",t_ret);
	ret = t_ret;
    }

    exit(ret);
}
