#include "kdrisc.h"

char **__environ = NULL;

char *
__findenv(name, offset)
	register const char *name;
	int *offset;
{
	register int len;
	register char **P, *C;
	register const char *tmp;

	for (tmp = name, len = 0; *tmp && *tmp != '='; ++tmp, ++len);

     	if (__environ == NULL) 
		 return NULL;

	for (P = __environ; *P; ++P)
		if (!strncmp(*P, name, len))
			if (*(C = *P + len) == '=') {
				*offset = P - __environ;
				return(++C);
			}

	return(NULL);
}

/* Return the value of the environment variable NAME.  */
char *
getenv (name)
     const char *name;
{
  const size_t len = strlen (name);
  char **ep;

  if (__environ == NULL)
    return NULL;

  for (ep = __environ; *ep != NULL; ++ep) 
    if (!strncmp (*ep, name, len) && (*ep)[len] == '=')
      return &(*ep)[len + 1];

  return NULL;
}

/*
 * setenv --
 *	Set the value of the environmental variable "name" to be
 *	"value".  If rewrite is set, replace any current value.
 */
int
setenv(name, value, rewrite)
	register const char *name;
	register const char *value;
	int rewrite;
{

	static int alloced;			
	register char *C;
	int l_value, offset;
	char *__findenv();

	down(&kdrisc_sem);
	if (*value == '=')		
		++value;
	l_value = strlen(value);

	if ((C = __findenv(name, &offset))) {
		if (!rewrite) {
			up(&kdrisc_sem);
			return (0);
		}
		if (strlen(C) >= l_value) {	
			while ((*C++ = *value++));
			up(&kdrisc_sem);
			return (0);
		}
	} else {				
		register int	cnt = 0;
		register char	**P;

		if (__environ != NULL) 
		    for (P = __environ, cnt = 0; *P; ++P, ++cnt);

		if (alloced) {		
			__environ = (char **)DReallocMM((char *)__environ,
			    (size_t)(sizeof(char *) * (cnt + 2)));
			if (!__environ) {
				up(&kdrisc_sem);
				return (-1);
			}
		}
		else {		
			alloced = 1;	
			P = (char **)DAllocMM((size_t)(sizeof(char *) *
			    (cnt + 2)));
			if (!P) {
				up(&kdrisc_sem);
				return (-1);
			}
			memcpy(__environ, P, cnt * sizeof(char *));
			__environ = P;
		}
		__environ[cnt + 1] = NULL;
		offset = cnt;
	}

	for (C = (char *)name; *C && *C != '='; ++C);	
	if (!(__environ[offset] =			
	    DAllocMM((size_t)((int)(C - name) + l_value + 2))))	{
		up(&kdrisc_sem);
		return (-1);
	}
	for (C = __environ[offset]; (*C = *name++) && *C != '='; ++C)
		;
	for (*C++ = '='; (*C++ = *value++); )
		;
	up(&kdrisc_sem);
	return (0);
}

