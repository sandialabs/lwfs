/*
 * $Id: xml_main.cc,v 1.25 2005/10/13 18:08:10 lofstead Exp $
 */
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <sys/times.h>

using namespace std;

#include <libxml/parser.h>

#include "io.h"

extern "C"
{
#include "xmit.h"
}

#include "xmit_internal.h"

#include "operand.h"
#include "xml.h"

// timings output show microseconds (clock ticks)
#if 0
#define SHOW_TIMINGS(a,b) printf (a, b)
#else
#define SHOW_TIMINGS(a,b)
#endif

const char * SRC = "src";
const char * SRC_TYPE_PREFIX = "src_";
const int SRC_TYPE_PREFIX_LEN = 4;

// need to know the source of the elements so we can reconstruct type info.
struct TypedElement
{
    TypedElement (element * e1, xmlNodePtr r, const string & r_t) : e (e1), root_schema (r), root_type (r_t) {}

    bool operator == (const TypedElement & rhs) const
    {
        // since the elements may have come from different schemas, but represent the same type, only compare the elements
        if (*e == *(rhs.e))
        {
            return true;
        }

        return false;
    }

    element * e;
    xmlNodePtr root_schema;
    string root_type;
};

/* the graph of transforms use these nodes */
struct transformNode
{
    vector <TypedTransform> t;
    vector <transformNode *> next;
    string node;  // identifier for node assigned this transformNode
    string src_root; // source root type
    string dst_root; // source root type
    xmlNodePtr src_schema; // incoming data type
    xmlNodePtr dst_schema; // outgoing data type
    string contactInfo; // this is the stone contact info for this node

    transformNode () : src_schema (NULL), dst_schema (NULL) {}

    ~transformNode ()
    {
        for (vector <transformNode *>::iterator i = next.begin (); i != next.end (); i++)
        {
            delete (*i);
        }

        if (src_schema)
            xmlFreeNode (src_schema);
        if (dst_schema)
            xmlFreeNode (dst_schema);
    }

    string toString (const string & indent = "")
    {
        string ret = indent + "<";

        if (src_schema)
        {
           ret += "has src schema (";
           ret += src_root;
           ret += ")\n";
        }
        if (dst_schema)
        {
           ret += "has dst schema (";
           ret += dst_root;
           ret += ")\n";
        }

        ret += "transforms: ";
        for (vector <TypedTransform>::iterator i = t.begin (); i != t.end (); i++)
        {
            ret += (*i).t->toString ();
        }
        if (t.size () == 0)
            ret += "(no transforms)\n";

        for (vector <transformNode *>::iterator i = next.begin (); i != next.end (); i++)
        {
            ret += (*i)->toString (indent + " ");
        }

        ret += ">";

        return ret;
    }

    // a compact form
    string toString1 (const string & indent = "")
    {
        string ret = "<";

        if (src_schema)
            ret += "S(" + src_root + ")";
        if (dst_schema)
            ret += "D(" + dst_root + ")";
        if (t.size ())
        {
            char a [20];
            sprintf (a, "%d", t.size ());
            ret += "T(";
            ret += a;
            ret += ")";
        }

        for (vector <transformNode *>::iterator i = next.begin (); i != next.end (); i++)
        {
            ret += (*i)->toString1 (indent);
        }

        ret += ">";

        return ret;
    }
};

vector <operation *> extractIdenticalOperations (vector <operation *> & t1, vector <operation *> & t2);
vector <TypedTransform> extractIdenticalOperations (vector <TypedTransform> & t1, vector <operation *> & t2
                                                   ,xmlNodePtr schema, string & root);

void generateCopyElements (xmlNodePtr origSchema, const string & origRoot
                          ,xmlNodePtr schema, const string & rootName
                          ,element * e, char subscript, vector <TypedElement> & ev);
vector <operation *> generateCopyTransforms (const char * srcPrefix, const char * dstPrefix, vector <TypedElement> & ev);

void addTransformDataTypes (vector <TypedElement> & ev, xmlNodePtr output_schema, const string & output_root);
void addPropagatedSrcElements (vector <TypedElement> & p, const string & output_root, xmlNodePtr output_schema);

xmlNodePtr buildBaseSchema (const string & rootType, const string & schemaName);

void visitTransformNode (bool root, const xmlNodePtr root_schema, const string & root_root
                        ,transformNode * node
                        ,const xmlNodePtr src, const string & src_root
                        ,vector <TypedElement> * srcElements);

xmlNodePtr buildXML (transformNode & g);
void printSchemas (transformNode * g);

/* generate a transform graph for the single request passed in.  This is
 * here just to provide a consistent function interface for creating a
 * graph if we have just a single client.
 */
transformNode * generateTransformGraph (clientRequest & t1)
{
    transformNode * root = new transformNode ();

    root->src_schema = t1.src_schema;
    root->src_root = t1.src_root;
    root->dst_schema = t1.dst_schema;
    root->dst_root = t1.dst_root;
    for (vector <operation *>::iterator iter = t1.transforms.begin (); iter != t1.transforms.end (); iter++)
    {
        TypedTransform tt (*iter, t1.dst_schema, t1.dst_root);
        root->t.push_back (tt);
    }

    return root;
}

/* generate the transform graph with duplicates merged
 * This is intended for initial graph generation.  Need a new algorithm for
 * incrementally added a new transform to an existing graph.  It will need
 * to indicate where it is spliced in so that deployment can be handled
 * properly.  It merges the vectors passed in into the graph.
 */
