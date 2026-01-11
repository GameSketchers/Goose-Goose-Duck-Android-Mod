#include <list>
#include <vector>
#include <cstring>
#include <pthread.h>
#include <thread>
#include <string>
#include <jni.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <dlfcn.h>
#include <cmath>
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.hpp"
#include "Menu/Menu.hpp"
#include "Menu/Jni.hpp"
#include "Includes/Macros.h"

#define targetLibName OBFUSCATE("libil2cpp.so")

// ==================== VARIABLES ====================
bool GodMode = false;
bool UnlimitedVision = false;
bool NoCooldown = false;
bool ESPEnabled = false;
bool ESPLines = true;
bool ESPBox = true;
bool ESPDistance = true;
bool ESPName = true;
bool ESPEdgeIndicator = true;  // Yeni: Ekran dışı gösterge
bool DebugMode = false;
bool DroneView = false;
float DroneZoom = 10.0f;
bool SpeedHack = false;
float SpeedMultiplier = 1.5f;

void *localPlayerInstance = NULL;
void *localPlayerObject = NULL;
void *mainCameraObject = NULL;

struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };

// ==================== COLORS ====================
#define COLOR_WHITE    0xFFFFFFFF
#define COLOR_RED      0xFFFF0000
#define COLOR_GREEN    0xFF00FF00
#define COLOR_YELLOW   0xFFFFFF00
#define COLOR_CYAN     0xFF00FFFF
#define COLOR_MAGENTA  0xFFFF00FF
#define COLOR_GRAY     0xFF888888
#define COLOR_ORANGE   0xFFFF8800
#define COLOR_BLUE     0xFF0088FF

// ==================== OFFSETS ====================
#define OFFSET_NICKNAME       0x90
#define OFFSET_ISLOCAL        0x98
#define OFFSET_PLAYERROLE     0xA0
#define OFFSET_FOGOFWAR       0xF4
#define OFFSET_ISGHOST        0x178
#define OFFSET_TRANSFORMVIEW  0x2C0

#define OFFSET_TV_LATESTPOS        0x30
#define OFFSET_TV_LASTTRANSFORMPOS 0x38

#define OFFSET_LP_MAINCAMERA       0x78
#define OFFSET_LP_DISABLEMOVEMENT  0x98

// ==================== ESP DATA ====================
struct PlayerData {
    Vector2 position;
    bool isGhost;
    bool isLocal;
    int role;
    char name[64];
    bool isValid;
};

#define MAX_PLAYERS 20
PlayerData g_Players[MAX_PLAYERS];
int g_PlayerCount = 0;
Vector2 g_LocalPlayerPos;
pthread_mutex_t g_PlayerMutex = PTHREAD_MUTEX_INITIALIZER;

float g_ScreenWidth = 1080.0f;
float g_ScreenHeight = 2400.0f;
float g_DefaultOrthoSize = 5.0f;

// ESP başlangıç stabilizasyonu
int g_FrameCount = 0;
bool g_ESPStabilized = false;
#define STABILIZE_FRAMES 10

// ==================== ESP JNI ====================
JavaVM* g_JavaVM = NULL;
jclass g_MenuClass = NULL;
jmethodID g_DrawLineColorMethod = NULL;
jmethodID g_DrawBoxColorMethod = NULL;
jmethodID g_DrawTextColorMethod = NULL;
jmethodID g_ClearESPMethod = NULL;
jmethodID g_SetESPEnabledMethod = NULL;
jmethodID g_UpdateESPMethod = NULL;
jmethodID g_GetScreenWidthMethod = NULL;
jmethodID g_GetScreenHeightMethod = NULL;
bool g_ESPReady = false;

void (*OverrideOrthographicSize)(void*, float) = NULL;

// ==================== JNI ====================
JNIEnv* GetJNIEnv() {
    if (!g_JavaVM) return NULL;
    JNIEnv* env = NULL;
    if (g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        g_JavaVM->AttachCurrentThread(&env, NULL);
    }
    return env;
}

