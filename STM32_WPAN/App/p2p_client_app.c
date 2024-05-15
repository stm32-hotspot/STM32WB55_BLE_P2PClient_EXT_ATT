/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    p2p_client_app.c
  * @author  MCD Application Team
  * @brief   peer to peer Client Application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "app_common.h"

#include "dbg_trace.h"

#include "ble.h"
#include "p2p_client_app.h"

#include "stm32_seq.h"
#include "app_ble.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/

typedef enum
{
  P2P_START_TIMER_EVT,
  P2P_STOP_TIMER_EVT,
  P2P_NOTIFICATION_INFO_RECEIVED_EVT,
} P2P_Client_Opcode_Notification_evt_t;

typedef struct
{
  uint8_t * pPayload;
  uint8_t     Length;
}P2P_Client_Data_t;

typedef struct
{
  P2P_Client_Opcode_Notification_evt_t  P2P_Client_Evt_Opcode;
  P2P_Client_Data_t DataTransfered;
}P2P_Client_App_Notification_evt_t;

typedef struct
{
  /**
   * state of the P2P Client
   * state machine
   */
  APP_BLE_ConnStatus_t state;

  /**
   * connection handle
   */
  uint16_t connHandle;

  /**
   * handle of the P2P service
   */
  uint16_t P2PServiceHandle;

  /**
   * end handle of the P2P service
   */
  uint16_t P2PServiceEndHandle;

  /**
   * handle of the Tx characteristic - Write To Server
   *
   */
  uint16_t P2PWriteToServerCharHdle;

  /**
   * handle of the client configuration
   * descriptor of Tx characteristic
   */
  uint16_t P2PWriteToServerDescHandle;

  /**
   * handle of the Rx characteristic - Notification From Server
   *
   */
  uint16_t P2PNotificationCharHdle;

  /**
   * handle of the client configuration
   * descriptor of Rx characteristic
   */
  uint16_t P2PNotificationDescHandle;

}P2P_ClientContext_t;

/* USER CODE BEGIN PTD */
typedef struct{
  uint8_t                                     Device_Led_Selection;
  uint8_t                                     Led1;
}P2P_LedCharValue_t;

typedef struct{
  uint8_t                                     Device_Button_Selection;
  uint8_t                                     Button1;
}P2P_ButtonCharValue_t;

typedef struct
{

  uint8_t       Notification_Status; /* used to check if P2P Server is enabled to Notify */

  P2P_LedCharValue_t         LedControl;
  P2P_ButtonCharValue_t      ButtonStatus;

  uint16_t ConnectionHandle; 


} P2P_Client_App_Context_t;

/* USER CODE END PTD */

/* Private defines ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros -------------------------------------------------------------*/
#define UNPACK_2_BYTE_PARAMETER(ptr)  \
        (uint16_t)((uint16_t)(*((uint8_t *)ptr))) |   \
        (uint16_t)((((uint16_t)(*((uint8_t *)ptr + 1))) << 8))
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/**
 * START of Section BLE_APP_CONTEXT
 */

static P2P_ClientContext_t aP2PClientContext[BLE_CFG_CLT_MAX_NBR_CB];

/**
 * END of Section BLE_APP_CONTEXT
 */
