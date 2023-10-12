#ifndef _YHCHAOS_FIBER_H_
#define _YHCHAOS_FIBER_H_

#include <memory>
#include <functional>
#include <ucontext.h>

namespace yhchaos {
class Coroutine : public std::enable_shared_from_this<Coroutine> {
public:
    typedef std::shared_ptr<Coroutine> ptr;
    enum State{
        INIT,
        HOLD,
        READY,
        EXEC,
        EXCEPTION,
        TERM
    };
private:
    Coroutine();
public:
    Coroutine(std::function<void()> cb, size_t stackSize=0, bool useCaller=false);
    ~Coroutine();

    uint64_t getId() {return m_id};
    State getState() {return m_state};
    
    void reset(std::function<void()> cb);
    /**
     * @brief 从调度协程切换到此协程
    */
    void SwapInFromScdCo();

    /**
     * @brief 从此协程切换到调度协程
    */
    void SwapOutToScdCo();

    /**
     * @brief 从主协程切换到此协程
    */
    void SwapInFromRootCo();

    /**
     * @brief 从此协程切换到主协程
    */
    void SwapOut2RootCo();

    /**
     * @bried 将当前协程设为挂起状态切换回当前进程的调度协程
    */
    static void SetHoldAndSwap2Scd();

    /**
     * @bried 将当前协程设为就绪状态切换回当前进程的调度协程
    */
    static void SetReadyAndSwap2Scd();
    
    /**
     * @brief 设置当前线程正在执行的协程
    */
    static void setThreadCurCo(Coroutine* f);

    /**
     * @brief 获得当前线程正在执行的协程
    */
    static Coroutine::ptr getThreadCurCo(); 

    /**
     * @brief 协程总数
    */
    static uint64_t totalCoroutines();

    /**
     * @brief 协程执行的函数，执行完毕或切换回调度协程
    */
    static void CoFuncSwapOut2Scd();

    /**
     * @brief 协程执行的函数，执行完毕或切换回调度协程
    */
    static void CoFuncSwapOut2Root();
    
    /**
     * 获取线程当前执行协程的协程id
    */
    static getCurCoroutineId();
private:
    std::function<void()> m_cb;
    void* m_stack=nullptr;
    size_t m_stackSize=0;
    State m_state=INIT;
    ucontext_t m_ctx;
    uint64_t m_id=0;
}
}
#endif