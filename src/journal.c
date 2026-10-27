#include "journal.h"
#include "config.h"

void journal_open(journal_t* j, FILE* out, uint64_t seed, int n_events)
{
    j->out = out;
    fprintf(j->out, "# proteus journal v%d seed=%llu events=%d\n",
            JOURNAL_VERSION, (unsigned long long)seed, n_events);
    fprintf(j->out, "# t_ms,act_ava,act_avb,ash_trace,ser,oct,dop,port,event\n");
}

void journal_row(journal_t* j, double t_ms, float act_ava, float act_avb,
                 float ash_trace, float ser, float oct, float dop,
                 int port, int event_code)
{
    fprintf(j->out, "%.2f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%d\n",
            t_ms, act_ava, act_avb, ash_trace, ser, oct, dop, port,
            event_code);
}
