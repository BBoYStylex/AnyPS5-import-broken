import os
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import nid_names

SDK_OBJECT_NAMES = {
    "Need_sceLibcInternal", "_CTinfo",
    "_Clocale", "_CurrentRuneLocale",
    "_Dbl", "_DefaultRuneLocale",
    "_Denorm", "_Eps",
    "_Erf_one", "_Erf_small",
    "_FDenorm", "_FEps",
    "_FErf_one", "_FErf_small",
    "_FGamma_big", "_FInf",
    "_FNan", "_FRteps",
    "_FSnan", "_FXbig",
    "_FZero", "_Files",
    "_Flt", "_Gamma_big",
    "_Hugeval", "_Inf",
    "_LDenorm", "_LEps",
    "_LErf_one", "_LErf_small",
    "_LGamma_big", "_LInf",
    "_LNan", "_LRteps",
    "_LSnan", "_LXbig",
    "_LZero", "_Ldbl",
    "_Loctab", "_Nan",
    "_PJP_CPP_Copyright", "_PJP_C_Copyright",
    "_PathLocale", "_Randseed",
    "_Rteps", "_SceLibcDebugOut",
    "_SceLibcTelemetoryOut", "_Shared_ptr_flag",
    "_Snan", "_Stderr",
    "_Stdin", "_Stdout",
    "_Tls_setup__Costate", "_Tls_setup__Ctype",
    "_Tls_setup__Errno", "_Tls_setup__Locale",
    "_Tls_setup__Mbcurmax", "_Tls_setup__Mbstate",
    "_Tls_setup__Times", "_Tls_setup__Tolotab",
    "_Tls_setup__Touptab", "_Tls_setup__WCostate",
    "_Tls_setup__Wcstate", "_Tls_setup__Wctrans",
    "_Tls_setup__Wctype", "_Xbig",
    "_ZGVNSt10moneypunctIcLb0EE2idE", "_ZGVNSt10moneypunctIcLb1EE2idE",
    "_ZGVNSt10moneypunctIwLb0EE2idE", "_ZGVNSt10moneypunctIwLb1EE2idE",
    "_ZGVNSt14_Error_objectsIiE14_System_objectE", "_ZGVNSt14_Error_objectsIiE15_Generic_objectE",
    "_ZGVNSt14_Error_objectsIiE16_Iostream_objectE", "_ZGVNSt20_Future_error_objectIiE14_Future_objectE",
    "_ZGVNSt7codecvtIcc9_MbstatetE2idE", "_ZGVNSt7collateIcE2idE",
    "_ZGVNSt7collateIwE2idE", "_ZGVNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE",
    "_ZGVNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE", "_ZGVNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE",
    "_ZGVNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE", "_ZGVNSt8messagesIcE2idE",
    "_ZGVNSt8messagesIwE2idE", "_ZGVNSt8numpunctIcE2idE",
    "_ZGVNSt8numpunctIwE2idE", "_ZGVNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE",
    "_ZGVNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE", "_ZGVNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE",
    "_ZGVNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE", "_ZGVNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE",
    "_ZGVNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE", "_ZGVNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE",
    "_ZGVZNSt13basic_filebufIcSt11char_traitsIcEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit", "_ZGVZNSt13basic_filebufIwSt11char_traitsIwEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit",
    "_ZNSt10moneypunctIcLb0EE2idE", "_ZNSt10moneypunctIcLb0EE4intlE",
    "_ZNSt10moneypunctIcLb1EE2idE", "_ZNSt10moneypunctIcLb1EE4intlE",
    "_ZNSt10moneypunctIwLb0EE2idE", "_ZNSt10moneypunctIwLb0EE4intlE",
    "_ZNSt10moneypunctIwLb1EE2idE", "_ZNSt10moneypunctIwLb1EE4intlE",
    "_ZNSt12placeholders2_1E", "_ZNSt12placeholders2_2E",
    "_ZNSt12placeholders2_3E", "_ZNSt12placeholders2_4E",
    "_ZNSt12placeholders2_5E", "_ZNSt12placeholders2_6E",
    "_ZNSt12placeholders2_7E", "_ZNSt12placeholders2_8E",
    "_ZNSt12placeholders2_9E", "_ZNSt12placeholders3_10E",
    "_ZNSt12placeholders3_11E", "_ZNSt12placeholders3_12E",
    "_ZNSt12placeholders3_13E", "_ZNSt12placeholders3_14E",
    "_ZNSt12placeholders3_15E", "_ZNSt12placeholders3_16E",
    "_ZNSt12placeholders3_17E", "_ZNSt12placeholders3_18E",
    "_ZNSt12placeholders3_19E", "_ZNSt12placeholders3_20E",
    "_ZNSt13_Num_int_base10is_boundedE", "_ZNSt13_Num_int_base10is_integerE",
    "_ZNSt13_Num_int_base14is_specializedE", "_ZNSt13_Num_int_base5radixE",
    "_ZNSt13_Num_int_base8is_exactE", "_ZNSt13_Num_int_base9is_moduloE",
    "_ZNSt13_Regex_traitsIcE6_NamesE", "_ZNSt13_Regex_traitsIwE6_NamesE",
    "_ZNSt14_Error_objectsIiE14_System_objectE", "_ZNSt14_Error_objectsIiE15_Generic_objectE",
    "_ZNSt14_Error_objectsIiE16_Iostream_objectE", "_ZNSt14_Num_ldbl_base10has_denormE",
    "_ZNSt14_Num_ldbl_base10is_boundedE", "_ZNSt14_Num_ldbl_base10is_integerE",
    "_ZNSt14_Num_ldbl_base11round_styleE", "_ZNSt14_Num_ldbl_base12has_infinityE",
    "_ZNSt14_Num_ldbl_base13has_quiet_NaNE", "_ZNSt14_Num_ldbl_base14is_specializedE",
    "_ZNSt14_Num_ldbl_base15has_denorm_lossE", "_ZNSt14_Num_ldbl_base15tinyness_beforeE",
    "_ZNSt14_Num_ldbl_base17has_signaling_NaNE", "_ZNSt14_Num_ldbl_base5radixE",
    "_ZNSt14_Num_ldbl_base5trapsE", "_ZNSt14_Num_ldbl_base8is_exactE",
    "_ZNSt14_Num_ldbl_base9is_iec559E", "_ZNSt14_Num_ldbl_base9is_moduloE",
    "_ZNSt14_Num_ldbl_base9is_signedE", "_ZNSt14numeric_limitsIDiE6digitsE",
    "_ZNSt14numeric_limitsIDiE8digits10E", "_ZNSt14numeric_limitsIDiE9is_signedE",
    "_ZNSt14numeric_limitsIDsE6digitsE", "_ZNSt14numeric_limitsIDsE8digits10E",
    "_ZNSt14numeric_limitsIDsE9is_signedE", "_ZNSt14numeric_limitsIaE6digitsE",
    "_ZNSt14numeric_limitsIaE8digits10E", "_ZNSt14numeric_limitsIaE9is_signedE",
    "_ZNSt14numeric_limitsIbE6digitsE", "_ZNSt14numeric_limitsIbE8digits10E",
    "_ZNSt14numeric_limitsIbE9is_moduloE", "_ZNSt14numeric_limitsIbE9is_signedE",
    "_ZNSt14numeric_limitsIcE6digitsE", "_ZNSt14numeric_limitsIcE8digits10E",
    "_ZNSt14numeric_limitsIcE9is_signedE", "_ZNSt14numeric_limitsIdE12max_digits10E",
    "_ZNSt14numeric_limitsIdE12max_exponentE", "_ZNSt14numeric_limitsIdE12min_exponentE",
    "_ZNSt14numeric_limitsIdE14max_exponent10E", "_ZNSt14numeric_limitsIdE14min_exponent10E",
    "_ZNSt14numeric_limitsIdE6digitsE", "_ZNSt14numeric_limitsIdE8digits10E",
    "_ZNSt14numeric_limitsIeE12max_digits10E", "_ZNSt14numeric_limitsIeE12max_exponentE",
    "_ZNSt14numeric_limitsIeE12min_exponentE", "_ZNSt14numeric_limitsIeE14max_exponent10E",
    "_ZNSt14numeric_limitsIeE14min_exponent10E", "_ZNSt14numeric_limitsIeE6digitsE",
    "_ZNSt14numeric_limitsIeE8digits10E", "_ZNSt14numeric_limitsIfE12max_digits10E",
    "_ZNSt14numeric_limitsIfE12max_exponentE", "_ZNSt14numeric_limitsIfE12min_exponentE",
    "_ZNSt14numeric_limitsIfE14max_exponent10E", "_ZNSt14numeric_limitsIfE14min_exponent10E",
    "_ZNSt14numeric_limitsIfE6digitsE", "_ZNSt14numeric_limitsIfE8digits10E",
    "_ZNSt14numeric_limitsIhE6digitsE", "_ZNSt14numeric_limitsIhE8digits10E",
    "_ZNSt14numeric_limitsIhE9is_signedE", "_ZNSt14numeric_limitsIiE6digitsE",
    "_ZNSt14numeric_limitsIiE8digits10E", "_ZNSt14numeric_limitsIiE9is_signedE",
    "_ZNSt14numeric_limitsIjE6digitsE", "_ZNSt14numeric_limitsIjE8digits10E",
    "_ZNSt14numeric_limitsIjE9is_signedE", "_ZNSt14numeric_limitsIlE6digitsE",
    "_ZNSt14numeric_limitsIlE8digits10E", "_ZNSt14numeric_limitsIlE9is_signedE",
    "_ZNSt14numeric_limitsImE6digitsE", "_ZNSt14numeric_limitsImE8digits10E",
    "_ZNSt14numeric_limitsImE9is_signedE", "_ZNSt14numeric_limitsIsE6digitsE",
    "_ZNSt14numeric_limitsIsE8digits10E", "_ZNSt14numeric_limitsIsE9is_signedE",
    "_ZNSt14numeric_limitsItE6digitsE", "_ZNSt14numeric_limitsItE8digits10E",
    "_ZNSt14numeric_limitsItE9is_signedE", "_ZNSt14numeric_limitsIwE6digitsE",
    "_ZNSt14numeric_limitsIwE8digits10E", "_ZNSt14numeric_limitsIwE9is_signedE",
    "_ZNSt14numeric_limitsIxE6digitsE", "_ZNSt14numeric_limitsIxE8digits10E",
    "_ZNSt14numeric_limitsIxE9is_signedE", "_ZNSt14numeric_limitsIyE6digitsE",
    "_ZNSt14numeric_limitsIyE8digits10E", "_ZNSt14numeric_limitsIyE9is_signedE",
    "_ZNSt15_Num_float_base10has_denormE", "_ZNSt15_Num_float_base10is_boundedE",
    "_ZNSt15_Num_float_base10is_integerE", "_ZNSt15_Num_float_base11round_styleE",
    "_ZNSt15_Num_float_base12has_infinityE", "_ZNSt15_Num_float_base13has_quiet_NaNE",
    "_ZNSt15_Num_float_base14is_specializedE", "_ZNSt15_Num_float_base15has_denorm_lossE",
    "_ZNSt15_Num_float_base15tinyness_beforeE", "_ZNSt15_Num_float_base17has_signaling_NaNE",
    "_ZNSt15_Num_float_base5radixE", "_ZNSt15_Num_float_base5trapsE",
    "_ZNSt15_Num_float_base8is_exactE", "_ZNSt15_Num_float_base9is_iec559E",
    "_ZNSt15_Num_float_base9is_moduloE", "_ZNSt15_Num_float_base9is_signedE",
    "_ZNSt20_Future_error_objectIiE14_Future_objectE", "_ZNSt5ctypeIcE10table_sizeE",
    "_ZNSt5ctypeIcE2idE", "_ZNSt5ctypeIwE2idE",
    "_ZNSt6_Winit9_Init_cntE", "_ZNSt6chrono12steady_clock12is_monotonicE",
    "_ZNSt6chrono12steady_clock9is_steadyE", "_ZNSt6chrono12system_clock12is_monotonicE",
    "_ZNSt6chrono12system_clock9is_steadyE", "_ZNSt6locale2id7_Id_cntE",
    "_ZNSt6locale7_Locimp8_ClocptrE", "_ZNSt7codecvtIDic9_MbstatetE2idE",
    "_ZNSt7codecvtIDsc9_MbstatetE2idE", "_ZNSt7codecvtIcc9_MbstatetE2idE",
    "_ZNSt7codecvtIwc9_MbstatetE2idE", "_ZNSt7collateIcE2idE",
    "_ZNSt7collateIwE2idE", "_ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE",
    "_ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE", "_ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE",
    "_ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE", "_ZNSt8ios_base4Init9_Init_cntE",
    "_ZNSt8ios_base5_SyncE", "_ZNSt8ios_base6_IndexE",
    "_ZNSt8messagesIcE2idE", "_ZNSt8messagesIwE2idE",
    "_ZNSt8numpunctIcE2idE", "_ZNSt8numpunctIwE2idE",
    "_ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE", "_ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE",
    "_ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE", "_ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE",
    "_ZNSt9_Num_base10has_denormE", "_ZNSt9_Num_base10is_boundedE",
    "_ZNSt9_Num_base10is_integerE", "_ZNSt9_Num_base11round_styleE",
    "_ZNSt9_Num_base12has_infinityE", "_ZNSt9_Num_base12max_digits10E",
    "_ZNSt9_Num_base12max_exponentE", "_ZNSt9_Num_base12min_exponentE",
    "_ZNSt9_Num_base13has_quiet_NaNE", "_ZNSt9_Num_base14is_specializedE",
    "_ZNSt9_Num_base14max_exponent10E", "_ZNSt9_Num_base14min_exponent10E",
    "_ZNSt9_Num_base15has_denorm_lossE", "_ZNSt9_Num_base15tinyness_beforeE",
    "_ZNSt9_Num_base17has_signaling_NaNE", "_ZNSt9_Num_base5radixE",
    "_ZNSt9_Num_base5trapsE", "_ZNSt9_Num_base6digitsE",
    "_ZNSt9_Num_base8digits10E", "_ZNSt9_Num_base8is_exactE",
    "_ZNSt9_Num_base9is_iec559E", "_ZNSt9_Num_base9is_moduloE",
    "_ZNSt9_Num_base9is_signedE", "_ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE",
    "_ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE", "_ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE",
    "_ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE", "_ZSt10adopt_lock",
    "_ZSt10defer_lock", "_ZSt11try_to_lock",
    "_ZSt13_Cl_charnames", "_ZSt14_Cl_wcharnames",
    "_ZSt14_Raise_handler", "_ZSt18_String_cpp_unused",
    "_ZSt21_sceLibcClassicLocale", "_ZSt3cin",
    "_ZSt4_Fpz", "_ZSt4cerr",
    "_ZSt4clog", "_ZSt4cout",
    "_ZSt4wcin", "_ZSt5wcerr",
    "_ZSt5wclog", "_ZSt5wcout",
    "_ZSt6ignore", "_ZSt7_BADOFF",
    "_ZSt7nothrow", "_ZTIDh",
    "_ZTIDi", "_ZTIDn",
    "_ZTIDs", "_ZTIN10__cxxabiv116__enum_type_infoE",
    "_ZTIN10__cxxabiv117__array_type_infoE", "_ZTIN10__cxxabiv117__class_type_infoE",
    "_ZTIN10__cxxabiv117__pbase_type_infoE", "_ZTIN10__cxxabiv119__pointer_type_infoE",
    "_ZTIN10__cxxabiv120__function_type_infoE", "_ZTIN10__cxxabiv120__si_class_type_infoE",
    "_ZTIN10__cxxabiv121__vmi_class_type_infoE", "_ZTIN10__cxxabiv123__fundamental_type_infoE",
    "_ZTIN10__cxxabiv129__pointer_to_member_type_infoE", "_ZTIN6Dinkum7threads10lock_errorE",
    "_ZTIN6Dinkum7threads21thread_resource_errorE", "_ZTINSt6locale5facetE",
    "_ZTINSt6locale7_LocimpE", "_ZTINSt8ios_base7failureE",
    "_ZTIPDh", "_ZTIPDi",
    "_ZTIPDn", "_ZTIPDs",
    "_ZTIPKDh", "_ZTIPKDi",
    "_ZTIPKDn", "_ZTIPKDs",
    "_ZTIPKa", "_ZTIPKb",
    "_ZTIPKc", "_ZTIPKd",
    "_ZTIPKe", "_ZTIPKf",
    "_ZTIPKh", "_ZTIPKi",
    "_ZTIPKj", "_ZTIPKl",
    "_ZTIPKm", "_ZTIPKn",
    "_ZTIPKo", "_ZTIPKs",
    "_ZTIPKt", "_ZTIPKv",
    "_ZTIPKw", "_ZTIPKx",
    "_ZTIPKy", "_ZTIPa",
    "_ZTIPb", "_ZTIPc",
    "_ZTIPd", "_ZTIPe",
    "_ZTIPf", "_ZTIPh",
    "_ZTIPi", "_ZTIPj",
    "_ZTIPl", "_ZTIPm",
    "_ZTIPn", "_ZTIPo",
    "_ZTIPs", "_ZTIPt",
    "_ZTIPv", "_ZTIPw",
    "_ZTIPx", "_ZTIPy",
    "_ZTISi", "_ZTISo",
    "_ZTISt10bad_typeid", "_ZTISt10ctype_base",
    "_ZTISt10money_base", "_ZTISt10moneypunctIcLb0EE",
    "_ZTISt10moneypunctIcLb1EE", "_ZTISt10moneypunctIwLb0EE",
    "_ZTISt10moneypunctIwLb1EE", "_ZTISt11_Facet_base",
    "_ZTISt11logic_error", "_ZTISt11range_error",
    "_ZTISt11regex_error", "_ZTISt12bad_weak_ptr",
    "_ZTISt12codecvt_base", "_ZTISt12domain_error",
    "_ZTISt12future_error", "_ZTISt12length_error",
    "_ZTISt12out_of_range", "_ZTISt12system_error",
    "_ZTISt13bad_exception", "_ZTISt13basic_filebufIcSt11char_traitsIcEE",
    "_ZTISt13basic_filebufIwSt11char_traitsIwEE", "_ZTISt13basic_istreamIwSt11char_traitsIwEE",
    "_ZTISt13basic_ostreamIwSt11char_traitsIwEE", "_ZTISt13messages_base",
    "_ZTISt13runtime_error", "_ZTISt14error_category",
    "_ZTISt14overflow_error", "_ZTISt15basic_streambufIcSt11char_traitsIcEE",
    "_ZTISt15basic_streambufIwSt11char_traitsIwEE", "_ZTISt15underflow_error",
    "_ZTISt16invalid_argument", "_ZTISt16nested_exception",
    "_ZTISt17bad_function_call", "_ZTISt18bad_variant_access",
    "_ZTISt20bad_array_new_length", "_ZTISt22_Future_error_category",
    "_ZTISt22_System_error_category", "_ZTISt23_Generic_error_category",
    "_ZTISt24_Iostream_error_category", "_ZTISt4_Pad",
    "_ZTISt5_IosbIiE", "_ZTISt5ctypeIcE",
    "_ZTISt5ctypeIwE", "_ZTISt7_MpunctIcE",
    "_ZTISt7_MpunctIwE", "_ZTISt7codecvtIDic9_MbstatetE",
    "_ZTISt7codecvtIDsc9_MbstatetE", "_ZTISt7codecvtIcc9_MbstatetE",
    "_ZTISt7codecvtIwc9_MbstatetE", "_ZTISt7collateIcE",
    "_ZTISt7collateIwE", "_ZTISt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTISt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTISt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTISt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTISt8bad_cast",
    "_ZTISt8ios_base", "_ZTISt8messagesIcE",
    "_ZTISt8messagesIwE", "_ZTISt8numpunctIcE",
    "_ZTISt8numpunctIwE", "_ZTISt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTISt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTISt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTISt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTISt9bad_alloc",
    "_ZTISt9basic_iosIcSt11char_traitsIcEE", "_ZTISt9basic_iosIwSt11char_traitsIwEE",
    "_ZTISt9exception", "_ZTISt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTISt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTISt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTISt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTISt9time_base",
    "_ZTISt9type_info", "_ZTIa",
    "_ZTIb", "_ZTIc",
    "_ZTId", "_ZTIe",
    "_ZTIf", "_ZTIh",
    "_ZTIi", "_ZTIj",
    "_ZTIl", "_ZTIm",
    "_ZTIn", "_ZTIo",
    "_ZTIs", "_ZTIt",
    "_ZTIv", "_ZTIw",
    "_ZTIx", "_ZTIy",
    "_ZTSDi", "_ZTSDn",
    "_ZTSDs", "_ZTSN10__cxxabiv116__enum_type_infoE",
    "_ZTSN10__cxxabiv117__array_type_infoE", "_ZTSN10__cxxabiv117__class_type_infoE",
    "_ZTSN10__cxxabiv117__pbase_type_infoE", "_ZTSN10__cxxabiv119__pointer_type_infoE",
    "_ZTSN10__cxxabiv120__function_type_infoE", "_ZTSN10__cxxabiv120__si_class_type_infoE",
    "_ZTSN10__cxxabiv121__vmi_class_type_infoE", "_ZTSN10__cxxabiv123__fundamental_type_infoE",
    "_ZTSN10__cxxabiv129__pointer_to_member_type_infoE", "_ZTSN6Dinkum7threads10lock_errorE",
    "_ZTSN6Dinkum7threads21thread_resource_errorE", "_ZTSNSt6locale5facetE",
    "_ZTSNSt6locale7_LocimpE", "_ZTSNSt8ios_base7failureE",
    "_ZTSPDi", "_ZTSPDn",
    "_ZTSPDs", "_ZTSPKDi",
    "_ZTSPKDn", "_ZTSPKDs",
    "_ZTSPKa", "_ZTSPKb",
    "_ZTSPKc", "_ZTSPKd",
    "_ZTSPKe", "_ZTSPKf",
    "_ZTSPKh", "_ZTSPKi",
    "_ZTSPKj", "_ZTSPKl",
    "_ZTSPKm", "_ZTSPKs",
    "_ZTSPKt", "_ZTSPKv",
    "_ZTSPKw", "_ZTSPKx",
    "_ZTSPKy", "_ZTSPa",
    "_ZTSPb", "_ZTSPc",
    "_ZTSPd", "_ZTSPe",
    "_ZTSPf", "_ZTSPh",
    "_ZTSPi", "_ZTSPj",
    "_ZTSPl", "_ZTSPm",
    "_ZTSPs", "_ZTSPt",
    "_ZTSPv", "_ZTSPw",
    "_ZTSPx", "_ZTSPy",
    "_ZTSSi", "_ZTSSo",
    "_ZTSSt10bad_typeid", "_ZTSSt10ctype_base",
    "_ZTSSt10money_base", "_ZTSSt10moneypunctIcLb0EE",
    "_ZTSSt10moneypunctIcLb1EE", "_ZTSSt10moneypunctIwLb0EE",
    "_ZTSSt10moneypunctIwLb1EE", "_ZTSSt11_Facet_base",
    "_ZTSSt11logic_error", "_ZTSSt11range_error",
    "_ZTSSt11regex_error", "_ZTSSt12bad_weak_ptr",
    "_ZTSSt12codecvt_base", "_ZTSSt12domain_error",
    "_ZTSSt12future_error", "_ZTSSt12length_error",
    "_ZTSSt12out_of_range", "_ZTSSt12system_error",
    "_ZTSSt13bad_exception", "_ZTSSt13basic_filebufIcSt11char_traitsIcEE",
    "_ZTSSt13basic_filebufIwSt11char_traitsIwEE", "_ZTSSt13basic_istreamIwSt11char_traitsIwEE",
    "_ZTSSt13basic_ostreamIwSt11char_traitsIwEE", "_ZTSSt13messages_base",
    "_ZTSSt13runtime_error", "_ZTSSt14error_category",
    "_ZTSSt14overflow_error", "_ZTSSt15basic_streambufIcSt11char_traitsIcEE",
    "_ZTSSt15basic_streambufIwSt11char_traitsIwEE", "_ZTSSt15underflow_error",
    "_ZTSSt16invalid_argument", "_ZTSSt16nested_exception",
    "_ZTSSt17bad_function_call", "_ZTSSt18bad_variant_access",
    "_ZTSSt20bad_array_new_length", "_ZTSSt22_Future_error_category",
    "_ZTSSt22_System_error_category", "_ZTSSt23_Generic_error_category",
    "_ZTSSt24_Iostream_error_category", "_ZTSSt4_Pad",
    "_ZTSSt5_IosbIiE", "_ZTSSt5ctypeIcE",
    "_ZTSSt5ctypeIwE", "_ZTSSt7_MpunctIcE",
    "_ZTSSt7_MpunctIwE", "_ZTSSt7codecvtIDic9_MbstatetE",
    "_ZTSSt7codecvtIDsc9_MbstatetE", "_ZTSSt7codecvtIcc9_MbstatetE",
    "_ZTSSt7codecvtIwc9_MbstatetE", "_ZTSSt7collateIcE",
    "_ZTSSt7collateIwE", "_ZTSSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTSSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTSSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTSSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTSSt8bad_cast",
    "_ZTSSt8ios_base", "_ZTSSt8messagesIcE",
    "_ZTSSt8messagesIwE", "_ZTSSt8numpunctIcE",
    "_ZTSSt8numpunctIwE", "_ZTSSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTSSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTSSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTSSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTSSt9bad_alloc",
    "_ZTSSt9basic_iosIcSt11char_traitsIcEE", "_ZTSSt9basic_iosIwSt11char_traitsIwEE",
    "_ZTSSt9exception", "_ZTSSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTSSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTSSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE",
    "_ZTSSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE", "_ZTSSt9time_base",
    "_ZTSSt9type_info", "_ZTSa",
    "_ZTSb", "_ZTSc",
    "_ZTSd", "_ZTSe",
    "_ZTSf", "_ZTSh",
    "_ZTSi", "_ZTSj",
    "_ZTSl", "_ZTSm",
    "_ZTSs", "_ZTSt",
    "_ZTSv", "_ZTSw",
    "_ZTSx", "_ZTSy",
    "_ZTVN10__cxxabiv116__enum_type_infoE", "_ZTVN10__cxxabiv117__array_type_infoE",
    "_ZTVN10__cxxabiv117__class_type_infoE", "_ZTVN10__cxxabiv117__pbase_type_infoE",
    "_ZTVN10__cxxabiv119__pointer_type_infoE", "_ZTVN10__cxxabiv120__function_type_infoE",
    "_ZTVN10__cxxabiv120__si_class_type_infoE", "_ZTVN10__cxxabiv121__vmi_class_type_infoE",
    "_ZTVN10__cxxabiv123__fundamental_type_infoE", "_ZTVN10__cxxabiv129__pointer_to_member_type_infoE",
    "_ZTVN4IPMI6Server12EventHandlerE", "_ZTVN6Dinkum7threads10lock_errorE",
    "_ZTVN6Dinkum7threads21thread_resource_errorE", "_ZTVNSt6locale7_LocimpE",
    "_ZTVNSt8ios_base7failureE", "_ZTVSi",
    "_ZTVSo", "_ZTVSt10bad_typeid",
    "_ZTVSt10moneypunctIcLb0EE", "_ZTVSt10moneypunctIcLb1EE",
    "_ZTVSt10moneypunctIwLb0EE", "_ZTVSt10moneypunctIwLb1EE",
    "_ZTVSt11logic_error", "_ZTVSt11range_error",
    "_ZTVSt11regex_error", "_ZTVSt12bad_weak_ptr",
    "_ZTVSt12domain_error", "_ZTVSt12future_error",
    "_ZTVSt12length_error", "_ZTVSt12out_of_range",
    "_ZTVSt12system_error", "_ZTVSt13bad_exception",
    "_ZTVSt13basic_filebufIcSt11char_traitsIcEE", "_ZTVSt13basic_filebufIwSt11char_traitsIwEE",
    "_ZTVSt13basic_istreamIwSt11char_traitsIwEE", "_ZTVSt13basic_ostreamIwSt11char_traitsIwEE",
    "_ZTVSt13runtime_error", "_ZTVSt14error_category",
    "_ZTVSt14overflow_error", "_ZTVSt15underflow_error",
    "_ZTVSt16invalid_argument", "_ZTVSt16nested_exception",
    "_ZTVSt17bad_function_call", "_ZTVSt18bad_variant_access",
    "_ZTVSt20bad_array_new_length", "_ZTVSt22_Future_error_category",
    "_ZTVSt22_System_error_category", "_ZTVSt23_Generic_error_category",
    "_ZTVSt24_Iostream_error_category", "_ZTVSt4_Pad",
    "_ZTVSt5ctypeIcE", "_ZTVSt5ctypeIwE",
    "_ZTVSt7_MpunctIcE", "_ZTVSt7_MpunctIwE",
    "_ZTVSt7codecvtIDic9_MbstatetE", "_ZTVSt7codecvtIDsc9_MbstatetE",
    "_ZTVSt7codecvtIcc9_MbstatetE", "_ZTVSt7codecvtIwc9_MbstatetE",
    "_ZTVSt7collateIcE", "_ZTVSt7collateIwE",
    "_ZTVSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE", "_ZTVSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE",
    "_ZTVSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE", "_ZTVSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE",
    "_ZTVSt8bad_cast", "_ZTVSt8ios_base",
    "_ZTVSt8messagesIcE", "_ZTVSt8messagesIwE",
    "_ZTVSt8numpunctIcE", "_ZTVSt8numpunctIwE",
    "_ZTVSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE", "_ZTVSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE",
    "_ZTVSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE", "_ZTVSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE",
    "_ZTVSt9bad_alloc", "_ZTVSt9exception",
    "_ZTVSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE", "_ZTVSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE",
    "_ZTVSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE", "_ZTVSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE",
    "_ZTVSt9type_info", "_ZZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetffldEPcRS3_S6_RSt8ios_basePiE4_Src",
    "_ZZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetifldEPcRS3_S6_NSt5_IosbIiE9_FmtflagsERKSt6localeE4_Src", "_ZZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE9_GetffldxEPcRS3_S6_RSt8ios_basePiE4_Src",
    "_ZZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetffldEPcRS3_S6_RSt8ios_basePiE4_Src", "_ZZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetifldEPcRS3_S6_NSt5_IosbIiE9_FmtflagsERKSt6localeE4_Src",
    "_ZZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE9_GetffldxEPcRS3_S6_RSt8ios_basePiE4_Src", "_ZZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetmfldERS3_S5_bRSt8ios_basePcE4_Src",
    "_ZZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetmfldERS3_S5_bRSt8ios_basePwE4_Src", "_ZZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_bRSt8ios_basecRKSsE4_Src",
    "_ZZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_bRSt8ios_basewRKSbIwS2_SaIwEEE4_Src", "_ZZNSt13basic_filebufIcSt11char_traitsIcEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit",
    "_ZZNSt13basic_filebufIwSt11char_traitsIwEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit", "_Zero",
    "__cleanup", "__fe_dfl_env",
    "__isthreaded", "__mb_cur_max",
    "__mb_sb_limit", "__progname",
    "__stack_chk_guard", "__stderrp",
    "__stdinp", "__stdoutp",
    "_sigintr", "_thread_autoinit_dummy_decl",
    "_thread_autoinit_dummy_decl_stub", "daylight",
    "environ", "errno",
    "in6addr_any", "in6addr_loopback",
    "mono_build_date", "optarg",
    "opterr", "optind",
    "optopt", "optreset",
    "sceLibcDebugOut", "sceLibcOnce",
    "sce_net_dummy", "sce_net_in6addr_any",
    "sce_net_in6addr_linklocal_allnodes", "sce_net_in6addr_linklocal_allrouters",
    "sce_net_in6addr_loopback", "sce_net_in6addr_nodelocal_allnodes",
    "signgam", "stderr",
    "stdin", "stdout",
    "sys_nsig", "sys_siglist",
    "sys_signame", "timezone",
    "tzname",
}

