#ifndef foodaemonforkhfoo
#define foodaemonforkhfoo

/*
 * This file is part of libdaemon.
 *
 * libdaemon is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdaemon is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libdaemon; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <sys/types.h>

/** \mainpage
 *
 * For a brief explanation of libdaemons's purpose, have a look on <a
 * href="../../README.html">the README file</a>. Thank you!
 *
 */

/** \example testd.c
 * This is an example for the usage of libdaemon
 */

/** \file
 *
 * Contains an API for doing a daemonizing fork(). 
 *
 * You may daemonize by calling daemon_fork(), a function similar to
 * the plain fork(). If you want to return a return value of the
 * initialization procedure of the child from the parent, you may use
 * the daemon_retval_xxx() functions.
 */

/** Does a daemonizing fork(). For the new daemon process STDIN,
 * STDOUT, STDERR are connected to /dev/null, the process is a session
 * leader, the current directory is changed to /, the umask is set to
 * 777.
 * @return zero on success in the child, the PID of the new
 * daemon process in the parent, nonzero on failure
 */
pid_t daemon_fork(void);

/** Initializes the library for allowing the passing of a return value
 * from the daemon initialization to the parent process. Call this
 * before calling any of the other daemon_retval_xxx() functions in
 * the parent, before forking.
 * @return zero on success, nonzero on failure.
 */
int daemon_retval_init(void);

/** Frees the resources allocated by daemon_retval_init(). This should
 * be called if neither daemon_retval_wait() nor daemon_retval_send()
 * is used. If a fork took place, the function should be called in
 * both the parent and the daemon. */
void daemon_retval_done(void);

/** Wait the specified amount of seconds for the response of the
 * daemon process and return the integer passed to
 * daemon_retval_send() in the daemon process. Should be called just
 * once from the parent process only. A subsequent call to
 * daemon_retval_done() in the parent is ignored.
 * @param timeout The timeout in seconds
 * @return The integer passed daemon_retval_send() in the daemon
 * process, or negative on failure.
 */
int daemon_retval_wait(int timeout);

/** Send the specified integer to the parent process. Should be called
 * just once from the daemon process only. A subsequent call to
 * daemon_retval_done() in the daemon is ignored.
 * @param s The integer to pass to daemon_retval_wait() in the parent process
 * @return Zero on success, nonzero on failure.
 */
int daemon_retval_send(int s);

#endif
