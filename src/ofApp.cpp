#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    // 画像の読み込み
    inputOfImg.load("sample.jpg");
    inputOfImg.update();

    // Mat変換
    cv::Mat mat, mat_gray, mat_gaus, saliencyMap_norm;

    // Mat画像に変換
    mat = ofxCv::toCv(inputOfImg);

    // 白黒Mat画像に変換
    cvtColor(mat.clone(), mat_gray, cv::COLOR_BGR2GRAY);
    // ぼかし
    cv::GaussianBlur(mat_gray.clone(), mat_gaus, cv::Size(5, 5), 1, 1);

    // 顕著性マップ(SPECTRAL_RESIDUAL)に変換(顕著性マップを求めるアルゴリズム)
    cv::Ptr<cv::saliency::Saliency> saliencyAlgorithm;
    saliencyAlgorithm = cv::saliency::StaticSaliencySpectralResidual::create();
    saliencyAlgorithm->computeSaliency(mat_gaus.clone(), saliencyMap_SPECTRAL_RESIDUAL);

    // アルファチャンネルの正規化を行う
    cv::normalize(saliencyMap_SPECTRAL_RESIDUAL.clone(), saliencyMap_norm, 0.0, 255.0, cv::NORM_MINMAX);
    // Matの型（ビット深度）を変換する
    saliencyMap_norm.convertTo(saliencyMap, CV_8UC3);

    // 最小と最大の要素値とそれらの位置を求める
    //    minMaxLoc(saliencyMap, &minMax.min_val, &minMax.max_val, &minMax.min_loc, &minMax.max_loc, cv::Mat());

    // 画像(ofImage)に変換
    ofxCv::toOf(saliencyMap.clone(), outputOfSaliencyImg);
    outputOfSaliencyImg.update();

    // 二値化
    cv::Mat thresh;
    cv::threshold(saliencyMap.clone(), thresh, 0, 255, cv::THRESH_OTSU);
//    cv::threshold(saliencyMap.clone(), thresh, 255/2, 255, CV_THRESH_BINARY);

    // ノイズ除去
    cv::Mat opening;
    cv::Mat kernel(3, 3, CV_8U, cv::Scalar(1));
    cv::morphologyEx(thresh.clone(), opening, cv::MORPH_OPEN, kernel, cv::Point(-1,-1), 2);

    // 背景領域抽出
    cv::Mat sure_bg;
    cv::dilate(opening.clone(), sure_bg, kernel, cv::Point(-1,-1), 3);

    // 前景領域抽出
    cv::Mat dist_transform;
    cv::distanceTransform(opening, dist_transform, CV_DIST_L2, 5);

    cv::Mat sure_fg;
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(dist_transform, &minVal, &maxVal, &minLoc, &maxLoc);
    cv::threshold(dist_transform, sure_fg, 0.3*maxVal, 255, 0);

    dist_transform = dist_transform/maxVal;

    // 不明領域抽出
    cv::Mat unknown, sure_fg_uc1;
    sure_fg.convertTo(sure_fg_uc1, CV_8UC1);
    cv::subtract(sure_bg, sure_fg_uc1, unknown);

    // 前景ラベリング
    int compCount = 0;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    sure_fg.convertTo(sure_fg, CV_32SC1, 1.0);
    cv::findContours(sure_fg, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);
    if( contours.empty() ) return;
    cv::Mat markers = cv::Mat::zeros(sure_fg.rows, sure_fg.cols, CV_32SC1);
    int idx = 0;
    for( ; idx >= 0; idx = hierarchy[idx][0], compCount++ )
        cv::drawContours(markers, contours, idx, cv::Scalar::all(compCount+1), -1, 8, hierarchy, INT_MAX);
    markers = markers+1;

    // 不明領域は今のところゼロ
    for(int i=0; i<markers.rows; i++){
        for(int j=0; j<markers.cols; j++){
            unsigned char &v = unknown.at<unsigned char>(i, j);
            if(v==255){
                markers.at<int>(i, j) = 0;
            }
        }
    }

    // 分水嶺
    cv::watershed( mat, markers );

    // 背景黒のMat画像
    dividA = cv::Mat::zeros(mat.size(), CV_8UC3);

    cv::Mat wshed(markers.size(), CV_8UC3);
    std::vector<cv::Vec3b> colorTab;
    for(int i = 0; i < compCount; i++ )
    {
        int b = cv::theRNG().uniform(0, 255);
        int g = cv::theRNG().uniform(0, 255);
        int r = cv::theRNG().uniform(0, 255);

        colorTab.push_back(cv::Vec3b((uchar)b, (uchar)g, (uchar)r));
    }

    // paint the watershed image
    for(int i = 0; i < markers.rows; i++ ){
        for(int j = 0; j < markers.cols; j++ )
        {
            int index = markers.at<int>(i,j);
            if( index == -1 ) {
                wshed.at<cv::Vec3b>(i,j) = cv::Vec3b(255,255,255);
            } else if( index <= 0 || index > compCount ) {
                wshed.at<cv::Vec3b>(i,j) = cv::Vec3b(0,0,0);
            }
            else if( index == 1 ) {
                wshed.at<cv::Vec3b>(i,j) = colorTab[index - 1];
            }
            else {
//                ofLogNotice()<<"index: "<<index;
                dividA.at<cv::Vec3b>(i,j) = colorTab[index - 1];
            }
        }
    }

    cv::Mat imgG;
    cvtColor(saliencyMap.clone(), imgG, cv::COLOR_GRAY2BGR);
    wshed = wshed*0.5 + imgG*0.5;

    cv::Mat imgG2;
    cvtColor(saliencyMap.clone(), imgG2, cv::COLOR_GRAY2BGR);
    dividA = dividA*0.5 + imgG2*0.5;

    // 画像(ofImage)に変換
