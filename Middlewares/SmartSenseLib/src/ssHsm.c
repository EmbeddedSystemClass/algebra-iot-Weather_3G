#include "ssHsm.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"

static ssHsmEventType const startMsg = {HSM_START_EVT};
static ssHsmEventType const entryMsg = {HSM_ENTRY_EVT};
static ssHsmEventType const exitMsg = {HSM_EXIT_EVT};
#define MAX_STATE_NESTING 8

/* Hsm ctor */
void HsmCtor(ssHsmType *me, char const *name, EvtHndlr topHndlr) 
{
  StateCtor(&me->top, "top", 0, topHndlr);
  me->name = name;
}

void StateCtor(ssHsmStateType *me, char const *name, ssHsmStateType *super, EvtHndlr hndlr)
{ 
  me->super = super;
  me->hndlr = hndlr;
  me->name = name;
}

/* enter and start the top state */
void HsmOnStart(ssHsmType *me) 
{
  me->curr = &me->top;
  me->next = 0;
  StateOnEvent(me->curr, me, &entryMsg);
  while (StateOnEvent(me->curr, me, &startMsg), me->next) 
  {
    ssHsmStateType *entryPath[MAX_STATE_NESTING];
    register ssHsmStateType **trace = entryPath;
    register ssHsmStateType *s;
    *trace = 0;
    for (s = me->next; s != me->curr; s = s->super) 
    {
      *(++trace) = s;                         /* trace path to target */
    }
    while (s = *trace--) 
    {                 /* retrace entry from source */
      StateOnEvent(s, me, &entryMsg);
    }
    me->curr = me->next;
    me->next = 0;
  }
}

/* state machine "engine"...................................................*/
void HsmOnEvent(ssHsmType *me, ssHsmEventType const *msg) 
{
  ssHsmStateType *entryPath[MAX_STATE_NESTING];
  register ssHsmStateType **trace;
  register ssHsmStateType *s;
  for (s = me->curr; s; s = s->super) 
  {
    me->source = s;                 /* level of outermost event handler */
    msg = StateOnEvent(s, me, msg);
    if (msg == 0) 
    {
      if (me->next) 
      {                      /* state transition taken? */
        trace = entryPath;
        *trace = 0;
        for (s = me->next; s != me->curr; s = s->super) 
        {
          *(++trace) = s;                 /* trace path to target */
        }
        while (s = *trace--) 
        {            /* retrace entry from LCA */
          StateOnEvent(s, me, &entryMsg);
        }
        me->curr = me->next;
        me->next = 0;
        while (StateOnEvent(me->curr, me, &startMsg), me->next) 
        {
          trace = entryPath;
          *trace = 0;
          for (s = me->next; s != me->curr; s = s->super) 
          {
            *(++trace) = s;            /* record path to target */
          }
          while (s = *trace--) 
          {             /* retrace the entry */
            StateOnEvent(s, me, &entryMsg);
          }
          me->curr = me->next;
          me->next = 0;
        }
      }
      break;                                       /* event processed */
    }
  }
}


/* exit current states and all superstates up to LCA */
void HsmExit_(ssHsmType *me, unsigned char toLca) 
{
  register ssHsmStateType *s = me->curr;
  while (s != me->source) 
  {
    StateOnEvent(s, me, &exitMsg);
    s = s->super;   
  }
  while (toLca--) 
  {
    StateOnEvent(s, me, &exitMsg);
    s = s->super;
  }
  me->curr = s;
}


/* find # of levels to Least Common Ancestor */
unsigned char HsmToLCA_(ssHsmType *me, ssHsmStateType *target) 
{
  ssHsmStateType *s, *t;
  unsigned char toLca = 0;
  if (me->source == target) 
  {
    return 1;
  }
  for (s = me->source; s; ++toLca, s = s->super) 
  {
    for (t = target; t; t = t->super) 
    {
      if (s == t) 
      {
        return toLca;
      }
   }
  }
  return 0;
}