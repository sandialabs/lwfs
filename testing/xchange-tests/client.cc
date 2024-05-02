/*
 * $Id: client.cc,v 1.18 2005/10/14 13:45:27 lofstead Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <sys/times.h>
#include <unistd.h>

using namespace std;

#include "evpath.h"

#include "types.h"
#include "xml.h"

/*
 * This handler function is about as simple as they come.  Notice that the
 * event can be cast into the appropriate data type defined elsewhere (the
 * only type information that must be explicitly defined for the system to
 * work).  This will allow typesafe access to the binary data stream.  The
 * client_data is the paramter that was passed into the startFeed call.
 * The attrs is an ECho parameter that can generally be ignored except for
 * specific ECho uses.
 */
int handler (CManager cm, void * message, void * client_data, attr_list attrs)
{
// 1 for print time info, 0 for no time info output
#if 1
    printf ("%d\n", times (NULL));
#endif

// 0 for scientific example
// 1 for healthcare lab example
// 2 for healthcare ADT example
    const int example = 0;
switch (example)
{
case 0:
{
    GreyscaleImage * gi = (GreyscaleImage *) message;

    printf ("event data: %s\n", gi->name);
}
break;
case 1:
{
    Panel * p = (Panel *) message;

    printf ("event data: %s\n", p->patientid);
}
break;
case 2:
{
    dst_ADT * adt = (dst_ADT *) message;

    printf ("BMI: %f\n", adt->bmi);
}
break;
}
    if (attrs)
        dump_attr_list (attrs);

    return 0;
}

/* this handler will dump the size metrics and then resubmit for normal processing */
int metrics_handler (CManager cm, void * message, void * client_data, attr_list attrs)
{
    if (attrs)
    {
        dump_attr_list (attrs);
    }

    EVsubmit ((EVsource) client_data, message, NULL);

    return 0;
}

/*
 * Client side template for a derived event channel from an XML
 * described transformation.  See server.cc for details on the
 * protocol.
 *
 * To get a derived, typed stream:
 * 1. create a handler function (handler function above)
 * 2. create a CManager to manage the connections for this process
 * 3. create a EControlContext to hold the context for this stream
 * 4. call startFeed (...) to start the flow of data
 * 5. loop in a CM friendly way, like CMsleep, to allow network traffic
 *    and therefore the feed of data to start trickling into the app.
 *
 * This would be an appropriate place to put in threads or other such
 * changes, such as part of a GUI application event loop.  Details of
 * what to do and how are left to the client application writer.
 * Notice that it is not necessary to clean up the connection information
 * explicitly.  By the process exiting, it will clean up all of that
 * automatically.
 */
int main (int argc, char ** argv)
{
    if (argc < 2)
    {
        fprintf (stderr, "usage: %s <filename>\n", argv [0]);

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

    EVstone client_stone = EValloc_stone (cm);
    EVassoc_terminal_action (cm, client_stone, d_out.data_subformat_list, handler, NULL);

    EVstone input_stone = client_stone;
    CMFormatList incoming_type = d_out.data_subformat_list;
// use 1 for local transform, 0 for no local transform
#if 0
    EVstone transform_stone = EValloc_stone (cm);
    char * transform = create_transform_action_spec (d_in.data_subformat_list
                                                    ,d_out.data_subformat_list
                                                    ,(char *) t.filter.c_str ()
                                                    );
    EVaction transform_action = EVassoc_immediate_action (cm, transform_stone, transform, NULL);
    EVaction_set_output (cm, transform_stone, transform_action, 0, client_stone);
    input_stone = transform_stone;
    incoming_type = d_in.data_subformat_list;
#endif

// use 1 for collect metrics, 0 for no metrics
#if 1
    EVstone new_input_stone = EValloc_stone (cm);
    EVsource source_handle = EVcreate_submit_handle (cm, input_stone, incoming_type);
    EVassoc_terminal_action (cm, new_input_stone, incoming_type, metrics_handler, (void *) source_handle);
    input_stone = new_input_stone;
#endif

    char * string_list = "";
    attr_list contact_list = CMget_contact_list (cm);
    if (contact_list)
    {
        string_list = attr_list_to_string (contact_list);
    }

    fprintf (stderr, "ticks per second: %d\n", sysconf (_SC_CLK_TCK));
    fprintf (stderr, "Contact List \"%d:%s\"\n", input_stone, string_list);

    // run continuously.
    CMrun_network (cm);

    // exit
    CManager_close (cm);

    return 0;
}
/*
 * $Log: client.cc,v $
 * Revision 1.18  2005/10/14 13:45:27  lofstead
 * updated to include healthcare ADT/BMI example
 *
 * Revision 1.17  2005/09/15 20:51:23  lofstead
 * code cleanup (no functional changes)
 *
 */
