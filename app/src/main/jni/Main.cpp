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
#include <chrono>
#include <atomic>
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.hpp"
#include "Menu/Menu.hpp"
#include "Menu/Jni.hpp"
#include "Includes/Macros.h"

#define targetLibName OBFUSCATE("libil2cpp.so")

// Mini Map Ayarları
bool MiniMapEnabled = false;
bool MiniMapShowPlayers = true;
bool MiniMapShowDead = true;
bool MiniMapTouchTP = true;
bool MiniMapHideInVote = true;

// ESP Ayarları (Orijinal Yapı)
bool ESPEnabled = true;
bool ESPLines = true;
bool ESPBox = true;
bool ESPDistance = true;
bool ESPName = true;
bool ESPEdgeIndicator = true;
bool ESPHideInVote = true;
bool ESPHideInLobby = false;
bool SeeGhosts = true;
bool DebugMode = false;

// Görev & Sabotaj Ayarları
bool SafeAutoTasks = false;
bool AutoRepairSabotage = false;
bool btnRepairSabotageNow = false;
bool btnCompleteOneTask = false;
bool btnUnlockSabotages = false;

// İletişim
bool HearDeadVoice = false;
bool HearFarPlayers = false;

// Hareket & Diğer
bool AutoReady = false;
bool AntiDeath = false;
bool UnlimitedVision = false;
bool NoCooldown = false;
bool NoClip = false;
bool RemoveRoof = false;
bool DroneView = false;
float DroneZoom = 5.0f;
bool SpeedHack = false;
float SpeedMultiplier = 1.5f;

bool btnCallEmergency = false;

// Thread-safe teleport kuyruğu (Crash engelleme)
std::atomic<bool> g_PendingDirectTP(false);
std::atomic<float> g_TargetDirectTPX(0.0f);
std::atomic<float> g_TargetDirectTPY(0.0f);

void* g_TasksHandler = NULL;
void* g_RoofHandler = NULL;
void* g_GameManager = NULL;
void* g_PlayerPropertiesManager = NULL;
void* g_MapManager = NULL;
bool g_RoofRemovedThisRound = false;

std::chrono::steady_clock::time_point g_LastSafeTaskTime;
std::chrono::steady_clock::time_point g_SabotageDetectedTime;
bool g_SabotagePending = false;

void *localPlayerInstance = NULL;
void *localPlayerObject = NULL;
void *mainCameraObject = NULL;
bool isInVotingScreen = false;
bool isInSpotlightScreen = false;
bool isInGame = false;
bool isInLobby = true;
int localPlayerRole = 0;
int g_CurrentGameState = 0;
int g_CurrentMapId = -1;
char g_CurrentMapName[64] = "None";

int g_DroneViewDelay = 0;
#define DRONE_VIEW_DELAY_FRAMES 60
bool g_DroneViewReady = false;
bool g_DroneViewInitialized = false;

float g_CameraOrthoSize = 5.0f;

struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };
struct Quaternion { float x, y, z, w; };

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
#define COLOR_GOLD     0xFFFFD700

// PlayableEntity (TypeDefIndex: 6395)
#define OFFSET_PE_ENTITYNUMBER       0x88
#define OFFSET_PE_NICKNAME           0x90
#define OFFSET_PE_ISLOCAL            0x98
#define OFFSET_PE_PLAYERROLE         0xA0
#define OFFSET_PE_ISPLAYERROLESET    0xA8
#define OFFSET_PE_KILLEDBY           0xB0
#define OFFSET_PE_TARGETOPACITY      0xB8
#define OFFSET_PE_TASKSREMAINING     0xC0
#define OFFSET_PE_HASKILLED          0xC5
#define OFFSET_PE_TEAMID             0xDC
#define OFFSET_PE_FOGOFWAR           0xF4
#define OFFSET_PE_KILLEDLOCATION     0x100
#define OFFSET_PE_ISRUNNING          0x121
#define OFFSET_PE_ISGHOST            0x178
#define OFFSET_PE_ISINFECTED         0x17E
#define OFFSET_PE_ISDOWNED           0x180
#define OFFSET_PE_INVENT             0x182
#define OFFSET_PE_HASBOMB            0x187
#define OFFSET_PE_ISINVISIBLE        0x189
#define OFFSET_PE_ISINPELICAN        0x18D
#define OFFSET_PE_ISMORPHED          0x18F
#define OFFSET_PE_ISSPECTATOR        0x200
#define OFFSET_PE_COSMETICS          0x230
#define OFFSET_PE_TRANSFORMVIEW      0x2C8
#define OFFSET_PE_PLAYERCOLLIDER     0x2E0
#define OFFSET_PE_WALLCHECKCOLLIDER  0x2E8
#define OFFSET_PE_WALLCOLLISIONHANDLER 0x2F0
#define OFFSET_PE_CONFINECOLLIDER    0x2F8
#define OFFSET_PE_STATIC_DEADPLAYERSCOUNT  0x4

// MapManager (TypeDefIndex: 1288)
#define OFFSET_MM_ROOMMAP            0x180

// PlayerController (TypeDefIndex: 6407)
#define OFFSET_PC_READYSTATE         0x388

// LocalPlayer (TypeDefIndex: 6370)
#define OFFSET_LP_MAINCAMERA              0x78
#define OFFSET_LP_STATECAMERA             0x80
#define OFFSET_LP_SCRIPTABLESTATE         0x88
#define OFFSET_LP_INVOTINGSCREEN          0xC3
#define OFFSET_LP_INVOTINGTRANSITION      0xC4
#define OFFSET_LP_INGAMESTARTSPOTLIGHT    0xC5
#define OFFSET_LP_INGAMEENDSPOTLIGHT      0xC6
#define OFFSET_LP_CANSEEGHOSTS            0x150

// GameManager (TypeDefIndex: 22788)
#define OFFSET_GM_GAMESTATE               0x148

// BetterPhotonTransformView (TypeDefIndex: 2016)
#define OFFSET_TV_LATESTPOS          0x30
#define OFFSET_TV_LASTTRANSFORMPOS   0x38

// CinemachineStateDrivenCamera (TypeDefIndex: 20534)
#define OFFSET_CSDC_STATE            0x108
#define OFFSET_CS_RAWPOSITION        0x4C

// GGDRole (TypeDefIndex: 5744)
#define OFFSET_ROLE_TYPE             0x10

// TasksHandler (TypeDefIndex: 6344)
#define OFFSET_TH_SORTEDASSIGNEDTASKS  0x38

// GameTask (TypeDefIndex: 5656)
#define OFFSET_GT_TASKID             0x10
#define OFFSET_GT_ISSABOTAGE         0x53
#define OFFSET_GT_ISIMPOSTORTASK     0x80
#define OFFSET_GT_ISFAKETASK         0xD1

// WallCollisionCheckHandler (TypeDefIndex: 1106)
#define OFFSET_WCCH_INWALL           0x20

// PlayerProperties (TypeDefIndex: 1537)
#define OFFSET_PP_READYSTATE         0x14

struct PlayerInfo {
    void* instance;
    bool isLocal;
    bool isValid;
};

struct PlayerData {
    void* instance;
    Vector2 position;
    Vector2 killedLocation;
    bool hasKilledLoc;
    bool isGhost;
    bool isLocal;
    int role;
    int teamId;
    int entityNumber;
    int cosmColorVal;
    char name[128];
    bool isValid;
    float distanceToCamera;
    bool isDowned;
    bool inVent;
    bool isInvisible;
    bool isInPelican;
    bool isSpectator;
    bool isMorphed;
    bool isRunning;
    bool hasKilledThisRound;
    float opacity;
    bool isInfected;
    bool hasBomb;
    int tasksRemaining;
    char killedBy[128];
    bool confirmedKiller;
};

#define MAX_PLAYERS 20
PlayerInfo g_PlayerInstances[MAX_PLAYERS];
int g_PlayerInstanceCount = 0;
PlayerData g_RenderPlayers[MAX_PLAYERS];
int g_RenderPlayerCount = 0;

Vector2 g_LocalPlayerPos;
Vector3 g_CameraPosition = {0, 0, 0};
void* g_CinemachineCamera = nullptr;
bool g_CameraPositionValid = false;

int g_DeadPlayersCount = 0;
int g_LocalTasksRemaining = 0;
char g_LocalKilledBy[128] = "";
bool g_LocalIsInfected = false;
bool g_LocalHasBomb = false;

float g_ScreenWidth = 1080.0f;
float g_ScreenHeight = 2400.0f;
float g_DefaultOrthoSize = 5.0f;

int g_FrameCount = 0;
bool g_ESPStabilized = false;
#define STABILIZE_FRAMES 3

#define MAX_ESP_BUFFER 32768
char g_ESPBatchBuffer[MAX_ESP_BUFFER];
int g_ESPBatchOffset = 0;

char g_MiniMapBatchBuffer[8192];
static bool s_LastMiniMapState = false;

inline void BatchClear() {
    g_ESPBatchOffset = 0;
    g_ESPBatchBuffer[0] = '\0';
}

inline void BatchAddLine(float x1, float y1, float x2, float y2, int color) {
    if (g_ESPBatchOffset >= MAX_ESP_BUFFER - 64) return;
    g_ESPBatchOffset += snprintf(g_ESPBatchBuffer + g_ESPBatchOffset, MAX_ESP_BUFFER - g_ESPBatchOffset,
                                 "L%.0f,%.0f,%.0f,%.0f,%d;", x1, y1, x2, y2, color);
}

inline void BatchAddBox(float x, float y, float w, float h, int color) {
    if (g_ESPBatchOffset >= MAX_ESP_BUFFER - 64) return;
    g_ESPBatchOffset += snprintf(g_ESPBatchBuffer + g_ESPBatchOffset, MAX_ESP_BUFFER - g_ESPBatchOffset,
                                 "B%.0f,%.0f,%.0f,%.0f,%d;", x, y, w, h, color);
}

inline void BatchAddText(float x, float y, const char* text, int color) {
    if (!text || g_ESPBatchOffset >= MAX_ESP_BUFFER - 256) return;
    char safeText[128];
    int j = 0;
    for (int i = 0; text[i] && j < 126; i++) {
        if (text[i] == ',' || text[i] == ';') safeText[j++] = ' ';
        else safeText[j++] = text[i];
    }
    safeText[j] = '\0';
    g_ESPBatchOffset += snprintf(g_ESPBatchBuffer + g_ESPBatchOffset, MAX_ESP_BUFFER - g_ESPBatchOffset,
                                 "T%.0f,%.0f,%s,%d;", x, y, safeText, color);
}

