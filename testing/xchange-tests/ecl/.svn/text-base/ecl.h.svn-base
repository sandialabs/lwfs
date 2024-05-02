#ifndef __ECL__H__
#define __ECL__H__
/*! \file */

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(FUNCPROTO) || defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#ifndef ARGS
#define ARGS(args) args
#endif
#else
#ifndef ARGS
#define ARGS(args) (/*args*/)
#endif
#endif


/*!
 * ecl_parse_context is the basic handle controlling generation of
 * subroutines and functions.  It is used in nearly all calls to setup
 * parameter profiles and other information and to generate the code.  Using
 * a separate handle rather than static variables to store this information
 * over the multiple calls needed for code generation ensures thread
 * safety. 
 */
typedef struct parse_struct *ecl_parse_context;

/*!
 * ecl_exec_context is used to provide an execution context for 
 * ECL-generated procedures.  The full set of uses for this have not been 
 * determined, but it will support execution emulation on platforms where 
 * we do not have dynamic code generation, extraction of static state from
 * functions, and the ability to pass information "through" a generated
 * function from the calling context and make it available to a subroutine
 * that might be called by the generated code.
 */
typedef struct ecl_exec_struct *ecl_exec_context;

/*!
 * sm_ref is a pointer to an internal ECL data structure representing a
 * semantic element of a program fragment (E.G. a parameter definition).
 */
typedef struct sm_struct *sm_ref;

#ifndef IOOffset
typedef struct _IOField {
    char *field_name;		/* Field name */
    char *field_type;		/* Representation type desired */
    int field_size;		/* Size in bytes of representation */
    int field_offset;		/* Offset from base to put field value */
} IOField, *IOFieldList;

/*!
 * A structure to hold Format Name / Field List associations.
 *
 *
 *  This is used to associate names with field lists.  Together these define 
 *  a structure that can be composed into larger structures.
 */
typedef struct _IOformat_list {
    /*! the name to be associated with this structure */
    char *format_name;
    /*! the PBIO-style list of fields within this structure */
    IOFieldList field_list;
}IOFormatRec, *IOFormatList;

#define IOOffset(p_type,field) \
	((int) (((char *) (&(((p_type)0)->field))) - ((char *) 0)))
#if defined(__STDC__) || defined(__ANSI_CPP__) || defined(_MSC_VER)
#define IOstr(s) #s
#else
#define IOstr(s) "s"
#endif
#define IOArrayDecl(type, size) IOstr(type[size])
#define IOArrayDecl2(type, size, size2) IOstr(type[size][size2])
#endif

/*!
  A structure to hold external entries.

  This is used to associate textual names with addresses.
*/
typedef struct extern_entry {
    /*! the textual name of the external entry */
    char *extern_name;
    /*! the address of the external entry */
    void *extern_value;
} ecl_extern_entry;

/*!
 * A list of ecl_extern_entry structures.  This is used to specify the
 addresses of external  functions or variables that a function may access.
*/
typedef ecl_extern_entry *ecl_extern_list;

/*! Create a handle to a new ecl_parse_context.
 *
 * This is generally the first call in dynamically generating a
 * subroutine/function.  The ecl_parse_context created here is used in all
 * future calls, setting up the parameter profile and generating the code.
 * \return Will return a new initialized ecl_parse_context unless there is
 * no available memory.
 */
extern ecl_parse_context new_ecl_parse_context(void);

/*!
 * Free a handle to an ecl_parse_context.
 *
 * \param context the ecl_parse_context to be free'd.
 * Calling this routine frees all memory associated with the parse context,
 * but not that of code that has been generated from this context.
 */
extern void ecl_free_parse_context ARGS((ecl_parse_context context));

/*!
 * Associate a set of "name, external address" pairs with a parse context 
 *
 *  These names and addresses are used to resolve (external references in
 *  generated code).  In addition to associating the addresses, the full
 *  prototypes of the functions or variables must be added to the parse
 *  context, usually with ecl_parse_for_context().
 *
 * \param context the ecl_parse_context to which the externs are associated.
 * \param externs the list of "name, external address" pairs to be
 *  associated.  This list should be terminated with a {NULL, 0} pair.
 */
