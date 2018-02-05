#include <opencv2/opencv.hpp>

IplImage * src;
IplImage * dst;


struct mystroke // �������� ��ĥ�� �ϱ����� ���� ����ü
{
	int x;
	int y;
	int r;
	CvScalar f;
};

void makespline(IplImage * blur, IplImage * canvas, mystroke stroke) // ���� �׷��ִ� �Լ�
{
	int x1 = stroke.x; // �ش� ��ĥ�� x��ǥ
	int y1 = stroke.y; // �ش� ��ĥ�� y��ǥ
	int lastdx = 0; 
	int lastdy = 0;
	int x2 = 0;
	int y2 = 0;
	
	int minstrokelength = 2; // �ּ� ���� ���� ���� (������ �ּ� ����)
	int maxstrokelength = 4; // �ִ� ���� ���� ����
	
	for(int i=1; i<maxstrokelength; i++)
	{
		if(x1<0 || x1>canvas->width-1)
			break;
		if(y1<0 || y1>canvas->height-1)
			break;

		CvScalar g = cvGet2D(blur, y1, x1); // ���� �̹����� ���� ��
		CvScalar c = cvGet2D(canvas, y1, x1); // ĵ���� �̹����� ���� ��
		CvScalar s = stroke.f; // ��ĥ�� ��

		float diff1 = (g.val[0]-c.val[0])*(g.val[0]-c.val[0]) + (g.val[1]-c.val[1])*(g.val[1]-c.val[1]) + (g.val[2]-c.val[2])*(g.val[2]-c.val[2]);
		float diff2 = (g.val[0]-s.val[0])*(g.val[0]-s.val[0]) + (g.val[1]-s.val[1])*(g.val[1]-s.val[1]) + (g.val[2]-s.val[2])*(g.val[2]-s.val[2]);
		
		if(i > minstrokelength && diff1 < diff2) // ������ �ּұ��̸�ŭ�� �׷Ȱ�, ���� �̹����� ĵ���� �̹����� ���̰� ���� �̹����� ���� ���� ���̺��� ũ�� �׸��� ����
			break;
		
		x2 = x1-1;
		y2 = y1-1;
		if(x2<0) x2 = 0;
		if(y2<0) y2 = 0;
		
		//************************************ �׷����Ʈ ���� ���ϱ� **************************************//
		CvScalar yy = cvGet2D(blur, y1, x1); // I(i, j): ��ǥ(i,j)�� ��
		CvScalar yx = cvGet2D(blur, y1, x2); // I(i-1, j) ��ǥ(i-1,j)�� ��
		CvScalar xy = cvGet2D(blur, y2, x1); // I(i, j-1): ��ǥ(i,j-1)�� ��
		
		// luminance ��� ���
		float yy0 = yy.val[0]*0.11 + yy.val[1]*0.59+ yy.val[2]*0.3;
		float yx0 = yx.val[0]*0.11 + yx.val[1]*0.59+ yx.val[2]*0.3;
		float xy0 = xy.val[0]*0.11 + xy.val[1]*0.59+ xy.val[2]*0.3;

		float gx = yy0 - yx0; // x��̺�
		float gy = yy0 - xy0; // y��̺�

		// ���� �׷����Ʈ ���� ���ϱ�
		float magni = sqrt(gx*gx+gy*gy); // �׷����Ʈ ������ ũ��

		if(magni<0.001)// �׷����Ʈ�� ������ ������ ���̻� �׸��� ����
			break;

		float gux = gx / magni;
		float guy = gy / magni;

		//************************************ �׷����Ʈ ���� ���ϱ� **************************************//

		float dx = -guy; // ĥ�� ����, direction ���ϱ�
		float dy = gux;

		if(lastdx*dx + lastdy*dy<0)//ĥ�� �� ������ �� �ϸ��� ���⼱��
		{
			dx = -dx;
			dy = -dy;
		}

		//**********************����, �������� ����*********************//
		int xx1 = x1;
		int yy1 = y1;
		x1 = x1+stroke.r*dx;
		y1 = y1+stroke.r*dy;
		lastdx = dx;
		lastdy = dy;
		cvLine(canvas,cvPoint(xx1, yy1), cvPoint(x1, y1), s, 0.95*stroke.r);
	}
}

