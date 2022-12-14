#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include "aruco_samples_utility.hpp"

 

void calibrate(std::string folder, std::string file){
   int ChessBoard[2]{6,9};

   std::vector<std::vector<cv::Point3f> > objpoints;
   std::vector<std::vector<cv::Point2f> > imgpoints;
   std::vector<cv::Point3f> objp;

   for(int i{0}; i<ChessBoard[1]; i++){
      for(int j{0}; j<ChessBoard[0]; j++){
         objp.push_back(cv::Point3f(j,i,0));
      }   
   }

   std::vector<cv::String> images;

   std::string path = folder + "*.jpg";
   cv::glob(path, images);

   cv::Mat frame, gray;

   std::vector<cv::Point2f> corners;

   
   for(int i{0}; i<images.size(); i++)
   {
      frame = cv::imread(images[i]);
      cv::cvtColor(frame,gray,cv::COLOR_BGR2GRAY);
   
      if(bool success = cv::findChessboardCorners(gray, cv::Size(ChessBoard[0], ChessBoard[1]), 
      corners, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE)){
         cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);
         
         cv::cornerSubPix(gray,corners,cv::Size(11,11), cv::Size(-1,-1),criteria);
         
         cv::drawChessboardCorners(frame, cv::Size(ChessBoard[0], ChessBoard[1]), corners, success);
         
         objpoints.push_back(objp);
         imgpoints.push_back(corners);
      }
      cv::imshow("Image",frame);
      cv::waitKey(1);
   }
   
   cv::destroyAllWindows();
   
   cv::Mat cameraMatrix,distCoeffs,rvec,tvec;
   

   double repError = cv::calibrateCamera(objpoints, imgpoints, cv::Size(gray.rows,gray.cols), 
                                          cameraMatrix, distCoeffs, rvec, tvec);
   
   std::string outputFile = file + (std::string)".yml";

   bool success = saveCameraParams(outputFile, gray.size(), 1, 0, cameraMatrix,
                                   distCoeffs, repError);

   if(!success) {
      std::cout << "[LOG ]: Cannot save output file" << std::endl;
   }
   else{
      std::cout << "[LOG ]: Rep Error: " << repError << std::endl;
      std::cout << "[LOG ]: Calibration saved to " << outputFile << std::endl;
   }
}




int main(int argc, char* argv[]){
   
   calibrate("/home/nanzat/images/", "cam");

   return 0;
}