inline void BatchAddIcon(float x, float y, const char* icon, int color) {
    if (!icon || g_ESPBatchOffset >= MAX_ESP_BUFFER - 64) return;
    g_ESPBatchOffset += snprintf(g_ESPBatchBuffer + g_ESPBatchOffset, MAX_ESP_BUFFER - g_ESPBatchOffset,
                                 "I%.0f,%.0f,%s,%d;", x, y, icon, color);
}

JavaVM* g_JavaVM = NULL;
jclass g_MenuClass = NULL;
jmethodID g_BatchDrawMethod = NULL;
jmethodID g_SetESPEnabledMethod = NULL;
jmethodID g_GetScreenWidthMethod = NULL;
jmethodID g_GetScreenHeightMethod = NULL;

jmethodID g_SetMiniMapVisibleMethod = NULL;
jmethodID g_SetMiniMapIdMethod = NULL;
jmethodID g_UpdateMiniMapBatchMethod = NULL;
jmethodID g_SetMapShowPlayersMethod = NULL;
jmethodID g_SetMapShowDeadBodiesMethod = NULL;
jmethodID g_SetMapTouchTeleportMethod = NULL;

bool g_ESPReady = false;

// LocalPlayer.OverrideOrthographicSize - RVA: 0x3E455BC
void (*OverrideOrthographicSize)(void*, float) = NULL;

// PlayableEntity.TeleportTo - RVA: 0x3E61054
void (*TeleportTo)(void*, Vector2, bool) = NULL;

// LocalPlayer.SetCanSeeGhosts - RVA: 0x3E4CF04
void (*SetCanSeeGhosts)(void*, bool) = NULL;

// TasksHandler.CompleteTask - RVA: 0x3E2D680
void (*TasksHandler_CompleteTask)(void*, void*, bool, bool, bool, bool) = NULL;

// TasksHandler.UpdateTaskVisuals - RVA: 0x3E2EDEC
void (*TasksHandler_UpdateTaskVisuals)(void*) = NULL;

// RoofHandler.DeactivateRoofs - RVA: 0x3DBF188
void (*RoofHandler_DeactivateRoofs)(void*, bool) = NULL;

// PlayerController.CallEmergency - RVA: 0x3E72C18
void (*PlayerController_CallEmergency)(void*) = NULL;

// Collider2D.set_isTrigger - RVA: 0x754CABC
void (*Collider2D_set_isTrigger)(void*, bool) = NULL;

// GameManager.IsInGame - RVA: 0x3ADC5B4
bool (*GameManager_IsInGame)(void*) = NULL;

// GameManager.IsInLobby - RVA: 0x3ADC5C4
bool (*GameManager_IsInLobby)(void*) = NULL;

// GameManager.IsInMeeting - RVA: 0x3ADC5D4
bool (*GameManager_IsInMeeting)(void*) = NULL;

// PlayerPropertiesManager.ChangeReadyState - RVA: 0x3AC4978
void (*PlayerPropertiesManager_ChangeReadyState)(void*, int) = NULL;

// PlayerPropertiesManager.GetUserProperties - RVA: 0x3AC5A98
void* (*PlayerPropertiesManager_GetUserProperties)(void*) = NULL;

typedef void* (*il2cpp_string_new_t)(const char*);
il2cpp_string_new_t il2cpp_string_new_func = NULL;

JNIEnv* GetJNIEnv() {
    if (!g_JavaVM) return NULL;
    JNIEnv* env = NULL;
    if (g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        g_JavaVM->AttachCurrentThread(&env, NULL);
    }
    return env;
}

extern "C" JNIEXPORT void JNICALL
Java_com_android_support_MiniMapView_nativeDirectTeleport(JNIEnv *env, jclass clazz, jfloat x, jfloat y) {
    g_TargetDirectTPX.store(x);
    g_TargetDirectTPY.store(y);
    g_PendingDirectTP.store(true);
}

Vector2 GetCameraPosition2D() {
    if (g_CameraPositionValid) return {g_CameraPosition.x, g_CameraPosition.y};
    return g_LocalPlayerPos;
}

void WideCharToUTF8(void* instance, uintptr_t offset, char* outName, int maxLen) {
    memset(outName, 0, maxLen);
    if (!instance) return;
    uintptr_t strPtr = *(uintptr_t*)((uintptr_t)instance + offset);
    if (!strPtr) { strcpy(outName, ""); return; }
    int length = *(int*)(strPtr + 0x10);
    uint16_t* chars = (uint16_t*)(strPtr + 0x14);
    if (length <= 0 || length > 50) { strcpy(outName, ""); return; }
    int outIdx = 0;
    for (int i = 0; i < length && outIdx < maxLen - 4; i++) {
        uint16_t c = chars[i];
        if (c < 0x80) { if (c >= 0x20) outName[outIdx++] = (char)c; }
        else if (c < 0x800) {
            outName[outIdx++] = (char)(0xC0 | (c >> 6));
            outName[outIdx++] = (char)(0x80 | (c & 0x3F));
        }
        else {
            outName[outIdx++] = (char)(0xE0 | (c >> 12));
            outName[outIdx++] = (char)(0x80 | ((c >> 6) & 0x3F));
            outName[outIdx++] = (char)(0x80 | (c & 0x3F));
        }
    }
    outName[outIdx] = '\0';
}

void GetPlayerNickname(void* instance, char* outName, int maxLen) {
    WideCharToUTF8(instance, OFFSET_PE_NICKNAME, outName, maxLen);
    if (outName[0] == '\0') strcpy(outName, "Player");
}

void GetKilledBy(void* instance, char* outName, int maxLen) {
    WideCharToUTF8(instance, OFFSET_PE_KILLEDBY, outName, maxLen);
}

void GetTaskId(void* task, char* outId, int maxLen) {
    WideCharToUTF8(task, OFFSET_GT_TASKID, outId, maxLen);
}

int GetRoleType(void* instance) {
    if (!instance) return -1;
    void* rolePtr = *(void**)((uintptr_t)instance + OFFSET_PE_PLAYERROLE);
    if (!rolePtr) return -1;
    return (int)*(short*)((uintptr_t)rolePtr + OFFSET_ROLE_TYPE);
}

int GetPlayerColorId(void* instance) {
    if (!instance) return 0;
    void* cosm = *(void**)((uintptr_t)instance + OFFSET_PE_COSMETICS);
    if (cosm) {
        int colorId = *(int*)((uintptr_t)cosm + 0x10);
        if (colorId >= 0 && colorId < 20) return colorId;
    }
    int entityNum = *(int*)((uintptr_t)instance + OFFSET_PE_ENTITYNUMBER);
    return (entityNum >= 0 && entityNum < 20) ? entityNum : 0;
}

void DetectCurrentMap() {
    if (g_MapManager) {
        WideCharToUTF8(g_MapManager, OFFSET_MM_ROOMMAP, g_CurrentMapName, sizeof(g_CurrentMapName));
        if (g_CurrentMapName[0] != '\0') {
            int parsedId = atoi(g_CurrentMapName);
            if (parsedId >= 0 && parsedId < 15) {
                g_CurrentMapId = parsedId;
            }
        }
    }
}

float GetCurrentOrthoSize() {
    if (DroneView && g_DroneViewReady) return DroneZoom;
    return g_DefaultOrthoSize;
}

const char* GetGameStateName(int state) {
    switch (state) {
        case 0: return "InLobby";
        case 1: return "Drafting";
        case 2: return "InGame";
        case 3: return "Opening";
        case 4: return "Discussion";
        case 5: return "Voting";
        case 6: return "Waiting";
        case 7: return "Proceeding";
        default: return "Unknown";
    }
}

Vector2 GetPlayerPosition(void* instance, bool forLocal) {
    Vector2 pos = {0, 0};
    if (!instance) return pos;
    void* tv = *(void**)((uintptr_t)instance + OFFSET_PE_TRANSFORMVIEW);
    if (tv) {
        if (forLocal) {
            pos = *(Vector2*)((uintptr_t)tv + OFFSET_TV_LASTTRANSFORMPOS);
            if (pos.x == 0 && pos.y == 0)
                pos = *(Vector2*)((uintptr_t)tv + OFFSET_TV_LATESTPOS);
        } else {
            pos = *(Vector2*)((uintptr_t)tv + OFFSET_TV_LATESTPOS);
        }
    }
    return pos;
}

float GetESPScale() {
    float orthoSize = g_DefaultOrthoSize;
    if (orthoSize <= 0) orthoSize = 5.0f;
    return g_ScreenHeight / (orthoSize * 2.0f);
}

struct RoleInfo { const char* name; int color; };

bool IsKillerRole(int roleId) {
    switch(roleId) {
        case 2: case 9: case 10: case 12: case 14: case 17: case 18: case 19:
        case 23: case 25: case 27: case 33: case 36: case 38: case 41: case 44:
        case 46: case 48: case 51: case 58: case 59: case 60: case 62: case 65:
        case 66: case 74: case 75: case 79: case 81: case 84: case 85: case 103:
        case 104: case 106: case 108: case 109: case 110:
            return true;
        default:
            return false;
    }
}

RoleInfo GetRoleInfo(int roleId) {
    RoleInfo info;
    if (roleId == 57) { info.name = "Pelican"; info.color = COLOR_GREEN; return info; }
    if (roleId == 16 || roleId == 40) { info.name = (roleId == 16) ? "Vulture" : "DND Vulture"; info.color = COLOR_GREEN; return info; }
    if (roleId == 21) { info.name = "Pigeon"; info.color = COLOR_ORANGE; return info; }
    if (roleId == 3 || roleId == 34) { info.name = (roleId == 3) ? "Dodo" : "Dueling Dodo"; info.color = COLOR_YELLOW; return info; }
    if (roleId == 24 || roleId == 39) { info.name = (roleId == 24) ? "Falcon" : "DND Falcon"; info.color = COLOR_ORANGE; return info; }
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
            case 110: info.name = "Witch Doctor"; break;
            default: info.name = "Killer"; break;
        }
        info.color = COLOR_RED;
        return info;
    }
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
        case 111: info.name = "Cuckoo"; break;
        case 112: info.name = "Swordsman"; break;
        case 113: info.name = "Magpie"; break;
        default: info.name = "Unknown"; break;
    }
    info.color = COLOR_WHITE;
    return info;
}

