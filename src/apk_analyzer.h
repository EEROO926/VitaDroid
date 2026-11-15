#ifndef VITADROID_APK_ANALYZER_H
#define VITADROID_APK_ANALYZER_H

typedef int (*ApkLogFn)(const char *, ...);

void analyze_apk(const char *apk_path, ApkLogFn log_fn);

#endif
