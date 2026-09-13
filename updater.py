#!/usr/bin/env python3
import os
import re
import sys
import shutil

# Windows terminal TrueColor/ANSI desteğini aktif et
if os.name == 'nt':
    os.system('')

class C:
    RESET   = "\033[0m"
    BOLD    = "\033[1m"
    DIM     = "\033[2m"
    
    RED     = "\033[91m"
    GREEN   = "\033[92m"
    YELLOW  = "\033[93m"
    BLUE    = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN    = "\033[96m"
    WHITE   = "\033[97m"

def rgb(r, g, b):
    return f"\033[38;2;{r};{g};{b}m"

# Target Field Mappings categorized by class name
FIELD_TARGETS = {
    "PlayableEntity": {
        "OFFSET_PE_ENTITYNUMBER": [r"\bint\s+entityNumber\b"],
        "OFFSET_PE_NICKNAME": [r"\bstring\s+nickname\b"],
        "OFFSET_PE_ISLOCAL": [r"\bbool\s+isLocal\b"],
        "OFFSET_PE_PLAYERROLE": [r"\bGGDRole\s+playerRole\b"],
        "OFFSET_PE_ISPLAYERROLESET": [r"\bbool\s+isPlayerRoleSet\b"],
        "OFFSET_PE_KILLEDBY": [r"\bstring\s+killedBy\b"],
        "OFFSET_PE_TARGETOPACITY": [r"\bfloat\s+targetOpacity\b"],
        "OFFSET_PE_TASKSREMAINING": [r"\bint\s+tasksRemaining\b"],
        "OFFSET_PE_HASKILLED": [r"\bbool\s+hasKilledThisRound\b", r"\bbool\s+hasKilled\b"],
        "OFFSET_PE_TEAMID": [r"\bint\s+teamId\b"],
        "OFFSET_PE_FOGOFWAR": [r"\bbool\s+fogOfWarEnabled\b", r"\bbool\s+fogOfWar\b"],
        "OFFSET_PE_ISRUNNING": [r"\bbool\s+isRunning\b"],
        "OFFSET_PE_ISGHOST": [r"\bbool\s+isGhost\b"],
        "OFFSET_PE_ISINFECTED": [r"\bbool\s+isInfected\b"],
        "OFFSET_PE_ISDOWNED": [r"\bbool\s+isDowned\b"],
        "OFFSET_PE_INVENT": [r"\bbool\s+inVent\b"],
        "OFFSET_PE_HASBOMB": [r"\bbool\s+hasBomb\b"],
        "OFFSET_PE_ISINVISIBLE": [r"\bbool\s+isInvisible\b"],
        "OFFSET_PE_ISINPELICAN": [r"\bbool\s+isInPelican\b"],
        "OFFSET_PE_ISMORPHED": [r"\bbool\s+isMorphed\b"],
        "OFFSET_PE_ISSPECTATOR": [r"\bbool\s+isSpectator\b"],
        "OFFSET_PE_RIGIDBODY": [r"\bRigidbody2D\s+rigidBody\b", r"\bRigidbody2D\s+rigidbody\b"],
        "OFFSET_PE_TRANSFORMVIEW": [r"\bBetterPhotonTransformView\s+transformView\b"],
        "OFFSET_PE_BODYCOLLIDER": [r"\bCapsuleCollider2D\s+bodyCollider\b"],
        "OFFSET_PE_PLAYERCOLLIDER": [r"\bCapsuleCollider2D\s+playerCollider\b"],
        "OFFSET_PE_WALLCHECKCOLLIDER": [r"\bCapsuleCollider2D\s+wallCheckCollider\b"],
        "OFFSET_PE_WALLCOLLISIONHANDLER": [r"\bWallCollisionCheckHandler\s+wallCollisionCheckHandler\b"],
        "OFFSET_PE_CONFINECOLLIDER": [r"\bBoxCollider2D\s+confineCollider\b"],
        "OFFSET_PE_STATIC_DEADPLAYERSCOUNT": [r"\bstatic\s+int\s+deadPlayersCount\b"]
    },
    "LocalPlayer": {
        "OFFSET_LP_MAINCAMERA": [r"\bCamera\s+mainCamera\b"],
        "OFFSET_LP_STATECAMERA": [r"\bCinemachineStateDrivenCamera\s+stateCamera\b"],
        "OFFSET_LP_SCRIPTABLESTATE": [r"\bCinemachineVirtualCamera\s+scriptableState\b"],
        "OFFSET_LP_INVOTINGSCREEN": [r"\bbool\s+inVotingScreen\b"],
        "OFFSET_LP_INVOTINGTRANSITION": [r"\bbool\s+inVotingScreenIncludingTransition\b", r"\bbool\s+inVotingTransition\b"],
        "OFFSET_LP_INGAMESTARTSPOTLIGHT": [r"\bbool\s+inGameStartSpotlightScreen\b"],
        "OFFSET_LP_INGAMEENDSPOTLIGHT": [r"\bbool\s+inGameEndSpotlightScreen\b"],
        "OFFSET_LP_CANSEEGHOSTS": [r"\bbool\s+canSeeGhosts\b"]
    },
    "BetterPhotonTransformView": {
        "OFFSET_TV_LATESTPOS": [r"\bVector2\s+latestPos\b"],
        "OFFSET_TV_LASTTRANSFORMPOS": [r"\bVector2\s+lastTransformPos\b"]
    },
    "CinemachineStateDrivenCamera": {
        "OFFSET_CSDC_STATE": [r"\bCameraState\s+m_State\b"]
    },
    "GGDRole": {
        "OFFSET_ROLE_TYPE": [r"\bRoleType\s+type\b", r"\bRoleType\s+roleType\b"]
    },
    "TasksHandler": {
        "OFFSET_TH_SORTEDASSIGNEDTASKS": [r"\bList<GameTask>\s+sortedAssignedTasks\b"]
    },
    "GameTask": {
        "OFFSET_GT_TASKID": [r"\bstring\s+taskId\b"],
        "OFFSET_GT_ISFAKETASK": [r"\bbool\s+isFakeTask\b"]
    },
    "WallCollisionCheckHandler": {
        "OFFSET_WCCH_INWALL": [r"\bbool\s+inWall\b"]
    }
}

