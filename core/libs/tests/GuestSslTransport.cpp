#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include "prx/libc/include/general/VabiMacros.hpp"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecp.h>
#include <mbedtls/entropy.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/pk.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <thread>
#include <vector>

struct SslData {
    std::uint8_t* ptr;
    std::size_t size;
};

struct PollDescriptor {
    int descriptor;
    short events;
    short revents;
};

using SslVerifyCallback = int (APS5_VABI *)(int, unsigned int, void**, int, void*);

extern "C" {
int APS5_VABI sceNetSocket(const char*, int, int, int);
int APS5_VABI sceNetConnect(int, const void*, std::uint32_t);
int APS5_VABI sceNetSetsockopt(int, int, int, const void*, std::uint32_t);
int APS5_VABI sceNetSocketClose(int);
int APS5_VABI poll_nid_postfix(PollDescriptor*, std::uint32_t, int);
int APS5_VABI sceSslInit_nid_postfix(std::size_t);
int APS5_VABI sceSslTerm_nid_postfix(int);
int APS5_VABI sceSslLoadCert(int, int, SslData**, SslData*, SslData*);
int APS5_VABI sceSslCreateConnection(int, int, const char*);
int APS5_VABI sceSslDeleteConnection(int);
int APS5_VABI sceSslSetVerifyCallback(int, SslVerifyCallback, void*);
int APS5_VABI sceSslConnect(int);
int APS5_VABI sceSslRead(int, void*, std::size_t, int);
int APS5_VABI sceSslWrite(int, void*, std::size_t, int);
int APS5_VABI sceSslClose(int);
int APS5_VABI sceSslEnableVerifyOption(int, std::uint32_t);
int APS5_VABI sceSslDisableVerifyOption(int, std::uint32_t);
int APS5_VABI sceSslSetMinSslVersion(int, int);
int APS5_VABI sceSslSetAlpn(int, const char*);
const char* APS5_VABI sceSslGetAlpnSelected(int);
void* APS5_VABI sceSslGetSubjectName(int, void*);
int APS5_VABI sceSslGetNameEntryCount(int, void*);
int APS5_VABI sceSslGetNameEntryInfo(int, void*, int, char*, std::size_t, char*, std::size_t, std::size_t*);
int APS5_VABI sceSslFreeSslCertName(int, void*);
}

namespace {

#ifdef _WIN32
using HostSocket = SOCKET;
constexpr HostSocket InvalidHostSocket = INVALID_SOCKET;
void CloseHost(HostSocket socket) { ::closesocket(socket); }
#else
using HostSocket = int;
constexpr HostSocket InvalidHostSocket = -1;
void CloseHost(HostSocket socket) { ::close(socket); }
#endif

constexpr int ErrorUnknownCa = static_cast<int>(0x8095F00C);
constexpr int ErrorCnCheck = static_cast<int>(0x8095F00B);
constexpr int ErrorCallbackCheck = static_cast<int>(0x8095F0FD);
constexpr int ErrorWantPollIn = static_cast<int>(0x8095F013);
constexpr int ErrorWantPollOut = static_cast<int>(0x8095F014);
constexpr int ErrorEof = static_cast<int>(0x8095177C);
constexpr std::uint32_t FlagServerVerify = 0x1;
constexpr std::uint32_t FlagCnCheck = 0x4;
constexpr std::uint32_t FlagNotAfterCheck = 0x8;
constexpr std::uint32_t FlagNotBeforeCheck = 0x10;
constexpr std::uint32_t FlagKnownCaCheck = 0x20;
constexpr std::uint32_t AllChecks = FlagServerVerify | FlagCnCheck | FlagNotAfterCheck | FlagNotBeforeCheck | FlagKnownCaCheck;
constexpr int NetAfInet = 2;
constexpr int NetSockStream = 1;
constexpr int NetSolSocket = 0xFFFF;
constexpr int NetSoNbio = 0x1200;
constexpr short PollIn = 0x1;
constexpr short PollOut = 0x4;

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "ssl transport check failed at line %d\n", line);
        std::abort();
    }
}
#define Require(value) Check((value), __LINE__)

