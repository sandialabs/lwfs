/*
 * $Id: server.cc,v 1.23 2005/10/14 13:45:27 lofstead Exp $
 */
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>

#include <netinet/in.h>

using namespace std;

#include "evpath.h"

#include "types.h"
#include "xml.h"

// see the function body below for details of these
int readData (int rs, string & filename, string & xmlfile);
int readResponse (int rs);

/*
 * Looping structure that accepts connections, parses the XML to
 * find the source file information and data type information,
 * creates the stream returning the ID to the client, and
 * streams the file to the client.
 *
 * Protocol:
 * 1. Server (this process) listens on port PORT (defined in
 *    types.h currently) for a TCP connection.
 * 2. Client connects to the server sending the following
 *    packet: <filepath>\n<xml size>\nxml
 * 3. Server parses the packet extracting the information.
 * 4. Server uses the parsed XML to create an event channel
 *    and returns that back to the client awaiting a response.
 * 5. Client parses the XML and uses it along with the returned
 *    channel ID to subscribe to the src channel, create the
 *    ECL to perform the filtering operation, and describe the
 *    newly derived data channel.  It then subscribes to the
 *    derived channel sedning an OK\n\n back to the server to
 *    signify that it is ready to receive.
 * 6. Server receives a packet (content doesn't matter) from the
 *    client and starts sending.
 * 7. When the server is done sending, it unsubscribes from the
 *    channel and destroys the local information.
 * 8. Server loops back to wait for another request.
 *
 * Currently there is a bug in the libxml library that prevents
 * this from operating as a loop.  Once a newer version of the
 * library becomes available, the looping will work properly.
 */
int main (int argc, char ** argv)
{
    if (argc < 3)
    {
        fprintf (stderr, "usage: %s <filename> <contact_list> ...\n", argv [0]);

        return -1;
    }

    string filename = argv [1];
    string xmlfile;

    readFile (filename, xmlfile);

    CManager cm = CManager_create ();
    // CMlisten_specific (cm, NULL);  // since we don't listen, no need

    // generate the format lists
    Transform t = parseXMLdata (xmlfile.c_str (), xmlfile.length ());
    data_struct d_in = generateDataStruct (t.src_ioc, t.src_root);
    data_struct d_out = generateDataStruct (t.dst_ioc, t.dst_root);

    // need to allocate an extra place and put a -1 in the last element
    // as an indicator of the EOL for the list of stones to split to...
    // argc = app xml cl cl cl cl ... so lop off the first two
    // but add one for the -1 terminator
    EVstone * output_stones = (EVstone *) malloc ((argc - 0) * sizeof(EVstone));
    memset (output_stones, 0, (argc - 1) * sizeof (EVstone));

    EVstone split_stone = EValloc_stone (cm);
    EVstone transform_stone = EValloc_stone (cm);

    // generate the remote_stone and contact_list from params
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
    EVaction transform_action = EVassoc_immediate_action (cm, transform_stone
                                                         ,transform, NULL);
    EVaction_set_output (cm, transform_stone, transform_action, 0, split_stone); 

// 1 for do a transform, 0 for no transform on server
#if 1
    EVsource source_handle = EVcreate_submit_handle (cm, transform_stone
                                                    ,d_in.data_subformat_list);
#else
    EVsource source_handle = EVcreate_submit_handle (cm, split_stone
                                                    ,d_in.data_subformat_list);
#endif

    // submit events
// 0 for scientific example
// 1 for healthcare lab example
// 2 for healthcare ADT example
const int example = 2;
switch (example)
{
case 0:
{
    for (int z = 0; z < 130; z++)
    {
        ColorImage ci;
        ci.name = "color";
        for (int i = 0; i < 640; i++)
        {
            for (int j = 0; j < 480; j++)
            {
                ci.line [i].pixel [j].R = 50;
                ci.line [i].pixel [j].G = 10;
                ci.line [i].pixel [j].B = 30;

                ci.block [i].spot [j] = 1;
            }
        }

        EVsubmit (source_handle, &ci, NULL);

        CMsleep (cm, 1);
    }
}
break;
case 1:
{
    //for (int z = 0; z < 101000; z++)
    for (int z = 0; z < 10; z++)
    {
        Result r;

        r.patientid = "123456";
        r.order = z;
        r.s1.r1 = 1.0;
        r.s1.r2 = 2.0;
        r.s1.r3 = 3.0;
        r.s1.r4 = 4.0;
        r.s1.r5 = 5.0;
        r.s1.r6 = 6.0;
        r.s1.r7 = 7.0;
        r.s1.r8 = 8.0;
        r.s2.r1 = 11.0;
        r.s2.r2 = 12.0;
        r.s2.r3 = 13.0;
        r.s2.r4 = 14.0;
        r.s2.r5 = 15.0;
        r.s2.r6 = 16.0;
        r.s2.r7 = 17.0;
        r.s2.r8 = 18.0;

        EVsubmit (source_handle, &r, NULL);
    }
}
break;
case 2:
{
    for (int z = 0; z < 10; z++)
    {
        src_ADT adt;

        adt.patientid = "12asd";
        adt.height = 68;
        adt.weight = 175;

        EVsubmit (source_handle, &adt, NULL);
    }
}
break;
}

    CManager_close (cm);

    return 0;
}

