/* proteusd: the defense daemon, thin cli over the session. the live wire
   body (nftables, real peers) lands at r2. exit codes: 0 ok, 1 usage or
   init, 2 non finite. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "session.h"

static void usage(FILE* out)
{
    fprintf(out,
        "proteusd - bio stochastic moving defense, r1\n"
        "  -s SEED        noise realization, same seed same bytes\n"
        "  -d SECONDS     simulated session length, default 10\n"
        "  -e FILE        event schedule csv (t_ms,kind,amp), optional\n"
        "  -o FILE        party journal, default stdout\n"
        "  -r             realtime pacing, one wall clock tick per tick\n"
        "  --idle-divisor N  stretch the tick period while quiet (1..100)\n"
        "  --socket PATH  status and log unix socket, seqpacket\n"
        "  --group GID    socket group, members need no root\n"
        "  --listen       accept network peers, live scan sensor\n"
        "  --nft          mirror bans into nftables (needs privileges)\n"
        "  --seccomp      install the syscall whitelist (needs profile)\n"
        "  --addr IP      listener address, default 127.0.0.1\n");
}

int main(int argc, char** argv)
{
    session_cfg_t cfg;
    session_stats_t stats;
    const char* out_path = NULL;
    FILE* out = stdout;
    int i, rc;

    memset(&cfg, 0, sizeof(cfg));
    cfg.seed = 1;
    cfg.duration_s = 10.0;
    cfg.socket_group = -1;
    cfg.idle_divisor = 1;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s") && i + 1 < argc)
            cfg.seed = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "-d") && i + 1 < argc)
            cfg.duration_s = strtod(argv[++i], NULL);
        else if (!strcmp(argv[i], "-e") && i + 1 < argc)
            cfg.events_path = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc)
            out_path = argv[++i];
        else if (!strcmp(argv[i], "-r"))
            cfg.realtime = 1;
        else if (!strcmp(argv[i], "--socket") && i + 1 < argc)
            cfg.socket_path = argv[++i];
        else if (!strcmp(argv[i], "--group") && i + 1 < argc)
            cfg.socket_group = strtol(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--listen"))
            cfg.listen = 1;
        else if (!strcmp(argv[i], "--nft"))
            cfg.nft = 1;
        else if (!strcmp(argv[i], "--seccomp"))
            cfg.seccomp = 1;
        else if (!strcmp(argv[i], "--addr") && i + 1 < argc)
            cfg.bind_addr = argv[++i];
        else if (!strcmp(argv[i], "--idle-divisor") && i + 1 < argc) {
            cfg.idle_divisor = (int)strtol(argv[++i], NULL, 10);
            if (cfg.idle_divisor < 1 || cfg.idle_divisor > IDLE_DIVISOR_MAX) {
                fprintf(stderr, "proteusd: idle divisor 1..%d\n",
                        IDLE_DIVISOR_MAX);
                return 1;
            }
        }
        else {
            usage(stderr);
            return 1;
        }
    }
    if (cfg.duration_s <= 0.0) {
        fprintf(stderr, "proteusd: duration must be positive\n");
        return 1;
    }
    if (out_path) {
        out = fopen(out_path, "w");
        if (!out) {
            fprintf(stderr, "proteusd: cannot write %s\n", out_path);
            return 1;
        }
    }
    cfg.journal_out = out;
    setvbuf(out, NULL, _IOLBF, 0);
    rc = session_run(&cfg, &stats);
    if (out != stdout)
        fclose(out);
    return rc;
}
