# libSceAc3Enc technical debt

### Unknown function info

- [sceAc3EncCreateEncoder (`PGxMwm8uS0Y`), sceAc3EncDeleteEncoder (`jBntlBiKty8`), sceAc3EncEncode (`E79Au3w61PM`), sceAc3EncEncodeAsync (`336zdcN++6g`), sceAc3EncFlush (`0ugwOX1BRjo`), sceAc3EncIsEncoding (`6eEiDgUsIPw`) and sceAc3EncWaitEncode (`hemgFpeEQFA`)](../../../core/libs/prx/libSceAc3Enc/Export.cpp) (libSceAc3Enc) - names and NIDs are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceAc3Enc.c) and each NID matches a same-library export in PS5 13.60 firmware. No credible prototypes were found. They are declared without parameters solely as throwing export stubs; this is not an ABI claim and callers must not call them until signatures are verified.
