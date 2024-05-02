/*
 *************************************************************************
 *                                                                       *
 *                  General Hash Functions Hash Test                     *
 *                                                                       *
 * Author: Arash Partow - 2002                                           *
 * URL: http://www.partow.net                                            *
 *                                                                       *
 * Copyright notice:                                                     *
 * Free use of the General Hash Functions Library is permitted under the *
 * guidelines and in accordance with the most current version of the     *
 * Common Public License.                                                *
 * http://www.opensource.org/licenses/cpl.php                            *
 *                                                                       *
 *************************************************************************
*/



#include <stdio.h>
#include <stdlib.h>
#include "hash_funcs.h"

int main(int argc, char* argv[])
{

   char* key = "abcdefghijklmnopqrstuvwxyz1234567890";

   printf("General Hash Function Test     \n");
   printf("By Arash Partow - 2002         \n");
   printf("Key:                         %s\n",key);
   printf("1. RS-Hash Function Value:   %u\n",RSHash(key,36));
   printf("2. JS-Hash Function Value:   %u\n",JSHash(key,36));
   printf("3. PJW-Hash Function Value:  %u\n",PJWHash(key,36));
   printf("4. ELF-Hash Function Value:  %u\n",ELFHash(key,36));
   printf("5. BKDR-Hash Function Value: %u\n",BKDRHash(key,36));
   printf("6. SDBM-Hash Function Value: %u\n",SDBMHash(key,36));
   printf("7. DJB-Hash Function Value:  %u\n",DJBHash(key,36));
   printf("8. AP-Hash Function Value:   %u\n",APHash(key,36));

   exit(EXIT_SUCCESS);
   return 1;

}