/* USER CODE BEGIN PV */
static P2P_Client_App_Context_t P2P_Client_App_Context;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void Gatt_Notification(P2P_Client_App_Notification_evt_t *pNotification);
static SVCCTL_EvtAckStatus_t Event_Handler(void *Event);
/* USER CODE BEGIN PFP */
static tBleStatus Write_Char(uint16_t UUID, uint8_t Service_Instance, uint8_t *pPayload);
static tBleStatus Write_Char_Ext(uint16_t UUID, uint8_t Service_Instance, uint8_t *pPayload);
static void Button1_Trigger_Received( void );
static void Button2_Trigger_Received( void );
static void Button3_Trigger_Received( void );
static void Update_Service( void );
/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void P2PC_APP_Init(void)
{
  uint8_t index =0;
/* USER CODE BEGIN P2PC_APP_Init_1 */
  UTIL_SEQ_RegTask( 1<< CFG_TASK_SEARCH_SERVICE_ID, UTIL_SEQ_RFU, Update_Service );
  UTIL_SEQ_RegTask( 1<< CFG_TASK_SW1_BUTTON_PUSHED_ID, UTIL_SEQ_RFU, Button1_Trigger_Received );
  UTIL_SEQ_RegTask( 1<< CFG_TASK_SW2_BUTTON_PUSHED_ID, UTIL_SEQ_RFU, Button2_Trigger_Received );
  UTIL_SEQ_RegTask( 1<< CFG_TASK_SW3_BUTTON_PUSHED_ID, UTIL_SEQ_RFU, Button3_Trigger_Received );

  /**
   * Initialize LedButton Service
   */
  P2P_Client_App_Context.Notification_Status=0;
  P2P_Client_App_Context.ConnectionHandle =  0x00;

  P2P_Client_App_Context.LedControl.Device_Led_Selection=0x00;/* device Led */
  P2P_Client_App_Context.LedControl.Led1=0x00; /* led OFF */
  P2P_Client_App_Context.ButtonStatus.Device_Button_Selection=0x01;/* Device1 */
  P2P_Client_App_Context.ButtonStatus.Button1=0x00;
/* USER CODE END P2PC_APP_Init_1 */
  for(index = 0; index < BLE_CFG_CLT_MAX_NBR_CB; index++)
  {
    aP2PClientContext[index].state= APP_BLE_IDLE;
  }

  /**
   *  Register the event handler to the BLE controller
   */
  SVCCTL_RegisterCltHandler(Event_Handler);

#if(CFG_DEBUG_APP_TRACE != 0)
  APP_DBG_MSG("-- P2P CLIENT INITIALIZED \n");
#endif

/* USER CODE BEGIN P2PC_APP_Init_2 */

/* USER CODE END P2PC_APP_Init_2 */
  return;
}

void P2PC_APP_Notification(P2PC_APP_ConnHandle_Not_evt_t *pNotification)
{
/* USER CODE BEGIN P2PC_APP_Notification_1 */

/* USER CODE END P2PC_APP_Notification_1 */
  switch(pNotification->P2P_Evt_Opcode)
  {
/* USER CODE BEGIN P2P_Evt_Opcode */

/* USER CODE END P2P_Evt_Opcode */

  case PEER_CONN_HANDLE_EVT :
/* USER CODE BEGIN PEER_CONN_HANDLE_EVT */
    P2P_Client_App_Context.ConnectionHandle = pNotification->ConnectionHandle;
/* USER CODE END PEER_CONN_HANDLE_EVT */
      break;

    case PEER_DISCON_HANDLE_EVT :
/* USER CODE BEGIN PEER_DISCON_HANDLE_EVT */
      {
      uint8_t index = 0;
      P2P_Client_App_Context.ConnectionHandle =  0x00;
      while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].state != APP_BLE_IDLE))
      {
        aP2PClientContext[index].state = APP_BLE_IDLE;
      }
      BSP_LED_Off(LED_BLUE); 
        
#if OOB_DEMO == 0
      UTIL_SEQ_SetTask(1<<CFG_TASK_CONN_DEV_1_ID, CFG_SCH_PRIO_0);
#endif 
      }
/* USER CODE END PEER_DISCON_HANDLE_EVT */
      break;

    default:
/* USER CODE BEGIN P2P_Evt_Opcode_Default */

/* USER CODE END P2P_Evt_Opcode_Default */
      break;
  }
/* USER CODE BEGIN P2PC_APP_Notification_2 */

