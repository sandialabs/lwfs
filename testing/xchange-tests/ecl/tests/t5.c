#include "config.h"
#include "ecl.h"
#include "malloc.h"
#include "assert.h"
#include <stdio.h>
#include <string.h>

struct order_header {
    int order_id;
    int advertiser_number;
    int agency_number;
    int contract_number;
    int start_date;
    int end_date;
    int estimate_number;
    char class;
    char *product_name;
    int conflict_code1;
    int conflict_code2;
};

struct spot_week {
    unsigned char mo;
    unsigned char tu;
    unsigned char we;
    unsigned char th;
    unsigned char fr;
    unsigned char sa;
    unsigned char su;
};

struct spot {
    int order_item_id;
    unsigned char broadcast_week;
    char *program_id;
    short rate_section;
    short length;
    unsigned char bookend;
    int start_date;
    int end_date;
    int cost_per_spot;
    int spots_per_week;
    struct spot_week spots_per_day;
};

struct station_order {
    int station_id;
    char *task_name;
    char *salesperson_code;
    int order_id;
    int advertiser_number;
    int agency_number;
    int contract_number;
    int start_date;
    int end_date;
    int estimate_number;
    char class;
    char *product_name;
    int conflict_code1;
    int conflict_code2;
    int spot_count;
    struct spot *spots;
};

IOField order_header_fields[] = {
    {NULL, NULL, 0, 0}
};

IOField spot_week_fields[] = {
    {"MO", "unsigned integer", sizeof(unsigned char),
     IOOffset(struct spot_week *, mo)},
    {"TU", "unsigned integer", sizeof(unsigned char),
     IOOffset(struct spot_week *, tu)},
    {"WE", "unsigned integer", sizeof(unsigned char),
     IOOffset(struct spot_week *, we)},
    {"TH", "unsigned integer", sizeof(unsigned char),
     IOOffset(struct spot_week *, th)},
    {"FR", "unsigned integer", sizeof(unsigned char),
     IOOffset(struct spot_week *, fr)},
    {"SA", "unsigned integer", sizeof(unsigned char),
     IOOffset(struct spot_week *, sa)},
    {"SU", "unsigned integer", sizeof(unsigned char),
     IOOffset(struct spot_week *, su)},
    {NULL, NULL, 0, 0}
};

IOField spot_fields[] = {
    {"ORDER_ITEM_ID", "integer", sizeof(int), 
     IOOffset(struct spot *, order_item_id)},
    {"BROADCAST_WEEK", "integer", sizeof(unsigned char), 
     IOOffset(struct spot *, broadcast_week)},
    {"PROGRAM_ID", "string", sizeof(char *), 
     IOOffset(struct spot *, program_id)},
    {"RATE_SECTION", "integer", sizeof(short), 
     IOOffset(struct spot *, rate_section)},
    {"LENGTH", "integer", sizeof(short), 
     IOOffset(struct spot *, length)},
    {"BOOKEND", "integer", sizeof(char), 
     IOOffset(struct spot *, bookend)},
    {"START_DATE", "integer", sizeof(int),
     IOOffset(struct spot *, start_date)},
    {"END_DATE", "integer", sizeof(int),
     IOOffset(struct spot *, end_date)},
    {"COST_PER_SPOT", "integer", sizeof(int), 
     IOOffset(struct spot *, cost_per_spot)},
    {"SPOTS_PER_WEEK", "integer", sizeof(int), 
     IOOffset(struct spot *, spots_per_week)},
    {"SPOTS_PER_DAY", "spot_week", sizeof(struct spot_week), 
     IOOffset(struct spot *, spots_per_day)},
    {NULL, NULL, 0, 0}
};

IOField station_order_fields[] = {
    {"STATION_ID", "integer", sizeof(int), 
     IOOffset(struct station_order *, station_id)},
    {"TASK_NAME", "string", sizeof(char *), 
     IOOffset(struct station_order *, task_name)},
    {"SALESPERSON_CODE", "string", sizeof(char *), 
     IOOffset(struct station_order *, salesperson_code)},
    {"MO_ORDER_ID", "integer", sizeof(int),
     IOOffset(struct station_order *, order_id)},
    {"ADVERTISER_NO", "integer", sizeof(int),
     IOOffset(struct station_order *, advertiser_number)},
    {"AGENCY_NO", "integer", sizeof(int),
     IOOffset(struct station_order *, agency_number)},
    {"CONTRACT_NO", "integer", sizeof(int),
     IOOffset(struct station_order *, contract_number)},
    {"FLIGHT_START_DATE", "integer", sizeof(int),
     IOOffset(struct station_order *, start_date)},
    {"FLIGHT_END_DATE", "integer", sizeof(int),
     IOOffset(struct station_order *, end_date)},
    {"ESTIMATE_NO", "integer", sizeof(int),
     IOOffset(struct station_order *, estimate_number)},
    {"CLASS", "char", sizeof(char),
     IOOffset(struct station_order *, class)},
    {"PRODUCT_NAME", "string", sizeof(char *),
     IOOffset(struct station_order *, product_name)},
    {"CONFLICT_CODE_1", "integer", sizeof(int),
     IOOffset(struct station_order *, conflict_code1)},
    {"CONFLICT_CODE_2", "integer", sizeof(int),
     IOOffset(struct station_order *, conflict_code2)},
    {"SPOT_COUNT", "integer", sizeof(int), 
     IOOffset(struct station_order *, spot_count)},
    {"SPOTS", "spot[SPOT_COUNT]", sizeof(struct spot),
     IOOffset(struct station_order *, spots)},
    {NULL, NULL, 0, 0}
};

