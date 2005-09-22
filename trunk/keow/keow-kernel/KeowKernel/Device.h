// Device.h: interface for the Device class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEVICE_H__570A7152_1A87_434C_903E_9EA5C8896B0A__INCLUDED_)
#define AFX_DEVICE_H__570A7152_1A87_434C_903E_9EA5C8896B0A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class Device  
{
public:
	Device(string dev, int major, int minor);
	virtual ~Device();

	int m_major, m_minor;	//major/minor device numbers
	string m_dev;			//name under /dev
};

#endif // !defined(AFX_DEVICE_H__570A7152_1A87_434C_903E_9EA5C8896B0A__INCLUDED_)
