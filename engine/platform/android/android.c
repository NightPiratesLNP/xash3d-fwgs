#include "platform/platform.h"
#include "input.h"
#include "client.h"
#include "sound.h"
#include "errno.h"
#include <pthread.h>
#include <sys/prctl.h>

#include <android/log.h>
#include <jni.h>
#if XASH_SDL
#include <SDL.h>
#endif // XASH_SDL

struct jnimethods_s
{
	JNIEnv *env;
	jobject activity;
	jclass actcls;
	jmethodID loadAndroidID;
	jmethodID getAndroidID;
	jmethodID saveAndroidID;
} jni;

int g_android_custom_width = 0;
int g_android_custom_height = 0;

/*
========================
Android_ParseResolutionArgs
========================
*/
void Android_ParseResolutionArgs( void )
{
	for( int i = 0; i < host.argc - 1; ++i )
	{
		if( !strcmp( host.argv[i], "-width" ))
			g_android_custom_width = atoi( host.argv[i+1] );
		if( !strcmp( host.argv[i], "-height" ))
			g_android_custom_height = atoi( host.argv[i+1] );
	}

	if( g_android_custom_width > 0 && g_android_custom_height > 0 )
	{
		Con_Printf( "Android: Using custom resolution: %dx%d\n", g_android_custom_width, g_android_custom_height );

		char tmp[16];

		snprintf( tmp, sizeof( tmp ), "%d", g_android_custom_width );
		Cvar_Set( "width", tmp );

		snprintf( tmp, sizeof( tmp ), "%d", g_android_custom_height );
		Cvar_Set( "height", tmp );
	}
}

void Android_Init( void )
{
	memset( &jni, 0, sizeof( jni ));

	Android_ParseResolutionArgs();

#if XASH_SDL
	jni.env = (JNIEnv *)SDL_AndroidGetJNIEnv();
	jni.activity = (jobject)SDL_AndroidGetActivity();
	jni.actcls = (*jni.env)->GetObjectClass( jni.env, jni.activity );
	jni.loadAndroidID = (*jni.env)->GetMethodID( jni.env, jni.actcls, "loadAndroidID", "()Ljava/lang/String;" );
	jni.getAndroidID = (*jni.env)->GetMethodID( jni.env, jni.actcls, "getAndroidID", "()Ljava/lang/String;" );
	jni.saveAndroidID = (*jni.env)->GetMethodID( jni.env, jni.actcls, "saveAndroidID", "(Ljava/lang/String;)V" );

	SDL_SetHint( SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight" );
	SDL_SetHint( SDL_HINT_JOYSTICK_HIDAPI_STEAM, "1" );
	SDL_SetHint( SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "0" );
	SDL_SetHint( SDL_HINT_ANDROID_BLOCK_ON_PAUSE_PAUSEAUDIO, "0" );
	SDL_SetHint( SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1" );
#endif
}

void *Android_GetNativeObject( const char *name )
{
	if( !strcasecmp( name, "JNIEnv" ) )
	{
		return (void *)jni.env;
	}
	else if( !strcasecmp( name, "ActivityClass" ) )
	{
		return (void *)jni.actcls;
	}

	return NULL;
}

const char *Android_GetAndroidID( void )
{
	static char id[32];
	jstring resultJNIStr;
	const char *resultCStr;

	if( COM_CheckString( id ) ) return id;

	resultJNIStr = (*jni.env)->CallObjectMethod( jni.env, jni.activity, jni.getAndroidID );
	resultCStr = (*jni.env)->GetStringUTFChars( jni.env, resultJNIStr, NULL );
	Q_strncpy( id, resultCStr, sizeof( id ) );
	(*jni.env)->ReleaseStringUTFChars( jni.env, resultJNIStr, resultCStr );
	(*jni.env)->DeleteLocalRef( jni.env, resultJNIStr );

	return id;
}

const char *Android_LoadID( void )
{
	static char id[32];
	jstring resultJNIStr;
	const char *resultCStr;

	resultJNIStr = (*jni.env)->CallObjectMethod( jni.env, jni.activity, jni.loadAndroidID );
	resultCStr = (*jni.env)->GetStringUTFChars( jni.env, resultJNIStr, NULL );
	Q_strncpy( id, resultCStr, sizeof( id ) );
	(*jni.env)->ReleaseStringUTFChars( jni.env, resultJNIStr, resultCStr );
	(*jni.env)->DeleteLocalRef( jni.env, resultJNIStr );

	return id;
}

void Android_SaveID( const char *id )
{
	jstring JStr = (*jni.env)->NewStringUTF( jni.env, id );
	(*jni.env)->CallVoidMethod( jni.env, jni.activity, jni.saveAndroidID, JStr );
	(*jni.env)->DeleteLocalRef( jni.env, JStr );
}

void Platform_ShellExecute( const char *path, const char *parms )
{
#if XASH_SDL
	SDL_OpenURL( path );
#endif
}
