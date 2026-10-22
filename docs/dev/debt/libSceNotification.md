# libSceNotification technical debt

### Unknown function info

- [libSceNotification exports](../../../core/libs/prx/libSceNotification/Export.cpp) (libSceNotification) - the host does not implement the notification service, so every call throws through `NotImplemented_nid_no_patch`. sceNotificationSend, sceNotificationSendById, sceNotificationShowPsButtonPersistentBanner and sceNotificationHidePsButtonPersistentBanner are named by the [NID name database](https://github.com/zecoxao/sce_symbols/blob/2883963a0a514ba6e77407a08ef4730a65e88254/aerolib.csv), and the first two by the public PS5 payload SDK's [stub table](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/sce_stubs/libSceNotification.c) as well. No source declares their prototypes, so the parameter lists of the first three are unverified and sceNotificationHidePsButtonPersistentBanner takes no parameters: that is not an ABI claim and a caller must not call it until the signature is verified
