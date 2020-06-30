/*
 * buf_factory.h
 *
 *  Created on: 2019Äê10ÔÂ12ÈÕ
 *      Author: Administrator
 */

#ifndef TOOLS_BUF_FACTORY_H_
#define TOOLS_BUF_FACTORY_H_

#include "list.h"
#include "os.h"

struct Buffer
{
	void *memory;
	struct list_head *node;
};

struct buffer_factory
{
	struct Buffer *buf_array;
	struct list_head list;
	OS_MUTEX list_mutex;
};

int32_t create_buf_factory(uint32_t num, uint32_t size, struct buffer_factory **p);
struct Buffer *buf_factory_produce(struct buffer_factory *g_buf_factory);
void buf_factory_recycle(int32_t arg, void *buf,struct buffer_factory *g_buf_factory);

#endif /* TOOLS_BUF_FACTORY_H_ */
