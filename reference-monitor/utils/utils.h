char *encrypt_password(const char *password);
char *get_full_path(const char *rel_path);
int is_directory(const char *path);
int rf_state_check(struct reference_monitor *rf);
int is_rf_rec(struct reference_monitor *rf);
int euid_check(kuid_t euid);
int password_check(char *password, struct reference_monitor *rf);