/*****************************************************************************/
/* old server main                                                           */
/*****************************************************************************/
int automated_main ()
{
printf ("currently commented out pending rework with EVPath\n");
/*
    // These four are the connection information for ECho
    CManager cm;
    EControlContext cc;
    EChannel src_channel;
    ECSourceHandle handle;

    // initialize echo and create the connection context
    cm = CManager_create ();
    cc = ECho_CM_init (cm);

    // start the socket loop
    int ss;           // server socket
    int rs;           // receive socket
    int transmitted;  // number of bytes transmitted

    // sending socket information
    struct sockaddr_in sa;
    int sa_len = sizeof (sa);

    // receiving socket information
    struct sockaddr_in sr;
    int sr_len = sizeof (sr);

    // get a socket
    ss = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ss == -1)
    {
        fprintf (stderr, "socket request refused\n");
        exit (-1);
    }

    // make the socket an incoming socket
    sa.sin_family = AF_INET;
    sa.sin_port = htons (PORT);
    sa.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (ss, (const struct sockaddr *) &sa, sa_len))
    {
        fprintf (stderr, "bind request refused\n");
        close (ss);
        exit (-1);
    }

    // make the socket able to queue requests
    if (listen (ss, BACKLOG))
    {
        fprintf (stderr, "listen request refused\n");
        close (ss);
        exit (-1);
    }

    // keep looping while we can get network connections.
    while ((rs = accept (ss
                        ,(struct sockaddr *) &sr
                        ,(socklen_t *) &sr_len)
           ) != -1
          )
    {
        char buf [BUF_LEN];      // message buffer
        int buf_len;             // buffer length
        char * buf_ptr;          // used to advance the pointer during reads
        memset (buf, 0, BUF_LEN);

        // get the message from the incoming socket
        string filename;
        string xmlfile;
        
        if (readData (rs, filename, xmlfile) == -1)
        {
            fprintf (stderr, "error during receive from server\n");
            close (rs);
            continue;
        }

        Transform t = parseXMLdata (xmlfile.c_str (), xmlfile.length ());
        ECdata_struct d = generateDataStruct (t.src_ioc, t.src_root);

        src_channel = EChannel_typed_create (cc
                                            ,d.data_field_list
                                            ,d.data_subformat_list
                                            );
        handle = ECsource_typed_subscribe (src_channel
                                          ,d.data_field_list
                                          ,d.data_subformat_list
                                          );

        //printf ("Channel ID is %s\n", ECglobal_id (src_channel));

        // send the response to the client
        sprintf (buf, "%s\n\n", ECglobal_id (src_channel));
        buf_len = strlen (buf);
        transmitted = send (rs, buf, buf_len, 0);
        if (transmitted != buf_len)
        {
            fprintf (stderr, "error sending response to client\n");
            close (rs);
            continue;
        }
        CMsleep (cm, 1); // give things a chance to clear or it doesn't work

        if (readResponse (rs) == -1)
        {
            fprintf (stderr, "error during receive from server\n");
            close (rs);
            continue;
        }

        // close the incoming socket
        close (rs);

        // This loop should be replaced with an appropriate file reader or
        // other such code appropriate for the server.  While it would be
        // most appropriate for this to be just a loop that ends, the truth
        // is that the XML parser gets confused on the XML file the second
        // time through.  Until a less buggy library can be obtained, this
        // will have to be either an infinite loop or a single connection
        // and exit.
        while (1)
        //for (int x = 0; x < 10; x++)
        {
            ColorImage ci;
            ci.name = "color";
            for (int i = 0; i < 640; i++)
            {
                for (int j = 0; j < 480; j++)
                {
                    ci.line [i].pixel [j].R = 50;
                    ci.line [i].pixel [j].G = 10;
                    ci.line [i].pixel [j].B = 30;

                    ci.block [i].spot [j] = 1;
                }
            }

            printf ("submitting event\n");
            ECsubmit_typed_event (handle, &ci);

            CMsleep (cm, 1);
        }

        // cleanup and wait for next request
        ECcancel_source_subscribe (handle);
        EChannel_destroy (src_channel);
    }

    // close the outgoing socket
    close (ss);
*/

    return 1;
}