void ApplyDroneViewDelayed() {
    if (!localPlayerObject || !OverrideOrthographicSize) return;
    if (DroneView) {
        if (g_DroneViewDelay < DRONE_VIEW_DELAY_FRAMES) { g_DroneViewDelay++; return; }
        if (!g_DroneViewInitialized) {
            g_DroneViewInitialized = true;
            g_DroneViewReady = true;
            OverrideOrthographicSize(localPlayerObject, DroneZoom);
        }
        else if (g_DroneViewReady) {
            OverrideOrthographicSize(localPlayerObject, DroneZoom);
        }
    }
}

void ResetDroneViewDelay() {
    g_DroneViewDelay = 0;
    g_DroneViewReady = false;
    g_DroneViewInitialized = false;
    g_ESPStabilized = false;
    g_FrameCount = 0;
}

void DisableDroneView() {
    if (localPlayerObject && OverrideOrthographicSize)
        OverrideOrthographicSize(localPlayerObject, g_DefaultOrthoSize);
    g_DroneViewReady = false;
    g_DroneViewInitialized = false;
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

    g_SetMiniMapVisibleMethod = env->GetStaticMethodID(g_MenuClass, "setMiniMapVisible", "(Z)V");
    g_SetMiniMapIdMethod = env->GetStaticMethodID(g_MenuClass, "setMiniMapId", "(I)V");
    g_UpdateMiniMapBatchMethod = env->GetStaticMethodID(g_MenuClass, "updateMiniMapBatch", "(Ljava/lang/String;)V");
    g_SetMapShowPlayersMethod = env->GetStaticMethodID(g_MenuClass, "setMapShowPlayers", "(Z)V");
    g_SetMapShowDeadBodiesMethod = env->GetStaticMethodID(g_MenuClass, "setMapShowDeadBodies", "(Z)V");
    g_SetMapTouchTeleportMethod = env->GetStaticMethodID(g_MenuClass, "setMapTouchTeleport", "(Z)V");

    g_ESPReady = (g_BatchDrawMethod != NULL);
    env->DeleteLocalRef(cls);
    LOGI("ESP & MiniMap Init: %s", g_ESPReady ? "SUCCESS" : "FAILED");
}

void UpdateScreenSize() {
    JNIEnv* env = GetJNIEnv();
    if (!env || !g_MenuClass || !g_GetScreenWidthMethod) return;
    int w = env->CallStaticIntMethod(g_MenuClass, g_GetScreenWidthMethod);
    int h = env->CallStaticIntMethod(g_MenuClass, g_GetScreenHeightMethod);
    if (w > 100 && h > 100) { g_ScreenWidth = (float)w; g_ScreenHeight = (float)h; }
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

void SendMiniMapBatch(JNIEnv* env) {
    if (!env || !g_MenuClass || !g_SetMiniMapVisibleMethod) return;

    bool isPlaying = isInGame && !isInLobby && (localPlayerInstance != NULL) && !isInSpotlightScreen && (g_CurrentGameState >= 2);

    if (MiniMapHideInVote && isInVotingScreen) {
        isPlaying = false;
    }

    bool shouldShowMap = MiniMapEnabled && isPlaying;

    if (shouldShowMap != s_LastMiniMapState) {
        s_LastMiniMapState = shouldShowMap;
        env->CallStaticVoidMethod(g_MenuClass, g_SetMiniMapVisibleMethod, (jboolean)shouldShowMap);
    }

    if (!shouldShowMap) return;

    if (g_SetMiniMapIdMethod && g_CurrentMapId >= 0) {
        env->CallStaticVoidMethod(g_MenuClass, g_SetMiniMapIdMethod, (jint)g_CurrentMapId);
    }

    if (!g_UpdateMiniMapBatchMethod) return;

    static int mapThrottle = 0;
    if (++mapThrottle < 4) return;
    mapThrottle = 0;

    int offset = 0;
    g_MiniMapBatchBuffer[0] = '\0';

    for (int i = 0; i < g_RenderPlayerCount; i++) {
        PlayerData* p = &g_RenderPlayers[i];
        if (!p->isValid) continue;

        int colorId = GetPlayerColorId(p->instance);

        char safeName[48];
        int j = 0;
        for (int k = 0; p->name[k] && j < 46; k++) {
            if (p->name[k] == ',' || p->name[k] == ';') safeName[j++] = ' ';
            else safeName[j++] = p->name[k];
        }
        safeName[j] = '\0';

        bool isDeadPlayer = p->isGhost || p->isDowned;
        Vector2 targetCoord = p->position;

        // ÖLÜ OYUNCU HİZALAMA:
        // Eğer killedLocation geçerliyse öldüğü odaya sabitler.
        // Eğer henüz yazılmadıysa son bilinen pozisyonu kullanır, asla es geçmez (continue yapılmaz).
        if (isDeadPlayer) {
            if (p->hasKilledLoc) {
                targetCoord = p->killedLocation;
            }
        }

        offset += snprintf(g_MiniMapBatchBuffer + offset, sizeof(g_MiniMapBatchBuffer) - offset,
                           "%.1f,%.1f,%d,%d,%d,%s;",
                           targetCoord.x, targetCoord.y,
                           isDeadPlayer ? 1 : 0,
                           p->isLocal ? 1 : 0,
                           colorId, safeName);
        if (offset >= sizeof(g_MiniMapBatchBuffer) - 100) break;
    }

    jstring jdata = env->NewStringUTF(g_MiniMapBatchBuffer);
    if (jdata) {
        env->CallStaticVoidMethod(g_MenuClass, g_UpdateMiniMapBatchMethod, jdata);
        env->DeleteLocalRef(jdata);
    }
}

inline bool WorldToScreen(Vector2 world, Vector2 camPos, float scale, float* sx, float* sy) {
    float dx = world.x - camPos.x;
    float dy = world.y - camPos.y;
    float cx = g_ScreenWidth * 0.5f;
    float cy = g_ScreenHeight * 0.5f;
    *sx = cx + (dx * scale);
    *sy = cy - (dy * scale);
    return true;
}

bool ClipLine(float* x1, float* y1, float* x2, float* y2) {
    const float xmin = 0.0f, ymin = 0.0f, xmax = g_ScreenWidth, ymax = g_ScreenHeight;
    int outcode1 = 0, outcode2 = 0;
    if (*x1 < xmin) outcode1 |= 1; else if (*x1 > xmax) outcode1 |= 2;
    if (*y1 < ymin) outcode1 |= 8; else if (*y1 > ymax) outcode1 |= 4;
    if (*x2 < xmin) outcode2 |= 1; else if (*x2 > xmax) outcode2 |= 2;
    if (*y2 < ymin) outcode2 |= 8; else if (*y2 > ymax) outcode2 |= 4;

    while (true) {
        if (!(outcode1 | outcode2)) return true;
        if (outcode1 & outcode2) return false;

        float x = 0.0f, y = 0.0f;
        int outcodeOut = outcode1 ? outcode1 : outcode2;

        if (outcodeOut & 4) {
            x = *x1 + (*x2 - *x1) * (ymax - *y1) / (*y2 - *y1);
            y = ymax;
        } else if (outcodeOut & 8) {
            x = *x1 + (*x2 - *x1) * (ymin - *y1) / (*y2 - *y1);
            y = ymin;
        } else if (outcodeOut & 2) {
            y = *y1 + (*y2 - *y1) * (xmax - *x1) / (*x2 - *x1);
            x = xmax;
        } else if (outcodeOut & 1) {
            y = *y1 + (*y2 - *y1) * (xmin - *x1) / (*x2 - *x1);
            x = xmin;
        }

        if (outcodeOut == outcode1) {
            *x1 = x; *y1 = y;
            outcode1 = 0;
            if (*x1 < xmin) outcode1 |= 1; else if (*x1 > xmax) outcode1 |= 2;
            if (*y1 < ymin) outcode1 |= 8; else if (*y1 > ymax) outcode1 |= 4;
        } else {
            *x2 = x; *y2 = y;
            outcode2 = 0;
            if (*x2 < xmin) outcode2 |= 1; else if (*x2 > xmax) outcode2 |= 2;
            if (*y2 < ymin) outcode2 |= 8; else if (*y2 > ymax) outcode2 |= 4;
        }
    }
}

inline void GetEdgePosition(float targetX, float targetY, float playerX, float playerY, float* edgeX, float* edgeY) {
    float padding = 60.0f;
    float minX = padding, maxX = g_ScreenWidth - padding;
    float minY = padding, maxY = g_ScreenHeight - padding;

    float dx = targetX - playerX;
    float dy = targetY - playerY;

    if (dx == 0 && dy == 0) {
        *edgeX = playerX;
        *edgeY = playerY;
        return;
    }

    float t = 1.0f;
    if (dx > 0) { float tX = (maxX - playerX) / dx; if (tX > 0 && tX < t) t = tX; }
    else if (dx < 0) { float tX = (minX - playerX) / dx; if (tX > 0 && tX < t) t = tX; }

    if (dy > 0) { float tY = (maxY - playerY) / dy; if (tY > 0 && tY < t) t = tY; }
    else if (dy < 0) { float tY = (minY - playerY) / dy; if (tY > 0 && tY < t) t = tY; }

    *edgeX = playerX + dx * t;
    *edgeY = playerY + dy * t;

    if (*edgeX < minX) *edgeX = minX; else if (*edgeX > maxX) *edgeX = maxX;
    if (*edgeY < minY) *edgeY = minY; else if (*edgeY > maxY) *edgeY = maxY;
}

void ExecuteRepairSabotage() {
    if (!g_TasksHandler || !TasksHandler_CompleteTask || !il2cpp_string_new_func) return;
    void* taskList = *(void**)((uintptr_t)g_TasksHandler + OFFSET_TH_SORTEDASSIGNEDTASKS);
    if (!taskList) return;
    void* items = *(void**)((uintptr_t)taskList + 0x10);
    int count = *(int*)((uintptr_t)taskList + 0x18);
    if (!items || count <= 0) return;

    for (int i = 0; i < count; i++) {
        void* task = *(void**)((uintptr_t)items + 0x20 + (i * 8));
        if (!task) continue;
        bool isSabotage = *(bool*)((uintptr_t)task + OFFSET_GT_ISSABOTAGE);
        bool isFake = *(bool*)((uintptr_t)task + OFFSET_GT_ISFAKETASK);
        if (isSabotage && !isFake) {
            char taskId[64]; GetTaskId(task, taskId, sizeof(taskId));
            if (taskId[0] == '\0') continue;
            void* taskIdStr = il2cpp_string_new_func(taskId);
            if (taskIdStr) {
                TasksHandler_CompleteTask(g_TasksHandler, taskIdStr, false, false, false, false);
                LOGI("Sabotage repaired: %s", taskId);
            }
        }
    }
    if (TasksHandler_UpdateTaskVisuals) TasksHandler_UpdateTaskVisuals(g_TasksHandler);
}

void ExecuteCompleteSingleTask() {
    if (!g_TasksHandler || !TasksHandler_CompleteTask || !il2cpp_string_new_func) return;
    void* taskList = *(void**)((uintptr_t)g_TasksHandler + OFFSET_TH_SORTEDASSIGNEDTASKS);
    if (!taskList) return;
    void* items = *(void**)((uintptr_t)taskList + 0x10);
    int count = *(int*)((uintptr_t)taskList + 0x18);
    if (!items || count <= 0) return;

    for (int i = 0; i < count; i++) {
        void* task = *(void**)((uintptr_t)items + 0x20 + (i * 8));
        if (!task) continue;
        bool isSabotage = *(bool*)((uintptr_t)task + OFFSET_GT_ISSABOTAGE);
        bool isFake = *(bool*)((uintptr_t)task + OFFSET_GT_ISFAKETASK);
        if (!isSabotage && !isFake) {
            char taskId[64]; GetTaskId(task, taskId, sizeof(taskId));
            if (taskId[0] == '\0') continue;
            void* taskIdStr = il2cpp_string_new_func(taskId);
            if (taskIdStr) {
                TasksHandler_CompleteTask(g_TasksHandler, taskIdStr, false, false, false, false);
                LOGI("Single task completed: %s", taskId);
                if (TasksHandler_UpdateTaskVisuals) TasksHandler_UpdateTaskVisuals(g_TasksHandler);
                break;
            }
        }
    }
}

// 1.2s gecikmeli ve thread-safe sabotaj açma
void ExecuteUnlockSabotages() {
    if (!g_TasksHandler || !TasksHandler_CompleteTask || !il2cpp_string_new_func) return;

    void* taskList = *(void**)((uintptr_t)g_TasksHandler + OFFSET_TH_SORTEDASSIGNEDTASKS);
    if (!taskList) return;
    void* items = *(void**)((uintptr_t)taskList + 0x10);
    int count = *(int*)((uintptr_t)taskList + 0x18);
    if (!items || count <= 0) return;

    std::vector<std::string> impostorTaskIds;
    for (int i = 0; i < count; i++) {
        void* task = *(void**)((uintptr_t)items + 0x20 + (i * 8));
        if (!task) continue;

        bool isImpostor = *(bool*)((uintptr_t)task + OFFSET_GT_ISIMPOSTORTASK);
        bool isFake = *(bool*)((uintptr_t)task + OFFSET_GT_ISFAKETASK);

        // Yalnızca sabotaj kilidini açan görevler
        if (isImpostor && !isFake) {
            char taskId[64];
            GetTaskId(task, taskId, sizeof(taskId));
            if (taskId[0] != '\0') {
                impostorTaskIds.push_back(std::string(taskId));
            }
        }
    }

    if (impostorTaskIds.empty()) return;

    std::thread([impostorTaskIds]() {
        for (const auto& taskId : impostorTaskIds) {
            if (!isInGame || isInLobby || !g_TasksHandler) break;

            void* taskIdStr = il2cpp_string_new_func(taskId.c_str());
            if (taskIdStr) {
                TasksHandler_CompleteTask(g_TasksHandler, taskIdStr, false, false, false, false);
                LOGI("Impostor sabotage task completed (with 1.2s delay): %s", taskId.c_str());
            }

            if (TasksHandler_UpdateTaskVisuals && g_TasksHandler) {
                TasksHandler_UpdateTaskVisuals(g_TasksHandler);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        }
    }).detach();
}

void HandleTasksAndSabotageLogic() {
    if (!isInGame || isInLobby || !g_TasksHandler) return;
    auto now = std::chrono::steady_clock::now();

    if (btnRepairSabotageNow) {
        ExecuteRepairSabotage();
        btnRepairSabotageNow = false;
    }
    if (btnCompleteOneTask) {
        ExecuteCompleteSingleTask();
        btnCompleteOneTask = false;
    }
    if (btnUnlockSabotages) {
        ExecuteUnlockSabotages();
        btnUnlockSabotages = false;
    }

    if (AutoRepairSabotage) {
        void* taskList = *(void**)((uintptr_t)g_TasksHandler + OFFSET_TH_SORTEDASSIGNEDTASKS);
        if (taskList) {
            void* items = *(void**)((uintptr_t)taskList + 0x10);
            int count = *(int*)((uintptr_t)taskList + 0x18);
            bool hasSabotage = false;
            for (int i = 0; i < count && items; i++) {
                void* task = *(void**)((uintptr_t)items + 0x20 + (i * 8));
                if (task && *(bool*)((uintptr_t)task + OFFSET_GT_ISSABOTAGE)) {
                    hasSabotage = true;
                    break;
                }
            }

            if (hasSabotage) {
                if (!g_SabotagePending) {
                    g_SabotagePending = true;
                    g_SabotageDetectedTime = now;
                } else {
                    auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_SabotageDetectedTime).count();
                    if (waitTime >= 1800) {
                        ExecuteRepairSabotage();
                        g_SabotagePending = false;
                    }
                }
            } else {
                g_SabotagePending = false;
            }
        }
    }

    if (SafeAutoTasks) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_LastSafeTaskTime).count();
        if (elapsed >= 5500) {
            ExecuteCompleteSingleTask();
            g_LastSafeTaskTime = now;
        }
    }
}