transformNode * generateTransformGraph (clientRequest & t1, clientRequest & t2)
{
    transformNode * root = new transformNode ();
    transformNode * node = NULL; // temp marker

    // put the common elements in the root node
    vector <operation *> i = extractIdenticalOperations (t1.transforms, t2.transforms);
    for (vector <operation *>::iterator iter = i.begin (); iter != i.end (); iter++)
    {
        TypedTransform tt (*iter, t1.dst_schema, t1.dst_root);
        root->t.push_back (tt);
    }
    root->src_schema = t1.src_schema;
    root->src_root = t1.src_root;

    node = new transformNode ();
    root->next.push_back (node);
    node->node = t1.node;
    for (vector <operation *>::iterator iter = t1.transforms.begin (); iter != t1.transforms.end (); iter++)
    {
        TypedTransform tt (*iter, t1.dst_schema, t1.dst_root);
        node->t.push_back (tt);
    }
    node->dst_schema = t1.dst_schema;
    node->dst_root = t1.dst_root;

    node = new transformNode ();
    root->next.push_back (node);
    for (vector <operation *>::iterator iter = t2.transforms.begin (); iter != t2.transforms.end (); iter++)
    {
        TypedTransform tt (*iter, t2.dst_schema, t2.dst_root);
        node->t.push_back (tt);
    }
    node->node = t2.node;
    node->dst_schema = t2.dst_schema;
    node->dst_root = t2.dst_root;

    return root;
}

/* helper to determine if there are any matching transforms in the lists
 * It might be better to count the matches, but that is slower.  With a count,
 * we could choose the one with the largest number of matches as the next node
 * in the list.
 */
bool hasIdenticals (vector <TypedTransform> & t1, vector <operation *> & t2)
{
    for (vector <TypedTransform>::iterator i = t1.begin (); i != t1.end (); i++)
    {
        for (vector <operation *>::iterator j = t2.begin (); j != t2.end (); j++)
        {
            if (*((*i).t) == **j)
            {
                return true;
            }
        }
    }

    return false;
}

/* merge a new transform graph into the existing deployment graph.  This is
 * an iterative procedure that starts at the root trying to map completely
 * the transforms.  Each time it maps completely, it uses that node as a
 * new root and iterates trying to match the rest.
 *
 * 4 possible scenarios:
 * 1. identical ops in graph and transform completely overlapping root node
 *      check to find a next node that has some overlap.  If one is found,
 *      use it as a new root and loop.  Otherwise, add a new node linked in
 *      at this point with the remaining transforms.
 * 2. identical ops in graph and transform incompletely overlapping root node
 *      split the root leaving the common elements and add a node attached to
 *      the root to house the rest of the new transforms
 * 3. no identical ops in graph and transform with transforms in root
 *      split the root leaving no transforms adding a subsequent node
 *      containing the old root contents and a new node for the new transforms
 *      linked in from the now empty root node
 * 4. no identical ops in graph and transform with empty root
 *      add a new node containing all of the transforms and link from the empty
 *      root
 */
transformNode * mergeInTransformGraph (transformNode * root, clientRequest & t)
{
    transformNode * node = NULL; // temp holder since needed a lot
    transformNode * r = root;  // start at the root and progress inward

    while (r)
    {
        vector <TypedTransform> identicals = extractIdenticalOperations (r->t, t.transforms, t.dst_schema, t.dst_root);

        if (identicals.size () != 0)
        {
            if (r->t.size () == 0) // complete match on current root
            {
                // check the children looping if there is reason, otherwise add a new node and be done
                bool foundNext = false;
                for (vector <transformNode *>::iterator i = r->next.begin (); !foundNext && i != r->next.end (); i++)
                {
                    if (hasIdenticals ((*i)->t, t.transforms))
                    {
                        r = *i;
                        foundNext = true;
                    }
                }
                if (!foundNext)
                {
                    // scenario 3/4
                    if (r->t.size () != 0) // scenario 3
                    {
                        node = new transformNode ();
                        node->next = r->next;
                        node->t = r->t;
                        r->next.clear ();
                        r->t.clear ();
                        r->next.push_back (node);
                        node = new transformNode ();
                        for (vector <operation *>::iterator iter = t.transforms.begin (); iter != t.transforms.end (); iter++)
                        {
                            TypedTransform tt (*iter, t.dst_schema, t.dst_root);
                            node->t.push_back (tt);
                        }
                        node->node = t.node;
                        node->dst_schema = t.dst_schema;
                        node->dst_root = t.dst_root;
                        r->next.push_back (node);
                    }
                    else  // scenario 4
                    {
                        node = new transformNode ();
                        for (vector <operation *>::iterator iter = t.transforms.begin (); iter != t.transforms.end (); iter++)
                        {
                            TypedTransform tt (*iter, t.dst_schema, t.dst_root);
                            node->t.push_back (tt);
                        }
                        node->node = t.node;
                        node->dst_schema = t.dst_schema;
                        node->dst_root = t.dst_root;
                        r->next.push_back (node);
                    }

                    r = NULL; // we are done
                }
            }
            else  // split, add a new node, and be done
            {
                node = new transformNode ();
                node->t = r->t;
                node->next = r->next;
                r->next.clear ();
                r->next.push_back (node);
                r->t = identicals;
                node = new transformNode ();
                for (vector <operation *>::iterator iter = t.transforms.begin (); iter != t.transforms.end (); iter++)
                {
                    TypedTransform tt (*iter, t.dst_schema, t.dst_root);
                    node->t.push_back (tt);
                }
                node->node = t.node;
                node->dst_schema = t.dst_schema;
                node->dst_root = t.dst_root;
                r->next.push_back (node);

                r = NULL; // we are done
            }
        }
        else
        {
            if (r->t.size () != 0) // scenario 3
            {
                node = new transformNode ();
                node->next = r->next;
                node->t = r->t;
                r->next.clear ();
                r->t.clear ();
                r->next.push_back (node);
                node = new transformNode ();
                for (vector <operation *>::iterator iter = t.transforms.begin (); iter != t.transforms.end (); iter++)
                {
                    TypedTransform tt (*iter, t.dst_schema, t.dst_root);
                    node->t.push_back (tt);
                }
                node->node = t.node;
                node->dst_schema = t.dst_schema;
                node->dst_root = t.dst_root;
                r->next.push_back (node);
            }
            else  // scenario 4
            {
                node = new transformNode ();
                for (vector <operation *>::iterator iter = t.transforms.begin (); iter != t.transforms.end (); iter++)
                {
                    TypedTransform tt (*iter, t.dst_schema, t.dst_root);
                    node->t.push_back (tt);
                }
                node->node = t.node;
                node->dst_schema = t.dst_schema;
                node->dst_root = t.dst_root;
                r->next.push_back (node);
            }

            r = NULL; // we are done
        }
    }

    return root;
}

