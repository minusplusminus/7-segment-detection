//
//  skewAngle.cpp
//  7-segment
//
//  Created by Martijn Mellema on 22-10-14.
//  Copyright (c) 2014 Martijn Mellema. All rights reserved.
//

#include "skewAngle.h"


void skewAngle::hough_transform(Mat& im,Mat& orig,double* skew)
{
    double max_r=sqrt(pow(.5*im.cols,2)+pow(.5*im.rows,2));
    int angleBins = 180;
    Mat acc = Mat::zeros(Size(2*max_r,angleBins),CV_32SC1);
    int cenx = im.cols/2;
    int ceny = im.rows/2;
    for(int x=1;x<im.cols-1;x++)
    {
        for(int y=1;y<im.rows-1;y++)
        {
            if(im.at<uchar>(y,x)==255)
            {
                for(int t=0;t<angleBins;t++)
                {
                    double r =(x-cenx)*cos((double)t/angleBins*CV_PI)+(y-ceny)*sin((double)t    /angleBins*CV_PI);
                    r+=max_r;
                    acc.at<int>(t,int(r))++;
                }
            }
        }
    }
    Mat thresh;
    normalize(acc,acc,255,0,NORM_MINMAX);
    convertScaleAbs(acc,acc);
    /*debug
     Mat cmap;
     applyColorMap(acc,cmap,COLORMAP_JET);
     imshow("cmap",cmap);
     imshow("acc",acc);*/
    
    Point maxLoc;
    minMaxLoc(acc,0,0,0,&maxLoc);
    double theta = (double)maxLoc.y/angleBins*CV_PI;
    double rho = maxLoc.x-max_r;
    if(abs(sin(theta))<0.000001)//check vertical
    {
        //when vertical, line equation becomes
        //x = rho
        double m = -cos(theta)/sin(theta);
        Point2d p1 = Point2d(rho+im.cols/2,0);
        Point2d p2 = Point2d(rho+im.cols/2,im.rows);
        line(orig,p1,p2,Scalar(0,0,255),1);
        *skew=90;
        cout<<"skew angle "<<" 90"<<endl;
    }else
    {
        //convert normal form back to slope intercept form
        //y = mx + b
        double m = -cos(theta)/sin(theta);
        double b = rho/sin(theta)+im.rows/2.-m*im.cols/2.;
        Point2d p1 = Point2d(0,b);
        Point2d p2 = Point2d(im.cols,im.cols*m+b);
        line(orig,p1,p2,Scalar(0,0,255),1);
        double skewangle;
        skewangle= p1.x-p2.x>0? (atan2(p1.y-p2.y,p1.x-p2.x)*180./CV_PI):(atan2(p2.y-p1.y,p2.    x-p1.x)*180./CV_PI);
        *skew=skewangle;
        cout<<"skew angle "<<skewangle<<endl;
    }
}

Mat skewAngle::preprocess1(Mat& im)
{
    Mat ret = Mat::zeros(im.size(),CV_32SC1);
    
    for(int x=1;x<im.cols-1;x++)
    {
        for(int y=1;y<im.rows-1;y++)
        {
            
            int gy = (im.at<uchar>(y-1,x+1)-im.at<uchar>(y-1,x-1))
            +2*(im.at<uchar>(y,x+1)-im.at<uchar>(y,x-1))
            +(im.at<uchar>(y+1,x+1)-im.at<uchar>(y+1,x-1));
            int gx = (im.at<uchar>(y+1,x-1) -im.at<uchar>(y-1,x-1))
            +2*(im.at<uchar>(y+1,x)-im.at<uchar>(y-1,x))
            +(im.at<uchar>(y+1,x+1)-im.at<uchar>(y-1,x+1));
            int g2 = (gy*gy + gx*gx);
            ret.at<int>(y,x)=g2;
        }
    }
    normalize(ret,ret,255,0,NORM_MINMAX);
    ret.convertTo(ret,CV_8UC1);
    threshold(ret,ret,50,255,THRESH_BINARY);
    return ret;
}

 Mat skewAngle::preprocess2(Mat& im)
{
    // 1) assume white on black and does local thresholding
    // 2) only allow voting top is white and buttom is black(buttom text line)
    Mat thresh;
    //thresh=255-im;
    thresh=im.clone();
    adaptiveThreshold(thresh,thresh,255,CV_ADAPTIVE_THRESH_GAUSSIAN_C,THRESH_BINARY,15,-2);
    Mat ret = Mat::zeros(im.size(),CV_8UC1);
    for(int x=1;x<thresh.cols-1;x++)
    {
        for(int y=1;y<thresh.rows-1;y++)
        {
            bool toprowblack = thresh.at<uchar>(y-1,x)==0 ||  thresh.at<uchar>(y-1,x-1)==0     || thresh.at<uchar>(y-1,x+1)==0;
            bool belowrowblack = thresh.at<uchar>(y+1,x)==0 ||  thresh.at<uchar>(y+1,    x-1)==0 || thresh.at<uchar>(y+1,x+1)==0;
            
            uchar pix=thresh.at<uchar>(y,x);
            if((!toprowblack && pix==255 && belowrowblack))
            {
                ret.at<uchar>(y,x) = 255;
            }
        }
    }
    thresh.release();
    return ret;
}
Mat skewAngle::rot(Mat& im,double thetaRad)
{
    cv::Mat rotated;
    double rskew = thetaRad* CV_PI/180;
    double nw = abs(sin(thetaRad))*im.rows+abs(cos(thetaRad))*im.cols;
    double nh = abs(cos(thetaRad))*im.rows+abs(sin(thetaRad))*im.cols;
    cv::Mat rot_mat = cv::getRotationMatrix2D(Point2d(nw*.5,nh*.5), thetaRad*180/CV_PI, 1);
    Mat pos = Mat::zeros(Size(1,3),CV_64FC1);
    pos.at<double>(0)=(nw-im.cols)*.5;
    pos.at<double>(1)=(nh-im.rows)*.5;
    Mat res = rot_mat*pos;
    rot_mat.at<double>(0,2) += res.at<double>(0);
    rot_mat.at<double>(1,2) += res.at<double>(1);
    cv::warpAffine(im, rotated, rot_mat,Size(nw,nh), cv::INTER_LANCZOS4);
    
    rot_mat.release();
    pos.release();
    res.release();
    
    return rotated;
}


 Mat skewAngle::rotateImage(Mat gray)
{
    Mat preprocessed = preprocess2(gray);
    double skew;
    hough_transform(preprocessed,gray,&skew);
    Mat rotated = rot(gray,skew* CV_PI/180);
    
    preprocessed.release();
    gray.release();
    
    return rotated;
}