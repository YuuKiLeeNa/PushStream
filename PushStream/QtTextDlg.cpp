#include "QtTextDlg.h"
#include<QLabel>
#include<QTextEdit>
#include<QLineEdit>
#include<QBoxLayout>
#include<QImage>
#include<QPixmap>
#include<string>

//QtTextDlg::QtTextDlg(QWidget* parentWidget):QtBaseTitleMoveWidget(parentWidget)
//, m_genImage(std::make_shared<GLDrawText>())
//{
//	setTitleText("set text");
//	setFixedSize(800,600);
//	initUI();
//}

QtTextDlg::QtTextDlg(std::shared_ptr<GLDrawText>ptr, QWidget* parentWidget) :QtBaseTitleMoveWidget(parentWidget)
, m_genImage(ptr)
{
	setTitleText("set text");
	setFixedSize(800, 600);
	initUI();
	std::string str = ptr->text();
	if (!str.empty())
		m_edit->setText(QString::fromStdString(str));
}

void QtTextDlg::initUI()
{
	QWidget*wid = getBodyWidget();

	m_label = new QLabel(wid);
	m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_edit = new QTextEdit(wid);
	m_edit->setFixedSize(400,200);

	m_colorEdit = new QLineEdit(wid);
	m_colorEdit->setText("#FF0000A0");
	m_colorEdit->setEnabled(false);

	QVBoxLayout* lay = new QVBoxLayout;
	lay->setAlignment(Qt::AlignCenter);
	lay->addWidget(m_label);
	lay->addWidget(m_colorEdit);
	lay->addWidget(m_edit);

	wid->setLayout(lay);
	initConnection();
}

void QtTextDlg::initConnection()
{
	QObject::connect(m_edit, SIGNAL(textChanged()), this,SLOT(SlotTextChanged()));
}

void QtTextDlg::SlotTextChanged()
{
	std::string str(m_edit->toPlainText().toUtf8().data(), m_edit->toPlainText().toUtf8().size());
	/*if (str.empty()) 
	{
		return;
	}*/
	m_genImage->setText(str);
	/*m_genImage.setBold(true);
 	m_genImage.setGuassianAndMedian(true);
	m_genImage.setMorphological(true);*/

	std::string strColor = m_colorEdit->text().toStdString();
	unsigned char b = std::stoi(strColor.substr(1,2), nullptr, 16);
	unsigned char g = std::stoi(strColor.substr(3, 2), nullptr, 16);
	unsigned char r = std::stoi(strColor.substr(5, 2), nullptr, 16);
	unsigned char a = std::stoi(strColor.substr(7, 2), nullptr, 16);

	m_genImage->setColor(b, g, r, a);

	if (m_genImage->genImage()) 
	{
		cv::Mat img = m_genImage->getImage();
		//cv::Mat img_argb32;
		//cv::cvtColor(img, img_argb32, cv::COLOR_BGRA2RGBA)
		QImage qimg(img.data, img.cols, img.rows, QImage::Format::Format_ARGB32);
		QPixmap pixmap = QPixmap::fromImage(qimg);
		m_label->setPixmap(pixmap);
		m_genImage->init();
	}
	else 
	{
		m_label->setPixmap(QPixmap());
		m_genImage->init();
	}

}
