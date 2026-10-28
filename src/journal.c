#include "journal.h"
#include "config.h"

void journal_open(journal_t* j, FILE* out, uint64_t seed, int n_events)
{
    j->out = out;
    fprintf(j->out, "# proteus journal v%d seed=%llu events=%d\n",
            JOURNAL_VERSION, (unsigned long long)seed, n_events);
    fprintf(j->out,
            "# t_ms,act_ava,act_avb,ash_trace,ser,oct,dop,det_score,"
            "port,cause,event\n");
}

void journal_line(char* buf, int n, double t_ms, float act_ava,
                  float act_avb, float ash_trace, float ser, float oct,
                  float dop, float det_score, int port, int cause,
                  int event)
{
    snprintf(buf, (size_t)n,
             "%.2f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.3f,%d,%d,%d\n",
             t_ms, act_ava, act_avb, ash_trace, ser, oct, dop, det_score,
             port, cause, event);
}

void journal_emit(journal_t* j, const char* line)
{
    fputs(line, j->out);
}
