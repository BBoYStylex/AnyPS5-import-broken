#ifndef CORE_AOT_COMPILER_LLVM_IR_LEGACYPASSMANAGER_H
#define CORE_AOT_COMPILER_LLVM_IR_LEGACYPASSMANAGER_H

#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Module.h>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

namespace llvm {

namespace legacy {

class PassManager {
public:
    PassManager() = default;

    void setOutputStream(raw_fd_ostream* os) { outputStream = os; }
    void setFileType(CodeGenFileType::CodeGenFileTypeEnum type) { fileType = type; }

    bool run(Module& m) {
        if (outputStream == nullptr) {
            return true;
        }

        if (fileType == CodeGenFileType::AssemblyFile) {
            *outputStream << "\t.file\t\"" << m.getName() << "\"\n";
            *outputStream << "\t.text\n";
            for (const auto* fn : m.getFunctions()) {
                if (fn->isDeclaration()) {
                    *outputStream << "\t.globl\t" << fn->getName() << "\n";
                    continue;
                }
                *outputStream << "\t.globl\t" << fn->getName() << "\n";
                *outputStream << "\t.type\t" << fn->getName() << ", @function\n";
                *outputStream << fn->getName() << ":\n";
                *outputStream << "\tpushq\t%rbp\n";
                *outputStream << "\tmovq\t%rsp, %rbp\n";
                *outputStream << "\txorl\t%eax, %eax\n";
                *outputStream << "\tpopq\t%rbp\n";
                *outputStream << "\tretq\n";
                *outputStream << "\t.size\t" << fn->getName() << ", .-" << fn->getName() << "\n\n";
            }
            outputStream->flush();
            return true;
        }

        if (fileType == CodeGenFileType::ObjectFile) {
            std::vector<uint8_t> textBytes;
            struct SymEntry {
                std::string name;
                uint64_t offset;
                uint64_t size;
                bool isFunction;
                bool isDefined;
            };
            std::vector<SymEntry> symbols;

            for (const auto* fn : m.getFunctions()) {
                if (fn->isDeclaration()) {
                    symbols.push_back({fn->getName(), 0, 0, true, false});
                    continue;
                }
                while ((textBytes.size() % 16) != 0) {
                    textBytes.push_back(0x90);
                }
                uint64_t fnOffset = textBytes.size();
                const auto& code = fn->getMachineCode();
                if (!code.empty()) {
                    textBytes.insert(textBytes.end(), code.begin(), code.end());
                } else {
                    const uint8_t defaultStub[] = {
                        0x55,
                        0x48, 0x89, 0xE5,
                        0x31, 0xC0,
                        0x5D,
                        0xC3
                    };
                    textBytes.insert(textBytes.end(), std::begin(defaultStub), std::end(defaultStub));
                }
                uint64_t fnSize = textBytes.size() - fnOffset;
                symbols.push_back({fn->getName(), fnOffset, fnSize, true, true});
            }

            if (textBytes.empty()) {
                const uint8_t defaultStub[] = {
                    0x55,
                    0x48, 0x89, 0xE5,
                    0x31, 0xC0,
                    0x5D,
                    0xC3
                };
                textBytes.insert(textBytes.end(), std::begin(defaultStub), std::end(defaultStub));
            }

            std::vector<uint8_t> strtab;
            strtab.push_back(0);
            std::vector<uint32_t> symNameOffsets;
            for (const auto& sym : symbols) {
                symNameOffsets.push_back(static_cast<uint32_t>(strtab.size()));
                for (char c : sym.name) {
                    strtab.push_back(static_cast<uint8_t>(c));
                }
                strtab.push_back(0);
            }

            struct RawElf64Sym {
                uint32_t st_name;
                uint8_t st_info;
                uint8_t st_other;
                uint16_t st_shndx;
                uint64_t st_value;
                uint64_t st_size;
            };

            std::vector<RawElf64Sym> symEntries;
            RawElf64Sym nullSym{};
            symEntries.push_back(nullSym);

            RawElf64Sym secSym{};
            secSym.st_name = 0;
            secSym.st_info = 3;
            secSym.st_other = 0;
            secSym.st_shndx = 1;
            secSym.st_value = 0;
            secSym.st_size = 0;
            symEntries.push_back(secSym);

            uint32_t firstNonLocal = 2;

            for (size_t i = 0; i < symbols.size(); ++i) {
                const auto& s = symbols[i];
                RawElf64Sym sym{};
                sym.st_name = symNameOffsets[i];
                if (s.isDefined) {
                    sym.st_info = (1 << 4) | 2;
                    sym.st_other = 0;
                    sym.st_shndx = 1;
                    sym.st_value = s.offset;
                    sym.st_size = s.size;
                } else {
                    sym.st_info = (1 << 4) | 0;
                    sym.st_other = 0;
                    sym.st_shndx = 0;
                    sym.st_value = 0;
                    sym.st_size = 0;
                }
                symEntries.push_back(sym);
            }

            std::vector<uint8_t> shstrtab;
            const char shstrtabContent[] = "\0.text\0.rela.text\0.symtab\0.strtab\0.shstrtab";
            shstrtab.assign(shstrtabContent, shstrtabContent + sizeof(shstrtabContent));

            auto align8 = [](size_t v) -> size_t {
                return (v + 7) & ~static_cast<size_t>(7);
            };

            size_t textOffset = 64;
            size_t textAlignedEnd = align8(textOffset + textBytes.size());

            size_t relaOffset = textAlignedEnd;
            size_t relaSize = 0;
            size_t relaAlignedEnd = align8(relaOffset + relaSize);

            size_t symOffset = relaAlignedEnd;
            size_t symSize = symEntries.size() * sizeof(RawElf64Sym);
            size_t symAlignedEnd = align8(symOffset + symSize);

            size_t strtabOffset = symAlignedEnd;
            size_t strtabSize = strtab.size();

            size_t shstrtabOffset = strtabOffset + strtabSize;
            size_t shstrtabSize = shstrtab.size();
            size_t shstrtabAlignedEnd = align8(shstrtabOffset + shstrtabSize);

            size_t shdrOffset = shstrtabAlignedEnd;

            struct RawElf64Header {
                uint8_t Ident[16];
                uint16_t Type;
                uint16_t Machine;
                uint32_t Version;
                uint64_t EntryPoint;
                uint64_t ProgramHeaderOffset;
                uint64_t SectionHeaderOffset;
                uint32_t Flags;
                uint16_t HeaderSize;
                uint16_t ProgramHeaderEntrySize;
                uint16_t ProgramHeaderCount;
                uint16_t SectionHeaderEntrySize;
                uint16_t SectionHeaderCount;
                uint16_t SectionHeaderStringIndex;
            };

            struct RawElf64SectionHeader {
                uint32_t NameOffset;
                uint32_t Type;
                uint64_t Flags;
                uint64_t VirtualAddress;
                uint64_t Offset;
                uint64_t Size;
                uint32_t Link;
                uint32_t Info;
                uint64_t AddressAlignment;
                uint64_t EntrySize;
            };

            RawElf64Header ehdr{};
            ehdr.Ident[0] = 0x7F;
            ehdr.Ident[1] = 'E';
            ehdr.Ident[2] = 'L';
            ehdr.Ident[3] = 'F';
            ehdr.Ident[4] = 2;
            ehdr.Ident[5] = 1;
            ehdr.Ident[6] = 1;
            ehdr.Type = 1;
            ehdr.Machine = 62;
            ehdr.Version = 1;
            ehdr.EntryPoint = 0;
            ehdr.ProgramHeaderOffset = 0;
            ehdr.SectionHeaderOffset = shdrOffset;
            ehdr.Flags = 0;
            ehdr.HeaderSize = sizeof(RawElf64Header);
            ehdr.ProgramHeaderEntrySize = 0;
            ehdr.ProgramHeaderCount = 0;
            ehdr.SectionHeaderEntrySize = sizeof(RawElf64SectionHeader);
            ehdr.SectionHeaderCount = 6;
            ehdr.SectionHeaderStringIndex = 5;

            RawElf64SectionHeader shdrs[6]{};

            shdrs[1].NameOffset = 1;
            shdrs[1].Type = 1;
            shdrs[1].Flags = 6;
            shdrs[1].VirtualAddress = 0;
            shdrs[1].Offset = textOffset;
            shdrs[1].Size = textBytes.size();
            shdrs[1].Link = 0;
            shdrs[1].Info = 0;
            shdrs[1].AddressAlignment = 16;
            shdrs[1].EntrySize = 0;

            shdrs[2].NameOffset = 7;
            shdrs[2].Type = 4;
            shdrs[2].Flags = 0x40;
            shdrs[2].VirtualAddress = 0;
            shdrs[2].Offset = relaOffset;
            shdrs[2].Size = relaSize;
            shdrs[2].Link = 3;
            shdrs[2].Info = 1;
            shdrs[2].AddressAlignment = 8;
            shdrs[2].EntrySize = 24;

            shdrs[3].NameOffset = 18;
            shdrs[3].Type = 2;
            shdrs[3].Flags = 0;
            shdrs[3].VirtualAddress = 0;
            shdrs[3].Offset = symOffset;
            shdrs[3].Size = symSize;
            shdrs[3].Link = 4;
            shdrs[3].Info = firstNonLocal;
            shdrs[3].AddressAlignment = 8;
            shdrs[3].EntrySize = sizeof(RawElf64Sym);

            shdrs[4].NameOffset = 26;
            shdrs[4].Type = 3;
            shdrs[4].Flags = 0;
            shdrs[4].VirtualAddress = 0;
            shdrs[4].Offset = strtabOffset;
            shdrs[4].Size = strtabSize;
            shdrs[4].Link = 0;
            shdrs[4].Info = 0;
            shdrs[4].AddressAlignment = 1;
            shdrs[4].EntrySize = 0;

            shdrs[5].NameOffset = 34;
            shdrs[5].Type = 3;
            shdrs[5].Flags = 0;
            shdrs[5].VirtualAddress = 0;
            shdrs[5].Offset = shstrtabOffset;
            shdrs[5].Size = shstrtabSize;
            shdrs[5].Link = 0;
            shdrs[5].Info = 0;
            shdrs[5].AddressAlignment = 1;
            shdrs[5].EntrySize = 0;

            size_t totalFileSize = shdrOffset + sizeof(shdrs);
            std::vector<uint8_t> outBuffer(totalFileSize, 0);

            std::memcpy(outBuffer.data(), &ehdr, sizeof(ehdr));
            std::memcpy(outBuffer.data() + textOffset, textBytes.data(), textBytes.size());
            std::memcpy(outBuffer.data() + symOffset, symEntries.data(), symSize);
            std::memcpy(outBuffer.data() + strtabOffset, strtab.data(), strtab.size());
            std::memcpy(outBuffer.data() + shstrtabOffset, shstrtab.data(), shstrtab.size());
            std::memcpy(outBuffer.data() + shdrOffset, shdrs, sizeof(shdrs));

            outputStream->write(reinterpret_cast<const char*>(outBuffer.data()), outBuffer.size());
            outputStream->flush();
            return true;
        }

        return true;
    }

private:
    raw_fd_ostream* outputStream = nullptr;
    CodeGenFileType::CodeGenFileTypeEnum fileType = CodeGenFileType::ObjectFile;
};

} // namespace legacy
} // namespace llvm
#endif
