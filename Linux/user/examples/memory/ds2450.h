#define INPUT		0
#define INPUT_PULLUP	1
#define OUTPUT		2
#define LOW		0
#define HIGH		1
#define DEFAULT		0
#define INTERNAL	1

typedef struct ds2450 {
    char id[16];
    int mem_fd;
    int adc_fd;
} *ds2450_t;

#ifdef DS2450_C
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN ds2450_t ds2450_new(const char *id);
EXTERN void ds2450_free(ds2450_t d);
EXTERN int ds2450_pinMode(ds2450_t d, unsigned pin, unsigned mode);
#define ds2450_digitalWrite(d, p, v) ds2450_pinMode((d), (p), OUTPUT|((v)?1:0))
EXTERN int ds2450_digitalRead(ds2450_t d, unsigned pin);
EXTERN int ds2450_analogWrite(ds2450_t d, unsigned pin, unsigned val);
EXTERN int ds2450_analogConvert(ds2450_t d, unsigned p1, unsigned p2, unsigned p3, unsigned p4);
EXTERN int ds2450_analogReadLast(ds2450_t d, unsigned pin);
EXTERN int ds2450_analogReadLastAll(ds2450_t d, unsigned short res[4]);
EXTERN int ds2450_analogRead(ds2450_t d, unsigned pin);
EXTERN int ds2450_analogResolution(ds2450_t d, unsigned pin, unsigned val);
EXTERN int ds2450_analogReference(ds2450_t d, unsigned pin, unsigned val);

#undef EXTERN