/* USER CODE END P2PC_APP_Notification_2 */
  return;
}
/* USER CODE BEGIN FD */
void P2PC_APP_SW1_Button_Action(void)
{
  UTIL_SEQ_SetTask(1<<CFG_TASK_SW1_BUTTON_PUSHED_ID, CFG_SCH_PRIO_0);
}

void P2PC_APP_SW2_Button_Action(void)
{
  UTIL_SEQ_SetTask(1<<CFG_TASK_SW2_BUTTON_PUSHED_ID, CFG_SCH_PRIO_0);
}

void P2PC_APP_SW3_Button_Action(void)
{
  UTIL_SEQ_SetTask(1<<CFG_TASK_SW3_BUTTON_PUSHED_ID, CFG_SCH_PRIO_0);
}
/* USER CODE END FD */

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/

/**
 * @brief  Event handler
 * @param  Event: Address of the buffer holding the Event
 * @retval Ack: Return whether the Event has been managed or not
 */
static SVCCTL_EvtAckStatus_t Event_Handler(void *Event)
{
  SVCCTL_EvtAckStatus_t return_value;
  hci_event_pckt *event_pckt;
  evt_blecore_aci *blecore_evt;

  P2P_Client_App_Notification_evt_t Notification;

  return_value = SVCCTL_EvtNotAck;
  event_pckt = (hci_event_pckt *)(((hci_uart_pckt*)Event)->data);

  switch(event_pckt->evt)
  {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
    {
      blecore_evt = (evt_blecore_aci*)event_pckt->data;
      switch(blecore_evt->ecode)
      {

        case ACI_ATT_READ_BY_GROUP_TYPE_RESP_VSEVT_CODE:
        {
          aci_att_read_by_group_type_resp_event_rp0 *pr = (void*)blecore_evt->data;
          uint8_t numServ, i, idx;
          uint16_t uuid, handle;

          uint8_t index;
          handle = pr->Connection_Handle;
          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].state != APP_BLE_IDLE))
          {
            APP_BLE_ConnStatus_t status;

            status = APP_BLE_Get_Client_Connection_Status(aP2PClientContext[index].connHandle);

            if((aP2PClientContext[index].state == APP_BLE_CONNECTED_CLIENT)&&
                    (status == APP_BLE_IDLE))
            {
              /* Handle deconnected */

              aP2PClientContext[index].state = APP_BLE_IDLE;
              aP2PClientContext[index].connHandle = 0xFFFF;
              break;
            }
            index++;
          }

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {
            aP2PClientContext[index].connHandle= handle;

            numServ = (pr->Data_Length) / pr->Attribute_Data_Length;

            /* the event data will be
             * 2bytes start handle
             * 2bytes end handle
             * 2 or 16 bytes data
             * we are interested only if the UUID is 16 bit.
             * So check if the data length is 6
             */
#if (UUID_128BIT_FORMAT==1)
          if (pr->Attribute_Data_Length == 20)
          {
            idx = 16;
#else
          if (pr->Attribute_Data_Length == 6)
          {
            idx = 4;
#endif
              for (i=0; i<numServ; i++)
              {
                uuid = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx]);
                if(uuid == P2P_SERVICE_UUID)
                {
#if(CFG_DEBUG_APP_TRACE != 0)
                  APP_DBG_MSG("-- GATT : P2P_SERVICE_UUID FOUND - connection handle 0x%x \n", aP2PClientContext[index].connHandle);
#endif
#if (UUID_128BIT_FORMAT==1)
                aP2PClientContext[index].P2PServiceHandle = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx-16]);
                aP2PClientContext[index].P2PServiceEndHandle = UNPACK_2_BYTE_PARAMETER (&pr->Attribute_Data_List[idx-14]);
#else
                aP2PClientContext[index].P2PServiceHandle = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx-4]);
                aP2PClientContext[index].P2PServiceEndHandle = UNPACK_2_BYTE_PARAMETER (&pr->Attribute_Data_List[idx-2]);
#endif
                  aP2PClientContext[index].state = APP_BLE_DISCOVER_CHARACS ;
                }
                idx += 6;
              }
            }
          }
        }
        break;

        case ACI_ATT_READ_BY_TYPE_RESP_VSEVT_CODE:
        {

          aci_att_read_by_type_resp_event_rp0 *pr = (void*)blecore_evt->data;
          uint8_t idx;
          uint16_t uuid, handle;

          /* the event data will be
           * 2 bytes start handle
           * 1 byte char properties
           * 2 bytes handle
           * 2 or 16 bytes data
           */

          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))
            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            /* we are interested in only 16 bit UUIDs */
