/******************************************************************************
 * @file pipe_link
 * @brief Manage the communication with a pipe.
 * @author Luos
 ******************************************************************************/
#ifndef PIPE_LINK_H
#define PIPE_LINK_H

#include "luos.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void PipeLink_Send(container_t *service, void *data, uint32_t size);
short PipeLink_Find(container_t *service);
short PipeLink_GetId(void);
void PipeLink_SetStreamingChannel(void *streamingChannel);

#endif /* PIPE_LINK_H */