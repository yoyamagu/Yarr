#include "Receiver.hh"
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <unistd.h>
#include "RCDImasterL.hh"
#include <AxiStreamDma.h>
//#include <boost/date_time/posix_time/posix_time.hpp>
#include <chrono>
namespace PgpTrans{

RCDImaster* RCDImaster::_instance=0;
std::mutex RCDImaster::_guard;

RCDImaster* RCDImaster::instance(){
  if( ! _instance){
    std::unique_lock<std::mutex> ml(_guard);
    if( ! _instance){
      _instance=new RCDImaster;
    }
  }
  return _instance;
}
    
    

RCDImaster::RCDImaster():_receiver(0), _tid(0),
                         _status(0), _data(0), _current(0), _handshake(false),_blockread(false){
  _rxData=new uint32_t* [16];
  for(int i=0;i<16;i++){
    _rxData[i]=new uint32_t [8192];
    _size[i]=0;
  }
}
RCDImaster::~RCDImaster(){
  for(int i=0;i<16;i++)delete [] _rxData[i];
  delete [] _rxData;
}

void RCDImaster::setReceiver(Receiver* receiver){
  _receiver=receiver;}
Receiver* RCDImaster::receiver(){return _receiver;}

  uint32_t RCDImaster::readRegister(uint32_t address, uint32_t &value){
    return rwRegister(false, address, 0, value);
  }
  uint32_t RCDImaster::writeRegister(uint32_t address, uint32_t value){
    uint32_t dummy;
    return rwRegister(true, address, value, dummy);
  }
  
  uint32_t RCDImaster::rwRegister(bool write, uint32_t address, uint32_t value,uint32_t &retvalue){
    std::cout<<"Register write address "<<address<<" value "<<value<<std::endl;
    std::cout<<"RCDImaster"<<std::endl;
    _tid++;
    uint32_t   firstUser;
    uint32_t   lastUser;
    uint32_t   axisDest;
    uint32_t   txSize;
    firstUser = 0x2; // SOF
    lastUser  = 0;
    axisDest=1; //VC
    _txData[0]=_tid;
    _txData[1]= write?0x40000000 : 0;
    _txData[1]|= (address&0x3fffffff); // write address
    _txData[2]=value;
    _txData[3]=0;
    txSize=16;
    std::unique_lock<std::mutex> pl( _data_mutex );
    int retval=axisWrite(_fd, _txData, txSize,firstUser,lastUser,axisDest);
    if(retval<=0){
      printf("Could not write to PGP device.\n");
      return RECEIVEFAILED;
    }
    // boost::posix_time::time_duration timeout= boost::posix_time::microseconds(RECEIVETIMEOUT/1000);

    std::cv_status signalled=_data_cond.wait_for(pl,std::chrono::microseconds(RECEIVETIMEOUT/1000));
    if(signalled==std::cv_status::timeout){ //timeout. Something went wrong with the data.
      printf("PGP Write Register: No reply from front end.\n");
      return RECEIVEFAILED;
    }
    retvalue=_data;
    return _status;
  }
  
  uint32_t RCDImaster::blockWrite(uint32_t* data, int size, bool handshake,bool byteswap){
    //handshake=false;
    printf("Blockwrite: Size %d\n",size);
    uint32_t   firstUser;
    uint32_t   lastUser;
    uint32_t   axisDest;
    uint32_t   txSize;
    firstUser = 0x2; // SOF
    lastUser  = 0;
    axisDest=3; //VC
    _txData[0]=0;
    _txData[1]= handshake? 1 : 0;
    _txData[2]=0;
    _txData[3]=0;
    txSize=16+size*sizeof(uint32_t);
    uint32_t* payload=&_txData[4];
   for(int i=0;i<size;i++){
#ifdef SWAP_DATA
#warning Swapping of data turned on
     if(byteswap)
       payload[i]= ((data[i]&0xff)<<8) | ((data[i]&0xff00)>>8) |
	 ((data[i]&0xff0000)<<8) | ((data[i]&0xff000000)>>8);
     else
       payload[i]=data[i]<<16 | data[i]>>16;
#else
     if(byteswap)
       payload[i]= ((data[i]&0xff)<<24) | ((data[i]&0xff00)<<8) |
	 ((data[i]&0xff0000)>>8) | ((data[i]&0xff000000)>>24);
     else{
       payload[i]=data[i];
       std::cout<<"data "<<std::hex << data[i] << " " <<std::dec<< std::endl;
     }
#endif
   }
   _handshake=handshake;
   if(handshake){
     _data_mutex.lock();
   }
   //std::cout<<"TxSize = "<<txSize<<std::endl;
   int retval=axisWrite(_fd, _txData, txSize,firstUser,lastUser,axisDest);
   //std::cout<<"Retval = "<<retval<<std::endl;
   if(retval<=0){
     printf("Could not write to PGP device.\n");
     return RECEIVEFAILED;
   }
   if(handshake){
     std::unique_lock<std::mutex> pl(_data_mutex, std::adopt_lock);
     //    boost::posix_time::time_duration timeout= boost::posix_time::microseconds(RECEIVETIMEOUT/1000);
     //    int signalled=_data_cond.timed_wait(pl,timeo);
     std::cv_status signalled=_data_cond.wait_for(pl,std::chrono::microseconds(RECEIVETIMEOUT/1000));
     if(signalled==std::cv_status::timeout){ //timeout. Something went wrong with the data.
       printf("PGP Write Register: No reply from front end.\n");
       return RECEIVEFAILED;
     }
   } 
   //printf("Done posting config data\n");
   return 0;
  }