// ==================== UTF-16 TO ASCII ====================
void WideCharToAscii(void* instance, char* outName, int maxLen) {
    memset(outName, 0, maxLen);
    
    uintptr_t strPtr = *(uintptr_t*)((uintptr_t)instance + OFFSET_NICKNAME);
    if (!strPtr) { strcpy(outName, "Unknown"); return; }
    
    int length = *(int*)(strPtr + 0x10);
    uint16_t* chars = (uint16_t*)(strPtr + 0x14);
    
    if (length <= 0 || length > 50) { strcpy(outName, "Player"); return; }
    
    int outIdx = 0;
    for (int i = 0; i < length && outIdx < maxLen - 1; i++) {
        uint16_t c = chars[i];
        if (c >= 0x20 && c <= 0x7E) outName[outIdx++] = (char)c;
        else if (c == 0x011F || c == 0x011E) outName[outIdx++] = 'g';
        else if (c == 0x0131) outName[outIdx++] = 'i';
        else if (c == 0x0130) outName[outIdx++] = 'I';
        else if (c == 0x015F || c == 0x015E) outName[outIdx++] = 's';
        else if (c == 0x00FC || c == 0x00DC) outName[outIdx++] = 'u';
        else if (c == 0x00F6 || c == 0x00D6) outName[outIdx++] = 'o';
        else if (c == 0x00E7 || c == 0x00C7) outName[outIdx++] = 'c';
    }
    if (outIdx == 0) strcpy(outName, "Player");
    else outName[outIdx] = '\0';
}

// ==================== GET ORTHO SIZE ====================
float GetCurrentOrthoSize() {
    if (DroneView) return DroneZoom;
    return g_DefaultOrthoSize;
}

// ==================== GET POSITION ====================
Vector2 GetPlayerPosition(void* instance, bool forLocal) {
    Vector2 pos = {0, 0};
    if (!instance) return pos;
    
    void* tv = *(void**)((uintptr_t)instance + OFFSET_TRANSFORMVIEW);
    if (tv) {
        if (forLocal) {
            pos = *(Vector2*)((uintptr_t)tv + OFFSET_TV_LASTTRANSFORMPOS);
            if (pos.x == 0 && pos.y == 0) {
                pos = *(Vector2*)((uintptr_t)tv + OFFSET_TV_LATESTPOS);
            }
        } else {
            pos = *(Vector2*)((uintptr_t)tv + OFFSET_TV_LATESTPOS);
        }
    }
    return pos;
}

// ==================== CALCULATE SCALE ====================
float GetESPScale() {
    float orthoSize = GetCurrentOrthoSize();
    float pixelsPerUnit = g_ScreenHeight / (orthoSize * 2.0f);
    return pixelsPerUnit;
}

// ==================== ESP INIT ====================
void InitESP(JNIEnv *env) {
    if (g_ESPReady) return;
    if (!env) return;
    if (!g_JavaVM) env->GetJavaVM(&g_JavaVM);
    
    jclass cls = env->FindClass("com/android/support/Menu");
    if (!cls) return;
    
    g_MenuClass = (jclass)env->NewGlobalRef(cls);
    g_DrawLineColorMethod = env->GetStaticMethodID(g_MenuClass, "drawESPLineColor", "(FFFFI)V");
    g_DrawBoxColorMethod = env->GetStaticMethodID(g_MenuClass, "drawESPBoxColor", "(FFFFI)V");
    g_DrawTextColorMethod = env->GetStaticMethodID(g_MenuClass, "drawESPTextColor", "(FFLjava/lang/String;I)V");
    g_ClearESPMethod = env->GetStaticMethodID(g_MenuClass, "clearESP", "()V");
    g_SetESPEnabledMethod = env->GetStaticMethodID(g_MenuClass, "setESPEnabled", "(Z)V");
    g_UpdateESPMethod = env->GetStaticMethodID(g_MenuClass, "updateESP", "()V");
    g_GetScreenWidthMethod = env->GetStaticMethodID(g_MenuClass, "getScreenWidth", "()I");
    g_GetScreenHeightMethod = env->GetStaticMethodID(g_MenuClass, "getScreenHeight", "()I");
    
    g_ESPReady = (g_ClearESPMethod && g_DrawTextColorMethod);
    env->DeleteLocalRef(cls);
}

