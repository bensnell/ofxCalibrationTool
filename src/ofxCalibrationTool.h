#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"

// The target file for this calibration protcol is a json document 
// with the following elements:
// (1) key "calibration_targets" to a list of at least 3 positions with physical
// space (x, y, z) coordinates in meters. Usually, these targets are on the floor.
// (2) key "calibration_square" to a list of three unique indices {1,2,3} of the preceding 
// targets. Using the right hand rule, vector{[1]-[0]}.cross(vector{[2]-[0]}).norm() is the 
// z direction. This entry is needed because otherwise, there is not enough information to
// know which way is up

// This tool transforms virtual raw coordinates to virtual real coordinates
class ofxCalibrationTool {
public:

	ofxCalibrationTool();
	~ofxCalibrationTool();

	void setupParams();

	void setup();

	// Update this tool with the tool center point used for calibration
	void update(glm::vec3 _tcp);

	void loadTargetFile(string _vtfFilename = "");

	// Begin the calibration process
	void beginCalibration();
	bool isCalibrating();
	void lockTarget();

	void drawStatus();

	void clearCalibration();
	void saveCalibration();

	// -----

	void loadCalibration(string scfFilename = "");
	ofNode transformVirtualRawToReal(ofNode& from);


private:

	bool bCalibrating = false;

	string vtfFilename = "targets.vtf"; // virtual target file (json)

	// Last tcp observed
	glm::vec3 tcp;
	float lastObservationTime = -1;

	//bool bLockOnStatic = false;
	//float lockTime = 10;
	//float lockDeviance = 0.02; // meters 

};