# Target Method Signatures categorized by (Class, MethodName)
METHOD_TARGETS = {
    ("PlayableEntity", "Update"): r"void\s+Update\s*\(",
    ("PlayableEntity", "LateUpdate"): r"void\s+LateUpdate\s*\(",
    ("PlayableEntity", "TurnIntoGhost"): r"void\s+TurnIntoGhost\s*\(",
    ("PlayableEntity", "Despawn"): r"void\s+Despawn\s*\(",
    ("PlayableEntity", "TeleportTo"): r"void\s+TeleportTo\s*\(",

    ("LocalPlayer", "Update"): r"void\s+Update\s*\(",
    ("LocalPlayer", "GetPlayerSpeed"): r"float\s+GetPlayerSpeed\s*\(",
    ("LocalPlayer", "StartRound"): r"void\s+StartRound\s*\(",
    ("LocalPlayer", "OverrideOrthographicSize"): r"void\s+OverrideOrthographicSize\s*\(",
    ("LocalPlayer", "SetCanSeeGhosts"): r"void\s+SetCanSeeGhosts\s*\(",

    ("CinemachineStateDrivenCamera", "InternalUpdateCameraState"): r"void\s+InternalUpdateCameraState\s*\(",

    ("GGDRole", "OnEnterVent"): r"void\s+OnEnterVent\s*\(",
    ("GGDRole", "OnExitVent"): r"void\s+OnExitVent\s*\(",
    ("GGDRole", "SetVentCooldown"): r"void\s+SetVentCooldown\s*\(",

    ("TasksHandler", "OnEnable"): r"void\s+OnEnable\s*\(",
    ("TasksHandler", "OnDisable"): r"void\s+OnDisable\s*\(",
    ("TasksHandler", "CompleteTask"): r"void\s+CompleteTask\s*\(",
    ("TasksHandler", "UpdateTaskVisuals"): r"void\s+UpdateTaskVisuals\s*\(",

    ("RoofHandler", "Awake"): r"void\s+Awake\s*\(",
    ("RoofHandler", "OnDestroy"): r"void\s+OnDestroy\s*\(",
    ("RoofHandler", "DeactivateRoofs"): r"void\s+DeactivateRoofs\s*\(",

    ("PlayerController", "CallEmergency"): r"void\s+CallEmergency\s*\(",

    ("Collider2D", "set_isTrigger"): r"void\s+set_isTrigger\s*\(",
    ("WallCollisionCheckHandler", "OnCollisionEnter2D"): r"void\s+OnCollisionEnter2D\s*\("
}

