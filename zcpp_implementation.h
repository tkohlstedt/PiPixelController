#ifndef __ZCPP_IMPLEMENTATION_H__
#define __ZCPP_IMPLEMENTATION_H__

typedef struct _zcppParam
{
    int universeStart;
    int universeEnd;
    char *buffer;
    thread_ctrl *hwconfig;
} zcppParam;

void *zcpp_multicast_listen(void *listen_parameters);
void *zcpp_listen(void *listen_parameters);

#endif