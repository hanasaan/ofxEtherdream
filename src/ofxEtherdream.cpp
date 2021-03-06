#include "ofxEtherdream.h"

//--------------------------------------------------------------
void ofxEtherdream::setup(bool bStartThread, int idEtherdream) {

    bSetupByDacId = false;
    idEtherdreamConnection = idEtherdream;
    this->dacIdEtherdream = 0;
    
    etherdream_lib_start();
    
    setPPS(30000);
    setWaitBeforeSend(false);
    
	/* Sleep for a bit over a second, to ensure that we see broadcasts
	 * from all available DACs. */
	usleep(1000000);
    
    init();
    
    if(bStartThread) start();
}

//--------------------------------------------------------------
void ofxEtherdream::setupByDacId(unsigned long dacIdEtherdream, bool bStartThread, bool bConnect, int beforeSleepUsec) {
    
    bSetupByDacId = true;
    idEtherdreamConnection = INT_MAX;
    this->dacIdEtherdream = dacIdEtherdream;
    
    etherdream_lib_start();
    
    setPPS(30000);
    setWaitBeforeSend(false);
    
    /* Sleep for a bit over a second, to ensure that we see broadcasts
     * from all available DACs. */
    usleep(beforeSleepUsec);

    {
        int device_num = etherdream_dac_count();
        if (!device_num) {
            ofLogWarning() << "ofxEtherdream::init - No DACs found";
            return;
        }
        
        device = etherdream_get(dacIdEtherdream);
        if (device == NULL) return;
        if (bConnect) {
            ofLogNotice() << "ofxEtherdream::init - Connecting...";
            
            if (etherdream_connect(device) < 0) return;
            
            ofLogNotice() << "ofxEtherdream::init - done with dac id : " << dacIdEtherdream;
            state = ETHERDREAM_FOUND;
        } else {
            state = ETHERDREAM_FOUND_DISCONNECTED;
        }
    }
    
    if(bStartThread && bConnect) start();
}

void ofxEtherdream::resetup(bool bStartThread)
{
    if (device == NULL) {
        return;
    }
    if (etherdream_connect(device) < 0) return;
    
    ofLogNotice() << "ofxEtherdream::init - done with dac id : " << dacIdEtherdream;
    state = ETHERDREAM_FOUND;
    
    if(bStartThread) start();
}



//--------------------------------------------------------------
bool ofxEtherdream::stateIsFound() {
    return state == ETHERDREAM_FOUND;
}

//--------------------------------------------------------------
bool ofxEtherdream::checkConnection(bool bForceReconnect) {
    if(device->state == ST_SHUTDOWN || device->state == ST_BROKEN || device->state == ST_DISCONNECTED) {
        
        if(bForceReconnect) {
            kill();
            if (bSetupByDacId) {
                setupByDacId(dacIdEtherdream);
            } else {
                setup(true, idEtherdreamConnection);
            }
        }
        if (state == ETHERDREAM_FOUND) {
            if(lock()) {
                state = ETHERDREAM_FOUND_DISCONNECTED;
                unlock();
            }
        }
        return false;
    }
    return true;
}

//--------------------------------------------------------------
void ofxEtherdream::init() {
    int device_num = etherdream_dac_count();
	if (!device_num || idEtherdreamConnection>=device_num) {
		ofLogWarning() << "ofxEtherdream::init - No DACs found";
		return;
	}
    
    vector<unsigned long> dac_ids;
	for (int i=0; i<device_num; i++) {
		ofLogNotice() << "ofxEtherdream::init - " << i << " Ether Dream " << etherdream_get_id(etherdream_get(i));
        dac_ids.push_back(etherdream_get_id(etherdream_get(i)));
    }
    std::sort(dac_ids.begin(),dac_ids.end());
    
    device = etherdream_get(dac_ids[idEtherdreamConnection]);
    
    ofLogNotice() << "ofxEtherdream::init - Connecting...";
    if (etherdream_connect(device) < 0) return;

    ofLogNotice() << "ofxEtherdream::init - done with dac id : " << dac_ids[idEtherdreamConnection];
    
    state = ETHERDREAM_FOUND;
}

