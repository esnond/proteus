# proteus. bio-stochastic moving defense on top of the petri connectome
# engine, vendored under src/petri. c99, libc + libm only for the core;
# the network body adds libmnl/libnftnl at stage r2, not before.
#
# -ffp-contract=off is part of the determinism contract (no fma contraction),
# same as the engine we vendor. no -ffast-math, ever.

CC       = cc
AR       = ar
CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Werror -O2 \
           -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE \
           -ffp-contract=off -D_DEFAULT_SOURCE -Isrc -Isrc/petri
LDFLAGS  = -pie
LDLIBS   = -lm

PETRI_SRC = src/petri/body.c src/petri/connectome.c src/petri/gcamp.c \
            src/petri/metrics.c src/petri/modulators.c src/petri/mutants.c \
            src/petri/neuron.c src/petri/plasticity.c src/petri/rng.c \
            src/petri/sim.c src/petri/world.c src/petri/evolution.c
PROTEUS_SRC = src/bridge.c src/detector.c src/events.c src/ipc.c \
              src/journal.c src/motor.c src/rate.c src/reflex.c \
              src/session.c
LIB_SRC  = $(PETRI_SRC) $(PROTEUS_SRC)
LIB_OBJ  = $(LIB_SRC:.c=.o)
HDR      = $(wildcard src/*.h) $(wildcard src/petri/*.h)

all: libproteus.a proteusd proteusctl

libproteus.a: $(LIB_OBJ)
	$(AR) rcs $@ $(LIB_OBJ)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

proteusd: src/proteusd.o libproteus.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ src/proteusd.o libproteus.a $(LDLIBS)

proteusctl: src/proteusctl.o libproteus.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ src/proteusctl.o libproteus.a $(LDLIBS)

# sanitized build for hunting memory and ub bugs
debug: CFLAGS := -std=c99 -pedantic -Wall -Wextra -Werror -O0 -g \
                 -fsanitize=address,undefined -ffp-contract=off \
                 -D_DEFAULT_SOURCE -Isrc -Isrc/petri
debug: clean all

test: all
	sh tests/run_tests.sh

clean:
	rm -f proteusd proteusctl libproteus.a $(LIB_OBJ) src/proteusd.o src/proteusctl.o

.PHONY: all debug test clean