/* returns a list of the extracted, identical elements modifying
 * the original lists.
 *
 * It turns out that you cannot use erase and iter++ at the same time.
 * instead, you have to do a structure like I have below (yuck!).
 */
vector <operation *> extractIdenticalOperations (vector <operation *> & t1, vector <operation *> & t2)
{
    vector <operation *> identical_ops;

    vector <operation *>::iterator iter = t1.begin ();
    while (iter != t1.end ())
    {
        bool eraset1 = false;
        operation * x = *iter;

        vector <operation *>::iterator iter2 = t2.begin ();
        while (iter2 != t2.end ())
        {
            operation * y = *iter2;

            if (*x == *y)
            {
                eraset1 = true;
                iter2 = t2.erase (iter2);
                identical_ops.push_back (x);
                break;  // no need to look some more for a match
            }
            else
            {
                iter2++;
            }
        }

        if (eraset1)
        {
            iter = t1.erase (iter);
        }
        else
        {
            iter++;
        }
    }

    return identical_ops;
}

vector <TypedTransform> extractIdenticalOperations (vector <TypedTransform> & t1, vector <operation *> & t2, xmlNodePtr schema, string & root)
{
    vector <TypedTransform> identical_ops;

    vector <TypedTransform>::iterator iter = t1.begin ();
    while (iter != t1.end ())
    {
        bool eraset1 = false;
        TypedTransform x = *iter;

        vector <operation *>::iterator iter2 = t2.begin ();
        while (iter2 != t2.end ())
        {
            operation * y = *iter2;

            if (*(x.t) == *y)
            {
                eraset1 = true;
                iter2 = t2.erase (iter2);
                identical_ops.push_back (x);
                break;  // no need to look for some more for a match
            }
            else
            {
                iter2++;
            }
        }

        if (eraset1)
        {
            iter = t1.erase (iter);
        }
        else
        {
            iter++;
        }
    }

    return identical_ops;
}

int main (int argc, char ** argv)
{
    int ret = 0;

    xmlDocPtr doc = NULL;
    xmlDocPtr doc2 = NULL;
    xmlDocPtr doc3 = NULL;

    clientRequest t1;
    clientRequest t2;
    clientRequest t3;

    xmlInitParser ();

/*
    t1.node = "t3a";
    SHOW_TIMINGS ("pre read t3a: %d\n", times (NULL));
    doc = readXML ("test/image/t3a.xml", t1);
    SHOW_TIMINGS ("post read t3a: %d\n", times (NULL));

    t2.node = "t3b";
    SHOW_TIMINGS ("pre read t3b: %d\n", times (NULL));
    doc2 = readXML ("test/image/t3b.xml", t2);
    SHOW_TIMINGS ("post read t3b: %d\n", times (NULL));

    t3.node = "t3c";
    SHOW_TIMINGS ("pre read t3c: %d\n", times (NULL));
    doc3 = readXML ("test/image/t3c.xml", t3);
    SHOW_TIMINGS ("post read t3c: %d\n", times (NULL));

    if (!doc)
    {
        fprintf (stderr, "problem parsing doc1\n");

        if (doc2)
            xmlFreeDoc (doc2);
        if (doc3)
            xmlFreeDoc (doc3);
        xmlCleanupParser ();

        return -1;
    }

    if (!doc2)
    {
        fprintf (stderr, "problem parsing doc2\n");

        xmlFreeDoc (doc);
        if (doc3)
            xmlFreeDoc (doc3);
        xmlCleanupParser ();

        return -1;
    }

    if (!doc3)
    {
        fprintf (stderr, "problem parsing doc3\n");

        xmlFreeDoc (doc);
        xmlFreeDoc (doc2);
        xmlCleanupParser ();

        return -1;
    }
*/

    t1.node = "h2.xml";
    doc = readXML ("test/healthcare/h2.xml", t1);
    transformNode * graph = generateTransformGraph (t1);
#if 0
//    SHOW_TIMINGS ("pre initial graph: %d\n", times (NULL));
    transformNode * graph = generateTransformGraph (t1, t2);
/*
    SHOW_TIMINGS ("post initial graph: %d\n", times (NULL));
    cout << "graph with t1, t2" << endl;
    cout << graph->toString1 () << endl;
    cout << "done" << endl;
    SHOW_TIMINGS ("pre merge graph: %d\n", times (NULL));
*/
    graph = mergeInTransformGraph (graph, t3);
/*
    SHOW_TIMINGS ("post merge graph: %d\n", times (NULL));
    cout << "graph with t1, t2, t3" << endl;
    cout << graph->toString1 () << endl;
    cout << "done" << endl;
*/
#endif

    // build out the schemas for each node
    SHOW_TIMINGS ("pre build out graph: %d\n", times (NULL));
    visitTransformNode (true, graph->src_schema, graph->src_root, graph, graph->src_schema, graph->src_root, NULL);
    SHOW_TIMINGS ("post build out graph: %d\n", times (NULL));

    // display the appropriate XMLs for each node in the graph
    printSchemas (graph);

    // cleanup
    xmlFreeDoc (doc);
    xmlFreeDoc (doc2);
    xmlFreeDoc (doc3);
    xmlCleanupParser ();

    return ret;
}

