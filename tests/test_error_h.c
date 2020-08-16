#include "../src/error.h"

int main(void) {
    ERROR("testing%s","");
    WARNING("testing%s","");
    SUCCESS("testing%s","");
    STDERR("testing%s","");
    INFO("testing%s","");
    DEBUG("testing%s","");

    //MALLOC_CHK(NULL);
    //F_CHK(NULL, __FILE__);
    //NULL_CHK(NULL);
    NEG_CHK(-10);

    return 0;
}
