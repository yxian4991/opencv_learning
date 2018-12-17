/*
 Written in 2018-12-17
 aim to rotate a rotated picture with rect
 but to different picture,such as NeedRotated0\1\2 ,differnt results?
 the key is the class "RotateRect" & the function "minAreaRect"
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


float perimeter(const vector<Point2f> &a);//求周长
bool VerifySize(RotatedRect candidate);//求满足条件的矩形

int main()
{

	g_src = imread("NeedRotate1.jpg");
	if (g_src.empty())
	{
		cout << "无法打开图片";
		return -1;
	}
	imshow("原始图片", g_src);
	cvtColor(g_src, g_img_gray, COLOR_BGR2GRAY);
	namedWindow(NAMEWINDOW1, WINDOW_AUTOSIZE);
	createTrackbar("阈值", NAMEWINDOW1, &threshold_value, threshold_max, On_threshold);
	On_threshold(0, 0);
	waitKey(0);
	return 0;
}

//矩形周长
float perimeter(const vector<Point2f> &a)
{
	float sum = 0, dx, dy;
	for (size_t i = 0; i<a.size(); i++)
	{
		size_t i2 = (i + 1) % a.size();
		dx = a[i].x - a[i2].x;
		dy = a[i].y - a[i2].y;
		sum += sqrt(dx*dx + dy*dy);
	}
	return sum;
}

//搜索条件
bool VerifySize(RotatedRect candidate)
{
	float minError = 0.2; //面积高于20%的误差范围
	float maxError = 0.8; //面积低于80%的误差范围
	float srcArea = g_src.rows*g_src.cols;
	float area = candidate.size.height*candidate.size.width;
	if (area > (srcArea*minError) && area < (srcArea*maxError))
		return true;
	else
	{
		return false;
		cerr << "no satified rect !" << endl;
	}
}

void On_threshold(int, void*)
{
	Mat img_thres;
	vector<Vec4i>hierarchy;
	vector<vector<Point>>contours;
	vector<vector<Point2f>>detector; //用来存放顶点
	vector<vector<Point2f>>FinalDetector;//存放去掉临近矩形的最终矩形
	//阈值分割
	threshold(g_img_gray, img_thres, threshold_value, threshold_max, THRESH_BINARY);
	findContours(img_thres, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
	Mat drawing = Mat::zeros(img_thres.size(), CV_8UC1);;
	drawContours(drawing, contours, -1, Scalar(255), 10, 8, vector<Vec4i>(), 0, Point());
	imshow("分割角点",drawing);
	Mat result(g_src.size(), CV_8UC3, Scalar::all(255));
	cout << "contours.size: " << contours.size() << endl << endl;
	//按行存储顶点
	for (int i = 0; i < contours.size(); i++)
	{
		Point2f vertices[4]; //用来存放旋转矩形四个顶点
		RotatedRect mr = minAreaRect(Mat(contours[i]));
		if (VerifySize(mr)) //验证是否是所需要尺寸
		{
			cout << "rect is found"<< endl;
			mr.points(vertices);
			for (int k = 0; k < 4; k++)
			{
				line(result, vertices[k], vertices[(k + 1) % 4], Scalar(0, 0, 255), 2, 8, 0);
				cout << "未排序第" << k + 1 << "个点坐标为" << vertices[k] << endl;
			}
			Point v1 = vertices[1] - vertices[0];
			Point v2 = vertices[2] - vertices[0];
			double o = v1.x*v2.y - v2.x*v1.y;
			if (o < 0.0) //逆时针存储四个点
				swap(vertices[1], vertices[3]);
			cout << "排序后第1个点坐标为" << vertices[0] << endl;
			cout << "排序后第2个点坐标为" << vertices[1] << endl;
			cout << "排序后第3个点坐标为" << vertices[2] << endl;
			cout << "排序后第4个点坐标为" << vertices[3] << endl;
			vector<Point2f> m;
			for (int i = 0; i<4; i++)
			{
				m.push_back(vertices[i]);
			}
			detector.push_back(m);//detector每行存储一个矩形的四个顶点位置信息
		}
	}
	cout << "detector.size:" << detector.size() << endl<<endl;//size表明有几行即表明有几个矩形
    //找出角点太接近的元素  
	vector< pair<int, int> > CloseCandidates;
	for (int i = 0; i < detector.size(); i++)
	{
		for (int j = i + 1; j < detector.size(); j++)
		{
			float distSquared = 0;
			for (int c = 0; c < 4; c++)
			{
				Point v = detector[i][c] - detector[j][c];
				distSquared += v.dot(v);
				cout << "i=" << i << "-c=" << c << "-detector[i][c]=" << detector[i][c] << endl;
				cout << "j=" << j << "-c=" << c << "-detector[j][c]=" << detector[j][c] << endl;
			}
			distSquared /= 4;
			if (distSquared < 200)//距离阈值
			{
				CloseCandidates.push_back(pair<int, int>(i, j));//第i和j临近的矩形靠的太近，一行一对i和j的值
			}
		}
	}
	//记录周长小的矩形编号
	cout << "CloseCandidates.size:" << CloseCandidates.size() << endl<<endl;
	vector<bool> removalMask(detector.size(), false);
	for (int i = 0; i < CloseCandidates.size(); i++)
	{
		cout << "CloseCandidates[i].first:"  << CloseCandidates[i].first  << endl;
		cout << "CloseCandidates[i].second:" << CloseCandidates[i].second << endl;
		float p1 = perimeter(detector[CloseCandidates[i].first]);
		float p2 = perimeter(detector[CloseCandidates[i].second]);
		int removalIndex;
		if (p1 > p2)
		{
			removalIndex = CloseCandidates[i].second;
		}
		else
		{
			removalIndex = CloseCandidates[i].first;
		}
		removalMask[removalIndex] = true;
	}
	FinalDetector.clear();//清空格式化
	for (int i = 0; i < detector.size(); i++)
	{ 
		//周长小的矩形将不被记录到最终的矩形顶点信息
		if (!removalMask[i])
		{
			FinalDetector.push_back(detector[i]);
			cout << "第" << i << "个FinalDetector: " << FinalDetector[i] << endl;
		}
	}
	cout << "最终的矩形个数为: " << FinalDetector.size() << endl << endl;
	vector<RotatedRect>rotateRects;
	for (int i = 0; i < FinalDetector.size(); i++)
	{
		RotatedRect r = minAreaRect(Mat(FinalDetector[i]));
		Point2f ver[4];
		r.points(ver);
		for (int k = 0; k < 4; k++)
		{
			line(g_src, ver[k], ver[(k + 1) % 4], Scalar(0, 0, 255), 2, 8, 0);
			cout << "第" << k + 1 << "个点坐标为" << ver[k] << endl;
		}
		rotateRects.push_back(r);
	}
	for (size_t i = 0; i < rotateRects.size(); i++)
	{
		Mat dst(g_src.size(), g_src.type());
		Mat rotMat(2, 3, CV_32FC1);
		float r = (float)rotateRects[i].size.width / (float)rotateRects[i].size.height;
		float  angle = rotateRects[i].angle;
		if (r < 1)
			angle = angle + 90; //修正角度
		cout << "angle=" << angle << endl;
		//旋转至水平
		rotMat = getRotationMatrix2D(rotateRects[i].center, angle, 1);
		warpAffine(g_src, dst, rotMat, dst.size());
		imshow("旋转图像", dst);
		//裁剪矩形
		Mat dstimg;
		getRectSubPix(dst, rotateRects[i].size, rotateRects[i].center, dstimg);
		imshow("裁剪图像", dstimg);
	}
	imshow(NAMEWINDOW1, g_src);

}
