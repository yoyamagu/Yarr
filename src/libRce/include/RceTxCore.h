#ifndef RCETXCORE_H
#define RCETXCORE_H



#include <iostream>
#include <thread>
#include <mutex>

#include "TxCore.h"
#include "RceCom.h"

class RceTxCore : virtual public TxCore {
    public:
        RceTxCore(RceCom *com);
        RceTxCore();
        ~RceTxCore();

        void setCom(RceCom *com) {m_com = com;}
        RceCom* getCom() {return m_com;}

        void writeFifo(uint32_t value);
        void releaseFifo() {this->writeFifo(0x0);m_com->releaseFifo();} // Add some padding
        
        void setCmdEnable(uint32_t value) {std::cout <<"en "; }
        uint32_t getCmdEnable() {return 0x0;}
        void maskCmdEnable(uint32_t value, uint32_t mask) {}

        void setTrigEnable(uint32_t value);
        uint32_t getTrigEnable() {return triggerProc.joinable() || !m_com->isEmpty();}
        void maskTrigEnable(uint32_t value, uint32_t mask) {}

        void setTrigConfig(enum TRIG_CONF_VALUE cfg) {}
        void setTrigFreq(double freq) {}
        void setTrigCnt(uint32_t count);
        void setTrigTime(double time) {}
        void setTrigWordLength(uint32_t length) {}
        void setTrigWord(uint32_t *word) {}

        void toggleTrigAbort() {}

        bool isCmdEmpty() {
            bool rtn = m_com->isEmpty();
            return rtn;
        }
        bool isTrigDone() {
            bool rtn = !trigProcRunning && m_com->isEmpty();
            return rtn;
        }

        uint32_t getTrigInCount() {return 0x0;}
        
        void setTriggerLogicMask(uint32_t mask) {}
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) {}
        void resetTriggerLogic() {}

    private:
        RceCom *m_com;

        unsigned m_trigCnt;
        std::mutex accMutex;
        std::thread triggerProc;
        bool trigProcRunning;
        void doTrigger();
};

#endif
