
/*
 * Copyright (c) 2002, 2004, Christopher Clark
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 	* Redistributions of source code must retain the above copyright 
 *        notice, this list of conditions and the following disclaimer.
 *
 *	* Redistributions in binary form must reproduce the above 
 *        copyright notice, this list of conditions and the 
 *        following disclaimer in the documentation and/or other 
 *        materials provided with the distribution.
 *
 *	* Neither the name of the original author; nor the names of 
 *	  any contributors may be used to endorse or promote products derived
 *	  from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "hash_funcs.h"

unsigned int RSHash(char* str, unsigned int len)
{

   unsigned int b    = 378551;
   unsigned int a    = 63689;
   unsigned int hash = 0;
   unsigned int i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = hash*a+(*str);
      a = a*b;
   }

   return (hash & 0x7FFFFFFF);

}
/* End Of RS Hash Function */


unsigned int JSHash(char* str, unsigned int len)
{

   unsigned int hash = 1315423911;
   unsigned int i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash ^= ((hash << 5) + (*str) + (hash >> 2));
   }

   return (hash & 0x7FFFFFFF);

}
/* End Of JS Hash Function */


unsigned int PJWHash(char* str, unsigned int len)
{

   unsigned int BitsInUnignedInt = (unsigned int)(sizeof(unsigned int) * 8);
   unsigned int ThreeQuarters    = (unsigned int)((BitsInUnignedInt  * 3) / 4);
   unsigned int OneEighth        = (unsigned int)(BitsInUnignedInt / 8);
   unsigned int HighBits         = (unsigned int)(0xFFFFFFFF) << (BitsInUnignedInt - OneEighth);
   unsigned int hash             = 0;
   unsigned int test             = 0;
   unsigned int i                = 0;

   for(i = 0; i < len; str++, i++)
   {

      hash = (hash << OneEighth) + (*str);

      if((test = hash & HighBits)  != 0)
      {

         hash = (( hash ^ (test >> ThreeQuarters)) & (~HighBits));

      }

   }

 return (hash & 0x7FFFFFFF);

}
/* End Of  P. J. Weinberger Hash Function */


unsigned int ELFHash(char* str, unsigned int len)
{

   unsigned int hash = 0;
   unsigned int x    = 0;
   unsigned int i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = (hash << 4) + (*str);
      if((x = hash & 0xF0000000L) != 0)
      {
         hash ^= (x >> 24);
         hash &= ~x;
      }
   }

   return (hash & 0x7FFFFFFF);

}
/* End Of ELF Hash Function */


unsigned int BKDRHash(char* str, unsigned int len)
{

   unsigned int seed = 131; /* 31 131 1313 13131 131313 etc.. */
   unsigned int hash = 0;
   unsigned int i    = 0;

   for(i = 0; i < len; str++, i++)
   {

      hash = (hash*seed)+(*str);

   }

   return (hash & 0x7FFFFFFF);

}
/* End Of BKDR Hash Function */


unsigned int SDBMHash(char* str, unsigned int len)
{

   unsigned int hash = 0;
   unsigned int i    = 0;

   for(i = 0; i < len; str++, i++)
   {

      hash = (*str) + (hash << 6) + (hash << 16) - hash;

   }

   return (hash & 0x7FFFFFFF);

}
/* End Of SDBM Hash Function */


unsigned int DJBHash(char* str, unsigned int len)
{

   unsigned int hash = 5381;
   unsigned int i    = 0;

   for(i = 0; i < len; str++, i++)
   {

      hash = ((hash << 5) + hash) + (*str);

   }

   return (hash & 0x7FFFFFFF);


}
/* End Of DJB Hash Function */


unsigned int APHash(char* str, unsigned int len)
{

   unsigned int hash = 0;
   unsigned int i    = 0;

   for(i = 0; i < len; str++, i++)
   {

      if ((i & 1) == 0)
      {
         hash ^=((hash << 7)^(*str)^(hash >> 3));
      }
      else
      {
         hash ^= (~((hash << 11)^(*str)^(hash >> 5)));
      }

   }

   return (hash & 0x7FFFFFFF);


}
/* End Of AP Hash Function */