/*
 * This reads the first data packet from the client extracting out the
 * filename parameter and the xml data stream.  This code is complex and
 * long enough to warrant extraction into a separate location.
 */
int readData (int rs, string & filename, string & xmlfile)
{
    char buf [BUF_LEN];
    int buf_len;
    char * buf_ptr;
    enum STATE {START  // have not seen the end of the filename/size yet
               ,MIDDLE // have not seen the end of the xml stream yet
               ,END    // read all there is to read
               };
    STATE state = START;
    long transmitted;
    long size;

    memset (buf, 0, BUF_LEN);
    buf_len = BUF_LEN;
    buf_ptr = buf;

    // read the buffer first looking for the two newlines that would
    // indicate that both the filename and the size have been read.  Then
    // parse those pieces to get a size that indicates how long the xml
    // stream is.  Continue reading data from the socket until the length
    // specified in the size has been read.  This will be the entire XML file.
    while (state != END || (state == MIDDLE && size > 0))
    {
        // receive bytes
        transmitted = recv (rs, buf_ptr, buf_len - 1, 0);
        if (transmitted == -1)
        {
            return -1;
        }

        // validate the number of newlines read so far
        if (state == START && count_newlines (buf) >= 2)
        {
            state = MIDDLE;
            char * temp;
            char * temp1;

            temp = strchr (buf, '\n');
            filename.append (buf, temp - buf);
            temp1 = strchr (temp + 1, '\n');
            *temp1 = '\0';
            size = atol (temp + 1);
            xmlfile.append (temp1 + 1);
            size -= xmlfile.length ();
            if (size == 0)
            {
                state = END;
            }
            else
            {
                buf_ptr = buf;
                memset (buf, 0, transmitted + 1);
            }
        }
        else  // move the start of the buffer if we haven't read two yet
        {
            if (state == START)
            {
                buf_ptr += transmitted;
                buf_len -= transmitted;
            }
            else
            {
                xmlfile.append (buf);
                size -= transmitted;
                if (size > 0)
                {
                    memset (buf, 0, transmitted + 1);
                }
                else
                {
                    state = END;
                }
            }
        }
    }

    return 1;
}

/*
 * Read until two newlines have been sent.  This will be a marker for success.
 * There is no marker for a failure on the client side.  This is a
 * consideration for future robustness enhancements (beyond scope for this
 * initial work).
 */
int readResponse (int rs)
{
    char buf [BUF_LEN];
    int buf_len;
    char * buf_ptr;
    bool done = false;
    int transmitted;

    // get the message from the incoming socket
    memset (buf, 0, BUF_LEN);
    buf_len = BUF_LEN;
    buf_ptr = buf;

    // read until we get two newlines
    while (!done)
    {
        // receive bytes
        transmitted = recv (rs, buf_ptr, buf_len, 0);
        if (transmitted == -1)
        {
            return -1;
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

    return 0;
}
/*
 * $Log: server.cc,v $
 * Revision 1.23  2005/10/14 13:45:27  lofstead
 * updated to include healthcare ADT/BMI example
 *
 * Revision 1.22  2005/09/15 20:51:24  lofstead
 * code cleanup (no functional changes)
 *
 */
