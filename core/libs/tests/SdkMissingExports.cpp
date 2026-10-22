#include <nid/NidCompute.hpp>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <cstdlib>
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
#endif

namespace {

void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

using Export = void (*)();

#ifdef _WIN32
using Module = HMODULE;
Module Load(const std::filesystem::path& path) { return LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH); }
Export Resolve(Module module, const std::string& name) { return reinterpret_cast<Export>(GetProcAddress(module, name.c_str())); }
#else
using Module = void*;
Module Load(const std::filesystem::path& path) { return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL); }
Export Resolve(Module module, const std::string& name) { return reinterpret_cast<Export>(dlsym(module, name.c_str())); }
#endif

void CheckLibrary(const std::filesystem::path& directory, const std::filesystem::path& self, const std::string& library, const std::vector<const char*>& names) {
    const auto module = Load(directory / (library + ".prx"));
    Require(module != nullptr, library + " does not load");
    std::size_t refused = 0;
    for (std::size_t index = 0; index < names.size(); ++index) {
        const auto* name = names[index];
        const auto nid = Nid::ComputeNid(name, "");
        const auto exportFunction = Resolve(module, nid);
        Require(exportFunction != nullptr, library + " does not export the NID of " + name);
        const auto report = std::filesystem::temp_directory_path() / "sdk-export-report.txt";
        std::filesystem::remove(report);
        auto command = "\"" + self.string() + "\" \"" + directory.string() + "\" " + library + " " + std::to_string(index) + " \"" + report.string() + "\"";
#ifdef _WIN32
        command = "\"" + command + "\"";
#endif
        const int status = std::system(command.c_str());
        std::string text;
        { std::ifstream file(report); std::getline(file, text); }
        std::filesystem::remove(report);
        Require(status != 0 && text.find("loaded") != std::string::npos && text.find("returned") == std::string::npos,
            library + " exports " + name + " as something other than a pending export");
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
    if (argc == 5) {
        const std::filesystem::path directory(argv[1]);
        const std::string library(argv[2]);
        const auto module = Load(directory / (library + ".prx"));
        if (module == nullptr) return 2;
        const auto& names = library == "libSceGLSlimVSH" ? libSceGLSlimVSHNames : libSceIpmiNames;
        const auto index = std::stoi(argv[3]);
        const auto nid = Nid::ComputeNid(names[index], "");
        const auto exportFunction = Resolve(module, nid);
        if (exportFunction == nullptr) return 2;
        { std::ofstream file(argv[4]); file << "loaded"; }
        exportFunction();
        { std::ofstream file(argv[4], std::ios::app); file << " returned"; }
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
