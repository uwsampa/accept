#ifndef CONFIG_H
#define CONFIG_H

#define BLOCK_SIZE 64

#define GRAY 0
#define RGB  1

extern UINT8 Lqt [BLOCK_SIZE];
extern UINT8 Cqt [BLOCK_SIZE];
extern UINT16 ILqt [BLOCK_SIZE];
extern UINT16 ICqt [BLOCK_SIZE];

extern INT16 CB [BLOCK_SIZE];
extern INT16 CR [BLOCK_SIZE];
extern INT16 Temp [BLOCK_SIZE];

extern INT16 global_ldc1;
extern INT16 global_ldc2;
extern INT16 global_ldc3;

extern UINT32 lcode;
extern UINT16 bitindex;

#endif