void UpdateScreenSize() {
    JNIEnv* env = GetJNIEnv();
    if (!env || !g_MenuClass) return;
    
    if (g_GetScreenWidthMethod && g_GetScreenHeightMethod) {
        int w = env->CallStaticIntMethod(g_MenuClass, g_GetScreenWidthMethod);
        int h = env->CallStaticIntMethod(g_MenuClass, g_GetScreenHeightMethod);
        if (w > 100 && h > 100) {
            g_ScreenWidth = (float)w;
            g_ScreenHeight = (float)h;
        }
    }
}

// ==================== ESP DRAW ====================
void DrawLine(float x1, float y1, float x2, float y2, int color) {
    if (!g_ESPReady || !g_DrawLineColorMethod) return;
    JNIEnv* env = GetJNIEnv();
    if (env) env->CallStaticVoidMethod(g_MenuClass, g_DrawLineColorMethod, x1, y1, x2, y2, color);
}

void DrawBox(float x, float y, float w, float h, int color) {
    if (!g_ESPReady || !g_DrawBoxColorMethod) return;
    JNIEnv* env = GetJNIEnv();
    if (env) env->CallStaticVoidMethod(g_MenuClass, g_DrawBoxColorMethod, x, y, w, h, color);
}

void DrawText(float x, float y, const char* text, int color) {
    if (!g_ESPReady || !g_DrawTextColorMethod || !text) return;
    JNIEnv* env = GetJNIEnv();
    if (env) {
        jstring jstr = env->NewStringUTF(text);
        if (jstr) {
            env->CallStaticVoidMethod(g_MenuClass, g_DrawTextColorMethod, x, y, jstr, color);
            env->DeleteLocalRef(jstr);
        }
    }
}

void ClearESP() {
    if (!g_ESPReady) return;
    JNIEnv* env = GetJNIEnv();
    if (env && g_ClearESPMethod) env->CallStaticVoidMethod(g_MenuClass, g_ClearESPMethod);
}

void UpdateESPView() {
    if (!g_ESPReady) return;
    JNIEnv* env = GetJNIEnv();
    if (env && g_UpdateESPMethod) env->CallStaticVoidMethod(g_MenuClass, g_UpdateESPMethod);
}

void SetESPEnabled(bool e) {
    if (!g_ESPReady) return;
    JNIEnv* env = GetJNIEnv();
    if (env && g_SetESPEnabledMethod) env->CallStaticVoidMethod(g_MenuClass, g_SetESPEnabledMethod, (jboolean)e);
}

// ==================== WORLD TO SCREEN ====================
bool WorldToScreen(Vector2 world, float* sx, float* sy) {
    if (g_ScreenWidth <= 0 || g_ScreenHeight <= 0) return false;
    
    float dx = world.x - g_LocalPlayerPos.x;
    float dy = world.y - g_LocalPlayerPos.y;
    
    float scale = GetESPScale();
    
    float cx = g_ScreenWidth / 2.0f;
    float cy = g_ScreenHeight / 2.0f;
    
    *sx = cx + (dx * scale);
    *sy = cy - (dy * scale);
    
    return true;
}

// ==================== LINE CLIPPING ====================
#define INSIDE 0
#define LEFT   1
#define RIGHT  2
#define BOTTOM 4
#define TOP    8

int ComputeOutCode(float x, float y, float xmin, float ymin, float xmax, float ymax) {
    int code = INSIDE;
    if (x < xmin) code |= LEFT;
    else if (x > xmax) code |= RIGHT;
    if (y < ymin) code |= TOP;
    else if (y > ymax) code |= BOTTOM;
    return code;
}

bool ClipLine(float* x1, float* y1, float* x2, float* y2) {
    float xmin = 0, ymin = 0;
    float xmax = g_ScreenWidth, ymax = g_ScreenHeight;
    
    int outcode1 = ComputeOutCode(*x1, *y1, xmin, ymin, xmax, ymax);
    int outcode2 = ComputeOutCode(*x2, *y2, xmin, ymin, xmax, ymax);
    
    while (true) {
        if (!(outcode1 | outcode2)) return true;
        else if (outcode1 & outcode2) return false;
        else {
            float x, y;
            int outcodeOut = outcode1 ? outcode1 : outcode2;
            
            if (outcodeOut & BOTTOM) {
                x = *x1 + (*x2 - *x1) * (ymax - *y1) / (*y2 - *y1);
                y = ymax;
            } else if (outcodeOut & TOP) {
                x = *x1 + (*x2 - *x1) * (ymin - *y1) / (*y2 - *y1);
                y = ymin;
            } else if (outcodeOut & RIGHT) {
                y = *y1 + (*y2 - *y1) * (xmax - *x1) / (*x2 - *x1);
                x = xmax;
            } else {
                y = *y1 + (*y2 - *y1) * (xmin - *x1) / (*x2 - *x1);
                x = xmin;
            }
            
            if (outcodeOut == outcode1) {
                *x1 = x; *y1 = y;
                outcode1 = ComputeOutCode(*x1, *y1, xmin, ymin, xmax, ymax);
            } else {
                *x2 = x; *y2 = y;
                outcode2 = ComputeOutCode(*x2, *y2, xmin, ymin, xmax, ymax);
            }
        }
    }
}

