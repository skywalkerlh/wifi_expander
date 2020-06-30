/*
 * buf_factory.c
 *
 *  Created on: 2019Äê10ÔÂ12ÈÕ
 *      Author: Administrator
 */

#include <stdio.h>
#include <stdlib.h>
#include "em_core.h"
#include "rtt.h"
#include "buf_factory.h"



int32_t create_buf_factory(uint32_t num, uint32_t size, struct buffer_factory **p)
{
	RTOS_ERR  err;
	int32_t i = 0;
	int32_t j = 0;

	struct buffer_factory *buf_factory;

	buf_factory  = malloc(sizeof( struct buffer_factory));
	if(buf_factory == NULL)
		return -1;

	init_list_head(&buf_factory->list);

	OSMutexCreate(&(buf_factory->list_mutex),
				"buf_factory_mutex",
				&err);

	buf_factory->buf_array = calloc(num, sizeof(struct Buffer));
	if(buf_factory->buf_array == NULL)
	{
		free(buf_factory);
		return -1;
	}

	for(i=0;i<num;i++)
	{
		buf_factory->buf_array[i].memory = malloc(size);
		if(buf_factory->buf_array[i].memory== NULL)
		{
			for(j=i-1; j>=0; j--)
				free(buf_factory->buf_array[i].memory);
			free(buf_factory->buf_array);
			free(buf_factory);
			return -1;
		}
		buf_factory->buf_array[i].node = malloc(sizeof(struct list_head));
		if(buf_factory->buf_array[i].node == NULL)
		{
			for(j=i-1; j>=0; j--)
				free(buf_factory->buf_array[i].node);

			for(j=i; j>=0; j--)
				free(buf_factory->buf_array[i].memory);

			free(buf_factory->buf_array);
			free(buf_factory);
			return -1;
		}
		buf_factory->buf_array[i].node->owner = &buf_factory->buf_array[i];
		list_add_tail(buf_factory->buf_array[i].node, &buf_factory->list);
	}

	*p = buf_factory;

	return 0;
}

struct Buffer *buf_factory_produce(struct buffer_factory *buf_factory)
{
	CPU_TS ts;
	RTOS_ERR  err;
	struct list_head *node_of_buf;
	struct Buffer *buf;

	OSMutexPend(&(buf_factory->list_mutex),
				0,
				OS_OPT_PEND_BLOCKING,
				&ts,
				&err);
	if(list_empty(&buf_factory->list))
	{
		OSMutexPost(&(buf_factory->list_mutex),
					OS_OPT_POST_NONE,
					&err);
		return NULL;
	}

//		DBG("buf factory crash !");

	node_of_buf = buf_factory->list.next;
	buf = (struct Buffer *)(node_of_buf->owner);
	list_del(node_of_buf);

	OSMutexPost(&(buf_factory->list_mutex),
				OS_OPT_POST_NONE,
				&err);

	return buf;
}

void buf_factory_recycle(int32_t arg, void  *__buf__  , struct buffer_factory *buf_factory)
{
	CPU_TS ts;
	RTOS_ERR  err;
	struct Buffer *buf;

	buf = (struct Buffer*)__buf__;

	OSMutexPend(&(buf_factory->list_mutex),
				0,
				OS_OPT_PEND_BLOCKING,
				&ts,
				&err);

	list_add_tail(buf->node, &buf_factory->list);

	OSMutexPost(&(buf_factory->list_mutex),
				OS_OPT_POST_NONE,
				&err);
}
