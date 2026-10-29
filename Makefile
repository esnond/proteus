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
           -ffp-contract=off -D_DEFAULT_SOURCE -Isrc -Isrc/petri $(NFT_CFLAGS)
LDFLAGS  = -pie
LDLIBS   = -lm
# libmnl resolution: system pkg-config first, the local deps build
# second, an honest error when neither is present
NFT_CFLAGS  = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags libmnl 2>/dev/null)
NFT_LIBS    = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs libmnl 2>/dev/null)
ifneq ($(SECCOMP),)
CFLAGS += -DHAS_SECCOMP_PROFILE
# the seccomp binary links statically: a dynamic loader makes its own
# syscalls before main and the whitelist from a trace of one run does
# not cover another run of the dynamic binary
LDFLAGS += -static
endif
ifneq ($(STATIC),)
LDFLAGS += -static
endif
ifeq ($(NFT_CFLAGS),)
ifneq ($(wildcard $(HOME)/deps/usr/lib/libmnl.a),)
NFT_CFLAGS  = -I$(HOME)/deps/usr/include
NFT_LIBS    = -L$(HOME)/deps/usr/lib -lmnl
endif
endif

PETRI_SRC = src/petri/body.c src/petri/connectome.c src/petri/gcamp.c \
            src/petri/metrics.c src/petri/modulators.c src/petri/mutants.c \
            src/petri/neuron.c src/petri/plasticity.c src/petri/rng.c \
            src/petri/sim.c src/petri/world.c src/petri/evolution.c
PROTEUS_SRC = src/bridge.c src/detector.c src/events.c src/ipc.c \
              src/journal.c src/motor.c src/nft.c src/rate.c src/reflex.c \
              src/seccomp.c src/sensor.c src/session.c src/trust.c
