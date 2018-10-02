/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

/*------------------------- INCLUDED FILES ************************************/
#include "ssButton.h"
#include "FreeRTOS.h"
#include "bsp.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "ssSysCom.h"
#include "ssFsm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define BUTTON_DAEMON_STACK_SIZE        128
#define BUTTON_DAEMON_PRIORITY          11
#define BUTTON_DAEMON_NAME              "Btn Daemon"
  
#define BUTTON_CRITICAL_SECTION_ENTER() xSemaphoreTake(buttonsTableMutex, portMAX_DELAY/portMAX_DELAY)
#define BUTTON_CRITICAL_SECTION_EXIT() xSemaphoreGive(buttonsTableMutex)

/* notification data format:
  - reserved: bits 31:16
  - event type: bits 15:8
  - button id: bits 7:0
 */
#define BTN_NFC_EVENT_SIZE          8  
#define BTN_NFC_EVENT_OFFSET        8
#define BTN_NFC_EVENT_MASK          (((1<<BTN_NFC_EVENT_SIZE) - 1) << BTN_NFC_EVENT_OFFSET)
#define BTN_NFC_BUTTON_ID_SIZE      8  
#define BTN_NFC_BUTTON_ID_OFFSET    0
#define BTN_NFC_BUTTON_ID_MASK          (((1<<BTN_NFC_BUTTON_ID_SIZE) - 1) << BTN_NFC_BUTTON_ID_OFFSET)

#define BTN_NFC_EVENT_GET(nfc_value)            (((nfc_value) & BTN_NFC_EVENT_MASK) >> BTN_NFC_EVENT_OFFSET) 
#define BTN_NFC_EVENT_SET(nfc_value_ptr, event) ((*(nfc_value_ptr)) = (((event) << BTN_NFC_EVENT_OFFSET) & BTN_NFC_EVENT_MASK) | ((*(nfc_value_ptr)) & (~BTN_NFC_EVENT_MASK))) 
  
#define BTN_NFC_BUTTON_ID_GET(nfc_value)            (((nfc_value) & BTN_NFC_BUTTON_ID_MASK) >> BTN_NFC_BUTTON_ID_OFFSET) 
#define BTN_NFC_BUTTON_ID_SET(nfc_value_ptr, button_id) ((*(nfc_value_ptr)) = (((button_id) << BTN_NFC_BUTTON_ID_OFFSET) & BTN_NFC_BUTTON_ID_MASK) | ((*(nfc_value_ptr)) & (~BTN_NFC_BUTTON_ID_MASK))) 
  
#define BTN_NFC_VALUE_INIT(button_id, event)  ((((button_id) << BTN_NFC_BUTTON_ID_OFFSET) & BTN_NFC_BUTTON_ID_MASK) | (((event) << BTN_NFC_EVENT_OFFSET) & BTN_NFC_EVENT_MASK))
  
#define BTN_PRESS_DEBOUNCE_INTERVAL          (20 / portTICK_RATE_MS)
#define BTN_RELEASE_DEBOUNCE_INTERVAL        (100 / portTICK_RATE_MS)
 
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
   
typedef struct
{
  ssSysComCpidType cpid;
  uint8_t buttonId;
  uint8_t events;
} ssButtonDataType;

typedef enum
{
  BUTTON_FSM_STATE_DISABLED = 0,
  BUTTON_FSM_STATE_RELEASED,
  BUTTON_FSM_STATE_DEBOUNCE_PRESS,
  BUTTON_FSM_STATE_PRESSED,
  BUTTON_FSM_STATE_DEBOUNCE_RELEASED,
} ssButtonFsmStateEnum;
  
typedef enum
{
  BUTTON_FSM_EVENT_DISABLE = FSM_EVENT_USER_START,
  BUTTON_FSM_EVENT_ENABLE,
  BUTTON_FSM_EVENT_PRESS,
  BUTTON_FSM_EVENT_RELEASE,
  BUTTON_FSM_EVENT_PRESS_TIMER,
  BUTTON_FSM_EVENT_RELEASE_TIMER,
} ssButtonFsmEventEnum;
  
