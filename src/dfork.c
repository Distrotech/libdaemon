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

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "dfork.h"
#include "dnonblock.h"
#include "dlog.h"

static int _daemon_retval_pipe[2] = { -1, -1 };

static int _null_open(int f, int fd) {
    int fd2;

    if ((fd2 = open("/dev/null", f)) < 0)
        return -1;
    
    if (fd2 == fd)
        return fd;

    if (dup2(fd2, fd) < 0)
        return -1;

    close(fd2);
    return fd;
}

pid_t daemon_fork(void) {
    pid_t pid;
    int _pipe[2];
    FILE *pipe_in, *pipe_out;
    
    pid_t p = (pid_t) -1;

    if (pipe(_pipe) < 0 || !(pipe_out = fdopen(_pipe[0], "r")) || !(pipe_in = fdopen(_pipe[1], "w"))) {
        daemon_log(LOG_ERR, "pipe() failed: %s", strerror(errno));
        return (pid_t) -1;
    }

    daemon_nonblock(_pipe[1], 1);
        
    if ((pid = fork()) < 0) { // First fork
        daemon_log(LOG_ERR, "First fork() failed: %s\n", strerror(errno));
        fclose(pipe_in);
        fclose(pipe_out);
        return (pid_t) -1;

    } else if (pid == 0) {
        // First child

        fclose(pipe_out);
        
        if (_null_open(O_RDONLY, 0) < 0) {
            daemon_log(LOG_ERR, "Failed to open /dev/null for STDIN: %s", strerror(errno));
            goto fail;
        }
        
        if (_null_open(O_WRONLY, 1) < 0) {
            daemon_log(LOG_ERR, "Failed to open /dev/null for STDOUT: %s", strerror(errno));
            goto fail;
        }
        
        if (_null_open(O_WRONLY, 2) < 0) {
            daemon_log(LOG_ERR, "Failed to open /dev/null for STDERR: %s", strerror(errno));
            goto fail;
        }
        
        setsid();
        umask(0777);
        chdir("/");
        
        if ((pid = fork()) < 0) { // Second fork
            daemon_log(LOG_ERR, "Second fork() failed: %s", strerror(errno));
            goto fail;

        } else if (pid == 0) {
            // Second child
            p = getpid();
            fwrite(&p, sizeof(p), 1, pipe_in);
            fclose(pipe_in);


            if (daemon_log_use & DAEMON_LOG_AUTO)
                daemon_log_use = DAEMON_LOG_SYSLOG;
            
            return 0;

        } else {
            // Second father
            fclose(pipe_in);
            exit(0);
        }
            
    fail:
        fwrite(&p, sizeof(p), 1, pipe_in);
        fclose(pipe_in);
        exit(0);

    } else { // First father
        if (fread(&p, sizeof(p), 1, pipe_out) != 1)
            p = (pid_t) -1;

        fclose(pipe_out);
        return p;
    }

}

int daemon_retval_init(void) {
    if (pipe(_daemon_retval_pipe) < 0)
        return -1;

    return 0;
}

void daemon_retval_done(void) {
    if (_daemon_retval_pipe[0] >= 0)
        close(_daemon_retval_pipe[0]);
    
    if (_daemon_retval_pipe[1] >= 0)
        close(_daemon_retval_pipe[1]);

    _daemon_retval_pipe[0] = _daemon_retval_pipe[1] = -1;
}

int daemon_retval_send(int i) {
    ssize_t r;
    r = write(_daemon_retval_pipe[1], &i, sizeof(i));

    daemon_retval_done();

    if (r != sizeof(i)) {

        if (r < 0)
            daemon_log(LOG_ERR, "read() failed while writing return value to pipe: %s", strerror(errno));
        else
            daemon_log(LOG_ERR, "write() too short while writing return value from pipe");
        
        return -1;
    }

    return 0;
}

int daemon_retval_wait(int timeout) {
    ssize_t r;
    int i;

    if (timeout > 0) {
        struct timeval tv = { timeout, 0 };
        int s;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(_daemon_retval_pipe[0], &fds);

        if ((s = select(FD_SETSIZE, &fds, 0, 0, &tv)) != 1) {
            
            if (s < 0)
                daemon_log(LOG_ERR, "select() failed while waiting for return value: %s", strerror(errno));
            else
                daemon_log(LOG_ERR, "Timeout reached while wating for return value");
        
            return -1;
        }
    }

    if ((r = read(_daemon_retval_pipe[0], &i, sizeof(i))) != sizeof(i)) {

        if (r < 0)
            daemon_log(LOG_ERR, "read() failed while reading return value from pipe: %s", strerror(errno));
        else if (r == 0)
            daemon_log(LOG_ERR, "read() failed with EOF while reading return value from pipe.");
        else if (r > 0)
            daemon_log(LOG_ERR, "read() too short while reading return value from pipe.");
        
        return -1;
    }

    daemon_retval_done();
    
    return i;
}

