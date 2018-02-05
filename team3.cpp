#include <opencv2/opencv.hpp>

IplImage * src;
IplImage * dst;


struct mystroke // 무작위로 붓칠을 하기위해 만든 구조체
{
	int x;
	int y;
	int r;
	CvScalar f;
};

void makespline(IplImage * blur, IplImage * canvas, mystroke stroke) // 라인 그려주는 함수
{
	int x1 = stroke.x; // 해당 붓칠의 x좌표
	int y1 = stroke.y; // 해당 붓칠의 y좌표
	int lastdx = 0; 
	int lastdy = 0;
	int x2 = 0;
	int y2 = 0;
	
	int minstrokelength = 2; // 최소 이을 점의 개수 (라인의 최소 길이)
	int maxstrokelength = 4; // 최대 이을 점의 개수
	
	for(int i=1; i<maxstrokelength; i++)
	{
		if(x1<0 || x1>canvas->width-1)
			break;
		if(y1<0 || y1>canvas->height-1)
			break;

		CvScalar g = cvGet2D(blur, y1, x1); // 블러링 이미지에 대한 색
		CvScalar c = cvGet2D(canvas, y1, x1); // 캔버스 이미지에 대한 색
		CvScalar s = stroke.f; // 붓칠의 색

		float diff1 = (g.val[0]-c.val[0])*(g.val[0]-c.val[0]) + (g.val[1]-c.val[1])*(g.val[1]-c.val[1]) + (g.val[2]-c.val[2])*(g.val[2]-c.val[2]);
		float diff2 = (g.val[0]-s.val[0])*(g.val[0]-s.val[0]) + (g.val[1]-s.val[1])*(g.val[1]-s.val[1]) + (g.val[2]-s.val[2])*(g.val[2]-s.val[2]);
		
		if(i > minstrokelength && diff1 < diff2) // 라인의 최소길이만큼을 그렸고, 블러링 이미지와 캔버스 이미지의 차이가 블러링 이미지와 붓의 색의 차이보다 크면 그리지 않음
			break;
		
		x2 = x1-1;
		y2 = y1-1;
		if(x2<0) x2 = 0;
		if(y2<0) y2 = 0;
		
		//************************************ 그래디언트 벡터 구하기 **************************************//
		CvScalar yy = cvGet2D(blur, y1, x1); // I(i, j): 좌표(i,j)의 색
		CvScalar yx = cvGet2D(blur, y1, x2); // I(i-1, j) 좌표(i-1,j)의 색
		CvScalar xy = cvGet2D(blur, y2, x1); // I(i, j-1): 좌표(i,j-1)의 색
		
		// luminance 밝기 계산
		float yy0 = yy.val[0]*0.11 + yy.val[1]*0.59+ yy.val[2]*0.3;
		float yx0 = yx.val[0]*0.11 + yx.val[1]*0.59+ yx.val[2]*0.3;
		float xy0 = xy.val[0]*0.11 + xy.val[1]*0.59+ xy.val[2]*0.3;

		float gx = yy0 - yx0; // x편미분
		float gy = yy0 - xy0; // y편미분

		// 단위 그래디언트 벡터 구하기
		float magni = sqrt(gx*gx+gy*gy); // 그레디언트 벡터의 크기

		if(magni<0.001)// 그래디언트가 굉장히 작으면 더이상 그리지 않음
			break;

		float gux = gx / magni;
		float guy = gy / magni;

		//************************************ 그래디언트 벡터 구하기 **************************************//

		float dx = -guy; // 칠할 방향, direction 정하기
		float dy = gux;

		if(lastdx*dx + lastdy*dy<0)//칠할 두 방향중 더 완만한 방향선택
		{
			dx = -dx;
			dy = -dy;
		}

		//**********************필터, 단위벡터 생략*********************//
		int xx1 = x1;
		int yy1 = y1;
		x1 = x1+stroke.r*dx;
		y1 = y1+stroke.r*dy;
		lastdx = dx;
		lastdy = dy;
		cvLine(canvas,cvPoint(xx1, yy1), cvPoint(x1, y1), s, 0.95*stroke.r);
	}
}