extern void ecl_assoc_externs ARGS((ecl_parse_context context,
				    ecl_extern_list externs));

/*!
 * \brief This is used to establish the parameter profile and return type 
 * of the subroutine to be generated.
 *
 * \param decl  the textual declaration for the subroutine
 * \param context  the context for subroutine generation
 * 
 * 
 *  The 'decl' parameter is a C-style declaration, without a 
 *  terminating semicolon.  I.E. something like:
 * 	"int proc(double d, int *i)"
 *  
 */
extern void
ecl_subroutine_declaration(const char *decl, ecl_parse_context context);

/*!
 * \brief Make a structured type, represented by a PBIO-style IOFieldList,
 *  available in a particular context.
 *
 * \param name the name to be associated with the new structured type.
 * \param field_list the PBIO-style IOFieldList that describes the layout of
 *  the structure.
 * \param context the context in which the type is to be made available.
 */
extern void ecl_add_struct_type ARGS((const char *name, 
				      IOFieldList field_list, 
				      ecl_parse_context context));

/*!
 * ecl_code is a handle to the generated code.  In addition to the generated
 * code block, it holds external data pointers and supplemental information.
 */
typedef struct _ecl_code_struct {
    /*! the entry point of the generated function */
    void (*func)(void);
    /*! the memory block containing the code of the generated function */
    void *code_memory_block;
    /*! the static data block referenced by the generated function */
    void *data;
    /*! the size of the static data block referenced by the generated function */
    int static_size_required;
    /*! a pointer to the DRISC context used to generate the function \internal 
    */
    void *drisc_context;
} *ecl_code;

/*!
 * generate code for a function body in a given context
 *
 * \param code the string representing the function body.
 * \param context the context in which the function body is to be generated.
 */
ecl_code ecl_code_gen ARGS((char *code, ecl_parse_context context));

/*!
 * perform syntactical and semantic checking of a function body without
 * actually generating code.
 *
 * \param code the string representing the function body.
 * \param context the context in which the function body is to be checked.
 */
int ecl_code_verify ARGS((char *code, ecl_parse_context context));

/*!
 * Free all resources associated with the generated code associated with the
 * ecl_code handle.
 *
 * \param code the handle to the resources that will be free'd.
 */
extern void ecl_code_free ARGS((ecl_code code));

/*!
 * create an execution context associated with a code block
 *
 * \param code the handle to the code bloc
 * \return the created execution context
 */
extern ecl_exec_context ecl_create_exec_context ARGS((ecl_code code));

/*!
 * Free all resources associated with the generated code associated with the
 * ecl_exec_context handle.
 *
 * \param code the handle to the resources that will be free'd.
 */
extern void ecl_exec_context_free ARGS((ecl_exec_context ec));

/*!
 * Associate application-level data with an execution context.  This is
 * typically done so that it can be retrieved by a called subprocess.  The
 * key value is any 32-bit value, presumably chosen to minimize the
 * possibility of conflict.  The value that can be associated is a long.
 *
 * \param ec the ecl_exec_context value to which to associate the data
 * \param key the value that will serve as a key to retrieve the data
 * \param value the 'long' data that will be associated with the key
 */
extern void ecl_assoc_client_data(ecl_exec_context ec, int key, long value);

/*!
 * Retrieve application-level data with an execution context.  This is
 * typically done by a called subprocess to retrieve data that was available
 * to the broader application.  The key value is any 32-bit value,
 * presumably chosen to minimize the possibility of conflict.  The value
 * returned is one which had been associated via ecl_assoc_client_data().
 * If no value had been associated, -1 is returned. * 
 *
 * \param ec the ecl_exec_context value to which to associate the data
 * \param key the value that will serve as a key to retrieve the data
 * \return the 'long' data that was associated with the key
 */
extern long ecl_get_client_data(ecl_exec_context ec, int key);

/*!
 * \brief This parses a string to determine what external
 *  functions and variables are visible during code generation.
 *
 * The input string is of standard C syntax for external variable and
 * function declarations.  All previously registered types are available.
 * \param code The input string containing declarations.
 * \param context The parse context in which the declarations should be
 * visible.
*/
int ecl_parse_for_context ARGS((char *code, ecl_parse_context context));

