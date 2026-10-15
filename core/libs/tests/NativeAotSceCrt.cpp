#include "native/NativeAotSceCrt.hpp"
#include "native/NativeAotSceAbi.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/FileStream.hpp"
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

extern "C" int pthread_join_nid_postfix(Pthread, void**);

namespace {
using State = NativeAotGuestState;
static_assert(!NativeAotSce::ScalarArgument<long double>);
static_assert(!NativeAotSce::ScalarArgument<void (*)()>);
void Require(bool value) { if (!value) std::abort(); }
void Set(State& state, unsigned index, std::uint64_t value) {
    constexpr unsigned registers[] = {7,6,2,1,8,9};
    if (index < 6) state.Gpr[registers[index]] = value;
    else std::memcpy(reinterpret_cast<void*>(state.Gpr[4] + 8 + (index - 6) * 8), &value, 8);
}
template<class T> void Pointer(State& state, unsigned index, T* value) { Set(state,index,reinterpret_cast<std::uint64_t>(value)); }
void Floating(State& state, unsigned index, double value) { std::memcpy(state.Xmm[index], &value, 8); }
void Call(State& state, const char* name) { auto function = NativeAotSce::CrtProvider(name); Require(function != nullptr); function(state); }

std::uint64_t Mixed(int a,double b,int c,double d,int e,double f,int g,double h,int i,double j,int k,double l,int m,double n,int o,double p,int q,double r,int s) {
    Require(a==1 && c==3 && e==5 && g==7 && i==9 && k==11 && m==13 && o==15 && q==17 && s==19);
    Require(b==2 && d==4 && f==6 && h==8 && j==10 && l==12 && n==14 && p==16 && r==18);
    return 0xfedcba9876543210ull;
}

void MixedArguments() {
    State state{};
    std::uint64_t stack[]={0,13,15,17,0,19};
    const double last=18; std::memcpy(&stack[4],&last,8);
    state.Gpr[4]=reinterpret_cast<std::uint64_t>(stack);
    for(unsigned i=0;i<6;++i) Set(state,i,i*2+1);
    for(unsigned i=0;i<8;++i) Floating(state,i,i*2+2);
    NativeAotSce::ScalarBridge<Mixed>(state);
    Require(state.Gpr[0]==0xfedcba9876543210ull);
}

void Formatting() {
    State state{};
    std::uint64_t stack[16]{}; state.Gpr[4]=reinterpret_cast<std::uint64_t>(stack);
    std::array<char,128> text; text.fill('X');
    Pointer(state,0,text.data());Set(state,1,120);Pointer(state,2,"%d %s %.2f %llx %d %d");
    Set(state,3,static_cast<std::uint64_t>(-7));Pointer(state,4,"hello");Set(state,5,0x123456789abcdef0);Set(state,6,41);Set(state,7,42);Floating(state,0,3.25);
    Call(state,"snprintf");
    Require(std::string(text.data())=="-7 hello 3.25 123456789abcdef0 41 42");
    Require(state.Gpr[0]==std::strlen(text.data()) && text[120]=='X');
    Set(state,1,4);Call(state,"snprintf");Require(std::string(text.data())=="-7 " && state.Gpr[0]>3);
    text.fill('X');Set(state,1,0);Call(state,"snprintf");Require(text[0]=='X');
    Pointer(state,0,text.data());Pointer(state,1,"%2$d %1$d %3$.1f");Set(state,2,11);Set(state,3,22);Floating(state,0,4.5);
    Call(state,"sprintf");Require(std::string(text.data())=="22 11 4.5");
    Pointer(state,1,"%Lf");bool rejected=false;try{Call(state,"sprintf");}catch(const std::invalid_argument&){rejected=true;}Require(rejected);
    Pointer(state,1,"%1$Lf");rejected=false;try{Call(state,"sprintf");}catch(const std::invalid_argument&){rejected=true;}Require(rejected);
    std::array<std::byte,176> save{};std::uint64_t integer=27;double fraction=0.25;
    std::memcpy(save.data(),&integer,8);std::memcpy(save.data()+48,&fraction,8);
    VaList args{0,48,stack+1,save.data()};
    Pointer(state,0,text.data());Set(state,1,text.size());Pointer(state,2,"%d %.2f");Pointer(state,3,&args);
    Call(state,"vsnprintf");Require(std::string(text.data())=="27 0.25");Require(args.gp_offset==0 && args.fp_offset==48);
}

void StreamsAndScanning() {
    State state{};std::uint64_t stack[16]{};state.Gpr[4]=reinterpret_cast<std::uint64_t>(stack);
    auto* native=std::tmpfile();Require(native!=nullptr);FileStream file(native);
    Pointer(state,0,&file);Pointer(state,1,"%d %.2f");Set(state,2,17);Floating(state,0,2.25);Call(state,"fprintf");Require(state.Gpr[0]==7);
    std::rewind(native);int number=0;double value=0;
    Pointer(state,0,&file);Pointer(state,1,"%d %lf");Pointer(state,2,&number);Pointer(state,3,&value);Call(state,"fscanf");Require(state.Gpr[0]==2 && number==17 && value==2.25);
    long long wide=0;Pointer(state,0,"99 123456789012 1.5");Pointer(state,1,"%*d %ld %lf");Pointer(state,2,&wide);Pointer(state,3,&value);Call(state,"sscanf");Require(state.Gpr[0]==2 && wide==123456789012ll && value==1.5);
    Pointer(state,1,"%ls");bool rejected=false;try{Call(state,"sscanf");}catch(const std::invalid_argument&){rejected=true;}Require(rejected);
    std::fclose(native);
}

std::atomic<unsigned> callbacks{0};
std::uint64_t Callback(State* state, std::uint64_t) {
    const auto* left = reinterpret_cast<const int*>(state->Gpr[7]);
    const auto* right = reinterpret_cast<const int*>(state->Gpr[6]);
    state->Gpr[0] = static_cast<std::uint64_t>(*left < *right ? -1 : *left > *right ? 1 : 0);
    callbacks.fetch_add(1);
    std::uint64_t result;
    std::memcpy(&result, reinterpret_cast<const void*>(state->Gpr[4]), 8);
    state->Gpr[4] += 8;
    return result;
}
std::uint64_t Worker(State* state, std::uint64_t) {
    auto* value = reinterpret_cast<std::atomic<unsigned>*>(state->Gpr[7]);
    value->fetch_add(1);
    state->Gpr[0] = state->Gpr[7];
    std::uint64_t result;
    std::memcpy(&result, reinterpret_cast<const void*>(state->Gpr[4]), 8);
    state->Gpr[4] += 8;
    return result;
}
void ConvertedCallbacks() {
    static const std::uint8_t bytes[8192]{};
    static const NativeAotGuestSegment segments[] = {{0, sizeof(bytes), sizeof(bytes), 0, 5}};
    static const NativeAotGuestEntry entries[] = {{4096, Callback}, {4112, Worker}};
    static std::uint64_t base;
    static NativeAotGuestImage image{};
    image.Version=9;image.Bytes=bytes;image.ByteCount=sizeof(bytes);image.Segments=segments;image.SegmentCount=1;
    image.Entries=entries;image.EntryCount=2;image.EntryPoint=4096;image.Span=sizeof(bytes);image.Base=&base;image.Concurrent=1;
    NativeAotGuest::Load(image);
    State state{};
    std::array<int,16> values{5,1,9,-5,4,3,10,2,7,7,-1,0,12,-14,91,11};
    Pointer(state,0,values.data());Set(state,1,values.size());Set(state,2,sizeof(int));Set(state,3,base+4096);
    Call(state,"qsort");Require(std::is_sorted(values.begin(),values.end()) && callbacks.load()>0);
    for (unsigned run=0;run<16;++run) {
        Pthread thread=nullptr;std::atomic<unsigned> value{0};
        Pointer(state,0,&thread);Set(state,1,0);Set(state,2,base+4112);Pointer(state,3,&value);
        Call(state,"pthread_create");Require(state.Gpr[0]==0 && thread!=nullptr);
        void* returned=nullptr;Require(pthread_join_nid_postfix(thread,&returned)==0);
        Require(value.load()==1 && returned==&value);
    }
}

void AggregatesAndCallbackRejections() {
    State state{};Set(state,0,static_cast<std::uint64_t>(-23));Set(state,1,5);Call(state,"lldiv");
    Require(state.Gpr[0]==static_cast<std::uint64_t>(-4) && state.Gpr[2]==static_cast<std::uint64_t>(-3));
    Set(state,1,0);bool rejected=false;try{Call(state,"lldiv");}catch(const std::invalid_argument&){rejected=true;}Require(rejected);
    Set(state,3,0);rejected=false;try{Call(state,"qsort");}catch(const std::invalid_argument&){rejected=true;}Require(rejected);
    Set(state,2,0);rejected=false;try{Call(state,"pthread_create");}catch(const std::invalid_argument&){rejected=true;}Require(rejected);
    Require(NativeAotSce::CrtProvider("unqualified")==nullptr);
}
}

int main() { MixedArguments();Formatting();StreamsAndScanning();AggregatesAndCallbackRejections();ConvertedCallbacks(); }
