//
//  ofxEtherdream.h
//  ofxILDA
//
//  Created by Daito Manabe + Yusuke Tomoto (Rhizomatiks)
//  Mods by Memo Akten
//
//
#pragma once

#include "ofMain.h"
#include "etherdream.h"
#include "ofxIldaFrame.h"

class ofxEtherdream : public ofThread {
public:
    ofxEtherdream():state(ETHERDREAM_NOTFOUND), bAutoConnect(false), dacIdEtherdream(0), device(NULL) {}
    
    ~ofxEtherdream() {
        kill();
    }
    
    bool stateIsFound();
    
    void kill() {
        clear();
        if (isThreadRunning()) {
            ofSleepMillis(200);
            stop();
        }
        if(stateIsFound()) {
            etherdream_stop(device);
            etherdream_disconnect(device);
        }
    }
    
    void setup(bool bStartThread = true, int idEtherdream = 0);
    void setupByDacId(unsigned long dacIdEtherdream, bool bStartThread = true);
    virtual void threadedFunction();
    
    
    // check if the device has shutdown (weird bug in etherdream driver) and reconnect if nessecary
    bool checkConnection(bool bForceReconnect = true);
    
    void clear();
    void start();
    void stop();

    void addPoints(const vector<ofxIlda::Point>& _points);
    void addPoints(const ofxIlda::Frame &ildaFrame);
    
    void setPoints(const vector<ofxIlda::Point>& _points);
    void setPoints(const ofxIlda::Frame &ildaFrame);
    
    void send();
    
    void setPPS(int i);
    int getPPS() const;
    
    void setWaitBeforeSend(bool b);
    bool getWaitBeforeSend() const;
    
    static vector<unsigned long> listDevices();
    
    bool isAutoConnect() const { return bAutoConnect; }
    void setAutoConnect(bool b) { bAutoConnect = b; }
    
    void setSyncFunction(void (*sync_function_ptr)(void)) {
        if (device && checkConnection(false)) {
            etherdream_set_sync_function_ptr(device, sync_function_ptr);
        }
    }
private:
    void init();
    
public:
    enum EtherDreamState {
        ETHERDREAM_NOTFOUND = 0,
        ETHERDREAM_FOUND,
        ETHERDREAM_FOUND_DISCONNECTED
    };
    
private:
    EtherDreamState state;
    int pps;
    bool bWaitBeforeSend;
    bool bAutoConnect;
    bool bSetupByDacId;
    
    struct etherdream *device;
    vector<ofxIlda::Point> points;
    
    int idEtherdreamConnection;
    unsigned long dacIdEtherdream;
public:
    EtherDreamState getState() const { return state; }
    string getStateString() const;
};
