/*
 * $Id: xml.cc,v 1.35 2005/10/17 20:48:53 lofstead Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <set>
#include <vector>
#include <list>
#include <queue>

#include <libxml/parser.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

#include "io.h"
#include "echo.h"

#include "xmit.h"
#include "xmit_internal.h"

#include "types.h"
#include "xml.h"

#include "operand.h"

/*
 * When generating the ECL, it is necessary to discover the vars used in the
 * elements and their bounds.  This will allow proper generation of the looping
 * constructs.  This data structure is used to support that operation.
 */
struct var
{
    char name;          // The name of the var
    unsigned int lower; // since array bounds, must be non-negative to be valid
    unsigned int upper; // since array bounds, must be non-negative to be valid
};

// see the function body below to get the description of these forward decls.
vector <var> getVarsAndLimits (const element * elements
                              ,const string & rootName
                              ,const IOContext & ioc
                              );
operand * parseOperand (xmlNodePtr node);

/*
 * To create a dervived typed channel, do the following:
 * 1. Open the source channel
 * 2. Create a derived channel calling EChannel_typed_derive_data
 *    using all of the destination types for the parameters but
 *    the ECglobal_id (src_channel) as the second parameter.
 * 3. ECsink_typed_subscribe to the dst channel returned from #2
 * 4. The handler should process the dst data types
 */
static EChannel src_channel;
static EChannel dst_channel;
static ECSinkHandle handle;
static data_struct d;

// please refer to xml.h for a description and usage information
/*  startFeed will have to be reworked to be in accordance with the EVPath way of
    doing things.  Also needs to take into account the way to setup the
    channels (client back to source).

    Much of the startup (but not communications) has been created in server, client
    and inode files.

ECSinkHandle startFeed (CManager cm
                       ,EControlContext cc
                       ,char * server
                       ,char * filepath
                       ,char * xml
                       ,ECTypedHandlerFunction handler
                       ,void * client_data
                       )
{
    Transform t = parseXMLfile (xml);
    d = generateDataStruct (t.dst_ioc, t.dst_root);

    struct sockaddr_in sa;   // socket information
    struct hostent * hp;     // host information
    int s;                   // the outgoing socket
    int transmitted;         // the number of bytes transmitted
    int done = 0;            // are we done receiving data
    char buf [BUF_LEN];      // transmission/receive buffer
    int buf_len;             // buffer length
    char * buf_ptr;          // used to advance the pointer during reads

    // get the IP address of the host
    hp = gethostbyname (server);
    if (hp == NULL)
    {
        fprintf (stderr, "could not find host %s\n", server);
        exit (-1);
    }

    // get a socket through which to connect to the server
    s = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
    {
        fprintf (stderr, "could not get a socket\n");
        exit (-1);
    }

    // setup the connection information
    sa.sin_family = AF_INET;
    sa.sin_port = htons (PORT);
    memcpy (&sa.sin_addr, hp->h_addr, hp->h_length);

    // connect to the server
    if (connect (s, (struct sockaddr *) &sa, sizeof (sa)) != 0)
    {
        fprintf (stderr, "could not connect to the server\n");
        close (s);
        exit (-1);
    }

    string xmlfile;
    FILE * f = fopen (xml, "rb");
    long size = 0;
    int read_size;
    while (!feof (f))
    {
        read_size = fread (buf, 1, BUF_LEN, f);
        size += read_size;
        xmlfile.append (buf, read_size);
    }

    sprintf (buf, "%s\n%ld\n%s", filepath, size, xmlfile.c_str ());
    buf_len = strlen (buf);
    // send request to the server
    transmitted = send (s, buf, buf_len, 0);
    if (transmitted != buf_len)
    {
        fprintf (stderr, "error sending buffer to server.  "
                         "Should be %d, only sent %d\n", buf_len, transmitted);
        close (s);
        exit (-1);
    }

    // prepare to read the response
    memset (buf, 0, BUF_LEN);
    buf_len = BUF_LEN;
    buf_ptr = buf;

    // read until we get two newlines
    while (!done)
    {
        // receive bytes
        transmitted = recv (s, buf_ptr, buf_len, 0);
        if (transmitted == -1)
        {
            fprintf (stderr, "error during receive from server\n");
            close (s);
            exit (-1);
        }

        // validate the number of newlines read so far
        if (count_newlines (buf) >= 2)
        {
            done = 1;
            buf [strlen (buf) - 2] = '\0';
        }
        else  // move the start of the buffer if we haven't read two yet
        {
            buf_ptr += transmitted;
            buf_len -= transmitted;
        }
    }

    src_channel = EChannel_open (cc, buf);
    dst_channel = EChannel_typed_derive_data (cc
                                             ,ECglobal_id (src_channel)
                                             ,(char *) t.filter.c_str ()
                                             ,d.data_field_list
                                             ,d.data_subformat_list
                                             ,&d
                                             );

    sprintf (buf, "OK\n\n");
    buf_len = strlen (buf);
    // send request to the server
    transmitted = send (s, buf, buf_len, 0);
    if (transmitted != buf_len)
    {
        fprintf (stderr, "error sending buffer to server.  "
                         "Should be %d, only sent %d\n", buf_len, transmitted);
        close (s);
        exit (-1);
    }
    close (s);
    handle = ECsink_typed_subscribe (dst_channel
                                    ,d.data_field_list
                                    ,d.data_subformat_list
                                    ,handler
                                    ,NULL
                                    );

    return handle;
}
*/

