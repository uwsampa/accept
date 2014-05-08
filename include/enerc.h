#ifndef ENERC_H
#define ENERC_H

#define TAG_ENDORSEMENT 39945
#define TAG_DEDORSEMENT 39946

#define APPROX __attribute__((qual("approx")))
#define ENDORSE(e) (_Pragma("clang diagnostic push") \
                    _Pragma("clang diagnostic ignored \"-Wunused-value\"") \
                    TAG_ENDORSEMENT \
                    _Pragma("clang diagnostic pop") \
                    , (e) \
                   )
#define DEDORSE(e) (_Pragma("clang diagnostic push") \
                    _Pragma("clang diagnostic ignored \"-Wunused-value\"") \
                    TAG_DEDORSEMENT \
                    _Pragma("clang diagnostic pop") \
                    , (e) \
                   )

// Benchmark instrumentation.
#ifdef __cplusplus
//extern "C" void accept_roi_begin();
//extern "C" void accept_roi_end();
#else
//void accept_roi_begin();
//void accept_roi_end();
#endif

#endif