// recursively print out the XML descriptors for each node in the graph
void printSchemas (transformNode * g)
{
    // this xml node is our document we can send to the nodes
    xmlNodePtr xml = buildXML (*g);
    cout << "XML for node " << g->node << endl;
    printSchema (xml);

    xmlFreeNode (xml);

    // make sure we can convert them to PBIO properly
    IOContext ioci = create_IOcontext (); // input
    IOContext ioco = create_IOcontext (); // output

    pbio_schema_converter source (ioci);
    source.add_types (g->src_schema);

    pbio_schema_converter destination (ioco);
    destination.add_types (g->dst_schema);

    printf ("\nECL:\n");
    printECL (ioci, g->src_root, ioco, g->dst_root, g->t);

    for (vector <transformNode *>::iterator i = g->next.begin (); i != g->next.end (); i++)
    {
        printSchemas (*i);
    }
}

/* recursive function to generate node-specific transform, propagation, and contact info.
 */
void visitTransformNode (bool root   // are we at the root of the tree
                        ,const xmlNodePtr root_schema, const string & root_type  // type being served
                        ,transformNode * node // current node to process
                        ,const xmlNodePtr src_schema, const string & src_type // last node's output type
                        ,vector <TypedElement> * srcElements // propagation elements
                        )
{
/*
cout << "=====================================================================================" << endl;
cout << "root " << root << endl;
cout << "root_schema " << root_schema << endl;
printSchema (root_schema);
cout << "root_type " << root_type << endl;
cout << "node " << node << endl;
cout << "src_schema " << src_schema << endl;
printSchema (src_schema);
cout << "src_type " << src_type << endl;
cout << "srcElements size " << (srcElements ? srcElements->size () : 0) << endl;
cout << "-------------------------------------------------------------------------------------" << endl;
*/
    if (node->next.size () != 0) // we are not at a dst node
    {
        if (!root)  // int->int case
        {
            // cout << "int->int" << endl;
            // 1. Create this node's src schema by copying in the schema passed in
            node->src_schema = buildBaseSchema (src_type, "src");
            node->src_root = src_type;
            vector <TypedElement> ev;
            generateCopyElements (root_schema, root_type, root_schema, root_type, NULL, 'a', ev);
            addTransformDataTypes (ev, node->src_schema, node->src_root);

            // 2. Create this node's dst schema by getting the dst elements from the transforms in this node
            ev.clear ();
            for (vector <TypedTransform>::iterator i = node->t.begin ()
                ;i != node->t.end ()
                ;i++
                )
            {
                operation * o = i->t;
                while (o)
                {
                    TypedElement te (o->dst, i->root_schema, i->root_type);
                    ev.push_back (te);
                    o = o->next;
                }
            }
            node->dst_schema = buildBaseSchema (src_type, "dst");
            node->dst_root = src_type;

            addTransformDataTypes (ev, node->dst_schema, node->dst_root);

            // 3. Generate a list of the required propagation elements by recursively calling using the dst schema
            for (vector <transformNode *>::iterator i = node->next.begin ()
                ;i != node->next.end ()
                ;i++
                )
            {
                vector <TypedElement> ev3;
                visitTransformNode (false, root_schema, root_type, *i, node->dst_schema, node->dst_root, &ev3);
                for (vector <TypedElement>::iterator j = ev3.begin (); j != ev3.end (); j++)
                {
                    // need to check for duplicates
                    srcElements->push_back (*j);
                }
            }

            // 4. Add the list to the dst schema
            addTransformDataTypes (*srcElements, node->dst_schema, node->dst_root);

            // 5. Add the required src elements from this node's transforms to the propagation elements
            for (vector <TypedTransform>::iterator i = node->t.begin ()
                ;i != node->t.end ()
                ;i++
                )
            {
                operation * o = i->t;
                while (o)
                {
                    vector <element *> a = o->src->getSrcElements ();
                    for (vector <element *>::iterator j = a.begin (); j != a.end (); j++)
                    {
                        // need to check for duplicates
                        // this will have to change from root_schema/root_type when real
                        // derived intermediate types are created.  Until that point, this
                        // will work.
                        TypedElement te (*j, root_schema, root_type);
                        srcElements->push_back (te);
                    }
                    o = o->next;
                }
            }

            // 6. Remove the elements generated in this node from those needing to be
            //    propagated from upstream
            vector <TypedElement>::iterator iter = srcElements->begin ();
            while (iter != srcElements->end ())
            {
                bool eraset1 = false;

                for (vector <TypedElement>::iterator iter2 = ev.begin ()
                    ;iter2 != ev.end ()
                    ;iter2++
                    )
                {
                    if (*iter == *iter2)
                    {
                        eraset1 = true;
                        break;  // no need to look some more for a match
                    }
                }

                if (eraset1)
                {
                    iter = srcElements->erase (iter);
                }
                else
                {
                    iter++;
                }
            }

            // 7. Add the list to the src schema
            addTransformDataTypes (*srcElements, node->src_schema, node->src_root);
            // cout << "end int->int" << endl;
        }
        else  // src->int case
        {
            // cout << "src->int" << endl;
            // 2. Create this node's dst schema by getting the dst elements from the transforms in this node
            vector <TypedElement> ev;
            for (vector <TypedTransform>::iterator i = node->t.begin ()
                ;i != node->t.end ()
                ;i++
                )
            {
                operation * o = i->t;
                while (o)
                {
                    TypedElement te (o->dst, i->root_schema, i->root_type);
                    ev.push_back (te);
                    o = o->next;
                }
            }
            node->dst_schema = buildBaseSchema (src_type, "dst");
            node->dst_root = src_type;

            addTransformDataTypes (ev, node->dst_schema, node->dst_root);

            vector <TypedElement> x;   // dummy item to keep step 3 consisent across cases
            srcElements = &x;          // dummy item to keep step 3 consisent across cases
            // 3. Generate a list of the required propagation elements by recursively calling using the dst schema
            for (vector <transformNode *>::iterator i = node->next.begin ()
                ;i != node->next.end ()
                ;i++
                )
            {
                vector <TypedElement> e2;
                visitTransformNode (false, root_schema, root_type, *i, node->dst_schema, node->dst_root, &e2);
                for (vector <TypedElement>::iterator j = e2.begin (); j != e2.end (); j++)
                {
                    // need to check for duplicates
                    srcElements->push_back (*j);
                }
            }

            // 4. Add the list to the dst schema
            addTransformDataTypes (*srcElements, node->dst_schema, node->dst_root);
            // cout << "end src->int" << endl;
        }
    }
    else
    {
        if (!root)  // int->dst case
        {
            // cout << "int->dst" << endl;
            // 1. Create this node's src schema by copying in the schema passed in
            node->src_schema = buildBaseSchema (src_type, "src");
            node->src_root = src_type;
            vector <TypedElement> ev;
            generateCopyElements (node->dst_schema, node->dst_root, node->dst_schema, node->dst_root, NULL, 'a', ev);
            addTransformDataTypes (ev, node->src_schema, node->src_root);

            // 5. Add the required src elements from this node's transforms to the propagation elements
            if (node->t.size () == 0) // if no transforms, we need all of the dst elements
            {
                generateCopyElements (node->dst_schema, node->dst_root, node->dst_schema, node->dst_root, NULL, 'a', *srcElements);
            }
            else
            {
                for (vector <TypedTransform>::iterator i = node->t.begin ()
                    ;i != node->t.end ()
                    ;i++
                    )
                {
                    operation * o = i->t;
                    while (o)
                    {
                        vector <element *> a = o->src->getSrcElements ();
                        for (vector <element *>::iterator j = a.begin (); j != a.end (); j++)
                        {
                            // need to check for duplicates
                            TypedElement te (*j, i->root_schema, i->root_type);
                            srcElements->push_back (te);
                        }
                        o = o->next;
                    }
                }
            }

            // 7. Add the list to the src schema
            addTransformDataTypes (*srcElements, node->src_schema, node->src_root);
            // cout << "end int->dst" << endl;
        }
        else  // src->dst case
        {
            // cout << "src->dst" << endl;
            // no steps necessary
        }
    }
}