/******************************************************************************/
// for descriptions of these functions, refer to the function bodies.
string generateECL (const IOContext & src, const string & src_root
                   ,const IOContext & dst, const string & dst_root
                   ,vector <TypedTransform> & op
                   );
Transform parseXML (const char * xml);

/* Read an XML file parsing into the schema and transform pieces.
 * Returns either the root node pointer or NULL if there is a problem reading.
 */
xmlDocPtr readXML (char * pathname, clientRequest & t)
{
    xmlDocPtr doc = NULL;
    xmlNodePtr node = NULL;
    xmlNodePtr root = NULL;
    xmlNodePtr transformNode = NULL;
    xmlNodePtr rootTypes = NULL;

    doc = xmlParseFile (pathname);

    if (!doc)
    {
        return NULL;
    }

    root = xmlDocGetRootElement (doc);
    
    if (xmlStrcmp (root->name, (const xmlChar *) "transformation"))
    {
        fprintf (stderr, "document root element must be transformation: %s\n", (const char *) root->name);

        xmlFreeDoc (doc);

        return NULL;
    }

    for (node = root->children; node; node = node->next)
    {
        if (!xmlStrcmp (node->name, (const xmlChar *) "schema"))
        {
            if (t.src_schema == NULL)
                t.src_schema = node;
            else
                if (t.dst_schema == NULL)
                    t.dst_schema = node;
        }
        else
        {
            if (!xmlStrcmp (node->name, (const xmlChar *) "transform"))
            {
                transformNode = node;
            }
            else
            {
                if (!xmlStrcmp (node->name, (const xmlChar *) "rootformats"))
                {
                    rootTypes = node;
                }
            }
        }
    }

    if (!transformNode || t.src_schema == NULL || t.dst_schema == NULL)
    {
        fprintf (stderr, "source format, destination format, and transform entries must be provided\n");

        xmlFreeDoc (doc);

        return NULL;
    }

    readRootFormats (rootTypes, t.src_root, t.dst_root);
    readTransform (transformNode, t.transforms);

    return doc;
}

// parse an XML file into a Transform structure
Transform parseXMLfile (const char * pathname)
{
    return parseXML (pathname);
}

/*
 * Transform a string containing an xml file into a Transform structure. 
 *
 * While the amount of work to process an in memory XML file may seem extreme,
 * it is actually the smallest amount of work that would get around the bugs in
 * the system.  Here are the problems that forced these measures:
 * 1. parseXMLMemory call from libxml causes some memory faults in my generated
 *    objects.  The identical file parsed from a file doesn't work if parsed
 *    from memory.
 * 2. mktemp, for some unknown reason, core dumps whenever I attempt to use a
 *    stack-based allocation of the string.
 */
Transform parseXMLdata (const char * xml, int size)
{
    char * t = "/tmp/jlXXXXXX";
    char * temp = (char *) malloc (strlen (t) + 1);
    strcpy (temp, t);
    temp = mktemp (temp);

    FILE * f = fopen (temp, "wb");
    if (f != NULL)
    {
        fwrite (xml, 1, size, f);
        fclose (f);

        Transform t = parseXML (temp);

        unlink (temp);
        free (temp);

        return t;
    }

    free (temp);

    Transform tx;

    return tx;
}

/*
 * Parse an xml file into the Transform structure.  This looks for proper
 * structure and then generates the proper pieces based on the contents.  This
 * is where the XMIT library calls are made to generate the type information.
 * The xml parsing is done using libxml.  Please refer to that documentation
 * for more information about the idiosynchracies of that library.
 */