int
main()
{
    {
	static char extern_string[] = "int printf(string format, ...);";

#ifndef PRINTF_DEFINED
	extern int printf();
#endif
	static ecl_extern_entry externs[] = 
	{
	    {"printf", (void*)(long)printf},
	    {(void*)0, (void*)0}
	};
	/* test external call */
	static char code[] = "{\n\
	order_out.STATION_ID = order.STATION_ID;\n\
	order_out.TASK_NAME = order.TASK_NAME;\n\
	order_out.SPOT_COUNT = order.SPOT_COUNT;\n\
	order_out.SPOTS[1].BROADCAST_WEEK = 5;\n\
	order_out.SPOTS[1].PROGRAM_ID = order.SPOTS[1].PROGRAM_ID;\n\
	order_out.SPOT_COUNT = 4;\n\
	if (order.SPOTS[0].PROGRAM_ID == \"EM2\") {\n\
		order_out.SPOTS[0].PROGRAM_ID = \"My Favorite Martian\";\n\
	}\n\
	if (order.SPOTS[0].PROGRAM_ID == \"EM5\") {\n\
		order_out.SPOTS[1].PROGRAM_ID = \"Dance Fever\";\n\
	}\n\
	if (order.SPOTS[1].PROGRAM_ID == \"EM2\") {\n\
		order_out.SPOTS[2].PROGRAM_ID = \"Gilligan's Island\";\n\
	}\n\
	if (order.SPOTS[1].PROGRAM_ID == \"EM5\") {\n\
		order_out.SPOTS[3].PROGRAM_ID = \"I Dream of Genie\";\n\
	}\n\
		}";

	struct station_order order, out_order;
	struct spot spots[2];

	ecl_parse_context context = new_ecl_parse_context();

	ecl_code gen_code;
	void (*func)(void*, void*);

	order.station_id = 11514;
	order.task_name = "PollOrdersTask";
	order.salesperson_code = "2-30-3001";
	order.order_id = 1004;
	order.advertiser_number = 2701;
	order.agency_number = 1701;
	order.contract_number = 0;
	order.start_date = 61101;
	order.end_date = 61701;
	order.estimate_number = 0;
	order.class = 'L';
	order.product_name = "Fall Special";
	order.conflict_code1 = 275;
	order.conflict_code2 = 0;
	order.spot_count = 2;
    
	spots[0].order_item_id = 999;
	spots[0].broadcast_week = 1;
	spots[0].program_id = "EM5";
	spots[0].rate_section = 2;
	spots[0].length = 30;
	spots[0].bookend = 1;
	spots[0].start_date = 61101;
	spots[0].end_date = 61701;
	spots[0].cost_per_spot = 120;
	spots[0].spots_per_week = 10;
	spots[0].spots_per_day.mo = 5;
	spots[0].spots_per_day.tu = 2;
	spots[0].spots_per_day.we = 1;
	spots[0].spots_per_day.th = 0;
	spots[0].spots_per_day.fr = 2;
	spots[0].spots_per_day.sa = 0;
	spots[0].spots_per_day.su = 0;
    

	spots[1].order_item_id = 999;
	spots[1].broadcast_week = 2;
	spots[1].program_id = "EM2";
	spots[1].rate_section = 2;
	spots[1].length = 30;
	spots[1].bookend = 1;
	spots[1].start_date = 61101;
	spots[1].end_date = 61701;
	spots[1].cost_per_spot = 120;
	spots[1].spots_per_week = 10;
	spots[1].spots_per_day.mo = 5;
	spots[1].spots_per_day.tu = 2;
	spots[1].spots_per_day.we = 1;
	spots[1].spots_per_day.th = 0;
	spots[1].spots_per_day.fr = 2;
	spots[1].spots_per_day.sa = 0;
	spots[1].spots_per_day.su = 0;
    
	order.spots = (struct spot *)&spots;

	ecl_assoc_externs(context, externs);
	ecl_parse_for_context(extern_string, context);

	ecl_add_struct_type("spot_week", spot_week_fields, context);
	ecl_add_struct_type("spot", spot_fields, context);
	ecl_add_struct_type("station_order", station_order_fields, context);
	ecl_subroutine_declaration("void proc(station_order *order, station_order *order_out)", context);

	gen_code = ecl_code_gen(code, context);
	func = (void (*)(void*,void*))(long)gen_code->func;
	memset(&out_order, 0, sizeof(out_order));
	(func)((void*)&order, (void*)&out_order);
	if (out_order.station_id != order.station_id) {
	    fprintf(stderr, "Missed station_id\n");
	    return 1;
	}

	if (strcmp(out_order.task_name, order.task_name) != 0) {
	    fprintf(stderr, "Missed task_name\n");
	    return 1;
	}

	if (out_order.spot_count != 4) {
	    fprintf(stderr, "Missed spot_count\n");
	    return 1;
	}
	if (out_order.spots[0].program_id != NULL) {
	    fprintf(stderr, "Spots[0] not NULL\n");
	    return 1;
	}
	if (strcmp(out_order.spots[1].program_id, "Dance Fever") != 0) {
	    fprintf(stderr, "Spots[1] not right\n");
	    return 1;
	}
	if (strcmp(out_order.spots[2].program_id, "Gilligan's Island") != 0) {
	    fprintf(stderr, "Spots[2] not right\n");
	    return 1;
	}
	if (out_order.spots[3].program_id != NULL) {
	    fprintf(stderr, "Spots[3] not NULL\n");
	    return 1;
	}
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);
    }
    return 0;
}
