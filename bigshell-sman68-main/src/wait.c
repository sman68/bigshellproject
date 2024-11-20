#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "jobs.h"
#include "params.h"
#include "parser.h"
#include "wait.h"

int wait_on_fg_pgid(pid_t const pgid) {
    if (pgid < 0) return -1;
    jid_t const jid = jobs_get_jid(pgid);
    if (jid < 0) return -1;

    if (kill(-pgid, SIGCONT) < 0) return -1;

    if (is_interactive) {
        if (tcsetpgrp(STDIN_FILENO, pgid) < 0) return -1;
    }

    int retval = 0;
    int last_status = 0;

    for (;;) {
        int status;
        pid_t res = waitpid(-pgid, &status, WUNTRACED);
        if (res < 0) {
            if (errno == ECHILD) {
                errno = 0;
                if (jobs_get_status(jid, &status) < 0) goto err;
                if (WIFEXITED(status)) {
                    params.status = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    params.status = 128 + WTERMSIG(status);
                }
                jobs_remove_jid(jid);
                goto out;
            }
            goto err;
        }

        last_status = status;

        if (jobs_set_status(jid, status) < 0) goto err;
        if (WIFSTOPPED(status)) {
            fprintf(stderr, "[%jd] Stopped\n", (intmax_t)jid);
            goto out;
        }
    }

out:
    if (is_interactive) {
        if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0) return -1;
    }
    return retval;
err:
    retval = -1;
    if (is_interactive) {
        tcsetpgrp(STDIN_FILENO, getpgrp());
    }
    return retval;
}


/* XXX DO NOT MODIFY XXX */
int wait_on_fg_job(jid_t jid)
{
    pid_t pgid = jobs_get_pgid(jid);
    if (pgid < 0) return -1;
    return wait_on_fg_pgid(pgid);
}

int wait_on_bg_jobs()
{
    size_t job_count = jobs_get_joblist_size();
    struct job const *jobs = jobs_get_joblist();
    for (size_t i = 0; i < job_count; ++i) {
        pid_t pgid = jobs[i].pgid;
        jid_t jid = jobs[i].jid;
        for (;;) {
            /* Modify the following line to wait for process group
             * make sure to do a nonblocking wait!
             */
            int status;
            pid_t pid = waitpid(-pgid, &status, WNOHANG);
            if (pid == 0) {
                /* Unwaited children that haven't exited */
                break;
            } else if (pid < 0) {
                /* Error -- some errors are ok though! */
                if (errno == ECHILD) {
                    /* No children -- print saved exit status */
                    errno = 0;
                    if (jobs_get_status(jid, &status) < 0) return -1;
                    if (WIFEXITED(status)) {
                        fprintf(stderr, "[%jd] Done\n", (intmax_t)jid);
                    } else if (WIFSIGNALED(status)) {
                        fprintf(stderr, "[%jd] Terminated\n", (intmax_t)jid);
                    }
                    jobs_remove_pgid(pgid);
                    job_count = jobs_get_joblist_size();
                    jobs = jobs_get_joblist();
                    break;
                }
                return -1; /* Other errors are not ok */
            }

            /* Record status for reporting later when we see ECHILD */
            if (jobs_set_status(jid, status) < 0) return -1;

            /* Handle case where a process in the group is stopped */
            if (WIFSTOPPED(status)) {
                fprintf(stderr, "[%jd] Stopped\n", (intmax_t)jid);
                break;
            }
        }
    }
    return 0;
}