Transform parseXML (const char * xml)
{
    Transform t;                    // the transform to return
    xmlDocPtr doc = NULL;           // the parsed xml document
    xmlNodePtr node = NULL;         // the temp pointer to elements in xml
    vector <xmlNodePtr> schemas;    // the discovered schemas in the xml
    xmlNodePtr transformNode = NULL;// the singular node of transformation ops
    xmlNodePtr rootTypes = NULL;    // the singular node identifying root types

    vector <operation *> transform; // parsed transformation

    xmlInitParser ();

    doc = xmlParseFile (xml);

    if (!doc)
    {
        fprintf (stderr, "problem parsing doc\n");

        xmlCleanupParser ();

        return t;
    }

    node = xmlDocGetRootElement (doc);

    if (xmlStrcmp (node->name, (const xmlChar *) "transformation"))
    {
        fprintf (stderr, "document root element must be transformation: %s\n"
                ,(const char *) node->name
                );

        xmlFreeDoc (doc);
        xmlCleanupParser ();

        return t;
    }

    for (node = node->xmlChildrenNode; node; node = node->next)
    {
        if (!xmlStrcmp (node->name, (const xmlChar *) "schema"))
        {
            schemas.push_back (node);
        }
        else
        {
            if (!xmlStrcmp (node->name, (const xmlChar *) "transform"))
            {
                transformNode = node;
            }
            else
            {
                if (!xmlStrcmp (node->name, (const xmlChar *) "rootformats"))
                {
                    rootTypes = node;
                }
            }
        }
    }

    if (!transformNode || schemas.size () < 2)
    {
        fprintf (stderr, "source format, destination format, and transform"
                         " entries must be provided\n"
                );

        xmlFreeDoc (doc);
        xmlCleanupParser ();

        return t;
    }

    t.src_ioc = create_IOcontext (); // input
    t.dst_ioc = create_IOcontext (); // output

    // parse the input schemas
    pbio_schema_converter source (t.src_ioc);
    source.add_types (schemas.front ());
    schemas.erase (schemas.begin ());

    // parse the output schemas
    pbio_schema_converter destination (t.dst_ioc);
    xmlNodePtr dst_schema = schemas.front ();
    destination.add_types (schemas.front ());
    schemas.erase (schemas.begin ());

    // parse the root data type node
    readRootFormats (rootTypes, t.src_root, t.dst_root);

    // parse the transform node
    readTransform (transformNode, transform);

    vector <TypedTransform> transforms;
    TypedTransform tt (NULL, dst_schema, t.dst_root);
    tt.t = *(transform.begin ());
    operation * o = tt.t;
    for (vector <operation *>::iterator iter = transform.begin (); iter != transform.end (); iter++)
    {
        if (iter != transform.begin ())
        {
            o->next = *iter;
            o = o->next;
        }
    }
    transforms.push_back (tt);

    // generate the ECL from the parsed transform.  This may ultimately be
    // better done elsewhere from a raw transform returned in the Transform
    // struct.  This will especially be true in the future when different
    // target languages are available.
    t.filter = generateECL (t.src_ioc, t.src_root
                           ,t.dst_ioc, t.dst_root
                           ,transforms
                           );

    xmlFreeDoc (doc);
    xmlCleanupParser ();

    return t;
}

/*
 * Parse the root types node to determine which of the supplied types provided
 * for each of the source and destination formats is the root type.  Without
 * this information, it would not be possible to definitively tell which of the
 * specified types is the root type.  Presumably it would be the last one
 * specified, but that may not always be the case.  This was done here rather
 * than in the type specification itself to make it easier to use the XMIT
 * library for the type parsing.
 */
void readRootFormats (const xmlNodePtr rootTypes, string & src, string & dst)
{
    xmlNodePtr node = rootTypes->xmlChildrenNode;

    while (node)
    {
        if (!xmlStrcmp (node->name, (const xmlChar *) "dst"))
        {
            dst = (const char *) XML_GET_CONTENT (node->children);
        }
        if (!xmlStrcmp (node->name, (const xmlChar *) "src"))
        {
            src = (const char *) XML_GET_CONTENT (node->children);
        }

        node = node->next;
    }
}

/*
 * Given the type information (so we can generate the proper loop boundaries)
 * and the transformation operations, generate appropriate ECL code.  The
 * strategy is simple.  Go through each operand looking for the vars for
 * loop generation and then call the "toString ()" appropriately on each
 * side to generate the proper code.  There is also the code surrounding
 * this to track the nesting level to generate the proper number of closing
 * braces for the loops.  Also note the decls management described below.
 */