template <typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

int HostSend(void* context, const unsigned char* bytes, std::size_t length) {
    const auto socket = *static_cast<HostSocket*>(context);
#ifdef _WIN32
    const int sent = ::send(socket, reinterpret_cast<const char*>(bytes), static_cast<int>(length), 0);
    if (sent < 0) return WSAGetLastError() == WSAEWOULDBLOCK ? MBEDTLS_ERR_SSL_WANT_WRITE : MBEDTLS_ERR_NET_SEND_FAILED;
#else
    const auto sent = ::send(socket, bytes, length, MSG_NOSIGNAL);
    if (sent < 0) return errno == EAGAIN || errno == EWOULDBLOCK ? MBEDTLS_ERR_SSL_WANT_WRITE : MBEDTLS_ERR_NET_SEND_FAILED;
#endif
    return static_cast<int>(sent);
}

int HostRecv(void* context, unsigned char* bytes, std::size_t length) {
    const auto socket = *static_cast<HostSocket*>(context);
#ifdef _WIN32
    const int received = ::recv(socket, reinterpret_cast<char*>(bytes), static_cast<int>(length), 0);
    if (received < 0) return WSAGetLastError() == WSAEWOULDBLOCK ? MBEDTLS_ERR_SSL_WANT_READ : MBEDTLS_ERR_NET_RECV_FAILED;
#else
    const auto received = ::recv(socket, bytes, length, 0);
    if (received < 0) return errno == EAGAIN || errno == EWOULDBLOCK ? MBEDTLS_ERR_SSL_WANT_READ : MBEDTLS_ERR_NET_RECV_FAILED;
#endif
    return static_cast<int>(received);
}

struct Server {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context drbg;
    mbedtls_pk_context key;
    mbedtls_x509_crt certificate;
    mbedtls_ssl_config config;
    std::string pem;
    HostSocket listener = InvalidHostSocket;
    std::uint16_t port = 0;
    std::thread thread;

