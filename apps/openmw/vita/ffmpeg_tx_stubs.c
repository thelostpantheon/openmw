/*
 * Stub av_tx_init / av_tx_uninit to prevent 12MB of unused FFT tables
 * from being linked into the Vita executable.
 */
#ifdef __vita__

#include <stddef.h>
#include <stdint.h>

typedef struct AVTXContext AVTXContext;

int av_tx_init(AVTXContext** ctx, void** tx, int type, int inv, int len, const void* scale, uint64_t flags)
{
    (void)ctx; (void)tx; (void)type; (void)inv; (void)len; (void)scale; (void)flags;
    if (ctx)
        *ctx = NULL;
    return -1;
}

void av_tx_uninit(AVTXContext** ctx)
{
    (void)ctx;
}

#endif /* __vita__ */
