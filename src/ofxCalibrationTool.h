#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofRTLS.h"

class ofxCalibrationTool {
public:

	/// \brief Create an object to connect with motive's cameras. There should only be one per program.
	ofxCalibrationTool();
	~ofxCalibrationTool();


private:

	ofxRTLS tracker;


};