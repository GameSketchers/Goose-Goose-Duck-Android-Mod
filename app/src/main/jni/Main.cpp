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

bool AntiDeath = false;
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

float TeleportX = 0.0f;
float TeleportY = 0.0f;
float SavedPosX = 0.0f;
float SavedPosY = 0.0f;
bool btnTeleport = false;
bool btnSetPosition = false;

void *localPlayerInstance = NULL;
void *localPlayerObject = NULL;
void *mainCameraObject = NULL;
bool isInVotingScreen = false;
bool isInGame = false;
bool isInLobby = true;
int localPlayerRole = 0;

int g_DroneViewDelay = 0;
#define DRONE_VIEW_DELAY_FRAMES 60
bool g_DroneViewReady = false;

float g_CameraOrthoSize = 5.0f;

struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };

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
// OFFSETS - Source: dump.cs (Goose Goose Duck)
// ====================================================================================

// PlayableEntity (TypeDefIndex: 6285) - Handlers.GameHandlers.PlayerHandlers
#define OFFSET_PE_NICKNAME           0x90
#define OFFSET_PE_ISLOCAL            0x98
#define OFFSET_PE_PLAYERROLE         0xA0
#define OFFSET_PE_ISPLAYERROLESET    0xA8
#define OFFSET_PE_TEAMID             0xDC
#define OFFSET_PE_FOGOFWAR           0xF4
#define OFFSET_PE_ISGHOST            0x178
#define OFFSET_PE_ENTITYNUMBER       0x88
#define OFFSET_PE_TRANSFORMVIEW      0x2C0
#define OFFSET_PE_TARGETOPACITY      0xB8
#define OFFSET_PE_HASKILLED          0xC5
#define OFFSET_PE_ISDOWNED           0x180
#define OFFSET_PE_INVENT             0x182
#define OFFSET_PE_ISINVISIBLE        0x189
#define OFFSET_PE_ISSPECTATOR        0x200
#define OFFSET_PE_ISMORPHED          0x18F
#define OFFSET_PE_ISRUNNING          0x121

// GGDRole - Role type as short
#define OFFSET_ROLE_TYPE             0x12

// BetterPhotonTransformView - Position data
#define OFFSET_TV_LATESTPOS          0x30
#define OFFSET_TV_LASTTRANSFORMPOS   0x38

// LocalPlayer (TypeDefIndex: 6261) - Handlers.GameHandlers.PlayerHandlers
#define OFFSET_LP_MAINCAMERA         0x78
#define OFFSET_LP_INVOTINGSCREEN     0xC3

// ====================================================================================

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
    bool isDowned;
    bool inVent;
    bool isInvisible;
    bool isSpectator;
    bool isMorphed;
    bool isRunning;
    bool hasKilledThisRound;
    float opacity;
};

#define MAX_PLAYERS 20
PlayerData g_Players[MAX_PLAYERS];
int g_PlayerCount = 0;
Vector2 g_LocalPlayerPos;

PlayerData g_RenderPlayers[MAX_PLAYERS];
int g_RenderPlayerCount = 0;

float g_ScreenWidth = 1080.0f;
float g_ScreenHeight = 2400.0f;
float g_DefaultOrthoSize = 5.0f;

int g_FrameCount = 0;
bool g_ESPStabilized = false;
#define STABILIZE_FRAMES 3

#define MAX_ESP_BUFFER 32768
char g_ESPBatchBuffer[MAX_ESP_BUFFER];
int g_ESPBatchOffset = 0;

inline void BatchClear() {
    g_ESPBatchOffset = 0;
    g_ESPBatchBuffer[0] = '\0';
}

inline void BatchAddLine(float x1, float y1, float x2, float y2, int color) {
    if (g_ESPBatchOffset >= MAX_ESP_BUFFER - 64) return;
    g_ESPBatchOffset += snprintf(g_ESPBatchBuffer + g_ESPBatchOffset, 
        MAX_ESP_BUFFER - g_ESPBatchOffset,
        "L%.0f,%.0f,%.0f,%.0f,%d;", x1, y1, x2, y2, color);
}

inline void BatchAddBox(float x, float y, float w, float h, int color) {
    if (g_ESPBatchOffset >= MAX_ESP_BUFFER - 64) return;
    g_ESPBatchOffset += snprintf(g_ESPBatchBuffer + g_ESPBatchOffset,
        MAX_ESP_BUFFER - g_ESPBatchOffset,
        "B%.0f,%.0f,%.0f,%.0f,%d;", x, y, w, h, color);
}