void makebrush(int brushsize, int diff_cri) // ���ϴ� �� ������� �׸��� �׷����� �Լ���, diff_cri�� �� �׸��� ���̿� ���� ���� ����
{
	float sum; // �˻�������� ������ ������ ����ġ, �̸� ���� ������ ������, �˻������ �� �׸��� ���� ��� ���̸� �� �� �ִ�.
	float aver; // �˻������ �� �׸� ���� ���̸� ��ճ� ��
	float pre; // ������ �� ���� �ȼ� ���� ���� ���̸� ������ ����
	int n=0;
	int x0;
	int y0;

	CvScalar z;// ���� �ȿ��� ���� ���� ���̰� ū ���� ���� �����ϱ� ����
	CvSize size = cvGetSize(src);

	int count = size.height/float(brushsize)*size.width/float(brushsize);

	mystroke *c = (mystroke*)malloc(sizeof(mystroke)*count*3);

	IplImage * gimg1 = cvCreateImage(size, 8, 3); // ���� ������ ����þ��� ���� �׸�
	cvSmooth(src, gimg1, CV_GAUSSIAN, brushsize*2-5);

	for(int y=0; y<size.height; y+=brushsize)
		 for(int x=0; x<size.width; x+=brushsize)
 		 {
			 sum=0; // �ʱ�ȭ
			 pre = 0; // �ʱ�ȭ
			 CvScalar s, g;
			 int count =0;

			 for(int u=0; u<brushsize; u++)
				 for(int v=0; v<brushsize; v++)
				 {
					 count++;
					 int x2 = x+u; // �̹��������� �˻�������� ���� ��ǥ
					 int y2 = y+v; // �̹��������� �˻�������� ���� ��ǥ
					  
				     if(x2<0 || x2>size.width-1) continue;
					 if(y2<0 || y2>size.height-1) continue;
					 
					 s = cvGet2D(dst, y2, x2); // ���� �̹���
					 g = cvGet2D(gimg1, y2, x2); // ���콺�� ���� �̹���
					 
					 float diff = (s.val[0]-g.val[0])*(s.val[0]-g.val[0])+(s.val[1]-g.val[1])*(s.val[1]-g.val[1])+(s.val[2]-g.val[2])*(s.val[2]-g.val[2]); // �ȼ� ���� ���� ���̸� ���Ѵ�.
					 diff=sqrt(diff);
					 sum = sum + diff; // �ȼ� ���� ���� ���̸� sum�� ������Ų��.
     
					 if(pre<diff) // ���� �ȿ��� ���� ���� ���̰� ū ���� ���� �����ϱ� ����
					 {
						 pre=diff;
						 z = g;
						 x0=x2;
						 y0=y2;
					 }
				 }
			 
			 aver =sum/count; // �� �׸� ���� ������ ����� ���
			 if(aver>diff_cri) // �� �׸��� ���̰� ũ�� ���� ���̰� ���� ���� ������ ������ ĥ�ϰ�, �� ������ ���� ��´�.
			 {
				 c[n].f = z; // �� �׸��� ���̰� ���� ���� ���� ����
				 c[n].x =x0;
				 c[n].y =y0;
				 c[n].r =brushsize*0.8;
				 n++;
			 }
		 }

	for(int i=0; i<=n; i++) // ���� ���� ��ĥ�� ������ �ٲ���
	{
		int random = rand()%n;
		mystroke temp = c[n];
		c[n] = c[random];
		c[random] = temp;
	}

	
	for(int i=0; i<=n; i++) // ��ĥ ����ŭ ���� ����
		makespline(gimg1, dst, c[i]);
	
	cvReleaseImage(&gimg1);
	free(c);
}
 
int main()
{
	char name[200]; // ���������� ������ִ� �ּҸ� ������ ����
	printf("Input File Name:");
	scanf("%s", name);
	src = cvLoadImage(name); // ���� ������ �ּҷ� ���� ������ src�� ����
	CvSize size0 = cvGetSize(src);
	dst = cvCreateImage(size0, 8, 3); // ���
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