// ==================== EDGE INDICATOR ====================
void GetEdgePosition(float sx, float sy, float* edgeX, float* edgeY) {
    float cx = g_ScreenWidth / 2.0f;
    float cy = g_ScreenHeight / 2.0f;
    
    float dx = sx - cx;
    float dy = sy - cy;
    
    float padding = 80.0f;  // Kenardan uzaklık
    
    float maxX = g_ScreenWidth - padding;
    float maxY = g_ScreenHeight - padding;
    float minX = padding;
    float minY = padding;
    
    // Açıyı hesapla
    float angle = atan2f(dy, dx);
    
    // Ekran kenarına olan mesafeyi hesapla
    float tX = (dx > 0) ? (maxX - cx) / dx : (minX - cx) / dx;
    float tY = (dy > 0) ? (maxY - cy) / dy : (minY - cy) / dy;
    
    float t = fminf(fabsf(tX), fabsf(tY));
    
    *edgeX = cx + dx * t;
    *edgeY = cy + dy * t;
    
    // Sınırları kontrol et
    if (*edgeX < minX) *edgeX = minX;
    if (*edgeX > maxX) *edgeX = maxX;
    if (*edgeY < minY) *edgeY = minY;
    if (*edgeY > maxY) *edgeY = maxY;
}

// ==================== RENDER ESP ====================
void RenderESP() {
    if (!ESPEnabled || !g_ESPReady) return;
    
    UpdateScreenSize();
    
    if (g_ScreenWidth <= 0) g_ScreenWidth = 1080;
    if (g_ScreenHeight <= 0) g_ScreenHeight = 2400;
    
    // Stabilizasyon kontrolü
    if (!g_ESPStabilized) {
        g_FrameCount++;
        if (g_FrameCount < STABILIZE_FRAMES) {
            return;  // İlk birkaç frame atla
        }
        g_ESPStabilized = true;
    }
    
    float cx = g_ScreenWidth / 2.0f;
    float cy = g_ScreenHeight / 2.0f;
    float scale = GetESPScale();
    float orthoSize = GetCurrentOrthoSize();
    
    pthread_mutex_lock(&g_PlayerMutex);
    
    // ===== DEBUG PANEL =====
    if (DebugMode) {
        float dy = 80;
        
        char info1[64];
        snprintf(info1, sizeof(info1), "Screen: %.0fx%.0f", g_ScreenWidth, g_ScreenHeight);
        DrawText(180, dy, info1, COLOR_MAGENTA); dy += 28;
        
        char info2[64];
        snprintf(info2, sizeof(info2), "OrthoSize: %.2f Scale: %.1f", orthoSize, scale);
        DrawText(180, dy, info2, COLOR_MAGENTA); dy += 28;
        
        char info3[64];
        snprintf(info3, sizeof(info3), "DroneView: %s Zoom: %.1f", DroneView ? "ON" : "OFF", DroneZoom);
        DrawText(180, dy, info3, DroneView ? COLOR_GREEN : COLOR_GRAY); dy += 28;
        
        char info4[64];
        snprintf(info4, sizeof(info4), "Players: %d Stabilized: %s", g_PlayerCount, g_ESPStabilized ? "YES" : "NO");
        DrawText(180, dy, info4, COLOR_MAGENTA); dy += 28;
        
        if (localPlayerInstance) {
            char info5[64];
            snprintf(info5, sizeof(info5), "MyPos: %.2f, %.2f", g_LocalPlayerPos.x, g_LocalPlayerPos.y);
            DrawText(180, dy, info5, COLOR_CYAN); dy += 28;
        }
    }
    
    // ===== ESP DRAWING =====
    if (!localPlayerInstance) {
        pthread_mutex_unlock(&g_PlayerMutex);
        return;
    }
    
    for (int i = 0; i < g_PlayerCount; i++) {
        PlayerData* p = &g_Players[i];
        if (!p->isValid || p->isLocal || p->isGhost) continue;
        if (p->position.x == 0 && p->position.y == 0) continue;
        
        float sx, sy;
        WorldToScreen(p->position, &sx, &sy);
        
        int color = COLOR_WHITE;
        
        float dist = sqrtf(powf(p->position.x - g_LocalPlayerPos.x, 2) + 
                          powf(p->position.y - g_LocalPlayerPos.y, 2));
        
        // Ekran içinde mi?
        bool onScreen = (sx >= 0 && sx <= g_ScreenWidth && sy >= 0 && sy <= g_ScreenHeight);
        
        // ===== LINES (her zaman çiz) =====
        if (ESPLines) {
            float lx1 = cx, ly1 = cy;
            float lx2 = sx, ly2 = sy;
            
            if (ClipLine(&lx1, &ly1, &lx2, &ly2)) {
                DrawLine(lx1, ly1, lx2, ly2, color);
            }
        }
        
        if (onScreen) {
            // ===== BOX (daha aşağı) =====
            if (ESPBox) {
                float playerWidthUnits = 0.9f;
                float playerHeightUnits = 1.6f;
                
                float boxW = playerWidthUnits * scale;
                float boxH = playerHeightUnits * scale;
                
                if (boxW < 45) boxW = 45;
                if (boxH < 70) boxH = 70;
                if (boxW > 200) boxW = 200;
                if (boxH > 320) boxH = 320;
                
                // Box pozisyonu - daha aşağı kaydırıldı (0.50f)
                float boxX = sx - boxW / 2.0f;
                float boxY = sy - boxH * 0.50f;
                
                DrawBox(boxX, boxY, boxW, boxH, color);
            }
            
            // ===== DISTANCE (daha büyük) =====
            if (ESPDistance) {
                char dt[24];
                snprintf(dt, sizeof(dt), "%.1f m", dist);
                
                float boxH = 1.6f * scale;
                if (boxH < 70) boxH = 70;
                if (boxH > 320) boxH = 320;
                
                DrawText(sx, sy - boxH * 0.50f - 25, dt, COLOR_YELLOW);
            }
            
            // ===== NAME (daha büyük) =====
            if (ESPName) {
                float boxH = 1.6f * scale;
                if (boxH < 70) boxH = 70;
                if (boxH > 320) boxH = 320;
                
                DrawText(sx, sy - boxH * 0.50f - 55, p->name, COLOR_WHITE);
            }
        } 
        else if (ESPEdgeIndicator) {
            // ===== EDGE INDICATOR (ekran dışı) =====
            float edgeX, edgeY;
            GetEdgePosition(sx, sy, &edgeX, &edgeY);
            
            // Yön oku çiz (küçük üçgen)
            float arrowSize = 15.0f;
            float angle = atan2f(sy - cy, sx - cx);
            
            // Ok başı
            float tipX = edgeX;
            float tipY = edgeY;
            
            // Ok gövdesi (çizgi yerine nokta)
            DrawBox(tipX - 6, tipY - 6, 12, 12, COLOR_ORANGE);
            
            // İsim ve mesafe
            char edgeText[48];
            snprintf(edgeText, sizeof(edgeText), "%s %.0fm", p->name, dist);
            
            // Kenara göre text pozisyonu ayarla
            float textX = edgeX;
            float textY = edgeY;
            
            // Sol kenardaysa sağa kaydır
            if (edgeX < 100) textX = edgeX + 50;
            // Sağ kenardaysa sola kaydır
            else if (edgeX > g_ScreenWidth - 100) textX = edgeX - 50;
            
            // Üst kenardaysa aşağı kaydır
            if (edgeY < 100) textY = edgeY + 30;
            // Alt kenardaysa yukarı kaydır
            else if (edgeY > g_ScreenHeight - 100) textY = edgeY - 30;
            
            DrawText(textX, textY, edgeText, COLOR_ORANGE);
        }
    }
    
    pthread_mutex_unlock(&g_PlayerMutex);
}

