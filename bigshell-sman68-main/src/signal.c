#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <errno.h>
#include <stddef.h>
#include "signal.h"

static void
interrupting_signal_handler(int signo)
{
  /* Its only job is to interrupt system calls--like read()--when
   * certain signals arrive--like Ctrl-C.
   */
}

static struct sigaction ignore_action = {.sa_handler = SIG_IGN},
                        old_sigtstp, old_sigint, old_sigttou;
/* Ignore certain signals.
 * 
 * @returns 0 on succes, -1 on failure
 *
 *
 * The list of signals to ignore:
 *   - SIGTSTP
 *   - SIGINT
 *   - SIGTTOU
 *
 * Should be called immediately on entry to main() 
 *
 * Saves old signal dispositions for a later call to signal_restore()
 */

int
signal_init(void)
{
  if (sigaction(SIGTSTP, &ignore_action, &old_sigtstp) < 0) return -1;
  if (sigaction(SIGINT, &ignore_action, &old_sigint) < 0) return -1;
  if (sigaction(SIGTTOU, &ignore_action, &old_sigttou) < 0) return -1;
  return 0;
}


/** enable signal to interrupt blocking syscalls (read/getline, etc) 
 *
 * @returns 0 on succes, -1 on failure
 *
 * does not save old signal disposition
 */
int
signal_enable_interrupt(int sig)
{
  struct sigaction act = {0};
  act.sa_handler = interrupting_signal_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  if (sigaction(sig, &act, NULL) < 0) return -1;
  return 0;
}


/** ignore a signal
 *
 * @returns 0 on success, -1 on failure
 *
 * does not save old signal disposition
 */
int
signal_ignore(int sig)
{
  struct sigaction act = {0};
  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  if (sigaction(sig, &act, NULL) < 0) return -1;
  return 0;
}

/** Restores signal dispositions to what they were when bigshell was invoked
 *
 * @returns 0 on success, -1 on failure
 *
 */
int
signal_restore(void)
{
  if (sigaction(SIGTSTP, &old_sigtstp, NULL) < 0) return -1;
  if (sigaction(SIGINT, &old_sigint, NULL) < 0) return -1;
  if (sigaction(SIGTTOU, &old_sigttou, NULL) < 0) return -1;
  return 0;
}