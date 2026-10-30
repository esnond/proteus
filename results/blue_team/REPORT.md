# blue team: t7 validation

finding: t7_registration_fuzz, a malformed/oversized IPC command keeps a client slot.

reproduction: sandbox, same uid, no root/nft/netns.
  `./proteusd -s 31 -d 30 -r --socket /tmp/blue_t7b.sock -o /dev/null`
  plus a python SOCK_SEQPACKET probe: 8 connections per payload, one status request after.
  observed: 8x `b"zzzz"` -> status replied (port=40003), 0 slots held; 8x `b"zzzz"+4000B` -> status replied (port=40003), 0 slots held; 8x `b"log "+4000B` -> status closed with no reply, 8 slots held; closing those 8 sockets -> status replied again (port=40004), daemon alive.
  pre-fix source `6247849:src/ipc.c` stores an unknown command in
  `ipc->client[i]` with mode `CMD_UNKNOWN` and never closes it (the send loop
  reaches `else ... continue`), so the slot is held until restart.

verdict: valid. the pre-fix leak is real and is closed by `378c5cf`. the
  oversized variant is not closed: `recv()` without `MSG_TRUNC` silently
  truncates to `IPC_CMD_MAX-1` = 31 B and parses the first 31 bytes, so
  `"log "+4000B` is accepted as a log subscription. the t7 row "7 malformed
  cases, daemon alive" covers 6 unknown/registration cases and one oversized
  unknown (`b"x"*4000`); it never asserts that a connection was closed or that
  a slot is free, so PASS is not evidence for the claimed property.

mitigation: `src/ipc.c:ipc_tick`. existing `CMD_UNKNOWN` branch (line 204)
  closes unknown commands and closes the malformed/unknown leak. missing half:
  `recv(c, cmd, sizeof(cmd)-1, MSG_DONTWAIT)` -> add `MSG_TRUNC` and reject
  `n > (ssize_t)sizeof(cmd)-1` before `cmd[n]='\0'`. this closes the oversized
  path because the peer is dropped regardless of the packet prefix. one
  condition, no redesign.

verification: runtime probes against the HEAD binary: 8x unknown -> 0 slots held (fixed, verified); 8x oversized unknown -> 0 slots held (fixed, verified); 8x oversized valid-prefix -> 8 slots held (still open).
  the `MSG_TRUNC` mitigation was not applied (blue team is read-only on
  `src/`), so its verification is not run and the oversized variant is open,
  not closed.

residual: any peer that passes `allowed()`, same euid, or a member of the socket
  group when `--group` is used, can hold all 8 `IPC_CLIENTS_MAX` slots with 8
  `"log "` packets and block the status/log channel (the 9th client is closed
  without a reply). the tick loop, detection and bans are unaffected. slots are
  released when the holder closes (verified). the same uid is the documented
  trust boundary; the group member is a different uid and the availability
  effect is not documented. closing condition: the `MSG_TRUNC` check, plus a
  subscriber age/idle reaper or a subscriber cap below `IPC_CLIENTS_MAX` if
  status must stay available. the regression test must be extended: 8 malformed
  + 8 oversized connections, then require a status reply, "daemon alive" is not
  the property.

severity_after: low

summary: 1 closed (unknown/malformed command), 0 narrowed, 1 open (oversized
valid-prefix slot), 0 unverified.
attention: t7's PASS is not backed by a slot assertion; the fix comment names
"unknown or oversized" but only unknown is implemented, and neither the fix
nor the test covers 8-subscriber slot exhaustion.

## post-fix verification (session, after the blue team report)

mitigation applied: src/ipc.c:ipc_tick, recv() now uses MSG_TRUNC and any
datagram larger than IPC_CMD_MAX-1 is closed before parsing.

verification, same probe, 16 held sockets: pre-fix binary (4c88bcf:src/ipc.c): status BLOCKED, 8 slots held; post-fix binary: status replies, 0 slots held.
regression test: tests/run_tests.sh "ipc: oversized commands must not hold
slots" (8 unknown + 8 oversized, then a status reply is required).
gate: make test green, ubsan clean.

residual after the fix (still open, published): 8 legitimate short `log`
subscribers from a permitted uid (same euid or a --group member) can still
fill IPC_CLIENTS_MAX and hold the status channel until they close. the
oversized vector is closed, the availability vector is not. closing
condition unchanged: subscriber idle/age reaper or a subscriber cap that
reserves one slot for status.
