/** hsm.h -- Hierarchical State Machine interface
 */
#ifndef hsm_h
#define hsm_h
#include <stdio.h>

typedef struct {
  int evt;
} ssHsmEventType;

typedef struct ssHsmType ssHsmType;

typedef ssHsmEventType const *(*EvtHndlr)(ssHsmType*, ssHsmEventType const*);

typedef struct ssHsmStateType ssHsmStateType;

struct ssHsmStateType {
  ssHsmStateType *super; /* pointer to superstate */
  EvtHndlr hndlr; /* state’s handler function */
  char const *name;
};

void StateCtor(ssHsmStateType *me, char const *name, ssHsmStateType *super, EvtHndlr hndlr);

#define StateOnEvent(me_, ctx_, msg_) (*(me_)->hndlr)((ctx_), (msg_)) \
    
struct ssHsmType { /* Hierarchical State Machine base class */
  char const *name;                               /* pointer to static name */
  ssHsmStateType *curr;                                    /* current state */
  ssHsmStateType *next;           /* next state (non 0 if transition taken) */
  ssHsmStateType *source;                   /* source state during last transition */
  ssHsmStateType top;                              /* top-most state object */
};

void HsmCtor(ssHsmType *me, char const *name, EvtHndlr topHndlr);
void HsmOnStart(ssHsmType *me); /* enter and start the top state */
void HsmOnEvent(ssHsmType *me, ssHsmEventType const *msg); /* “HSM engine” */

void stateTrans(ssHsmType *me, ssHsmStateType *target);

/* protected: */
unsigned char HsmToLCA_(ssHsmType *me, ssHsmStateType *target);
void HsmExit_(ssHsmType *me, unsigned char toLca);
#define STATE_CURR(me_) (((ssHsmType *)me_)->curr)
#define STATE_START(me_, target_) (((ssHsmType *)me_)->next = (target_))
   
#define STATE_TRAN(me_, target_) if (1) { \
    static unsigned char toLca_ = 0xFF; \
    configASSERT(((ssHsmType *)me_)->next == 0);\
    if (toLca_ == 0xFF) \
      toLca_ = HsmToLCA_((ssHsmType *)(me_), (target_));\
    HsmExit_((ssHsmType *)(me_), toLca_);\
    ((ssHsmType *)(me_))->next = (target_);\
  } else ((void)0)


#define HSM_START_EVT (-1)
#define HSM_ENTRY_EVT (-2)
#define HSM_EXIT_EVT  (-3)
    
#endif