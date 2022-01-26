/******************************************************************************
 * @file biometric_security
 * @brief This is an app exemple for a Biometric Security System. It won't work 
 * as is so if you want to see it inside of a project, I made multiple version
 * that you can go check on my github :
 * https://github.com/mariebidouille
 * @author MarieBidouille
 * @version 0.0.0
 ******************************************************************************/

//#include "product_config.h" //if your are using it into a project you need to uncomment this line 
#include "biometric_security.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef enum 
{
    BIOMETRIC_SECURITY_TYPE = LUOS_LAST_TYPE
}security_system_type_t;

#define UPDATE_PERIOD_MS 200

/*******************************************************************************
 * Variables
 ******************************************************************************/
static service_t *app;
static angular_position_t angle;

uint8_t enroll_last_state;
uint8_t delete_last_state;
uint8_t up_last_state;
uint8_t down_last_state;

uint8_t up_ask = 0;
uint8_t fingerprint_busy = 0;

/*******************************************************************************
 * Functions
 ******************************************************************************/
void BiometricSecurity_MsgHandler(service_t *service, msg_t *msg);

uint8_t BiometricSecurity_SetServoPosition(void);
uint8_t BiometricSecurity_CheckFingerprint(void);
uint8_t BiometricSecurity_ControlLed(uint8_t green_state);
uint8_t BiometricSecurity_LcdPrint(char* text, uint16_t length);

/******************************************************************************
 * @brief init must be call in service init
 * @param None
 * @return None
 ******************************************************************************/
void BiometricSecurity_Init(void)
{
    revision_t revision = {0};
    
    app = Luos_CreateService(BiometricSecurity_MsgHandler, BIOMETRIC_APP, "Garage", revision);

    Luos_Detect(app);
}

/******************************************************************************
 * @brief loop must be call in service loop
 * @param None
 * @return None
 ******************************************************************************/
void BiometricSecurity_Loop(void)
{
    static uint32_t detection_date = 0;
    static uint8_t system_init = 0;

    if (Luos_IsNodeDetected()&&!system_init)
    { 
        int id_btn[4];
        id_btn[0] = RoutingTB_IDFromAlias("btn_enroll");
        id_btn[1] = RoutingTB_IDFromAlias("btn_delete"); 
        id_btn[2] = RoutingTB_IDFromAlias("btn_up"); 
        id_btn[3] = RoutingTB_IDFromAlias("btn_down"); 

        for (int i=0; i<4; i++)
        {
            if (id_btn[i]>0)
            {
                msg_t pub_msg;
                pub_msg.header.target = id_btn[i];
                pub_msg.header.target_mode = IDACK;
                time_luos_t time = TimeOD_TimeFrom_ms(UPDATE_PERIOD_MS);
                TimeOD_TimeToMsg(&time, &pub_msg);
                pub_msg.header.cmd = UPDATE_PUB;
                Luos_SendMsg(app, &pub_msg);
            }
        }

        if (RoutingTB_IDFromType(LCD_TYPE)>0)
        {
            msg_t pub_msg;
            pub_msg.header.target = RoutingTB_IDFromType(LCD_TYPE);
            pub_msg.header.target_mode = ID;
            pub_msg.header.cmd = REINIT;
            Luos_SendMsg(app, &pub_msg);

            BiometricSecurity_LcdPrint("Security System", sizeof("Security System")-1);
        }

        angle = 0;
        BiometricSecurity_ControlLed(0);
        BiometricSecurity_SetServoPosition();

        system_init = 1;
    }
    return;
}

/******************************************************************************
 * @brief message handler must be call in service init 
 * @param service
 * @param msg
 * @return None
 ******************************************************************************/
