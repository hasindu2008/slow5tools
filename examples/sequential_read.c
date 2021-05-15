
// an example programme that uses slow5lib to read a SLOW5 file sequentially

#include "slow5.h"

#define FILE_PATH "examples/example.slow5"

#define TO_PICOAMPS(RAW_VAL,DIGITISATION,OFFSET,RANGE) (((RAW_VAL)+(OFFSET))*((RANGE)/(DIGITISATION)))

int main(){

    slow5_file_t *sp = slow5_open(FILE_PATH,"r");
    slow5_rec_t *rec = NULL;
    int ret=0;

    while((ret = slow5_get_next(&rec,sp)) != -2){
        if(ret<0){
            fprintf(stderr,"Error in slow5_get_next. Error code %d\n",ret);
            exit(EXIT_FAILURE);
        }

        printf("%s\t",rec->read_id);
        uint64_t len_raw_signal = rec->len_raw_signal;
        for(uint64_t i=0;i<len_raw_signal;i++){
            double pA = TO_PICOAMPS(rec->raw_signal[i],rec->digitisation,rec->offset,rec->range);
            printf("%f ",pA);
        }
        printf("\n");

    }

    slow5_rec_free(rec);

    slow5_close(sp);

}
