

#ifndef DEFINES_H_
#define DEFINES_H_

#define CHANNEL RMT_CHANNEL_0
#define GPIO undefined
#define NUMLEDS (60)
#define BUFSIZE (NUMLEDS * 3)
#define RAINBOW_PIN (4)	 // 4, 21, 12, 15
#define CHAN1 (1)	 // 4, 21, 12, 15
#define VUMETER_PIN (21)
#define CHAN2 (3)
#define DIFF_PIN (12)
#define CHAN3 (5)
#define PHASE_PIN (15)
#define CHAN4 (7)
#define LEDPRIORITY (configMAX_PRIORITIES-1)
#define ANALYSISPRIORITY (configMAX_PRIORITIES-2)
#define FFTPRIORITY (configMAX_PRIORITIES-3)
#define TESTPRIORITY (2)
#define FFTLEN (4096)
#define LG2FFTLEN (12)

#endif /* DEFINES_H_ */