// ==================== HOOKS ====================

void (*old_Update)(void *instance);
void Update(void *instance) {
    if (instance) {
        bool isLocal = *(bool*)((uintptr_t)instance + OFFSET_ISLOCAL);
        
        if (isLocal) {
            pthread_mutex_lock(&g_PlayerMutex);
            g_PlayerCount = 0;
            memset(g_Players, 0, sizeof(g_Players));
            pthread_mutex_unlock(&g_PlayerMutex);
            
            localPlayerInstance = instance;
            g_LocalPlayerPos = GetPlayerPosition(instance, true);
            
            if (UnlimitedVision) {
                *(bool*)((uintptr_t)instance + OFFSET_FOGOFWAR) = false;
            }
        }
        
        if (ESPEnabled) {
            pthread_mutex_lock(&g_PlayerMutex);
            if (g_PlayerCount < MAX_PLAYERS) {
                PlayerData* p = &g_Players[g_PlayerCount];
                p->position = GetPlayerPosition(instance, isLocal);
                p->isGhost = *(bool*)((uintptr_t)instance + OFFSET_ISGHOST);
                p->isLocal = isLocal;
                p->role = *(int*)((uintptr_t)instance + OFFSET_PLAYERROLE);
                WideCharToAscii(instance, p->name, sizeof(p->name));
                p->isValid = true;
                g_PlayerCount++;
            }
            pthread_mutex_unlock(&g_PlayerMutex);
        }
    }
    old_Update(instance);
}