    Server() {
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&drbg);
        mbedtls_pk_init(&key);
        mbedtls_x509_crt_init(&certificate);
        mbedtls_ssl_config_init(&config);
        Require(mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, nullptr, 0) == 0);
        Require(mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) == 0);
        Require(mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key), mbedtls_ctr_drbg_random, &drbg) == 0);

        mbedtls_x509write_cert writer;
        mbedtls_x509write_crt_init(&writer);
        mbedtls_x509write_crt_set_version(&writer, MBEDTLS_X509_CRT_VERSION_3);
        mbedtls_x509write_crt_set_md_alg(&writer, MBEDTLS_MD_SHA256);
        mbedtls_x509write_crt_set_subject_key(&writer, &key);
        mbedtls_x509write_crt_set_issuer_key(&writer, &key);
        Require(mbedtls_x509write_crt_set_subject_name(&writer, "CN=localhost") == 0);
        Require(mbedtls_x509write_crt_set_issuer_name(&writer, "CN=localhost") == 0);
        Require(mbedtls_x509write_crt_set_validity(&writer, "20200101000000", "20991231235959") == 0);
        const unsigned char serial[] = {1};
        Require(mbedtls_x509write_crt_set_serial_raw(&writer, const_cast<unsigned char*>(serial), sizeof(serial)) == 0);
        Require(mbedtls_x509write_crt_set_basic_constraints(&writer, 1, -1) == 0);
        std::vector<unsigned char> buffer(4096);
        Require(mbedtls_x509write_crt_pem(&writer, buffer.data(), buffer.size(), mbedtls_ctr_drbg_random, &drbg) == 0);
        mbedtls_x509write_crt_free(&writer);
        pem = reinterpret_cast<const char*>(buffer.data());
        Require(mbedtls_x509_crt_parse(&certificate, reinterpret_cast<const unsigned char*>(pem.c_str()), pem.size() + 1) == 0);

        Require(mbedtls_ssl_config_defaults(&config, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) == 0);
        mbedtls_ssl_conf_rng(&config, mbedtls_ctr_drbg_random, &drbg);
        mbedtls_ssl_conf_authmode(&config, MBEDTLS_SSL_VERIFY_NONE);
        Require(mbedtls_ssl_conf_own_cert(&config, &certificate, &key) == 0);
        static const char* protocols[] = {"http/1.1", nullptr};
        Require(mbedtls_ssl_conf_alpn_protocols(&config, protocols) == 0);

        listener = ::socket(AF_INET, SOCK_STREAM, 0);
        Require(listener != InvalidHostSocket);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Require(::bind(listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0);
        Require(::listen(listener, 8) == 0);
#ifdef _WIN32
        int length = sizeof(address);
#else
        socklen_t length = sizeof(address);
#endif
        Require(::getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length) == 0);
        port = ntohs(address.sin_port);
        thread = std::thread([this] { Serve(); });
    }

    ~Server() {
        CloseHost(listener);
        thread.join();
        mbedtls_ssl_config_free(&config);
        mbedtls_x509_crt_free(&certificate);
        mbedtls_pk_free(&key);
        mbedtls_ctr_drbg_free(&drbg);
        mbedtls_entropy_free(&entropy);
    }

    void Serve() {
        for (;;) {
            HostSocket client = ::accept(listener, nullptr, nullptr);
            if (client == InvalidHostSocket) return;
            mbedtls_ssl_context ssl;
            mbedtls_ssl_init(&ssl);
            if (mbedtls_ssl_setup(&ssl, &config) == 0) {
                mbedtls_ssl_set_bio(&ssl, &client, HostSend, HostRecv, nullptr);
                int result;
                while ((result = mbedtls_ssl_handshake(&ssl)) == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE) {}
                if (result == 0) Echo(ssl);
            }
            mbedtls_ssl_free(&ssl);
            CloseHost(client);
        }
    }

    static void Echo(mbedtls_ssl_context& ssl) {
        unsigned char buffer[64];
        for (;;) {
            int received = mbedtls_ssl_read(&ssl, buffer, sizeof(buffer));
            if (received == MBEDTLS_ERR_SSL_WANT_READ || received == MBEDTLS_ERR_SSL_WANT_WRITE || received == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET) continue;
            if (received <= 0) return;
            if (received == 5 && std::memcmp(buffer, "close", 5) == 0) {
                while (mbedtls_ssl_close_notify(&ssl) == MBEDTLS_ERR_SSL_WANT_WRITE) {}
                return;
            }
            int offset = 0;
            while (offset < received) {
                const int written = mbedtls_ssl_write(&ssl, buffer + offset, static_cast<std::size_t>(received - offset));
                if (written == MBEDTLS_ERR_SSL_WANT_READ || written == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
                if (written <= 0) return;
                offset += written;
            }
        }
    }
};

int ConnectSocket(std::uint16_t port) {
    const int socket = sceNetSocket("ssl-test", NetAfInet, NetSockStream, 0);
    Require(socket >= 0);
    std::array<std::uint8_t, 16> address{16, NetAfInet, static_cast<std::uint8_t>(port >> 8), static_cast<std::uint8_t>(port & 0xFF), 127, 0, 0, 1};
    Require(sceNetConnect(socket, address.data(), address.size()) == 0);
    return socket;
}

int LoadAuthority(int context, std::string& pem) {
    SslData authority{reinterpret_cast<std::uint8_t*>(pem.data()), pem.size() + 1};
    SslData* list[] = {&authority};
    return sceSslLoadCert(context, 1, list, nullptr, nullptr);
}

struct CallbackRecord {
    int connection = 0;
    unsigned int error = 0;
    int count = 0;
    int result = 0;
    int context = 0;
    std::string subject;
};