SDK_OBJECT_NIDS = {nid_names.compute_nid(name) for name in SDK_OBJECT_NAMES}


class Export:
    def __init__(self, name, rva, kind):
        self.name = name
        self.rva = rva
        self.kind = kind

    def __repr__(self):
        return f"{self.name} at 0x{self.rva:x} ({self.kind})"


def elf_exports(path, data):
    """Every exported symbol of an ELF library, with its address and kind."""
    header = struct.unpack_from("<16sHHIQQQIHHHHHH", data, 0)
    section_offset, section_size, section_count = header[6], header[11], header[12]
    if not section_offset:
        return []
    sections = [struct.unpack_from("<IIQQQQIIQQ", data, section_offset + index * section_size)
                for index in range(section_count)]
    names = []
    for section in sections:
        if section[1] != 11 or section[9] != 24:
            continue
        strings = sections[section[6]]
        for position in range(0, section[5], 24):
            name, info, _, value, _, _ = struct.unpack_from("<IBBHQQ", data, section[4] + position)
            if not name or not value:
                continue
            end = data.find(b"\0", strings[4] + name)
            if end < 0:
                continue
            names.append(Export(data[strings[4] + name:end].decode("utf-8", "replace"), value,
                                "code" if (info & 0xf) == 2 else "data"))
    return names