//--------------------------------------------------------------
void ofxEtherdream::threadedFunction() {
    while (isThreadRunning() != 0) {
        
        switch (state) {
            case ETHERDREAM_NOTFOUND:
                if(bAutoConnect) init();
                break;
                
            case ETHERDREAM_FOUND:
                if(lock()) {
                    send();
                    unlock();
                }
                break;
            case ETHERDREAM_FOUND_DISCONNECTED:
                if(bAutoConnect) {
                    clear();
                    if (bSetupByDacId) {
                        setupByDacId(dacIdEtherdream, false);
                    } else {
                        setup(false, idEtherdreamConnection);
                    }
                }
                break;
                
        }
        ofSleepMillis(1);
    }
}

//--------------------------------------------------------------
void ofxEtherdream::start() {
    startThread();  // TODO: blocking or nonblocking?
}

//--------------------------------------------------------------
void ofxEtherdream::stop() {
    stopThread();
}

//--------------------------------------------------------------
void ofxEtherdream::send() {
    if(!stateIsFound() || points.empty()) return;
    
    if(bWaitBeforeSend) etherdream_wait_for_ready(device);
    else if(!etherdream_is_ready(device)) return;
    
    // DODGY HACK: casting ofxIlda::Point* to etherdream_point*
    int res = etherdream_write(device, (etherdream_point*)points.data(), points.size(), pps, 1);
    if (res != 0) {
        ofLogVerbose() << "ofxEtherdream::write " << res;
    }
    points.clear();
}


//--------------------------------------------------------------
void ofxEtherdream::clear() {
    if(lock()) {
        points.clear();
        unlock();
    }
}

//--------------------------------------------------------------
void ofxEtherdream::addPoints(const vector<ofxIlda::Point>& _points) {
    if(lock()) {
        if(!_points.empty()) {
            points.insert(points.end(), _points.begin(), _points.end());
        }
        unlock();
    }
}


//--------------------------------------------------------------
void ofxEtherdream::addPoints(const ofxIlda::Frame &ildaFrame) {
    addPoints(ildaFrame.getPoints());
}


//--------------------------------------------------------------
void ofxEtherdream::setPoints(const vector<ofxIlda::Point>& _points) {
    if (state == ETHERDREAM_FOUND) {
        if(lock()) {
            points = _points;
            unlock();
        }
    }
}


//--------------------------------------------------------------
void ofxEtherdream::setPoints(const ofxIlda::Frame &ildaFrame) {
    setPoints(ildaFrame.getPoints());
}

//--------------------------------------------------------------
void ofxEtherdream::setWaitBeforeSend(bool b) {
    if(lock()) {
        bWaitBeforeSend = b;
        unlock();
    }
}

//--------------------------------------------------------------
bool ofxEtherdream::getWaitBeforeSend() const {
    return bWaitBeforeSend;
}


//--------------------------------------------------------------
void ofxEtherdream::setPPS(int i) {
    if(lock()) {
        pps = i;
        unlock();
    }
}

//--------------------------------------------------------------
int ofxEtherdream::getPPS() const {
    return pps;
}

string ofxEtherdream::getStateString() const
{
    switch (state) {
        case ETHERDREAM_NOTFOUND:
            return "Not Found";
        case ETHERDREAM_FOUND:
            return "Found";
        case ETHERDREAM_FOUND_DISCONNECTED:
            return "Disconnected";
    }
    return "Error";
}


vector<unsigned long> ofxEtherdream::listDevices()
{
    etherdream_lib_start();
    ofSleepMillis(1000);
    int device_num = etherdream_dac_count();

    vector<unsigned long> dac_ids;
    for (int i=0; i<device_num; i++) {
        ofLogNotice() << "ofxEtherdream::init - " << i << " Ether Dream " << etherdream_get_id(etherdream_get(i));
        dac_ids.push_back(etherdream_get_id(etherdream_get(i)));
    }
    std::sort(dac_ids.begin(),dac_ids.end());

    return dac_ids;
}

dac_status ofxEtherdream::getDacStatus() const
{
    return device->conn.resp.dac_status;
}

