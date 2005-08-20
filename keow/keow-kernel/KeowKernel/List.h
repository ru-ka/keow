//Simple List implementation (like STL) (see comment in includes.h)
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LIST_H__4305518F_D005_41B7_8993_8F7E70C9A0AC__INCLUDED_)
#define AFX_LIST_H__4305518F_D005_41B7_8993_8F7E70C9A0AC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "List.h"


template<class T>
class list
{
public:
	class iterator;
protected:
	class node
	{
		friend list;
		friend iterator;

		node(T val)
			: value(val)
			, next(0), prev(0)
		{
		}

		node *prev, *next;
		T value;
	};
	node *head, *tail;

public:
	class iterator
	{
	private:
		friend list;
		node *n;
	public:
		iterator()
		{
			n=0;
		}

		T& operator*()
		{
			return n->value;
		}

		iterator& operator=(iterator& other)
		{
			n = other.n;
			return *this;
		}

		bool operator==(iterator& other)
		{
			return n == other.n;
		}

		bool operator!=(iterator& other)
		{
			return n != other.n;
		}

		void operator++()
		{
			if(n!=0)
			{
				n = n->next;
			}
		}
	};

public:
	list();
	~list();

	list& operator=(const list& other);

	list& push_back(T element);
	void pop_back();
	iterator find(T value);
	void erase(iterator it);
	void erase(T value);
	void clear();
	bool empty() const;
	int size() const;

	iterator begin() const;
	iterator end() const;
};


/////////////////////////////////////////////////////////////////


template<class T> inline
list<T>::list<T>()
{
	head=tail=NULL;
}

template<class T> inline
list<T>::~list<T>()
{
	clear();
}

template<class T> inline
list<T>& list<T>::operator=(const list<T>& other)
{
	clear();

	list<T>::iterator it;
	for(it=other.begin(); it!=other.end(); ++it)
	{
		push_back( *it );
	}
	return *this;
}


template<class T> inline
list<T>& list<T>::push_back(T element)
{
	list<T>::node *n = new list<T>::node(element);
	n->next = NULL;
	n->prev = tail;
	if(tail) //already have a tail?
		tail->next=n; //add to end of list
	else
		head = n; //first element in list
	tail = n; //new tail
if(head && head->next==head->prev && head->prev!=0) DebugBreak();
if(tail && tail->next==tail->prev && tail->prev!=0) DebugBreak();
	return *this;
}

template<class T> inline
void list<T>::pop_back()
{
	list<T>::node *n = tail; //save

	tail = tail->prev; //move tail back one
	if(tail)
		tail->next = NULL; //new tail is end
	else
		head=NULL; //removed entire list

if(head && head->next==head->prev && head->prev!=0) DebugBreak();
if(tail && tail->next==tail->prev && tail->prev!=0) DebugBreak();
	delete n;
}

template<class T> inline
list<T>::iterator list<T>::find(T value)
{
	list<T>::iterator it;
	for(it=begin(); it!=end(); ++it)
	{
		if(it.n->value == value)
			return it;
	}
	return it;
}

template<class T> inline
void list<T>::erase(list<T>::iterator it)
{
	if(it.n==NULL)
		return;

	list<T>::node *n = it.n;
	if(n->prev)
		n->prev->next = n->next;
	if(n->next)
		n->next->prev = n->prev;

	delete n;
}

template<class T> inline
void list<T>::erase(T value)
{
	list<T>::erase(list<T>::find(value));
}

template<class T> inline
void list<T>::clear()
{
	while(!empty())
		pop_back();
}

template<class T> inline
bool list<T>::empty() const
{
	return head==NULL;
}

template<class T> inline
int list<T>::size() const
{
	int cnt = 0;
	list<T>::iterator it;
	for(it=begin(); it!=end(); ++it)
	{
		++cnt;
	}
	return cnt;
}


template<class T> inline
list<T>::iterator list<T>::begin() const
{
	list<T>::iterator it;
	it.n = head;
	return it;
}

template<class T> inline
list<T>::iterator list<T>::end() const
{
	list<T>::iterator it;
	it.n = NULL;
	return it;
}


#endif // !defined(AFX_LIST_H__4305518F_D005_41B7_8993_8F7E70C9A0AC__INCLUDED_)