#if (UUID_128BIT_FORMAT==1)
            idx = 17;
            if (pr->Handle_Value_Pair_Length == 21)
#else
              idx = 5;
            if (pr->Handle_Value_Pair_Length == 7)
#endif
            {
              pr->Data_Length -= 1;
              while(pr->Data_Length > 0)
              {
                uuid = UNPACK_2_BYTE_PARAMETER(&pr->Handle_Value_Pair_Data[idx]);
                /* store the characteristic handle not the attribute handle */
#if (UUID_128BIT_FORMAT==1)
                handle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_Value_Pair_Data[idx-14]);
#else
                handle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_Value_Pair_Data[idx-2]);
#endif
                if(uuid == P2P_WRITE_CHAR_UUID)
                {
#if(CFG_DEBUG_APP_TRACE != 0)
                  APP_DBG_MSG("-- GATT : WRITE_UUID FOUND - connection handle 0x%x\n", aP2PClientContext[index].connHandle);
#endif
                  aP2PClientContext[index].state = APP_BLE_DISCOVER_WRITE_DESC;
                  aP2PClientContext[index].P2PWriteToServerCharHdle = handle;
                }

                else if(uuid == P2P_NOTIFY_CHAR_UUID)
                {
#if(CFG_DEBUG_APP_TRACE != 0)
                  APP_DBG_MSG("-- GATT : NOTIFICATION_CHAR_UUID FOUND  - connection handle 0x%x\n", aP2PClientContext[index].connHandle);
#endif
                  aP2PClientContext[index].state = APP_BLE_DISCOVER_NOTIFICATION_CHAR_DESC;
                  aP2PClientContext[index].P2PNotificationCharHdle = handle;
                }
#if (UUID_128BIT_FORMAT==1)
                pr->Data_Length -= 21;
                idx += 21;
#else
                pr->Data_Length -= 7;
                idx += 7;
