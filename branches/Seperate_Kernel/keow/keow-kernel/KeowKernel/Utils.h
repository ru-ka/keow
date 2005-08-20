// Utils.h: interface for the Utils class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UTILS_H__23B86EDD_923F_42A7_94E3_68A42DB40A3D__INCLUDED_)
#define AFX_UTILS_H__23B86EDD_923F_42A7_94E3_68A42DB40A3D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


void ktrace(const char *format, ...);
void halt();

const int SIZE4k = 4*1024;
const int SIZE64k = 64*1024;

#endif // !defined(AFX_UTILS_H__23B86EDD_923F_42A7_94E3_68A42DB40A3D__INCLUDED_)
