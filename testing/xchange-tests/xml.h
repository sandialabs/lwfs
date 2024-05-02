/*
 * $Id: xml.h,v 1.9 2005/10/13 18:08:10 lofstead Exp $
 */
#include <libxml/parser.h>
#include <evpath.h>
#include "operand.h"

// When a transform is parsed, it is returned in this format
struct Transform
{
    string filter;      // ECL for the transformation
    string src_root;    // name of the type of the root element
    string dst_root;    // name of the type of the root element
    IOContext src_ioc;  // context holding all of the subtypes
    IOContext dst_ioc;  // context holding all of the subtypes
};

/* This represents the necessary information to represent a client request.
 * Each client sends in a list of transforms.  This is used to help track
 * back the resulting nodes in the transform graph to the requesting client.
 */
struct clientRequest
{
    string node;  // node that requested the feed
    string src_root;  // incoming root type
    string dst_root;  // outgoing root type
    vector <operation *> transforms;
    // do not clean these up because they are copies of other pointers
    xmlNodePtr src_schema; // incoming data type
    xmlNodePtr dst_schema; // outgoing data type

    clientRequest () : src_schema (NULL), dst_schema (NULL) {}
};

/*
 * A replacement for the old ECdata_struct to hold the pieces I need.
 * This is a copy of the old structure.
 */
typedef struct
{
    IOFieldList data_field_list;
    CMFormatList data_subformat_list;
    void * initial_value;
} data_struct;

// we need to know where transforms come from to be able to keep type safe
// in the future, we need to have both the source and destination types.
// This is currently the destination type schema.
struct TypedTransform
{
    TypedTransform (operation * o, xmlNodePtr r, const string & r_r) : t (o), root_schema (r), root_type (r_r) {}

    operation * t;
    xmlNodePtr root_schema;
    string root_type;
};

/*
 * This is called by a client to setup the data feed from the server.
 *
 * An example usage of this can be found in client.cc
 *
 * params:
 * cm - The connection manager for the client.  This is specified as a
 *      parameter to give the programmer the opportunity to do any special
 *      processing (such as initializing a threaded environment) is a way
 *      compatible with CM.
 * cc - Like above, give the programmer flexibility.
 * server - The network name of the machine on which the server process is
 *      running.  This will be used with a DNS name resolution to obtain the
 *      connection to that machine.
 * filepath - Generally a path on the remote machine to the file containing
 *      the requested data.  It can also be used for any other purpose the
 *      server desires (such as specifying a live stream to attach).
 * xml - The xml file that describes the data formats and transformation.  The
 *      entire file is sent just in case there is a need to know any of the
 *      additional information on the server side.  Current implementation
 *      only needs the src information on the server and all of the information
 *      on the client machine.  The transformation is sent to the server
 *      automatically by ECho when it is registered.
 * handler - The handler function for the generated events containing the
 *      requested data.
 * client_data - the additional data that will be passed to the handler on
 *      each invocation.  This will allow access to other information in the
 *      application.
 * return:
 * ECSinkHandle - The handle to the channel being subscribed to.  This can be
 *      used to later cancel the subscription or gather information about the
 *      connection.
 */
/* This is an anachronism in EVPath world.  Need to rework completely
ECSinkHandle startFeed (CManager cm
                       ,EControlContext cc
                       ,char * server
                       ,char * filepath
                       ,char * xml
                       ,ECTypedHandlerFunction handler
                       ,void * client_data
                       );
*/

/*
 * When the server or the client needs to be able to parse an XML file into
 * the Transform struct, one of the following can be called.  These are
 * generally used internally by the client and in the standard, generic
 * server module.  General client use of these calls is limited, but
 * possible.  One use might be to see the filter code generated from the
 * supplied XML description.  NOTE: this use of these functions is not
 * guaranteed to work in future versions of this software.
 */
Transform parseXMLfile (const char * pathname);
Transform parseXMLdata (const char * xml, int size);

/*
 * The calls to register the types with ECho in order to create and subscribe
 * to a channel require this ECdata_struct.  This call will create one for
 * the appropriate elements given the IOContext and the root type name.
 */
data_struct generateDataStruct (const IOContext & ioc, const string & root);

/* read the contents of the file specified by filename into contents
 */
int readFile (string & filename, string & contents);

// other exports from xml.cc used or useful elsewhere
void readTransform (xmlNodePtr node, vector <operation *> & op);
void readRootFormats (const xmlNodePtr rootTypes, string & src, string & dst);

void printTransform (const struct operation * op);
void printECL (const IOContext & src, const string & src_root
              ,const IOContext & dst, const string & dst_root
              ,vector <TypedTransform> & op);
void printXMLAttributes (xmlAttrPtr attributes);
int printXMLTree (int indent, xmlNodePtr node);
xmlDocPtr readXML (char * pathname, clientRequest & t);
void printFormat (const IOContext & ioc, const string & rootName);
void printFields (const string & indent, const IOFieldList & fl, const IOContext & ioc);
void printSchema (const xmlNodePtr root);
/*
 * $Log: xml.h,v $
 * Revision 1.9  2005/10/13 18:08:10  lofstead
 * updated to get printECL to work with newer structure
 *
 * Revision 1.8  2005/09/15 20:51:24  lofstead
 * code cleanup (no functional changes)
 *
 */