void HandleAutoReady() {
    if (!AutoReady || !isInLobby || !g_PlayerPropertiesManager || !PlayerPropertiesManager_ChangeReadyState) return;

    if (localPlayerInstance) {
        int readyState = *(int*)((uintptr_t)localPlayerInstance + OFFSET_PC_READYSTATE);
        if (readyState == 1) return;
    } else if (PlayerPropertiesManager_GetUserProperties) {
        void* userProps = PlayerPropertiesManager_GetUserProperties(g_PlayerPropertiesManager);
        if (userProps) {
            int readyState = *(int*)((uintptr_t)userProps + OFFSET_PP_READYSTATE);
            if (readyState == 1) return;
        }
    }

    static auto lastReadyCheck = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastReadyCheck).count() >= 1000) {
        PlayerPropertiesManager_ChangeReadyState(g_PlayerPropertiesManager, 1);
        lastReadyCheck = now;
    }
}

void ApplyNoClip(void* instance, bool enable) {
    if (!instance) return;

    void* playerCollider = *(void**)((uintptr_t)instance + OFFSET_PE_PLAYERCOLLIDER);
    void* confineCollider = *(void**)((uintptr_t)instance + OFFSET_PE_CONFINECOLLIDER);
    void* wallCollisionHandler = *(void**)((uintptr_t)instance + OFFSET_PE_WALLCOLLISIONHANDLER);

    if (Collider2D_set_isTrigger) {
        if (playerCollider) Collider2D_set_isTrigger(playerCollider, enable);
        if (confineCollider) Collider2D_set_isTrigger(confineCollider, enable);
    }

    if (wallCollisionHandler) {
        *(bool*)((uintptr_t)wallCollisionHandler + OFFSET_WCCH_INWALL) = false;
    }
}

void CollectPlayerDataFromInstance(void* instance, PlayerData* p, Vector2 camPos) {
    if (!instance || !p) return;
    p->instance = instance;
    bool isLocal = *(bool*)((uintptr_t)instance + OFFSET_PE_ISLOCAL);
    p->position = GetPlayerPosition(instance, isLocal);

    p->isGhost = *(bool*)((uintptr_t)instance + OFFSET_PE_ISGHOST);
    p->isDowned = *(bool*)((uintptr_t)instance + OFFSET_PE_ISDOWNED);

    p->hasKilledLoc = false;
    if (p->isGhost || p->isDowned) {
        Vector2 kLoc = *(Vector2*)((uintptr_t)instance + OFFSET_PE_KILLEDLOCATION);
        if (kLoc.x != 0.0f || kLoc.y != 0.0f) {
            p->killedLocation = kLoc;
            p->hasKilledLoc = true;
        }
    }

    p->isLocal = isLocal;
    p->role = GetRoleType(instance);
    p->teamId = *(int*)((uintptr_t)instance + OFFSET_PE_TEAMID);
    p->entityNumber = *(int*)((uintptr_t)instance + OFFSET_PE_ENTITYNUMBER);

    void* cosm = *(void**)((uintptr_t)instance + OFFSET_PE_COSMETICS);
    p->cosmColorVal = cosm ? *(int*)((uintptr_t)cosm + 0x10) : -999;

    GetPlayerNickname(instance, p->name, sizeof(p->name));
    p->inVent = *(bool*)((uintptr_t)instance + OFFSET_PE_INVENT);
    p->isInvisible = *(bool*)((uintptr_t)instance + OFFSET_PE_ISINVISIBLE);
    p->isInPelican = *(bool*)((uintptr_t)instance + OFFSET_PE_ISINPELICAN);
    p->isSpectator = *(bool*)((uintptr_t)instance + OFFSET_PE_ISSPECTATOR);
    p->isMorphed = *(bool*)((uintptr_t)instance + OFFSET_PE_ISMORPHED);
    p->isRunning = *(bool*)((uintptr_t)instance + OFFSET_PE_ISRUNNING);
    p->hasKilledThisRound = *(bool*)((uintptr_t)instance + OFFSET_PE_HASKILLED);
    p->opacity = *(float*)((uintptr_t)instance + OFFSET_PE_TARGETOPACITY);
    p->isInfected = *(bool*)((uintptr_t)instance + OFFSET_PE_ISINFECTED);
    p->hasBomb = *(bool*)((uintptr_t)instance + OFFSET_PE_HASBOMB);
    p->tasksRemaining = *(int*)((uintptr_t)instance + OFFSET_PE_TASKSREMAINING);
    GetKilledBy(instance, p->killedBy, sizeof(p->killedBy));
    p->confirmedKiller = false;
    float dx = p->position.x - camPos.x;
    float dy = p->position.y - camPos.y;
    p->distanceToCamera = sqrtf(dx*dx + dy*dy);
    p->isValid = true;
}

