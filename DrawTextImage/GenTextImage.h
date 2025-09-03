#pragma once
#include<string>
#include<opencv2/opencv.hpp>


class GenTextImage 
{
public:
	GenTextImage() :GenTextImage(""){}
	GenTextImage(const std::string& text, int fontSize = 5, cv::HersheyFonts fontType = cv::FONT_HERSHEY_SIMPLEX, const cv::Scalar_<unsigned char>& bgra = {0xFF,0xFF,0xFF,0xFF});
	void setText(const std::string& text, int fontSize, cv::HersheyFonts fontType = cv::FONT_HERSHEY_SIMPLEX, const cv::Scalar_<unsigned char>& bgra= { 0xFF,0xFF,0xFF,0xFF });

	void setText(const std::string& text) {
		m_text = text;
	}

	std::string text() {return m_text;}
	void setFontSize(int size) { m_fontSize = size; }

	void setFont(cv::HersheyFonts fontType) {
		m_fontType = fontType;
	}

	void setBold(bool b) 
	{
		m_bBold = b;
	}

	bool genImage();
	void setColor(unsigned char R, unsigned char G, unsigned char B, unsigned A = 255) 
	{
		m_color = cv::Scalar(B,G,R,A);
	}


	void setGuassianAndMedian(bool b) 
	{
		m_bGuassian = b;
		m_bMedian = b;
	}

	void setMorphological(bool b) 
	{
		m_bMorphological = b;
	}

	void setGuassianAndMedianFactor(int ksize = 5)
	{
		if (m_ksize % 2 == 0)
			m_ksize = m_ksize+1;
		else
			m_ksize = ksize; 
	}
	void setMorphologicalFactor(int ksize = 3)
	{
		if (m_MorphologicalKsize % 2 == 0)
			m_MorphologicalKsize = m_MorphologicalKsize + 1;
		else
			m_MorphologicalKsize = ksize;
	}

	cv::Mat getImage() {return m_image;}

	operator bool() 
	{
		return !m_text.empty();
	}

	cv::Mat image() { return m_image; }

protected:
	std::string m_text;
	int m_fontSize;
	cv::HersheyFonts m_fontType = cv::FONT_HERSHEY_SIMPLEX;
	cv::Mat m_image;
	cv::Scalar_<unsigned char> m_color = {255, 255, 255,255};
	bool m_bGuassian = false;
	bool m_bMedian = false;

	bool m_bMorphological = false;
	int m_ksize = 3;
	int m_MorphologicalKsize = 5;
	bool m_bBold = false;
};