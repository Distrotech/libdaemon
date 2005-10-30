/* $Id$ */

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stddef.h>

#include "dpid.h"
#include "dlog.h"

#ifndef ETIME
#define ETIME ETIMEDOUT /* For FreeBSD */
#endif

#define VARRUN "/var/run"

const char *daemon_pid_file_ident = NULL;
daemon_pid_file_proc_t daemon_pid_file_proc = daemon_pid_file_proc_default;

const char *daemon_pid_file_proc_default(void) {
    static char fn[PATH_MAX];
    snprintf(fn, sizeof(fn), "%s/%s.pid", VARRUN, daemon_pid_file_ident ? daemon_pid_file_ident : "unknown");
    return fn;
}

static int lock_file(int fd, int enable) {
    struct flock f;

    memset(&f, 0, sizeof(f));
    f.l_type = enable ? F_WRLCK : F_UNLCK;
    f.l_whence = SEEK_SET;
    f.l_start = 0;
    f.l_len = 0;
    
    if (fcntl(fd, F_SETLKW, &f) < 0) {
        daemon_log(LOG_WARNING, "fcntl(F_SETLKW) failed: %s", strerror(errno));
        return -1;
    }

    return 0;
}

pid_t daemon_pid_file_is_running(void) {
    const char *fn;
    static char txt[256];
    int fd = -1, locked = -1;
    pid_t ret = (pid_t) -1, pid;
    ssize_t l;

    if (!(fn = daemon_pid_file_proc()))
        goto finish;

    if ((fd = open(fn, O_RDWR, 0644)) < 0) {
        if (errno != ENOENT)
            daemon_log(LOG_WARNING, "Failed to open PID file: %s", strerror(errno));

        goto finish;
    }

    if ((locked = lock_file(fd, 1)) < 0)
        goto finish;
    
    if ((l = read(fd, txt, sizeof(txt)-1)) < 0) {
        daemon_log(LOG_WARNING, "read(): %s", strerror(errno));
        unlink(fn);
        goto finish;
    }

    txt[l] = 0;
    
    if ((pid = (pid_t) atoi(txt)) <= 0) {
        daemon_log(LOG_WARNING, "PID file corrupt, removing. (%s)", fn);
        unlink(fn);
        goto finish;
    }

    if (kill(pid, 0) != 0 && errno != EPERM) {
        daemon_log(LOG_WARNING, "Process %lu died: %s; removing PID file. (%s)", (unsigned long) pid, strerror(errno), fn);
        unlink(fn);
        goto finish;
    }

    ret = pid;
    
finish:

    if (fd >= 0) {
        if (locked >= 0)
            lock_file(fd, 0);
        close(fd);
    }
    
    return ret;
}

int daemon_pid_file_kill(int s) {
    pid_t pid;
    
    if ((pid = daemon_pid_file_is_running()) < 0)
        return -1;

    if (kill(pid, s) < 0)
        return -1;

    return 0;
}

int daemon_pid_file_kill_wait(int s, int m) {
    pid_t pid;
    time_t t;
    
    if ((pid = daemon_pid_file_is_running()) < 0)
        return -1;

    if (kill(pid, s) < 0)
        return -1;

    t = time(NULL) + m;

    for (;;) {
        int r;
        struct timeval tv = { 0, 100000 };

        if (time(NULL) > t) {
            errno = ETIME;
            return -1;
        }
            
        if ((r = kill(pid, 0)) < 0 && errno != ESRCH)
            return -1;

        if (r)
            return 0;

        if (select(0, NULL, NULL, NULL, &tv) < 0)
            return -1;
    }
}

int daemon_pid_file_create(void) {
    const char *fn;
    int fd = -1;
    int ret = -1;
    int locked = -1;
    char t[64];
    ssize_t l;
    mode_t u;

    u = umask(022);

    if (!(fn = daemon_pid_file_proc()))
        goto finish;
    
    if ((fd = open(fn, O_CREAT|O_RDWR|O_EXCL, 0644)) < 0) {
        fprintf(stderr, "open(%s): %s", fn, strerror(errno));
        goto finish;
    }

    if ((locked = lock_file(fd, 1)) < 0) {
        unlink(fn);
        goto finish;
    }

    snprintf(t, sizeof(t), "%lu\n", (unsigned long) getpid());

    if (write(fd, t, l = strlen(t)) != l) {
        daemon_log(LOG_WARNING, "write(): %s", strerror(errno));
        unlink(fn);
        goto finish;
    }

    ret = 0;

finish:

    if (fd >= 0) {
        if (locked >= 0)
            lock_file(fd, 0);
            
        close(fd);
    }
    
    umask(u);
    
    return ret;
}

int daemon_pid_file_remove(void) {
    const char *fn;
    
    if (!(fn = daemon_pid_file_proc()))
        return -1;
    
    if (unlink(fn) < 0)
        return -1;

    return 0;
}

