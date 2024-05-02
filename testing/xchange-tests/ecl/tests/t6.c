#include "ecl.h"
#include "malloc.h"
#include "assert.h"
#include <stdio.h>
#include <string.h>

typedef struct
{
  unsigned short x;
  unsigned short y;
  unsigned short z;
  unsigned short r;
  unsigned short g;
  unsigned short b;
} PipelinedPoint;

typedef struct
{
   int num_points;
   PipelinedPoint *polygon_points;
} PolygonPoints;
typedef struct
{
  int num_points;
  PolygonPoints* image_data;
} FrameData;

static IOField PipelinedPoint_field_list[] = {
 {"x", "integer", sizeof(unsigned short), IOOffset(PipelinedPoint*, x)},
 {"y", "integer", sizeof(unsigned short), IOOffset(PipelinedPoint*, y)},
 {"z", "integer", sizeof(unsigned short), IOOffset(PipelinedPoint*, z)},
 {"r", "integer", sizeof(unsigned short), IOOffset(PipelinedPoint*, r)},
 {"g", "integer", sizeof(unsigned short), IOOffset(PipelinedPoint*, g)},
 {"b", "integer", sizeof(unsigned short), IOOffset(PipelinedPoint*, b)},
 {NULL, NULL}
};
static IOField PolygonPoints_field_list[] = {
{"num_points", "integer", sizeof(int), IOOffset(PolygonPoints*, num_points)},
{"polygon_points", "PipelinedPoint[num_points]", sizeof(PipelinedPoint), IOOffset(PolygonPoints*, polygon_points)},
 {NULL, NULL}
};
static IOField FrameData_field_list[] = {
{"num_points", "integer", sizeof(int), IOOffset(FrameData*, num_points)},
{"image_data", "PolygonPoints[num_points]", sizeof(PolygonPoints), IOOffset(FrameData*, image_data)},
{NULL, NULL, 0 , 0}
};


int
main() 
{
    static char extern_string[] = "int printf(string format, ...);";
    static ecl_extern_entry externs[] =
    {
        {"printf", (void*)(long)printf},
        {(void*)0, (void*)0}
    };
    static char code[] = "{\n\
    FrameData *f;\n\
        output.num_points = 1;\
        output.image_data[0].num_points = 1;\
     }";
    FrameData data;
   
    ecl_parse_context context = new_ecl_parse_context();

    ecl_code gen_code;
    void (*func)(void*);

    ecl_assoc_externs(context, externs);
    ecl_parse_for_context(extern_string, context);

    ecl_add_struct_type("PipelinedPoint", PipelinedPoint_field_list, context);
    ecl_add_struct_type("PolygonPoints", PolygonPoints_field_list, context);
    ecl_add_struct_type("FrameData", FrameData_field_list, context);
    ecl_subroutine_declaration("int proc(FrameData *output)", context);
   
    gen_code = ecl_code_gen(code, context);
    func = (void (*)(void*))(long)gen_code->func;

    data.num_points = 0;
    data.image_data = NULL;
    func(&data);
    ecl_code_free(gen_code);
    ecl_free_parse_context(context);
    if ((data.num_points != 1) || (data.image_data[0].num_points != 1)) 
	return 1;
    return 0;
}