//    ofxCv::toOf(wshed.clone(), outputOfImg);
    ofxCv::toOf(dividA.clone(), outputOfImg);

    outputOfImg.update();

    // 画素値の反転（現状 : 0:黒:顕著性が低い, 255:白:顕著性が高い）
    for(int y = 0; y < saliencyMap.cols; ++y){
        for(int x = 0; x < saliencyMap.rows; ++x){
            saliencyMap.at<uchar>( x, y ) = 255 - (int)saliencyMap.at<uchar>(x, y);
            //        ofLog()<<"(int)saliencyMap.at<uchar>("<<x<<","<<y<< ") : "<<(int)saliencyMap.at<uchar>( x, y );
        }
    }
    // ヒートマップへ変換 :（0:赤:顕著性が高い, 255:青:顕著性が低い）
    applyColorMap(saliencyMap.clone(), saliencyMap_color, cv::COLORMAP_JET);

    // 画像(ofImage)に変換
    ofxCv::toOf(saliencyMap_color.clone(), outputOfHeatMapImg);
    outputOfHeatMapImg.update();

}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){

    // 元画像
    inputOfImg.draw(0,0,ofGetWidth()/2, ofGetHeight()/2);
    // 顕著性マップ(SPECTRAL_RESIDUAL)を出力
    outputOfSaliencyImg.draw(ofGetWidth()/2,0,ofGetWidth()/2, ofGetHeight()/2);
    // 顕著性マップを出力
    outputOfHeatMapImg.draw(0,ofGetHeight()/2,ofGetWidth()/2, ofGetHeight()/2);
    // 顕著性マップのヒートマップを出力
    outputOfImg.draw(ofGetWidth()/2,ofGetHeight()/2,ofGetWidth()/2, ofGetHeight()/2);

    // Label
    ofDrawBitmapStringHighlight("original", 20, 20);
    ofDrawBitmapStringHighlight("saliencyMap", ofGetWidth()/2+20, 20);
    ofDrawBitmapStringHighlight("saliencyMap-heatMap", 20, ofGetHeight()/2+20);
    ofDrawBitmapStringHighlight("watershed", ofGetWidth()/2+20, ofGetHeight()/2+20);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
