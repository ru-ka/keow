// ConstantMapping.h: interface for the ConstantMapping class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONSTANTMAPPING_H__ADA6F5DB_E98D_4F0F_B686_3288741C964F__INCLUDED_)
#define AFX_CONSTANTMAPPING_H__ADA6F5DB_E98D_4F0F_B686_3288741C964F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class ConstantMapping  
{
public:
	static DWORD ElfProtectionToWin32Protection(linux::Elf32_Word prot);
	static int Win32ErrToUnixError(DWORD err);

private:
	ConstantMapping() {}
};

#endif // !defined(AFX_CONSTANTMAPPING_H__ADA6F5DB_E98D_4F0F_B686_3288741C964F__INCLUDED_)