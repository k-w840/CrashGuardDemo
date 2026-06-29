#ifndef ZWMOBILE_GUARD_REPORT_JSON_H
#define ZWMOBILE_GUARD_REPORT_JSON_H

#include "ZWMobileGuardRawReport.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ZWRawBinaryImage *g_binaryImages;
extern uint32_t g_binaryImageCount;

void zwMobileGuardWriteJSONReport(const ZWRawCrashReport *report, const char *filePath);

#ifdef __cplusplus
}
#endif

#endif // ZWMOBILE_GUARD_REPORT_JSON_H
