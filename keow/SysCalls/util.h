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