string generateECL (const IOContext & src, const string & src_root
                   ,const IOContext & dst, const string & dst_root
                   ,vector <TypedTransform> & operations
                   )
{
    ostringstream decls;    // since the list of declarations won't be
                            // known until after all of the operations
                            // are parsed, keep them separate to avoid
                            // duplicate decls and to make easier gen.
    ostringstream ops;      // The code generated for each of the ops.
    ostringstream ret;      // The composite string of decls and ops.
    set <char> decl_vars;   // Used to track vars for decl purposes.

    // 1. Search the dst of the operations for var usage.  Map those to
    //    the corresponding elements in the dst format to obtain bounds
    //    information.
    // 2. Generate the appropriate list of local vars based on these data
    // 3. Assign to output.x from input.y as dictated by the operations
    //    inserting loops and other constructs as appropriate

    for (vector <TypedTransform>::iterator ttiter = operations.begin (); ttiter != operations.end (); ttiter++)
    {
        TypedTransform & tt = *ttiter;
        operation * op = tt.t;
        while (op)
        {
        vector <var> vars = getVarsAndLimits (op->dst, tt.root_type, dst);
        int count = vars.size ();
        if (count)
        {
            while (vars.size () != 0)
            {
                var v = vars.front ();
                vars.erase (vars.begin ());
                decl_vars.insert (v.name);
                ops << "for (" << v.name << "=" << v.lower
                    << "; " << v.name << " < " << v.upper
                    << "; " << v.name << "++)\n{\n";

            }

            ops << op->dst->toString ("output.");
            ops << " = " << op->src->toString ("input.");
            ops << ";\n";

            for (;count > 0; count--)
            {
                ops << "}\n";
            }
        }
        else
        {
            ops << op->dst->toString ("output.");
            ops << " = " << op->src->toString ("input.");
            ops << ";";
            ops << "\n";
        }

        op = op->next;
        }
    }

    for (set <char>::iterator i = decl_vars.begin ()
        ;i != decl_vars.end ()
        ;i++
        )
    {
        decls << "int " << *i << ";\n";
    }

    ret << "{\n";

    ret << decls.str () << endl;

    ret << ops.str () << endl;

    ret << "\nreturn 1;\n";
    ret << "}\n";

    return ret.str ();
}

/*
 * This is really a relic of early testing, but is also useful for the testing
 * harness (xml_main.cc) to see what the generated ECL is like without having
 * to have both the client and server processes running.
 */
void printECL (const IOContext & src, const string & src_root
              ,const IOContext & dst, const string & dst_root
              ,vector <TypedTransform> & op
              )
{
    cout << generateECL (src, src_root, dst, dst_root, op);
}

/*
 * Support function to parse the element looking for the vars used in the
 * transform operand declaration and then through the type defintions to find
 * the range specifiers so that the proper looping bounds can be generated.
 * While the low range may seem a bit odd, it will be more useful once the
 * subset operation is introduced requiring a less than full view of the
 * data elements.
 */
vector <var> getVarsAndLimits (const element * elements
                              ,const string & rootName
                              ,const IOContext & ioc
                              )
{
    vector <var> vars;

    string name = rootName;
    while (elements != NULL)
    {
        IOFormat format = get_IOformat_by_name_IOcontext (ioc, name.c_str ());
        IOFieldList fl = field_list_of_IOformat (format);
        bool done = false;

        for (int i = 0; !done && fl [i].field_name != 0; i++)
        {
            if (!strcmp (fl [i].field_name, elements->name))
            {
                string type = fl [i].field_type;
                int subscript = type.find ("[");
                if (subscript != -1)
                {
                    string prefix = type.substr (0, subscript);
                    string suffix = type.substr (subscript + 1
                                                ,type.length () - subscript - 2
                                                );
                    var v;
                    v.name = elements->subscript [0];
                    v.lower = 0;
                    v.upper = atoi (suffix.c_str ());
                    vars.push_back (v);
                    elements = elements->subelement;
                    name = prefix;

                    done = true;
                }
                else // no subscript on this element, check subelements
                {
                    elements = elements->subelement;
                    name = type;

                    done = true;
                }
            }
        }

        if (!done)
        {
            elements = elements->subelement;
        }
    }

    return vars;
}

/*
 * The channel creation/subscription functions require this data structure
 * to describe the type information.  Rather than have the user have to
 * figure out the set of types (since the same type could be used more
 * than once in a type tree), this will generate the unique set of types
 * and the root type in the proper structure as needed by ECho.
 */