def pe_sections(data):
    offset = struct.unpack_from("<I", data, 0x3c)[0]
    count = struct.unpack_from("<H", data, offset + 6)[0]
    size_of_optional = struct.unpack_from("<H", data, offset + 20)[0]
    table = offset + 24 + size_of_optional
    sections = []
    for index in range(count):
        entry = table + index * 40
        name = data[entry:entry + 8].rstrip(b"\0").decode("ascii", "replace")
        virtual_size = struct.unpack_from("<I", data, entry + 8)[0]
        virtual_address = struct.unpack_from("<I", data, entry + 12)[0]
        raw_size = struct.unpack_from("<I", data, entry + 16)[0]
        raw_offset = struct.unpack_from("<I", data, entry + 20)[0]
        sections.append((name, max(virtual_size, raw_size), virtual_address, raw_offset))
    return sections


def pe_exports(path, data):
    """Every exported symbol of a PE library, with its RVA and kind."""
    offset = struct.unpack_from("<I", data, 0x3c)[0]
    directory = offset + 24 + 112
    rva, size = struct.unpack_from("<II", data, directory)
    if not rva or size < 40:
        return []
    sections = pe_sections(data)

    def to_offset(target):
        for name, extent, virtual_address, raw_offset in sections:
            if virtual_address <= target < virtual_address + extent:
                return raw_offset + target - virtual_address
        raise AssertionError(f"{path}: the RVA 0x{target:x} lies outside every section")

    table = struct.unpack_from("<IIHHIIIIIII", data, to_offset(rva))
    function_count, name_count = table[6], table[7]
    functions_rva, names_rva, ordinals_rva = table[8], table[9], table[10]
    if not name_count:
        return []
    functions = struct.unpack_from(f"<{function_count}I", data, to_offset(functions_rva))
    names = struct.unpack_from(f"<{name_count}I", data, to_offset(names_rva))
    ordinals = struct.unpack_from(f"<{name_count}H", data, to_offset(ordinals_rva))
    result = []
    for index in range(name_count):
        start = to_offset(names[index])
        end = data.find(b"\0", start)
        name = data[start:end].decode("ascii", "replace")
        address = functions[ordinals[index]]
        kind = "data"
        for section, extent, virtual_address, _ in sections:
            if virtual_address <= address < virtual_address + extent and section == ".text":
                kind = "code"
        result.append(Export(name, address, kind))
    return result


