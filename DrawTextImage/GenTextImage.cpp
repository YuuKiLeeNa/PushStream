#include "GenTextImage.h"
#include "opencv2/freetype.hpp"


GenTextImage::GenTextImage(const std::string& text, int fontSize, cv::HersheyFonts fontType, const cv::Scalar_<unsigned char>& bgra)
{
	setText(text, fontSize, fontType, bgra);
}



void GenTextImage::setText(const std::string& text, int fontSize, cv::HersheyFonts fontType, const cv::Scalar_<unsigned char>& bgra)
{
	m_text = text;
	m_fontSize = fontSize;
	m_fontType = fontType;
	m_color = bgra;
}


bool GenTextImage::genImage()
{
	if (m_text.empty())
	{
		m_image = cv::Mat();
		return false;
	}

	int fontFace = cv::FONT_HERSHEY_SIMPLEX;



	//std::string font_path = R"(C:\Windows\Fonts\simsun.ttc)";
	int	font_size = 52;
	cv::Ptr<cv::freetype::FreeType2> ft2 = cv::freetype::createFreeType2();
	ft2->loadFontData(R"(C:\Windows\Fonts\simhei.ttf)", 0); // 替换为你的字体文件路径

	// 使用 FreeType 绘制中文文本
	//ft2->putText(img, "你好，OpenCV!", cv::Point(50, 250), 30, cv::Scalar(255, 255, 255), -1, cv::LINE_AA);




	//cv::Mat image = cv::Mat::zeros(400, 800, CV_8UC3);
	//// 创建 FreeType 字体对象
	//cv::Ptr<cv::freetype::FreeType2> ft3 = cv::freetype::createFreeType2();
	//ft3->loadFontData(R"(C:\Windows\Fonts\simsun.ttc)", 0); // 替换为你的字体文件路径
	//// 设置要绘制的中文文本
	//std::string text = "你好，世界！";
	//// 绘制文本，参数包括图像、文本、位置、字体大小、颜色等
	//ft3->putText(image, text, cv::Point(50, 200), 36, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, true);
	//cv::imshow("image", image);
	//cv::waitKey(0);




	// 计算文字大小
	int baseLine;
	int thickness = m_bBold ? 1 : 1;
	cv::Size textSize = ft2->getTextSize(m_text, font_size, thickness, &baseLine);

	cv::Mat tmp = cv::Mat::zeros(textSize, CV_8UC4);
	ft2->putText(tmp, m_text, cv::Point(0, textSize.height-1), font_size, m_color, -1, cv::LINE_AA, true);
	//cv::imshow("text", tmp);
	//cv::waitKey(0);
	if (m_bMorphological)
	{
		cv::Mat kernelMat = cv::getStructuringElement(cv::MorphShapes::MORPH_CROSS, { m_MorphologicalKsize, m_MorphologicalKsize });
		cv::Mat grayMat,binMat, maskMat, dstMat;
		cv::cvtColor(tmp, grayMat, cv::COLOR_BGRA2GRAY);
		cv::threshold(grayMat, binMat, 0, 255, cv::ThresholdTypes::THRESH_BINARY);
		//cv::imshow("bin", binMat);
		cv::morphologyEx(binMat, maskMat, cv::MorphTypes::MORPH_OPEN, kernelMat);
		//cv::imshow("MORPH_OPEN", maskMat);
		cv::copyTo(tmp, dstMat, maskMat);
		tmp = dstMat;
	}


	if (m_bGuassian)
		cv::GaussianBlur(tmp, tmp, {m_ksize, m_ksize}, 0);

	if (m_bMedian)
		cv::medianBlur(tmp, tmp, m_ksize);


	m_image = tmp;
	return true;
}

