#include "Fe65p2Cmd.h"

Fe65p2Cmd::Fe65p2Cmd(TxCore *arg_core) {
    core = arg_core;
}

Fe65p2Cmd::~Fe65p2Cmd() {

}

void Fe65p2Cmd::writeGlobal(uint16_t *cfg) {
    for (unsigned i=0; i<Fe65p2GlobalCfg::numRegs; i++) {
        uint32_t cmd = MOJO_HEADER;
        cmd |= ((GLOBAL_REG_BASE + i) << 16);
        cmd |= (0xffff & cfg[i]);
        core->writeFifo(0x0);
        core->writeFifo(cmd);
    }
    core->writeFifo(0x0);
    core->writeFifo(MOJO_HEADER + (PULSE_REG << 16) + PULSE_SHIFT_GLOBAL);
    while(core->isCmdEmpty() == 0);
    usleep(10); // Need to wait for Mojo to send
}

void Fe65p2Cmd::writePixel(uint16_t *bitstream) {
    for (unsigned i=0; i<Fe65p2PixelCfg::n_Words; i++) {
        uint32_t cmd = MOJO_HEADER;
        cmd |= ((PIXEL_REG_BASE + i) << 16);
        cmd |= (0xffff & bitstream[i]);
        core->writeFifo(0x0);
        core->writeFifo(cmd);
    }
    core->writeFifo(0x0);
    core->writeFifo(MOJO_HEADER + (PULSE_REG << 16) + PULSE_SHIFT_PIXEL);
    while(core->isCmdEmpty() == 0);
    usleep(15); // Need to wait for Mojo to send
}

void Fe65p2Cmd::writePixel(uint16_t mask) {
    for (unsigned i=0; i<Fe65p2PixelCfg::n_Words; i++) {
        uint32_t cmd = MOJO_HEADER;
        cmd |= ((PIXEL_REG_BASE + i) << 16);
        cmd |= (0xffff & mask);
        core->writeFifo(0x0);
        core->writeFifo(cmd);
    }
    core->writeFifo(0x0);
    core->writeFifo(MOJO_HEADER + (PULSE_REG << 16) + PULSE_SHIFT_PIXEL);
    while(core->isCmdEmpty() == 0);
    usleep(15); // Need to wait for Mojo to send
}

void Fe65p2Cmd::setLatency(uint16_t lat) {
    core->writeFifo(0x0);
    core->writeFifo(MOJO_HEADER + (LAT_REG << 16) + lat);
    while(core->isCmdEmpty() == 0);
}

void Fe65p2Cmd::injectAndTrigger() {
    core->writeFifo(0x0);
    core->writeFifo(MOJO_HEADER + (PULSE_REG << 16) + PULSE_INJECT);
    while(core->isCmdEmpty() == 0);
}

void Fe65p2Cmd::reset() {
    setStaticReg(STATIC_RST0);
    setStaticReg(STATIC_RST1);
    this->writeStaticReg();
    usleep(1000);
    unsetStaticReg(STATIC_RST0);
    unsetStaticReg(STATIC_RST1);
    this->writeStaticReg();
}

void Fe65p2Cmd::clocksOn() {
    setStaticReg(STATIC_DATA_CLK);
    setStaticReg(STATIC_BX_CLK);
    this->writeStaticReg();
}

void Fe65p2Cmd::clocksOff() {
    unsetStaticReg(STATIC_DATA_CLK);
    unsetStaticReg(STATIC_BX_CLK);
    this->writeStaticReg();
}

void Fe65p2Cmd::enAnaInj() {
    setStaticReg(STATIC_ANINJ);
    this->writeStaticReg();
}

// TODO disable all injections
void Fe65p2Cmd::enDigInj() {
    unsetStaticReg(STATIC_ANINJ);
    this->writeStaticReg();
}

void Fe65p2Cmd::writeStaticReg() {
    core->writeFifo(0x0);
    core->writeFifo(MOJO_HEADER + (STATIC_REG << 16) + static_reg);
    while(core->isCmdEmpty() == 0);
}

void Fe65p2Cmd::setStaticReg(uint32_t bit) {
    static_reg |= bit;
}

void Fe65p2Cmd::unsetStaticReg(uint32_t bit) {
    static_reg &= ~(bit);
}
