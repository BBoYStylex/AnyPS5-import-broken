#!/usr/bin/env python3
import re
import sys
import unittest
from pathlib import Path

SOURCE_EXPORTS = (
    "_ZN3sce3Xml10SimpleDataC1EPKcm",
    "_ZN3sce3Xml10SimpleDataC1Ev",
    "_ZN3sce3Xml10SimpleDataC2EPKcm",
    "_ZN3sce3Xml10SimpleDataC2Ev",
    "_ZN3sce3Xml11Initializer10initializeEPKNS0_13InitParameterE",
    "_ZN3sce3Xml11Initializer9terminateEv",
    "_ZN3sce3Xml11InitializerC1Ev",
    "_ZN3sce3Xml11InitializerC2Ev",
    "_ZN3sce3Xml11InitializerD1Ev",
    "_ZN3sce3Xml11InitializerD2Ev",
    "_ZN3sce3Xml12MemAllocatorC2Ev",
    "_ZN3sce3Xml12MemAllocatorD0Ev",
    "_ZN3sce3Xml12MemAllocatorD1Ev",
    "_ZN3sce3Xml12MemAllocatorD2Ev",
    "_ZN3sce3Xml13AttributeList10initializeEPKNS0_11InitializerE",
    "_ZN3sce3Xml13AttributeList12addAttributeEPKNS0_6StringES4_",
    "_ZN3sce3Xml13AttributeList5clearEv",
    "_ZN3sce3Xml13AttributeList9terminateEv",
    "_ZN3sce3Xml13AttributeListC1ERKS1_",
    "_ZN3sce3Xml13AttributeListC1Ev",
    "_ZN3sce3Xml13AttributeListC2ERKS1_",
    "_ZN3sce3Xml13AttributeListC2Ev",
    "_ZN3sce3Xml13AttributeListD1Ev",
    "_ZN3sce3Xml13AttributeListD2Ev",
    "_ZN3sce3Xml14VarAllocBuffer5clearEv",
    "_ZN3sce3Xml14VarAllocBuffer7copyStrERKNS0_6StringE",
    "_ZN3sce3Xml14VarAllocBuffer9terminateEv",
    "_ZN3sce3Xml14VarAllocBufferC1EPKNS0_11InitializerE",
    "_ZN3sce3Xml14VarAllocBufferC2EPKNS0_11InitializerE",
    "_ZN3sce3Xml14VarAllocBufferD0Ev",
    "_ZN3sce3Xml14VarAllocBufferD1Ev",
    "_ZN3sce3Xml14VarAllocBufferD2Ev",
    "_ZN3sce3Xml18SerializeParameterC1Ev",
    "_ZN3sce3Xml18SerializeParameterC2Ev",
    "_ZN3sce3Xml20bXResultToResultTypeEi",
    "_ZN3sce3Xml23getMemManagerDebugLevelEv",
    "_ZN3sce3Xml23setMemManagerDebugLevelEi",
    "_ZN3sce3Xml3Dom15DocumentBuilder10initializeEPKNS0_11InitializerE",
    "_ZN3sce3Xml3Dom15DocumentBuilder11getDocumentEv",
    "_ZN3sce3Xml3Dom15DocumentBuilder16setResolveEntityEb",
    "_ZN3sce3Xml3Dom15DocumentBuilder20setSkipIgnorableTextEb",
    "_ZN3sce3Xml3Dom15DocumentBuilder26setSkipIgnorableWhiteSpaceEb",
    "_ZN3sce3Xml3Dom15DocumentBuilder5parseEPKNS0_6StringEb",
    "_ZN3sce3Xml3Dom15DocumentBuilder9terminateEv",
    "_ZN3sce3Xml3Dom15DocumentBuilderC1Ev",
    "_ZN3sce3Xml3Dom15DocumentBuilderC2Ev",
    "_ZN3sce3Xml3Dom15DocumentBuilderD1Ev",
    "_ZN3sce3Xml3Dom15DocumentBuilderD2Ev",
    "_ZN3sce3Xml3Dom4Node11appendChildENS1_6NodeIdE",
    "_ZN3sce3Xml3Dom4Node11removeChildENS1_6NodeIdE",
    "_ZN3sce3Xml3Dom4Node12insertBeforeENS1_6NodeIdES3_",
    "_ZN3sce3Xml3Dom4NodeC1ENS1_6NodeIdE",
    "_ZN3sce3Xml3Dom4NodeC1ERKS2_",
    "_ZN3sce3Xml3Dom4NodeC2ENS1_6NodeIdE",
    "_ZN3sce3Xml3Dom4NodeC2ERKS2_",
    "_ZN3sce3Xml3Dom4NodeD1Ev",
    "_ZN3sce3Xml3Dom4NodeD2Ev",
    "_ZN3sce3Xml3Dom4NodeaSERKS2_",
    "_ZN3sce3Xml3Dom6NodeIdC1EPvS3_b",
    "_ZN3sce3Xml3Dom6NodeIdC1Ev",
    "_ZN3sce3Xml3Dom6NodeIdC2EPvS3_b",
    "_ZN3sce3Xml3Dom6NodeIdC2Ev",
    "_ZN3sce3Xml3Dom6NodeIdaSES2_",
    "_ZN3sce3Xml3Dom6NodeIdcvPvEv",
    "_ZN3sce3Xml3Dom6NodeIdeqES2_",
    "_ZN3sce3Xml3Dom6NodeIdeqEi",
    "_ZN3sce3Xml3Dom6NodeIdneES2_",
    "_ZN3sce3Xml3Dom6NodeIdneEi",
    "_ZN3sce3Xml3Dom6NodeIdntEv",
    "_ZN3sce3Xml3Dom8Document10importNodeENS1_6NodeIdES3_PKS2_S3_",
    "_ZN3sce3Xml3Dom8Document10initializeEPKNS0_11InitializerE",
    "_ZN3sce3Xml3Dom8Document10insertNodeENS1_6NodeIdES3_S3_",
    "_ZN3sce3Xml3Dom8Document11removeChildENS1_6NodeIdES3_",
    "_ZN3sce3Xml3Dom8Document11resetStatusEv",
    "_ZN3sce3Xml3Dom8Document11setWritableEv",
    "_ZN3sce3Xml3Dom8Document12importParentEPKS2_NS1_6NodeIdE",
    "_ZN3sce3Xml3Dom8Document12setAttrValueENS1_6NodeIdEPKNS0_6StringES6_",
    "_ZN3sce3Xml3Dom8Document12setAttributeENS1_6NodeIdEPKNS0_6StringES6_",
    "_ZN3sce3Xml3Dom8Document13createElementEPKNS0_6StringEPKNS0_13AttributeListES5_",
    "_ZN3sce3Xml3Dom8Document13recurseDeleteENS1_6NodeIdE",
    "_ZN3sce3Xml3Dom8Document14createTextNodeEPKNS0_6StringE",
    "_ZN3sce3Xml3Dom8Document15addElementChildENS1_6NodeIdEPKNS0_6StringEPKNS0_13AttributeListES6_",
    "_ZN3sce3Xml3Dom8Document15removeAttributeENS1_6NodeIdEPKNS0_6StringE",
    "_ZN3sce3Xml3Dom8Document16removeAttributesENS1_6NodeIdE",
    "_ZN3sce3Xml3Dom8Document16setAttributeListENS1_6NodeIdEPKNS0_13AttributeListE",
    "_ZN3sce3Xml3Dom8Document7setTextENS1_6NodeIdEPKNS0_6StringE",
    "_ZN3sce3Xml3Dom8Document9serializeEPKNS0_18SerializeParameterEPNS0_6StringE",
    "_ZN3sce3Xml3Dom8Document9terminateEv",
    "_ZN3sce3Xml3Dom8DocumentC1ERKS2_",
    "_ZN3sce3Xml3Dom8DocumentC1Ev",
    "_ZN3sce3Xml3Dom8DocumentC2ERKS2_",
    "_ZN3sce3Xml3Dom8DocumentC2Ev",
    "_ZN3sce3Xml3Dom8DocumentD1Ev",
    "_ZN3sce3Xml3Dom8DocumentD2Ev",
    "_ZN3sce3Xml3Dom8DocumentaSERKS2_",
    "_ZN3sce3Xml3Dom8NodeList10initializeEPKNS0_11InitializerE",
    "_ZN3sce3Xml3Dom8NodeList10insertLastENS1_6NodeIdE",
    "_ZN3sce3Xml3Dom8NodeList10removeItemENS1_6NodeIdE",
    "_ZN3sce3Xml3Dom8NodeList11insertFirstENS1_6NodeIdE",
    "_ZN3sce3Xml3Dom8NodeList5clearEv",
    "_ZN3sce3Xml3Dom8NodeList9terminateEv",
    "_ZN3sce3Xml3Dom8NodeListC1ERKS2_",
    "_ZN3sce3Xml3Dom8NodeListC1Ev",
    "_ZN3sce3Xml3Dom8NodeListC2ERKS2_",
    "_ZN3sce3Xml3Dom8NodeListC2Ev",
    "_ZN3sce3Xml3Dom8NodeListD1Ev",
    "_ZN3sce3Xml3Dom8NodeListD2Ev",
    "_ZN3sce3Xml3Sax6Parser10initializeEPKNS0_11InitializerE",
    "_ZN3sce3Xml3Sax6Parser11setUserDataEPv",
    "_ZN3sce3Xml3Sax6Parser16setResolveEntityEb",
    "_ZN3sce3Xml3Sax6Parser18setDocumentHandlerEPNS1_15DocumentHandlerE",
    "_ZN3sce3Xml3Sax6Parser26setSkipIgnorableWhiteSpaceEb",
    "_ZN3sce3Xml3Sax6Parser5parseEPKNS0_6StringEb",
    "_ZN3sce3Xml3Sax6Parser5resetEv",
    "_ZN3sce3Xml3Sax6Parser9terminateEv",
    "_ZN3sce3Xml3Sax6ParserC1Ev",
    "_ZN3sce3Xml3Sax6ParserC2Ev",
    "_ZN3sce3Xml3Sax6ParserD1Ev",
    "_ZN3sce3Xml3Sax6ParserD2Ev",
    "_ZN3sce3Xml4Attr10initializeEPKNS0_11InitializerE",
    "_ZN3sce3Xml4Attr7setNameEPKNS0_6StringE",
    "_ZN3sce3Xml4Attr8setValueEPKNS0_6StringE",
    "_ZN3sce3Xml4Attr9terminateEv",
    "_ZN3sce3Xml4AttrC1ERKS1_",
    "_ZN3sce3Xml4AttrC1Ev",
    "_ZN3sce3Xml4AttrC2ERKS1_",
    "_ZN3sce3Xml4AttrC2Ev",
    "_ZN3sce3Xml4AttrD1Ev",
    "_ZN3sce3Xml4AttrD2Ev",
    "_ZN3sce3Xml4AttraSERKS1_",
    "_ZN3sce3Xml4Util9strResultEi",
    "_ZN3sce3Xml6StringC1EPKc",
    "_ZN3sce3Xml6StringC1EPKcm",
    "_ZN3sce3Xml6StringC1ERKS1_",
    "_ZN3sce3Xml6StringC1Ev",
    "_ZN3sce3Xml6StringC2EPKc",
    "_ZN3sce3Xml6StringC2EPKcm",
    "_ZN3sce3Xml6StringC2ERKS1_",
    "_ZN3sce3Xml6StringC2Ev",
    "_ZN3sce3Xml6StringaSERKS1_",
    "_ZNK3sce3Xml13AttributeList11isAvailableEv",
    "_ZNK3sce3Xml13AttributeList12getAttributeEPKNS0_6StringE",
    "_ZNK3sce3Xml13AttributeList12getAttributeEj",
    "_ZNK3sce3Xml13AttributeList9getLengthEv",
    "_ZNK3sce3Xml3Dom13DocumentDebug13getStructSizeEv",
    "_ZNK3sce3Xml3Dom13DocumentDebug16getAttrTableSizeEv",
    "_ZNK3sce3Xml3Dom13DocumentDebug16getCharTableSizeEv",
    "_ZNK3sce3Xml3Dom13DocumentDebug19getElementTableSizeEv",
    "_ZNK3sce3Xml3Dom4Node11getNodeNameEv",
    "_ZNK3sce3Xml3Dom4Node11getNodeTypeEv",
    "_ZNK3sce3Xml3Dom4Node11isAvailableEv",
    "_ZNK3sce3Xml3Dom4Node12getLastChildEv",
    "_ZNK3sce3Xml3Dom4Node12getNodeValueEv",
    "_ZNK3sce3Xml3Dom4Node13getAttributesEPNS1_8NodeListE",
    "_ZNK3sce3Xml3Dom4Node13getAttributesEv",
    "_ZNK3sce3Xml3Dom4Node13getChildNodesEPNS1_8NodeListE",
    "_ZNK3sce3Xml3Dom4Node13getChildNodesEv",
    "_ZNK3sce3Xml3Dom4Node13getFirstChildEv",
    "_ZNK3sce3Xml3Dom4Node13getParentNodeEv",
    "_ZNK3sce3Xml3Dom4Node13hasAttributesEv",
    "_ZNK3sce3Xml3Dom4Node13hasChildNodesEv",
    "_ZNK3sce3Xml3Dom4Node14getNextSiblingEv",
    "_ZNK3sce3Xml3Dom4Node16getOwnerDocumentEv",
    "_ZNK3sce3Xml3Dom8Document10getDocRootEv",
    "_ZNK3sce3Xml3Dom8Document10getSiblingENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document10getXmlMetaEv",
    "_ZNK3sce3Xml3Dom8Document10isReadOnlyEv",
    "_ZNK3sce3Xml3Dom8Document11getAttrNameENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document11getNextAttrENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document11getNodeNameENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document11getNodeTypeENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document11isAvailableEv",
    "_ZNK3sce3Xml3Dom8Document12getAttrValueENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document12getAttributeENS1_6NodeIdEPKNS0_6StringE",
    "_ZNK3sce3Xml3Dom8Document12getFirstAttrENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document12getLastChildENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document13getAttributesENS1_6NodeIdEPNS1_8NodeListE",
    "_ZNK3sce3Xml3Dom8Document13getChildNodesENS1_6NodeIdEPNS1_8NodeListE",
    "_ZNK3sce3Xml3Dom8Document13getEntityTypeENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document13getFirstChildENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document13hasAttributesENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document13hasChildNodesENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document14getSkippedTextENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document20getElementsByTagNameENS1_6NodeIdEPKNS0_6StringEPNS1_8NodeListE",
    "_ZNK3sce3Xml3Dom8Document7getRootEv",
    "_ZNK3sce3Xml3Dom8Document7getTextENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document9getEntityENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document9getParentENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8Document9getStatusEv",
    "_ZNK3sce3Xml3Dom8NodeList11isAvailableEv",
    "_ZNK3sce3Xml3Dom8NodeList4itemEj",
    "_ZNK3sce3Xml3Dom8NodeList8findItemENS1_6NodeIdE",
    "_ZNK3sce3Xml3Dom8NodeList8findItemEPKNS0_6StringE",
    "_ZNK3sce3Xml3Dom8NodeList9getLengthEv",
    "_ZNK3sce3Xml3Dom8NodeListixEj",
    "_ZNK3sce3Xml4Attr11isAvailableEv",
    "_ZNK3sce3Xml4Attr7getNameEv",
    "_ZNK3sce3Xml4Attr8getValueEv",

)
PENDING = re.compile(
    r"^int APS5_VABI (?P<name>\w+)\(void\) \{\n"
    r"    NotImplemented_nid_no_patch\(__func__\);\n"
    r"    return 0;\n"
    r"\}$", re.MULTILINE)