#endif
              }
            }
          }
        }
        break;

        case ACI_ATT_FIND_INFO_RESP_VSEVT_CODE:
        {
          aci_att_find_info_resp_event_rp0 *pr = (void*)blecore_evt->data;

          uint8_t numDesc, idx, i;
          uint16_t uuid, handle;

          /*
           * event data will be of the format
           * 2 bytes handle
           * 2 bytes UUID
           */

          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))

            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            numDesc = (pr->Event_Data_Length) / 4;
            /* we are interested only in 16 bit UUIDs */
            idx = 0;
            if (pr->Format == UUID_TYPE_16)
            {
              for (i=0; i<numDesc; i++)
              {
                handle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_UUID_Pair[idx]);
                uuid = UNPACK_2_BYTE_PARAMETER(&pr->Handle_UUID_Pair[idx+2]);

                if(uuid == CLIENT_CHAR_CONFIG_DESCRIPTOR_UUID)
                {
#if(CFG_DEBUG_APP_TRACE != 0)
                  APP_DBG_MSG("-- GATT : CLIENT_CHAR_CONFIG_DESCRIPTOR_UUID- connection handle 0x%x\n", aP2PClientContext[index].connHandle);
#endif
                  if( aP2PClientContext[index].state == APP_BLE_DISCOVER_NOTIFICATION_CHAR_DESC)
                  {

                    aP2PClientContext[index].P2PNotificationDescHandle = handle;
                    aP2PClientContext[index].state = APP_BLE_ENABLE_NOTIFICATION_DESC;

                  }
                }
                idx += 4;
              }
            }
          }
        }
        break; /*ACI_ATT_FIND_INFO_RESP_VSEVT_CODE*/

        case ACI_GATT_NOTIFICATION_VSEVT_CODE:
        {
          aci_gatt_notification_event_rp0 *pr = (void*)blecore_evt->data;
          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))
            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            if ( (pr->Attribute_Handle == aP2PClientContext[index].P2PNotificationCharHdle) &&
                    (pr->Attribute_Value_Length == (2)) )
            {

              Notification.P2P_Client_Evt_Opcode = P2P_NOTIFICATION_INFO_RECEIVED_EVT;
              Notification.DataTransfered.Length = pr->Attribute_Value_Length;
              Notification.DataTransfered.pPayload = &pr->Attribute_Value[0];

              Gatt_Notification(&Notification);

              /* INFORM APPLICATION BUTTON IS PUSHED BY END DEVICE */

            }
          }
        }
        break;/* end ACI_GATT_NOTIFICATION_VSEVT_CODE */
        
        case ACI_GATT_NOTIFICATION_EXT_VSEVT_CODE:
        {
          aci_gatt_notification_ext_event_rp0 *pr = (void*)blecore_evt->data;
          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))
            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            if (pr->Attribute_Handle == aP2PClientContext[index].P2PNotificationCharHdle)
            {

              Notification.P2P_Client_Evt_Opcode = P2P_NOTIFICATION_INFO_RECEIVED_EVT;
              Notification.DataTransfered.Length = pr->Attribute_Value_Length;
              Notification.DataTransfered.pPayload = &pr->Attribute_Value[0];

              Gatt_Notification(&Notification);

              /* INFORM APPLICATION BUTTON IS PUSHED BY END DEVICE */

            }
          }
        }
        break;/* end ACI_GATT_NOTIFICATION_EXT_VSEVT_CODE */

        case ACI_GATT_PROC_COMPLETE_VSEVT_CODE:
        {
          aci_gatt_proc_complete_event_rp0 *pr = (void*)blecore_evt->data;
#if(CFG_DEBUG_APP_TRACE != 0)
          APP_DBG_MSG("-- GATT : ACI_GATT_PROC_COMPLETE_VSEVT_CODE \n");
          APP_DBG_MSG("\n");
#endif

          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))
            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            UTIL_SEQ_SetTask( 1<<CFG_TASK_SEARCH_SERVICE_ID, CFG_SCH_PRIO_0);

          }
        }
        break; /*ACI_GATT_PROC_COMPLETE_VSEVT_CODE*/
        
        case ACI_ATT_PREPARE_WRITE_RESP_VSEVT_CODE:
        {
#if(CFG_DEBUG_APP_TRACE != 0)
          APP_DBG_MSG("-- GATT : ACI_ATT_PREPARE_WRITE_RESP_VSEVT_CODE\r\n");
#endif       
          UTIL_SEQ_SetEvt(1<< CFG_PREPARE_WRITE_EVT_RSP_ID);
        }
        break; /* end ACI_ATT_PREPARE_WRITE_RESP_VSEVT_CODE */
          
        case ACI_ATT_EXEC_WRITE_RESP_VSEVT_CODE:
        {
#if(CFG_DEBUG_APP_TRACE != 0)
          APP_DBG_MSG("-- GATT : ACI_ATT_EXEC_WRITE_RESP_VSEVT_CODE\r\n");
#endif          
          UTIL_SEQ_SetEvt(1<< CFG_EXECUTE_WRITE_EVT_RSP_ID);
        }
        break; /* end ACI_ATT_EXEC_WRITE_RESP_VSEVT_CODE */
        
        case ACI_GATT_READ_EXT_VSEVT_CODE:
        {
          APP_DBG_MSG("ACI_GATT_READ_EXT_VSEVT_CODE \r\n");
          aci_gatt_read_ext_event_rp0 *read_ext;
          read_ext = (aci_gatt_read_ext_event_rp0 *)blecore_evt->data;
          APP_DBG_MSG("-- P2P APPLICATION CLIENT : EXTENDED READ RESPONSE RECEIVED\r\n");
          APP_DBG_MSG("** Extended Read Data (Length: %d | Offset: %x): 0x", read_ext->Event_Data_Length, read_ext->Offset);
          for (uint8_t i = 0; i < read_ext->Event_Data_Length; i++)
          {
            APP_DBG_MSG("%02X ", read_ext->Attribute_Value[i]);
          }
          APP_DBG_MSG("\r\n\r\n");
          
          // Delay to Give Time for Log Message to Print (10 ms)
          HAL_Delay(10);
        }
        break; /* end ACI_GATT_READ_EXT_VSEVT_CODE */
        
        default:
          break;
      }
    }

    break; /* HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE */

    default:
      break;
  }

  return(return_value);
}/* end BLE_CTRL_Event_Acknowledged_Status_t */

