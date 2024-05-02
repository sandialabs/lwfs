/**  @file pablo_interface.C
 *   
 *   @brief Makes calls to the Pablo SDDF library to generate
 *          trace files. 
 *   
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 *   @version $Revision: 406 $
 *   @date $Date: 2005-10-07 15:08:29 -0600 (Fri, 07 Oct 2005) $
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <AsciiPipeWriter.h>
#include <BinaryPipeWriter.h>
#include <OutputFileStreamPipe.h>
#include <RecordDossier.h>
#include <StructureDescriptor.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "support/timer/timer.h"
#include "support/logger/logger.h"


/*------------ SUPPORT FOR INTERVAL EVENTS WITH PABLO ------------*/

const char *default_fname = "trace.sddf"; 
const int default_ftype = 0;  // binary

enum RecordTags {
    GENERIC_RECORD=1,
    COUNT_RECORD,
    INTERVAL_RECORD,
    THROUGHPUT_RECORD
};

/* The RecordDossier holds information about the 
 * structure of a record.  */
static RecordDossier *genericRecord = NULL; 
static RecordDossier *countRecord = NULL; 
static RecordDossier *intervalRecord = NULL; 
static RecordDossier *throughputRecord = NULL; 

/* Where to put the data */
static OutputFileStreamPipe *outFile = NULL;
static PipeWriter *pipeWriter = NULL;

/* When the trace library was initialized */
double starttime; 


/**
  * A generic event has the following fields
  *
  *	double timestamp;  
  *	int id;
  *	int pid;
  *	char *data;
  */
