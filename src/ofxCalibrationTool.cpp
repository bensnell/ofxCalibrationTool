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
	RUI_SHARE_PARAM_WCN("Cal: Calibration Filename", calFilename);

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
		calculateTransformation(rawTargets, realTargets, transMat);

		// Export this matrix as a calibration file
		saveCalibration(ofToDataPath(calFilename), transMat);

		ofLogNotice("ofxCalibrationTool") << "Saved calibration file " << calFilename;
	}
}

// --------------------------------------------------------------
void ofxCalibrationTool::saveCalibration(string _filepath, glm::mat4x4& _mat) {

	// Convert to vectors for easier saving
	vector< vector<float> > mat;
	for (int row = 0; row < 4; row++) {
		vector<float> matRow;
		for (int col = 0; col < 4; col++) {
			matRow.push_back(_mat[row][col]);
		}
		mat.push_back(matRow);
	}
	ofJson cal;
	cal["calibration_transformation_matrix"] = mat; // is this possible?
	ofSaveJson(ofToDataPath(_filepath), cal);
}

// --------------------------------------------------------------
void ofxCalibrationTool::loadCalibrationFile(string _calFilename) {
	if (!_calFilename.empty()) {
		calFilename = _calFilename;
		RUI_PUSH_TO_CLIENT();
	}
	if (calFilename.empty()) {
		ofLogNotice("ofxCalibrationTool") << "Cannot load missing calibration file. Exiting";
		return;
	}

	// Validate the file
	ofFile file(ofToDataPath(calFilename));
	if (!file.exists()) {
		ofLogError("ofxCalibrationTool") << "Calibration file " << calFilename << " does not exist. Exiting";
		return;
	}
	file >> js;
	if (!js.contains("calibration_transformation_matrix")) {
		ofLogError("ofxCalibrationTool") << "Calibration file " << calFilename << " does not contain key " << "calibration_transformation_matrix. Exiting.";
		return;
	}
	if (js["calibration_transformation_matrix"].size() != 4) {
		ofLogError("ofxCalibrationTool") << "Calibration File must contain valid transformation matrix.";
		return;
	}
	for (int i = 0; i < 4; i++) {
		if (js["calibration_transformation_matrix"][i].size() != 4) {
			ofLogError("ofxCalibrationTool") << "Calibration File must contain valid transformation matrix.";
			return;
		}
	}

	// Load the file's contents
	glm::mat4x4 tmp;
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			tmp[row][col] = js["calibration_transformation_matrix"][row][col];
		}
	}
	// TODO: Should we check if it's valid?

	ofLogNotice("ofxCalibrationTool") << "Successfully loaded Calibration file \"" << calFilename << "\"";

	// Save the matrix
	transMat = tmp;
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
glm::mat4x4 ofxCalibrationTool::getMapped(glm::mat4x4 in) {
	return applyTransformation(in, transMat);
}

// --------------------------------------------------------------
void ofxCalibrationTool::map(glm::mat4x4& inOut) {
	glm::mat4x4 tmp = applyTransformation(inOut, transMat);
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			inOut[i][j] = tmp[i][j];
		}
	}
}

// --------------------------------------------------------------
bool ofxCalibrationTool::isCalibrating() {

	return bCalibrating;
}

// --------------------------------------------------------------
bool ofxCalibrationTool::isTargetPlanLoaded() {

	return !realTargets.empty() && !square.empty();
}

//--------------------------------------------------------------
cv::Vec3d ofxCalibrationTool::CalculateMean(const cv::Mat_<cv::Vec3d>& points) {
	cv::Mat_<cv::Vec3d> result;
	cv::reduce(points, result, 0, CV_REDUCE_AVG);
	return result(0, 0);
}