def read_exports(path):
    data = path.read_bytes()
    if data[:4] == b"\x7fELF":
        return elf_exports(path, data)
    if data[:2] == b"MZ":
        return pe_exports(path, data)
    raise AssertionError(f"{path}: neither ELF nor PE")


class DataObjectExportTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(os.environ.get("ANYPS5_SOURCE_ROOT", Path.cwd())).resolve()
        self.patched = self.root / "build" / "core" / "libs" / "libs"

    def libraries(self):
        directory = self.patched
        if not directory.is_dir():
            self.skipTest(f"{directory} is not built")
        libraries = sorted(directory.glob("*.prx"))
        if not libraries:
            self.skipTest(f"{directory} holds no library")
        return libraries

    def test_no_object_export_is_a_function(self):
        """An export the SDK declares as an object must land in data, not in code.

        The SDK's stub tables mark a symbol `@object` when it is a data
        object rather than a function.  A library that exports such a symbol
        as a throwing function is wrong in a way that matters: a consumer
        that resolves the NID and reads the object's address reads the
        machine code of the stub, and there is no trap, so the read
        succeeds with garbage.
        """
        found = 0
        for library in self.libraries():
            for export in read_exports(library):
                if export.name not in SDK_OBJECT_NIDS:
                    continue
                found += 1
                with self.subTest(library=library.name, export=export.name):
                    self.assertEqual(export.kind, "data",
                                     f"{export.name} resolves to {export}, so a consumer reads code where the object is")
        self.assertGreater(found, 0, "no library exports an object the SDK declares")

    def test_the_object_names_are_the_ones_the_sdk_declares(self):
        """The names are the SDK's own, and each names one NID."""
        self.assertEqual(len(SDK_OBJECT_NAMES), 759)
        for name in sorted(SDK_OBJECT_NAMES):
            with self.subTest(name=name):
                self.assertIn(nid_names.compute_nid(name), SDK_OBJECT_NIDS)


if __name__ == "__main__":
    unittest.main()