void ProcessKillers() {
    for (int i = 0; i < g_RenderPlayerCount; i++) {
        PlayerData* dead = &g_RenderPlayers[i];
        if (!dead->isValid || dead->killedBy[0] == '\0') continue;
        for (int j = 0; j < g_RenderPlayerCount; j++) {
            PlayerData* killer = &g_RenderPlayers[j];
            if (!killer->isValid) continue;

            char entityStr[16];
            snprintf(entityStr, sizeof(entityStr), "%d", killer->entityNumber);

            if (strcmp(dead->killedBy, killer->name) == 0 || strcmp(dead->killedBy, entityStr) == 0) {
                killer->confirmedKiller = true;
            }
        }
    }
}

void RenderDebugPanelBatch() {
    if (!DebugMode) return;
    float centerX = g_ScreenWidth * 0.5f;
    float startY = 100;
    float lineHeight = 28;
    float maxY = g_ScreenHeight - 80;
    char buf[256];

    BatchAddText(centerX, startY, "=== DEBUG INFO ===", COLOR_MAGENTA);
    startY += lineHeight + 5;

    snprintf(buf, sizeof(buf), "Screen:%.0fx%.0f | Ortho:%.1f | Scale:%.0f", g_ScreenWidth, g_ScreenHeight, GetCurrentOrthoSize(), GetESPScale());
    BatchAddText(centerX, startY, buf, COLOR_CYAN); startY += lineHeight;

    snprintf(buf, sizeof(buf), "PlayerPos: X=%.2f Y=%.2f", g_LocalPlayerPos.x, g_LocalPlayerPos.y);
    BatchAddText(centerX, startY, buf, COLOR_CYAN); startY += lineHeight;

    snprintf(buf, sizeof(buf), "CamPos: X=%.2f Y=%.2f Z=%.2f [%s]", g_CameraPosition.x, g_CameraPosition.y, g_CameraPosition.z, g_CameraPositionValid ? "VALID" : "INVALID");
    BatchAddText(centerX, startY, buf, g_CameraPositionValid ? COLOR_LIME : COLOR_RED); startY += lineHeight;

    Vector2 excitingPos = GetCameraPosition2D();
    snprintf(buf, sizeof(buf), "ESP Using: X=%.2f Y=%.2f", excitingPos.x, excitingPos.y);
    BatchAddText(centerX, startY, buf, COLOR_GOLD); startY += lineHeight;

    snprintf(buf, sizeof(buf), "Drone:%c | Ready:%c | Init:%c | Delay:%d/%d | Zoom:%.1f", DroneView ? 'Y' : 'N', g_DroneViewReady ? 'Y' : 'N', g_DroneViewInitialized ? 'Y' : 'N', g_DroneViewDelay, DRONE_VIEW_DELAY_FRAMES, DroneZoom);
    BatchAddText(centerX, startY, buf, COLOR_CYAN); startY += lineHeight;

    snprintf(buf, sizeof(buf), "GM:%c | State:%s(%d) | InGame:%c | Lobby:%c | Vote:%c | Players:%d | Dead:%d", g_GameManager ? 'Y' : 'N', GetGameStateName(g_CurrentGameState), g_CurrentGameState, isInGame ? 'Y' : 'N', isInLobby ? 'Y' : 'N', isInVotingScreen ? 'Y' : 'N', g_RenderPlayerCount, g_DeadPlayersCount);
    BatchAddText(centerX, startY, buf, COLOR_CYAN); startY += lineHeight;

    snprintf(buf, sizeof(buf), "MapManager:%c | MapID:%d | RoomMap:%s", g_MapManager ? 'Y' : 'N', g_CurrentMapId, g_CurrentMapName);
    BatchAddText(centerX, startY, buf, COLOR_GOLD); startY += lineHeight;

    snprintf(buf, sizeof(buf), "NoClip:%c | AutoReady:%c", NoClip ? 'Y' : 'N', AutoReady ? 'Y' : 'N');
    BatchAddText(centerX, startY, buf, COLOR_CYAN); startY += lineHeight;

    RoleInfo myRole = GetRoleInfo(localPlayerRole);
    snprintf(buf, sizeof(buf), "[ME] %s (ID:%d) | Tasks:%d | Infected:%c | Bomb:%c", myRole.name, localPlayerRole, g_LocalTasksRemaining, g_LocalIsInfected ? 'Y' : 'N', g_LocalHasBomb ? 'Y' : 'N');
    BatchAddText(centerX, startY, buf, myRole.color); startY += lineHeight;

    if (g_LocalKilledBy[0] != '\0') {
        snprintf(buf, sizeof(buf), "Killed By: %s", g_LocalKilledBy);
        BatchAddText(centerX, startY, buf, COLOR_RED); startY += lineHeight;
    }

    snprintf(buf, sizeof(buf), "TasksHandler:%c | RoofHandler:%c | SafeTasks:%c | RepairSabotage:%c", g_TasksHandler ? 'Y' : 'N', g_RoofHandler ? 'Y' : 'N', SafeAutoTasks ? 'Y' : 'N', AutoRepairSabotage ? 'Y' : 'N');
    BatchAddText(centerX, startY, buf, COLOR_ORANGE); startY += lineHeight;

    startY += 8;
    BatchAddText(centerX, startY, "--- PLAYER LIST ---", COLOR_YELLOW); startY += lineHeight;

    for (int i = 0; i < g_RenderPlayerCount && startY < maxY; i++) {
        PlayerData* p = &g_RenderPlayers[i];
        if (!p->isValid) continue;
        RoleInfo ri = GetRoleInfo(p->role);
        char flags[20] = ""; int fi = 0;
        if (p->isLocal) flags[fi++] = 'L'; if (p->isGhost) flags[fi++] = 'G';
        if (p->isDowned) flags[fi++] = 'D'; if (p->inVent) flags[fi++] = 'V';
        if (p->isInvisible) flags[fi++] = 'I'; if (p->isInPelican) flags[fi++] = 'P';
        if (p->isSpectator) flags[fi++] = 'S'; if (p->isMorphed) flags[fi++] = 'M';
        if (p->isRunning) flags[fi++] = 'R'; if (p->hasKilledThisRound || p->confirmedKiller) flags[fi++] = 'K';
        if (p->isInfected) flags[fi++] = '*'; if (p->hasBomb) flags[fi++] = 'B';
        flags[fi] = '\0'; if (fi == 0) strcpy(flags, "-");

        snprintf(buf, sizeof(buf), "#%02d %-10s %s(%d) T:%d Ent:%d Csm:%d [%s]", p->entityNumber, p->name, ri.name, p->role, p->teamId, p->entityNumber, p->cosmColorVal, flags);
        int color = p->isLocal ? COLOR_CYAN : (p->isGhost ? COLOR_GRAY : ri.color);
        BatchAddText(centerX, startY, buf, color); startY += lineHeight;
        if (p->killedBy[0] != '\0') {
            snprintf(buf, sizeof(buf), "   -> Killed by: %s", p->killedBy);
            BatchAddText(centerX, startY, buf, COLOR_GRAY); startY += lineHeight;
        }
    }

    if (startY < maxY - lineHeight) {
        startY += 10;
        BatchAddText(centerX, startY, "Flags: L=Local G=Ghost D=Down V=Vent I=Invis P=Pelican S=Spec M=Morph R=Run K=Kill *=Infected B=Bomb", COLOR_GRAY);
    }
}

void RenderESPBatch() {
    if (!ESPEnabled) return;
    if (ESPHideInVote && isInVotingScreen) return;
    if (ESPHideInLobby && isInLobby) return;

    if (!g_ESPStabilized) { g_FrameCount++; if (g_FrameCount < STABILIZE_FRAMES) return; g_ESPStabilized = true; }

    float scale = GetESPScale();
    Vector2 camPos = GetCameraPosition2D();
    Vector2 localPos = g_LocalPlayerPos;

    float playerSx, playerSy;
    WorldToScreen(localPos, camPos, scale, &playerSx, &playerSy);

    for (int i = 0; i < g_RenderPlayerCount; i++) {
        PlayerData* p = &g_RenderPlayers[i];
        if (!p->isValid || p->isLocal) continue;
        if (p->isGhost && !SeeGhosts) continue;
        if (p->isInPelican) continue;
        if (p->position.x == 0 && p->position.y == 0) continue;

        float sx, sy;
        WorldToScreen(p->position, camPos, scale, &sx, &sy);
        RoleInfo ri = GetRoleInfo(p->role);
        int color = p->isGhost ? COLOR_GRAY : ri.color;
        float dist = p->distanceToCamera;
        bool onScreen = (sx >= 0 && sx <= g_ScreenWidth && sy >= 0 && sy <= g_ScreenHeight);

        if (ESPLines) {
            float lx1 = playerSx, ly1 = playerSy, lx2 = sx, ly2 = sy;
            if (ClipLine(&lx1, &ly1, &lx2, &ly2)) {
                BatchAddLine(lx1, ly1, lx2, ly2, color);
            }
        }

        bool hasKill = p->hasKilledThisRound || p->confirmedKiller;

        if (onScreen) {
            float boxW = 0.9f * scale, boxH = 1.6f * scale;
            if (boxW < 45) boxW = 45; if (boxH < 70) boxH = 70;
            if (boxW > 200) boxW = 200; if (boxH > 320) boxH = 320;
            float boxX = sx - boxW * 0.5f, boxY = sy - boxH * 0.40f;
            if (ESPBox) BatchAddBox(boxX, boxY, boxW, boxH, color);
            if (ESPDistance) { char dt[16]; snprintf(dt, sizeof(dt), "%.1fm", dist); BatchAddText(sx, boxY - 25, dt, COLOR_YELLOW); }
            if (ESPName) {
                float nameY = boxY - 55;
                int iconCount = 0;
                if (p->isInfected) iconCount++;
                if (p->hasBomb) iconCount++;
                if (hasKill) iconCount++;

                float iconX = sx - 90 - (iconCount * 20);
                if (hasKill)       { BatchAddIcon(iconX, nameY + 5, "\uE636", COLOR_RED); iconX += 40; }
                if (p->isInfected) { BatchAddIcon(iconX, nameY + 5, "\uE9E0", COLOR_LIME); iconX += 40; }
                if (p->hasBomb)    { BatchAddIcon(iconX, nameY + 5, "\uEE0A", COLOR_ORANGE); }
                BatchAddText(sx, nameY, p->name, color);
            }
        } else if (ESPEdgeIndicator) {
            float edgeX, edgeY;
            GetEdgePosition(sx, sy, playerSx, playerSy, &edgeX, &edgeY);
            BatchAddBox(edgeX - 8, edgeY - 8, 16, 16, color);
            int iconCount = 0;
            if (p->isInfected) iconCount++;
            if (p->hasBomb) iconCount++;
            if (hasKill) iconCount++;

            float iconX = edgeX - 70 - (iconCount * 20), iconY = edgeY - 25;
            if (hasKill)       { BatchAddIcon(iconX, iconY, "\uE636", COLOR_RED); iconX += 40; }
            if (p->isInfected) { BatchAddIcon(iconX, iconY, "\uE9E0", COLOR_LIME); iconX += 40; }
            if (p->hasBomb)    { BatchAddIcon(iconX, iconY, "\uEE0A", COLOR_ORANGE); }
            char et[64]; snprintf(et, sizeof(et), "%s %.0fm", p->name, dist);
            float tx = edgeX, ty = edgeY - 25;
            if (edgeX < 150) tx = 150; else if (edgeX > g_ScreenWidth - 150) tx = g_ScreenWidth - 150;
            if (edgeY < 80) ty = edgeY + 35;
            BatchAddText(tx, ty, et, color);
        }
    }
}

