/**
 * @file skim.c
 * @brief skim through a S/BLOW5 file
 * @author Hasindu Gamaarachchi (hasindu@garvan.org.au)
 * @date 26/08/2021
 */

#include <getopt.h>
#include "error.h"
#include "cmd.h"
#include "misc.h"
#include "thread.h"
#include <slow5/slow5.h>
#include "slow5_misc.h"

#define USAGE_MSG "Usage: %s [OPTIONS] [SLOW5_FILE]\n"
#define HELP_LARGE_MSG \
    "Skims through requested components in a SLOW5/BLOW5 file."\
    "If no options are provided, all the SLOW5 records except the raw signal will be printed.\n" \
    USAGE_MSG \
    "\n" \
    "OPTIONS:\n" \
    HELP_MSG_THREADS \
    HELP_MSG_BATCH \
    "    --hdr              		  print the header only\n" \
    "    --rid              		  print the list of read ids only\n" \
    HELP_MSG_HELP \

extern int slow5tools_verbosity_level;


static void print_rid(slow5_file_t* sp){
    int ret=0;
    ret = slow5_idx_load(sp);
    if(ret<0){
        fprintf(stderr,"Error in loading index\n");
        exit(EXIT_FAILURE);
    }

    uint64_t num_reads = 0;
    char **read_ids = slow5_get_rids(sp, &num_reads);
    if(read_ids==NULL){
        fprintf(stderr,"Error in getting list of read IDs\n");
        exit(EXIT_FAILURE);
    }

    for(uint64_t i=0; i<num_reads; i++) {
        printf("%s\n",read_ids[i]);
    }

    slow5_idx_unload(sp);
}

static void print_hdr(slow5_file_t* sp){
    slow5_press_method_t press_method = {SLOW5_COMPRESS_NONE,SLOW5_COMPRESS_NONE};
    slow5_hdr_print(sp->header,SLOW5_FORMAT_ASCII,press_method);
}

struct aux_print_param {
    slow5_file_t *sp;
    slow5_rec_t *rec;
    char* field;
    char* buff;
    size_t c;
    size_t n;
};

/* Following can be docorated by some macro magic, but this is easier for now
   Also, another option is directly accessing khash and using slow5_data_to_str
*/

