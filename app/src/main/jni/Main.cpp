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
bool ESPEdgeIndicator = true;
bool ESPHideInVote = true;
bool ESPHideInLobby = false;
bool DebugMode = false;
bool DroneView = false;
float DroneZoom = 10.0f;
bool SpeedHack = false;
float SpeedMultiplier = 1.5f;

void *localPlayerInstance = NULL;
void *localPlayerObject = NULL;
void *mainCameraObject = NULL;
bool isInVotingScreen = false;
bool isInGame = false;
bool isInLobby = true;
int localPlayerRole = 0;

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
#define COLOR_PINK     0xFFFF69B4
#define COLOR_LIME     0xFF32CD32
#define COLOR_PURPLE   0xFF9932CC
#define COLOR_GOLD     0xFFFFD700
#define COLOR_CRIMSON  0xFFDC143C
#define COLOR_TEAL     0xFF008080

// ====================================================================================
// OFFSETS - dump.cs'den alındı
// ====================================================================================

// --- PlayableEntity (TypeDefIndex: 6285) ---
#define OFFSET_PE_NICKNAME           0x90
#define OFFSET_PE_ISLOCAL            0x98
#define OFFSET_PE_PLAYERROLE         0xA0   // GGDRole* (pointer)
#define OFFSET_PE_TEAMID             0xDC
#define OFFSET_PE_FOGOFWAR           0xF4
#define OFFSET_PE_ISGHOST            0x178
#define OFFSET_PE_ENTITYNUMBER       0x88
#define OFFSET_PE_TRANSFORMVIEW      0x2C0

// --- GGDRole (TypeDefIndex: 5656) ---
#define OFFSET_ROLE_TYPE             0x12   // RoleType (short)

// --- BetterPhotonTransformView (TypeDefIndex: 1961) ---
#define OFFSET_TV_LATESTPOS          0x30
#define OFFSET_TV_LASTTRANSFORMPOS   0x38

// --- LocalPlayer (TypeDefIndex: 6261) ---
#define OFFSET_LP_MAINCAMERA         0x78
#define OFFSET_LP_INVOTINGSCREEN     0xC3

// ==================== ESP DATA ====================
struct PlayerData {
    Vector2 position;
    bool isGhost;
    bool isLocal;
    int role;
    int teamId;
    int entityNumber;
    char name[64];
    bool isValid;
    float distanceToLocal;
};

#define MAX_PLAYERS 20
PlayerData g_Players[MAX_PLAYERS];
int g_PlayerCount = 0;
Vector2 g_LocalPlayerPos;

// Hızlı erişim için mutex yerine atomic-like yaklaşım
volatile bool g_DataReady = false;
PlayerData g_RenderPlayers[MAX_PLAYERS];  // Render için kopyalanan data
int g_RenderPlayerCount = 0;

float g_ScreenWidth = 1080.0f;
float g_ScreenHeight = 2400.0f;
float g_DefaultOrthoSize = 5.0f;

int g_FrameCount = 0;
bool g_ESPStabilized = false;
#define STABILIZE_FRAMES 3  // Azaltıldı

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

