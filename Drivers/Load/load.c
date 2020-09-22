/******************************************************************************
 * @file load
 * @brief driver example a simple load
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "load.h"
#include "HX711.h"
#include "string.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

/*******************************************************************************
 * Variables
 ******************************************************************************/
uint8_t new_data_ready = 0;
volatile force_t load = 0.0;
char have_to_tare = 0;

/*******************************************************************************
 * Function
 ******************************************************************************/
static void Load_MsgHandler(module_t *module, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Load_Init(void)
{
    hx711_init(128);
    Luos_CreateModule(Load_MsgHandler, LOAD_MOD, "load_mod", STRINGIFY(VERSION));
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Load_Loop(void)
{
    if (hx711_is_ready())
    {
        load = hx711_get_units(1);
        new_data_ready = 1;
    }
    if (have_to_tare)
    {
        hx711_tare(10);
        have_to_tare = 0;
    }
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this module
 * @param Module destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Load_MsgHandler(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == ASK_PUB_CMD)
    {
        if (new_data_ready)
        {
            msg_t pub_msg;
            // fill the message infos
            pub_msg.header.target_mode = ID;
            pub_msg.header.target = msg->header.source;
            ForceOD_ForceToMsg((force_t *)&load, &pub_msg);
            Luos_SendMsg(module, &pub_msg);
            new_data_ready = 0;
        }
        return;
    }
    if (msg->header.cmd == REINIT)
    {
        // tare
        have_to_tare = 1;
        return;
    }
    if (msg->header.cmd == RESOLUTION)
    {
        // put this value in scale
        float value = 0.0;
        memcpy(&value, msg->data, sizeof(value));
        hx711_set_scale(value);
        return;
    }
    if (msg->header.cmd == OFFSET)
    {
        // offset the load measurement using the scale parameter
        force_t value = 0.0;
        ForceOD_ForceFromMsg(&value, msg);
        hx711_set_offset((long)(value * hx711_get_scale()));
        return;
    }
}
