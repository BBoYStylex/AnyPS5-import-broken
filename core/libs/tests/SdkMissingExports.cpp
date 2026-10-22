#include <nid/NidCompute.hpp>
#include <nid/NidPatcherUtils.hpp>
#include <nid/ElfPatcher.hpp>
#include <nid/PeNidPatcher.hpp>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

using Export = void (*)();

#ifdef _WIN32
using Module = HMODULE;
#else
using Module = void*;
#endif

std::vector<std::uint8_t> ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error(path.string() + " cannot be read");
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

#ifdef _WIN32
std::vector<std::string> ExportedNames(const std::filesystem::path& path) {
    using namespace Nid;
    const auto pe = ReadFile(path);
    const auto peOffset = static_cast<std::size_t>(Internal::Read<std::uint16_t>(pe, 0x3cu));
    Require(pe.size() > peOffset + 6u && pe[peOffset] == 'P' && pe[peOffset + 1u] == 'E', path.string() + " is not a PE file");
    const auto coffOffset = peOffset + 4u;
    const auto numberOfSections = Internal::Read<std::uint16_t>(pe, coffOffset + 2u);
    const auto sizeOfOptionalHeader = Internal::Read<std::uint16_t>(pe, coffOffset + 16u);
    const auto optionalHeaderOffset = coffOffset + 20u;
    const auto magic = Internal::Read<std::uint16_t>(pe, optionalHeaderOffset);
    Require(magic == 0x20bu || magic == 0x10bu, path.string() + " has an unsupported optional header");
    const auto dataDirectoryOffset = optionalHeaderOffset + (magic == 0x20bu ? 112u : 96u);
    const auto exportRva = Internal::Read<std::uint32_t>(pe, dataDirectoryOffset);
    if (exportRva == 0u) return {};
    const auto sectionOffset = optionalHeaderOffset + sizeOfOptionalHeader;
    const auto rvaToOffset = [&](std::uint32_t rva) {
        for (std::uint16_t i = 0; i < numberOfSections; ++i) {
            const auto header = Internal::Read<PeSectionHeader>(pe, sectionOffset + i * sizeof(PeSectionHeader));
            const auto size = std::max<std::uint32_t>(header.VirtualSize, header.SizeOfRawData);
            if (rva >= header.VirtualAddress && rva < header.VirtualAddress + size)
                return static_cast<std::size_t>(header.PointerToRawData + (rva - header.VirtualAddress));
        }
        throw std::runtime_error(path.string() + " has an export rva outside its sections");
    };
    const auto directory = Internal::Read<PeExportDirectory>(pe, rvaToOffset(exportRva));
    std::vector<std::string> names;
    names.reserve(directory.NumberOfNames);
    for (std::uint32_t i = 0; i < directory.NumberOfNames; ++i) {
        const auto nameRva = Internal::Read<std::uint32_t>(pe, rvaToOffset(directory.AddressOfNames) + i * 4u);
        names.push_back(Internal::ReadCStr(pe, rvaToOffset(nameRva)));
    }
    return names;
}
#else
std::vector<std::string> ExportedNames(const std::filesystem::path& path) {
    using namespace Nid;
    const auto elf = ReadFile(path);
    Require(elf.size() >= sizeof(Elf64_Ehdr) && elf[4] == 2, path.string() + " is not an ELF64 file");
    const auto header = Internal::Read<Elf64_Ehdr>(elf, 0u);
    std::size_t dynSymOffset = 0u, dynSymSize = 0u, dynStrOffset = 0u;
    for (std::uint16_t i = 0; i < header.e_shnum; ++i) {
        const auto offset = static_cast<std::size_t>(header.e_shoff) + i * sizeof(Elf64_Shdr);
        const auto section = Internal::Read<Elf64_Shdr>(elf, offset);
        if (section.sh_type != 11u) continue;
        dynSymOffset = static_cast<std::size_t>(section.sh_offset);
        dynSymSize = static_cast<std::size_t>(section.sh_size);
        const auto strings = Internal::Read<Elf64_Shdr>(elf, static_cast<std::size_t>(header.e_shoff) +
            static_cast<std::size_t>(section.sh_link) * sizeof(Elf64_Shdr));
        dynStrOffset = static_cast<std::size_t>(strings.sh_offset);
    }
    Require(dynSymOffset != 0u, path.string() + " has no .dynsym section");
    std::vector<std::string> names;
    for (std::size_t offset = 0; offset < dynSymSize; offset += sizeof(Elf64_Sym)) {
        const auto symbol = Internal::Read<Elf64_Sym>(elf, dynSymOffset + offset);
        if (symbol.st_name == 0u) continue;
        names.push_back(Internal::ReadCStr(elf, dynStrOffset + symbol.st_name));
    }
    return names;
}
#endif

