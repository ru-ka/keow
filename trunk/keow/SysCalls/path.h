/*
 * Copyright 2005 Paul Walker
 *
 * GNU General Public License
 * 
 * This file is part of: Kernel Emulation on Windows (keow)
 *
 * Keow is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Keow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Keow; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef KEOW_PATH_H
#define KEOW_PATH_H

class Path {
public:
	Path();
	~Path();

	void SetUnixPath(const char * unixp, bool FollowSymLinks = true);

	const char * UnixPath();
	const char * Win32Path();

private:
	void CalculateWin32Path();
	void CollapseUnixPath();
	void ApplyPathElement(const char *pStr);

	char m_UnixPath[MAX_PATH+1];
	char m_Win32Path[MAX_PATH+1];
	bool m_Win32Calculated;
	bool m_FollowSymLinks;
};

#endif //KEOW_PATH_H