LIB_SRC  = $(PETRI_SRC) $(PROTEUS_SRC)
LIB_OBJ  = $(LIB_SRC:.c=.o)
HDR      = $(wildcard src/*.h) $(wildcard src/petri/*.h)

all: libproteus.a proteusd proteusctl tools

libproteus.a: $(LIB_OBJ)
	$(AR) rcs $@ $(LIB_OBJ)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

proteusd: src/proteusd.o libproteus.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ src/proteusd.o libproteus.a $(LDLIBS) $(NFT_LIBS)

proteusctl: src/proteusctl.o libproteus.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ src/proteusctl.o libproteus.a $(LDLIBS) $(NFT_LIBS)

# sanitized build for hunting memory and ub bugs
debug: CFLAGS := -std=c99 -pedantic -Wall -Wextra -Werror -O0 -g \
                 -fsanitize=address,undefined -ffp-contract=off \
                 -D_DEFAULT_SOURCE -Isrc -Isrc/petri
debug: clean all

test: all
	sh tests/run_tests.sh

clean: tools_clean
	rm -f proteusd proteusctl libproteus.a $(LIB_OBJ) src/proteusd.o src/proteusctl.o

.PHONY: all debug test clean

# r2 traffic generators, plain c99 sockets
TOOLS = scanner legit_client brute_forcer mixed_traffic
TOOLS_SRC = tools/tools_common.c

tools: $(TOOLS)

scanner: tools/scanner.c $(TOOLS_SRC)
	$(CC) $(CFLAGS) -o $@ tools/scanner.c $(TOOLS_SRC) $(LDLIBS)

legit_client: tools/legit_client.c $(TOOLS_SRC)
	$(CC) $(CFLAGS) -o $@ tools/legit_client.c $(TOOLS_SRC) $(LDLIBS)

brute_forcer: tools/brute_forcer.c $(TOOLS_SRC)
	$(CC) $(CFLAGS) -o $@ tools/brute_forcer.c $(TOOLS_SRC) $(LDLIBS)

mixed_traffic: tools/mixed_traffic.c $(TOOLS_SRC)
	$(CC) $(CFLAGS) -o $@ tools/mixed_traffic.c $(TOOLS_SRC) $(LDLIBS)

tools_clean:
	rm -f scanner legit_client brute_forcer mixed_traffic

# ===== ATLAS OF PARTIES (r3) =====
# Phase 2: causal attribution, divergence, learnability
# Targets: atlas-run (journals), atlas-train, atlas-validate, atlas (full pipeline)

ATLAS_SCHEDULE = tests/fixtures/burst.csv
ATLAS_DURATION = 600
ATLAS_MAX_HOURS = 48

# Run N parties headless, emit journals to results/atlas_journals/
atlas-run:
	@echo "=== ATLAS: Running parties ==="
	./tools/atlas_runner.sh \
	    --seeds 1001..1100 \
	    --schedule $(ATLAS_SCHEDULE) \
	    --out-dir results/atlas_journals \
	    --max-hours $(ATLAS_MAX_HOURS) \
	    --checkpoint results/atlas_journals/checkpoint.txt \
	    --resume \
	    --duration $(ATLAS_DURATION)

# Run stress seeds 3001..3050
atlas-stress:
	@echo "=== ATLAS: Stress seeds ==="
	./tools/atlas_runner.sh \
	    --seeds 3001..3050 \
	    --schedule $(ATLAS_SCHEDULE) \
	    --out-dir results/atlas_journals \
	    --max-hours 12 \
	    --checkpoint results/atlas_journals/stress_checkpoint.txt \
	    --resume \
	    --duration $(ATLAS_DURATION)

# Analyze training seeds 1001..1100 -> results/atlas_train/*.csv
atlas-train: atlas-run
	@echo "=== ATLAS: Training analysis ==="
	./tools/atlas_train.sh

# Analyze held-out seeds 2001..2100 -> results/atlas_validate/*.csv
# STRICT: separate journals, no peeking at training
atlas-validate:
	@echo "=== ATLAS: Held-out validation ==="
	./tools/atlas_validate.sh

# Full pipeline: run + train + validate + manifest
atlas: atlas-train atlas-validate
	@echo "=== ATLAS: Full pipeline complete ==="
	@echo "Training:   results/atlas_train/atlas_parties.csv"
	@echo "Validation: results/atlas_validate/atlas_parties.csv"
	@echo "Manifests:  results/atlas_train/manifest.txt"
	@echo "            results/atlas_validate/manifest.txt"

# Quick sanity: single party analysis (for development)
atlas-check: proteusd
	@echo "=== ATLAS: Single party check (seed 1001) ==="
	./tools/atlas_runner.sh --seeds 1001..1001 --schedule $(ATLAS_SCHEDULE) --out-dir results/atlas_journals --max-hours 1 --duration $(ATLAS_DURATION)
	python3 tools/atlas_analyze.py results/atlas_journals/seed_1001.csv 1 1001 $$(sha256sum $(ATLAS_SCHEDULE) | cut -d' ' -f1) $$(sha256sum proteusd | cut -d' ' -f1)

.PHONY: atlas-run atlas-stress atlas-train atlas-validate atlas atlas-check

# ===== PHASE 3: OBSERVER MODELS (g1-g7 hypotheses) =====
# Phase 3: learnability, active probing, reconstruction, g1-g7 experiments

HYP_SCHEDULE_BIO = tests/fixtures/burst.csv
HYP_SCHEDULE_SEEDED = tests/fixtures/seeded_schedule.csv
HYP_SCHEDULE_RANDOM = tests/fixtures/random_schedule.csv
HYP_MAX_HOURS = 72

# Run g1-g7 experiments on training seeds 1001..1100
g1-g7-run: proteusd
	@echo "=== PHASE 3: Running g1-g7 hypotheses (training seeds) ==="
	./tools/g1_g7_runner.sh \
	    --max-hours $(HYP_MAX_HOURS) \
	    --checkpoint results/hypotheses/g1_g7_checkpoint.txt \
	    --resume

# Run g1-g7 validation on held-out seeds 2001..2100
g1-g7-validate: proteusd
	@echo "=== PHASE 3: Validating g1-g7 hypotheses (held-out seeds) ==="
	./tools/g1_g7_validate.sh \
	    --max-hours $(HYP_MAX_HOURS) \
	    --checkpoint results/hypotheses_validate/g1_g7_checkpoint.txt \
	    --resume

# Full Phase 3 pipeline
phase3: g1-g7-run g1-g7-validate
	@echo "=== PHASE 3 COMPLETE ==="
	@echo "Training results:  results/hypotheses/g1_g7_results.csv"
	@echo "Validation results: results/hypotheses_validate/g1_g7_results.csv"
	@echo "Manifests: results/hypotheses/manifest.txt"
	@echo "         results/hypotheses_validate/manifest.txt"

# Quick sanity: single journal learnability check
learnability-check: proteusd
	@echo "=== Learnability check (seed 1001) ==="
	python3 tools/learnability.py results/atlas_journals/seed_1001.csv $(HYP_SCHEDULE_BIO)

# Test all observers on single journal
observers-check: proteusd
	@echo "=== Passive observer ==="
	python3 tools/observer_passive.py results/atlas_journals/seed_1001.csv
	@echo "=== Active observer ==="
	python3 tools/observer_active.py results/atlas_journals/seed_1001.csv 65
	@echo "=== Reconstruction observer ==="
	python3 tools/observer_reconstruction.py results/atlas_journals/seed_1001.csv $(HYP_SCHEDULE_BIO) 1000

.PHONY: g1-g7-run g1-g7-validate phase3 learnability-check observers-check

# ===== PACKAGING (r6) =====
# install/uninstall, version, dist

VERSION = 1.2.0
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
SYSTEMDDIR = /etc/systemd/system

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 proteusd $(DESTDIR)$(BINDIR)/proteusd
	install -m 755 proteusctl $(DESTDIR)$(BINDIR)/proteusctl
	install -d $(DESTDIR)$(SYSTEMDDIR)
	install -m 644 packaging/proteusd.service $(DESTDIR)$(SYSTEMDDIR)/proteusd.service
	@echo "Installed to $(DESTDIR)$(BINDIR) and $(DESTDIR)$(SYSTEMDDIR)"
	@echo "Run: systemctl daemon-reload && systemctl enable --now proteusd"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/proteusd
	rm -f $(DESTDIR)$(BINDIR)/proteusctl
	rm -f $(DESTDIR)$(SYSTEMDDIR)/proteusd.service
	@echo "Uninstalled"

version:
	@echo $(VERSION)

dist: clean
	git archive --format=tar.gz --prefix=proteus-$(VERSION)/ -o proteus-$(VERSION).tar.gz HEAD
	@echo "Created proteus-$(VERSION).tar.gz"

.PHONY: install uninstall version dist