#ifdef _WIN32
Module Load(const std::filesystem::path& path) { return LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH); }
Export Resolve(Module module, const std::string& name) { return reinterpret_cast<Export>(GetProcAddress(module, name.c_str())); }
int RunChild(const std::filesystem::path& self, const std::filesystem::path& directory, const std::string& library, std::size_t index) {
    std::string command = "\"" + self.string() + "\" \"" + directory.string() + "\" " + library + " " + std::to_string(index);
    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION process{};
    std::vector<char> buffer(command.begin(), command.end());
    buffer.push_back('\0');
    if (!CreateProcessA(nullptr, buffer.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process)) return -1;
    WaitForSingleObject(process.hProcess, 30000);
    DWORD status = 0;
    GetExitCodeProcess(process.hProcess, &status);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return static_cast<int>(status);
}
#else
Module Load(const std::filesystem::path& path) { return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL); }
Export Resolve(Module module, const std::string& name) { return reinterpret_cast<Export>(dlsym(module, name.c_str())); }
int RunChild(const std::filesystem::path& self, const std::filesystem::path& directory, const std::string& library, std::size_t index) {
    const auto child = fork();
    if (child == 0) {
        const auto indexText = std::to_string(index);
        const auto libraryText = library.c_str();
        const auto directoryText = directory.c_str();
        execl(self.c_str(), self.c_str(), directoryText, libraryText, indexText, static_cast<char*>(nullptr));
        _exit(127);
    }
    int status = 0;
    waitpid(child, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}
#endif

void CheckLibrary(const std::filesystem::path& directory, const std::filesystem::path& self, const std::string& library, const std::vector<const char*>& names) {
    const auto exported = ExportedNames(directory / (library + ".prx"));
    std::size_t refused = 0;
    for (std::size_t index = 0; index < names.size(); ++index) {
        const auto* name = names[index];
        const auto nid = Nid::ComputeNid(name, "");
        Require(std::find(exported.begin(), exported.end(), nid) != exported.end(), library + " does not export the NID of " + name);
        Require(std::find(exported.begin(), exported.end(), name) == exported.end(), library + " still exports " + name + " under its plain name");
        const int status = RunChild(self, directory, library, index);
        Require(status != 0, library + " exports " + name + " as something other than a pending export");
        ++refused;
    }
    Require(refused == names.size(), library + " did not refuse every export");
    std::printf("%s: %zu exports refuse\n", library.c_str(), refused);
}

const std::vector<const char*> libSceGLSlimVSHNames{

    "eglBindAPI",

    "eglBindTexImage",

    "eglChooseConfig",

    "eglCreateContext",

    "eglCreatePbufferFromClientBuffer",

    "eglCreatePbufferSurface",

    "eglCreateWindowSurface",

    "eglDestroyContext",

    "eglDestroySurface",

    "eglGetConfigAttrib",

    "eglGetConfigs",

    "eglGetCurrentContext",

    "eglGetCurrentDisplay",

    "eglGetCurrentSurface",

    "eglGetDisplay",

    "eglGetError",

    "eglGetProcAddress",

    "eglInitialize",

    "eglMakeCurrent",

    "eglQueryAPI",

    "eglQueryContext",

    "eglQueryString",

    "eglQuerySurface",

    "eglReleaseTexImage",

    "eglReleaseThread",

    "eglSurfaceAttrib",

    "eglSwapBuffers",

    "eglSwapInterval",

    "eglTerminate",

    "eglWaitClient",

    "eglWaitGL",

    "eglWaitNative",

    "glActiveTexture",

    "glAttachShader",

    "glBindAttribLocation",

    "glBindBuffer",

    "glBindFramebuffer",

    "glBindRenderbuffer",

    "glBindTexture",

    "glBlendColor",

    "glBlendEquation",

    "glBlendEquationSeparate",

    "glBlendFunc",

    "glBlendFuncSeparate",

    "glBufferData",

    "glBufferSubData",

    "glCheckFramebufferStatus",

    "glClear",

    "glClearColor",

    "glClearDepthf",

    "glClearStencil",

    "glColorMask",

    "glCompileShader",

    "glCompressedTexImage2D",

    "glCompressedTexSubImage2D",

    "glCopyTexImage2D",

    "glCopyTexSubImage2D",

    "glCreateProgram",

    "glCreateShader",

    "glCullFace",

    "glDeleteBuffers",

    "glDeleteFramebuffers",

    "glDeleteProgram",

    "glDeleteRenderbuffers",

    "glDeleteShader",

    "glDeleteTextures",

    "glDepthFunc",

    "glDepthMask",

    "glDepthRangef",

    "glDetachShader",

    "glDisable",

    "glDisableVertexAttribArray",

    "glDrawArrays",

    "glDrawElements",

    "glEnable",

    "glEnableVertexAttribArray",

    "glFinish",

    "glFlush",

    "glFramebufferRenderbuffer",

    "glFramebufferTexture2D",

    "glFrontFace",

    "glGenBuffers",

    "glGenFramebuffers",

    "glGenRenderbuffers",

    "glGenTextures",

    "glGenerateMipmap",

    "glGetActiveAttrib",

    "glGetActiveUniform",

    "glGetAttachedShaders",

    "glGetAttribLocation",

    "glGetBooleanv",

    "glGetBufferParameteriv",

    "glGetError",

    "glGetFloatv",

    "glGetFramebufferAttachmentParameteriv",

    "glGetIntegerv",

    "glGetProgramInfoLog",

    "glGetProgramiv",

    "glGetRenderbufferParameteriv",

    "glGetShaderInfoLog",

    "glGetShaderPrecisionFormat",

    "glGetShaderiv",

    "glGetString",

    "glGetTexParameterfv",

    "glGetTexParameteriv",

    "glGetUniformLocation",

    "glGetUniformfv",

    "glGetUniformiv",

    "glGetVertexAttribPointerv",

    "glGetVertexAttribfv",

    "glGetVertexAttribiv",

    "glHint",

    "glIsBuffer",

    "glIsEnabled",

    "glIsFramebuffer",

    "glIsProgram",

    "glIsRenderbuffer",

    "glIsShader",

    "glIsTexture",

    "glLineWidth",

    "glLinkProgram",

    "glOrbisMapTextureResourceSCE",

    "glOrbisTexImageCanvas2DSCE",

    "glOrbisTexImageResourceSCE",

    "glOrbisUnmapTextureResourceSCE",

    "glPixelStorei",

    "glPolygonOffset",

    "glReadPixels",

    "glReleaseShaderCompiler",

    "glRenderbufferStorage",

    "glSampleCoverage",

    "glScissor",

    "glShaderBinary",

    "glShaderSource",

    "glStencilFunc",

    "glStencilFuncSeparate",

    "glStencilMask",

    "glStencilMaskSeparate",

    "glStencilOp",

    "glStencilOpSeparate",

    "glTexImage2D",

    "glTexParameterf",

    "glTexParameterfv",

    "glTexParameteri",

    "glTexParameteriv",

    "glTexSubImage2D",

    "glUniform1f",

    "glUniform1fv",

    "glUniform1i",

    "glUniform1iv",

    "glUniform2f",

    "glUniform2fv",

    "glUniform2i",

    "glUniform2iv",

    "glUniform3f",

    "glUniform3fv",

    "glUniform3i",

    "glUniform3iv",

    "glUniform4f",

    "glUniform4fv",

    "glUniform4i",

    "glUniform4iv",

    "glUniformMatrix2fv",

    "glUniformMatrix3fv",

    "glUniformMatrix4fv",

    "glUseProgram",

    "glValidateProgram",

    "glVertexAttrib1f",

    "glVertexAttrib1fv",

    "glVertexAttrib2f",

    "glVertexAttrib2fv",

    "glVertexAttrib3f",

    "glVertexAttrib3fv",

    "glVertexAttrib4f",

    "glVertexAttrib4fv",

    "glVertexAttribPointer",

    "glViewport",

    "scePigletAllocateSystemMemoryEx",

    "scePigletAllocateVideoMemoryEx",

    "scePigletGetConfigurationVSH",

    "scePigletReleaseSystemMemoryEx",

    "scePigletReleaseVideoMemoryEx",

    "scePigletSetConfigurationVSH",

    "sceSlimglClientStartIPC",

    "sceSlimglCompositorAllocateIndex",

    "sceSlimglCompositorCreateIndirectRenderTarget",

    "sceSlimglCompositorDeleteIndirectRenderTarget",

    "sceSlimglCompositorFlush",

    "sceSlimglCompositorFlushWithRepeat",

    "sceSlimglCompositorGetCanvasHandle",

    "sceSlimglCompositorGetError",

    "sceSlimglCompositorReleaseIndex",

    "sceSlimglCompositorSetEndOfFrameCommand",

    "sceSlimglCompositorSetIndirectCanvasCommand",

    "sceSlimglCompositorSetIndirectCompositionCommand",

    "sceSlimglCompositorSetIndirectRenderTargetConfigCommand",

    "sceSlimglCompositorSetInvisibleCanvasCommand",

    "sceSlimglCompositorSetMemoryCommand",

    "sceSlimglCompositorSetPostEventCommand",

    "sceSlimglCompositorWaitEndOfRendering",

    "sceSlimglCompositorWaitPostEvent",

    "sceSlimglIPCEnd",

    "sceSlimglIPCSetup",

};


const std::vector<const char*> libSceIpmiNames{

    "_ZN4IPMI4impl10ClientImpl10disconnectEv",

    "_ZN4IPMI4impl10ClientImpl10initializeEPKNS_6Client6ConfigEPv",

    "_ZN4IPMI4impl10ClientImpl11getUserDataEv",

    "_ZN4IPMI4impl10ClientImpl12tryGetResultEjPiPvPmm",

    "_ZN4IPMI4impl10ClientImpl12tryGetResultEjjPiPNS_10BufferInfoEj",

    "_ZN4IPMI4impl10ClientImpl13pollEventFlagEjmjPm",

    "_ZN4IPMI4impl10ClientImpl13waitEventFlagEjmjPmPj",

    "_ZN4IPMI4impl10ClientImpl16invokeSyncMethodEjPKNS_8DataInfoEjPiPNS_10BufferInfoEj",

    "_ZN4IPMI4impl10ClientImpl16invokeSyncMethodEjPKvmPiPvPmm",

    "_ZN4IPMI4impl10ClientImpl17invokeAsyncMethodEjPKNS_8DataInfoEjPjPKNS_6Client12EventNotifeeE",

    "_ZN4IPMI4impl10ClientImpl17invokeAsyncMethodEjPKvmPjPKNS_6Client12EventNotifeeE",

    "_ZN4IPMI4impl10ClientImpl19terminateConnectionEv",

    "_ZN4IPMI4impl10ClientImpl6getMsgEjPvPmmPj",

    "_ZN4IPMI4impl10ClientImpl7connectEPKvmPi",

    "_ZN4IPMI4impl10ClientImpl7destroyEv",

    "_ZN4IPMI4impl10ClientImpl9tryGetMsgEjPvPmm",

    "_ZN4IPMI4impl10ClientImplC1Ev",

    "_ZN4IPMI4impl10ClientImplC2Ev",

    "_ZN4IPMI4impl10ClientImplD0Ev",

    "_ZN4IPMI4impl10ClientImplD1Ev",

    "_ZN4IPMI4impl10ClientImplD2Ev",

    "_ZN4IPMI4impl10ServerImpl11getUserDataEv",

    "_ZN4IPMI4impl10ServerImpl11tryDispatchEPvm",

    "_ZN4IPMI4impl10ServerImpl13createSessionEPPNS_7SessionEPvS5_",

    "_ZN4IPMI4impl10ServerImpl13runDispatcherEPvm",

    "_ZN4IPMI4impl10ServerImpl18shutdownDispatcherEv",

    "_ZN4IPMI4impl10ServerImpl7destroyEv",

    "_ZN4IPMI4impl10ServerImplC1Ev",

    "_ZN4IPMI4impl10ServerImplC2Ev",

    "_ZN4IPMI4impl10ServerImplD0Ev",

    "_ZN4IPMI4impl10ServerImplD1Ev",

    "_ZN4IPMI4impl10ServerImplD2Ev",

    "_ZN4IPMI4impl11SessionImpl10trySendMsgEjPKvm",

    "_ZN4IPMI4impl11SessionImpl11getUserDataEv",

    "_ZN4IPMI4impl11SessionImpl11tryDispatchEPvm",

    "_ZN4IPMI4impl11SessionImpl12getClientPidEv",

    "_ZN4IPMI4impl11SessionImpl12setEventFlagEjm",

    "_ZN4IPMI4impl11SessionImpl13emptyMsgQueueEj",

    "_ZN4IPMI4impl11SessionImpl26respondToSyncMethodRequestEiPKNS_10BufferInfoEj",

    "_ZN4IPMI4impl11SessionImpl26respondToSyncMethodRequestEiPKvm",

    "_ZN4IPMI4impl11SessionImpl27respondToAsyncMethodRequestEjiPKvm",

    "_ZN4IPMI4impl11SessionImpl27respondToAsyncMethodRequestEjjiPKNS_8DataInfoEj",

    "_ZN4IPMI4impl11SessionImpl6getMsgEjPvPmmPj",

    "_ZN4IPMI4impl11SessionImpl7destroyEv",

    "_ZN4IPMI4impl11SessionImpl7sendMsgEjPKvmPj",

    "_ZN4IPMI4impl11SessionImpl9getServerEv",

    "_ZN4IPMI4impl11SessionImpl9tryGetMsgEjPvPmm",

    "_ZN4IPMI4impl11SessionImplC1Ev",

    "_ZN4IPMI4impl11SessionImplC2Ev",

    "_ZN4IPMI4impl11SessionImplD0Ev",

    "_ZN4IPMI4impl11SessionImplD1Ev",

    "_ZN4IPMI4impl11SessionImplD2Ev",

    "_ZN4IPMI6Client6Config14MsgQueueConfig20estimateMsgQueueSizeEmj",

    "_ZN4IPMI6Client6Config14MsgQueueConfigC1Ev",

    "_ZN4IPMI6Client6Config14MsgQueueConfigC2Ev",

    "_ZN4IPMI6Client6Config24estimateClientMemorySizeEv",

    "_ZN4IPMI6Client6Config26SyncMethodInvocationConfigC1Ev",

    "_ZN4IPMI6Client6Config26SyncMethodInvocationConfigC2Ev",

    "_ZN4IPMI6Client6Config27AsyncMethodInvocationConfigC1Ev",

    "_ZN4IPMI6Client6Config27AsyncMethodInvocationConfigC2Ev",

    "_ZN4IPMI6Client6ConfigC1Ev",

    "_ZN4IPMI6Client6ConfigC2Ev",

    "_ZN4IPMI6Client6createEPPS0_PKNS0_6ConfigEPvS6_",

    "_ZN4IPMI6ClientC2Ev",

    "_ZN4IPMI6ClientD0Ev",

    "_ZN4IPMI6ClientD1Ev",

    "_ZN4IPMI6ClientD2Ev",

    "_ZN4IPMI6Server12EventHandler15onSessionKilledEPNS_7SessionE",

    "_ZN4IPMI6Server12EventHandler20onSyncMethodDispatchEPNS_7SessionEjPKNS_8DataInfoEjPNS_10BufferInfoEj",

    "_ZN4IPMI6Server12EventHandler20onSyncMethodDispatchEPNS_7SessionEjPvmmS4_m",

    "_ZN4IPMI6Server12EventHandler21onAsyncMethodDispatchEPNS_7SessionEjjPKNS_8DataInfoEj",

    "_ZN4IPMI6Server12EventHandler21onAsyncMethodDispatchEPNS_7SessionEjjPvmS4_m",

    "_ZN4IPMI6Server12EventHandlerD0Ev",

    "_ZN4IPMI6Server12EventHandlerD1Ev",

    "_ZN4IPMI6Server12EventHandlerD2Ev",

    "_ZN4IPMI6Server6ConfigC1Ev",

    "_ZN4IPMI6Server6ConfigC2Ev",

    "_ZN4IPMI6Server6createEPPS0_PKNS0_6ConfigEPvS6_",

    "_ZN4IPMI6ServerC2Ev",

    "_ZN4IPMI6ServerD0Ev",

    "_ZN4IPMI6ServerD1Ev",

    "_ZN4IPMI6ServerD2Ev",

    "_ZN4IPMI7Session6Config25estimateSessionMemorySizeEv",

    "_ZN4IPMI7SessionC2Ev",

    "_ZN4IPMI7SessionD0Ev",

    "_ZN4IPMI7SessionD1Ev",

    "_ZN4IPMI7SessionD2Ev",

    "_ZNK4IPMI4impl11SessionImpl16isPeerPrivilegedEv",

    "_ZNK4IPMI6Server6Config29estimateTempWorkingMemorySizeEv",

    "sceIpmiMgrGetIpcBlockerPid",

    "sceIpmiMgrGetIpcBlockerPidByAppId",

    "sceIpmiMgrPauseDump",

    "sceIpmiMgrRaiseExceptionToPid",

    "sceIpmiMgrStartDump",

};


} // namespace

