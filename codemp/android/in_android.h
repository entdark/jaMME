//#include "quake_common/port_act_defs.h"

#ifdef __cplusplus
extern "C"
{
#endif
int PortableKeyEvent(int state, int code ,int unitcode);
void PortableTextPaste(const char *paste);
void PortableAction(int state, int action/*,int param = 0*/);
void PortableMove(float fwd, float strafe);
void PortableMoveFwd(float fwd);
void PortableMoveSide(float strafe);
void PortableLookPitch(int mode, float pitch);
void PortableLookYaw(int mode, float pitch);
void PortableCommand(const char * cmd);
void PortableMouse(float dx,float dy);
void PortableMouseAbs(float x,float y);
void PortableInit(int argc,const char ** argv);
void PortableShutDown(void);
void PortableFrame(void);
int PortableInMenu(void);
int PortableShowKeyboard(void);
int PortableInAutomap(void);
int PortableLookScale();
void PortablePrint(char *msg);
void PortableVibrateFeedback(int time);
#ifdef __cplusplus
}
void Android_JNI_ShowNotification(const char *message);
#endif