/* buildBaseSchema - creates a default root element with the appropriate schema name
 */
xmlNodePtr buildBaseSchema (const string & rootType, const string & schemaName)
{
    xmlNodePtr new_schema = xmlNewNode (NULL, BAD_CAST "schema");
    xmlNsPtr ns = xmlNewNs (new_schema, BAD_CAST "http://www.w3.org/1999/XMLSchema", BAD_CAST schemaName.c_str ());
    xmlSetNs (new_schema, ns);
    xmlNewProp (new_schema, BAD_CAST "targetNamespace", BAD_CAST "http://www.cc.gatech.edu/~lofstead/schemas");

    xmlNodePtr node = xmlNewNode (ns, BAD_CAST "complexType");
    xmlNewProp (node, BAD_CAST "name", BAD_CAST rootType.c_str ());
    xmlAddChild (new_schema, node);

    return new_schema;
}

/* addTransformDataTypes - add the type information to the output_schema for the passed in
 *                         transforms copying from the schema contained in the TypedElement
 *
 * There are two different procedures.  If the destination type is an intermediate node, then
 * there isn't a fixed ordering of elements and they must be discovered.  If it is a terminal
 * node, then both the starting and final types are known and the transform just needs to be
 * generated.  This step is not needed.
 *
 * Procedure:
 * 1. Obtain a list of downstream needed elements.
 * 2. For each needed element, if it is not generated in the transforms, add a copy transform.
 * 3. Parse through the stored transforms noting information necessary for creating a new type.
 * 4. Fixup the gathered information filling in the offset and sizing information.
 * 5. Register the types.
        // Strategy:
        // for each transform
        //     start the element at the root of the destination element
        //     start the type being built with the root type (cached type)
        //     for each element nesting in the destination
        //         if we find the field in the current cached type (we've seen it already)
        //             set the cached type we are evaluating to be the one for the current field
        //         else
        //             look in the corresponding dst for the field to pull it out
        //             if the type is not simple
        //                 add a new chached type
        //                 if this the last element on the list for this transform
        //                     recursively add all type information so that we don't lose anything
        //             update the searching locations and cache position for looping
 */