int main(int argc, char** argv) {
    if (argc == 4) {
        const std::filesystem::path directory(argv[1]);
        const std::string library(argv[2]);
        const auto module = Load(directory / (library + ".prx"));
        if (module == nullptr) return 2;
        const auto& names = library == "libSceGLSlimVSH" ? libSceGLSlimVSHNames : libSceIpmiNames;
        const auto index = std::stoi(argv[3]);
        const auto nid = Nid::ComputeNid(names[index], "");
        const auto exportFunction = Resolve(module, nid);
        if (exportFunction == nullptr) return 2;
        using GuestSetTerminate = void (*)(void (*)());
        const auto guestSetTerminate = reinterpret_cast<GuestSetTerminate>(
            Resolve(module, Nid::ComputeNid("_ZSt13set_terminatePFvvE", "")));
        const auto refuse = []() { std::_Exit(3); };
        if (guestSetTerminate != nullptr) guestSetTerminate(refuse);
        std::set_terminate(refuse);
        try {
            exportFunction();
        } catch (...) {
            std::_Exit(3);
        }
        return 0;
    }
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <patched libraries directory>\n", argv[0]);
        return 2;
    }
    try {
        const std::filesystem::path directory(argv[1]);
        const std::filesystem::path self = std::filesystem::absolute(argv[0]);
        CheckLibrary(directory, self, "libSceGLSlimVSH", libSceGLSlimVSHNames);
        CheckLibrary(directory, self, "libSceIpmi", libSceIpmiNames);
        std::cout << "SDK export coverage passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