void Gatt_Notification(P2P_Client_App_Notification_evt_t *pNotification)
{
/* USER CODE BEGIN Gatt_Notification_1*/

/* USER CODE END Gatt_Notification_1 */
  switch(pNotification->P2P_Client_Evt_Opcode)
  {
/* USER CODE BEGIN P2P_Client_Evt_Opcode */

/* USER CODE END P2P_Client_Evt_Opcode */

    case P2P_NOTIFICATION_INFO_RECEIVED_EVT:
/* USER CODE BEGIN P2P_NOTIFICATION_INFO_RECEIVED_EVT */
    {
      P2P_Client_App_Context.LedControl.Device_Led_Selection=pNotification->DataTransfered.pPayload[0];
      switch(P2P_Client_App_Context.LedControl.Device_Led_Selection) {

        case 0x01 : {

          P2P_Client_App_Context.LedControl.Led1=pNotification->DataTransfered.pPayload[1];

          if(P2P_Client_App_Context.LedControl.Led1==0x00){
            BSP_LED_Off(LED_BLUE);
          } else {
            BSP_LED_On(LED_BLUE);
          }
          
          APP_DBG_MSG("-- P2P APPLICATION CLIENT : NOTIFICATION RECEIVED\r\n");
          APP_DBG_MSG("** Notification Data: 0x%02X %02X\r\n", pNotification->DataTransfered.pPayload[0], pNotification->DataTransfered.pPayload[1]);
          APP_DBG_MSG("\r\n");
          
          break;
        }
        
        default : {
          
          APP_DBG_MSG("-- P2P APPLICATION CLIENT : EXTENDED NOTIFICATION RECEIVED\r\n");
          APP_DBG_MSG("** Extended Notification Data (Length: %d): 0x", pNotification->DataTransfered.Length);
          for (uint16_t i = 0; i < pNotification->DataTransfered.Length; i++)
          {
            APP_DBG_MSG("%02X ", pNotification->DataTransfered.pPayload[i]);
          }
          APP_DBG_MSG("\r\n\r\n");
          
          // Delay to Give Time for Log Message to Print (5 ms)
          HAL_Delay(5);
          
          break;
        }
      }

    }
/* USER CODE END P2P_NOTIFICATION_INFO_RECEIVED_EVT */
      break;

    default:
/* USER CODE BEGIN P2P_Client_Evt_Opcode_Default */

/* USER CODE END P2P_Client_Evt_Opcode_Default */
      break;
  }
/* USER CODE BEGIN Gatt_Notification_2*/

/* USER CODE END Gatt_Notification_2 */
  return;
}

