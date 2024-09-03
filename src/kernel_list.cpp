/*************************************************************************
    > File Name: src/kernel_list.cpp
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-10-31
 ************************************************************************/

#include "Log_Message.h"
#include "kernel_list.h"

Klist::Klist()
{
	Head = NULL;
	Mutex = PTHREAD_MUTEX_INITIALIZER;

	Init();
}

Klist::~Klist()
{
	DelALL();
	pthread_mutex_destroy(&Mutex);
}

bool Klist::Init(void)
{
    Head = new Node;
    if(Head == NULL){
        log_message(ERROR, "new: %s", strerror(errno));
        return false;
    }else{
        INIT_LIST_HEAD(&(Head->list));
    }
    return true;
}

Node *Klist::CreateNode(void)
{
    Node *node = new Node;
    if(node == NULL){
        log_message(ERROR, "new: %s", strerror(errno));
        return NULL;
    }else{
        node->data.status = false;
        node->list.next = NULL;
        node->list.prev = NULL;
    }
    return node;
}

Node *Klist::FindNode(Node_Data *data)
{
    struct list_head *ptr=NULL;
    struct list_head *n=NULL;
    Node *p=NULL;
	pthread_mutex_lock(&Mutex);
    list_for_each_safe(ptr,n,&(Head->list)){
        p = list_entry(ptr, Node, list);
        if(p->data.status == data->status){
			pthread_mutex_unlock(&Mutex);
            return p;
        }
    }
	pthread_mutex_unlock(&Mutex);
    return NULL;
}

bool Klist::HeadTail(Node *new_node)
{
    if((Head == NULL) || (new_node == NULL)){
        log_message(ERROR, "插入节点为NULL");
        return false;
    }
	pthread_mutex_lock(&Mutex);
    list_add_tail(&(new_node->list), &(Head->list));
	pthread_mutex_unlock(&Mutex);
	return true;
}

bool Klist::HeadAdd(Node *new_node)
{
    if((Head == NULL) || (new_node == NULL)){
        log_message(ERROR, "插入节点为NULL");
        return false;
    }
	pthread_mutex_lock(&Mutex);
    list_add(&(new_node->list), &(Head->list));
	pthread_mutex_unlock(&Mutex);
	return true;
}

bool Klist::NodeTail(Node *node, Node *new_node)
{
    if((node == NULL) || (new_node == NULL)){
        log_message(ERROR, "插入节点为NULL");
        return false;
    }
	pthread_mutex_lock(&Mutex);
    list_add_tail(&(new_node->list), &(node->list));
	pthread_mutex_unlock(&Mutex);
	return true;
}

bool Klist::NodeAdd(Node *node, Node *new_node)
{
    if((node == NULL) || (new_node == NULL)){
        log_message(ERROR, "插入节点为NULL");
        return false;
    }
	pthread_mutex_lock(&Mutex);
    list_add(&(new_node->list), &(node->list));
	pthread_mutex_unlock(&Mutex);
	return true;
}

bool Klist::DelALL(void)
{
    struct list_head *ptr=NULL;
    struct list_head *n=NULL;
    Node *p=NULL;

	pthread_mutex_lock(&Mutex);
    list_for_each_safe(ptr,n,&(Head->list)){
        p=list_entry(ptr, Node, list);
        list_del(&(p->list));
        free(p);
    }
	delete Head;
    Head = NULL;
	pthread_mutex_unlock(&Mutex);

	return false;
}

bool Klist::DelNode(Node_Data *data)
{
    struct list_head *ptr=NULL;
    struct list_head *n=NULL;
    Node *p=NULL;

	pthread_mutex_lock(&Mutex);
    list_for_each_safe(ptr,n,&(Head->list)){
        p = list_entry(ptr, Node, list);
        if(p->data.status == data->status){
            list_del(&(p->list));
            free(p);
        }
    }
	pthread_mutex_unlock(&Mutex);

	return true;
}

bool Klist::Node_Change(Node_Data *data)
{
    struct list_head *ptr=NULL;
    struct list_head *n=NULL;
    Node *p=NULL;

	pthread_mutex_lock(&Mutex);
    list_for_each_safe(ptr,n,&(Head->list)){
        p=list_entry(ptr, Node, list);
        if(p->data.Line == data->Line){
			memcpy(&p->data, data, sizeof(Node_Data));
            p->data.status = true;
			pthread_mutex_unlock(&Mutex);
            return true;
        }
    }
	pthread_mutex_unlock(&Mutex);

    p = CreateNode();
	memcpy(&p->data, data, sizeof(Node_Data));
	p->data.status = true;

    HeadTail(p);

	return true;
}

bool Klist::ShowAdd(int Line, void *QLED_Data, int QLED_Data_Len, unsigned int timeout)
{
    Node_Data data;
    data.QLED_Data_Len = QLED_Data_Len;
    memcpy(data.QLED_Data, QLED_Data, QLED_Data_Len);
    data.TimeOut = timeout;
    data.Line = Line;

	Node_Change(&data);

    return true;
}

/**任务链表初始化,增加屏幕显示控制数据帧队列**/
void Klist::ListTaskInit(void)
{
    Node *node;
    for(int i = 0; i < 11; i++){
        node = CreateNode();
		node->data.Line = i;
		HeadTail(node);
    }
}
