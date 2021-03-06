#ifndef BOOKKEEPER_H
#define BOOKKEEPER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Bookkeeper
// # Comment: Global container for data
// ################################

#include <iostream>
#include <mutex>
#include <deque>

#include "RawData.h"
#include "Fei4EventData.h"
#include "HistogramBase.h"
#include "ResultBase.h"
#include "ClipBoard.h"

#include "FrontEnd.h"
#include "Fei4.h"
#include "Fe65p2.h"
#include "TxCore.h"
#include "RxCore.h"

class Bookkeeper {
    public:
        Bookkeeper(TxCore *arg_tx, RxCore *arg_rx);
        ~Bookkeeper();
        
        // TODO should only add generic Fe class
        void addFe(FrontEnd *fe, unsigned txChannel, unsigned rxChannel);
        void addFe(FrontEnd *fe, unsigned channel);
		
        void delFe(unsigned rxChannel);
		void delFe(FrontEnd *fe);

		FrontEnd* getFeByChannel(unsigned channel);
		FrontEnd* getFe(unsigned rxChannel);
        FrontEnd* getLastFe();
        FrontEnd* getGlobalFe() {
            return g_fe;
        }

		bool isChannelUsed(unsigned arg_channel);
        
        // Construct mask of active channels
        uint32_t getTxMask();
        uint32_t getRxMask();

        void setTargetTot(int v) {target_tot = v;}
        int getTargetTot() {return target_tot;}
        
        void setTargetCharge(int v) {target_charge = v;}
        int getTargetCharge() {return target_charge;}

        void setTargetThreshold(int v) {target_threshold = v;}
        int getTargetThreshold() {return target_threshold;}
        
        // TODO make private, not nice like that
        Fei4 *g_fe;
        Fe65p2 *g_fe65p2;
        TxCore *tx;
        RxCore *rx;
        
        std::vector<FrontEnd*> feList;

        ClipBoard<RawDataContainer> rawData;

        // per rx link
	    std::map<unsigned, ClipBoard<Fei4Data> > eventMap;
	    std::map<unsigned, ClipBoard<HistogramBase> > histoMap;
	    std::map<unsigned, ClipBoard<HistogramBase> > resultMap;
		std::map<unsigned, std::mutex> mutexMap;	
        
		std::vector<FrontEnd*> activeFeList;

    private:
        //uint32_t activeTxMask;
        //uint32_t activeRxMask;

		//uint32_t activeMask;
		//uint32_t usedChannels;

        int target_tot;
        int target_threshold;
        int target_charge;
};

#endif