void ClearAllESP() {
    g_PlayerInstanceCount = 0; g_RenderPlayerCount = 0; g_ESPStabilized = false; g_FrameCount = 0;
    isInGame = false; isInLobby = true; localPlayerInstance = NULL; localPlayerObject = NULL;
    g_DeadPlayersCount = 0; g_LocalTasksRemaining = 0; g_LocalKilledBy[0] = '\0';
    g_LocalIsInfected = false; g_LocalHasBomb = false; g_RoofRemovedThisRound = false;
    g_CameraPosition = {0, 0, 0}; g_CinemachineCamera = nullptr; g_CameraPositionValid = false;
    s_LastMiniMapState = false;
    g_CurrentMapId = -1;
    strcpy(g_CurrentMapName, "None");
    ResetDroneViewDelay();

    JNIEnv* env = GetJNIEnv();
    if (env && g_MenuClass) {
        if (g_BatchDrawMethod) {
            jstring empty = env->NewStringUTF("");
            if (empty) { env->CallStaticVoidMethod(g_MenuClass, g_BatchDrawMethod, empty); env->DeleteLocalRef(empty); }
        }
        if (g_SetMiniMapVisibleMethod) {
            env->CallStaticVoidMethod(g_MenuClass, g_SetMiniMapVisibleMethod, (jboolean)false);
        }
    }
}

void RefreshPlayerDataAndRender() {
    if (!g_ESPReady) return;
    if (isInSpotlightScreen) { BatchClear(); JNIEnv* env = GetJNIEnv(); if (env) SendBatchESP(env); return; }

    Vector2 camPos = GetCameraPosition2D();
    g_LocalPlayerPos = localPlayerInstance ? GetPlayerPosition(localPlayerInstance, true) : g_LocalPlayerPos;

    DetectCurrentMap();

    g_RenderPlayerCount = 0;
    for (int i = 0; i < g_PlayerInstanceCount && g_RenderPlayerCount < MAX_PLAYERS; i++) {
        if (g_PlayerInstances[i].isValid && g_PlayerInstances[i].instance) {
            CollectPlayerDataFromInstance(g_PlayerInstances[i].instance, &g_RenderPlayers[g_RenderPlayerCount], camPos);
            g_RenderPlayerCount++;
        }
    }

    ProcessKillers();

    static int screenUpdateCounter = 0;
    if (++screenUpdateCounter >= 60) { UpdateScreenSize(); screenUpdateCounter = 0; }

    JNIEnv* env = GetJNIEnv();
    if (env) {
        if (ESPEnabled || DebugMode) {
            BatchClear();
            if (DebugMode) RenderDebugPanelBatch();
            if (ESPEnabled) RenderESPBatch();
            SendBatchESP(env);
        }
        SendMiniMapBatch(env);
    }
}

int (*old_get_deadPlayersCount)() = NULL;

// CinemachineStateDrivenCamera.InternalUpdateCameraState - RVA: 0x4437958
void (*old_StateCameraUpdate)(void* instance, Vector3 worldUp, float deltaTime);
void StateCameraUpdate(void* instance, Vector3 worldUp, float deltaTime) {
    old_StateCameraUpdate(instance, worldUp, deltaTime);
    if (instance) {
        g_CinemachineCamera = instance;
        g_CameraPosition = *(Vector3*)((uintptr_t)instance + OFFSET_CSDC_STATE + OFFSET_CS_RAWPOSITION);
        g_CameraPositionValid = true;
        RefreshPlayerDataAndRender();
    }
}

// PlayableEntity.Update - RVA: 0x3E4FC30
void (*old_Update)(void *instance);
void Update(void *instance) {
    if (instance) {
        bool isLocal = *(bool*)((uintptr_t)instance + OFFSET_PE_ISLOCAL);
        if (isLocal) {
            localPlayerInstance = instance;
            localPlayerRole = GetRoleType(instance);

            if (!g_GameManager) {
                isInLobby = !*(bool*)((uintptr_t)instance + OFFSET_PE_ISPLAYERROLESET);
                isInGame = !isInLobby;
            }

            g_PlayerInstanceCount = 0;
            g_LocalPlayerPos = GetPlayerPosition(instance, true);
            g_LocalTasksRemaining = *(int*)((uintptr_t)instance + OFFSET_PE_TASKSREMAINING);
            g_LocalIsInfected = *(bool*)((uintptr_t)instance + OFFSET_PE_ISINFECTED);
            g_LocalHasBomb = *(bool*)((uintptr_t)instance + OFFSET_PE_HASBOMB);
            GetKilledBy(instance, g_LocalKilledBy, sizeof(g_LocalKilledBy));
            if (old_get_deadPlayersCount) g_DeadPlayersCount = old_get_deadPlayersCount();
            if (UnlimitedVision) *(bool*)((uintptr_t)instance + OFFSET_PE_FOGOFWAR) = false;
            if (NoClip) ApplyNoClip(instance, true);
            if (btnCallEmergency && PlayerController_CallEmergency) { PlayerController_CallEmergency(instance); btnCallEmergency = false; LOGI("Emergency called"); }

            // Güvenli Teleport (Main Loop)
            if (g_PendingDirectTP.load() && TeleportTo) {
                Vector2 targetPos = {g_TargetDirectTPX.load(), g_TargetDirectTPY.load()};
                TeleportTo(instance, targetPos, true);
                g_PendingDirectTP.store(false);
            }

            HandleTasksAndSabotageLogic();
            HandleAutoReady();
        }
        if ((ESPEnabled || DebugMode || MiniMapEnabled) && g_PlayerInstanceCount < MAX_PLAYERS) {
            g_PlayerInstances[g_PlayerInstanceCount].instance = instance;
            g_PlayerInstances[g_PlayerInstanceCount].isLocal = isLocal;
            g_PlayerInstances[g_PlayerInstanceCount].isValid = true;
            g_PlayerInstanceCount++;
        }
    }
    old_Update(instance);
}

// GameManager.Update - RVA: 0x3AD9E54
void (*old_GameManager_Update)(void* instance);
void GameManager_Update(void* instance) {
    if (instance) {
        g_GameManager = instance;
        g_CurrentGameState = *(int*)((uintptr_t)instance + OFFSET_GM_GAMESTATE);

        // 0 = InLobby, 1 = Drafting
        isInLobby = (g_CurrentGameState <= 1);
        isInGame = (g_CurrentGameState >= 2);

        // Lobiye dönüldüğünde haritayı kesin olarak kapat
        if (isInLobby && s_LastMiniMapState) {
            s_LastMiniMapState = false;
            JNIEnv* env = GetJNIEnv();
            if (env && g_MenuClass && g_SetMiniMapVisibleMethod) {
                env->CallStaticVoidMethod(g_MenuClass, g_SetMiniMapVisibleMethod, (jboolean)false);
            }
        }

        // 3 = Opening, 4 = Discussion, 5 = Voting, 6 = Waiting, 7 = Proceeding
        isInVotingScreen = (g_CurrentGameState >= 3 && g_CurrentGameState <= 7);
    }
    old_GameManager_Update(instance);
}

// PlayableEntity.LateUpdate - RVA: 0x3E509E8
void (*old_LateUpdate)(void *instance);
void LateUpdate(void *instance) { old_LateUpdate(instance); }

// PlayableEntity.TurnIntoGhost - RVA: 0x3E5B99C
void (*old_TurnIntoGhost)(void *instance, int deathReason);
void TurnIntoGhost(void *instance, int deathReason) {
    if (AntiDeath && instance == localPlayerInstance) return;
    old_TurnIntoGhost(instance, deathReason);
}

// LocalPlayer.Update - RVA: 0x3E3536C
void (*old_LocalPlayer_Update)(void *instance);
void LocalPlayer_Update(void *instance) {
    old_LocalPlayer_Update(instance);
    if (instance) {
        localPlayerObject = instance;
        void* cam = *(void**)((uintptr_t)instance + OFFSET_LP_MAINCAMERA);
        if (cam) mainCameraObject = cam;
        if (!g_GameManager) {
            bool inVote1 = *(bool*)((uintptr_t)instance + OFFSET_LP_INVOTINGSCREEN);
            bool inVote2 = *(bool*)((uintptr_t)instance + OFFSET_LP_INVOTINGTRANSITION);
            isInVotingScreen = inVote1 || inVote2;
        }
        bool startSpotlight = *(bool*)((uintptr_t)instance + OFFSET_LP_INGAMESTARTSPOTLIGHT);
        bool endSpotlight = *(bool*)((uintptr_t)instance + OFFSET_LP_INGAMEENDSPOTLIGHT);
        isInSpotlightScreen = startSpotlight || endSpotlight;
        if (SeeGhosts && SetCanSeeGhosts) SetCanSeeGhosts(instance, true);
        if (DroneView && OverrideOrthographicSize) ApplyDroneViewDelayed();
    }
}

// LocalPlayer.GetPlayerSpeed - RVA: 0x3E44BE8
float (*old_GetPlayerSpeed)(void *instance);
float GetPlayerSpeed(void *instance) {
    float speed = old_GetPlayerSpeed(instance);
    return SpeedHack ? speed * SpeedMultiplier : speed;
}

