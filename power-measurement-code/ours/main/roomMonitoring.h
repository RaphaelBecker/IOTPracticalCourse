#ifndef ROOMMONITORING_H
#define ROOMMONITORING_H

#define triggerPinIn 34
#define triggerPinOut 35


void timeWatchDog();
void monitorTriggerPinFlags();
void configureRoomMonitoring();
void taskRoomMonitoring();
void publishCountOnChange();
void publishCountEverySecond();

#endif /* ROOMMONITORING_H */