#ifndef UTILS
#define UTILS


char *encrypt_password(const char *password);
int rf_state_check(int state);
int is_rf_rec(struct reference_monitor *rf);
int euid_check(kuid_t euid);
int password_check(char *password, struct reference_monitor *rf);
char *get_path_from_dentry(struct dentry *dentry);
#endif