void addTransformDataTypes (vector <TypedElement> & ev, xmlNodePtr output_schema, const string & output_root)
{
    xmlNodePtr node = NULL;                     // generic temp holder
    xmlNodePtr output_schema_root_node = NULL;  // used to navigate the schema to be generated into
    xmlNodePtr src_schema_root_node = NULL;     // used to remember the root point of the source of the element

    for (vector <TypedElement>::iterator eviter = ev.begin (); eviter != ev.end (); eviter++)
    {
        xmlNodePtr src_schema = eviter->root_schema;
        string src_root = eviter->root_type;
        //cout << "element: " << eviter->e->toString () << endl;
        //cout << "src schema:" << endl;
        //printSchema (src_schema);

        // must start the src_schema_root_node pointer to be the correct root type
        for (node = src_schema->children; node; node = node->next)
        {
            if (!xmlStrcmp (node->name, BAD_CAST "complexType"))
            {
                xmlChar * t = xmlGetProp (node, BAD_CAST "name");
                if (!src_root.compare ((char *) t))
                {
                    src_schema_root_node = node;
                    xmlFree (t);

                    break;
                }
                xmlFree (t);
            }
        }

        // must start the output_schema_root_node to be the correct root type
        for (node = output_schema->children; node; node = node->next)
        {
            if (!xmlStrcmp (node->name, BAD_CAST "complexType"))
            {
                xmlChar * t = xmlGetProp (node, BAD_CAST "name");
                if (!output_root.compare ((char *) t))
                {
                    output_schema_root_node = node;
                    xmlFree (t);

                    break;
                }
                xmlFree (t);
            }
        }

        // iterate over each nested level of the element to create the proper structure
        element * e = eviter->e;
        xmlNodePtr current_output_node = output_schema_root_node;  // where we are inserting
        xmlNodePtr current_src_node = src_schema_root_node;        // where we are pulling from

        //cout << "looking for: " << e->toString () << endl;
        while (e)
        {
            //cout << "this iteration: " << e->toString () << endl;
            // loop through the current level in the output type to see if we can find it
            for (node = current_output_node->children; node; node = node->next)
            {
                //cout << "node name: " << (char *) node->name << endl;
                if (!xmlStrcmp (node->name, BAD_CAST "element"))  // if it is an element
                {
                xmlChar * val = xmlGetProp (node, BAD_CAST "name");
                if (!xmlStrcmp (val, BAD_CAST e->name))
                {
                    //cout << "found it" << endl;
                    // get the type
                    string type;
                    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
                    {
                        if (!xmlStrcmp (attr->name, (xmlChar *) "type"))
                        {
                            type = (char *) attr->children->content;
                            break;
                        }
                    }
                    //cout << "type is " << type << endl;
                    // and see if we need to navigate to another type
                    if (type.compare (0, 4, "xsd:")) // not a primitive type
                    {
                        xmlNodePtr temp;
                        for (temp = output_schema->children; temp; temp = temp->next)
                        {
                            if (!xmlStrcmp (temp->name, BAD_CAST "complexType"))
                            {
                                xmlChar * t = xmlGetProp (temp, BAD_CAST "name");
                                if (!type.compare ((char *) t))
                                {
                                    current_output_node = temp;
                                    xmlFree (t);
                                    break;
                                }
                                xmlFree (t);
                            }
                        }
                        break;
                    }
                    else  // primitives can be ignored
                    {
                        break;
                    }
                }
                }
            }

            // we didn't find the name
            if (!node)
            {
                //cout << "didn't find it" << endl;
                // find in src_schema and copy
                for (node = current_src_node->children; node; node = node->next)
                {
                    xmlChar * val = xmlGetProp (node, BAD_CAST "name");
                    //if (!val) {cout << "bad val" << endl; continue;}
                    //cout << "checking current_src_node node " << (char *) val << " with " << e->name << endl;
                    if (!xmlStrcmp (val, BAD_CAST e->name))
                    {
                        //cout << "match" << endl;
                        string new_type;
                        xmlNodePtr new_node = xmlNewNode (output_schema->ns, BAD_CAST node->name);
                        for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
                        {
                            xmlAttrPtr attribute = xmlNewProp (new_node, attr->name, attr->children->content);

                            // if this is the type node, take note for later processing
                            if (!xmlStrcmp (attribute->name, (xmlChar *) "type"))
                            {
                                new_type = (char *) attribute->children->content;
                            }
                        }
                        //printSchema (new_node);

                        xmlAddChild (current_output_node, new_node);
                        //cout << "adding it" << endl;

                        //cout << "new type is " << new_type << endl;
                        if (new_type.compare (0, 4, "xsd:")) // not a primitive type
                        {
                            node = xmlNewNode (output_schema->ns, BAD_CAST "complexType");
                            xmlNewProp (node, BAD_CAST "name", BAD_CAST new_type.c_str ());

                            // to make the parsing work properly, will push all new types on front
                            xmlAddPrevSibling (output_schema->children, node);
                            current_output_node = node;

                            // adjust the current_src_node pointer to be for the newly found type
                            for (node = src_schema->children; node; node = node->next)
                            {
                                if (!xmlStrcmp (node->name, BAD_CAST "complexType"))
                                {
                                    if (!new_type.compare ((char *) xmlGetProp (node, BAD_CAST "name")))
                                    {
                                        current_src_node = node;
                                        break;
                                    }
                                }
                            }
                        }

                        break; // go to the next element
                    }
                }
            }
            else
            {
                // need to make current_src_node keep pace with the nesting so that we can
                // add multiple elements at a nested level
                string type = (char *) xmlGetProp (node, BAD_CAST "type");
                if (type.compare (0, 4, "xsd:")) // not a primitive type
                {
                    xmlNodePtr temp;
                    for (temp = src_schema->children; temp; temp = temp->next)
                    {
                        if (!xmlStrcmp (temp->name, BAD_CAST "complexType"))
                        {
                            if (!type.compare ((char *) xmlGetProp (temp, BAD_CAST "name")))
                            {
                               current_src_node = temp;
                               break;
                            }
                        }
                    }
                }
            }

            e = e->subelement;
        }
    }
}

/* addPropagatedSrcElements - given the vector of elements to propagate, p, add
 * the appropriate copies to the output_schema in the appropriate nested context.

 * make similar to addTransformDataTypes since it works at this point
 */