//--------------------------------------------------------------
cv::Mat_<double> ofxCalibrationTool::FindRigidTransform(const cv::Mat_<cv::Vec3d>& points1, const cv::Mat_<cv::Vec3d>& points2) {
	// TODO: account for reflections?
	
	/* Calculate centroids. */
	cv::Vec3d t1 = -CalculateMean(points1);
	cv::Vec3d t2 = -CalculateMean(points2);

	cv::Mat_<double> T1 = cv::Mat_<double>::eye(4, 4);
	T1(0, 3) = t1[0];
	T1(1, 3) = t1[1];
	T1(2, 3) = t1[2];

	cv::Mat_<double> T2 = cv::Mat_<double>::eye(4, 4);
	T2(0, 3) = -t2[0];
	T2(1, 3) = -t2[1];
	T2(2, 3) = -t2[2];

	/* Calculate covariance matrix for input points. Also calculate RMS deviation from centroid
	 * which is used for scale calculation.
	 */
	cv::Mat_<double> C(3, 3, 0.0);
	double p1Rms = 0, p2Rms = 0;
	for (int ptIdx = 0; ptIdx < points1.rows; ptIdx++) {
		cv::Vec3d p1 = points1(ptIdx, 0) + t1;
		cv::Vec3d p2 = points2(ptIdx, 0) + t2;
		p1Rms += p1.dot(p1);
		p2Rms += p2.dot(p2);
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				C(i, j) += p2[i] * p1[j];
			}
		}
	}

	cv::Mat_<double> u, s, vh;
	cv::SVD::compute(C, s, u, vh);

	cv::Mat_<double> R = u * vh;

	if (cv::determinant(R) < 0) {
		R -= u.col(2) * (vh.row(2) * 2.0);
	}

	double scale = sqrt(p2Rms / p1Rms);
	R *= scale;

	cv::Mat_<double> M = cv::Mat_<double>::eye(4, 4);
	R.copyTo(M.colRange(0, 3).rowRange(0, 3));

	cv::Mat_<double> result = T2 * M * T1;
	result /= result(3, 3);

	return result.rowRange(0, 3);
}

//--------------------------------------------------------------
bool ofxCalibrationTool::calculateTransformation(vector<glm::vec3> & testPoints, vector<glm::vec3> & refPoints, glm::mat4x4 & outTransMat) {
	if (testPoints.size() != refPoints.size()) return false;
	if (testPoints.empty() || refPoints.empty()) return false;

	int nPoints = testPoints.size();

	// Convert the input set of points into the type this algorithm prefers

	// This is the set of input points (the raw data whose transformation we want to know)
	cv::Mat_<cv::Vec3d> from = cv::Mat_<cv::Vec3d>::zeros(nPoints, 1);
	for (int i = 0; i < nPoints; i++) {
		from.at<cv::Vec3d>(i)[0] = testPoints[i].x;
		from.at<cv::Vec3d>(i)[1] = testPoints[i].y;
		from.at<cv::Vec3d>(i)[2] = testPoints[i].z;
	}

	// This is the set of reference points (the real data we want to match as best we can)
	cv::Mat_<cv::Vec3d> to = cv::Mat_<cv::Vec3d>::zeros(nPoints, 1);
	for (int i = 0; i < nPoints; i++) {
		to.at<cv::Vec3d>(i)[0] = refPoints[i].x;
		to.at<cv::Vec3d>(i)[1] = refPoints[i].y;
		to.at<cv::Vec3d>(i)[2] = refPoints[i].z;
	}

	// Compute the rigid transformation
	cv::Mat_<double> out = FindRigidTransform(from, to);

	// Create the 4x4 transformation matrix
	cv::Mat_<double> lastRow = cv::Mat_<double>::zeros(1, 4);
	lastRow.at<double>(0, 3) = 1;
	out.push_back(lastRow);

	// Create the matrix in glm form (apply transpose in the process)
	// and set the output
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			outTransMat[col][row] = out.at<double>(row, col);
		}
	}

	return true;
}

//--------------------------------------------------------------
glm::mat4x4 ofxCalibrationTool::applyTransformation(glm::mat4x4 & frame, glm::mat4x4 & transMat) {

	return transMat * frame;
}

//--------------------------------------------------------------
glm::vec3 ofxCalibrationTool::applyTransformation(glm::vec3 & point, glm::mat4x4 & transMat) {

	glm::mat4x4 transFrame = transMat * glm::translate(point);
	return glm::vec3(transFrame[3][0],
		transFrame[3][1],
		transFrame[3][2]);
}

// --------------------------------------------------------------


// --------------------------------------------------------------


// --------------------------------------------------------------