uint8_t P2P_Client_APP_Get_State( void ) {
  return aP2PClientContext[0].state;
}
/* USER CODE BEGIN LF */
/**
 * @brief  Feature Characteristic update
 * @param  pFeatureValue: The address of the new value to be written
 * @retval None
 */
tBleStatus Write_Char(uint16_t UUID, uint8_t Service_Instance, uint8_t *pPayload)
{
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  uint8_t index;

  index = 0;
  while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
          (aP2PClientContext[index].state != APP_BLE_IDLE))
  {
    switch(UUID)
    {
      case P2P_WRITE_CHAR_UUID: /* SERVER RX -- so CLIENT TX */
        ret = aci_gatt_write_without_resp(aP2PClientContext[index].connHandle,
                                         aP2PClientContext[index].P2PWriteToServerCharHdle,
                                         2, /* charValueLen */
                                         (uint8_t *)  pPayload);
        break;
      default:
        break;
    }
    index++;
  }

  return ret;
}/* end Write_Char() */

/**
 * @brief  Feature Characteristic update
 * @param  pFeatureValue: The address of the new value to be written
 * @retval None
 */
tBleStatus Write_Char_Ext(uint16_t UUID, uint8_t Service_Instance, uint8_t *pPayload)
{
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  uint8_t index;

  index = 0;
  while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
          (aP2PClientContext[index].state != APP_BLE_IDLE))
  {
    switch(UUID)
    {
      case P2P_WRITE_CHAR_UUID:      
        
        ret = aci_att_prepare_write_req(aP2PClientContext[index].connHandle,aP2PClientContext[index].P2PWriteToServerCharHdle,0,242,(uint8_t *)  pPayload);
        UTIL_SEQ_WaitEvt(1<< CFG_PREPARE_WRITE_EVT_RSP_ID);
        APP_DBG_MSG("  aci_att_prepare_write_req : %x\r\n\r\n", ret);
        
        ret = aci_att_prepare_write_req(aP2PClientContext[index].connHandle,aP2PClientContext[index].P2PWriteToServerCharHdle,242,242,(uint8_t *)  pPayload+242);
        UTIL_SEQ_WaitEvt(1<< CFG_PREPARE_WRITE_EVT_RSP_ID);
        APP_DBG_MSG("  aci_att_prepare_write_req : %x\r\n\r\n", ret);
        
        ret = aci_att_prepare_write_req(aP2PClientContext[index].connHandle,aP2PClientContext[index].P2PWriteToServerCharHdle,484,25,(uint8_t *)  pPayload+484);
        UTIL_SEQ_WaitEvt(1<< CFG_PREPARE_WRITE_EVT_RSP_ID);
        APP_DBG_MSG("  aci_att_prepare_write_req : %x\r\n\r\n", ret);
        
        ret = aci_att_execute_write_req(aP2PClientContext[index].connHandle,0x01);
        UTIL_SEQ_WaitEvt(1<< CFG_EXECUTE_WRITE_EVT_RSP_ID);
        APP_DBG_MSG("  aci_att_execute_write_req : %x\r\n\r\n", ret);
        
        break;
      default:
        break;
    }
    index++;
  }

  return ret;
}/* end Write_Char_Ext() */

void Button1_Trigger_Received(void)
{
  APP_DBG_MSG("-- P2P APPLICATION CLIENT : BUTTON 1 PUSHED - WRITE TO SERVER\n");
  APP_DBG_MSG("\r\n");
  if(P2P_Client_App_Context.ButtonStatus.Button1 == 0x00)
  {
    P2P_Client_App_Context.ButtonStatus.Button1 = 0x01;
  }else {
    P2P_Client_App_Context.ButtonStatus.Button1 = 0x00;
  }

  Write_Char( P2P_WRITE_CHAR_UUID, 0, (uint8_t *)&P2P_Client_App_Context.ButtonStatus);

  return;
}