void addPropagatedSrcElements (vector <TypedElement> & p, const string & output_root, xmlNodePtr output_schema)
{
    xmlNodePtr node = NULL; // temp

    for (vector <TypedElement>::iterator i = p.begin (); i != p.end (); i++)
    {
        string src_root = i->root_type; // root of the source of this element
        xmlNodePtr src_schema = i->root_schema; // schema of the source of this element

        xmlNodePtr output_schema_root_node = NULL; // remember the root complexType for output_schema
        for (node = output_schema->children; node; node = node->next)
        {
            if (!xmlStrcmp (node->name, BAD_CAST "complexType"))
            {
                xmlChar * t = xmlGetProp (node, BAD_CAST "name");
                if (!output_root.compare ((char *) t))
                {
                    output_schema_root_node = node;
                    xmlFree (t);

                    break;
                }
                xmlFree (t);
            }
        }

        // remember the root complexType for src
        xmlNodePtr src_schema_root_node = NULL;
        for (node = src_schema->children; node; node = node->next)
        {
            if (!xmlStrcmp (node->name, BAD_CAST "complexType"))
            {
                xmlChar * t = xmlGetProp (node, BAD_CAST "name");
                if (!src_root.compare ((char *) t))
                {
                    src_schema_root_node = node;
                    xmlFree (t);

                    break;
                }
                xmlFree (t);
            }
        }

        xmlNodePtr output_schema_propagation_root_node = NULL; // propagation root type in output schema
        xmlNodePtr current_output_node = NULL; // pointer into output_schema as we move around
        xmlNodePtr current_src_node = NULL; // pointer into src_schema as we move around

        // add the propagated root type element "src"
        output_schema_propagation_root_node = xmlNewNode (output_schema->ns, BAD_CAST "complexType");
        xmlNewProp (output_schema_propagation_root_node, BAD_CAST "name", BAD_CAST SRC_TYPE_PREFIX);
        if (output_schema->children)
        {
            xmlAddPrevSibling (output_schema->children, output_schema_propagation_root_node);
        }
        else
        {
            xmlAddChild (output_schema, output_schema_propagation_root_node);
        }

        // add pointer to the type in the root complexType element
        node = xmlNewNode (output_schema->ns, BAD_CAST "element");
        xmlNewProp (node, BAD_CAST "name", BAD_CAST SRC);
        xmlNewProp (node, BAD_CAST "type", BAD_CAST SRC_TYPE_PREFIX);
        xmlAddChild (output_schema_root_node, node);

        // go through the propagated elements copying the type information from
        // the source schema into the propagation element in the output_schema schema
        element * e = i->e;
        current_src_node = src_schema_root_node;
        current_output_node = output_schema_propagation_root_node;

        while (e)
        {
            for (node = current_output_node->children; node; node = node->next)
            {
                xmlChar * val = xmlGetProp (node, BAD_CAST "name");
                if (!xmlStrcmp (val, BAD_CAST e->name))
                {
                    // if it is a complex type, change output to that type
                    xmlChar * p = xmlGetProp (node, BAD_CAST "type");
                    string type = (char *) p;
                    xmlFree (p);
                    if (type.compare (0, 4, "xsd:")) // not a primitive
                    {
                        string new_type = type; // keep the "src_" prefix for finding in output_schema
                        type.erase (0, SRC_TYPE_PREFIX_LEN); // remove the "src_" prefix for finding in src
                        // update our output_schema reference for this type
                        for (xmlNodePtr node1 = output_schema->children; node1; node1 = node1->next)
                        {
                            if (!xmlStrcmp (node1->name, BAD_CAST "complexType"))
                            {
                                xmlChar * t = xmlGetProp (node1, BAD_CAST "name");
                                if (!new_type.compare ((char *) t))
                                {
                                    xmlFree (t);
                                    current_output_node = node1;
                                    break;
                                }
                                xmlFree (t);
                            }
                        }

                        for (xmlNodePtr node1 = src_schema->children; node1; node1 = node1->next)
                        {
                            if (!xmlStrcmp (node1->name, BAD_CAST "complexType"))
                            {
                                xmlChar * t = xmlGetProp (node1, BAD_CAST "name");
                                if (!type.compare ((char *) t))
                                {
                                    xmlFree (t);
                                    current_src_node = node1;
                                    break;
                                }
                                xmlFree (t);
                            }
                        }
                    }

                    break;
                }
            }

            // we didn't find the name in the output_schema already
            if (!node)
            {
                // find in src_schema and copy
                for (node = current_src_node->children; node; node = node->next)
                {
                    xmlChar * val = xmlGetProp (node, BAD_CAST "name");
                    if (!xmlStrcmp (val, BAD_CAST e->name))
                    {
                        xmlFree (val);
                        string new_type;
                        // create an "element" node
                        xmlNodePtr new_node = xmlNewNode (output_schema->ns, BAD_CAST node->name);
                        // add element to output_schema type
                        xmlAddChild (current_output_node, new_node);
                        // copy the properties fixing up the "type"
                        for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
                        {
                            // if this is the "type" attribute
                            if (!xmlStrcmp (attr->name, (xmlChar *) "type"))
                            {
                                string old_type = (char *) attr->children->content;
                                new_type = old_type;
                                if (new_type.compare (0, 4, "xsd:")) // not a primitive
                                {
                                    new_type.insert (0, SRC_TYPE_PREFIX);

                                    // create a new type for output_schema
                                    xmlNodePtr node2 = xmlNewNode (output_schema->ns, BAD_CAST "complexType");
                                    xmlNewProp (node2, BAD_CAST "name", BAD_CAST new_type.c_str ());

                                    // add new type to output_schema (in front for proper parsing)
                                    xmlAddPrevSibling (output_schema->children, node2);

                                    // change output_schema pointer to be new type
                                    current_output_node = node2;

                                    // change src pointer to be new type
                                    for (node2 = src_schema->children; node2; node2 = node2->next)
                                    {
                                        if (!xmlStrcmp (node2->name, BAD_CAST "complexType"))
                                        {
                                            xmlChar * prop = xmlGetProp (node2, BAD_CAST "name");
                                            if (!old_type.compare ((char *) prop))
                                            {
                                                current_src_node = node2;
                                                xmlFree (prop);
                                                break;
                                            }

                                            xmlFree (prop);
                                        }
                                    }
                                }
                                xmlAttrPtr attribute = xmlNewProp (new_node, attr->name, BAD_CAST new_type.c_str ());
                            }
                            else
                            {
                                xmlAttrPtr attribute = xmlNewProp (new_node, attr->name, attr->children->content);
                            }
                        }

                        break; // go to the next element
                    }
                    xmlFree (val);
                }
            }

            e = e->subelement;
        }
    }
}

/* generateCopyElements - given a schema and the rootName type, generate a list of
 * the elements in the ev parameter vector for generation of the assignment copy
 * transforms.  This is a recursive function using the e parameter as the base
 * prefix for each element added to the vector.  Initially the e parameter should
 * be NULL indicating a root position.
 */