typedef struct 
{
  ssFsmType super;
  ssFsmStateType disabled;
  ssFsmStateType press_debounce;
  ssFsmStateType down;
  ssFsmStateType release_debounce;
  ssFsmStateType up;
  /* guard and state common data */
  ssButtonDataType *btn_data;
  TimerHandle_t timer;
} ssButtonFsmType;



typedef struct 
{
  ssButtonFsmType fsm;
  ssButtonDataType data;
} ssButtonType;



/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

TaskHandle_t buttonDaemonHandle = NULL;

ssButtonType buttonsTable[SS_BUTTON_COUNT];
SemaphoreHandle_t buttonsTableMutex = NULL;


/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

void ButtonDaemon(void *argument);
void UserButtonCallback(uint32_t button, uint32_t event);

void ButtonFsmRaiseEvent(uint32_t button, uint32_t event);

/* state machine constructor and state handlers */
static void ButtonFsmCtor(ssButtonFsmType *me, ssButtonDataType *data);
static void ButtonFsmDisabledHandler(ssButtonFsmType *me, ssFsmEventType event);
static void ButtonFsmPressDebounceHandler(ssButtonFsmType *me, ssFsmEventType event);
static void ButtonFsmDownHandler(ssButtonFsmType *me, ssFsmEventType event);
static void ButtonFsmReleaseDebounceHandler(ssButtonFsmType *me, ssFsmEventType event);
static void ButtonFsmUpHandler(ssButtonFsmType *me, ssFsmEventType event);

static void ButtonTimerCallback(TimerHandle_t xTimer);
static void ButtonTimerStart(TimerHandle_t xTimer, uint32_t button, uint32_t event, uint32_t period);

static void ButtonClientNotify(ssSysComCpidType cpid, uint32_t buttonId, uint32_t event);


/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
void ssButtonInit(void)
{
  uint32_t i;
  BaseType_t ret;
 
  for(i=0; i<SS_BUTTON_COUNT; i++)
  {
    buttonsTable[i].data.cpid = SS_SYSCOM_CPID_INVALID;
    buttonsTable[i].data.buttonId = 0;
    buttonsTable[i].data.events = BUTTON_EVENT_NONE;
    ButtonFsmCtor(&buttonsTable[i].fsm, &buttonsTable[i].data);
  }
  
  ret = xTaskCreate(ButtonDaemon,
                    BUTTON_DAEMON_NAME,
                    BUTTON_DAEMON_STACK_SIZE, /* Stack depth in words. */
                    NULL, /* We are not using the task parameter. */
                    BUTTON_DAEMON_PRIORITY, /* This task will run at priority 1. */
                    &buttonDaemonHandle
                    );
  configASSERT(ret == pdPASS);
  
  buttonsTableMutex = xSemaphoreCreateMutex();
  configASSERT(buttonsTableMutex);
}