void makebrush(int brushsize, int diff_cri) // 원하는 붓 사이즈로 그림을 그려내는 함수로, diff_cri는 두 그림의 차이에 대한 기준 값임
{
	float sum; // 검사범위안의 점들의 차이의 누적치, 이를 점의 개수로 나누면, 검사범위의 두 그림의 색의 평균 차이를 알 수 있다.
	float aver; // 검사범위의 두 그림 색의 차이를 평균낸 것
	float pre; // 이전의 점 간의 픽셀 간의 색의 차이를 저장할 변수
	int n=0;
	int x0;
	int y0;

	CvScalar z;// 범위 안에서 제일 색의 차이가 큰 점의 색을 저장하기 위함
	CvSize size = cvGetSize(src);

	int count = size.height/float(brushsize)*size.width/float(brushsize);

	mystroke *c = (mystroke*)malloc(sizeof(mystroke)*count*3);

	IplImage * gimg1 = cvCreateImage(size, 8, 3); // 원본 사진에 가우시안을 가할 그림
	cvSmooth(src, gimg1, CV_GAUSSIAN, brushsize*2-5);

	for(int y=0; y<size.height; y+=brushsize)
		 for(int x=0; x<size.width; x+=brushsize)
 		 {
			 sum=0; // 초기화
			 pre = 0; // 초기화
			 CvScalar s, g;
			 int count =0;

			 for(int u=0; u<brushsize; u++)
				 for(int v=0; v<brushsize; v++)
				 {
					 count++;
					 int x2 = x+u; // 이미지에서의 검사범위안의 가로 좌표
					 int y2 = y+v; // 이미지에서의 검사범위안의 세로 좌표
					  
				     if(x2<0 || x2>size.width-1) continue;
					 if(y2<0 || y2>size.height-1) continue;
					 
					 s = cvGet2D(dst, y2, x2); // 원본 이미지
					 g = cvGet2D(gimg1, y2, x2); // 가우스를 가한 이미지
					 
					 float diff = (s.val[0]-g.val[0])*(s.val[0]-g.val[0])+(s.val[1]-g.val[1])*(s.val[1]-g.val[1])+(s.val[2]-g.val[2])*(s.val[2]-g.val[2]); // 픽셀 간의 색의 차이를 구한다.
					 diff=sqrt(diff);
					 sum = sum + diff; // 픽셀 간의 색의 차이를 sum에 누적시킨다.
     
					 if(pre<diff) // 범위 안에서 제일 색의 차이가 큰 점의 색을 저장하기 위함
					 {
						 pre=diff;
						 z = g;
						 x0=x2;
						 y0=y2;
					 }
				 }
			 
			 aver =sum/count; // 두 그림 색의 차이의 평균을 계산
			 if(aver>diff_cri) // 두 그림의 차이가 크면 가장 차이가 나는 점의 색으로 영역을 칠하고, 그 색으로 원을 찍는다.
			 {
				 c[n].f = z; // 두 그림의 차이가 많을 때의 색을 저장
				 c[n].x =x0;
				 c[n].y =y0;
				 c[n].r =brushsize*0.8;
				 n++;
			 }
		 }

	for(int i=0; i<=n; i++) // 서로 간의 붓칠의 순서를 바꿔줌
	{
		int random = rand()%n;
		mystroke temp = c[n];
		c[n] = c[random];
		c[random] = temp;
	}

	
	for(int i=0; i<=n; i++) // 붓칠 수만큼 선을 그음
		makespline(gimg1, dst, c[i]);
	
	cvReleaseImage(&gimg1);
	free(c);
}
 
int main()
{
	char name[200]; // 원본사진이 저장되있는 주소를 저장할 변수
	printf("Input File Name:");
	scanf("%s", name);
	src = cvLoadImage(name); // 원본 사진의 주소로 원본 사진을 src에 저장
	CvSize size0 = cvGetSize(src);
	dst = cvCreateImage(size0, 8, 3); // 결과
	cvSet(dst, cvScalar(255, 255, 255), 0);

	makebrush(35, 40);
	makebrush(20, 30);
	makebrush(10, 30);
	makebrush(5, 30);
	makebrush(3, 30);
	

	cvShowImage("src", src);
	cvShowImage("dst", dst);
	cvWaitKey();

	cvReleaseImage(&src);
	cvReleaseImage(&dst);
 
	return 0;
}