/*!
 * Duplicate a handle to an ecl_parse_context.
 *
 * \param context the ecl_parse_context to be duplicated.
 */
extern ecl_parse_context ecl_copy_context ARGS((ecl_parse_context context));

/*!
 *  err_out_func_t is a function pointer type.   Functions matching this
 *  profile can be used as call-out handlers for ECL errors.
 *  \param client_data an uninspected value passed in from 
 *    ecl_set_error_func()
 *  \param string the textual representation of the error.
*/
typedef void (*err_out_func_t) ARGS((void *client_data, char *string));
/*!
 * ecl_set_error_func establishes a new error output routine for ECL.
 *
 * By default, errors are sent to stdout.  If the error output is set then
 * errors can be captured for other disposition.
 *  \param context the context in which errors are to be captured
 *  \param err_func the function to be called when errors occur
 */

void ecl_set_error_func ARGS((ecl_parse_context context, 
			      err_out_func_t err_func));
/*!
 * This will dump (to stdout) a disassembled version of the 
 * machine code that has been generated
 *  \param code the ecl_code handle containing the code to be dumped.
 */
void ecl_dump ARGS((ecl_code code));

/*!
 * This will generate rollback code for message morphing 
 *  \param format1 the new format. This is the format of the input message.
 *  \param format2 the old format. This is the format of the output message.
 *  \param xform_code  The ECL code string that transforms data from format1 to format2.
 */
extern ecl_code
gen_rollback_code ARGS((IOFormatList format1, IOFormatList format2, char *xform_code));

/*!
 *  \brief This is used to create a semantic reference to a
 *  structured type directly from a PBIO-style IOFieldList.
 *  
 *  This is used to generate the sm_ref that is then used with
 *  ecl_add_decl_to_parse_context() or ecl_build_param_node();
 *
 * \param name the name to be associated with the structured type
 * \param field_list the internal fields of the structured type, given as a
 *         PBIO IOFieldList
 *  
 *  \deprecated  Use ecl_add_struct_type()
 *  
*/
extern sm_ref ecl_build_type_node ARGS((const char *name, 
					IOFieldList field_list));

/*!
 *  \brief This is used to make a semantic
 *  reference to a  structured type available in a particular context.
 *  
 *  This is used with a sm_ref that is generated from ecl_build_type_node().
 *
 * \param id the name to be associated with the type.
 * \param node the sm_ref from ecl_build_type_node().
 * \param context the context in which the type is to be made available.
 *  
 *  \deprecated  Use ecl_add_struct_type()
 *  
*/
extern void ecl_add_decl_to_parse_context ARGS((const char *id, sm_ref node,
						ecl_parse_context context));

/*!
 * \brief This is used to declare a parameter to the routine
 * to be generated.  
 *
 * \param id the name to be associated with the parameter.
 * \param typ the sm_ref representing the structured type of the parameter.
 * \param param_num the index of this parameter (starting at zero).
 * 
 *  \deprecated  Use ecl_subroutine_declaration().
 *  
 */
extern sm_ref
ecl_build_param_node ARGS((const char *id, sm_ref typ, int param_num));

/*!
 * \brief Declare a parameter to the generated function.
 *
 * The parameter declaration consist of a textual id that will be used to
 * reference the parameter, a character type that is parsed to determine the
 * semantic type of the parameter and a parameter number (starting at zero)
 * that determines the position of the parameter.
 * \param id the name associated with the parameter.
 * \param typ the string that is parsed to specify the type of the
 *     parameter.
 * \param param_num the zero-based index of the parameter.
 * \param context the context in which the parameter will be available.
 *
 *  \deprecated  Use ecl_subroutine_declaration().
 */
extern void
ecl_add_param ARGS((const char *id, const char *typ, int param_num,
		    ecl_parse_context context));

/*!
 * \brief This establishes the return type for the
 * generated function. 
 *  \param typ A character string representing the return type.  This string
 *  is parsed and all acceptable E-Code types are valid.
 *  \param context The parse context affected.
 *
 *  \deprecated  Use ecl_subroutine_declaration().
 */

extern void
ecl_set_return_type ARGS((char *typ, ecl_parse_context context));

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