// ==================== HELPER FUNCTIONS ====================
void WideCharToAscii(void* instance, char* outName, int maxLen) {
    memset(outName, 0, maxLen);
    
    uintptr_t strPtr = *(uintptr_t*)((uintptr_t)instance + OFFSET_PE_NICKNAME);
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

int GetRoleType(void* instance) {
    if (!instance) return 0;
    void* rolePtr = *(void**)((uintptr_t)instance + OFFSET_PE_PLAYERROLE);
    if (!rolePtr) return 0;
    return (int)*(short*)((uintptr_t)rolePtr + OFFSET_ROLE_TYPE);
}

float GetCurrentOrthoSize() {
    if (DroneView) return DroneZoom;
    return g_DefaultOrthoSize;
}

Vector2 GetPlayerPosition(void* instance, bool forLocal) {
    Vector2 pos = {0, 0};
    if (!instance) return pos;
    
    void* tv = *(void**)((uintptr_t)instance + OFFSET_PE_TRANSFORMVIEW);
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

float GetESPScale() {
    float orthoSize = GetCurrentOrthoSize();
    if (orthoSize <= 0) orthoSize = 5.0f;
    return g_ScreenHeight / (orthoSize * 2.0f);
}

struct RoleInfo { const char* name; int color; };
RoleInfo GetRoleInfo(int roleId) {
    RoleInfo info;
    switch(roleId) {
        case 0: info.name = "In Lobby"; info.color = COLOR_GRAY; break;
        case 1: info.name = "Goose"; info.color = COLOR_GREEN; break;
        case 2: info.name = "Duck"; info.color = COLOR_RED; break;
        case 3: info.name = "Dodo"; info.color = COLOR_YELLOW; break;
        case 4: info.name = "Falcon"; info.color = COLOR_ORANGE; break;
        case 5: info.name = "Vulture"; info.color = COLOR_PURPLE; break;
        case 6: info.name = "Pelican"; info.color = COLOR_CYAN; break;
        case 7: info.name = "Morphling"; info.color = COLOR_PINK; break;
        case 8: info.name = "Silencer"; info.color = COLOR_CRIMSON; break;
        case 9: info.name = "Sheriff"; info.color = COLOR_GOLD; break;
        case 10: info.name = "Engineer"; info.color = COLOR_TEAL; break;
        case 11: info.name = "Detective"; info.color = COLOR_BLUE; break;
        case 12: info.name = "Medium"; info.color = COLOR_MAGENTA; break;
        default: info.name = "Role"; info.color = COLOR_WHITE; break;
    }
    return info;
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
    if (!env || !g_MenuClass || !g_GetScreenWidthMethod) return;
    
    int w = env->CallStaticIntMethod(g_MenuClass, g_GetScreenWidthMethod);
    int h = env->CallStaticIntMethod(g_MenuClass, g_GetScreenHeightMethod);
    if (w > 100 && h > 100) {
        g_ScreenWidth = (float)w;
        g_ScreenHeight = (float)h;
    }
}

// ==================== ESP DRAW (Inline for speed) ====================
inline void DrawLine(JNIEnv* env, float x1, float y1, float x2, float y2, int color) {
    if (env && g_DrawLineColorMethod)
        env->CallStaticVoidMethod(g_MenuClass, g_DrawLineColorMethod, x1, y1, x2, y2, color);
}

inline void DrawBox(JNIEnv* env, float x, float y, float w, float h, int color) {
    if (env && g_DrawBoxColorMethod)
        env->CallStaticVoidMethod(g_MenuClass, g_DrawBoxColorMethod, x, y, w, h, color);
}

inline void DrawText(JNIEnv* env, float x, float y, const char* text, int color) {
    if (!env || !g_DrawTextColorMethod || !text) return;
    jstring jstr = env->NewStringUTF(text);
    if (jstr) {
        env->CallStaticVoidMethod(g_MenuClass, g_DrawTextColorMethod, x, y, jstr, color);
        env->DeleteLocalRef(jstr);
    }
}

inline void ClearESP(JNIEnv* env) {
    if (env && g_ClearESPMethod) 
        env->CallStaticVoidMethod(g_MenuClass, g_ClearESPMethod);
}

inline void UpdateESPView(JNIEnv* env) {
    if (env && g_UpdateESPMethod) 
        env->CallStaticVoidMethod(g_MenuClass, g_UpdateESPMethod);
}

void SetESPEnabled(bool e) {
    JNIEnv* env = GetJNIEnv();
    if (env && g_SetESPEnabledMethod) 
        env->CallStaticVoidMethod(g_MenuClass, g_SetESPEnabledMethod, (jboolean)e);
}

// ==================== WORLD TO SCREEN ====================
inline bool WorldToScreen(Vector2 world, Vector2 localPos, float scale, float* sx, float* sy) {
    float dx = world.x - localPos.x;
    float dy = world.y - localPos.y;
    
    float cx = g_ScreenWidth * 0.5f;
    float cy = g_ScreenHeight * 0.5f;
    
    *sx = cx + (dx * scale);
    *sy = cy - (dy * scale);
    
    return true;
}

// ==================== LINE CLIPPING ====================
bool ClipLine(float* x1, float* y1, float* x2, float* y2) {
    const float xmin = 0, ymin = 0;
    const float xmax = g_ScreenWidth, ymax = g_ScreenHeight;
    
    int outcode1 = 0, outcode2 = 0;
    
    if (*x1 < xmin) outcode1 |= 1;
    else if (*x1 > xmax) outcode1 |= 2;
    if (*y1 < ymin) outcode1 |= 8;
    else if (*y1 > ymax) outcode1 |= 4;
    
    if (*x2 < xmin) outcode2 |= 1;
    else if (*x2 > xmax) outcode2 |= 2;
    if (*y2 < ymin) outcode2 |= 8;
    else if (*y2 > ymax) outcode2 |= 4;
    
    while (true) {
        if (!(outcode1 | outcode2)) return true;
        if (outcode1 & outcode2) return false;
        
        float x, y;
        int outcodeOut = outcode1 ? outcode1 : outcode2;
        
        if (outcodeOut & 4) { x = *x1 + (*x2 - *x1) * (ymax - *y1) / (*y2 - *y1); y = ymax; }
        else if (outcodeOut & 8) { x = *x1 + (*x2 - *x1) * (ymin - *y1) / (*y2 - *y1); y = ymin; }
        else if (outcodeOut & 2) { y = *y1 + (*y2 - *y1) * (xmax - *x1) / (*x2 - *x1); x = xmax; }
        else { y = *y1 + (*y2 - *y1) * (xmin - *x1) / (*x2 - *x1); x = xmin; }
        
        if (outcodeOut == outcode1) {
            *x1 = x; *y1 = y;
            outcode1 = 0;
            if (*x1 < xmin) outcode1 |= 1;
            else if (*x1 > xmax) outcode1 |= 2;
            if (*y1 < ymin) outcode1 |= 8;
            else if (*y1 > ymax) outcode1 |= 4;
        } else {
            *x2 = x; *y2 = y;
            outcode2 = 0;
            if (*x2 < xmin) outcode2 |= 1;
            else if (*x2 > xmax) outcode2 |= 2;
            if (*y2 < ymin) outcode2 |= 8;
            else if (*y2 > ymax) outcode2 |= 4;
        }
    }
}

// ==================== EDGE POSITION ====================
inline void GetEdgePosition(float sx, float sy, float* edgeX, float* edgeY) {
    float cx = g_ScreenWidth * 0.5f;
    float cy = g_ScreenHeight * 0.5f;
    float dx = sx - cx;
    float dy = sy - cy;
    
    if (dx == 0 && dy == 0) { *edgeX = cx; *edgeY = cy; return; }
    
    float padding = 100.0f;
    float maxX = g_ScreenWidth - padding, minX = padding;
    float maxY = g_ScreenHeight - padding, minY = padding;
    
    float tX = (dx > 0) ? ((maxX - cx) / dx) : ((minX - cx) / dx);
    float tY = (dy > 0) ? ((maxY - cy) / dy) : ((minY - cy) / dy);
    float t = fminf(fabsf(tX), fabsf(tY));
    if (t > 1) t = 1;
    
    *edgeX = cx + dx * t;
    *edgeY = cy + dy * t;
    
    if (*edgeX < minX) *edgeX = minX; else if (*edgeX > maxX) *edgeX = maxX;
    if (*edgeY < minY) *edgeY = minY; else if (*edgeY > maxY) *edgeY = maxY;
}

// ==================== RENDER DEBUG PANEL ====================
void RenderDebugPanel(JNIEnv* env) {
    if (!DebugMode || !env) return;
    
    float startX = 30, startY = 120, lineHeight = 28;
    float maxY = g_ScreenHeight - 100;
    
    DrawText(env, startX + 100, startY, "=== DEBUG INFO ===", COLOR_MAGENTA);
    startY += lineHeight + 5;
    
    char buf[128];
    snprintf(buf, sizeof(buf), "Screen: %.0fx%.0f | Ortho: %.2f | Scale: %.1f", 
             g_ScreenWidth, g_ScreenHeight, GetCurrentOrthoSize(), GetESPScale());
    DrawText(env, startX + 150, startY, buf, COLOR_CYAN);
    startY += lineHeight;
    
    snprintf(buf, sizeof(buf), "InGame: %s | Lobby: %s | Vote: %s | Players: %d", 
             isInGame ? "Y" : "N", isInLobby ? "Y" : "N", isInVotingScreen ? "Y" : "N", g_RenderPlayerCount);
    DrawText(env, startX + 160, startY, buf, COLOR_CYAN);
    startY += lineHeight;
    
    snprintf(buf, sizeof(buf), "LocalPos: %.2f, %.2f | MyRole: %d", 
             g_LocalPlayerPos.x, g_LocalPlayerPos.y, localPlayerRole);
    DrawText(env, startX + 100, startY, buf, COLOR_GREEN);
    startY += lineHeight + 10;
    
    DrawText(env, startX + 100, startY, "=== PLAYERS ===", COLOR_YELLOW);
    startY += lineHeight;
    
    for (int i = 0; i < g_RenderPlayerCount && startY < maxY; i++) {
        PlayerData* p = &g_RenderPlayers[i];
        if (!p->isValid) continue;
        
        RoleInfo ri = GetRoleInfo(p->role);
        snprintf(buf, sizeof(buf), "#%d %s%s | %s(%d) T:%d | %.1f,%.1f | %.1fm",
                 p->entityNumber, p->name,
                 p->isLocal ? "[YOU]" : (p->isGhost ? "[G]" : ""),
                 ri.name, p->role, p->teamId,
                 p->position.x, p->position.y, p->distanceToLocal);
        
        DrawText(env, startX + 200, startY, buf, 
                p->isLocal ? COLOR_CYAN : (p->isGhost ? COLOR_GRAY : ri.color));
        startY += lineHeight;
    }
}

// ==================== RENDER ESP ====================
void RenderESP(JNIEnv* env) {
    if (!ESPEnabled || !env) return;
    if (ESPHideInVote && isInVotingScreen) return;
    if (ESPHideInLobby && isInLobby) return;
    if (!isInGame) return;
    
    if (!g_ESPStabilized) {
        g_FrameCount++;
        if (g_FrameCount < STABILIZE_FRAMES) return;
        g_ESPStabilized = true;
    }
    
    float cx = g_ScreenWidth * 0.5f;
    float cy = g_ScreenHeight * 0.5f;
    float scale = GetESPScale();
    Vector2 localPos = g_LocalPlayerPos;
    
    for (int i = 0; i < g_RenderPlayerCount; i++) {
        PlayerData* p = &g_RenderPlayers[i];
        if (!p->isValid || p->isLocal || p->isGhost) continue;
        if (p->position.x == 0 && p->position.y == 0) continue;
        
        float sx, sy;
        WorldToScreen(p->position, localPos, scale, &sx, &sy);
        
        int color = COLOR_WHITE;
        float dist = p->distanceToLocal;
        
        bool onScreen = (sx >= 0 && sx <= g_ScreenWidth && sy >= 0 && sy <= g_ScreenHeight);
        
        // Lines
        if (ESPLines) {
            float lx1 = cx, ly1 = cy, lx2 = sx, ly2 = sy;
            if (ClipLine(&lx1, &ly1, &lx2, &ly2)) {
                DrawLine(env, lx1, ly1, lx2, ly2, color);
            }
        }
        
        if (onScreen) {
            float boxW = 0.9f * scale, boxH = 1.6f * scale;
            if (boxW < 45) boxW = 45; if (boxH < 70) boxH = 70;
            if (boxW > 200) boxW = 200; if (boxH > 320) boxH = 320;
            
            float boxX = sx - boxW * 0.5f;
            float boxY = sy - boxH * 0.40f;
            
            if (ESPBox) DrawBox(env, boxX, boxY, boxW, boxH, color);
            
            if (ESPDistance) {
                char dt[16];
                snprintf(dt, sizeof(dt), "%.1fm", dist);
                DrawText(env, sx, boxY - 25, dt, COLOR_YELLOW);
            }
            
            if (ESPName) {
                DrawText(env, sx, boxY - 55, p->name, COLOR_WHITE);
            }
        } 
        else if (ESPEdgeIndicator) {
            float edgeX, edgeY;
            GetEdgePosition(sx, sy, &edgeX, &edgeY);
            
            DrawBox(env, edgeX - 8, edgeY - 8, 16, 16, COLOR_ORANGE);
            
            char et[48];
            snprintf(et, sizeof(et), "%s %.0fm", p->name, dist);
            
            float tx = edgeX, ty = edgeY - 25;
            if (edgeX < 150) tx = 150;
            else if (edgeX > g_ScreenWidth - 150) tx = g_ScreenWidth - 150;
            if (edgeY < 80) ty = edgeY + 35;
            
            DrawText(env, tx, ty, et, COLOR_ORANGE);
        }
    }
}

// ==================== CLEAR ALL ====================
void ClearAllESP() {
    JNIEnv* env = GetJNIEnv();
    if (env) {
        ClearESP(env);
        UpdateESPView(env);
    }
    
    g_PlayerCount = 0;
    g_RenderPlayerCount = 0;
    g_ESPStabilized = false;
    g_FrameCount = 0;
    isInGame = false;
    isInLobby = true;
    localPlayerInstance = NULL;
    localPlayerObject = NULL;
}

// ==================== HOOKS ====================

// PlayableEntity.Update - RVA: 0x3DC28D8
void (*old_Update)(void *instance);
void Update(void *instance) {
    if (instance) {
        bool isLocal = *(bool*)((uintptr_t)instance + OFFSET_PE_ISLOCAL);
        
        if (isLocal) {
            localPlayerInstance = instance;
            isInGame = true;
            localPlayerRole = GetRoleType(instance);
            isInLobby = (localPlayerRole == 0);
            
            // Player listesini temizle
            g_PlayerCount = 0;
            g_LocalPlayerPos = GetPlayerPosition(instance, true);
            
            if (UnlimitedVision) {
                *(bool*)((uintptr_t)instance + OFFSET_PE_FOGOFWAR) = false;
            }
        }
        
        // Veri topla (mutex yok - daha hızlı)
        if ((ESPEnabled || DebugMode) && g_PlayerCount < MAX_PLAYERS) {
            PlayerData* p = &g_Players[g_PlayerCount];
            p->position = GetPlayerPosition(instance, isLocal);
            p->isGhost = *(bool*)((uintptr_t)instance + OFFSET_PE_ISGHOST);
            p->isLocal = isLocal;
            p->role = GetRoleType(instance);
            p->teamId = *(int*)((uintptr_t)instance + OFFSET_PE_TEAMID);
            p->entityNumber = *(int*)((uintptr_t)instance + OFFSET_PE_ENTITYNUMBER);
            WideCharToAscii(instance, p->name, sizeof(p->name));
            
            if (!isLocal) {
                float dx = p->position.x - g_LocalPlayerPos.x;
                float dy = p->position.y - g_LocalPlayerPos.y;
                p->distanceToLocal = sqrtf(dx*dx + dy*dy);
            } else {
                p->distanceToLocal = 0;
            }
            
            p->isValid = true;
            g_PlayerCount++;
        }
    }
    old_Update(instance);
}

// PlayableEntity.LateUpdate - RVA: 0x3DC368C (FixedUpdate yerine - daha hızlı!)
void (*old_LateUpdate)(void *instance);
void LateUpdate(void *instance) {
    old_LateUpdate(instance);
    
    if (instance && g_ESPReady) {
        bool isLocal = *(bool*)((uintptr_t)instance + OFFSET_PE_ISLOCAL);
        
        if (isLocal) {
            // Render için veriyi kopyala (hızlı copy)
            g_RenderPlayerCount = g_PlayerCount;
            memcpy(g_RenderPlayers, g_Players, sizeof(PlayerData) * g_PlayerCount);
            
            // Ekran boyutunu güncelle (her 30 frame'de bir)
            static int screenUpdateCounter = 0;
            if (++screenUpdateCounter >= 30) {
                UpdateScreenSize();
                screenUpdateCounter = 0;
            }
            
            JNIEnv* env = GetJNIEnv();
            if (env) {
                ClearESP(env);
                
                if (DebugMode) RenderDebugPanel(env);
                if (ESPEnabled) RenderESP(env);
                
                UpdateESPView(env);
            }
        }
    }
}

// PlayableEntity.TurnIntoGhost - RVA: 0x3DCE370
void (*old_TurnIntoGhost)(void *instance, int deathReason);
void TurnIntoGhost(void *instance, int deathReason) {
    if (GodMode && instance == localPlayerInstance) return;
    old_TurnIntoGhost(instance, deathReason);
}

// LocalPlayer.Update - RVA: 0x3D986B8
void (*old_LocalPlayer_Update)(void *instance);
void LocalPlayer_Update(void *instance) {
    if (instance) {
        localPlayerObject = instance;
        
        void* cam = *(void**)((uintptr_t)instance + OFFSET_LP_MAINCAMERA);
        if (cam) mainCameraObject = cam;
        
        isInVotingScreen = *(bool*)((uintptr_t)instance + OFFSET_LP_INVOTINGSCREEN);
        
        if (DroneView && OverrideOrthographicSize) {
            OverrideOrthographicSize(instance, DroneZoom);
        }
    }
    old_LocalPlayer_Update(instance);
}

// LocalPlayer.GetPlayerSpeed - RVA: 0x3DA7768
float (*old_GetPlayerSpeed)(void *instance);
float GetPlayerSpeed(void *instance) {
    float speed = old_GetPlayerSpeed(instance);
    return SpeedHack ? speed * SpeedMultiplier : speed;
}

// GGDRole.OnEnterVent - RVA: 0x3C55C94
void (*old_OnEnterVent)(void *instance, void* vent, bool setCooldown);
void OnEnterVent(void *instance, void* vent, bool setCooldown) {
    old_OnEnterVent(instance, vent, NoCooldown ? false : setCooldown);
}

// GGDRole.OnExitVent - RVA: 0x3C55D88
void (*old_OnExitVent)(void *instance, void* vent, bool setCooldown);
void OnExitVent(void *instance, void* vent, bool setCooldown) {
    old_OnExitVent(instance, vent, NoCooldown ? false : setCooldown);
}

// GGDRole.SetVentCooldown - RVA: 0x3C548AC
void (*old_SetVentCooldown)(void *instance, int startCooldown);
void SetVentCooldown(void *instance, int startCooldown) {
    old_SetVentCooldown(instance, NoCooldown ? 0 : startCooldown);
}

// PlayableEntity.Despawn - RVA: 0x3DC5328
void (*old_Despawn)(void *instance);
void Despawn(void *instance) {
    if (instance) {
        bool isLocal = *(bool*)((uintptr_t)instance + OFFSET_PE_ISLOCAL);
        if (isLocal) ClearAllESP();
    }
    old_Despawn(instance);
}

// ==================== FEATURES ====================
jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    InitESP(env);
    
    const char *features[] = {
        OBFUSCATE("Category_Vision & Cooldown"),
        OBFUSCATE("Toggle_Unlimited Vision"),
        OBFUSCATE("Toggle_No Vent Cooldown"),
        
        OBFUSCATE("Category_Camera"),
        OBFUSCATE("Toggle_Drone View"),
        OBFUSCATE("SeekBar_Zoom Level_5_25"),
        
        OBFUSCATE("Category_ESP Settings"),
        OBFUSCATE("Toggle_ESP Enabled"),
        OBFUSCATE("Toggle_True_ESP Lines"),
        OBFUSCATE("Toggle_True_ESP Box"),
        OBFUSCATE("Toggle_True_ESP Distance"),
        OBFUSCATE("Toggle_True_ESP Names"),
        OBFUSCATE("Toggle_True_Edge Indicator"),
        OBFUSCATE("Toggle_True_Hide in Vote Screen"),
        OBFUSCATE("Toggle_Hide in Lobby"),
        
        OBFUSCATE("Category_Debug Panel"),
        OBFUSCATE("Toggle_Show Debug Info"),
        
        OBFUSCATE("Category_Local Only (May Not Work)"),
        OBFUSCATE("Toggle_God Mode [LOCAL]"),
        OBFUSCATE("Toggle_Speed Hack [LOCAL]"),
        OBFUSCATE("SeekBar_Speed Multiplier_10_40"),
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
        case 0: UnlimitedVision = boolean;
            if (!boolean && localPlayerInstance) 
                *(bool*)((uintptr_t)localPlayerInstance + OFFSET_PE_FOGOFWAR) = true;
            break;
        case 1: NoCooldown = boolean; break;
        case 2: DroneView = boolean;
            if (!boolean && localPlayerObject && OverrideOrthographicSize)
                OverrideOrthographicSize(localPlayerObject, g_DefaultOrthoSize);
            g_ESPStabilized = false; g_FrameCount = 0;
            break;
        case 3: DroneZoom = (float)value; break;
        case 4: ESPEnabled = boolean;
            if (boolean) { g_ESPStabilized = false; g_FrameCount = 0; }
            SetESPEnabled(boolean || DebugMode);
            break;
        case 5: ESPLines = boolean; break;
        case 6: ESPBox = boolean; break;
        case 7: ESPDistance = boolean; break;
        case 8: ESPName = boolean; break;
        case 9: ESPEdgeIndicator = boolean; break;
        case 10: ESPHideInVote = boolean; break;
        case 11: ESPHideInLobby = boolean; break;
        case 12: DebugMode = boolean; SetESPEnabled(boolean || ESPEnabled); break;
        case 13: GodMode = boolean; break;
        case 14: SpeedHack = boolean; break;
        case 15: SpeedMultiplier = (float)value / 10.0f; break;
    }
}

// ==================== HACK THREAD ====================
ElfScanner g_il2cppELF;

void hack_thread() {
    LOGI("pthread created");

    while (!isLibraryLoaded(targetLibName)) sleep(1);

    do { sleep(1); g_il2cppELF = ElfScanner::createWithPath(targetLibName); } 
    while (!g_il2cppELF.isValid());

    LOGI("%s loaded", (const char*)targetLibName);

#if defined(__aarch64__)
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DC28D8")), Update, old_Update);
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DC368C")), LateUpdate, old_LateUpdate);  // LateUpdate!
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DCE370")), TurnIntoGhost, old_TurnIntoGhost);
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DC5328")), Despawn, old_Despawn);
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
void lib_main() { std::thread(hack_thread).detach(); }
