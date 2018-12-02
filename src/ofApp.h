#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxOsc.h"
#include "ofxHeatMap.h"

#include "saliencySpecializedClasses.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "iostream"
#include "sstream"
#include "vector"
#include "algorithm"

#include "constTools.h"

#define MACWIDTH 1024
#define MACHEIGHT 768

#define WINWIDTH 1920
#define WINHEIGHT 1080

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	void createSaliencyMap(cv::Mat img);
	void createWatershed(cv::Mat saliencyImg);

	void loadEyeGaze(bool path);

	ConstTools::InputFileName inputFileName;
	ConstTools::InputIPUFileName inputIPUFileName;
	ConstTools::InputMockFileName inputMockFileName;
	ConstTools::OutputFileName_Picture outputfileNamePic;
	ConstTools::OutputFileName_EyeGaze outputfileNameEye;

	ofImage inputOfImg, loadOfImage;

	struct OriginalMat {
		cv::Mat original, copy;
	};
	OriginalMat originalMatPicture, originalMatEyeGaze;

	ConstTools::OutputOfImg outputOfPicIMG, outputOfEyeIMG;

	struct ViewMat
	{
		cv::Mat saliencyMap, saliencyMapColor;
		cv::Mat watershedHighest, saliencyHighest;
		cv::Mat mat_mix;
	};
	ViewMat viewMatSaliency, viewMatEyeGaze;

	std::vector<cv::Vec3b> colorTabSaliency, colorTabEyeGaze;

	cv::Mat picMarkersSave, eyeMarkersSave;

	cv::Mat picImgG, eyeImgG;

	//    std::vector<int> pixelsList;

	struct SaliencyPoint
	{
		std::vector<int> save, backup;
	};
	SaliencyPoint saliencyPointPicture, saliencyPointEyeGaze;

	struct MaxSaliencyPoint
	{
		std::vector<int>::iterator iter;
		int maxIndex;
	};
	MaxSaliencyPoint maxSaliencyPointPicture, maxSaliencyPointEyeGaze;

	ConstTools::EnterState enterState;
	int enterCountPicture, enterCountEyeGaze;

	std::stringstream enterCountStringPicture, enterCountStringEyeGaze;

	ConstTools::Mode mode;

	ofxOscReceiver receiver;
	float remoteEyeGazeX, remoteEyeGazeY;

	void dumpOSC(ofxOscMessage m);

	cv::Mat eyeGazeMat;
	ConstTools::EyeTrackState eyeTrackState;
	ConstTools::LoadState loadState;

	ofxHeatMap heatmap;

	ConstTools::Infomation infomation;

	ConstTools::PrefixPath prefixPath;

	std::string fileName;
	std::string eyeGazePath = prefixPath.eyeGaze + "/" + outputfileNameEye.outputOfEyeGazeHeatMapImg + fileNameExtension.pngPath;

	ConstTools::FileNameExtension fileNameExtension;
};