void (*old_LateUpdate)(void *instance);
void LateUpdate(void *instance) {
    old_LateUpdate(instance);
    if (instance) {
        bool isLocal = *(bool*)((uintptr_t)instance + OFFSET_ISLOCAL);
        if (isLocal && ESPEnabled && g_ESPReady) {
            ClearESP();
            RenderESP();
            UpdateESPView();
        }
    }
}

void (*old_TurnIntoGhost)(void *instance, int deathReason);
void TurnIntoGhost(void *instance, int deathReason) {
    if (GodMode && instance == localPlayerInstance) return;
    old_TurnIntoGhost(instance, deathReason);
}

void (*old_LocalPlayer_Update)(void *instance);
void LocalPlayer_Update(void *instance) {
    if (instance) {
        localPlayerObject = instance;
        
        void* cam = *(void**)((uintptr_t)instance + OFFSET_LP_MAINCAMERA);
        if (cam) mainCameraObject = cam;
        
        if (DroneView && OverrideOrthographicSize) {
            OverrideOrthographicSize(instance, DroneZoom);
        }
    }
    old_LocalPlayer_Update(instance);
}

float (*old_GetPlayerSpeed)(void *instance);
float GetPlayerSpeed(void *instance) {
    float originalSpeed = old_GetPlayerSpeed(instance);
    if (SpeedHack) return originalSpeed * SpeedMultiplier;
    return originalSpeed;
}

void (*old_OnEnterVent)(void *instance, void* vent, bool setCooldown);
void OnEnterVent(void *instance, void* vent, bool setCooldown) {
    old_OnEnterVent(instance, vent, NoCooldown ? false : setCooldown);
}

void (*old_OnExitVent)(void *instance, void* vent, bool setCooldown);
void OnExitVent(void *instance, void* vent, bool setCooldown) {
    old_OnExitVent(instance, vent, NoCooldown ? false : setCooldown);
}

void (*old_SetVentCooldown)(void *instance, int startCooldown);
void SetVentCooldown(void *instance, int startCooldown) {
    old_SetVentCooldown(instance, NoCooldown ? 0 : startCooldown);
}