uint32_t ssButtonRegister(ssSysComCpidType cpid, uint8_t buttonId, uint8_t event)
{
  uint32_t ret = SS_BUTTON_STATUS_ERR;
  
  /* check parameters */
  configASSERT(buttonId < SS_BUTTON_COUNT);
  configASSERT(cpid != SS_SYSCOM_CPID_INVALID);
  configASSERT(event <= BUTTON_EVENT_ALL);
  
  /* can't be called before task is initialized */
  configASSERT(buttonDaemonHandle != NULL)
  
  BUTTON_CRITICAL_SECTION_ENTER();
  if(buttonsTable[buttonId].data.cpid == SS_SYSCOM_CPID_INVALID)
  {
    buttonsTable[buttonId].data.cpid = cpid;
    buttonsTable[buttonId].data.buttonId = buttonId;
    buttonsTable[buttonId].data.events = event;
    ret = SS_BUTTON_STATUS_OK;
  }
  else
  {
    /* error, already used */
    ret = SS_BUTTON_STATUS_ERR;
  }
  BUTTON_CRITICAL_SECTION_EXIT();
  
  if(ret == SS_BUTTON_STATUS_OK)
  {
    ButtonFsmRaiseEvent(buttonId, BUTTON_FSM_EVENT_ENABLE);
  }
  
  return ret;
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/
void ButtonDaemon(void *argument)
{
  BaseType_t ret;
  uint32_t notification;
  
  configASSERT(ssSysComUserRegister(BUTTON_DAEMON_TASK_CPID, ssSysComQueueCreate(SS_SYSCOM_QUEUE_SIZE_DEFAULT)) == BUTTON_DAEMON_TASK_CPID);
  BSP_Button_RegisterCallback(USER_BUTTON_ID, UserButtonCallback);
   
  while(1)
  {
    ret = xTaskNotifyWait(0, 0xffffffff, &notification, portMAX_DELAY);
    if(ret == pdTRUE)
    {
      uint32_t buttonId = BTN_NFC_BUTTON_ID_GET(notification);
      uint32_t event = BTN_NFC_EVENT_GET(notification);
     
      ssFsmOnEvent((ssFsmType *)&buttonsTable[buttonId].fsm, event); 
    }
  }
}


void ButtonFsmCtor(ssButtonFsmType *me, ssButtonDataType *data)
{
  ssFsmCtor((ssFsmType *)me, "btn");
  ssFsmStateCtor(&me->disabled, "disabled", (ssFsmEventHandler)ButtonFsmDisabledHandler);
  ssFsmStateCtor(&me->press_debounce, "press_db", (ssFsmEventHandler)ButtonFsmPressDebounceHandler);
  ssFsmStateCtor(&me->down, "down", (ssFsmEventHandler)ButtonFsmDownHandler);
  ssFsmStateCtor(&me->release_debounce, "rel_db", (ssFsmEventHandler)ButtonFsmReleaseDebounceHandler);
  ssFsmStateCtor(&me->up, "up", (ssFsmEventHandler)ButtonFsmUpHandler);
  
  me->timer = xTimerCreate("Btn Fsm",
                 BTN_PRESS_DEBOUNCE_INTERVAL,
                 pdFALSE,
                 (void *)0,
                 ButtonTimerCallback);
  configASSERT(me->timer);
  me->btn_data = data;
  
  ssFsmStart((ssFsmType *)me, &me->disabled);
}

void ButtonFsmDisabledHandler(ssButtonFsmType *me, ssFsmEventType event)
{
  switch(event)
  {
  case FSM_EVENT_START:
    break;
  case FSM_EVENT_ENTRY:
    break;
  case FSM_EVENT_EXIT:
    break;
  case BUTTON_FSM_EVENT_ENABLE:
    /* check pin in go to coresponding state */
    if(BSP_Button_GetPinState(me->btn_data->buttonId) == BSP_BUTTON_PRESSED)
    {
      ssFsmStateTrans((ssFsmType *)me, &me->down);
    }
    else
    {
      ssFsmStateTrans((ssFsmType *)me, &me->up);
    }
  default:
    /* ignore all other events */
    break;
  }
}


void ButtonFsmUpHandler(ssButtonFsmType *me, ssFsmEventType event)
{
  switch(event)
  {
  case FSM_EVENT_START:
    break;
  case FSM_EVENT_ENTRY:
    break;
  case FSM_EVENT_EXIT:
    break;
  case BUTTON_FSM_EVENT_PRESS:
    ssFsmStateTrans((ssFsmType *)me, &me->press_debounce);
  default:
    /* ignore all other events */
    break;
  }
}


void ButtonFsmPressDebounceHandler(ssButtonFsmType *me, ssFsmEventType event)
{
  switch(event)
  {
  case FSM_EVENT_START:
    break;
  case FSM_EVENT_ENTRY:
    /* Start the debouncing timer; button changes shall be ignored during that time */
    ButtonTimerStart(me->timer, me->btn_data->buttonId, BUTTON_FSM_EVENT_PRESS_TIMER, BTN_PRESS_DEBOUNCE_INTERVAL);
    break;
  case FSM_EVENT_EXIT:
    break;
  case BUTTON_FSM_EVENT_PRESS_TIMER:
    /* If the pin state is still 'pressed', means we detected a valid button press;
       otherwise this was a short glitch we'll ignore. */
    if(BSP_Button_GetPinState(me->btn_data->buttonId) == BSP_BUTTON_PRESSED)
    {
      ButtonClientNotify(me->btn_data->cpid, me->btn_data->buttonId, BUTTON_EVENT_PRESS);
      ssFsmStateTrans((ssFsmType *)me, &me->down);
    }
    else
    {
      ssFsmStateTrans((ssFsmType *)me, &me->up);
    }
  default:
    /* ignore all other events */
    break;
  }
}

void ButtonFsmDownHandler(ssButtonFsmType *me, ssFsmEventType event)
{
  switch(event)
  {
  case FSM_EVENT_START:
    break;
  case FSM_EVENT_ENTRY:
    break;
  case FSM_EVENT_EXIT:
    break;
  case BUTTON_FSM_EVENT_RELEASE:
    ssFsmStateTrans((ssFsmType *)me, &me->release_debounce);
  default:
    /* ignore all other events */
    break;
  }
}


void ButtonFsmReleaseDebounceHandler(ssButtonFsmType *me, ssFsmEventType event)
{
  switch(event)
  {
  case FSM_EVENT_START:
    break;
  case FSM_EVENT_ENTRY:
    /* Start the debouncing timer; button changes shall be ignored during that time */
    ButtonTimerStart(me->timer, me->btn_data->buttonId, BUTTON_FSM_EVENT_RELEASE_TIMER, BTN_RELEASE_DEBOUNCE_INTERVAL);
    break;
  case FSM_EVENT_EXIT:
    break;
  case BUTTON_FSM_EVENT_RELEASE_TIMER:
    /* If the pin state is still 'released', means we detected a valid button release;
       otherwise this was a short glitch we'll ignore. */
    if(BSP_Button_GetPinState(me->btn_data->buttonId) == BSP_BUTTON_RELEASED)
    {
      ButtonClientNotify(me->btn_data->cpid, me->btn_data->buttonId, BUTTON_EVENT_RELEASE);
      ssFsmStateTrans((ssFsmType *)me, &me->up);
    }
    else
    {
      ssFsmStateTrans((ssFsmType *)me, &me->down);
    }
  default:
    /* ignore all other events */
    break;
  }
}


void ButtonFsmRaiseEvent(uint32_t button, uint32_t event)
{
  uint32_t notification = 0;
  
  BTN_NFC_EVENT_SET(&notification, event);
  BTN_NFC_BUTTON_ID_SET(&notification, button);
  xTaskNotify(buttonDaemonHandle, notification, eSetValueWithOverwrite);
}


void UserButtonCallback(uint32_t button, uint32_t event)
{
  uint32_t notification = 0;
  BaseType_t higherPriorityTaskWoken = pdFALSE;
  
  if(event == BSP_BUTTON_EVENT_PRESS)
  {
     BTN_NFC_EVENT_SET(&notification, BUTTON_FSM_EVENT_PRESS);
  }
  else
  {
    BTN_NFC_EVENT_SET(&notification, BUTTON_FSM_EVENT_RELEASE);
  }
  BTN_NFC_BUTTON_ID_SET(&notification, button);
  xTaskNotifyFromISR(buttonDaemonHandle, notification, eSetValueWithOverwrite, &higherPriorityTaskWoken);
  portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

static void ButtonTimerCallback(TimerHandle_t xTimer)
{
  uint32_t notification;

  notification = (uint32_t) pvTimerGetTimerID(xTimer);
  xTaskNotify(buttonDaemonHandle, notification, eSetValueWithOverwrite);
}

static void ButtonTimerStart(TimerHandle_t xTimer, uint32_t button, uint32_t event, uint32_t period)
{
  uint32_t value = BTN_NFC_VALUE_INIT(button, event);
  vTimerSetTimerID(xTimer, (void *)value);
  configASSERT(xTimerChangePeriod(xTimer, period, 0) != pdFAIL); 
  configASSERT(xTimerStart(xTimer, 0) != pdFAIL);
}

static void ButtonClientNotify(ssSysComCpidType cpid, uint32_t buttonId, uint32_t event)
{
  void *msg;
  ModeCtrlButtonIndMsgType *payload;
  
  msg = ssSysComMsgCreate(BUTTON_PRESS_IND_MSG_ID, sizeof(ModeCtrlButtonIndMsgType), cpid);
  configASSERT(msg);
  payload = (ModeCtrlButtonIndMsgType *)ssSysComMsgPayloadGet(msg);
  payload->buttonId = buttonId;
  payload->event = event;
  ssSysComMsgSendS(&msg, BUTTON_DAEMON_TASK_CPID);
}

#ifdef __cplusplus
}
#endif



