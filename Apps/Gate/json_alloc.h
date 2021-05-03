#ifndef JSON_ALLOC_H
#define JSON_ALLOC_H

#define JSON_BUFF_SIZE  1024
#define JSON_RX_BUF_NUM 3
#define JSON_TX_BUF_NUM 3

void json_alloc_reinit(void);

char *json_alloc_get_rx_buf(void);
char *json_alloc_get_rx_task(void);
char *json_alloc_pull_rx_task(void);
char *json_alloc_set_rx_task(uint16_t carac_nbr);

char *json_alloc_get_tx_buf(void);
char *json_alloc_get_tx_task(void);
char *json_alloc_pull_tx_task(void);
char *json_alloc_set_tx_task(uint16_t carac_nbr);

#endif /* JSON_ALLOC_H */
