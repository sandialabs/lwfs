
/* 
 * !!!! THIS FILE IS MEANT TO BE INCLUDED DIRECTLY INTO THE SOURCE !!!
 *
 * The reason is that gengetopt_args_info is a different structure
 * depending on the options used to generate the command-line code.
 */

typedef struct {
    double val;
    double index;
} pair_t;


static void vmax(pair_t *in, pair_t *inout, int *len, MPI_Datatype *dptr) {
    int i;

    for (i=0; i<*len; ++i) {

	if ((*in).val > (*inout).val) {
	    (*inout).val = (*in).val;
	    (*inout).index = (*in).index;
	}
	in++;
	inout++;
    }
}

static void vmin(pair_t *in, pair_t *inout, int *len, MPI_Datatype *dptr) {
    int i;

    for (i=0; i<*len; ++i) {

	if ((*in).val < (*inout).val) {
	    (*inout).val = (*in).val;
	    (*inout).index = (*in).index;
	}
	in++;
	inout++;
    }
}

static void vsum(pair_t *in, pair_t *inout, int *len, MPI_Datatype *dptr) {
    int i;

    for (i=0; i<*len; ++i) {
	(*inout).val += (*in).val;
	(*inout).index += (*in).index;
	in++;
	inout++;
    }
}


static void print_stats(
        FILE *result_fp, 
        struct gengetopt_args_info *args_info, 
        double t_total,
	double t_open)
{
    int np = 1; 
    int myrank = 0;
    pair_t src; 
    pair_t t_total_min, t_total_max, t_total_avg;
    pair_t t_open_min, t_open_max, t_open_avg;
    MPI_Op min_op, max_op, sum_op;
    MPI_Datatype ctype; 

    MPI_Comm_size(MPI_COMM_WORLD, &np); 
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); 

    MPI_Type_contiguous(2, MPI_DOUBLE, &ctype);
    MPI_Type_commit(&ctype);
    MPI_Op_create((MPI_User_function *)vmax, 1, &max_op);
    MPI_Op_create((MPI_User_function *)vmin, 1, &min_op);
    MPI_Op_create((MPI_User_function *)vsum, 1, &sum_op);

    /* gather stats for total time */
    src.val = t_total; 
    src.index = myrank;

    MPI_Reduce(&src, &t_total_min, 1, ctype, min_op, 0, MPI_COMM_WORLD);
    MPI_Reduce(&src, &t_total_max, 1, ctype, max_op, 0, MPI_COMM_WORLD);
    MPI_Reduce(&src, &t_total_avg, 1, ctype, sum_op, 0, MPI_COMM_WORLD);

    /* gather stats for open */
    src.val = t_open; 
    src.index = myrank;

    MPI_Reduce(&src, &t_open_min, 1, ctype, min_op, 0, MPI_COMM_WORLD);
    MPI_Reduce(&src, &t_open_max, 1, ctype, max_op, 0, MPI_COMM_WORLD);
    MPI_Reduce(&src, &t_open_avg, 1, ctype, sum_op, 0, MPI_COMM_WORLD);


    if (myrank == 0) {
	static int first = 1; 
	t_total_avg.val = t_total_avg.val/np; 

	/* calculate the aggregate megabytes per operation */
	double mb = (double)(args_info->bytes_per_op_arg)*np/(1024*1024); 

	/* print the header */
	if (first) {
	    fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
	    fprintf(result_fp, "%s column   description\n", "%");
	    fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
	    fprintf(result_fp, "%s   0     number of clients\n", "%"); 
	    fprintf(result_fp, "%s   1     number of storage targets\n","%"); 
	    fprintf(result_fp, "%s   2     number of stripes/file\n","%"); 
	    fprintf(result_fp, "%s   3     number of operations\n","%"); 
	    fprintf(result_fp, "%s   4     aggregate MB/operation (MB-per_op) = np*bytes_per_op/(1024*1024)\n","%"); 
	    fprintf(result_fp, "%s   5     aggregate MB/trial  (MB_per_trial) = MB_per_op*num_ops\n","%"); 
	    fprintf(result_fp, "%s   6     aggregate throughput  (MB_per_sec) = MB_per_trial/total_max\n","%"); 
	    fprintf(result_fp, "%s   7     total_avg (s)\n","%"); 
	    fprintf(result_fp, "%s   8     total min (s)\n","%"); 
	    fprintf(result_fp, "%s   9     total_max (s)\n","%"); 
	    fprintf(result_fp, "%s   10    open_avg  (s)\n","%"); 
	    fprintf(result_fp, "%s   11    open_min  (s)\n","%"); 
	    fprintf(result_fp, "%s   12    open_max  (s)\n","%"); 
	    fprintf(result_fp, "%s ----------------------------------------------------\n", "%");

	    first = 0; 
	}

	/* print the row */
	fprintf(result_fp, "%05d   ", np);
	fprintf(result_fp, "%04d   ", args_info->num_ost_arg);
	fprintf(result_fp, "%04d   ", args_info->num_stripe_arg);
	fprintf(result_fp, "%03d   ", args_info->ops_per_trial_arg);  
	fprintf(result_fp, "%1.6e  ", mb);    
	fprintf(result_fp, "%1.6e  ", mb*args_info->ops_per_trial_arg);    
	fprintf(result_fp, "%1.6e  ", (mb*args_info->ops_per_trial_arg)/t_total_max.val);    
	fprintf(result_fp, "%1.6e  ", t_total_avg.val);
	fprintf(result_fp, "%1.6e  ", t_total_min.val);
	fprintf(result_fp, "%1.6e  ", t_total_max.val);
	fprintf(result_fp, "%1.6e  ", t_open_avg.val);
	fprintf(result_fp, "%1.6e  ", t_open_min.val);
	fprintf(result_fp, "%1.6e  ", t_open_max.val);
	fprintf(result_fp, "%05d\n", (int)t_total_max.index);

	fprintf(result_fp, "%s ----------------------------------------------------\n", "%");

	fflush(result_fp);
    }
}

