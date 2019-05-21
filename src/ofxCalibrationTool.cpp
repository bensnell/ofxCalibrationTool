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
	RUI_SHARE_PARAM_WCN("Cal: Target Plan Filename", tpFilename);
	string names[] = { "Manual", "Automated" };
	RUI_SHARE_ENUM_PARAM_WCN("Cal: Calibration Mode", mode, CALIBRATION_MANUAL, CALIBRATION_AUTOMATED, names);


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
void ofxCalibrationTool::loadTargetPlan(string _tpFilename) {
	
	if (!_tpFilename.empty()) {
		tpFilename = _tpFilename;
		RUI_PUSH_TO_CLIENT();
	}

	// Validate the file
	ofFile file(ofToDataPath(tpFilename));
	if (!file.exists()) {
		ofLogError("ofxCalibrationTool") << "Target Plan file " << tpFilename << " does not exist. Exiting";
		return;
	}
	file >> js;
	if (!js.contains("calibration_targets")) {
		ofLogError("ofxCalibrationTool") << "Target Plan " << tpFilename << " does not contain key " << "calibration_targets. Exiting.";
		return;
	}
	if (!js.contains("calibration_square")) {
		ofLogError("ofxCalibrationTool") << "Target Plan " << tpFilename << " does not contain key " << "calibration_square. Exiting.";
		return;
	}
	if (js["calibration_targets"].size() < 3) {
		ofLogError("ofxCalibrationTool") << "There must be at least three calibration targets in the Target Plan file. Exiting.";
		return;
	}
	for (int i = 0; i < js["calibration_targets"].size(); i++) {
		if (js["calibration_targets"][i].size() != 3) {
			ofLogError("ofxCalibrationTool") << "Target Plan targets must have (x, y, z) coordinates. Exiting";
			return;
		}
	}
	if (js["calibration_square"].size() != 3) {
		ofLogError("ofxCalibrationTool") << "Calibration square indices size should be 3, not " << js["calibration_square"].size() << ". Exiting.";
		return;
	}

	// Load the file's contents
	vector<glm::vec3> _targets;
	vector<int> _square;
	// Load the targets
	for (int i = 0; i < js["calibration_targets"].size(); i++) {
		_targets.push_back(glm::vec3(
			js["calibration_targets"][i][0],
			js["calibration_targets"][i][1],
			js["calibration_targets"][i][2]));
	}
	// Load the square
	for (int i = 0; i < 3; i++) {
		int index = js["calibration_square"][i];
		if (index < 0 || index >= _targets.size()) {
			ofLogError("ofxCalibrationTool") << "Target Plan file is invalid. Square index is incorrect. Exiting";
			return;
		}
		_square.push_back(index);
	}

	ofLogNotice("ofxCalibrationTool") << "Successfully loaded Target Plan file \"" << tpFilename << "\"";

	// Save these targets if all is successful
	realTargets = _targets;
	square = _square;
}

// --------------------------------------------------------------
void ofxCalibrationTool::beginCalibrationProtocol() {
	if (bCalibrating) {
		// Do you want to reset calibration?
		ofLogNotice("ofxCalibrationTool") << "Already calibrating. Call reset() to restart calibration";
		return;
	}
	if (!isTargetPlanLoaded()) {
		ofLogNotice("ofxCalibrationTool") << "Please load the Target File before calibrating";
		return;
	}
	bCalibrating = true;

	resetCalibrationProtocol();
}

// --------------------------------------------------------------
void ofxCalibrationTool::resetCalibrationProtocol() {

	// Begin with the first target
	currentTargetIndex = 0;

	// Clear the raw targets
	rawTargets.clear();
}

// --------------------------------------------------------------
void ofxCalibrationTool::lockTarget() {

	// Save the last observed location as the corresponding 
	rawTargets.push_back(tcp);

	// Progress to the next calibration target
	currentTargetIndex++;
	if (currentTargetIndex >= realTargets.size()) {
		// We're done, so stop calibrating
		bCalibrating = false;

		// Calculate the transformation matrix
		glm::mat4x4 tmp;






		transMat = tmp;

		// Export this matrix as a calibration file
		vector< vector<float> > mat;
		for (int row = 0; row < 4; row++) {
			vector<float> matRow;
			for (int col = 0; col < 4; col++) {
				matRow.push_back(transMat[row][col]);
			}
			mat.push_back(matRow);
		}
		ofJson cal;
		cal["calibration_transformation_matrix"] = mat; // is this possible?
		ofSaveJson(ofToDataPath(calFilename), cal);

		ofLogNotice("ofxCalibrationTool") << "Saved calibration file " << calFilename;
	}
}

// --------------------------------------------------------------
void ofxCalibrationTool::drawStatus(int x, int y) {
	if (!bCalibrating) return;

	stringstream ss;
	ss << std::fixed;
	ss << std::setprecision(3);

	// What is the calibration progress?
	ss << "Progress:\t[";
	for (int i = 0; i < currentTargetIndex; i++) ss << "===";
	for (int i = currentTargetIndex; i < realTargets.size(); i++) ss << "   ";
	ss << "]\t" << currentTargetIndex << " / " << realTargets.size() << "\n";

	// What is the calibration format? automated, manual
	ss << "Format:\t";
	switch (mode) {
	case CALIBRATION_MANUAL: ss << "Manual"; break;
	case CALIBRATION_AUTOMATED: ss << "Automated"; break;
	default: ss << "Unknown"; break;
	}
	ss << "\n";

	ss << "---\n";

	// Show the current position
	ss << "Tracker Position: (" << tcp.x << ", " << tcp.y << ", " << tcp.z << ")\n";

	ss << "---\n";

	// Prompt the user to align to the current target?
	// What is the current target index, location?
	ss << "Current Task:\n";
	ss << "Align the tracker to\n";
	ss << "\tTarget # " << currentTargetIndex << "\n";
	ss << "\tPosition: (" << realTargets[currentTargetIndex].x << ", " << realTargets[currentTargetIndex].y << ", " << realTargets[currentTargetIndex].z << ")\n";

	// What is the steadiness progression if the format is automated?
	if (mode == CALIBRATION_AUTOMATED) {
		ss << "\tAutomated Progress: [ ]"; // TODO


	}

	// Draw this string to screen
	ofDrawBitmapStringHighlight(ss.str(), x, y);
}

// --------------------------------------------------------------
bool ofxCalibrationTool::isCalibrating() {
	return bCalibrating;
}

// --------------------------------------------------------------
bool ofxCalibrationTool::isTargetPlanLoaded() {

	return !realTargets.empty() && !square.empty();
}

// --------------------------------------------------------------


// --------------------------------------------------------------


// --------------------------------------------------------------
