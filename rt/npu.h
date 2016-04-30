#define STAT_ADDR 0xFFFC0000
#define STAT_SIZE 0x00001000

#define DATA_ADDR 0xFFFEC000
#define DATA_SIZE 0x00014000

#define SRC_START 0xFFFF0000
#define DST_START 0xFFFF8000

#define SRC_OFFSET (SRC_START - DATA_ADDR)
#define DST_OFFSET (DST_START - DATA_ADDR)

#define isb() __asm__ __volatile__ ("isb" : : : "memory")
#define dsb() __asm__ __volatile__ ("dsb" : : : "memory")
#define dmb() __asm__ __volatile__ ("dmb" : : : "memory")
#define barrier() dmb(); dsb(); isb()
#define sev() __asm__ __volatile__ ("SEV\n")
#define wfe() __asm__ __volatile__ ("WFE\n")

extern void * src_start;
extern void * dst_start;

int npu_map();
int npu_unmap();

void npu();

void npu_config(char *);
