# libSceWebBrowserDialog technical debt

### Silent stubs

- [sceWebBrowserDialogOpen](../../../core/libs/prx/libSceWebBrowserDialog/Export.cpp) (libSceWebBrowserDialog) - finishes at once with no browser UI; `GetResult` reports user canceled
- [sceWebBrowserDialogOpenForPredeterminedContent](../../../core/libs/prx/libSceWebBrowserDialog/Export.cpp) (libSceWebBrowserDialog) - same as `Open`: finishes at once with no browser UI. Parameter list is unknown in-tree; the export stays parameterless like the other undischarged WebBrowserDialog stubs
