#ifndef JSON_TO_LUOS_H
#define JSON_TO_LUOS_H

#include "luos.h"
#include "stdint.h"

extern volatile char detection_ask;

void json_to_luos(container_t *container);

#endif /* JSON_TO_LUOS_H */
