#ifndef ROOMMONITORING_H
#define ROOMMONITORING_H

#define triggerPinIn 0
#define triggerPinOut 2


void timeWatchDog();
void monitorTriggerPinFlags();
void configureRoomMonitoring();
void taskRoomMonitoring();
void publishCountOnChange();
void publishCountEverySecond();

#endif /* ROOMMONITORING_H */