EXPORT = re.compile(r"\bAPS5_VABI\s+(\w+)\s*\([^;{]*\)\s*\{")


def tools_directory(library):
    for parent in [library, *library.parents]:
        if (parent / "tools" / "import_audit.py").is_file():
            return parent / "tools"
    raise SystemExit("the tools directory is not reachable from " + str(library))


if len(sys.argv) != 3:
    raise SystemExit("usage: test_export_nids.py <library source directory> <patched library>")
LIBRARY = Path(sys.argv[1]).resolve()
PATCHED = Path(sys.argv[2]).resolve()
sys.path.insert(0, str(tools_directory(LIBRARY)))
import import_audit
import nid_names


class PendingExportNidTests(unittest.TestCase):
    def source(self):
        return (LIBRARY / "PendingExports.cpp").read_text(encoding="utf-8", errors="replace")

    def declared(self):
        return {match.group("name") for match in PENDING.finditer(self.source())}

    def exports(self):
        if not PATCHED.is_file():
            self.skipTest(str(PATCHED) + " is not built")
        return import_audit.read_exports(PATCHED)

    def test_every_source_export_is_declared_as_a_pending_export(self):
        declared = self.declared()
        for name in SOURCE_EXPORTS:
            with self.subTest(export=name):
                self.assertIn(name, declared)

    def test_every_declared_export_resolves_by_its_nid(self):
        exports = self.exports()
        for name in EXPORT.findall(self.source()):
            with self.subTest(export=name):
                self.assertIn(nid_names.compute_nid(name), exports)

    def test_a_source_export_resolves_by_its_nid(self):
        exports = self.exports()
        for name in SOURCE_EXPORTS:
            with self.subTest(export=name):
                self.assertIn(nid_names.compute_nid(name), exports)

    def test_a_pending_export_does_not_keep_its_plain_name(self):
        exports = self.exports()
        for name in self.declared():
            with self.subTest(export=name):
                self.assertNotIn(name, exports)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
