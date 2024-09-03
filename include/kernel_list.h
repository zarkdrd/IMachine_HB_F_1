/**************************************************
     >Author: zarkdrd
     >Date: 2024-06-05 09:44:45
     >LastEditTime: 2024-06-06 11:47:12
     >LastEditors: zarkdrd
     >Description: 
     >FilePath: /IMachine_HB/include/kernel_list.h
**************************************************/
#ifndef _INCLUDE_KERNEL_LIST_H_
#define _INCLUDE_KERNEL_LIST_H_

#include "list.h"
#include <pthread.h>

typedef struct Klist_Node_Data{
    bool status;
    int Line;
    int QLED_Data_Len;
    unsigned int TimeOut;
    unsigned char QLED_Data[1024];
}Node_Data;

/**链表节点**/
typedef struct Klist_Node{
    struct Klist_Node_Data data;
    struct list_head list;
}Node;

class Klist{
	public:
		Klist();
		~Klist();

		bool Init(void);
		Node *CreateNode(void);
		Node *FindNode(Node_Data *data);

		bool HeadTail(Node *new_node);
		bool HeadAdd(Node *new_node);

		bool NodeTail(Node *node, Node *new_node);
		bool NodeAdd(Node *node, Node *_new_node);

		bool ShowALL(void);
		bool DelALL(void);
		bool DelNode(Node_Data *data);

		bool Node_Change(Node_Data *data);
		void ListTaskInit();
	public:
		bool ShowAdd(int Line, void *QLED_Data, int QLED_Data_Len, unsigned int timeout);
	private:
		Node *Head;
	public:
		pthread_mutex_t Mutex;
};

#endif //_INCLUDE_KERNEL_LIST_H_

