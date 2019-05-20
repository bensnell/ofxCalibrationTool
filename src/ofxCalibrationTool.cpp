#include "ofxCalibrationTool.h"

// --------------------------------------------------------------
ofxCalibrationTool::ofxCalibrationTool() {

}

// --------------------------------------------------------------
ofxCalibrationTool::~ofxCalibrationTool() {

}

// --------------------------------------------------------------
void ofxCalibrationTool::setupParams() {

	RUI_NEW_GROUP("Calibration Tool");


}

// --------------------------------------------------------------
void ofxCalibrationTool::setup() {



}

// --------------------------------------------------------------
void ofxCalibrationTool::update(glm::vec3 _tcp) {
	tcp = _tcp;
	lastObservationTime = ofGetElapsedTimef();
}

// --------------------------------------------------------------
void ofxCalibrationTool::loadTargetFile(string _vtfFilename) {
	
	if (_vtfFilename.empty()) return;
	vtfFilename = _vtfFilename;




}

// --------------------------------------------------------------
void ofxCalibrationTool::beginCalibration() {
	if (bCalibrating) {
		// Do you want to reset calibration?
	}



}

// --------------------------------------------------------------
void ofxCalibrationTool::lockTarget() {


}

// --------------------------------------------------------------
void ofxCalibrationTool::drawStatus() {
	if (!bCalibrating) return;

	// Show the status of the calibration protocol


}

// --------------------------------------------------------------
bool ofxCalibrationTool::isCalibrating() {
	return bCalibrating;
}

// --------------------------------------------------------------


// --------------------------------------------------------------


// --------------------------------------------------------------


// --------------------------------------------------------------
