# immune scope — r2.5 trust layer

purpose: registered local services get zero false positives; the
behavioral detector keeps its full input for everything unregistered.
the biology is not in this path (emergent-behavior wall holds: no token
logic in neuron/sim code).

## protocol

    service                         daemon (unix seqpacket, SO_PEERCRED)
    -------                         -----------------------------------
    connect(sock)            ->     allowed() check: owner, root, or group
    send("register")         ->     owner/root only, never group members
                             <-     "token=<64 hex> ttl_ms=600000"
    send TCP payload
      "PROTEUS TOKEN <hex>"  ->     sensor checks the token, marks the
                                    source IP trusted for 60 s

- token: 32 bytes from getrandom() (kernel CSPRNG). the connectome rng is
  a schedule generator and is NOT used for keys; that would be dishonest
  and the red team checks it (t1).
- lifetime: bounded by the process. the reaper stats /proc/<pid> every
  500 ms and drops dead or expired entries; a helper process cannot
  register on behalf of a service (observed: `proteusctl register` exits,
  its token dies with it).
- token table: fixed array of 32 entries, constant-time compare, no
  allocation after init.
- kernel fast path: a base chain `trust` at input priority raw with
  `meta mark 0x50 accept`; a service that sets SO_MARK (needs
  CAP_NET_ADMIN) is accepted before the ban chain.

## measured

sandbox netns, 10.0.0.2 daemon, sources 10.0.0.3 (service) and 10.0.0.4:

    status during the session: trust=1
    scan from 10.0.0.4 -> banned, kernel set { 10.0.0.4 }
    scan from 10.0.0.3 -> no ban, set stays { 10.0.0.4 }
    service exit      -> trust=0 within 800 ms

seccomp: profile regenerated with a registration in the trace, 36
syscalls; the daemon survives registration under the filter.

## threat model rows

| threat | mechanism | result |
|---|---|---|
| token guess | 256-bit getrandom | infeasible |
| replay after death | reaper stats the pid | revoked, tested |
| helper registers for a service | token bound to peer pid | no (observed) |
| same-uid attacker | same uid = same trust domain | MODEL BOUNDARY, documented |
| table exhaustion | 32 entries, registration needs owner/root | cap, denied |
| token timing | constant-time compare | see t6 |
| malformed registration | hex validation, fixed buffers | t7 |
| seccomp growth | regenerated profile, 36 syscalls | documented |
| nft mark spoof from wire | mark is local metadata | impossible by design |
| mark fast path actually fires | measured counter | **NOT observed** (see t9) |

## limitations, published

1. **the mark rule did not fire in this environment**: a marked local
   client still hit the ban rule (counter 0). tier A currently rests on
   the token path, not the kernel mark. r3: retest on the bench and on a
   non-loopback path; if it never fires, remove the rule.
2. registration is local (unix socket). anyone who can run as the daemon
   owner is inside the trust domain; that is the model, not a bug.
3. the token line is the *first payload*; it proves registration, it is
   not authentication of the user. payload confidentiality still needs
   standard crypto (TLS/Noise), not implemented here and not claimed.
4. a registered service that scans is exempt by construction (t10); the
   defense for that is the registration allowlist itself.
