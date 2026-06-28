#pragma once
#ifdef _WIN32
  #define OB_EXPORT __declspec(dllexport)
#else
  #define OB_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
    OB_EXPORT void*       ob_create();
    OB_EXPORT void        ob_destroy(void* handle);
    OB_EXPORT void        ob_reset(void* handle);

    // Submit FIX.4.2 message (35=D/F/G) → returns FIX ExecutionReport(s), newline-separated
    OB_EXPORT const char* ob_submit_fix(void* handle, const char* fix_message);

    // Read-only queries → returns JSON string
    OB_EXPORT const char* ob_get_snapshot(void* handle, int depth);
    OB_EXPORT const char* ob_get_trade_history(void* handle, int count);
    OB_EXPORT const char* ob_analyze_spread(void* handle);
    OB_EXPORT int         ob_size(void* handle);
}