// ==================== FEATURES ====================
jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    InitESP(env);
    
    const char *features[] = {
        OBFUSCATE("Category_Vision & Cooldown"),
        OBFUSCATE("Toggle_Unlimited Vision"),            // 0
        OBFUSCATE("Toggle_No Vent Cooldown"),            // 1
        
        OBFUSCATE("Category_Camera"),
        OBFUSCATE("Toggle_Drone View"),                  // 2
        OBFUSCATE("SeekBar_Zoom Level_5_25"),            // 3
        
        OBFUSCATE("Category_ESP Settings"),
        OBFUSCATE("Toggle_ESP Enabled"),                 // 4
        OBFUSCATE("Toggle_True_ESP Lines"),              // 5
        OBFUSCATE("Toggle_True_ESP Box"),                // 6
        OBFUSCATE("Toggle_True_ESP Distance"),           // 7
        OBFUSCATE("Toggle_True_ESP Names"),              // 8
        OBFUSCATE("Toggle_True_Edge Indicator"),         // 9 - Yeni
        
        OBFUSCATE("Category_Local Only (May Not Work)"),
        OBFUSCATE("Toggle_God Mode [LOCAL]"),            // 10
        OBFUSCATE("Toggle_Speed Hack [LOCAL]"),          // 11
        OBFUSCATE("SeekBar_Speed Multiplier_10_40"),     // 12
        
        OBFUSCATE("Category_Debug"),
        OBFUSCATE("Toggle_Show Debug Info"),             // 13
    };

    int count = sizeof(features) / sizeof(features[0]);
    jobjectArray ret = env->NewObjectArray(count, env->FindClass("java/lang/String"), env->NewStringUTF(""));
    for (int i = 0; i < count; i++) {
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));
    }
    return ret;
}

void Changes(JNIEnv *env, jclass clazz, jobject obj, jint featNum, jstring featName, 
             jint value, jlong Lvalue, jboolean boolean, jstring str) {
    if (!g_JavaVM) env->GetJavaVM(&g_JavaVM);
    
    switch (featNum) {
        case 0:
            UnlimitedVision = boolean;
            if (!boolean && localPlayerInstance) 
                *(bool*)((uintptr_t)localPlayerInstance + OFFSET_FOGOFWAR) = true;
            break;
        case 1: 
            NoCooldown = boolean;
            break;
        case 2: 
            DroneView = boolean;
            // DroneView kapatıldığında ESP'yi resetle
            if (!boolean) {
                g_ESPStabilized = false;
                g_FrameCount = 0;
                if (localPlayerObject && OverrideOrthographicSize) {
                    OverrideOrthographicSize(localPlayerObject, g_DefaultOrthoSize);
                }
            }
            break;
        case 3:
            DroneZoom = (float)value;
            break;
        case 4:
            ESPEnabled = boolean;
            // ESP açıldığında stabilizasyonu resetle
            if (boolean) {
                g_ESPStabilized = false;
                g_FrameCount = 0;
            }
            SetESPEnabled(boolean);
            if (!boolean) { ClearESP(); UpdateESPView(); }
            break;
        case 5: ESPLines = boolean; break;
        case 6: ESPBox = boolean; break;
        case 7: ESPDistance = boolean; break;
        case 8: ESPName = boolean; break;
        case 9: ESPEdgeIndicator = boolean; break;
        case 10: GodMode = boolean; break;
        case 11: SpeedHack = boolean; break;
        case 12: SpeedMultiplier = (float)value / 10.0f; break;
        case 13: DebugMode = boolean; break;
    }
}

// ==================== HACK THREAD ====================
ElfScanner g_il2cppELF;

void hack_thread() {
    LOGI("pthread created");

    while (!isLibraryLoaded(targetLibName)) sleep(1);

    do {
        sleep(1);
        g_il2cppELF = ElfScanner::createWithPath(targetLibName);
    } while (!g_il2cppELF.isValid());

    LOGI("%s loaded", (const char*)targetLibName);

#if defined(__aarch64__)
    
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DC28D8")), Update, old_Update);
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DC368C")), LateUpdate, old_LateUpdate);
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DCE370")), TurnIntoGhost, old_TurnIntoGhost);
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3D986B8")), LocalPlayer_Update, old_LocalPlayer_Update);
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DA7768")), GetPlayerSpeed, old_GetPlayerSpeed);
    
    OverrideOrthographicSize = (void (*)(void*, float))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3DA813C")));
    
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3C55C94")), OnEnterVent, old_OnEnterVent);
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3C55D88")), OnExitVent, old_OnExitVent);
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3C548AC")), SetVentCooldown, old_SetVentCooldown);
    
    LOGI("All hooks installed!");
    
#endif

    LOGI("Done");
}

__attribute__((constructor))
void lib_main() {
    std::thread(hack_thread).detach();
}