def ask_for_path(prompt_text, default_filename):
    while True:
        if os.path.exists(default_filename):
            user_input = input(f"{C.YELLOW}[?]{C.RESET} Enter path for {C.BOLD}{prompt_text}{C.RESET} [{C.CYAN}Default: {default_filename}{C.RESET}]: ").strip()
            if not user_input:
                return default_filename
            if os.path.exists(user_input):
                return user_input
            print(f"{C.RED}[-] File not found: {user_input}. Please try again.{C.RESET}")
        else:
            user_input = input(f"{C.YELLOW}[?]{C.RESET} Enter path for {C.BOLD}{prompt_text}{C.RESET}: ").strip()
            if os.path.exists(user_input):
                return user_input
            print(f"{C.RED}[-] File not found: {user_input}. Please try again.{C.RESET}")

def parse_dump(dump_path):
    print(f"\n{C.CYAN}[*]{C.RESET} Streaming & parsing {C.BOLD}'{dump_path}'{C.RESET} line by line...")
    extracted_fields = {}
    extracted_methods = {}
    extracted_typedefs = {}

    current_class = None
    last_rva = None

    class_regex = re.compile(r"public\s+(?:abstract\s+|sealed\s+|static\s+)?(?:class|struct)\s+(\w+)(?:[^{]*TypeDefIndex:\s*(\d+))?")
    rva_regex = re.compile(r"//\s*RVA:\s*(0x[0-9A-Fa-f]+)")
    offset_regex = re.compile(r"//\s*(0x[0-9A-Fa-f]+)")

    line_count = 0
    with open(dump_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            line_count += 1
            if line_count % 250000 == 0:
                print(f"    {C.DIM}-> Processed {line_count:,} lines...{C.RESET}")

            c_match = class_regex.search(line)
            if c_match:
                current_class = c_match.group(1)
                tdef = c_match.group(2)
                if tdef and current_class:
                    extracted_typedefs[current_class] = tdef
                last_rva = None
                continue

            if not current_class:
                continue

            r_match = rva_regex.search(line)
            if r_match:
                last_rva = r_match.group(1)
                continue

            if last_rva:
                for (cls_name, m_name), sig_pattern in METHOD_TARGETS.items():
                    if cls_name == current_class:
                        if re.search(sig_pattern, line):
                            key = f"{cls_name}.{m_name}"
                            if key not in extracted_methods:
                                extracted_methods[key] = last_rva
                last_rva = None

            if current_class in FIELD_TARGETS:
                for def_name, patterns in FIELD_TARGETS[current_class].items():
                    if def_name not in extracted_fields:
                        for pat in patterns:
                            if re.search(pat, line):
                                off_match = offset_regex.search(line)
                                if off_match:
                                    extracted_fields[def_name] = off_match.group(1)
                                    break

    print(f"{C.GREEN}[✓] Parsing complete!{C.RESET} Analyzed {C.BOLD}{line_count:,}{C.RESET} lines.")
    print(f"    {C.CYAN}•{C.RESET} Found {C.BOLD}{len(extracted_typedefs)}{C.RESET} TypeDef Indices")
    print(f"    {C.CYAN}•{C.RESET} Found {C.BOLD}{len(extracted_fields)}{C.RESET} Field Offsets")
    print(f"    {C.CYAN}•{C.RESET} Found {C.BOLD}{len(extracted_methods)}{C.RESET} Method RVAs")
    return extracted_typedefs, extracted_fields, extracted_methods

def update_cpp(cpp_path, typedefs, fields, methods):
    print(f"\n{C.CYAN}[*]{C.RESET} Updating C++ Source File: {C.BOLD}'{cpp_path}'{C.RESET}...")
    
    with open(cpp_path, "r", encoding="utf-8") as f:
        content = f.read()

    total_changes = 0

    # 1. Update TypeDefIndex comments
    for cls_name, tdef in typedefs.items():
        pattern = rf"(//\s*{cls_name}\s*\(TypeDefIndex:\s*)\d+(\))"
        new_content, count = re.subn(pattern, rf"\g<1>{tdef}\g<2>", content)
        if count > 0:
            content = new_content
            total_changes += count

    # 2. Update #define Field Offsets
    print(f"\n{C.MAGENTA}--- Updating Field Offsets ---{C.RESET}")
    for def_name, new_offset in fields.items():
        pattern = rf"(#define\s+{def_name}\s+)0x[0-9A-Fa-f]+"
        new_content, count = re.subn(pattern, rf"\g<1>{new_offset}", content)
        if count > 0:
            content = new_content
            total_changes += count
            print(f"    {C.GREEN}[+]{C.RESET} {def_name:<34} -> {C.YELLOW}{new_offset}{C.RESET}")

    # 3. Update Method RVAs in Comments, HOOKs, and getAbsoluteAddress calls
    print(f"\n{C.MAGENTA}--- Updating Method RVAs & Hooks ---{C.RESET}")
    for method_key, new_rva in methods.items():
        comment_pattern = rf"(//\s*{re.escape(method_key)}\s*-\s*RVA:\s*)0x[0-9A-Fa-f]+"
        content, c1 = re.subn(comment_pattern, rf"\g<1>{new_rva}", content)

        escaped_key = re.escape(method_key)
        hook_pattern = rf"(//\s*{escaped_key}.*?\n\s*HOOK\([^,]+,\s*str2Offset\(OBFUSCATE\(\")0x[0-9A-Fa-f]+(\"\)\))"
        content, c2 = re.subn(hook_pattern, rf"\g<1>{new_rva}\g<2>", content)

        get_addr_pattern = rf"(//\s*{escaped_key}.*?\n\s*[\w:]+\s*=\s*\([^;]+getAbsoluteAddress\([^,]+,\s*str2Offset\(OBFUSCATE\(\")0x[0-9A-Fa-f]+(\"\)\)\);)"
        content, c3 = re.subn(get_addr_pattern, rf"\g<1>{new_rva}\g<2>", content)

        if c1 or c2 or c3:
            total_changes += (c1 + c2 + c3)
            print(f"    {C.GREEN}[+]{C.RESET} {method_key:<50} -> {C.YELLOW}{new_rva}{C.RESET}")

    # Create a safe backup of the original file
    backup_path = cpp_path + ".bak"
    shutil.copyfile(cpp_path, backup_path)
    print(f"\n{C.BLUE}[✓] Backup created at: '{backup_path}'{C.RESET}")

    # Write updated content
    with open(cpp_path, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"{C.GREEN}{C.BOLD}[✓] Successfully patched '{cpp_path}' with {total_changes} replacements!{C.RESET}")

def print_colored_logo():
    raw_logo = """
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@+==#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@-%+:...::-%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@+..=-+:.......:-*%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@+.-%@#::........::*:*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@#.:@@=.:-:.:.......:@-=#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@#:.::=:-@*.:=..:...:-+@-:.-@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@+%:.:-.:#@@=-*:.-::::@#::-:.:%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@:@+.::..:.:-@@.=#:-%-..*:....-*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@+:......-::-:-::=*@@+::......=@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@#-......::....:.=:@@@@@%:......#-@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@=......:....::=:++@@@@-:#@:.:%...:*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@%+++-=.......:..+:=@@@@=+%**%%%.=#:..:=@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@%:-.......-.::=%@@@@@@*::+*@%*%-....:%@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@%+=....:+-.:*@:*=@%@@@@@@@@@%-%+...-+%@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@%:....-...:#@@@@@@@@@@@@@@@@%:+...:-%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@+.@...::.:#@@@@@@@@@@@@@@@@%:-...+-*%@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@*#@.......:*@@@@@@#@@@@@@@@.-=..=@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@*#+::.-+.:=:+@@@@%%%#*@@@%:....:*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@=+..+-....:*@@@%=@@@@#=:...:@+-+@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@+%=:....::...:%@@@@%:#@::+%=+@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@%*@+.++-=++:......+@@@@=@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@*#@@@@@@-@*:.-%@@@@@#:%@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@+:*%@%@@@@@@@@@@=-*%%%%%%%@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@%+:-+*@@@@@#@@@@@@@%=:@#%@@@@**+*@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@=:=%@%+*%@@@@@@-*@@%@@@#.@@@@@@@@@@@:*@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@%--*@@@%:%@@@@@@@@@@@@@@@@@@@@*+@@@@@@@@@@@@#*@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@+#@@@@@@@-:@@@@@@@@@@@@@@@@@@@@@:#@@@@@@@@@@@@@%:%@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@:@@@@@@@@*:@@@@@@@@@@@@@@@@@@@@@@.#@@@@@@@@@@@@@@@+*@@@@@@@@@@@@@@@
@@@@@@@@@@@@+#@@@@@@@@*-@@@@@@@@@@@@@@@@@@@@@#.#@@@@@@@@@@@@@@@@+#@@@@@@@@@@@@@@
@@@@@@@@@@@@+@@@@@@@@*.-@@@@@@@@@@@@@@@@@@@@-+::@@@@@@@@@@@@@@@@@=+@@@@@@@@@@@@@
@@@@@@@@@@@%=@@@@@#@-.::@@@@@@@@@@@@@@@@@@%:::--:@@@@=*#%@@@@@@@@@%+@@@@@@@@@@@@
@@@@@@@@@@@#*@@@@@==..:::@@@@@@@@@@+@@@@@--#@%:::-@@#-..*%@@@@@@@@@#-@@@@@@@@@@@
@@@@@@@@@@@#*@@@@@+..-::-=@@@@@@@@+@@@@%-::-*@+:.:*@=%-*+:+%@@@@@@@@@**@@@@@@@@@
@@@@@@@@@@@#*@@@@@...#=::::@@@@@@%@@@@::-==---=+:.::#+#@@@-*@@@@@@@@@@#:%@@@@@@@
@@@@@@@@@@@#*@@@@:...:-::.:.%@@@@+@@%-:::+@#-::....:@=@@@@@@+@@@@@@@@@@@=#@@@@@@
@@@@@@@@@@@#*@@@@.........:::+@@%@@#.....:.........-+*@@@@@@@==@@@@@@@@@@**@@@@@
@@@@@@@@@@@#*@@@@%............:@@@:.................:@@@@@@@@@@-@@@@@@@@@@*+@@@@
@@@@@@@@@@@#*@@@@@-......................:..........@@@@@@@@@@@@**@@@@@@@@@%:@@@
@@@@@@@@@@@#*@@@@-=-......................:........+@@@@@@@@@@@@@@+*@@@@@@@@@=@@
@@@@@@@@@@@*#@@@@@@+:.::.:+@@@@@@@@@@@@@@@@+...:...%@@@@@@@@@@@@@@@*.:@@@@@@@#*@
@@@@@@@@@@@=@@@@@@@+-=@@@@@@@@@@@@@@@@@@@@@@@@@@@#*@@@@@@@@@@@@@@@@+%@@@@@@@@@=%
@@@@@@@@@@@=@@@@@+*=%:--@@@@@@@@@@@@@@@@@@@@@@@+#-@@@@@@@@@@@@@@@+=@@@@@@@@##=%@
@@@@@@@@@@@=@@@@@-=*@:*+@@@@@@@@@@@@@@@@@@@@@@@*:@@@@@@@@@@@@@@@+%@@@@@@@@%--@@@
@@@@@@@@@@+#@@@@@#**@-@@@@@@@@@@@@@@@@@@@@@@@@@*=@@@@@@@@@@@@@@-#@@@@@@@%=-%@@@@
@@@@@@@@@@:@@@@@@@=%*-@@@@@@@@@@@@@@@@@@@@@@*=%=@@@@@@@@@@@@@*#@@@@@@@#-:#@@@@@@
@@@@@@@@@=@@%@@@@@=@-%@@@@@@@@@@@@@@@@@@@@@%*:+-@@@@@@@@@@@%+@@@@@@@@@:#@@@@@@@@
@@@@@@@@*#@@@@@@%%:=@@@@@@@@@@@@@@@@@@@@#+*@@@*++#@@@@@@@%+@@@@@@@@*+@@@@@@@@@@@
@@@@@@@%+@@@@@@@@=-%@@@@@@@@@@@@@@@@@@@+-=*.:+@@%%*=@@*-..-@@@@@@+*%@@@@@@@@@@@@
@@@@@@@#+@@@@@@@-:@@@@@@@@@@@@@@@@@@@@@@+=@@@*:+@@@@@@*@-..:*#%:*@@@@@@@@@@@@@@@
@@@@@@@-@@@@@@@-.:#@@@@@@@@@@@%=@@@@@@*-++*@@****@@@@@@@@+:.:=@@@@@@@@@@@@@@@@@@
@@@@@@%-@@@@@@-...:#@@%@@@@@@@@@@@@@@@@:+==#-%%@@##@@@@@@*::%@@@@@@@@@@@@@@@@@@@
@@@@@@-@@@@@@:=*.:#-:@@@@@@@@@@@@@@@@@@@@@=@-.....*=-@@:=@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@-@@@@@*+@@=:::+=+@@@@@@@@@@@@@@@@@@@#=::...:=%@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@:@@@@%--#@@:=-.::-+@@@@@@@@@@@@@@%=:-+-:==%%+*@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@+%@@@@#:-@@@@+::.....*@@@@@@@@@%-:::.::-:%@@@@-@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@+%@@@@-##@@@@@+=.......:****+::::..-:::+@@@@%+-+@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@=%@@@==@@@@@@@@-:.................:::-@@@@@@@@@:@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@#*@@@@:@@@@@@@@@@=.................--=@@@@@@@@@@-%@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@#*@@%=-##@@@@@@@@@+................-%@@@@@@@@%%@@+@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@=@@@@=@#@@@@@@@@@@@-..............:@@@@@@@@@@@%@@=@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@=@@%:.@@@@@@@@@@@@@@-............-@@@@@@@@@@@@@@@**@@@@@@@@@@@@@@@@@@@@@@@@@
@@@:@@@%+-%@@@@@@@@@@@@%*=..........=@@@@@@@@@@@@@@@@@=@@@@@@@@@@@@@@@@@@@@@@@@@
@@@:@@@%=-@@@@@@@@@@@@@@@*:........-@@@@@@@@@@@@@@@@@@@:@@@@@@@@@@@@@@@@@@@@@@@@
@@@:@@@@+-@@@@@@@@@@@@@@@@@-......:@@@@@@@@@@@@@@@@@@@@#-@@@@@@@@@@@@@@@@@@@@@@@
@@=@@@@@*.@#@@@@@@@@@@@@@@@@+.....*@@@@@@@@@@@@@@@@@@@@@=#@@@@@@@@@@@@@@@@@@@@@@
@@=@@@@@*.@#%@@@@@@@@@@@@@@@-=:@@=*#@@@@@@@@@@@@@@@@@@@@%*@@@@@@@@@@@@@@@@@@@@@@
@*#@%#@@*:%@@@@@@@@@@@@@@@@@@@-*@@:@@@@@@@@@@@@@@@@@@@@@@+%@@@@@@@@@@@@@@@@@@@@@
%+@@#-%%+*-@@@@@@@@@@@@@@@@@@@%:@@++%@@@@@@@@@@@@@@@@@@@@+*@@@@@@@@@@@@@@@@@@@@@
@*#@#.%%+@:@@@@@@@@@@@@@@@@@@@@:@@@:-#@@@@@@@@@@@@@@@@@@@@-@@@@@@@@@@@@@@@@@@@@@
@@=@#.%%+@:*@@@@@@@@@@@@@@@@@@*:@@@%+@@@@@@@@@@@@@@@@@@@@@=%@@@@@@@@@@@@@@@@@@@@
@@=@%=:=:-@=@@@@@@@@@@@@@@@@@@#:@@@@+%@@@@@@@@@@@@@@@@@@@@@:@@@@@@@@@@@@@@@@@@@@
@@@@=@-#+:::#@@@@@@@@@@@@@@@@@+:@@@@@-@@@@@@@@@@@@@@@@@@@@@:@@@@@@@@@@@@@@@@@@@@
@@@@@*#%-=%#+#@@@@@@@@@@@@@@@@=+@@@@@%:##@@@@@@@@@@@@@@@@@@@+@@@@@@@@@@@@@@@@@@@
@@@@@@@#%@@%=--%@@@@@@@@@@@@@@-@@@@@@@#=+@@@@@@@@@@@@@@@@@@@+@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@+%@@@@@@@@@@@@@@@@-@@@@@@@@*#@@@@@@@@@@@@@@@@@@@+#@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@:@@@@@@@@@@@@@@@@-@@@@@@@@@=#@@@@@@@@@@@@@@@@@@*#@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@++@@@@@@@@@@@@@@@-@@@@@@@@@@:#@@@@@@@@@@@@@@@@@*+@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@-%*@@@@@@@@@@@@@-@@@@@@@@@@@-%@@@@@@@@@@@@@@@@@=@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@%=*=@@@@@@@@@@@@-@@@@@@@@@@@%=@@@@@@@@@@@@@@@@@=@@@@@@@@@@@@@@@@@@
    """

    COLOR_SKIN_BRIGHT = rgb(255, 222, 195)
    COLOR_SKIN_SHADOW = rgb(245, 190, 160)
    COLOR_BLUSH       = rgb(235, 130, 145)
    COLOR_HAIR_LIGHT  = rgb(235, 185, 255)
    COLOR_HAIR_MID    = rgb(175, 120, 205)
    COLOR_HAIR_DARK   = rgb(110, 65, 140)
    COLOR_BG_DARK     = rgb(45, 30, 65)

    for line in raw_logo.split("\n"):
        if not line:
            continue
        colored_line = []
        for ch in line:
            if ch == '.':
                colored_line.append(f"{COLOR_SKIN_BRIGHT}{ch}")
            elif ch == ':':
                colored_line.append(f"{COLOR_SKIN_SHADOW}{ch}")
            elif ch in ['-', '=']:
                colored_line.append(f"{COLOR_BLUSH}{ch}")
            elif ch in ['+', '*']:
                colored_line.append(f"{COLOR_HAIR_LIGHT}{ch}")
            elif ch in ['#', '%']:
                colored_line.append(f"{COLOR_HAIR_MID}{ch}")
            elif ch == '@':
                colored_line.append(f"{COLOR_BG_DARK}{ch}")
            else:
                colored_line.append(f"{COLOR_HAIR_DARK}{ch}")
        print("".join(colored_line) + C.RESET)

def main():
    print_colored_logo()
    print(f"\n{C.CYAN}{C.BOLD}{'='*60}{C.RESET}")
    print(f"{C.YELLOW}{C.BOLD}        IL2CPP OFFSET & RVA AUTO-UPDATER TOOL         {C.RESET}")
    print(f"{C.MAGENTA}          by: anonimbiri & Google Gemini              {C.RESET}")
    print(f"{C.CYAN}{C.BOLD}{'='*60}{C.RESET}\n")

    if len(sys.argv) >= 3:
        cpp_path = sys.argv[1]
        dump_path = sys.argv[2]
    else:
        cpp_path = ask_for_path("C++ Source file (.cpp / .hpp)", "main.cpp")
        dump_path = ask_for_path("Il2CppDumper output file (dump.cs)", "dump.cs")

    if not os.path.isfile(cpp_path):
        print(f"{C.RED}[-] Error: C++ source file not found at '{cpp_path}'{C.RESET}")
        sys.exit(1)

    if not os.path.isfile(dump_path):
        print(f"{C.RED}[-] Error: dump.cs file not found at '{dump_path}'{C.RESET}")
        sys.exit(1)

    typedefs, fields, methods = parse_dump(dump_path)
    update_cpp(cpp_path, typedefs, fields, methods)
    print(f"\n{C.GREEN}{C.BOLD}[✓] All operations completed successfully!{C.RESET}\n")

if __name__ == "__main__":
    main()