// PlayerPropertiesManager.Initialize - RVA: 0x3AC4878
void (*old_PlayerPropertiesManager_Initialize)(void* instance) = NULL;
void hook_PlayerPropertiesManager_Initialize(void* instance) {
    if (instance) g_PlayerPropertiesManager = instance;
    old_PlayerPropertiesManager_Initialize(instance);
}

// MapManager.Internal_OnMapStart - RVA: 0x3831E54
void (*old_Internal_OnMapStart)(void* instance) = NULL;
void hook_Internal_OnMapStart(void* instance) {
    if (instance) {
        g_MapManager = instance;
        DetectCurrentMap();
    }
    old_Internal_OnMapStart(instance);
}

// MapManager.Internal_OnMapLoad - RVA: 0x3831058
void (*old_Internal_OnMapLoad)(void* instance) = NULL;
void hook_Internal_OnMapLoad(void* instance) {
    if (instance) {
        g_MapManager = instance;
        DetectCurrentMap();
    }
    old_Internal_OnMapLoad(instance);
}

// GGDRole.OnEnterVent - RVA: 0x3CD0438
void (*old_OnEnterVent)(void *instance, void* vent, bool setCooldown);
void OnEnterVent(void *instance, void* vent, bool setCooldown) { old_OnEnterVent(instance, vent, NoCooldown ? false : setCooldown); }

// GGDRole.OnExitVent - RVA: 0x3CD052C
void (*old_OnExitVent)(void *instance, void* vent, bool setCooldown);
void OnExitVent(void *instance, void* vent, bool setCooldown) { old_OnExitVent(instance, vent, NoCooldown ? false : setCooldown); }

// GGDRole.SetVentCooldown - RVA: 0x3CCF058
void (*old_SetVentCooldown)(void *instance, int startCooldown);
void SetVentCooldown(void *instance, int startCooldown) { old_SetVentCooldown(instance, NoCooldown ? 0 : startCooldown); }

// PlayableEntity.Despawn - RVA: 0x3E52680
void (*old_Despawn)(void *instance);
void Despawn(void *instance) {
    if (instance) {
        bool isLocal = *(bool*)((uintptr_t)instance + OFFSET_PE_ISLOCAL);
        if (isLocal) {
            if (NoClip) ApplyNoClip(instance, false);
            ClearAllESP();
        }
    }
    old_Despawn(instance);
}

// TasksHandler.OnEnable - RVA: 0x3E2A1B0
void (*old_TasksHandler_OnEnable)(void* instance);
void TasksHandler_OnEnable(void* instance) { if (instance) g_TasksHandler = instance; old_TasksHandler_OnEnable(instance); }

// TasksHandler.OnDisable - RVA: 0x3E2A2B8
void (*old_TasksHandler_OnDisable)(void* instance);
void TasksHandler_OnDisable(void* instance) { if (instance == g_TasksHandler) g_TasksHandler = NULL; old_TasksHandler_OnDisable(instance); }

// RoofHandler.Awake - RVA: 0x3DBEE78
void (*old_RoofHandler_Awake)(void* instance);
void RoofHandler_Awake(void* instance) { if (instance) g_RoofHandler = instance; old_RoofHandler_Awake(instance); }

// RoofHandler.OnDestroy - RVA: 0x3DBEF7C
void (*old_RoofHandler_OnDestroy)(void* instance);
void RoofHandler_OnDestroy(void* instance) { if (instance == g_RoofHandler) { g_RoofHandler = NULL; g_RoofRemovedThisRound = false; } old_RoofHandler_OnDestroy(instance); }

// LocalPlayer.StartRound - RVA: 0x3E3CEB4
void (*old_LocalPlayer_StartRound)(void* instance, bool isFirstRound);
void LocalPlayer_StartRound(void* instance, bool isFirstRound) {
    g_RoofRemovedThisRound = false;
    if (RemoveRoof && g_RoofHandler && RoofHandler_DeactivateRoofs) { RoofHandler_DeactivateRoofs(g_RoofHandler, true); g_RoofRemovedThisRound = true; }

    // Yeni tur başladı: Eski turun tüm ceset konumlarını sıfırla
    for (int i = 0; i < MAX_PLAYERS; i++) {
        g_RenderPlayers[i].hasKilledLoc = false;
        g_RenderPlayers[i].killedLocation = {0.0f, 0.0f};
    }

    old_LocalPlayer_StartRound(instance, isFirstRound);
}

// WallCollisionCheckHandler.OnCollisionEnter2D - RVA: 0x37E4400
void (*old_WallCollisionCheckHandler_OnCollisionEnter2D)(void* instance, void* collision);
void WallCollisionCheckHandler_OnCollisionEnter2D(void* instance, void* collision) {
    if (NoClip) {
        if (instance) *(bool*)((uintptr_t)instance + OFFSET_WCCH_INWALL) = false;
        return;
    }
    old_WallCollisionCheckHandler_OnCollisionEnter2D(instance, collision);
}

// VoiceChatHandler.CanHearPlayer - RVA: 0x3844A90
bool (*old_CanHearPlayer)(void* instance, void* targetController, void* otherPlayer, bool global);
bool hook_CanHearPlayer(void* instance, void* targetController, void* otherPlayer, bool global) {
    if (HearDeadVoice) return true;
    return old_CanHearPlayer(instance, targetController, otherPlayer, global);
}

// VoiceChatHandler.CanHearPlayerFromMeeting - RVA: 0x3844D80
bool (*old_CanHearPlayerFromMeeting)(void* instance, void* otherPlayer);
bool hook_CanHearPlayerFromMeeting(void* instance, void* otherPlayer) {
    if (HearFarPlayers) return true;
    return old_CanHearPlayerFromMeeting(instance, otherPlayer);
}

jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    InitESP(env);
    const char *features[] = {
            OBFUSCATE("Category_[\uE220]Vision & Cooldown"),
            OBFUSCATE("Toggle_[\uE220]Unlimited Vision"),
            OBFUSCATE("Toggle_[\uE2C4]Remove Roof"),
            OBFUSCATE("Toggle_[\uE492]No Vent Cooldown"),
            OBFUSCATE("Toggle_[\uE62A]See Ghosts"),

            OBFUSCATE("Category_[\uE730]Movement"),
            OBFUSCATE("Toggle_[\uE73A]No Clip"),
            OBFUSCATE("Toggle_[\uED74]Drone View"),
            OBFUSCATE("SeekBar_[\uE434]Zoom Level_5_40"),

            OBFUSCATE("Category_[\uEBB4]ESP Settings"),
            OBFUSCATE("Toggle_[\uEBB4]ESP Enabled"),
            OBFUSCATE("Toggle_True_[\uE6D2]ESP Lines"),
            OBFUSCATE("Toggle_True_[\uE6CE]ESP Box"),
            OBFUSCATE("Toggle_True_[\uE316]ESP Distance"),
            OBFUSCATE("Toggle_True_[\uE6F6]ESP Names"),
            OBFUSCATE("Toggle_True_[\uE0A2]Edge Indicator"),
            OBFUSCATE("Toggle_True_[\uE224]Hide in Vote Screen"),
            OBFUSCATE("Toggle_[\uE224]Hide in Lobby"),

            OBFUSCATE("Category_[\uE31A]Mini Map"),
            OBFUSCATE("Toggle_[\uE31A]Show Mini Map"),
            OBFUSCATE("Toggle_True_[\uE6F6]Show Map Players"),
            OBFUSCATE("Toggle_True_[\uE442]Show Map Dead Bodies"),
            OBFUSCATE("Toggle_True_[\uE2DE]Touch Teleport (Map)"),
            OBFUSCATE("Toggle_True_[\uE224]Hide Map in Vote Screen"),

            OBFUSCATE("Category_[\uE326]Voice"),
            OBFUSCATE("Toggle_[\uE326]Hear Dead Voice"),
            OBFUSCATE("Toggle_[\uE326]Hear Far Players"),

            OBFUSCATE("Category_[\uE242]Sabotage & Tasks"),
            OBFUSCATE("Button_[\uE242]Unlock Sabotages"),
            OBFUSCATE("Toggle_[\uE242]Auto Repair Sabotage"),
            OBFUSCATE("Button_[\uE242]Repair Sabotage Now"),
            OBFUSCATE("RichTextView_[\uE4E0]<b>Warning:</b> Auto Tasks has a ban risk! Think twice before using it."),
            OBFUSCATE("Toggle_[\uEBA6]Auto Tasks"),
            OBFUSCATE("Button_[\uEBA6]Complete 1 Task"),

            OBFUSCATE("Category_[\uE190]Miscellaneous"),
            OBFUSCATE("Button_[\uE0CE]Call Emergency"),
            OBFUSCATE("Toggle_[\uE186]Auto Ready (Lobby)"),

            OBFUSCATE("Category_[\uE5F4]Debug Panel"),
            OBFUSCATE("Toggle_[\uE2CE]Show Debug Info"),

            OBFUSCATE("Category_[\uE79E]Experimental [May Not Work]"),
            OBFUSCATE("Toggle_[\uE40A]Anti-Death [LOCAL]"),
            OBFUSCATE("Toggle_[\uE628]Speed Boost [LOCAL]"),
            OBFUSCATE("SeekBar_[\uE434]Speed Multiplier_10_40"),

            OBFUSCATE("Category_[\uE46A]About"),
            OBFUSCATE("RichTextView_[\uE348]<b>Goose Goose Duck Mod Menu</b><br/>Free and open source mod for Android.<br/>Use at your own risk!"),
            OBFUSCATE("ButtonLink_[\uE4FC]YouTube: @anonimbiri_IsBack_https://youtube.com/@anonimbiri_IsBack"),
            OBFUSCATE("ButtonLink_[\uE576]Developer: anonimbiri_https://github.com/anonimbiri-IsBack"),
            OBFUSCATE("ButtonLink_[\uE1BC]GitHub Open Source_https://github.com/GameSketchers/Goose-Goose-Duck-Android-Mod"),
    };
    int count = sizeof(features) / sizeof(features[0]);
    jobjectArray ret = env->NewObjectArray(count, env->FindClass("java/lang/String"), env->NewStringUTF(""));
    for (int i = 0; i < count; i++) env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));
    return ret;
}