extern "C"
int define_generic_event(const int tag, PipeWriter *pipeWriter)
{
    Attributes *attributes = new Attributes(); 
    FieldDescriptor *fieldP; 
    RecordDossier *newDossier; 
    StructureDescriptor *structureP;

    attributes->clearEntries(); 
    attributes->insert("event", "Generic trace event");
    structureP = new StructureDescriptor("event", *attributes);

    /* double timestamp */
    attributes->clearEntries(); 
    attributes->insert("timestamp", "Time of event"); 
    attributes->insert("units", "sec"); 
    fieldP = new FieldDescriptor("timestamp", *attributes, DOUBLE, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* int id */
    attributes->clearEntries(); 
    attributes->insert("id", "Identifier"); 
    fieldP = new FieldDescriptor("id", *attributes, INTEGER, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* int pid */
    attributes->clearEntries(); 
    attributes->insert("pid", "Process/Thread identifier"); 
    fieldP = new FieldDescriptor("pid", *attributes, INTEGER, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* char *data */
    attributes->clearEntries(); 
    attributes->insert("data", "App-specific character data"); 
    fieldP = new FieldDescriptor("data", *attributes, CHARACTER, 1);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* Now we can write the structure descriptor to the output pipe
     * and create the RecordDossier for a generic event.  */
    pipeWriter->putDescriptor(*structureP, tag); 

    /* create the global genericRecord structure */
    genericRecord = new RecordDossier(tag, *structureP); 
    delete structureP; 
    delete attributes; 

    return 0; 
}


/**
  * A count event has is a generic event with an extra count field.
  *
  *	double timestamp;  
  *	long count;
  *	int id;
  *	int pid;
  *	char *data;
  */
extern "C"
int define_count_event(const int tag, PipeWriter *pipeWriter)
{
    Attributes *attributes = new Attributes(); 
    FieldDescriptor *fieldP; 
    RecordDossier *newDossier; 
    StructureDescriptor *structureP;

    attributes->clearEntries(); 
    attributes->insert("count", "Generic trace event");
    structureP = new StructureDescriptor("count", *attributes);

    /* double timestamp */
    attributes->clearEntries(); 
    attributes->insert("timestamp", "Time of event"); 
    attributes->insert("units", "sec"); 
    fieldP = new FieldDescriptor("timestamp", *attributes, DOUBLE, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* long count */
    attributes->clearEntries(); 
    attributes->insert("count", "Count"); 
    fieldP = new FieldDescriptor("count", *attributes, LONG, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* int id */
    attributes->clearEntries(); 
    attributes->insert("id", "Identifier"); 
    fieldP = new FieldDescriptor("id", *attributes, INTEGER, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* int pid */
    attributes->clearEntries(); 
    attributes->insert("pid", "Process/Thread identifier"); 
    fieldP = new FieldDescriptor("pid", *attributes, INTEGER, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* char *data */
    attributes->clearEntries(); 
    attributes->insert("data", "App-specific character data"); 
    fieldP = new FieldDescriptor("data", *attributes, CHARACTER, 1);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* Now we can write the structure descriptor to the output pipe
     * and create the RecordDossier for a generic event.  */
    pipeWriter->putDescriptor(*structureP, tag); 

    /* create the global countRecord structure */
    countRecord = new RecordDossier(tag, *structureP); 
    delete structureP; 
    delete attributes; 

    return 0; 
}


/**
  * A count event has is a generic event with an extra count field.
  *
  *	double timestamp;  
  *	int id;
  *	int pid;
  *	int level;
  *     double duration; 
  *	char *data;
  */
extern "C"
int define_interval_event(const int tag, PipeWriter *pipeWriter)
{
    Attributes *attributes = new Attributes(); 
    FieldDescriptor *fieldP; 
    RecordDossier *newDossier; 
    StructureDescriptor *structureP;

    attributes->clearEntries(); 
    attributes->insert("interval", "Interval event");
    structureP = new StructureDescriptor("interval", *attributes);

    /* double timestamp */
    attributes->clearEntries(); 
    attributes->insert("timestamp", "Time of event"); 
    attributes->insert("units", "sec"); 
    fieldP = new FieldDescriptor("timestamp", *attributes, DOUBLE, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* int id */
    attributes->clearEntries(); 
    attributes->insert("id", "Identifier"); 
    fieldP = new FieldDescriptor("id", *attributes, INTEGER, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* int pid */
    attributes->clearEntries(); 
    attributes->insert("pid", "Process/Thread identifier"); 
    fieldP = new FieldDescriptor("pid", *attributes, INTEGER, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* int level */
    attributes->clearEntries(); 
    attributes->insert("level", "Level/Scope"); 
    fieldP = new FieldDescriptor("level", *attributes, INTEGER, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* double duration */
    attributes->clearEntries(); 
    attributes->insert("duration", "Length of interval"); 
    attributes->insert("units", "sec"); 
    fieldP = new FieldDescriptor("duration", *attributes, DOUBLE, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* char *data */
    attributes->clearEntries(); 
    attributes->insert("data", "App-specific character data"); 
    fieldP = new FieldDescriptor("data", *attributes, CHARACTER, 1);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* Now we can write the structure descriptor to the output pipe
     * and create the RecordDossier for a generic event.  */
    pipeWriter->putDescriptor(*structureP, tag); 

    /* create the global intervalRecord structure */
    intervalRecord = new RecordDossier(tag, *structureP); 
    delete structureP; 
    delete attributes; 

    return 0; 
}


/**
  * A throughput event has the following fields
  *
  *	double timestamp;  
  *	int id;
  *	int pid;
  *	int level;
  *	char *data;
  *	double duration;
  *	long count;
  */
extern "C"
int define_throughput_event(const int tag, PipeWriter *pipeWriter)
{
    Attributes *attributes = new Attributes(); 
    FieldDescriptor *fieldP; 
    RecordDossier *newDossier; 
    StructureDescriptor *structureP;

    attributes->clearEntries(); 
    attributes->insert("throughput", "Throughput Event");
    structureP = new StructureDescriptor("throughput", *attributes);

    /* double timestamp */
    attributes->clearEntries(); 
    attributes->insert("timestamp", "Time of event"); 
    attributes->insert("units", "sec"); 
    fieldP = new FieldDescriptor("timestamp", *attributes, DOUBLE, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* int id */
    attributes->clearEntries(); 
    attributes->insert("id", "Identifier"); 
    fieldP = new FieldDescriptor("id", *attributes, INTEGER, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* int pid */
    attributes->clearEntries(); 
    attributes->insert("pid", "Process/Thread identifier"); 
    fieldP = new FieldDescriptor("pid", *attributes, INTEGER, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* int level */
    attributes->clearEntries(); 
    attributes->insert("level", "Level/Scope"); 
    fieldP = new FieldDescriptor("level", *attributes, INTEGER, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* double duration */
    attributes->clearEntries(); 
    attributes->insert("duration", "Length of interval"); 
    attributes->insert("units", "sec"); 
    fieldP = new FieldDescriptor("duration", *attributes, DOUBLE, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* long count */
    attributes->clearEntries(); 
    attributes->insert("count", "Number of objects processed"); 
    fieldP = new FieldDescriptor("count", *attributes, LONG, 0);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* char *data */
    attributes->clearEntries(); 
    attributes->insert("data", "App-specific character data"); 
    fieldP = new FieldDescriptor("data", *attributes, CHARACTER, 1);
    structureP->insert(*fieldP); 
    delete fieldP; 

    /* Now we can write the structure descriptor to the output pipe
     * and create the RecordDossier for a generic event.  */
    pipeWriter->putDescriptor(*structureP, tag); 

    /* create the global throughputRecord structure */
    throughputRecord = new RecordDossier(tag, *structureP); 
    delete structureP; 
    delete attributes; 

    return 0; 
}

static int initialized = 0; 

/**
 * @brief Initialize the pablo output interface.
 *
 * @param fname @input_type  The name of the outputfile.
 * @param type  @input_type  Type of file (0=binary, 1=ascii)
 *
 */
extern "C"
int pablo_interface_init(const char *fname, const int ftype)
{
    int rc = 0; 
    static const int bufsize = 204800; 

    if (!initialized) {
	/* get the current time */
	time_t now = time(NULL); 

	/* store the start time */
	starttime = lwfs_get_time(); 

	Attributes attributes; 

	/* Open file */
	outFile = new OutputFileStreamPipe(fname, bufsize);
	if (ftype) {
	    pipeWriter = new AsciiPipeWriter(outFile); 
	}
	else {
	    pipeWriter = new BinaryPipeWriter(outFile); 
	}

	/* Stream Attribute */
	attributes.clearEntries(); 
	attributes.insert("run date", ctime(&now));
	/* ... what else goes in the header? */
	pipeWriter->putAttributes(attributes);

	//output_header(pipeWriter); 

	/* ---- Describe the types of records we expect ---- */
	define_generic_event(GENERIC_RECORD, pipeWriter); 
	define_count_event(COUNT_RECORD, pipeWriter); 
	define_interval_event(INTERVAL_RECORD, pipeWriter); 
	define_throughput_event(THROUGHPUT_RECORD, pipeWriter); 

	initialized = 1; 
    }

    return rc; 
}

extern "C"
int pablo_interface_fini(void) 
{
    if (initialized) {
	fprintf(stderr, "pablo_interface_fini... complete\n");
	delete genericRecord; 
	delete countRecord; 
	delete throughputRecord; 
	delete intervalRecord; 
	delete pipeWriter; 
	delete outFile; 

	initialized = 0; 
    }
}

static int output_record(RecordDossier *rec)
{
    int rc = 0; 
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 

    if (!rec)
	return rc; 
    /* need to protect the putData function */
    pthread_mutex_lock(&mutex);
    pipeWriter->putData(*rec);
    pthread_mutex_unlock(&mutex);

    return rc; 
}

/**
 * @brief Output a generic trace event. 
 *
 * @param eventID @input_type  The ID of the trace.
 * @param pid     @input_type  Process ID. 
 * @param data    @input_type  User-defined data passed in a character string. 
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
extern "C"
int pablo_output_generic_event(
		const int eventID, 
		const int pid, 
		const char *data)
{
    int rc = 1;

    pablo_interface_init(default_fname, default_ftype);

    genericRecord->setValue("timestamp", lwfs_get_time() - starttime); 
    genericRecord->setValue("id", eventID);
    genericRecord->setValue("pid", pid);

    /*fprintf(stderr, "setting data to \"%s\"\n",data);*/
    genericRecord->setCString("data", data);



    output_record(genericRecord); 

    return rc; 
}



/**
 * @brief Output an interval event. 
 *
 * We use the generic pablo trace and encode the additional information 
 * we want in the data field.  The new data field will be, "interval:$name:duration".
 *
 * Pablo has its own interval records, but they are inadequate because it is 
 * difficult to measure concurrent intervals (e.g., in threads).  
 * A better (and more efficient) way to do this would be to create our own 
 * Pablo record type, but this is a quick "hack" to address our needs. 
 * 
 * @param eventID @input_type  The ID of the trace. 
 * @param pid     @input_type  Process ID. 
 * @param level   @input_type  
 * @param data    @input_type  User-defined data passed in a character 
 *                             string. 
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
extern "C"
int pablo_output_interval_event(
		const int eventID,
		const int pid, 
		const int level, 
		const char *data, 
		double duration)
{
    int rc = 1; 

    pablo_interface_init(default_fname, default_ftype);

    intervalRecord->setValue("timestamp", lwfs_get_time() - starttime); 
    intervalRecord->setValue("id", eventID);
    intervalRecord->setValue("pid", pid);
    intervalRecord->setValue("level", level);
    intervalRecord->setValue("duration", duration);

    /*fprintf(stderr, "setting data to \"%s\"\n",data);*/
    intervalRecord->setCString("data", data);

    output_record(intervalRecord);

    return rc; 
}

extern "C"
int pablo_output_tput_event(
		const int eventID,
		const int pid, 
		const int level, 
		const char *data, 
		double duration,
		const long count)
{
    int rc = 1; 

    pablo_interface_init(default_fname, default_ftype);

    throughputRecord->setValue("timestamp", lwfs_get_time() - starttime); 
    throughputRecord->setValue("id", eventID);
    throughputRecord->setValue("pid", pid);
    throughputRecord->setValue("level", level);
    throughputRecord->setValue("duration", duration);
    throughputRecord->setValue("count", count);

    /*fprintf(stderr, "setting data to \"%s\"\n",data);*/
    throughputRecord->setCString("data", data);

    output_record(throughputRecord);

    return rc; 
}


/**
 * @brief Output a count event. 
 *
 * We use the generic pablo trace and encode the additional information 
 * we want in the data field.  The new data field will be, "interval:$name:duration".
 *
 * Pablo has its own count records, but they are inadequate because they only
 * increment values.  We want to increment, decrement, and set count events. 
 * A better (and more efficient) way to do this would be to create our own 
 * Pablo record type, but this is a quick "hack" that will still work. 
 * 
 * @param intervalID @input_type The interval ID (unique for each interval).
 * @param eventID @input_type  The ID of the trace. 
 * @param data    @input_type  User-defined data passed in a character 
 *                             string. 
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
extern "C"
int pablo_output_count_event(
		const int eventID,
		const int pid, 
		const char *data, 
		const int count)
{
    int rc = 1; 

    pablo_interface_init(default_fname, default_ftype);

    countRecord->setValue("timestamp", lwfs_get_time() - starttime); 
    countRecord->setValue("id", eventID);
    countRecord->setValue("pid", pid);
    countRecord->setValue("count", count);

    /*fprintf(stdout, "setting count data to \"%s\"\n",data);*/
    countRecord->setCString("data", data);

    output_record(countRecord);

    return rc;
}