void generateCopyElements (xmlNodePtr origSchema, const string & origRoot
                          ,xmlNodePtr schema, const string & rootName
                          ,element * e, char subscript, vector <TypedElement> & ev
                          )
{
    // find the root of the complex type specified by the root parameter
    // iterate over the list of elements in that type adding new elements to the vector for each
    // recurse for any complexType member of this type

    // remember the root complexType for src
    xmlNodePtr root = NULL;
    xmlNodePtr node = NULL;
    for (node = schema->children; node; node = node->next)
    {
        if (!xmlStrcmp (node->name, BAD_CAST "complexType"))
        {
            xmlChar * t = xmlGetProp (node, BAD_CAST "name");
            if (!rootName.compare ((char *) t))
            {
                root = node;
                xmlFree (t);

                break;
            }
            xmlFree (t);
        }
    }

    for (node = root->children; node; node = node->next)
    {
        if (!xmlHasProp (node, BAD_CAST "name")) continue; // if it isn't interesting, bail on it.
        xmlChar * t = xmlGetProp (node, BAD_CAST "type"); // check for non-primitive
        xmlChar * a = xmlGetProp (node, BAD_CAST "minOccurs"); // is it an array
        xmlChar * name = xmlGetProp (node, BAD_CAST "name");

        element * new_e;
        element * suffix = new element ();
        suffix->name = strdup ((char *) name);
        if (a)
        {
            suffix->subscript = subscript++;
        }
        if (e)
        {
            new_e = new element (e);
            new_e->addSubelement (suffix);
        }
        else
        {
            new_e = suffix;
        }

        string type = (char *) t;
        if (!type.compare (0, 4, "xsd:")) // a primitive type
        {
            TypedElement te (new_e, origSchema, origRoot);
            ev.push_back (te);
        }
        else
        {
            if (type.compare (0, SRC_TYPE_PREFIX_LEN, SRC_TYPE_PREFIX)) // not the source copy node
            {
                generateCopyElements (origSchema, origRoot, schema, type, new_e, subscript++, ev);
            }
            delete new_e;
        }
        xmlFree (name);
        xmlFree (a);
        xmlFree (t);
    }
}

/* addElements - helper for generateCopyTransforms below
 */
void addElements (const char * prefix, element * e, xmlNodePtr parent)
{
    xmlNodePtr n = NULL;  // working node
    int id = 1;
    char text [4];

    if (prefix || e->subelement || e->subscript != " ")
    {
        n = xmlNewNode (NULL, BAD_CAST "compound");
        xmlAddChild (parent, n);
        parent = n;
        if (prefix)
        {
            n = xmlNewNode (NULL, BAD_CAST "item");
            xmlNewProp (n, BAD_CAST "name", BAD_CAST prefix);
            sprintf (text, "%d", id++);
            xmlNewProp (n, BAD_CAST "id", BAD_CAST text);
            xmlAddChild (parent, n);
        }

        while (e)
        {
            n = xmlNewNode (NULL, BAD_CAST "item");
            xmlNewProp (n, BAD_CAST "name", BAD_CAST e->name);
            sprintf (text, "%d", id++);
            xmlNewProp (n, BAD_CAST "id", BAD_CAST text);
            if (e->subscript != " ")
            {
                xmlNewProp (n, BAD_CAST "subscript", BAD_CAST e->subscript.c_str ());
            }
            xmlAddChild (parent, n);
            e = e->subelement;
        }
    }
    else
    {
        xmlNodeSetContent (parent, BAD_CAST e->name);
    }
}

/* generateCopyTransforms - given a list of elements and optionally src and dst
 * prefix strings, generate the proper operations to copy the elements from the
 * input to the output
 */
vector <operation *> generateCopyTransforms (const char * srcPrefix, const char * dstPrefix, vector <TypedElement> & ev)
{
    xmlNodePtr root = xmlNewNode (NULL, BAD_CAST "transform");

    for (vector <TypedElement>::iterator i = ev.begin (); i != ev.end (); i++)
    {
        element * e = i->e;

        xmlNodePtr assign = xmlNewNode (NULL, BAD_CAST "assign");
        xmlAddChild (root, assign);

        xmlNodePtr n = xmlNewNode (NULL, BAD_CAST "dst");
        xmlAddChild (assign, n);
        addElements (dstPrefix, e, n);
        n = xmlNewNode (NULL, BAD_CAST "src");
        xmlAddChild (assign, n);
        addElements (srcPrefix, e, n);
    }

    vector <operation *> op;
    readTransform (root, op);

    return op;
}

/* buildXML - generate an XML document for the given types and transforms
 */
xmlNodePtr buildXML (transformNode & g)
{
    xmlNodePtr root = xmlNewNode (NULL, BAD_CAST "transformation");
    xmlAddChild (root, xmlCopyNode (g.src_schema, 1));
    xmlAddChild (root, xmlCopyNode (g.dst_schema, 1));
    xmlNodePtr n = xmlNewNode (NULL, BAD_CAST "rootformats");
    xmlAddChild (root, n);
    xmlNodePtr n1 = xmlNewNode (NULL, BAD_CAST "src");
    xmlAddChild (n, n1);
    xmlNodeSetContent (n1, BAD_CAST g.src_root.c_str ());
    n1 = xmlNewNode (NULL, BAD_CAST "dst");
    xmlAddChild (n, n1);
    xmlNodeSetContent (n1, BAD_CAST g.dst_root.c_str ());
    n = xmlNewNode (NULL, BAD_CAST "transform");
    xmlAddChild (root, n);
    for (vector <TypedTransform>::iterator i = g.t.begin (); i != g.t.end (); i++)
    {
        xmlAddChild (n, xmlCopyNode (i->t->transform, 1));
    }

    return root;
}
/*
 * $Log: xml_main.cc,v $
 * Revision 1.25  2005/10/13 18:08:10  lofstead
 * updated to get printECL to work with newer structure
 *
 * Revision 1.24  2005/10/02 21:32:00  lofstead
 *
 * added timing output code
 *
 * Revision 1.23  2005/09/15 20:51:24  lofstead
 * code cleanup (no functional changes)
 *
 */