data_struct generateDataStruct (const IOContext & ioc, const string & root)
{
    data_struct d;
    vector <_CMformat_list> subformats;
    std::queue <IOFieldList, list <IOFieldList> > worklist;

    IOFormat format = get_IOformat_by_name_IOcontext (ioc, root.c_str ());
    d.data_field_list = field_list_of_IOformat (format);

    worklist.push (d.data_field_list);
    while (!worklist.empty ())
    {
        IOFieldList fl = worklist.front ();
        worklist.pop ();

        for (int i = 0; fl [i].field_name != 0; i++)
        {
            string type = fl [i].field_type;
            if (   type == "string"
                || type == "integer"
                || type == "double"
                || type == "float"
               )
            {
                continue;
            }
            else
            {
                int subscript = type.find ("[");
                string prefix = type;
                if (subscript != -1)
                {
                    prefix = type.substr (0, subscript);
                }

                if (   prefix != "string"
                    && prefix != "integer"
                    && prefix != "double"
                    && prefix != "float"
                   )
                {
                    bool found = false;
                    for (vector<_CMformat_list>::iterator i = subformats.begin()
                        ;found == false && i != subformats.end ()
                        ;i++
                        )
                    {
                        if (prefix == (*i).format_name)
                        {
                            found = true;
                        }
                    }
                    if (!found)
                    {
                        _CMformat_list cmfl;

                        IOFormat format = get_IOformat_by_name_IOcontext
                                                       (ioc, prefix.c_str ());
                        cmfl.format_name = strdup (prefix.c_str ());
                        cmfl.field_list = field_list_of_IOformat (format); 

                        subformats.push_back (cmfl);
                        worklist.push (cmfl.field_list);
                    }
                }
            }
        }
    }

    // build the list properly
    d.data_subformat_list = (CMFormatRec *) malloc
                                               (  (subformats.size () + 2)
                                                * sizeof (_CMformat_list)
                                               );
    memset (d.data_subformat_list, 0, (subformats.size () + 2) * sizeof (_CMformat_list));
    int j = 0;
    d.data_subformat_list [j].format_name = strdup (root.c_str ());
    d.data_subformat_list [j].field_list = field_list_of_IOformat (format);
    j++;
    for (vector <_CMformat_list>::reverse_iterator i = subformats.rbegin ()
        ;i != subformats.rend ()
        ; i++, j++
        )
    {
        d.data_subformat_list [j].format_name = (*i).format_name;
        d.data_subformat_list [j].field_list = (*i).field_list;
    }
    d.data_subformat_list [j].format_name = NULL;
    d.data_subformat_list [j].field_list = NULL;

    int size = 0;
    for (int i = 0; d.data_field_list [i].field_name != 0; i++)
    {
        size +=  d.data_field_list [i].field_size;
    }
    d.initial_value = malloc (size);
    memset (d.initial_value, 0, size);

    return d;
}

/*
 * Another relic that is still useful in the testing harness (xml_main.cc).
 * This will generate a nice output of a operation without any of the pesky
 * type information or prefix concerns.  It is simple and clean making
 * checking a little easier to do.
 */
void printTransform (vector <operation *> & op)
{
    if (op.size () == 0)
    {
        return;
    }

    for (vector <operation *>::iterator i = op.begin (); i != op.end (); i++)
    {
        operation * o = *i;
        cout << "dst: " << o->dst->toString () << endl;
        cout << "src: " << o->src->toString () << endl;
    }
}

/*
 * Parses an xml node tree into a math structure.  Since this is a recursive
 * function, we need to keep the return type as the math type rather than
 * operand.  The first part looks for the type of operation and the second
 * part parses the actual operands to the operation.  In the future, this may
 * have to be tweaked a little to include some more advanced operators.
 */
struct math * parseApply (xmlNodePtr node)
{
    char * op =  NULL;
    int first = 1;
    struct math * t = new math ();
    struct math * ret = t;

    t->token = strdup ("(");

    node = node->xmlChildrenNode;
    while (   node
           && (   !xmlStrcmp (node->name, (const xmlChar *) "text")
               || !xmlStrcmp (node->name, (const xmlChar *) "comment")
              )
          )
    {
        node = node->next;
    }

    if (node)
    {
        if (!xmlStrcmp (node->name, (const xmlChar *) "eq"))
        {
            op = " = ";
        }
        else
        {
            if (!xmlStrcmp (node->name, (const xmlChar *) "plus"))
            {
                op = " + ";
            }
            else
            {
                if (!xmlStrcmp (node->name, (const xmlChar *) "minus"))
                {
                    op = " - ";
                }
                else
                {
                    if (!xmlStrcmp (node->name, (const xmlChar *) "times"))
                    {
                        op = " * ";
                    }
                    else
                    {
                        if (!xmlStrcmp (node->name, (const xmlChar *) "divide"))
                        {
                            op = " / ";
                        }
                        else
                        {
                            fprintf (stderr
                                    ,"unknown operation: %s\n"
                                    ,node->name
                                    );

                            op = " + ";
                        }
                    }
                }
            }
        }
    }

    while (node)
    {
        node = node->next;
        while (   node
               && (   !xmlStrcmp (node->name, (const xmlChar *) "text")
                   || !xmlStrcmp (node->name, (const xmlChar *) "comment")
                  )
              )
        {
            node = node->next;
        }

        if (node)
        {
            if (!xmlStrcmp (node->name, (const xmlChar *) "apply"))
            {
                struct math * b = parseApply (node);

                if (!first)
                {
                    t->next = new math ();
                    t = t->next;
                    t->token = strdup (op);
                }
                first = 0;
                t->next = b;
                while (t->next)
                {
                    t = t->next;
                }
            }
            if (!xmlStrcmp (node->name, (const xmlChar *) "ci"))
            {
                if (!first)
                {
                    t->next = new math ();
                    t = t->next;
                    t->token = strdup (op);
                }
                first = 0;
                t->next = new math ();
                t = t->next;
                t->identifier = (element *)parseOperand (node->xmlChildrenNode);
            }
            if (!xmlStrcmp (node->name, (const xmlChar *) "cn"))
            {
                if (!first)
                {
                    t->next = new math ();
                    t = t->next;
                    t->token = strdup (op);
                }
                first = 0;
                t->next = new math ();
                t = t->next;
                t->token = strdup
                      ((const char *) XML_GET_CONTENT (node->xmlChildrenNode));
            }
        }
    }
    
