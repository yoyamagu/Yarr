// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: YARR FW Library
// # Comment: Transmitter Core
// ################################

#include "SpecTxCore.h"

SpecTxCore::SpecTxCore() {
    verbose = false;
    enMask = 0x0;
}

void SpecTxCore::writeFifo(uint32_t value) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ 
            << " : Writing 0x" << std::hex << value << std::dec << std::endl;
    SpecCom::writeSingle(TX_ADDR | TX_FIFO, value);
}

void SpecTxCore::setCmdEnable(uint32_t value) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << value << std::dec << std::endl;
    SpecCom::writeSingle(TX_ADDR | TX_ENABLE, value);
    enMask = value;
}

uint32_t SpecTxCore::getCmdEnable() {
    return SpecCom::readSingle(TX_ADDR | TX_ENABLE);
}

void SpecTxCore::maskCmdEnable(uint32_t value, uint32_t mask) {
    uint32_t tmp = SpecCom::readSingle(TX_ADDR | TX_ENABLE);
    tmp &= ~mask;
    value |= tmp;
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << value << std::dec << std::endl;
    SpecCom::writeSingle(TX_ADDR | TX_ENABLE, value);
}

void SpecTxCore::setTrigEnable(uint32_t value) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << value << std::dec << std::endl;
    SpecCom::writeSingle(TX_ADDR | TRIG_EN, value);
}

uint32_t SpecTxCore::getTrigEnable() {
    return SpecCom::readSingle(TX_ADDR | TRIG_EN);
}

void SpecTxCore::maskTrigEnable(uint32_t value, uint32_t mask) {
    uint32_t tmp = SpecCom::readSingle(TX_ADDR | TX_ENABLE);
    tmp &= ~mask;
    value |= tmp;
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << value << std::dec << std::endl;
    SpecCom::writeSingle(TX_ADDR | TRIG_EN, value);
}

void SpecTxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Config 0x" << std::hex << cfg << std::dec << std::endl;
    SpecCom::writeSingle(TX_ADDR | TRIG_CONF, (uint32_t) cfg);
}

void SpecTxCore::setTrigFreq(double freq) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Frequency " << freq/1.0e3 << " kHz" <<std::endl;
    uint32_t tmp = 1.0/((double)TX_CLK_PERIOD * freq);
    SpecCom::writeSingle(TX_ADDR | TRIG_FREQ, tmp);
}

void SpecTxCore::setTrigCnt(uint32_t count) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Count " << count << std::endl;
    SpecCom::writeSingle(TX_ADDR | TRIG_COUNT, count);
}

  
void SpecTxCore::setTrigTime(double time) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Time " << time << " s" <<std::endl;
    uint64_t tmp = (1.0/(double)TX_CLK_PERIOD)*time;
    SpecCom::writeBlock(TX_ADDR | TRIG_TIME, (uint32_t*)&tmp, 2);
}

void SpecTxCore::setTrigWordLength(uint32_t length) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Length " << length << " bit" <<std::endl;
    SpecCom::writeSingle(TX_ADDR | TRIG_WORD_LENGTH, length);
}

void SpecTxCore::setTrigWord(uint32_t *word) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Word 0x" << std::hex 
            << word[4] << word[3] << word[2] << word[1] << std::dec << std::endl;
    SpecCom::writeBlock(TX_ADDR | TRIG_WORD, word, 4);
}

void SpecTxCore::toggleTrigAbort() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Toggling Trigger abort!" << std::endl;
    SpecCom::writeSingle(TX_ADDR | TRIG_ABORT, 0x1);
}

bool SpecTxCore::isTrigDone() {
    return SpecCom::readSingle(TX_ADDR | TRIG_DONE);
}
 
bool SpecTxCore::isCmdEmpty() {
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    return (SpecCom::readSingle(TX_ADDR | TX_EMPTY) & enMask);
}

uint32_t SpecTxCore::getTrigInCount() {
    return (SpecCom::readSingle(TX_ADDR | TRIG_IN_CNT));
}