inline void BatchAddText(float x, float y, const char* text, int color) {
    if (!text || g_ESPBatchOffset >= MAX_ESP_BUFFER - 128) return;
    char safeText[64];
    int j = 0;
    for (int i = 0; text[i] && j < 62; i++) {
        if (text[i] == ',' || text[i] == ';') safeText[j++] = '_';
        else safeText[j++] = text[i];
    }
    safeText[j] = '\0';
    
    g_ESPBatchOffset += snprintf(g_ESPBatchBuffer + g_ESPBatchOffset,
        MAX_ESP_BUFFER - g_ESPBatchOffset,
        "T%.0f,%.0f,%s,%d;", x, y, safeText, color);
}

JavaVM* g_JavaVM = NULL;
jclass g_MenuClass = NULL;
jmethodID g_BatchDrawMethod = NULL;
jmethodID g_SetESPEnabledMethod = NULL;
jmethodID g_GetScreenWidthMethod = NULL;
jmethodID g_GetScreenHeightMethod = NULL;
bool g_ESPReady = false;

// LocalPlayer.OverrideOrthographicSize - RVA: 0x3DA813C
void (*OverrideOrthographicSize)(void*, float) = NULL;

// PlayableEntity.TeleportTo - RVA: 0x3DD33EC
void (*TeleportTo)(void*, Vector2, bool) = NULL;

JNIEnv* GetJNIEnv() {
    if (!g_JavaVM) return NULL;
    JNIEnv* env = NULL;
    if (g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        g_JavaVM->AttachCurrentThread(&env, NULL);
    }
    return env;
}

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
    if (!instance) return -1;
    void* rolePtr = *(void**)((uintptr_t)instance + OFFSET_PE_PLAYERROLE);
    if (!rolePtr) return -1;
    return (int)*(short*)((uintptr_t)rolePtr + OFFSET_ROLE_TYPE);
}

bool IsPlayerInLobby(void* instance) {
    if (!instance) return true;
    bool isRoleSet = *(bool*)((uintptr_t)instance + OFFSET_PE_ISPLAYERROLESET);
    return !isRoleSet;
}