    t->next = new math ();
    t = t->next;
    t->token = strdup (")");

    return ret;
}

/*
 * A simple operand is defined as just a top level element.  When all else
 * fails, this is called assuming that the node represents just that level of
 * data.
 * To provide for the possibility of reading from a destination element as
 * part of an operation, we need to have a way to wrap an identifier so it
 * can be marked with where the data is coming from (source or destination).
 */
operand * parseSimple (xmlNodePtr node)
{
    element * e = new element ();

    if (!xmlStrcmp (node->name, (const xmlChar *) "simple"))
    {
        xmlChar * s = xmlGetProp (node, BAD_CAST "src");

        if (s && !xmlStrcmp (s, (const xmlChar *) "src"))
        {
            e->src = element::SRC;
        }
        else
        {
            if (s && !xmlStrcmp (s, (const xmlChar *) "dst"))
            {
                e->src = element::DST;
            }
        }
        e->name = strdup ((const char *) XML_GET_CONTENT
               (node->xmlChildrenNode));
    }
    else
    {
        e->name = strdup ((const char *) XML_GET_CONTENT (node));
    }

    e->subscript = ' ';
    e->subelement = NULL;

    return e;
}

/*
 * When a nested element needs to be referenced, the compound structure is
 * used.  This will parse that compound element into a recognizable nested
 * element object.
 */
operand * parseCompound (xmlNodePtr node)
{
    element * e = NULL;
    element ** e1 = &e;

    for (xmlNodePtr n = node->xmlChildrenNode; n; n = n->next)
    {
        if (!xmlStrcmp (n->name, (const xmlChar *) "item"))
        {
            *e1 = new element ();

            (*e1)->name = strdup ((const char *) xmlGetProp (n, BAD_CAST "name"));

            char * subscr = (char *) xmlGetProp (n, BAD_CAST "subscript");
            xmlChar * s = xmlGetProp (n, BAD_CAST "src");

            if (subscr != NULL)
            {
                (*e1)->subscript = subscr [0];
            }
            else
            {
                (*e1)->subscript = ' ';
            }

            (*e1)->subelement = NULL;

            if (s && !xmlStrcmp (s, (const xmlChar *) "src"))
            {
                (*e1)->src = element::SRC;
            }
            else
            {
                if (s && !xmlStrcmp (s, (const xmlChar *) "dst"))
                {
                    (*e1)->src = element::DST;
                }
            }
            xmlFree (s);
            xmlFree (subscr);

            e1 = &((*e1)->subelement);
        }
    }

    return e;
}

/*
 * To keep the type specifiers happy, this is a redirector function for the
 * parseApply to make it fit into the other structure more naturally.
 */
operand * parseMath (xmlNodePtr node)
{
    return parseApply (node);
}

/*
 * Parse a reduce operation into an appropriate operand.
 */
operand * parseReduce (xmlNodePtr node)
{
    reduce * r = new reduce ();

    r->factor = atoi ((const char *) xmlGetProp (node, BAD_CAST "factor"));
    r->identifier = (element *) parseOperand (node->xmlChildrenNode);

    return r;
}

/*
 * The top level parsing function that identifies the type of the operand
 * and passes off the work to the appropriate type-specific parsing
 * function.
 */
operand * parseOperand (xmlNodePtr node)
{
    enum OP_TYPE {NONE, COMPOUND, MATH, REDUCE, SIMPLE} op = NONE;
    xmlNodePtr n1 = node;

    for (; n1; n1 = n1->next)
    {
        if (!xmlStrcmp (n1->name, (const xmlChar *) "simple"))
        {
            op = SIMPLE;
            break;
        }
        else
        {
            if (!xmlStrcmp (n1->name, (const xmlChar *) "compound"))
            {
                op = COMPOUND;
                break;
            }
            else
            {
                if (!xmlStrcmp (n1->name, (const xmlChar *) "apply"))
                {
                    op = MATH;
                    break;
                }
                else
                {
                    if (!xmlStrcmp (n1->name, (const xmlChar *) "reduce"))
                    {
                        op = REDUCE;
                        break;
                    }
                }
            }
        }
    }

    switch (op)
    {
        case COMPOUND:
            return parseCompound (n1);

        case MATH:
            return parseMath (n1);

        case REDUCE:
            return parseReduce (n1);

        case SIMPLE:
            return parseSimple (n1);

        case NONE:
        default:
            return parseSimple (node);
    }
}

/*
 * This function reads the xml described transformation operations and calls
 * the right functions to convert them into the internal data structures.
 */