void BiometricSecurity_MsgHandler(service_t *service, msg_t *msg)
{
    if ((msg->header.cmd == IO_STATE)&&(RoutingTB_TypeFromID(msg->header.source) == STATE_TYPE))
    {
        if (RoutingTB_IDFromAlias("btn_enroll") == msg->header.source)
        {
            if ((!enroll_last_state)&&(enroll_last_state != msg->data[0]))
            {
                if ((!fingerprint_busy)&&(angle == 0))
                {
                    BiometricSecurity_CheckFingerprint();
                }
            }
            enroll_last_state = msg->data[0];
        }

        if (RoutingTB_IDFromAlias("btn_up") == msg->header.source) 
        {
            if ((!up_last_state)&&(up_last_state != msg->data[0]))
            {
                if ((!fingerprint_busy)&&(angle == 0))
                {
                    up_ask = 1;
                    BiometricSecurity_CheckFingerprint();
                }
            }
            up_last_state = msg->data[0];
        }

        if (RoutingTB_IDFromAlias("btn_down") == msg->header.source)
        {
            if ((!down_last_state)&&(down_last_state != msg->data[0])&&(!fingerprint_busy))
            {
                angle = 0;
                if (BiometricSecurity_SetServoPosition())
                {
                    BiometricSecurity_LcdPrint("Door closed", sizeof("Door closed")-1);
                }
            }
            down_last_state = msg->data[0];
        }

        if (RoutingTB_IDFromAlias("btn_delete") == msg->header.source)
        {
            if ((!delete_last_state)&&(delete_last_state != msg->data[0]))
            {
                if ((!fingerprint_busy)&&(angle == 0))
                {    
                    if (RoutingTB_IDFromType(FINGERPRINT_TYPE)>0)
                    {
                        msg_t pub_msg;
                        pub_msg.header.target_mode = msg->header.target_mode;
                        pub_msg.header.cmd = DELETE;
                        pub_msg.header.target = RoutingTB_IDFromType(FINGERPRINT_TYPE);
                        fingerprint_busy = 1;
                        BiometricSecurity_LcdPrint("Checking auth..", sizeof("Checking auth..")-1);
                        Luos_SendMsg(app, &pub_msg);
                    }
                    else
                    {
                        BiometricSecurity_LcdPrint("Can't find sensor",sizeof("Can't find sensor")-1);
                    }
                }
            }
            delete_last_state = msg->data[0];
        }
    }

    if (RoutingTB_TypeFromID(msg->header.source) == FINGERPRINT_TYPE)
    {
        fingerprint_busy = 0;
        switch (msg->header.cmd)
        {
        case ENROLL:
            (msg->data[0]) ? BiometricSecurity_LcdPrint("Finger saved !",sizeof("Finger saved !")-1) : BiometricSecurity_LcdPrint("Reg failed :(",sizeof("Reg failed :(")-1);
            break;

        case DELETE:
            (msg->data[0]) ? BiometricSecurity_LcdPrint("Fingers deleted",sizeof("Fingers deleted")-1) : BiometricSecurity_LcdPrint("Database intact",sizeof("Database intact")-1);
            break;

        case CHECK:
            if (msg->data[0])
            {
                if (up_ask)
                {
                    up_ask = 0;
                    angle = AngularOD_PositionFrom_deg(90);
                    if (BiometricSecurity_SetServoPosition())  BiometricSecurity_LcdPrint("Door open.", sizeof("Door open.")-1);
                }
                else
                {
                    if (RoutingTB_IDFromType(FINGERPRINT_TYPE)>0)
                    {
                        msg_t pub_msg;
                        pub_msg.header.target_mode = msg->header.target_mode;
                        pub_msg.header.target = msg->header.source;
                        pub_msg.header.cmd = ENROLL;
                        fingerprint_busy = 1;
                        BiometricSecurity_LcdPrint("Place new finger",sizeof("Place new finger")-1);
                        Luos_SendMsg(app, &pub_msg);
                    }
                    else
                    {
                        BiometricSecurity_LcdPrint("Can't find sensor",sizeof("Can't find sensor")-1);
                    }
                }
            }
            else
            {
                BiometricSecurity_LcdPrint("Auth failed :(",sizeof("Auth failed :(")-1);
                up_ask = 0;
            }
            break;

        default:
            BiometricSecurity_LcdPrint("Something wrong",sizeof("Something wrong")-1);
            break;
        }
    }
}

