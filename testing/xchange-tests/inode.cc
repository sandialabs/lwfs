/*
 * $Id: inode.cc,v 1.10 2005/09/15 20:51:23 lofstead Exp $
 */
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>
#include <string>

using namespace std;

#include "evpath.h"

#include "xml.h"

// metrics collection handler function
int handler (CManager cm, void * message, void * client_data, attr_list attrs)
{
    if (attrs)
    {
        dump_attr_list (attrs);
    }

    EVsubmit ((EVsource) client_data, message, NULL);

    return 0;
}

int main (int argc, char ** argv)
{
    if (argc < 3)
    {
        fprintf (stderr, "usage: %s <filename> <contact list> ...\n", argv [0]);

        return -1;
    }
    string filename = argv [1];
    string xmlfile;

    readFile (filename, xmlfile);

    CManager cm = CManager_create ();
    CMlisten_specific (cm, NULL);

    Transform t = parseXMLdata (xmlfile.c_str (), xmlfile.length ());
    data_struct d_in = generateDataStruct (t.src_ioc, t.src_root);
    data_struct d_out = generateDataStruct (t.dst_ioc, t.dst_root);

    // ---- procedure ----
    // create transform stone
    // create a split stone
    // create an output stone for each contact list
    // create filter and attach to transform stone
    // hook the split stone to each output stone
    // hook transform stone to split
    // print transform stone contact info
    // ---- procedure ----
    // if collecting metrics, add an input stone before transform stone
    // that sends to a handler fn that resubmits to the transform stone
    // after gathering whatever metrics it can.  Publish this stone instead.

    EVstone transform_stone = EValloc_stone (cm);
    EVstone split_stone = EValloc_stone (cm);

    // need to allocate an extra place and put a -1 in the last element
    // as an indicator of the EOL for the list of stones to split to...
    // argc = app xml cl cl cl cl ... so lop off the first two
    // but add one for the -1 terminator
    EVstone * output_stones = (EVstone *) malloc ((argc - 1) * sizeof (EVstone));
    memset (output_stones, 0, (argc - 1) * sizeof (EVstone));

    for (int i = 2; i < argc; i++)
    {
        int remote_stone = 0;

        sscanf (argv [i], "%d:", &remote_stone);
        char * list_str = strchr (argv [i], ':') + 1;
        attr_list contact_list = attr_list_from_string (list_str);

        EVstone output_stone = EValloc_stone (cm);
        EVassoc_output_action (cm, output_stone, contact_list, remote_stone);
        output_stones [i - 2] = output_stone;
    }
    output_stones [argc - 2] = -1; // the termination of the stones list

    EVaction split_action = EVassoc_split_action (cm, split_stone, output_stones);

    char * transform = create_transform_action_spec (d_in.data_subformat_list
                                                    ,d_out.data_subformat_list
                                                    ,(char *) t.filter.c_str ()
                                                    );
    EVaction transform_action = EVassoc_immediate_action (cm, transform_stone, transform, NULL);

    EVaction_set_output (cm, transform_stone, transform_action, 0, split_stone);

    EVstone input_stone = transform_stone;
// use 1 for metrics display, 0 for a staight through operation
#if 1
    input_stone = EValloc_stone (cm);
    EVsource source_handle = EVcreate_submit_handle (cm, transform_stone, d_in.data_subformat_list);
    EVassoc_terminal_action (cm, input_stone, d_in.data_subformat_list, handler, (void *) source_handle);
#endif

    char * string_list = "";
    attr_list contact_list = CMget_contact_list (cm);
    if (contact_list)
    {
        string_list = attr_list_to_string (contact_list);
    }

    printf ("Contact List \"%d:%s\"\n", input_stone, string_list);

    CMrun_network (cm);

    for (int i = 2; i < argc; i++)
    {
        EVfree_stone (cm, output_stones [i - 2]);
    }
    EVfree_stone (cm, split_stone);
    EVfree_stone (cm, transform_stone);
    free (output_stones);
    if (input_stone != transform_stone)
    {
        EVfree_stone (cm, input_stone);
    }

    return 0;
}
/*
 * $Log: inode.cc,v $
 * Revision 1.10  2005/09/15 20:51:23  lofstead
 * code cleanup (no functional changes)
 *
 */
