#pragma once
#include<atomic>
#include<memory>

class CSourceType :public std::enable_shared_from_this<CSourceType>
{
public:
	enum SourceDefineType { ST_FREE_TYPE_TEXT=0, ST_PIC, ST_AUDIO, ST_MEDIA, ST_CAMERA };
	CSourceType(SourceDefineType type):m_type(type){}
	virtual ~CSourceType() {}
	SourceDefineType type() {
		return m_type;
	}
	void setRemoved(bool b) 
	{
		m_bIsRemoved = b;
	}

	bool isRemoved() 
	{
		return m_bIsRemoved;
	}

protected:
	bool m_bIsRemoved = false;
	SourceDefineType m_type;
};