float GetCurrentOrthoSize() {
    if (DroneView && g_DroneViewReady) return DroneZoom;
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

// ====================================================================================
// ROLE INFO - Source: dump.cs RoleType enum (TypeDefIndex: 1500)
// ====================================================================================

struct RoleInfo { const char* name; int color; };

bool IsKillerRole(int roleId) {
    switch(roleId) {
        case 2:   // Duck
        case 9:   // Cannibal
        case 10:  // Morphling
        case 12:  // Silencer
        case 14:  // LoverDuck
        case 17:  // Professional
        case 18:  // Spy
        case 19:  // Mimic
        case 23:  // Assassin
        case 25:  // Hitman
        case 27:  // Snitch
        case 33:  // Demolitionist
        case 36:  // GHDuck
        case 38:  // DNDDuck
        case 41:  // DNDMorphling
        case 44:  // TTVampire
        case 46:  // TTThrall
        case 48:  // IdentityThief
        case 51:  // Ninja
        case 58:  // TTEThrall
        case 59:  // TTMummy
        case 60:  // SerialKiller
        case 62:  // Warlock
        case 65:  // EsperDuck
        case 66:  // Stalker
        case 74:  // BHCCrow
        case 75:  // BHCSinEater
        case 79:  // TLCIdentityThief
        case 81:  // TLCCamoDuck
        case 84:  // Carrier
        case 85:  // Parasite
        case 103: // Looter
        case 104: // Sniper
        case 106: // Hawk
        case 108: // TTDrTurducken
        case 109: // TTTurduckensMonster
            return true;
        default:
            return false;
    }
}

RoleInfo GetRoleInfo(int roleId) {
    RoleInfo info;
    
    // Pelican - GREEN
    if (roleId == 57) {
        info.name = "Pelican";
        info.color = COLOR_GREEN;
        return info;
    }
    
    // Vulture - GREEN
    if (roleId == 16 || roleId == 40) {
        info.name = (roleId == 16) ? "Vulture" : "DND Vulture";
        info.color = COLOR_GREEN;
        return info;
    }
    
    // Pigeon - ORANGE
    if (roleId == 21) {
        info.name = "Pigeon";
        info.color = COLOR_ORANGE;
        return info;
    }
    
    // Dodo - YELLOW
    if (roleId == 3 || roleId == 34) {
        info.name = (roleId == 3) ? "Dodo" : "Dueling Dodo";
        info.color = COLOR_YELLOW;
        return info;
    }
    
    // Falcon - ORANGE
    if (roleId == 24 || roleId == 39) {
        info.name = (roleId == 24) ? "Falcon" : "DND Falcon";
        info.color = COLOR_ORANGE;
        return info;
    }
    
    // Killers - RED
    if (IsKillerRole(roleId)) {
        switch(roleId) {
            case 2: info.name = "Duck"; break;
            case 9: info.name = "Cannibal"; break;
            case 10: info.name = "Morphling"; break;
            case 12: info.name = "Silencer"; break;
            case 14: info.name = "Lover Duck"; break;
            case 17: info.name = "Professional"; break;
            case 18: info.name = "Spy"; break;
            case 19: info.name = "Mimic"; break;
            case 23: info.name = "Assassin"; break;
            case 25: info.name = "Hitman"; break;
            case 27: info.name = "Snitch"; break;
            case 33: info.name = "Demolitionist"; break;
            case 36: info.name = "GH Duck"; break;
            case 38: info.name = "DND Duck"; break;
            case 41: info.name = "DND Morphling"; break;
            case 44: info.name = "Vampire"; break;
            case 46: info.name = "Thrall"; break;
            case 48: info.name = "Identity Thief"; break;
            case 51: info.name = "Ninja"; break;
            case 58: info.name = "TTE Thrall"; break;
            case 59: info.name = "Mummy"; break;
            case 60: info.name = "Serial Killer"; break;
            case 62: info.name = "Warlock"; break;
            case 65: info.name = "Esper Duck"; break;
            case 66: info.name = "Stalker"; break;
            case 74: info.name = "Crow"; break;
            case 75: info.name = "Sin Eater"; break;
            case 79: info.name = "TLC ID Thief"; break;
            case 81: info.name = "TLC Camo Duck"; break;
            case 84: info.name = "Carrier"; break;
            case 85: info.name = "Parasite"; break;
            case 103: info.name = "Looter"; break;
            case 104: info.name = "Sniper"; break;
            case 106: info.name = "Hawk"; break;
            case 108: info.name = "Dr Turducken"; break;
            case 109: info.name = "Monster"; break;
            default: info.name = "Killer"; break;
        }
        info.color = COLOR_RED;
        return info;
    }
    
    // None and all innocents - WHITE
    switch(roleId) {
        case 0: info.name = "None"; break;
        case 1: info.name = "Goose"; break;
        case 4: info.name = "Bounty"; break;
        case 5: info.name = "Mechanic"; break;
        case 6: info.name = "Technician"; break;
        case 7: info.name = "Medium"; break;
        case 8: info.name = "Vigilante"; break;
        case 11: info.name = "Sheriff"; break;
        case 13: info.name = "Canadian"; break;
        case 15: info.name = "Lover Goose"; break;
        case 20: info.name = "Detective"; break;
        case 22: info.name = "Birdwatcher"; break;
        case 26: info.name = "Bodyguard"; break;
        case 28: info.name = "Politician"; break;
        case 29: info.name = "Locksmith"; break;
        case 30: info.name = "Mortician"; break;
        case 31: info.name = "Celebrity"; break;
        case 32: info.name = "Party Goose"; break;
        case 35: info.name = "GH Goose"; break;
        case 37: info.name = "GH Bounty"; break;
        case 42: info.name = "FP Goose"; break;
        case 43: info.name = "Explore Goose"; break;
        case 45: info.name = "Peasant"; break;
        case 47: info.name = "Spectator"; break;
        case 49: info.name = "Adventurer"; break;
        case 50: info.name = "Avenger"; break;
        case 52: info.name = "Undertaker"; break;
        case 53: info.name = "Snoop"; break;
        case 54: info.name = "Esper"; break;
        case 55: info.name = "Invisibility"; break;
        case 56: info.name = "Astral"; break;
        case 61: info.name = "Engineer"; break;
        case 63: info.name = "Street Urchin"; break;
        case 64: info.name = "Tracker"; break;
        case 67: info.name = "Preacher"; break;
        case 68: info.name = "Inquisitor"; break;
        case 69: info.name = "Saint"; break;
        case 70: info.name = "High Priest"; break;
        case 71: info.name = "Demon Hunter"; break;
        case 72: info.name = "Initiate"; break;
        case 73: info.name = "Seamstress"; break;
        case 76: info.name = "TF Goose"; break;
        case 77: info.name = "Chicken"; break;
        case 78: info.name = "TLC Bodyguard"; break;
        case 80: info.name = "TLC Undertaker"; break;
        case 82: info.name = "Cupid"; break;
        case 83: info.name = "Survivalist"; break;
        case 86: info.name = "Drone"; break;
        case 87: info.name = "Scientist"; break;
        case 88: info.name = "HNS Role"; break;
        case 89: info.name = "Owl"; break;
        case 90: info.name = "Spotter"; break;
        case 91: info.name = "HNS Sniper"; break;
        case 92: info.name = "Lobbyist"; break;
        case 93: info.name = "Lost Duckling"; break;
        case 94: info.name = "Fortune Teller"; break;
        case 95: info.name = "Mime"; break;
        case 96: info.name = "Raven"; break;
        case 97: info.name = "Rabbit"; break;
        case 98: info.name = "Lucid Dreamer"; break;
        case 99: info.name = "Clown"; break;
        case 100: info.name = "Soldier"; break;
        case 101: info.name = "Coroner"; break;
        case 102: info.name = "Sensor"; break;
        case 105: info.name = "Delusional"; break;
        case 107: info.name = "AI"; break;
        default: info.name = "Unknown"; break;
    }
    info.color = COLOR_WHITE;
    return info;
}

void ApplyDroneViewDelayed() {
    if (!localPlayerObject || !OverrideOrthographicSize) return;
    
    if (DroneView) {
        if (g_DroneViewDelay < DRONE_VIEW_DELAY_FRAMES) {
            g_DroneViewDelay++;
            return;
        }
        
        g_DroneViewReady = true;
        OverrideOrthographicSize(localPlayerObject, DroneZoom);
    }
}

void ResetDroneViewDelay() {
    g_DroneViewDelay = 0;
    g_DroneViewReady = false;
    g_ESPStabilized = false;
    g_FrameCount = 0;
}

void DisableDroneView() {
    if (localPlayerObject && OverrideOrthographicSize) {
        OverrideOrthographicSize(localPlayerObject, g_DefaultOrthoSize);
    }
    g_DroneViewReady = false;
    g_DroneViewDelay = 0;
    g_ESPStabilized = false;
    g_FrameCount = 0;
}

void InitESP(JNIEnv *env) {
    if (g_ESPReady) return;
    if (!env) return;
    if (!g_JavaVM) env->GetJavaVM(&g_JavaVM);
    
    jclass cls = env->FindClass("com/android/support/Menu");
    if (!cls) return;
    
    g_MenuClass = (jclass)env->NewGlobalRef(cls);
    
    g_BatchDrawMethod = env->GetStaticMethodID(g_MenuClass, "batchDrawESP", "(Ljava/lang/String;)V");
    g_SetESPEnabledMethod = env->GetStaticMethodID(g_MenuClass, "setESPEnabled", "(Z)V");
    g_GetScreenWidthMethod = env->GetStaticMethodID(g_MenuClass, "getScreenWidth", "()I");
    g_GetScreenHeightMethod = env->GetStaticMethodID(g_MenuClass, "getScreenHeight", "()I");
    
    g_ESPReady = (g_BatchDrawMethod != NULL);
    env->DeleteLocalRef(cls);
    
    LOGI("ESP Init: %s", g_ESPReady ? "SUCCESS" : "FAILED");
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

void SetESPEnabled(bool e) {
    JNIEnv* env = GetJNIEnv();
    if (env && g_SetESPEnabledMethod) 
        env->CallStaticVoidMethod(g_MenuClass, g_SetESPEnabledMethod, (jboolean)e);
}

void SendBatchESP(JNIEnv* env) {
    if (!env || !g_BatchDrawMethod) return;
    
    jstring jdata = env->NewStringUTF(g_ESPBatchBuffer);
    if (jdata) {
        env->CallStaticVoidMethod(g_MenuClass, g_BatchDrawMethod, jdata);
        env->DeleteLocalRef(jdata);
    }
}

inline bool WorldToScreen(Vector2 world, Vector2 localPos, float scale, float* sx, float* sy) {
    float dx = world.x - localPos.x;
    float dy = world.y - localPos.y;
    
    float cx = g_ScreenWidth * 0.5f;
    float cy = g_ScreenHeight * 0.5f;
    
    *sx = cx + (dx * scale);
    *sy = cy - (dy * scale);
    
    return true;
}

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

void RenderDebugPanelBatch() {
    if (!DebugMode) return;
    
    float centerX = g_ScreenWidth * 0.5f;
    float startY = 100;
    float lineHeight = 28;
    float maxY = g_ScreenHeight - 80;
    
    BatchAddText(centerX, startY, "=== DEBUG INFO ===", COLOR_MAGENTA);
    startY += lineHeight + 5;
    
    char buf[256];
    
    snprintf(buf, sizeof(buf), "Screen:%.0fx%.0f | Ortho:%.1f | Scale:%.0f", 
             g_ScreenWidth, g_ScreenHeight, GetCurrentOrthoSize(), GetESPScale());
    BatchAddText(centerX, startY, buf, COLOR_CYAN);
    startY += lineHeight;
    
    snprintf(buf, sizeof(buf), "MyPos: X=%.2f Y=%.2f", 
             g_LocalPlayerPos.x, g_LocalPlayerPos.y);
    BatchAddText(centerX, startY, buf, COLOR_CYAN);
    startY += lineHeight;
    
    snprintf(buf, sizeof(buf), "Drone:%c | Ready:%c | Delay:%d/%d | Zoom:%.1f", 
             DroneView ? 'Y' : 'N', 
             g_DroneViewReady ? 'Y' : 'N',
             g_DroneViewDelay, DRONE_VIEW_DELAY_FRAMES,
             DroneZoom);
    BatchAddText(centerX, startY, buf, COLOR_CYAN);
    startY += lineHeight;
    
    snprintf(buf, sizeof(buf), "TP Target: X=%.0f Y=%.0f | Saved: X=%.1f Y=%.1f", 
             TeleportX, TeleportY, SavedPosX, SavedPosY);
    BatchAddText(centerX, startY, buf, COLOR_YELLOW);
    startY += lineHeight;
    
    snprintf(buf, sizeof(buf), "InGame:%c | Lobby:%c | Vote:%c | Players:%d", 
             isInGame ? 'Y' : 'N', isInLobby ? 'Y' : 'N', 
             isInVotingScreen ? 'Y' : 'N', g_RenderPlayerCount);
    BatchAddText(centerX, startY, buf, COLOR_CYAN);
    startY += lineHeight;
    
    RoleInfo myRole = GetRoleInfo(localPlayerRole);
    snprintf(buf, sizeof(buf), "[ME] %s (ID:%d)", myRole.name, localPlayerRole);
    BatchAddText(centerX, startY, buf, myRole.color);
    startY += lineHeight + 8;
    
    BatchAddText(centerX, startY, "--- PLAYER LIST ---", COLOR_YELLOW);
    startY += lineHeight;
    
    for (int i = 0; i < g_RenderPlayerCount && startY < maxY; i++) {
        PlayerData* p = &g_RenderPlayers[i];
        if (!p->isValid) continue;
        
        RoleInfo ri = GetRoleInfo(p->role);
        
        char flags[16] = "";
        int fi = 0;
        if (p->isLocal) flags[fi++] = 'L';
        if (p->isGhost) flags[fi++] = 'G';
        if (p->isDowned) flags[fi++] = 'D';
        if (p->inVent) flags[fi++] = 'V';
        if (p->isInvisible) flags[fi++] = 'I';
        if (p->isSpectator) flags[fi++] = 'S';
        if (p->isMorphed) flags[fi++] = 'M';
        if (p->isRunning) flags[fi++] = 'R';
        if (p->hasKilledThisRound) flags[fi++] = 'K';
        flags[fi] = '\0';
        if (fi == 0) strcpy(flags, "-");
        
        snprintf(buf, sizeof(buf), "#%02d %-8s %s(%d) T%d (%.1f|%.1f) %.0fm [%s]",
                 p->entityNumber,
                 p->name,
                 ri.name, p->role,
                 p->teamId,
                 p->position.x, p->position.y,
                 p->distanceToLocal,
                 flags);
        
        int color = p->isLocal ? COLOR_CYAN : (p->isGhost ? COLOR_GRAY : ri.color);
        BatchAddText(centerX, startY, buf, color);
        startY += lineHeight;
    }
    
    if (startY < maxY - lineHeight) {
        startY += 10;
        BatchAddText(centerX, startY, "Flags: L=Local G=Ghost D=Down V=Vent I=Invis S=Spec M=Morph R=Run K=Kill", COLOR_GRAY);
    }
}

void RenderESPBatch() {
    if (!ESPEnabled) return;
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
        
        RoleInfo ri = GetRoleInfo(p->role);
        int color = ri.color;
        
        float dist = p->distanceToLocal;
        
        bool onScreen = (sx >= 0 && sx <= g_ScreenWidth && sy >= 0 && sy <= g_ScreenHeight);
        
        if (ESPLines) {
            float lx1 = cx, ly1 = cy, lx2 = sx, ly2 = sy;
            if (ClipLine(&lx1, &ly1, &lx2, &ly2)) {
                BatchAddLine(lx1, ly1, lx2, ly2, color);
            }
        }
        
        if (onScreen) {
            float boxW = 0.9f * scale, boxH = 1.6f * scale;
            if (boxW < 45) boxW = 45; if (boxH < 70) boxH = 70;
            if (boxW > 200) boxW = 200; if (boxH > 320) boxH = 320;
            
            float boxX = sx - boxW * 0.5f;
            float boxY = sy - boxH * 0.40f;
            
            if (ESPBox) BatchAddBox(boxX, boxY, boxW, boxH, color);
            
            if (ESPDistance) {
                char dt[16];
                snprintf(dt, sizeof(dt), "%.1fm", dist);
                BatchAddText(sx, boxY - 25, dt, COLOR_YELLOW);
            }
            
            if (ESPName) {
                BatchAddText(sx, boxY - 55, p->name, color);
            }
        } 
        else if (ESPEdgeIndicator) {
            float edgeX, edgeY;
            GetEdgePosition(sx, sy, &edgeX, &edgeY);
            
            BatchAddBox(edgeX - 8, edgeY - 8, 16, 16, color);
            
            char et[48];
            snprintf(et, sizeof(et), "%s %.0fm", p->name, dist);
            
            float tx = edgeX, ty = edgeY - 25;
            if (edgeX < 150) tx = 150;
            else if (edgeX > g_ScreenWidth - 150) tx = g_ScreenWidth - 150;
            if (edgeY < 80) ty = edgeY + 35;
            
            BatchAddText(tx, ty, et, color);
        }
    }
}

void ClearAllESP() {
    g_PlayerCount = 0;
    g_RenderPlayerCount = 0;
    g_ESPStabilized = false;
    g_FrameCount = 0;
    isInGame = false;
    isInLobby = true;
    localPlayerInstance = NULL;
    localPlayerObject = NULL;
    
    ResetDroneViewDelay();
    
    SetESPEnabled(false);
}

// PlayableEntity.Update - RVA: 0x3DC28D8
void (*old_Update)(void *instance);
void Update(void *instance) {
    if (instance) {
        bool isLocal = *(bool*)((uintptr_t)instance + OFFSET_PE_ISLOCAL);
        
        if (isLocal) {
            localPlayerInstance = instance;
            isInGame = true;
            localPlayerRole = GetRoleType(instance);
            isInLobby = IsPlayerInLobby(instance);
            
            g_PlayerCount = 0;
            g_LocalPlayerPos = GetPlayerPosition(instance, true);
            
            if (UnlimitedVision) {
                *(bool*)((uintptr_t)instance + OFFSET_PE_FOGOFWAR) = false;
            }
            
            if (btnSetPosition) {
                SavedPosX = g_LocalPlayerPos.x;
                SavedPosY = g_LocalPlayerPos.y;
                TeleportX = g_LocalPlayerPos.x;
                TeleportY = g_LocalPlayerPos.y;
                btnSetPosition = false;
                LOGI("Position saved: %.2f, %.2f", SavedPosX, SavedPosY);
            }
            
            if (btnTeleport && TeleportTo) {
                Vector2 targetPos = {TeleportX, TeleportY};
                TeleportTo(instance, targetPos, true);
                btnTeleport = false;
                LOGI("Teleported to: %.2f, %.2f", TeleportX, TeleportY);
            }
        }
        
        if ((ESPEnabled || DebugMode) && g_PlayerCount < MAX_PLAYERS) {
            PlayerData* p = &g_Players[g_PlayerCount];
            p->position = GetPlayerPosition(instance, isLocal);
            p->isGhost = *(bool*)((uintptr_t)instance + OFFSET_PE_ISGHOST);
            p->isLocal = isLocal;
            p->role = GetRoleType(instance);
            p->teamId = *(int*)((uintptr_t)instance + OFFSET_PE_TEAMID);
            p->entityNumber = *(int*)((uintptr_t)instance + OFFSET_PE_ENTITYNUMBER);
            WideCharToAscii(instance, p->name, sizeof(p->name));
            
            p->isDowned = *(bool*)((uintptr_t)instance + OFFSET_PE_ISDOWNED);
            p->inVent = *(bool*)((uintptr_t)instance + OFFSET_PE_INVENT);
            p->isInvisible = *(bool*)((uintptr_t)instance + OFFSET_PE_ISINVISIBLE);
            p->isSpectator = *(bool*)((uintptr_t)instance + OFFSET_PE_ISSPECTATOR);
            p->isMorphed = *(bool*)((uintptr_t)instance + OFFSET_PE_ISMORPHED);
            p->isRunning = *(bool*)((uintptr_t)instance + OFFSET_PE_ISRUNNING);
            p->hasKilledThisRound = *(bool*)((uintptr_t)instance + OFFSET_PE_HASKILLED);
            p->opacity = *(float*)((uintptr_t)instance + OFFSET_PE_TARGETOPACITY);
            
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

// PlayableEntity.LateUpdate - RVA: 0x3DC368C
void (*old_LateUpdate)(void *instance);
void LateUpdate(void *instance) {
    old_LateUpdate(instance);
    
    if (instance && g_ESPReady) {
        bool isLocal = *(bool*)((uintptr_t)instance + OFFSET_PE_ISLOCAL);
        
        if (isLocal) {
            g_RenderPlayerCount = g_PlayerCount;
            memcpy(g_RenderPlayers, g_Players, sizeof(PlayerData) * g_PlayerCount);
            
            static int screenUpdateCounter = 0;
            if (++screenUpdateCounter >= 60) {
                UpdateScreenSize();
                screenUpdateCounter = 0;
            }
            
            if (ESPEnabled || DebugMode) {
                BatchClear();
                
                if (DebugMode) RenderDebugPanelBatch();
                if (ESPEnabled) RenderESPBatch();
                
                JNIEnv* env = GetJNIEnv();
                if (env) {
                    SendBatchESP(env);
                }
            }
        }
    }
}

// PlayableEntity.TurnIntoGhost - RVA: 0x3DCE370
void (*old_TurnIntoGhost)(void *instance, int deathReason);
void TurnIntoGhost(void *instance, int deathReason) {
    if (AntiDeath && instance == localPlayerInstance) return;
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
            ApplyDroneViewDelayed();
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

// OnEnterVent - RVA: 0x3C55C94
void (*old_OnEnterVent)(void *instance, void* vent, bool setCooldown);
void OnEnterVent(void *instance, void* vent, bool setCooldown) {
    old_OnEnterVent(instance, vent, NoCooldown ? false : setCooldown);
}

// OnExitVent - RVA: 0x3C55D88
void (*old_OnExitVent)(void *instance, void* vent, bool setCooldown);
void OnExitVent(void *instance, void* vent, bool setCooldown) {
    old_OnExitVent(instance, vent, NoCooldown ? false : setCooldown);
}

// SetVentCooldown - RVA: 0x3C548AC
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

jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    InitESP(env);
    
    const char *features[] = {
        OBFUSCATE("Category_Vision & Cooldown"),
        OBFUSCATE("Toggle_Unlimited Vision"),              // 0
        OBFUSCATE("Toggle_No Vent Cooldown"),              // 1
        
        OBFUSCATE("Category_Camera"),
        OBFUSCATE("Toggle_Drone View"),                    // 2
        OBFUSCATE("SeekBar_Zoom Level_5_25"),              // 3
        
        OBFUSCATE("Category_ESP Settings"),
        OBFUSCATE("Toggle_ESP Enabled"),                   // 4
        OBFUSCATE("Toggle_True_ESP Lines"),                // 5
        OBFUSCATE("Toggle_True_ESP Box"),                  // 6
        OBFUSCATE("Toggle_True_ESP Distance"),             // 7
        OBFUSCATE("Toggle_True_ESP Names"),                // 8
        OBFUSCATE("Toggle_True_Edge Indicator"),           // 9
        OBFUSCATE("Toggle_True_Hide in Vote Screen"),      // 10
        OBFUSCATE("Toggle_Hide in Lobby"),                 // 11
        
        OBFUSCATE("Category_Teleport"),
        OBFUSCATE("InputValue_999_Teleport X"),            // 12
        OBFUSCATE("InputValue_999_Teleport Y"),            // 13
        OBFUSCATE("Button_Set Current Position"),          // 14
        OBFUSCATE("Button_Teleport Now"),                  // 15
        
        OBFUSCATE("Category_Debug Panel"),
        OBFUSCATE("Toggle_Show Debug Info"),               // 16
        
        OBFUSCATE("Category_Experimental [May Not Work]"),
        OBFUSCATE("Toggle_Anti-Death [LOCAL]"),            // 17
        OBFUSCATE("Toggle_Speed Boost [LOCAL]"),           // 18
        OBFUSCATE("SeekBar_Speed Multiplier_10_40"),       // 19
		
		OBFUSCATE("Category_About"),
        OBFUSCATE("RichTextView_<b>Goose Goose Duck Mod Menu</b><br/>Free and open source mod for Android.<br/>Use at your own risk!"),
        OBFUSCATE("ButtonLink_YouTube: @anonimbiri_IsBack_https://youtube.com/@anonimbiri_IsBack"),
        OBFUSCATE("ButtonLink_Developer: anonimbiri_https://github.com/anonimbiri-IsBack"),
        OBFUSCATE("ButtonLink_GitHub Open Source_https://github.com/GameSketchers/Goose-Goose-Duck-Android-Mod"),

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
                *(bool*)((uintptr_t)localPlayerInstance + OFFSET_PE_FOGOFWAR) = true;
            break;
            
        case 1:
            NoCooldown = boolean;
            break;
            
        case 2:
            DroneView = boolean;
            if (!boolean) {
                DisableDroneView();
            } else {
                ResetDroneViewDelay();
            }
            break;
            
        case 3:
            DroneZoom = (float)value;
            if (DroneView && g_DroneViewReady && localPlayerObject && OverrideOrthographicSize) {
                OverrideOrthographicSize(localPlayerObject, DroneZoom);
            }
            break;
            
        case 4:
            ESPEnabled = boolean;
            if (boolean) {
                g_ESPStabilized = false;
                g_FrameCount = 0;
            }
            SetESPEnabled(boolean || DebugMode);
            break;
            
        case 5: ESPLines = boolean; break;
        case 6: ESPBox = boolean; break;
        case 7: ESPDistance = boolean; break;
        case 8: ESPName = boolean; break;
        case 9: ESPEdgeIndicator = boolean; break;
        case 10: ESPHideInVote = boolean; break;
        case 11: ESPHideInLobby = boolean; break;
        
        case 12:
            TeleportX = (float)value;
            break;
            
        case 13:
            TeleportY = (float)value;
            break;
            
        case 14:
            btnSetPosition = true;
            break;
            
        case 15:
            btnTeleport = true;
            break;
        
        case 16:
            DebugMode = boolean;
            SetESPEnabled(boolean || ESPEnabled);
            break;
            
        case 17:
            AntiDeath = boolean;
            break;
            
        case 18:
            SpeedHack = boolean;
            break;
            
        case 19:
            SpeedMultiplier = (float)value / 10.0f;
            break;
    }
}

ElfScanner g_il2cppELF;

void hack_thread() {
    LOGI("pthread created");

    while (!isLibraryLoaded(targetLibName)) sleep(1);

    do { sleep(1); g_il2cppELF = ElfScanner::createWithPath(targetLibName); } 
    while (!g_il2cppELF.isValid());

    LOGI("%s loaded", (const char*)targetLibName);

#if defined(__aarch64__)
    // PlayableEntity.Update - RVA: 0x3DC28D8
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DC28D8")), Update, old_Update);
    
    // PlayableEntity.LateUpdate - RVA: 0x3DC368C
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DC368C")), LateUpdate, old_LateUpdate);
    
    // PlayableEntity.TurnIntoGhost - RVA: 0x3DCE370
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DCE370")), TurnIntoGhost, old_TurnIntoGhost);
    
    // PlayableEntity.Despawn - RVA: 0x3DC5328
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DC5328")), Despawn, old_Despawn);
    
    // LocalPlayer.Update - RVA: 0x3D986B8
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3D986B8")), LocalPlayer_Update, old_LocalPlayer_Update);
    
    // LocalPlayer.GetPlayerSpeed - RVA: 0x3DA7768
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DA7768")), GetPlayerSpeed, old_GetPlayerSpeed);
    
    // LocalPlayer.OverrideOrthographicSize - RVA: 0x3DA813C
    OverrideOrthographicSize = (void (*)(void*, float))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3DA813C")));
    
    // PlayableEntity.TeleportTo - RVA: 0x3DD33EC
    TeleportTo = (void (*)(void*, Vector2, bool))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3DD33EC")));
    
    // OnEnterVent - RVA: 0x3C55C94
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3C55C94")), OnEnterVent, old_OnEnterVent);
    
    // OnExitVent - RVA: 0x3C55D88
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3C55D88")), OnExitVent, old_OnExitVent);
    
    // SetVentCooldown - RVA: 0x3C548AC
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3C548AC")), SetVentCooldown, old_SetVentCooldown);
    
    LOGI("All hooks installed!");
#endif
    LOGI("Done");
}

__attribute__((constructor))
void lib_main() { std::thread(hack_thread).detach(); }