void Changes(JNIEnv *env, jclass clazz, jobject obj, jint featNum, jstring featName, jint value, jlong Lvalue, jboolean boolean, jstring str) {
    if (!g_JavaVM) env->GetJavaVM(&g_JavaVM);
    switch (featNum) {
        case 0: UnlimitedVision = boolean; if (!boolean && localPlayerInstance) *(bool*)((uintptr_t)localPlayerInstance + OFFSET_PE_FOGOFWAR) = true; break;
        case 1: RemoveRoof = boolean; if (boolean && g_RoofHandler && RoofHandler_DeactivateRoofs && !g_RoofRemovedThisRound) { RoofHandler_DeactivateRoofs(g_RoofHandler, true); g_RoofRemovedThisRound = true; } else if (!boolean && g_RoofHandler && RoofHandler_DeactivateRoofs) { RoofHandler_DeactivateRoofs(g_RoofHandler, false); g_RoofRemovedThisRound = false; } break;
        case 2: NoCooldown = boolean; break;
        case 3: SeeGhosts = boolean; if (!boolean && localPlayerObject && SetCanSeeGhosts) SetCanSeeGhosts(localPlayerObject, false); break;
        case 4: NoClip = boolean; if (localPlayerInstance) ApplyNoClip(localPlayerInstance, boolean); break;
        case 5: DroneView = boolean; if (!boolean) DisableDroneView(); else ResetDroneViewDelay(); break;
        case 6: DroneZoom = (float)value; if (DroneView && g_DroneViewReady && localPlayerObject && OverrideOrthographicSize) OverrideOrthographicSize(localPlayerObject, DroneZoom); break;
        case 7: ESPEnabled = boolean; if (boolean) { g_ESPStabilized = false; g_FrameCount = 0; } SetESPEnabled(boolean || DebugMode); break;
        case 8: ESPLines = boolean; break;
        case 9: ESPBox = boolean; break;
        case 10: ESPDistance = boolean; break;
        case 11: ESPName = boolean; break;
        case 12: ESPEdgeIndicator = boolean; break;
        case 13: ESPHideInVote = boolean; break;
        case 14: ESPHideInLobby = boolean; break;

        case 15:
            MiniMapEnabled = boolean;
            if (env && g_SetMiniMapVisibleMethod) env->CallStaticVoidMethod(g_MenuClass, g_SetMiniMapVisibleMethod, (jboolean)boolean);
            break;
        case 16:
            MiniMapShowPlayers = boolean;
            if (env && g_SetMapShowPlayersMethod) env->CallStaticVoidMethod(g_MenuClass, g_SetMapShowPlayersMethod, (jboolean)boolean);
            break;
        case 17:
            MiniMapShowDead = boolean;
            if (env && g_SetMapShowDeadBodiesMethod) env->CallStaticVoidMethod(g_MenuClass, g_SetMapShowDeadBodiesMethod, (jboolean)boolean);
            break;
        case 18:
            MiniMapTouchTP = boolean;
            if (env && g_SetMapTouchTeleportMethod) env->CallStaticVoidMethod(g_MenuClass, g_SetMapTouchTeleportMethod, (jboolean)boolean);
            break;
        case 19:
            MiniMapHideInVote = boolean;
            break;

        case 20: HearDeadVoice = boolean; break;
        case 21: HearFarPlayers = boolean; break;

        case 22: btnUnlockSabotages = true; break;
        case 23: AutoRepairSabotage = boolean; break;
        case 24: btnRepairSabotageNow = true; break;
        case 25: SafeAutoTasks = boolean; if (boolean) g_LastSafeTaskTime = std::chrono::steady_clock::now(); break;
        case 26: btnCompleteOneTask = true; break;

        case 27: btnCallEmergency = true; break;
        case 28: AutoReady = boolean; break;
        case 29: DebugMode = boolean; SetESPEnabled(boolean || ESPEnabled); break;
        case 30: AntiDeath = boolean; break;
        case 31: SpeedHack = boolean; break;
        case 32: SpeedMultiplier = (float)value / 10.0f; break;
    }
}

ElfScanner g_il2cppELF;

void hack_thread() {
    LOGI("pthread created");
    while (!isLibraryLoaded(targetLibName)) sleep(1);
    do { sleep(1); g_il2cppELF = ElfScanner::createWithPath(targetLibName); } while (!g_il2cppELF.isValid());
    LOGI("%s loaded", (const char*)targetLibName);
    void* il2cppHandle = dlopen("libil2cpp.so", RTLD_NOW);
    if (il2cppHandle) {
        il2cpp_string_new_func = (il2cpp_string_new_t)dlsym(il2cppHandle, "il2cpp_string_new");
        LOGI("il2cpp_string_new: %p", il2cpp_string_new_func);
    }

#if defined(__aarch64__)
    // GameManager.Update - RVA: 0x3AD9E54
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3AD9E54")), GameManager_Update, old_GameManager_Update);

    // GameManager.IsInGame - RVA: 0x3ADC5B4
    GameManager_IsInGame = (bool (*)(void*))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3ADC5B4")));

    // GameManager.IsInLobby - RVA: 0x3ADC5C4
    GameManager_IsInLobby = (bool (*)(void*))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3ADC5C4")));

    // GameManager.IsInMeeting - RVA: 0x3ADC5D4
    GameManager_IsInMeeting = (bool (*)(void*))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3ADC5D4")));

    // PlayerPropertiesManager.Initialize - RVA: 0x3AC4878
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3AC4878")), hook_PlayerPropertiesManager_Initialize, old_PlayerPropertiesManager_Initialize);

    // PlayerPropertiesManager.ChangeReadyState - RVA: 0x3AC4978
    PlayerPropertiesManager_ChangeReadyState = (void (*)(void*, int))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3AC4978")));

    // PlayerPropertiesManager.GetUserProperties - RVA: 0x3AC5A98
    PlayerPropertiesManager_GetUserProperties = (void* (*)(void*))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3AC5A98")));

    // MapManager.Internal_OnMapStart - RVA: 0x3831E54
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3831E54")), hook_Internal_OnMapStart, old_Internal_OnMapStart);

    // MapManager.Internal_OnMapLoad - RVA: 0x3831058
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3831058")), hook_Internal_OnMapLoad, old_Internal_OnMapLoad);

    // PlayableEntity.Update - RVA: 0x3E4FC30
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3E4FC30")), Update, old_Update);

    // PlayableEntity.LateUpdate - RVA: 0x3E509E8
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3E509E8")), LateUpdate, old_LateUpdate);

    // PlayableEntity.TurnIntoGhost - RVA: 0x3E5B99C
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3E5B99C")), TurnIntoGhost, old_TurnIntoGhost);

    // PlayableEntity.Despawn - RVA: 0x3E52680
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3E52680")), Despawn, old_Despawn);

    // LocalPlayer.Update - RVA: 0x3E3536C
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3E3536C")), LocalPlayer_Update, old_LocalPlayer_Update);

    // LocalPlayer.GetPlayerSpeed - RVA: 0x3E44BE8
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3E44BE8")), GetPlayerSpeed, old_GetPlayerSpeed);

    // LocalPlayer.StartRound - RVA: 0x3E3CEB4
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3E3CEB4")), LocalPlayer_StartRound, old_LocalPlayer_StartRound);

    // CinemachineStateDrivenCamera.InternalUpdateCameraState - RVA: 0x4437958
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x4437958")), StateCameraUpdate, old_StateCameraUpdate);

    // LocalPlayer.OverrideOrthographicSize - RVA: 0x3E455BC
    OverrideOrthographicSize = (void (*)(void*, float))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3E455BC")));

    // PlayableEntity.TeleportTo - RVA: 0x3E61054
    TeleportTo = (void (*)(void*, Vector2, bool))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3E61054")));

    // LocalPlayer.SetCanSeeGhosts - RVA: 0x3E4CF04
    SetCanSeeGhosts = (void (*)(void*, bool))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3E4CF04")));

    // GGDRole.OnEnterVent - RVA: 0x3CD0438
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3CD0438")), OnEnterVent, old_OnEnterVent);

    // GGDRole.OnExitVent - RVA: 0x3CD052C
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3CD052C")), OnExitVent, old_OnExitVent);

    // GGDRole.SetVentCooldown - RVA: 0x3CCF058
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3CCF058")), SetVentCooldown, old_SetVentCooldown);

    // TasksHandler.OnEnable - RVA: 0x3E2A1B0
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3E2A1B0")), TasksHandler_OnEnable, old_TasksHandler_OnEnable);

    // TasksHandler.OnDisable - RVA: 0x3E2A2B8
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3E2A2B8")), TasksHandler_OnDisable, old_TasksHandler_OnDisable);

    // TasksHandler.CompleteTask - RVA: 0x3E2D680
    TasksHandler_CompleteTask = (void (*)(void*, void*, bool, bool, bool, bool))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3E2D680")));

    // TasksHandler.UpdateTaskVisuals - RVA: 0x3E2EDEC
    TasksHandler_UpdateTaskVisuals = (void (*)(void*))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3E2EDEC")));

    // RoofHandler.Awake - RVA: 0x3DBEE78
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DBEE78")), RoofHandler_Awake, old_RoofHandler_Awake);

    // RoofHandler.OnDestroy - RVA: 0x3DBEF7C
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3DBEF7C")), RoofHandler_OnDestroy, old_RoofHandler_OnDestroy);

    // RoofHandler.DeactivateRoofs - RVA: 0x3DBF188
    RoofHandler_DeactivateRoofs = (void (*)(void*, bool))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3DBF188")));

    // PlayerController.CallEmergency - RVA: 0x3E72C18
    PlayerController_CallEmergency = (void (*)(void*))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x3E72C18")));

    // Collider2D.set_isTrigger - RVA: 0x754CABC
    Collider2D_set_isTrigger = (void (*)(void*, bool))getAbsoluteAddress(targetLibName, str2Offset(OBFUSCATE("0x754CABC")));

    // WallCollisionCheckHandler.OnCollisionEnter2D - RVA: 0x37E4400
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x37E4400")), WallCollisionCheckHandler_OnCollisionEnter2D, old_WallCollisionCheckHandler_OnCollisionEnter2D);

    // VoiceChatHandler.CanHearPlayer - RVA: 0x3844A90
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3844A90")), hook_CanHearPlayer, old_CanHearPlayer);

    // VoiceChatHandler.CanHearPlayerFromMeeting - RVA: 0x3844D80
    HOOK(targetLibName, str2Offset(OBFUSCATE("0x3844D80")), hook_CanHearPlayerFromMeeting, old_CanHearPlayerFromMeeting);

    LOGI("All features and hooks installed!");
#endif
    LOGI("Done");
}

__attribute__((constructor))
void lib_main() { std::thread(hack_thread).detach(); }