  uint32_t RCDImaster::blockRead(uint32_t* data, int size, std::vector<uint32_t>& retvec){
    _blockread=true;
    blockWrite(data,size,true,false);
    _blockread=false;
    usleep(100);
    while(nBuffers()!=0){
      unsigned char *header, *payload;
      uint32_t headerlen, payloadlen;
      currentBuffer(header, headerlen, payload, payloadlen);
      if(header[2]==0x1f){
	discardCurrentBuffer();
	continue; //handshake
      }
      payloadlen/=sizeof(uint32_t);
      uint32_t* ptr=(uint32_t*)payload;
      for(uint32_t i=0;i<payloadlen;i++) retvec.push_back(*ptr++);
      discardCurrentBuffer();
      break;
    }
    return 0;
  }

  uint32_t RCDImaster::readBuffers(std::vector<unsigned char>& retvec){
    unsigned char *header, *payload;
    uint32_t headerlen, payloadlen;
    uint32_t count=0;
    while(nBuffers()!=0){
      count++;
      currentBuffer(header, headerlen, payload, payloadlen);
      for(uint32_t i=0;i<payloadlen;i++) retvec.push_back(payload[i]);
      if(payloadlen%3!=0){
	retvec.push_back(0);
       if(payloadlen%3==1) retvec.push_back(0);
      }
      discardCurrentBuffer();
    }
    return count;
  }
 
  uint32_t RCDImaster::sendCommand(unsigned char opcode, uint32_t context){
    uint32_t   firstUser;
    uint32_t   lastUser;
    uint32_t   axisDest;
    uint32_t   txSize;
    firstUser = 0x2; // SOF
    lastUser  = 0;
    axisDest=0; //VC
    _txData[0]=context&0xffffff;
    _txData[1]= (uint32_t)opcode;
    _txData[2]=0;
    _txData[3]=0;
    txSize=16;
    printf("Send Command %x\n",(uint32_t) opcode);
    int retval=axisWrite(_fd, _txData, txSize,firstUser,lastUser,axisDest);
    //std::cout<<"Retval = "<<retval<<std::endl;
    if(retval<=0){
      printf("Could not write to PGP device.\n");
      return RECEIVEFAILED;
    }
    return 0;
  }

  uint32_t RCDImaster::sendFragment(uint32_t *data, uint32_t size){
    uint32_t   firstUser;
    uint32_t   lastUser;
    uint32_t   axisDest;
    //    uint32_t   txSize;
    firstUser = 0x2; // SOF
    lastUser  = 0;
    axisDest=2; //VC
    printf("Send Fragment\n");
    int retval=axisWrite(_fd, data, size*sizeof(uint32_t),firstUser,lastUser,axisDest);
    //std::cout<<"Retval = "<<retval<<std::endl;
    if(retval<=0){
      printf("Could not write to PGP device.\n");
      return RECEIVEFAILED;
    }
    return 0;
  }