void readTransform (xmlNodePtr node, vector <operation *> & ops)
{
    xmlNodePtr n;

    for (node = node->xmlChildrenNode; node; node = node->next)
    {
        if (!xmlStrcmp (node->name, (const xmlChar *) "assign"))
        {
            operation * op = NULL;
            if (!op)
            {
                op = new operation ();
                if (op == NULL)
                {
                    cerr << "No memory to parse transform" << endl;
                }
            }

            op->transform = xmlCopyNode (node, 1);

            xmlNodePtr children = node->xmlChildrenNode;

            while (children)
            {
                // one of the following:
                // 1. element name
                // 2. compound element
                if (!xmlStrcmp (children->name, (const xmlChar *) "dst"))
                {
                    op->dst = (element *)
                                    parseOperand (children->xmlChildrenNode);
                }

                // one of the following:
                // 1. element name
                // 2. compound element
                // 3. apply math operation
                // 4. reduce scaling operation
                if (!xmlStrcmp (children->name, (const xmlChar *) "src"))
                {
                    op->src = parseOperand (children->xmlChildrenNode);
                }

                children = children->next;
            }

            op->next = NULL;
            ops.push_back (op);
        }
    }
}

void printSchema (const xmlNodePtr root)
{
    xmlDocPtr doc = xmlNewDoc ((xmlChar *) "1.0");
    xmlDocSetRootElement (doc, xmlCopyNode (root, 1));
    int buffersize;
    xmlChar * xmlbuff;
    xmlDocDumpFormatMemory (doc, &xmlbuff, &buffersize, 1);

    if (!xmlbuff)
    {
        cerr << "Error parsing new type: " << endl;
    }
    else
    {
        cout << (char *) xmlbuff << endl;
    }

    xmlFree (xmlbuff);
    xmlFreeDoc (doc);
}

/*
 * A relic that can used in a testing harness to find out the list
 * of all of the attributes for an XML node
 */
void printXMLAttributes (xmlAttrPtr attributes)
{
    for (;attributes; attributes = attributes->next)
    {
        const xmlChar * text = (const xmlChar *) "";
        xmlNodePtr node = attributes->children;
        while (node != NULL && xmlStrcmp (node->name, (const xmlChar *) "text"))
        {
            node = node->next;
        }
        if (node)
        {
            text = XML_GET_CONTENT (node);
        }
        printf ("(%s:%s)", attributes->name, text);
    }
}

/*
 * A relic used to display a raw xml document in a nested format.  This
 * is still useful for debugging and testing purposes.
 */
int printXMLTree (int indent, xmlNodePtr node)
{
    for (node = node->xmlChildrenNode; node; node = node->next)
    {
        if (   xmlStrcmp (node->name, (const xmlChar *) "text")
            && xmlStrcmp (node->name, (const xmlChar *) "comment")
           )
        {
            int i;
            for (i = indent; i > 0; i--)
            {
                printf ("   ");
            }
            printf ("%s\t", (const char *) node->name);
            printXMLAttributes (node->properties);
            printf ("\n");
            printXMLTree (indent + 1, node);
        }
    }

    return 0;
}

void printFormat (const IOContext & ioc, const string & rootName)
{
    cout << "schema: " << rootName << endl;

    IOFormat format = get_IOformat_by_name_IOcontext (ioc, rootName.c_str ());
    IOFieldList fl = field_list_of_IOformat (format);

    printFields ("", fl, ioc);
}

void printFields (const string & indent, const IOFieldList & fl, const IOContext & ioc)
{
    for (int i = 0; fl [i].field_name != 0; i++)
    {
        string type = fl [i].field_type;
        cout << indent << fl [i].field_name << "==" << type << endl;
        if (   type == "string"
            || type == "integer"
            || type == "double"
            || type == "float"
           )
        {
            continue;
        }
        else
        {
            int subscript = type.find ("[");
            string prefix = type;
            if (subscript != -1)
            {
                prefix = type.substr (0, subscript);
            }

            if (   prefix != "string"
                && prefix != "integer"
                && prefix != "double"
                && prefix != "float"
               )
            {
                IOFormat format = get_IOformat_by_name_IOcontext (ioc, prefix.c_str ());
                IOFieldList fl = field_list_of_IOformat (format);
                printFields (indent + "  ", fl, ioc);
            }
        }
    }
}

/*
 * For a given type, print out the PBIO information.  This is useful for
 * testing to make sure that the type specified in the XML matches what
 * would be expected for the system.  This is only used in the testing
 * harness (xml_main.cc).
 */
void print_pbio_fields (IOField fl [])
{
    IOField * pp;
    for (pp = fl; pp->field_name != NULL; pp++)
    {
        cout << pp->field_name << ":" << pp->field_type
             << ":" << pp->field_size << ":" << pp->field_offset << endl;
    }
}

/*
 * Given a string, count the number of newline characters in it.  This is
 * used in the network communications area to determine when we have read
 * sufficient portions of the network data stream to proceed to the next
 * step in setting up a data stream.
 */
