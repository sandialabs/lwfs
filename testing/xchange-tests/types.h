/*
 * $Id: types.h,v 1.8 2005/10/14 13:45:27 lofstead Exp $
 */
#define BACKLOG 10
#define BUF_LEN 10000
#define PORT    4710

int count_newlines (char * s);

/***********************/

/* source type info */
typedef struct _etype
{
    int x;
} etype;

/* destination type info */
typedef struct _dtype
{
    float x;
} dtype;

/***********************/

/* source type info */
typedef struct
{
    int R;
    int G;
    int B;
} ColorPixel;

typedef struct
{
    ColorPixel pixel [480];
} ColorPixelLine;

typedef struct
{
    int spot [480];
} SrcBlock;

typedef struct
{
    char * name;
    ColorPixelLine line [640];
    SrcBlock block [640];
} ColorImage;

/* destination type info */
typedef struct
{
    int pixel [480];
} GreyscalePixelLine;

typedef struct
{
    int spot [240];
} DstBlock;

typedef struct
{
    char * name;
    GreyscalePixelLine line [640];
    DstBlock block [320];
} GreyscaleImage;

/***********************/

/* source type info */
typedef struct
{
    float r1;
    float r2;
    float r3;
    float r4;
    float r5;
    float r6;
    float r7;
    float r8;
} Specimen;

typedef struct
{
    char * patientid;
    int order;
    Specimen s1;
    Specimen s2;
} Result;

/* destination type info */
typedef struct
{
    float r1;
    float r2;
    float r3;
    float r4;
    float r5;
} SubPanel;

typedef struct
{
    char * patientid;
    int order;
    SubPanel p1;
    SubPanel p2;
    SubPanel p3;
} Panel;

/***********************/

/* source type info */
typedef struct
{
    char * patientid;
    int height;
    int weight;
} src_ADT;

/* destination type info */
typedef struct
{
    char * patientid;
    int height;
    int weight;
    float bmi;
} dst_ADT;
/*
 * $Log: types.h,v $
 * Revision 1.8  2005/10/14 13:45:27  lofstead
 * updated to include healthcare ADT/BMI example
 *
 * Revision 1.7  2005/09/15 20:51:24  lofstead
 * code cleanup (no functional changes)
 *
 */
