#ifndef _ONENET_H_
#define _ONENET_H_



_Bool OneNet_DevLink(void);

_Bool OneNet_Ping(void);

void OneNet_SendData(void);

void OneNet_RevPro(unsigned char *cmd);

#endif
