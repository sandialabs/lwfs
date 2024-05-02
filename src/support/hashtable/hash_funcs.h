/*
  *********************************************************************
  *                                                                   *
  *                  General Hash Functions Library                   *
  * Author: Arash Partow - 2002                                       *
  * URL: http://www.partow.net                                        *
  *                                                                   *
  * Copyright Notice:                                                 *
  * Free use of this library is permitted under the guidelines and    *
  * in accordance with the most current version of the Common Public  *
  * License.                                                          *
  * http://www.opensource.org/licenses/cpl.php                        *
  *                                                                   *
  *********************************************************************
*/


#ifndef INCLUDE_GENERALHASHFUNCTION_H
#define INCLUDE_GENERALHASHFUNCTION_H


#include <stdio.h>


typedef unsigned int (*HashFunction)(char*);


unsigned int RSHash(char* str, unsigned int len);
unsigned int JSHash(char* str, unsigned int len);
unsigned int PJWHash(char* str, unsigned int len);
unsigned int ELFHash(char* str, unsigned int len);
unsigned int BKDRHash(char* str, unsigned int len);
unsigned int SDBMHash(char* str, unsigned int len);
unsigned int DJBHash(char* str, unsigned int len);
unsigned int APHash(char* str, unsigned int len);


#endif