static inline void cpy_str(struct aux_print_param *p, uint64_t len,const char *str){
    if(p->c-p->n-3 <= len){ // 3 is for '\t' and later '\n', '\0'
        p->c *= 2;
        p->buff = (char *)realloc(p->buff,p->c * sizeof(char));
        MALLOC_CHK(p->buff);
    }
    p->buff[p->n++] = '\t';
    memcpy(p->buff+p->n,str,len);
    p->n += len;
}
static void channel_number_print(struct aux_print_param *p){ //char*
    int ret=0;
    uint64_t len;
    char* str = slow5_aux_get_string(p->rec, p->field, &len, &ret);
    if(ret!=0){
        fprintf(stderr,"Error in getting auxiliary field %s from the file. Error code %d\n",p->field,ret);
        exit(EXIT_FAILURE);
    }
    if(str!=NULL){
        cpy_str(p,len,str);
    } else{
        assert(len==0);
        cpy_str(p,1,".");
    }

}
static void median_before_print(struct aux_print_param *p){ //double
    int ret=0;
    double t = slow5_aux_get_double(p->rec, p->field, &ret);
    if(ret!=0){
        fprintf(stderr,"Error in getting auxiliary field %s from the file. Error code %d\n",p->field,ret);
        exit(EXIT_FAILURE);
    }
    if(!isnan(t)){  //SLOW5_DOUBLE_NULL is the generic NaN value returned by nan("""") and thus t != SLOW5_DOUBLE_NULL is not correct
        size_t len;
        char *str = slow5_double_to_str(t, &len);
        cpy_str(p,len,str);
        free(str);
    } else {
        cpy_str(p,1,".");
    }

}
static void read_number_print(struct aux_print_param *p){ //int32_t
    int ret=0;
    int32_t t = slow5_aux_get_int32(p->rec, p->field, &ret);
    if(ret!=0){
        fprintf(stderr,"Error in getting auxiliary field %s from the file. Error code %d\n",p->field,ret);
        exit(EXIT_FAILURE);
    }
    if(t != SLOW5_INT32_T_NULL){
        char *str = NULL;
        int len = slow5_asprintf(&str,"%" PRId32 ,t);
        cpy_str(p,len,str);
        free(str);
    } else {
        cpy_str(p,1,".");
    }

}
static void start_mux_print(struct aux_print_param *p){ //uint8_t
    int ret=0;
    uint8_t t = slow5_aux_get_uint8(p->rec, p->field, &ret);
    if(ret!=0){
        fprintf(stderr,"Error in getting auxiliary field %s from the file. Error code %d\n",p->field,ret);
        exit(EXIT_FAILURE);
    }
    if(t != SLOW5_UINT8_T_NULL){
        char *str = NULL;
        int len = slow5_asprintf(&str,"%" PRIu8 ,t);
        cpy_str(p,len,str);
        free(str);
    } else {
        cpy_str(p,1,".");
    }

}
static void start_time_print(struct aux_print_param *p){ //uint64_t
    int ret=0;
    uint64_t t = slow5_aux_get_uint64(p->rec, p->field, &ret);
    if(ret!=0){
        fprintf(stderr,"Error in getting auxiliary field %s from the file. Error code %d\n",p->field,ret);
        exit(EXIT_FAILURE);
    }
    if(t != SLOW5_UINT64_T_NULL){
        char *str = NULL;
        int len = slow5_asprintf(&str,"%" PRIu64 ,t);
        cpy_str(p,len,str);
        free(str);
    } else {
        cpy_str(p,1,".");
    }
}
static void end_reason_print(struct aux_print_param *p){ //uint8_t
    int ret=0;
    uint8_t t = slow5_aux_get_enum(p->rec, p->field, &ret);
    if(ret!=0){
        fprintf(stderr,"Error in getting auxiliary field %s from the file. Error code %d\n",p->field,ret);
        exit(EXIT_FAILURE);
    }
    if(t != SLOW5_ENUM_NULL){
        uint8_t num_label = 0;
        char **labels = slow5_get_aux_enum_labels(p->sp->header, "end_reason", &num_label);
        if(labels==NULL){
            fprintf(stderr,"Error in getting list of enum labels\n");
            exit(EXIT_FAILURE);
        }
        char *str = labels[t];
        int len = strlen(str);
        cpy_str(p,len,str);
    } else {
        cpy_str(p,1,".");
    }
}


static void just_the_dot(struct aux_print_param *p){
    cpy_str(p,1,".");
}
static void (*aux_print_func(char *field))(struct aux_print_param *p){
    void (*aux_func)(struct aux_print_param *p) = NULL;
    if(strcmp(field,"channel_number")==0){ //char*
        aux_func = channel_number_print;
    } else if(strcmp(field,"median_before")==0){ //double
        aux_func = median_before_print;
    } else if(strcmp(field,"read_number")==0){ //int32_t
        aux_func = read_number_print;
    } else if(strcmp(field,"start_mux")==0){ // uint8_t
        aux_func = start_mux_print;
    } else if(strcmp(field,"start_time")==0){ //uint64_t
        aux_func = start_time_print;
    } else if(strcmp(field,"end_reason")==0){ //int8_t
        aux_func = end_reason_print;
    } else{
        aux_func = just_the_dot;
        WARNING("Field '%s' is not yet handled or not present in the input file. A '.' will be printed\n",field);
    }
    return aux_func;
}

typedef struct {
    struct aux_print_param p;
    char **aux;
    uint64_t num_aux;
    void (**aux_func)(struct aux_print_param *);
} skim_param_t;

