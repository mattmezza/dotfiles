/* Private, one-shot channel to the isolated PAM supervisor. */
struct fingerprint {
	int fd;
	pid_t pid;
};
int fingerprint_prepare(struct fingerprint *fp, const char *username);
void fingerprint_start(struct fingerprint *fp);
int fingerprint_result(struct fingerprint *fp);
void fingerprint_stop(struct fingerprint *fp);