int count_newlines (char * s)
{
    int count = 0;

    while (*s)
    {
        if (*s == '\n')
            count++;

        s++;
    }

    return count;
}

/*
 * read a file into a string object
 */
int readFile (string & filename, string & contents)
{
    FILE * f = fopen (filename.c_str (), "r");
    const int size = 4096;
    char buf [size];

    while (fgets (buf, size, f))
    {
        contents += buf;
    }

    fclose (f);

    return 0;
}

/*
 * $Log: xml.cc,v $
 * Revision 1.35  2005/10/17 20:48:53  lofstead
 * fixed initial case of generating transform list for ECL generation
 *
 * Revision 1.34  2005/10/13 18:08:10  lofstead
 * updated to get printECL to work with newer structure
 *
 * Revision 1.33  2005/09/15 20:51:24  lofstead
 * code cleanup (no functional changes)
 *
 * Revision 1.32  2005/09/15 01:38:21  lofstead
 * updated to use vectors of operation chains rather than just a chain of
 * individual operations as was previously the case.
 *
 * Revision 1.31  2005/02/26 18:29:26  lofstead
 * fixed problems from ECho days to EVPath incompatibility.  Closer, but not
 * quite there yet.
 *
 * Revision 1.30  2005/02/23 22:07:44  lofstead
 * added simple transform (t1s.xml) and fixed bugs in other code according
 * to the EVPath requirements
 *
 * Revision 1.29  2005/02/14 23:23:15  lofstead
 * Updates moving to EVPath.  Not finished yet.
 *
 * Revision 1.28  2004/12/16 15:19:33  lofstead
 * code cleanup
 *
 * Revision 1.27  2004/12/02 04:57:55  lofstead
 * introduced intermediate node daemon and updated server and client to work
 * with this new structure.  For a 3-part xml, there are 4 machines:
 * server a1.xml  (outputs c1)
 * inode a1.xml c1  (outputs c2)
 * inode a2.xml c2  (outputs c3)
 * client a3.xml c3
 *
 * Revision 1.26  2004/10/28 04:24:42  lofstead
 * Completed code for generation of intermediate type XML files.
 *
 * Revision 1.25  2004/10/19 20:09:55  lofstead
 * updated to use better libxml api and changed to capture xml when creating
 * intermediate types rather than trying to use PBIO IOField info.
 *
 * Revision 1.24  2004/10/11 00:00:14  lofstead
 * extended intermediate format to include simple tag and a src attribute
 * on both simple and complex to state if the described element is a src or
 * dst element (those are the values).
 *
 * Added generation of XML type description for the intermediate type.
 *
 * Revision 1.23  2004/04/22 03:16:55  lofstead
 * cleaned up the formatting of the code to <= 80 char lines
 *
 * Revision 1.22  2004/04/21 02:06:29  lofstead
 * expanded comments a lot and cleaned up code a little.  Fixed make environment
 * to represent the new structure properly (needing echo and cm).
 *
 * Revision 1.21  2004/04/19 22:44:03  lofstead
 * code cleanup
 *
 * Revision 1.20  2004/04/19 05:21:55  lofstead
 * server now takes the data type information from the passed across the network
 * XML file
 *
 * Revision 1.19  2004/04/19 03:24:22  lofstead
 * server now capable of receiving the XML file and protocol properly.  Next
 * step is to use that data to do the sending.
 *
 * Revision 1.18  2004/04/19 00:48:38  lofstead
 * introduced the use of the XML file for type declarations for the client
 * by abstracting the XML main into a parser function in xml.cc
 *
 * Revision 1.17  2004/04/18 22:29:47  lofstead
 * updated to pull all of the client-side code into xml.cc
 *
 * Revision 1.16  2004/04/18 20:38:34  lofstead
 * merged in client and server programs and extracted out testing main from code
 *
 * Revision 1.15  2004/04/02 23:33:33  lofstead
 * reduce code completed
 *
 * Revision 1.14  2004/04/02 21:11:31  lofstead
 * reduce operation completed for simple cases
 *
 * Revision 1.13  2004/03/31 20:32:38  lofstead
 * fixed up reduce code a bit.  Still needs more work.
 *
 * Revision 1.12  2004/03/22 23:37:34  lofstead
 * finished ECL generation for simple, compound, and math.  Added preliminary
 * support for reduce operation.
 *
 * Revision 1.11  2004/03/19 16:21:47  lofstead
 * updated to use new declarative syntax.  Now fully parses and prints.
 * Still need to do ECL integration and add other operations.
 *
 * Revision 1.10  2004/03/01 23:19:55  lofstead
 * in progress with new declarative transformation syntax (incl. parsing)
 *
 * Revision 1.9  2004/02/27 02:09:51  lofstead
 * got the printing of the pbio structure data types from the XML format working
 *
 * Revision 1.8  2004/02/26 19:49:06  lofstead
 * *** empty log message ***
 *
 */
