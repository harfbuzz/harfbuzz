/* File: harfbuzz.i */
%module harfbuzz
%{
#define HB_H_IN
#include "hb-common.h"
#include "hb-buffer.h"
#include "hb-blob.h"
#include "hb-shape.h"
#include "hb-font.h"
#include <stdio.h>
#include "jni.h"

// HEADER SECTION
#define MISSING_GLYPH_CODE  0
hb_position_t  CJNI_FUNC_hb_font_glyph_h_advance_func (hb_font_t *font, void *font_data, hb_codepoint_t glyph,  void *user_data);
hb_bool_t  CJNI_FUNC_hb_font_glyph_func (hb_font_t *font, void *font_data, hb_codepoint_t unicode, hb_codepoint_t variation_selector, hb_codepoint_t *glyph,  void *user_data);

hb_font_get_glyph_h_advance_func_t CJNI_CALLBACK_hb_font_glyph_h_advance_func;
hb_font_get_glyph_func_t CJNI_CALLBACK_hb_font_glyph_func;

void CJNI_hb_font_set_funcs (hb_font_t *font, hb_font_funcs_t *klass, char  *font_data,  hb_destroy_func_t  destroy);

// CODE SECTION

JavaVM *jvm;

JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM *vm, void *pvt ) {
    // storing the VM
    jvm = vm;
    fprintf( stdout, "* JNI_OnLoad called\n" );
    // TEST CODE
//    hb_bool_t res = CJNI_FUNC_hb_font_glyph_func (NULL, NULL, 234, 123, NULL, NULL);
//    printf("\n%d\n", res);
//    hb_position_t res2 = CJNI_FUNC_hb_font_glyph_h_advance_func (NULL, NULL, 234,  NULL);
//    printf("\n%d\n", res2);

    return JNI_VERSION_1_6;
}




hb_position_t CJNI_FUNC_hb_font_glyph_h_advance_func (hb_font_t *font, void *font_data, hb_codepoint_t glyph,  void *user_data)
{
    
    JNIEnv *env;
    jclass cls;
    jmethodID mid;
    int getEnvStat;
    printf("native CJNI_FUNC_hb_font_glyph_h_advance_func: %s\n",(char*) font_data);
    
     // double check it's all ok
    getEnvStat = (*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
	 printf( "GetEnv: not attached");
	if ((*jvm)->AttachCurrentThread(jvm, (void **) &env, NULL) != 0) {
	    printf("Failed to attach");
	}
    } else {
	if (getEnvStat == JNI_OK) {
	    printf("GetEnv OK");
	} else {
	    if (getEnvStat == JNI_EVERSION) {
		printf( "GetEnv: version not supported");
	    }
	}       
    }                                                          
                                                                                 
    cls = (*env)->FindClass(env, "prezi/graphics/text/backend/HarfBuzzJava");
    if (cls == NULL) {
	printf("class not found\n");
	jthrowable exc = (*env)->ExceptionOccurred(env);
	if(exc)
	{
	  (*env)->ExceptionDescribe(env);
	  (*env)->ExceptionClear(env);
	}    
    }
    mid = (*env)->GetStaticMethodID(env, cls, "CALL_hb_font_funcs_advance_func", "(Ljava/lang/Object;I)I");
    if (mid == NULL) {
	/* error handling */
	printf("method not found\n");
	jthrowable exc = (*env)->ExceptionOccurred(env);
	if(exc)
	{
	  (*env)->ExceptionDescribe(env);
	  (*env)->ExceptionClear(env);
	}    

    }
    jstring font_data_string = (*env)->NewStringUTF(env,(char*) font_data);
    jint ret = (*env)->CallStaticIntMethod(env, cls, mid, font_data_string, (jint) glyph);
    (*jvm)->DetachCurrentThread(jvm);

    return ((hb_position_t) ret);

}


