#include <opencv2/opencv.hpp>
#include <iostream>
#include <array>
#include <vector>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include <opencv2/aruco.hpp>
#include "aruco_samples_utility.hpp"

 

struct CameraParams{
   int camId;
   std::array<int, 2> resolution;
   int FPS;

   CameraParams(int id, int width, int height): 
      camId(id), resolution({width, height}){
   }
};



void printInfo(const CameraParams& par){
   std::cout << "[LOG ]:" 
   << " Cam: " << par.camId 
   << " Resolution of the video: " << par.resolution[0] << " x " << par.resolution[1] 
   << " FPS : " << par.FPS << std::endl; 

}

cv::Mat setDist(std::array<double, 5> d){
   cv::Mat dist(cv::Size(5, 1), 6);

   for (int i=0; i<5;i++){
      dist.at<double>(i) = d[i];
   }

   return dist;
}

cv::Mat setCamMat(std::array<std::array<double, 3>, 3> mtx){
   cv::Mat camMatrix(cv::Size(3, 3), 6);

   for (int i=0; i<3;i++){
      cv::Mat buff; 
         for(int j=0; j<3; j++){
            camMatrix.at<double>(i,j) = mtx[i][j];
         }
   }

   return camMatrix;
}

bool readParams(std::string file, cv::Mat &mtx, cv::Mat &dist){
   bool success = readCameraParameters(file, mtx, dist);
   return success;
}

class Camera{
public:
   std::string winname;
   cv::VideoCapture cap;
   bool isWorking;

   Camera(CameraParams params):
      cap(params.camId), isWorking(false){
      cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'));

      if (cap.isOpened() == false) {
         std::cout << "[LOG ]: Cannot open the video camera" << std::endl;
         return;
      }

      isWorking = true;

      cap.set(cv::CAP_PROP_FRAME_WIDTH, params.resolution[0]);
      cap.set(cv::CAP_PROP_FRAME_HEIGHT, params.resolution[1]);

   }

   cv::Mat getFrame(){
      cv::Mat frame;
      isWorking = cap.read(frame); 

      if (isWorking == false) {
         std::cout << "[LOG ]: Video camera is disconnected" << std::endl;
      }

      return frame;
   }

   CameraParams getCameraInfo(){
      CameraParams par(0, 0, 0);
      par.resolution[0] = cap.get(cv::CAP_PROP_FRAME_WIDTH);
      par.resolution[1] = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
      par.FPS = cap.get(cv::CAP_PROP_FPS);

      return par; 
   }

};

struct Marks{
   std::vector<int> ids;
   std::vector<std::vector<cv::Point2f>> corners;
   std::vector<cv::Vec3d> rvecs;
   std::vector<cv::Vec3d> tvecs;

   
   void detectMarks(cv::Mat frame){
      std::vector<int> markerIds;
      std::vector<std::vector<cv::Point2f>> markerCorners;

      cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
      cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_250);

      cv::aruco::detectMarkers(frame, dictionary, markerCorners, markerIds, parameters);

      ids = markerIds;
      corners = markerCorners;
   }

   cv::Mat drawMarks(cv::Mat frame){
      cv::aruco::drawDetectedMarkers(frame, corners, ids);

      return frame;      
   }

   void getVectors(cv::Mat cameraMatrix, cv::Mat distCoeffs){
      cv::aruco::estimatePoseSingleMarkers(corners, 0.22, cameraMatrix, distCoeffs, rvecs, tvecs);
   }

   cv::Mat drawAxis(cv::Mat frame, cv::Mat cameraMatrix, cv::Mat distCoeffs){
      cv::Mat img = frame.clone();
      for (int i=0; i < rvecs.size(); i++){
         cv::drawFrameAxes(img, cameraMatrix, distCoeffs, rvecs[i], tvecs[i], 0.11);
      }
      return img;
   }
   
   std::vector<cv::Point3d> getCameraPose(){
      std::vector<cv::Point3d> poses(ids.size());

      for(int k=0; k<ids.size();k++){   
      cv::Mat rotMat;
      cv::Mat transMat = cv::Mat::zeros(4, 4, CV_64F);
      cv::Rodrigues(rvecs[k], rotMat);

      

      for(int i=0;i<3;i++){
         for(int j=0;j<3;j++){
            transMat.at<double>(i,j)=rotMat.at<double>(i,j);
         }
      } 

      transMat.at<double>(0,3) = tvecs[k](0);
      transMat.at<double>(1,3) = tvecs[k](1);
      transMat.at<double>(2,3) = tvecs[k](2);
      transMat.at<double>(3,3) = 1.;
      
      std::cout<<tvecs[k]<<std::endl;
      std::cout<<transMat.inv()<<std::endl;

      cv::Mat invMat = transMat.inv();
      poses[k] = cv::Point3d(invMat.at<double>(0,3), invMat.at<double>(1,3), invMat.at<double>(2,3));

   }   
      return poses;
   }
};



int main(int argc, char* argv[]){
   CameraParams par(0, 1024, 768);
   Camera camera(par);

   printInfo(par);
   
   while (camera.isWorking){
      cv::Mat frame = camera.getFrame();

      par = camera.getCameraInfo();

      printInfo(par);

      cv::imshow(camera.winname, frame);

      if (cv::waitKey(10) == 27){
         std::cout << "[LOG ]: Esc key is pressed by user. Stopping the video" << std::endl;
         break;
      }
   }

   cv::Mat img = cv::imread("/home/nanzat/aruco_images/(115;75;220).jpg");
   
   Marks detected;
   detected.detectMarks(img);

   cv::imshow(" ", img);
   cv::waitKey(0);
   cv::destroyWindow(" ");


   cv::imshow("detected markers", detected.drawMarks(img));
   cv::waitKey(0);
   cv::destroyWindow("detected markers");

   cv::Mat cameraMatrix, distCoeffs;
   std::string path = "camera_params.yml";

   if(!readCameraParameters(path, cameraMatrix, distCoeffs)){
      return 1;
   }

   detected.getVectors(cameraMatrix, distCoeffs);

   cv::imshow("axis", detected.drawAxis(img, cameraMatrix, distCoeffs));
   cv::waitKey(0);
   cv::destroyWindow("axis");


   std::vector<cv::Point3d> poses = detected.getCameraPose();
   for(int i =0; i<4;i++){
      std::cout<<detected.ids[i]<<std::endl<<poses[i]<<std::endl;
   }
   cv::imshow("detected markers", detected.drawMarks(img));
   cv::waitKey(0);
   cv::destroyWindow("detected markers");

   return 0;
}