/******************************************************************************
 * @brief Function to open/close the door
 * @param None
 * @return whether or not the message was sent successfully
 ******************************************************************************/
uint8_t BiometricSecurity_SetServoPosition(void)
{
    if (RoutingTB_IDFromType(SERVO_MOTOR_TYPE)>0)
    {
        msg_t servo_msg;
        servo_msg.header.target_mode = ID;
        servo_msg.header.target = RoutingTB_IDFromType(SERVO_MOTOR_TYPE);
        
        AngularOD_PositionToMsg(&angle, &servo_msg);
        while(Luos_SendMsg(app, &servo_msg)!=SUCCEED)
        {
            Luos_Loop();
        }

        BiometricSecurity_ControlLed(AngularOD_PositionTo_deg(angle)/90);
        return 1;
    }
    else 
    {
        BiometricSecurity_LcdPrint("Can't find door",sizeof("Can't find door")-1);
        return 0;
    }
}

/******************************************************************************
 * @brief Check if the finger match a fingerprint in the database
 * @param None
 * @return whether or not the message was sent successfully
 ******************************************************************************/
uint8_t BiometricSecurity_CheckFingerprint(void)
{
    if ((RoutingTB_IDFromType(FINGERPRINT_TYPE)>0)&&(!fingerprint_busy))
    {
        
        BiometricSecurity_LcdPrint("Checking auth..", sizeof("Checking auth..")-1);
        msg_t fing_msg;
        fing_msg.header.cmd = CHECK;
        fing_msg.header.target_mode = ID;
        fing_msg.header.target = RoutingTB_IDFromType(FINGERPRINT_TYPE);
        fingerprint_busy = 1;
        while(Luos_SendMsg(app, &fing_msg)!=SUCCEED)
        {
            Luos_Loop();
        }
        return 1;
    }
    else
    {
        BiometricSecurity_LcdPrint("Can't find sensor", sizeof("Can't find sensor")-1);
        return 0;
    }
}

/******************************************************************************
 * @brief Function to control the led color
 * @param green_state
 * @return whether or not the message was sent successfully
 ******************************************************************************/
uint8_t BiometricSecurity_ControlLed(uint8_t green_state)
{
    msg_t led_msg;
    led_msg.header.size = sizeof(char);
    led_msg.header.target_mode = ID;
    led_msg.header.cmd = IO_STATE;
    if (RoutingTB_IDFromAlias("led_green")>0)
    {
        led_msg.header.target = RoutingTB_IDFromAlias("led_green");
        led_msg.data[0] = green_state;
        while(Luos_SendMsg(app, &led_msg)!=SUCCEED)
        {
            Luos_Loop();
        }
    }
    else return 0;
    if (RoutingTB_IDFromAlias("led_red")>0)
    {
        led_msg.header.target = RoutingTB_IDFromAlias("led_red");
        led_msg.data[0] = 1-green_state;
        while(Luos_SendMsg(app, &led_msg)!=SUCCEED)
        {
            Luos_Loop();
        }
        return 1;
    }
    else return 0;
}

/******************************************************************************
 * @brief Display text on lcd screen 
 * @param text to display
 * @param length of the text to display 
 * @return whether or not the message was sent successfully
 ******************************************************************************/
uint8_t BiometricSecurity_LcdPrint(char* text, uint16_t length)
{
    if (RoutingTB_IDFromType(LCD_TYPE)>0)
    {
        msg_t txt_msg;
        txt_msg.header.cmd = TEXT;
        txt_msg.header.size = length;
        txt_msg.header.target_mode = ID;
        txt_msg.header.target = RoutingTB_IDFromType(LCD_TYPE);
        
        memcpy(txt_msg.data, text, txt_msg.header.size);

        while(Luos_SendMsg(app, &txt_msg) != SUCCEED)
        {
            Luos_Loop();
        }
        return 1;
    }
    else return 0;
}
