#ifndef SEAFILE_CLIENT_UTILS_MAC_H_

int __mac_isHiDPI(void);
double __mac_getScaleFactor(void);
void __mac_setDockIconStyle(bool);
bool __mac_getDefault(const char* key);
void __mac_setDefault(const char* key, bool value);
void __mac_initDefaults(const char* key);

#endif /* SEAFILE_CLIENT_UTILS_MAC_H_ */