void Button2_Trigger_Received(void)
{
  APP_DBG_MSG("-- P2P APPLICATION CLIENT : BUTTON 2 PUSHED - EXTENDED WRITE TO SERVER\n ");
  APP_DBG_MSG(" \n\r");
  
  uint8_t payload[509];
  uint16_t length = 509;
  
  for (uint16_t i = 0; i < length; i++)
  {
    if (i < 254)
    {
      payload[i] = i + 1;
    }
    else
    {
      payload[i] = 0x0A;
    }
  }

  Write_Char_Ext( P2P_WRITE_CHAR_UUID, 0, (uint8_t *)&payload);

  return;
}

void Button3_Trigger_Received(void)
{
  APP_DBG_MSG("-- P2P APPLICATION CLIENT : BUTTON 3 PUSHED - EXTENDED READ TO SERVER\n ");
  APP_DBG_MSG(" \n\r");
  
  tBleStatus status = aci_gatt_read_char_value(aP2PClientContext[0].connHandle, aP2PClientContext[0].P2PWriteToServerCharHdle);
  
  if (status != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_read_char_value command, result: 0x%x \n\r", status);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gatt_read_char_value command\n\r");
  }
  
  return;
}

void Update_Service()
{
  uint16_t enable = 0x0001;
  uint16_t disable = 0x0000;
  uint8_t index;

  index = 0;
  while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
          (aP2PClientContext[index].state != APP_BLE_IDLE))
  {
    switch(aP2PClientContext[index].state)
    {
      case APP_BLE_DISCOVER_SERVICES:
        APP_DBG_MSG("P2P_DISCOVER_SERVICES\n");
        break;
      case APP_BLE_DISCOVER_CHARACS:
        APP_DBG_MSG("* GATT : Discover P2P Characteristics\n");
        aci_gatt_disc_all_char_of_service(aP2PClientContext[index].connHandle,
                                          aP2PClientContext[index].P2PServiceHandle,
                                          aP2PClientContext[index].P2PServiceEndHandle);
        break;
      case APP_BLE_DISCOVER_WRITE_DESC: /* Not Used - No descriptor */
        APP_DBG_MSG("* GATT : Discover Descriptor of TX - Write  Characteristic\n");
        aci_gatt_disc_all_char_desc(aP2PClientContext[index].connHandle,
                                    aP2PClientContext[index].P2PWriteToServerCharHdle,
                                    aP2PClientContext[index].P2PWriteToServerCharHdle+2);
        break;
      case APP_BLE_DISCOVER_NOTIFICATION_CHAR_DESC:
        APP_DBG_MSG("* GATT : Discover Descriptor of Rx - Notification  Characteristic\n");
        aci_gatt_disc_all_char_desc(aP2PClientContext[index].connHandle,
                                    aP2PClientContext[index].P2PNotificationCharHdle,
                                    aP2PClientContext[index].P2PNotificationCharHdle+2);
        break;
      case APP_BLE_ENABLE_NOTIFICATION_DESC:
        APP_DBG_MSG("* GATT : Enable Server Notification\n");
        aci_gatt_write_char_desc(aP2PClientContext[index].connHandle,
                                 aP2PClientContext[index].P2PNotificationDescHandle,
                                 2,
                                 (uint8_t *)&enable);
        aP2PClientContext[index].state = APP_BLE_CONNECTED_CLIENT;
        BSP_LED_Off(LED_RED);
        break;
      case APP_BLE_DISABLE_NOTIFICATION_DESC :
        APP_DBG_MSG("* GATT : Disable Server Notification\n");
        aci_gatt_write_char_desc(aP2PClientContext[index].connHandle,
                                 aP2PClientContext[index].P2PNotificationDescHandle,
                                 2,
                                 (uint8_t *)&disable);
        aP2PClientContext[index].state = APP_BLE_CONNECTED_CLIENT;
        break;
      default:
        break;
    }
    index++;
  }
  return;
}
/* USER CODE END LF */
