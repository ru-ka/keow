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

#ifndef LKFW_UTIL_H
#define LKFW_UTIL_H

class LinkedListItem
{
public:
	LinkedListItem()	{next=prev=NULL;}

	void AddAfter(LinkedListItem* other)
	{
		if(other)
		{
			next=other->next;
			other->next = this;
		}
		else
			next=NULL;
		prev=other;
		if(next)
			next->prev = this;
	}

	void AddAtEnd(LinkedListItem* other)
	{
		//get to end of list
		while(other->next)
			other=other->next;

		AddAfter(other);
	}

	void Remove()
	{
		if(next)
			next->prev = prev;
		if(prev)
			prev->next = next;
	}

	LinkedListItem *next, *prev;
};


#endif // LKFW_UTIL_H