static char* process_read2(slow5_rec_t *rec, struct aux_print_param p, char **aux, uint64_t num_aux, void (**aux_func)(struct aux_print_param *)){
    char *mem = NULL;
    char *digitisation_str = slow5_double_to_str(rec->digitisation, NULL);
    char *offset_str = slow5_double_to_str(rec->offset, NULL);
    char *range_str = slow5_double_to_str(rec->range, NULL);
    char *sampling_rate_str = slow5_double_to_str(rec->sampling_rate, NULL);

    int curr_len_tmp = slow5_asprintf(&mem,"%s\t%" PRIu32 "\t%s\t%s\t%s\t%s\t%" PRIu64 "\t.",
            rec->read_id, rec->read_group, digitisation_str, offset_str, range_str, sampling_rate_str, rec->len_raw_signal);

    free(digitisation_str);
    free(offset_str);
    free(range_str);
    free(sampling_rate_str);
    if (curr_len_tmp <= 0) {
        fprintf(stderr, "Error in printing read record\n");
        exit(EXIT_FAILURE);
    }
    size_t c = curr_len_tmp+1024;
    char *buff = (char *) malloc(c* sizeof(char));
    MALLOC_CHK(buff);
    size_t n=curr_len_tmp;
    strcpy(buff,mem);
    free(mem);

    if(aux != NULL){
        p.rec = rec;
        p.buff = buff;
        p.c = c;
        p.n = n;

        for(uint64_t i=0; i<num_aux; i++) {
            p.field = aux[i];
            if(aux_func[i] !=NULL) aux_func[i](&p);
        }
        n=p.n;
        c=p.c;
    }
    assert(c-2>=n);
    buff[n] = '\n';
    buff[n+1] = '\0';

    return buff;

}


void process_read(core_t *core, db_t *db, int32_t i) {
    //
    struct slow5_rec *read = NULL;
    char *record = db->mem_records[i];
    if (slow5_decode(&record, &db->mem_bytes[i], &read, core->fp) < 0 ) {
        exit(EXIT_FAILURE);
    } else {
        free(record);
    }

    skim_param_t *param = (skim_param_t *) core->param;

    struct aux_print_param p = param->p;
    char **aux = param->aux;
    uint64_t num_aux  = param->num_aux;
    void (**aux_func)(struct aux_print_param *) = param->aux_func;

    db->read_record[i].buffer = process_read2(read,p,aux,num_aux,aux_func);
    slow5_rec_free(read);
}

static void skim_data_parallel(slow5_file_t* sp,size_t num_threads, int64_t batch_size){
    int ret = 0;
    slow5_rec_t *rec = NULL;

    uint64_t num_aux = 0;
    char **aux = slow5_get_aux_names(sp->header, &num_aux);
    void (**aux_func)(struct aux_print_param *) = NULL;
    struct aux_print_param p;
    p.sp = sp;

    if(aux!=NULL){
        aux_func = (void ((**)(struct aux_print_param *)))malloc(sizeof(void (*)(struct aux_print_param *))*num_aux);
        MALLOC_CHK(aux_func);

        for(uint64_t i=0; i<num_aux; i++){
            aux_func[i] = aux_print_func(aux[i]);
        }
    }

    printf("#read_id\tread_group\tdigitisation\toffset\trange\tsampling_rate\tlen_raw_signal\traw_signal");
    for(uint64_t i=0; i<num_aux; i++) {
        printf("\t%s",aux[i]);
    }
    printf("\n");

    double time_get_to_mem = 0;
    double time_thread_execution = 0;
    double time_write = 0;
    int flag_end_of_file = 0;

    skim_param_t param;
    param.p = p;
    param.aux = aux;
    param.num_aux = num_aux;
    param.aux_func = aux_func;

    while(1) {

        db_t db = { 0 };
        db.mem_records = (char **) malloc(batch_size * sizeof(char*));
        db.mem_bytes = (size_t *) malloc(batch_size * sizeof(size_t));
        MALLOC_CHK(db.mem_records);
        MALLOC_CHK(db.mem_bytes);
        int64_t record_count = 0;
        size_t bytes;
        char *mem = NULL;
        double realtime = slow5_realtime();
        while (record_count < batch_size) {
            if ((ret = slow5_get_next_bytes(&mem,&bytes,sp)) <0) {
                if (slow5_errno != SLOW5_ERR_EOF) {
                    exit(EXIT_FAILURE);
                } else {
                    flag_end_of_file = 1;
                    break;
                }
            } else {
                db.mem_records[record_count] = (char *)mem;
                db.mem_bytes[record_count] = bytes;
                record_count++;
            }
        }
        time_get_to_mem += slow5_realtime() - realtime;

        realtime = slow5_realtime();
        // Setup multithreading structures
        core_t core;
        core.num_thread = num_threads;
        core.fp = sp;
        core.param = &param;

        db.n_batch = record_count;
        db.read_record = (raw_record_t*) malloc(record_count * sizeof *db.read_record);
        MALLOC_CHK(db.read_record);
        work_db(&core,&db,process_read);
        time_thread_execution += slow5_realtime() - realtime;

        realtime = slow5_realtime();
        for (int64_t i = 0; i < record_count; i++) {
            char *buff = (char *)db.read_record[i].buffer;
            printf("%s", buff);
            free(buff);
        }
        time_write += slow5_realtime() - realtime;

        // Free everything
        free(db.mem_bytes);
        free(db.mem_records);
        free(db.read_record);

        if(flag_end_of_file == 1){
            break;
        }

    }

    DEBUG("time_get_to_mem\t%.3fs", time_get_to_mem);
    DEBUG("time_skim\t%.3fs", time_thread_execution);
    DEBUG("time_write\t%.3fs", time_write);

    free(aux_func);
    if(ret != SLOW5_ERR_EOF){  //check if proper end of file has been reached
        fprintf(stderr,"Error in slow5_get_next. Error code %d\n",ret);
        exit(EXIT_FAILURE);
    }
    slow5_rec_free(rec);

}

