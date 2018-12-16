/*
 Written in 2018-12-15
 aim to rotate a rotated picture with some text
 with a rectboundary around the text
*/


#include<opencv2/opencv.hpp>
#include<iostream>
using namespace std;
using namespace cv;
#define NAMEWINDOW1 "二值化"
void On_threshold(int, void*);
int threshold_value = 180;
int threshold_max = 255;
Mat g_src;
Mat g_img_gray;
 
float perimeter(const vector<Point2f> &a)//求多边形周长。
{
	float sum = 0, dx, dy;
	for (size_t i = 0;i<a.size();i++)
	{
		size_t i2 = (i + 1) % a.size();
		dx = a[i].x - a[i2].x;
		dy = a[i].y - a[i2].y;
		sum += sqrt(dx*dx + dy*dy);
	}
	return sum;
}
bool VerifySize(RotatedRect candidate)
{
	float minError = 0.4; //40%的误差范围
	float maxError = 0.8; //80%的误差范围
	float srcArea = g_src.rows*g_src.cols;
	float area = candidate.size.height*candidate.size.width;
	if (area > (srcArea*minError) && area < (srcArea*maxError))
		return true;
	else
		return false;
        cerr<<"no rect been found"<<endl;
}
int main()
{
	
	g_src = imread("rotate.png");
	if (g_src.empty())
	{
		cout << "无法打开图片";
		return -1;
	}
	//imshow("原始图片", g_src);
	cvtColor(g_src, g_img_gray,COLOR_BGR2GRAY);
	namedWindow(NAMEWINDOW1, WINDOW_AUTOSIZE);
	createTrackbar("阈值", NAMEWINDOW1, &threshold_value, threshold_max, On_threshold);
	On_threshold(0, 0);
	waitKey(0);
	return 0;
}
void On_threshold(int, void*)
{
	Mat img_thres;
	threshold(g_img_gray, img_thres, threshold_value, threshold_max, THRESH_BINARY);
	vector<Vec4i>hierarchy;
	vector<vector<Point>>contours;
	//imshow(NAMEWINDOW1, img_thres);
	findContours(img_thres, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
	Mat result(g_src.size(), CV_8UC3, Scalar::all(255));
	vector<vector<Point2f>>markDetector;
	vector<vector<Point2f>>detector; //用来存放顶点
	for (int i = 0;i < contours.size();i++)
	{
		Point2f vertices[4]; //用来存放旋转矩形四个顶点
		RotatedRect mr = minAreaRect(Mat(contours[i]));
		if (VerifySize(mr)) //验证是否是所需要尺寸
		{
            cout << "rect is found"<< endl;
			mr.points(vertices);
			for (int k = 0;k < 4;k++)
			{
				//line(result, vertices[k], vertices[(k + 1) % 4], Scalar(0, 0, 255), 2, 8, 0);
				//cout << "第" << k + 1 << "个点坐标为" << vertices[i] << endl;
			}
			Point v1 = vertices[1] - vertices[0];
			Point v2 = vertices[2] - vertices[0];
			double o = v1.x*v2.y - v2.x*v1.y;
			if (o < 0.0) //逆时针存储四个点
				swap(vertices[1], vertices[3]);
			/*cout << "vertices[0]" << vertices[0] << endl;
			cout << "vertices[1]" << vertices[1] << endl;
			cout << "vertices[2]" << vertices[2] << endl;
			cout << "vertices[3]" << vertices[3] << endl;*/
			vector<Point2f> m;
			for(int i=0;i<4;i++)
			{ 
				m.push_back(vertices[i]);
			}
			//rotateRects.push_back(mr);
			detector.push_back(m);
		}
	}
	/*cout << rotateRects.size()<<endl;
	cout << detector.size();*/
	//移除那些角点互相离的太近的四边形      //移除角点太接近的元素  
	vector< pair<int, int> > tooNearCandidates;
	for (int i = 0;i < detector.size();i++)
	{
		for (int j = i + 1;j < detector.size();j++)
		{
			float distSquared = 0;
			for (int c = 0;c < 4;c++)
			{
				Point v = detector[i][c] - detector[j][c];
				distSquared += v.dot(v);
			}
			distSquared /= 4;
			if (distSquared < 100)
			{
				tooNearCandidates.push_back(pair<int, int>(i, j));
			}
		}
	}
	vector<bool> removalMask(detector.size(), false);
	for (int i = 0;i < tooNearCandidates.size();i++)
	{
		float p1 = perimeter(detector[tooNearCandidates[i].first]);
		float p2 = perimeter(detector[tooNearCandidates[i].second]);
		//谁周长小 移除谁
		int removalIndex;
		if (p1 > p2)
		{
			removalIndex = tooNearCandidates[i].second;
		}
		else 
		{
			removalIndex = tooNearCandidates[i].first;
		}
		removalMask[removalIndex] = true;
		//cout << removalIndex;
	}
	markDetector.clear();
	for (int i = 0;i < detector.size();i++)
	{
		if (!removalMask[i])
		{
			markDetector.push_back(detector[i]);
		}
	}
	cout << "检测到的矩形个数为: "<< markDetector.size() << endl;
	vector<RotatedRect>rotateRects;
	for (int i = 0;i < markDetector.size();i++)
	{
		RotatedRect r = minAreaRect(Mat(markDetector[i]));
		Point2f ver[4];
		r.points(ver);
		for (int k = 0;k < 4;k++)
		{
			line(g_src, ver[k], ver[(k + 1) % 4], Scalar(0, 0, 255), 2, 8, 0);
			cout << "第" << k + 1 << "个点坐标为" << ver[k] << endl;
		}
		rotateRects.push_back(r);
	}
	
	for (size_t i = 0; i < rotateRects.size(); i++)
	{
		Mat dst(g_src.size(),g_src.type());
		Mat rotMat(2, 3, CV_32FC1);
		float r = (float)rotateRects[i].size.width / (float)rotateRects[i].size.height;
		float  angle = rotateRects[i].angle;
		if (r < 1)
			angle = angle + 90; //修正角度
		rotMat = getRotationMatrix2D(rotateRects[i].center, angle, 1);
		warpAffine(g_src, dst, rotMat, dst.size());
		imshow("最终图像", dst);
		Mat dst1;
		getRectSubPix(dst, rotateRects[i].size, rotateRects[i].center, dst1); //裁剪矩形
		imshow("最终图像2", dst1);
	}
	imshow(NAMEWINDOW1, g_src);
	
}
