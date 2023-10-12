#include "coroutine.h"
#include <atomic>
#include "scheduler.h"
#include <iostream>
#include <assert>
namespace yhchaos{
//线程当前执行的协程
static thread_local Coroutine* t_CurCoroutine=nullptr;
//线程的主协程
static thread_local Coroutine::ptr t_RootCoroutine=nullptr;

static std::atomic<uint64_t> s_coroutine_id{0};
static std::atomic<uint64_t> s_coroutine_count{0};

static uint64_t g_coroutine_stack_size=128*1024;
//创建线程的主协程
Coroutine::Coroutine(){
    m_state=EXEC;
    setThreadCurCo(this);
    if(getcontext(&m_ctx)){

    }
    m_id=++s_coroutine_id;
    ++s_coroutine_count;
}
Coroutine::Coroutine(std::function<void()> cb, size_t stackSize=0, bool useCaller=false)
    :m_id(++s_coroutine_id)
    ,m_cb(cb) { 
    ++s_coroutine_count;
    m_stackSize=stackSize ? stackSize : g_coroutine_stack_size;
    m_stack = new size_t(m_stackSize);
    if(getcontext(&m_ctx)){
        cout<<endl;
    }
    m_ctx.uc_link=nullptr;
    m_ctx.uc_stack.ss_sp=m_stack;
    m_ctx.uc_stack.ss_size=m_stackSize;
    if(useCaller){
        makecontext(&m_ctx, &Coroutine::CoFuncSwapOut2Root, 0);
    }
    else{
        makecontext(m_ctx, &Coroutine::CoFuncSwapOut2Scd, 0);
    }
}   
Coroutine::~Coroutine(){
    --s_coroutine_count;
    if(m_stack){
        delete(m_stack);
    }
    else{
        Coroutine* cur=t_CurCoroutine;
        if(cur==this){
            t_CurCoroutine=nullptr;
        }
    }
}

uint64_t Coroutine::getId() {return m_id};
State Coroutine::getState() {return m_state};

void Coroutine::reset(std::function<void()> cb){
    m_cb=cb;
    m_state=INIT;
    if(getcontext(&m_ctx)){
        std::cout<<"error"<<endl;
    }
    m_ctx.uc_link=nullptr;
    m_ctx.uc_stack.ss_sp=m_stack;
    m_ctx.uc_satck.ss_size=m_stackSize;
    makecontext(&m_ctx, &Coroutine::MainFunc, 0);
}

void Coroutine::SwapInFromScdCo(){
    setThreadCurCo(this);
    m_state=EXEC;
    if(swapcontext(&Scheduler::getSchedulerCoroutine(), &m_ctx)){
        std::cout<<"error"<<endl;
    }
}
void Coroutine::SwapOutToScdCo(){
    setThreadCurCo(Scheduler::getSchedulerCoroutine());
    if(swapcontext(&m_ctx, &Scheduler::getSchedulerCoroutine())){
        cout<<"error"<<std::endl;
    }
}
void Coroutine::SwapInFromRootCo(){
    setThreadCurCo(this);
    m_state=EXEC;
    if(swapcontext(&t_RootCoroutine->m_ctx, &m_ctx)){
        std::cout<<"error"<std::endl;
    }
}
void Coroutine::SwapOut2RootCo(){
    setThis(t_RootCoroutine.get());
    if(swapcontext(&m_ctx, t_RootCoroutine->m_ctx)){
        //
    }
}

void Coroutine::SetHoldAndSwap2Scd(){
    Coroutine::ptr cur=getThreadCurCo();
    assert(cur->m_state==EXEC);
    m_state=HOLD;
    SwapOutToScdCo();
}
void Coroutine::SetReadyAndSwap2Scd(){
    Coroutine::ptr cur=getThrea();
    assert(cur->m_state==EXEC);
    m_state=READY;
    SwapOutToScdCo();
}

void Coroutine::setThreadCurCo(Coroutine* f){
    t_CurCoroutine=f;
}
Coroutine::ptr Coroutine::getThreadCurCo(){
    if(t_CurCoroutine){
        return t_CurCoroutine;
    }
    Coroutine::ptr mainCoroutine(new Coroutine);
    t_RootCoroutine=mainCoroutine;
    return t_CurCoroutine->shared_from_this();
} 

uint64_t Coroutine::totalCoroutines(){
    return s_coroutine_count;
}

void Coroutine::CoFuncSwapOut2Scd(){
    Coroutine::ptr cur=getThreadCurCo();
    try{
        cur->m_cb();
        cur->m_state=TERM;
        cur->m_cb=nullptr;
    }catch(std::exception& ex){
        cur->m_state=EXCEPTION;
        //
    }
    catch(...){
        cur->m_state=EXCEPTION;
        //error
    }
    auto raw_ptr=cur.get();
    cur.reset();
    raw_ptr->SwapOutToScdCo();

}
void Coroutine::CoFuncSwapOut2Root(){
    Coroutine::ptr cur=getThreadCurCo();
    try{
        cur->m_cb();
        cur->m_state=TERM;
        cur->m_cb=nullptr;
    }catch(std::exception& ex){
        cur->m_state=EXCEPTION;
        //error
    }catch(...){
        cur->m_state=EXCEPTION;
        //error
    }
    auto raw_ptr=cur.get();
    cur.reset();
    cur->SwapOut2RootCo();
}

uint_64 Coroutine::getCurCoroutineId(){
    if(t_CurCoroutine){
        return t_CurCoroutine->m_id;
    }
    return 0;
}
}