int skim_main(int argc, char **argv, struct program_meta *meta){

    // Debug: print arguments
    print_args(argc,argv);

    WARNING("%s","slow5tools is experiemental. Use with caution. Report any bugs under GitHub issues");

    static struct option long_opts[] = {
            {"help", no_argument, NULL, 'h' }, //0
            {"rid", no_argument, NULL, 0 },    //1
            {"hdr", no_argument, NULL, 0 }, //2
            {"threads",required_argument,  NULL, 't' }, //3
            {"batchsize",required_argument, NULL, 'K'}, //4
            {NULL, 0, NULL, 0 }
    };

    opt_t user_opts;
    init_opt(&user_opts);

    // Input arguments
    int longindex = 0;
    int opt;
    int rid=0;
    int hdr=0;

    // Parse options
    while ((opt = getopt_long(argc, argv, "ht:K:", long_opts, &longindex)) != -1) {
        DEBUG("opt='%c', optarg=\"%s\", optind=%d, opterr=%d, optopt='%c'",
                  opt, optarg, optind, opterr, optopt);
        switch (opt) {
            case 'h':
                DEBUG("displaying large help message%s","");

                fprintf(stdout, HELP_LARGE_MSG, argv[0]);

                EXIT_MSG(EXIT_SUCCESS, argv, meta);
                exit(EXIT_SUCCESS);
            case 'K':
                user_opts.arg_batch = optarg;
                break;
            case 't':
                user_opts.arg_num_threads = optarg;
                break;
            case 0  :
                switch (longindex) {
                    case 1:
                        rid = 1;
                        break;
                    case 2:
                        hdr = 2;
                        break;
                    default:
                        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                        EXIT_MSG(EXIT_FAILURE, argv, meta);
                        return EXIT_FAILURE;
                }
                break;
            default: // case '?'
                fprintf(stderr, HELP_SMALL_MSG, argv[0]);
                EXIT_MSG(EXIT_FAILURE, argv, meta);
                return EXIT_FAILURE;
        }
    }

    if(parse_num_threads(&user_opts,argc,argv,meta) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }
    if(parse_batch_size(&user_opts,argc,argv) < 0){
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        return EXIT_FAILURE;
    }

    if (argc - optind < 1){
        ERROR("%s", "Not enough arguments");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        exit(EXIT_FAILURE);
    }

    if (argc - optind > 1){
        ERROR("%s", "Too many arguments");
        fprintf(stderr, HELP_SMALL_MSG, argv[0]);
        EXIT_MSG(EXIT_FAILURE, argv, meta);
        exit(EXIT_FAILURE);
    }

    //mode check
    if(rid && hdr){
        ERROR("%s", "Incompatible options: --rid and --hdr cannot be specified together");
        exit(EXIT_FAILURE);
    }

    slow5_file_t* slow5File = slow5_open(argv[optind], "r");
    if(!slow5File){
        ERROR("Opening %s failed\n", argv[optind]);
        exit(EXIT_FAILURE);
    }

    if (rid){
        print_rid(slow5File);
    }
    else if (hdr){
        print_hdr(slow5File);
    }
    else {
        skim_data_parallel(slow5File, user_opts.num_threads, user_opts.read_id_batch_capacity);
    }

    slow5_close(slow5File);

    return EXIT_SUCCESS;
}