  int count=0;
  void RCDImaster::receive(){
    const uint32_t headerSize=8;
    int32_t  ret;
    uint32_t firstUser;
    uint32_t lastUser;
    uint32_t axisDest;
    ret = axisRead(_fd, _rxData[_current], 4096,&firstUser,&lastUser,&axisDest);
    if(ret == 0){
      std::cout<<"Receive: no data"<<std::endl;
      return;
    }
    // Bad size or error
    if ( (ret < 0) || (ret % 4) != 0 || (ret-4) < 5 || lastUser ) {
      std::cout << "MultDestAxis::receive -> "
		<< "Error in data receive. Rx=" << std::dec << ret
		<< ", Dest=" << std::dec << axisDest 
		<< ", Last=" << std::dec << lastUser << std::endl;
      unsigned char* rxd=(unsigned char*)_rxData[_current];
      for(int i=0;i<ret;i++)std::cout<<std::hex<<(unsigned)rxd[i]<<" "<<std::dec;
      std::cout<<std::endl;
      return;
    }
    _size[_current]=ret/sizeof(uint32_t);
    //if(ret != 16)count++;
    //else std::cout<<count<<std::endl;
    if (axisDest==1){ // Register
      uint32_t tid=_rxData[_current][0];
      //std::cout<<"Received Register"<<std::endl;
      if(tid!=_tid){
	printf ("Bad tid\n");
      }
      _status=_rxData[_current][3];
      _data=_rxData[_current][2];
      //std::cout<<"Value = "<<_data<<std::endl;
      std::unique_lock<std::mutex> pl( _data_mutex );
      _data_cond.notify_one();
    } else if (axisDest==0){ // data
      //std::cout<<"Data"<<std::endl;
      //std::cout<<"Header: "<<std::hex<<_rxData[0]<<std::dec<<std::endl;
      //for(int i=0;i<4;i++)std::cout<<std::hex<<_rxData[i]<<std::dec<<std::endl;
      if(_receiver!=NULL && _blockread==false){
	PgpData pgpdata;
	pgpdata.header=(unsigned char*)_rxData[_current];
	pgpdata.payload=&_rxData[_current][8];
	pgpdata.payloadSize=ret/sizeof(uint32_t) - headerSize;
        _receiver->receive(&pgpdata);
      }else{
	std::vector<uint32_t> rxdata;
	for(uint32_t i=0;i<ret/sizeof(uint32_t);i++)rxdata.push_back(_rxData[_current][i]);
	_buffers.push_back(rxdata);
      }
      if(_handshake){
	_handshake=false;
	std::unique_lock<std::mutex> pl( _data_mutex );
	_data_cond.notify_one();
      }
    } else if (axisDest==2){ // Atlas Event Fragment
      //  std::cout<<"Received Event Fragment:"<<std::endl;
            //for(int i=0;i<ret/sizeof(unsigned);i++){
	      //std::cout<<i<<": "<<std::hex<<_rxData[_current][i]<<std::dec<<std::endl;
            //}
      if(_receiver!=0){
	PgpData pgpdata;
	unsigned char header[32];
	header[2]=30; //TDCREADOUT
	pgpdata.header=header;
	pgpdata.payload=&_rxData[_current][0];
	pgpdata.payloadSize=ret/sizeof(uint32_t);
	_receiver->receive(&pgpdata);
      }
    }else{
      printf("Received message with dest = %d\n",axisDest);
      //assert(0);
      std::unique_lock<std::mutex> pl( _data_mutex );
      _data_cond.notify_one();
    }
    _current==15 ? _current=0: _current=_current+1;
  }
  
  uint32_t RCDImaster::nBuffers(){
    return _buffers.size();
  }

  int RCDImaster::currentBuffer(unsigned char*& header, uint32_t &headerSize, unsigned char*&payload, uint32_t &payloadSize){
    int retval=1;
    if(_buffers.empty()){
      header=0;
      headerSize=0;
      payload=0;
      payloadSize=0;
      retval=1;
    }else{
      std::vector<uint32_t> &data=*_buffers.begin();
      header=(unsigned char*)&data[0];
      headerSize=8*sizeof(uint32_t);
      payload=(unsigned char*)&data[8];
      payloadSize =(data.size()-8)*sizeof(uint32_t);
      retval=0;
    }
    return retval;
  }
   
  int RCDImaster::discardCurrentBuffer(){
    int retval=1;
    if(_buffers.empty()){
      retval=1;
    }else{
      _buffers.pop_front();
      retval=0;
    }
    return retval;
  }
  void RCDImaster::getOldData(int i, uint32_t *&data, int &size){
    int index=_current-i;
    if(index<0)index+=16;
    data=_rxData[index];
    size=_size[index];
  }
}




