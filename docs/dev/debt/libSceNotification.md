# libSceNotification technical debt

### Unknown function info

- [sceNotificationSend and sceNotificationSendById](../../../core/libs/prx/libSceNotification/Export.cpp) (libSceNotification) - the console shows the message in the system notification UI; the host reports it through the kernel notification path (`sceKernelSendNotificationRequest`), which writes the text to the standard error, so no toast is shown. The parameter meanings are unknown: the type, the flag and the id are not interpreted and every notification is reported as a text notification. A message that is not terminated within 1024 bytes throws, as the kernel request's message field is that long
- [sceNotificationShowPsButtonPersistentBanner and sceNotificationHidePsButtonPersistentBanner](../../../core/libs/prx/libSceNotification/Export.cpp) (libSceNotification) - the console shows a persistent banner next to the PS button and hides it again; the host has no banner, so the parameters are reported as a notification and kept until they are hidden. The parameter layout is unknown, so it is read as a terminated string
