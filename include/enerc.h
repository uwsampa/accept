#ifndef ENERC_H
#define ENERC_H

#define APPROX __attribute__((qual("approx")))
#define ENDORSE(e) (_Pragma("clang diagnostic push") \
                    _Pragma("clang diagnostic ignored \"-Wunused-value\"") \
                    9946037276205 \
                    _Pragma("clang diagnostic pop") \
                    , (e) \
                   )

#endif
