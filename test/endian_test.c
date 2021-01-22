#include <stdio.h>

static inline int ed_is_big(void)
{
    long one= 1;
    return !(*((char *)(&one)));
}

int main(){
    if(ed_is_big()){
        puts("big endian\n");
    }
    else{
        puts("little endian\n");
    }

}
