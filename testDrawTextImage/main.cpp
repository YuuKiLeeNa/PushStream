#include "DrawTextImage/GenTextImage.h"


int main(int argc, char*argv[])
{
	{
		GenTextImage img;
		img.setText("abcdefghijklmn");
		img.setGuassianAndMedianFactor(3);
		img.setGuassianAndMedian(true);
		img.genImage();
		cv::Mat mat = img.getImage();
		cv::imshow("GuassianMedian", mat);
	}

	{
		GenTextImage img1;
		img1.setText("abcdefghijklmn");
		img1.setMorphological(true);
		img1.genImage();
		cv::Mat mat1 = img1.getImage();
		cv::imshow("Morphologica", mat1);
	}

	{
		GenTextImage img3;
		img3.setText("abcdefghijklmn");
		img3.genImage();
		cv::Mat mat3 = img3.getImage();
		cv::imshow("noop", mat3);
	}

	{
		GenTextImage img4;
		
		img4.setText("abcdefghijklmn");
		img4.setColor(78, 204, 156);
		img4.setMorphologicalFactor(3);
		img4.setMorphological(true);
		img4.setGuassianAndMedianFactor(3);
		img4.setGuassianAndMedian(true);
		img4.genImage();
		cv::Mat mat4 = img4.getImage();
		cv::imshow("Morphologica+GuassianMedian", mat4);
	}


	cv::waitKey(0);
}
