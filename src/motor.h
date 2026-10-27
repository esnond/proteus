#ifndef PROTEUS_MOTOR_H
#define PROTEUS_MOTOR_H

/*
 * motor: the moving tcp listener. the walk order is deterministic, the
 * n-th bind takes port base + n mod window, so the same event history
 * gives the same port schedule. the journalled port always comes from
 * getsockname, never from the intent.
 */
typedef struct {
    int fd;      /* listening socket, -1 when closed */
    int port;    /* current listening port, 0 when closed */
    int binds;   /* completed binds, drives the walk order */
    int moves;   /* completed moves */
} motor_t;

/* bind the first port of the window, -1 on failure */
int  motor_init(motor_t* m);
/* close the current listener and open the next port in the window,
 * returns the new port or -1 */
int  motor_move(motor_t* m);
void motor_close(motor_t* m);

#endif
