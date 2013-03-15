#ifndef ENERC_H
#define ENERC_H

#ifdef __MSP430__
#define ENERC_TINY_TAGS
#endif

#ifdef ENERC_TINY_TAGS
#define TAG_ENDORSEMENT 39945
#define TAG_DEDORSEMENT 39946
#else
#define TAG_ENDORSEMENT 9946037276205
#define TAG_DEDORSEMENT 9946037276206
#endif

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

#endif