hb_bool_t CJNI_FUNC_hb_font_glyph_func (hb_font_t *font, void *font_data, hb_codepoint_t unicode, hb_codepoint_t variation_selector, hb_codepoint_t *glyph,  void *user_data) {
    
    JNIEnv *env;
    jclass cls;
    jmethodID mid;
    int getEnvStat;
    printf("native CJNI_FUNC_hb_font_glyph_func: %s\n", (char*) font_data);
    
     // double check it's all ok
    getEnvStat = (*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
	 printf( "GetEnv: not attached");
	if ((*jvm)->AttachCurrentThread(jvm, (void **) &env, NULL) != 0) {
	    printf("Failed to attach");
	}
    } else {
	if (getEnvStat == JNI_OK) {
	    printf("GetEnv OK");
	} else {
	    if (getEnvStat == JNI_EVERSION) {
		printf( "GetEnv: version not supported");
	    }
	}       
    }                                                          
                                                                                 
    cls = (*env)->FindClass(env, "prezi/graphics/text/backend/HarfBuzzJava");
    if (cls == NULL) {
	printf("class not found\n");
	jthrowable exc = (*env)->ExceptionOccurred(env);
	if(exc)
	{
	  (*env)->ExceptionDescribe(env);
	  (*env)->ExceptionClear(env);
	}    
    }
    // TODO create java objects
    mid = (*env)->GetStaticMethodID(env, cls, "CALL_hb_font_funcs_glyph_func", "(Ljava/lang/Object;I)I");
    if (mid == NULL) {
	/* error handling */
	printf("method not found\n");
	jthrowable exc = (*env)->ExceptionOccurred(env);
	if(exc)
	{
	  (*env)->ExceptionDescribe(env);
	  (*env)->ExceptionClear(env);
	}    

    }
    jstring font_data_string = (*env)->NewStringUTF(env,(char*) font_data);
    jint ret = (*env)->CallStaticIntMethod(env, cls, mid, font_data_string, (jint) unicode);
    // do we need to free string???
    *glyph = ((hb_codepoint_t) ret);
    (*jvm)->DetachCurrentThread(jvm);
    return ((hb_bool_t) (ret != MISSING_GLYPH_CODE));
}

#define CJNI_ID_BUFFER_SIZE 200
void CJNI_hb_font_set_funcs (hb_font_t *font, hb_font_funcs_t *klass, char* font_data,  hb_destroy_func_t  destroy){
    char *font_data_copy = malloc(CJNI_ID_BUFFER_SIZE * sizeof(char)); 
    strncpy ( font_data_copy, font_data, CJNI_ID_BUFFER_SIZE - 1);
    printf("native registered: %s\n", font_data_copy);
    hb_font_set_funcs (font, klass, (void*) font_data_copy, destroy);
}

// CONSTANTS

hb_font_get_glyph_h_advance_func_t CJNI_CALLBACK_hb_font_glyph_h_advance_func = &CJNI_FUNC_hb_font_glyph_h_advance_func;

hb_font_get_glyph_func_t CJNI_CALLBACK_hb_font_glyph_func = &CJNI_FUNC_hb_font_glyph_func;

void nullFunction(void * arg1){
}

int voidData = 0;
unsigned int unsignedIntData= 0;
void * NULL_SWIGTYPE_p_void = (void*) &voidData;
void (*NULL_SWIGTYPE_p_f_p_void__void)(void *) = &nullFunction;
unsigned int *NULL_SWIGTYPE_p_unsigned_int = &unsignedIntData;

%}
%include "arrays_java.i"
%include "stdint.i"
%define HB_H_IN
%enddef
%include "hb-common.h"
%include "hb-buffer.h"
%include "hb-blob.h"
%include "hb-shape.h"
%include "hb-font.h"
%include "hb-ucdn/ucdn.h"

%include "carrays.i"
%array_class(hb_feature_t, hb_feature_tArray);
%array_class(hb_glyph_info_t, hb_glyph_info_tArray);
%array_class(hb_glyph_position_t, hb_glyph_position_tArray);
%array_class(hb_codepoint_t, hb_codepoint_tArray);


hb_font_get_glyph_h_advance_func_t CJNI_CALLBACK_hb_font_glyph_h_advance_func = &CJNI_FUNC_hb_font_glyph_h_advance_func;

hb_font_get_glyph_func_t CJNI_CALLBACK_hb_font_glyph_func = &CJNI_FUNC_hb_font_glyph_func;

void * NULL_SWIGTYPE_p_void = (void*) &voidData;
void (*NULL_SWIGTYPE_p_f_p_void__void)(void*) = &nullFunction;
unsigned int *NULL_SWIGTYPE_p_unsigned_int = &unsignedIntData;

void CJNI_hb_font_set_funcs (hb_font_t *font, hb_font_funcs_t *klass, char  *font_data,  hb_destroy_func_t  destroy);
