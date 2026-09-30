#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <security/pam_appl.h>
#include "fingerprint.h"

#ifndef FP_TIMEOUT
#define FP_TIMEOUT 40
#endif
#ifndef FP_RETRY
#define FP_RETRY 2
#endif
#define FP_SERVICE "lok-fingerprint"
#define FP_CONFIG "/etc/pam.d/" FP_SERVICE

/* No password prompts or user selection: only informational PAM messages. */
static int
conversation(int n, const struct pam_message **msg,
             struct pam_response **response, void *data)
{
	int i;
	(void)data;
	if (n <= 0 || n > PAM_MAX_NUM_MSG)
		return PAM_CONV_ERR;
	for (i = 0; i < n; i++)
		if (!msg[i] || (msg[i]->msg_style != PAM_TEXT_INFO &&
		               msg[i]->msg_style != PAM_ERROR_MSG))
			return PAM_CONV_ERR;
	*response = calloc(n, sizeof(**response));
	return *response ? PAM_SUCCESS : PAM_BUF_ERR;
}

static int
authenticate(const char *username)
{
	struct stat st;
	pam_handle_t *pamh = NULL;
	struct pam_conv conv = { conversation, NULL };
	int rc, end;

	/* Never fall back to PAM's "other" service when our file is missing.
	 * The system administrator owns this policy, never the launching user. */
	if (lstat(FP_CONFIG, &st) || !S_ISREG(st.st_mode) ||
	    st.st_uid != 0 || (st.st_mode & (S_IWGRP | S_IWOTH)))
		return 0;
	rc = pam_start(FP_SERVICE, username, &conv, &pamh);
	if (rc != PAM_SUCCESS)
		return 0;
	rc = pam_authenticate(pamh, PAM_DISALLOW_NULL_AUTHTOK);
	end = pam_end(pamh, rc);
	return rc == PAM_SUCCESS && end == PAM_SUCCESS;
}

static time_t
now(void)
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts))
		_exit(1);
	return ts.tv_sec;
}

static void
reap(pid_t pid)
{
	int status;
	/* The child has default signals and is ours until reaped. */
	kill(pid, SIGKILL);
	while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
		;
}

static void
supervise(int fd, const char *username)
{
	struct pollfd pfd = { fd, POLLIN, 0 };
	pid_t attempt = -1;
	time_t deadline = 0, retry = 0;
	int rc, status;
	char command;

	/* Only the fixed start command is accepted; EOF means cancel. No PAM
	 * work occurs until the parent confirms all X screens are locked. */
	do { rc = poll(&pfd, 1, -1); } while (rc < 0 && errno == EINTR);
	if (rc <= 0 || recv(fd, &command, 1, 0) != 1 || command != 'S')
		_exit(1);
	for (;;) {
		pfd.revents = 0;
		rc = poll(&pfd, 1, 100);
		if ((rc < 0 && errno != EINTR) ||
		    (rc > 0 && pfd.revents))
			break; /* EOF, error, or any further command cancels. */
		if (attempt > 0) {
			rc = waitpid(attempt, &status, WNOHANG);
			if (rc == attempt) {
				attempt = -1;
				if (now() < deadline && WIFEXITED(status) &&
				    WEXITSTATUS(status) == 0) {
					command = 'Y';
					(void)send(fd, &command, 1, MSG_NOSIGNAL);
					break;
				}
				retry = now() + FP_RETRY;
			} else if (rc < 0 || now() >= deadline) {
				reap(attempt);
				attempt = -1;
				retry = now() + FP_RETRY;
			}
		}
		if (attempt < 0 && now() >= retry) {
			attempt = fork();
			if (attempt == 0) {
				close(fd); /* PAM cannot send an unlock message. */
				_exit(authenticate(username) ? 0 : 1);
			}
			if (attempt < 0)
				retry = now() + FP_RETRY;
			else
				deadline = now() + FP_TIMEOUT;
		}
	}
	if (attempt > 0)
		reap(attempt);
	close(fd);
	_exit(0);
}

int
fingerprint_prepare(struct fingerprint *fp, const char *username)
{
	int sockets[2];
	struct sigaction sa = { .sa_handler = SIG_DFL };
	fp->fd = -1;
	fp->pid = -1;
	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets))
		return 0;
	fp->pid = fork();
	if (fp->pid == 0) {
		close(sockets[0]);
		/* A setuid launch has the caller's real UID. Make the privileged
		 * supervisor root-owned in all UID slots so that caller processes
		 * cannot signal it and strand its PAM child. */
		if (geteuid() == 0 && setresuid(0, 0, 0) < 0)
			_exit(1);
		if (prctl(PR_SET_DUMPABLE, 0) < 0)
			_exit(1);
		/* Terminal signals target the locker, whose socket closure cancels
		 * us. Ignore those here so the supervisor can always reap PAM. */
		setsid();
		sigemptyset(&sa.sa_mask);
		sigaction(SIGTERM, &sa, NULL);
		sigaction(SIGINT, &sa, NULL);
		sigaction(SIGHUP, &sa, NULL);
		sigaction(SIGCHLD, &sa, NULL);
		supervise(sockets[1], username);
	}
	close(sockets[1]);
	if (fp->pid < 0) {
		close(sockets[0]);
		return 0;
	}
	fp->fd = sockets[0];
	return 1;
}

void
fingerprint_start(struct fingerprint *fp)
{
	char command = 'S';
	if (fp->fd >= 0 && send(fp->fd, &command, 1, MSG_NOSIGNAL) != 1)
		fingerprint_stop(fp);
}

int
fingerprint_result(struct fingerprint *fp)
{
	char result;
	ssize_t n;
	if (fp->fd < 0)
		return 0;
	n = recv(fp->fd, &result, 1, MSG_DONTWAIT);
	if (n == 1 && result == 'Y')
		return 1;
	if (n == 0 || n > 0 || (n < 0 && errno != EAGAIN && errno != EINTR))
		fingerprint_stop(fp);
	return 0;
}

void
fingerprint_stop(struct fingerprint *fp)
{
	int status;
	if (fp->fd >= 0)
		close(fp->fd);
	fp->fd = -1;
	if (fp->pid > 0)
		while (waitpid(fp->pid, &status, 0) < 0 && errno == EINTR)
			;
	fp->pid = -1;
}
