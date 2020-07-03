#pragma once

#include <stdlib.h>
#include <memory.h>
#include <string.h>

template <class T>
struct struct_node_list_t{
	T head;
	T tail;
};

template <class T>
struct class_node_list_t{
	typename T::node_t head;
	typename T::node_t tail;
};

#define NODE_LIST_INIT(head, tail)	do{ \
	(head).next = &(tail);	(head).prev = NULL;	\
	(tail).prev = &(head);	(tail).next = NULL;	\
}while (0)
#define NODE_LIST_PAIR_INIT(_pair)	NODE_LIST_INIT(_pair.head,_pair.tail)

#define NODE_LIST_ADD_TAIL(tail,pnode)	do{ \
	(pnode)->next = &(tail);	(pnode)->prev = (tail).prev;	\
	(tail).prev->next = (pnode);	tail.prev = (pnode);		\
}while (0)

#define NODE_LIST_ADD_HEAD(head,pnode)	do{ \
	(pnode)->next = head.next;	(pnode)->prev = &head;			\
	(head).next->prev = (pnode);	head.next = (pnode);		\
}while (0)

#define NODE_LIST_ADD(pprev,pnode)	do{ \
	(pnode)->next = pprev->next;	(pnode)->prev = pprev;		\
	pprev->next->prev = (pnode);	pprev->next = (pnode);		\
}while (0)


#define NODE_LIST_REMOVE(pnode)	do{ \
	(pnode)->next->prev = (pnode)->prev;	\
	(pnode)->prev->next = (pnode)->next;	\
}while (0)


#define NODE_AROUND_SELF(pnode)	(pnode)->next = (pnode)->prev = pnode