int APS5_VABI RecordingCallback(int connection, unsigned int error, void** certificates, int count, void* argument) {
    auto& record = *static_cast<CallbackRecord*>(argument);
    record.connection = connection;
    record.error = error;
    record.count = count;
    if (count > 0 && certificates && certificates[0]) {
        void* name = sceSslGetSubjectName(record.context, certificates[0]);
        Require(name != nullptr);
        Require(sceSslGetNameEntryCount(record.context, name) == 1);
        char oid[16]{};
        char value[64]{};
        std::size_t valueLength = 0;
        Require(sceSslGetNameEntryInfo(record.context, name, 0, oid, sizeof(oid), value, sizeof(value), &valueLength) == 0);
        record.subject = std::string(oid) + "=" + value;
        Require(sceSslFreeSslCertName(record.context, name) == 0);
    }
    return record.result;
}

void Wait(int socket, short events) {
    PollDescriptor waited{socket, events, 0};
    Require(poll_nid_postfix(&waited, 1, 5000) == 1);
}

}  // namespace

int main() {
#ifdef _WIN32
    WSADATA data;
    Require(WSAStartup(MAKEWORD(2, 2), &data) == 0);
#else
    std::signal(SIGPIPE, SIG_IGN);
#endif
    Server server;

    const int trusting = sceSslInit_nid_postfix(0x100000);
    Require(trusting > 0);
    Require(LoadAuthority(trusting, server.pem) == 0);
    const int untrusting = sceSslInit_nid_postfix(0x100000);
    Require(untrusting > 0);

    {
        const int socket = ConnectSocket(server.port);
        const int connection = sceSslCreateConnection(trusting, socket, "localhost");
        Require(connection > 0);
        Require(sceSslEnableVerifyOption(connection, AllChecks) == 0);
        Require(sceSslSetMinSslVersion(connection, 5) == 0);
        Require(sceSslSetAlpn(connection, "http/1.1") == 0);
        Require(sceSslGetAlpnSelected(connection) == nullptr);
        Require(sceSslConnect(connection) == 0);
        const char* selected = sceSslGetAlpnSelected(connection);
        Require(selected != nullptr && std::strcmp(selected, "http/1.1") == 0);
        char ping[] = "ping";
        Require(sceSslWrite(connection, ping, 4, 0) == 4);
        char buffer[16]{};
        Require(sceSslRead(connection, buffer, sizeof(buffer), 0) == 4);
        Require(std::memcmp(buffer, "ping", 4) == 0);
        char close[] = "close";
        Require(sceSslWrite(connection, close, 5, 0) == 5);
        Require(sceSslRead(connection, buffer, sizeof(buffer), 0) == ErrorEof);
        Require(sceSslClose(connection) == 0);
        Require(Throws([&] { sceSslRead(connection, buffer, sizeof(buffer), 0); }));
        Require(sceSslDeleteConnection(connection) == 0);
        Require(sceNetSocketClose(socket) == 0);
    }

    {
        const int socket = ConnectSocket(server.port);
        const int connection = sceSslCreateConnection(untrusting, socket, "localhost");
        Require(sceSslEnableVerifyOption(connection, AllChecks) == 0);
        Require(sceSslConnect(connection) == ErrorUnknownCa);
        Require(sceSslConnect(connection) == ErrorUnknownCa);
        char buffer[16]{};
        Require(Throws([&] { sceSslRead(connection, buffer, sizeof(buffer), 0); }));
        Require(Throws([&] { sceSslWrite(connection, buffer, sizeof(buffer), 0); }));
        Require(sceSslClose(connection) == 0);
        Require(sceSslDeleteConnection(connection) == 0);
        Require(sceNetSocketClose(socket) == 0);
    }

    {
        const int socket = ConnectSocket(server.port);
        const int connection = sceSslCreateConnection(trusting, socket, "wrong.invalid");
        Require(sceSslEnableVerifyOption(connection, AllChecks) == 0);
        Require(sceSslConnect(connection) == ErrorCnCheck);
        Require(sceSslDeleteConnection(connection) == 0);
        Require(sceNetSocketClose(socket) == 0);
    }

    {
        const int socket = ConnectSocket(server.port);
        const int connection = sceSslCreateConnection(trusting, socket, "wrong.invalid");
        Require(sceSslEnableVerifyOption(connection, AllChecks) == 0);
        Require(sceSslDisableVerifyOption(connection, FlagCnCheck) == 0);
        Require(sceSslConnect(connection) == 0);
        Require(sceSslClose(connection) == 0);
        Require(sceSslDeleteConnection(connection) == 0);
        Require(sceNetSocketClose(socket) == 0);
    }

    {
        const int socket = ConnectSocket(server.port);
        const int connection = sceSslCreateConnection(untrusting, socket, "localhost");
        Require(sceSslConnect(connection) == 0);
        Require(sceSslClose(connection) == 0);
        Require(sceSslDeleteConnection(connection) == 0);
        Require(sceNetSocketClose(socket) == 0);
    }

    {
        CallbackRecord record;
        record.context = untrusting;
        const int socket = ConnectSocket(server.port);
        const int connection = sceSslCreateConnection(untrusting, socket, "localhost");
        Require(sceSslEnableVerifyOption(connection, AllChecks) == 0);
        Require(sceSslSetVerifyCallback(connection, RecordingCallback, &record) == 0);
        Require(sceSslConnect(connection) == 0);
        Require(record.connection == connection);
        Require(record.error == static_cast<unsigned int>(ErrorUnknownCa));
        Require(record.count == 1);
        Require(record.subject == "CN=localhost");
        Require(sceSslClose(connection) == 0);
        Require(sceSslDeleteConnection(connection) == 0);
        Require(sceNetSocketClose(socket) == 0);
    }

    {
        CallbackRecord record;
        record.context = trusting;
        record.result = -1;
        const int socket = ConnectSocket(server.port);
        const int connection = sceSslCreateConnection(trusting, socket, "localhost");
        Require(sceSslEnableVerifyOption(connection, AllChecks) == 0);
        Require(sceSslSetVerifyCallback(connection, RecordingCallback, &record) == 0);
        Require(sceSslConnect(connection) == ErrorCallbackCheck);
        Require(record.error == 0);
        Require(sceSslDeleteConnection(connection) == 0);
        Require(sceNetSocketClose(socket) == 0);
    }

    {
        const int socket = ConnectSocket(server.port);
        const int enabled = 1;
        Require(sceNetSetsockopt(socket, NetSolSocket, NetSoNbio, &enabled, sizeof(enabled)) == 0);
        const int connection = sceSslCreateConnection(trusting, socket, "localhost");
        Require(sceSslEnableVerifyOption(connection, AllChecks) == 0);
        int result;
        int rounds = 0;
        while ((result = sceSslConnect(connection)) != 0) {
            Require(result == ErrorWantPollIn || result == ErrorWantPollOut);
            Wait(socket, result == ErrorWantPollIn ? PollIn : PollOut);
            Require(++rounds < 100);
        }
        char buffer[16]{};
        Require(sceSslRead(connection, buffer, sizeof(buffer), 0) == ErrorWantPollIn);
        char ping[] = "ping";
        Require(sceSslWrite(connection, ping, 4, 0) == 4);
        int received;
        rounds = 0;
        while ((received = sceSslRead(connection, buffer, sizeof(buffer), 0)) == ErrorWantPollIn) {
            Wait(socket, PollIn);
            Require(++rounds < 100);
        }
        Require(received == 4 && std::memcmp(buffer, "ping", 4) == 0);
        Require(sceSslClose(connection) == 0);
        Require(sceSslDeleteConnection(connection) == 0);
        Require(sceNetSocketClose(socket) == 0);
    }

    {
        const int connection = sceSslCreateConnection(trusting, 123456, "localhost");
        Require(Throws([&] { sceSslConnect(connection); }));
        Require(sceSslDeleteConnection(connection) == 0);
    }

    Require(sceSslTerm_nid_postfix(untrusting) == 0);
    Require(sceSslTerm_nid_postfix(trusting) == 0);
    return 0;
}
