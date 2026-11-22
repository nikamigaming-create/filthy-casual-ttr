"""Stage a local VR override; upstream files remain a separate dependency."""
from pathlib import Path
import hashlib, re, shutil, struct, json
ROOT=Path(__file__).resolve().parent.parent
UP=ROOT/'work/pool-upstream'
OUT=ROOT/'build/pool-vr/mod'
UDF=OUT/'NVSE/user_defined_functions/FilthyCasualPool'
UDF.mkdir(parents=True,exist_ok=True)
src=UP/'NVSE/user_defined_functions/Minigames/Pool'
def write(name,text):
    (UDF/name).write_text(text,encoding='utf-8')
# Generate defaults from the user's actual v4 source, without vendoring it.
original=(src/'Open.txt').read_text()
defaults=original.split('; BEGIN STATE DEFAULTS')[1].split('; END STATE DEFAULTS')[0]
needed=['Rack','Physics','DrawBalls','Audio','ShotStart','ResolveShot','PathClear',
        'FallbackOpponent','PlanCandidate','PlanBall','PlanStep','PlanFinish',
        'PlanOpponent','OpponentAvailable','ValidPosition','ClearTable','Challenge']
keys={'PlanCandidate':'planCandidateFn','PlanBall':'planBallFn','PlanStep':'planStepFn',
      'PlanFinish':'planFinishFn','ValidPosition':'valid'}
compiles=''
for n in needed:
    key=keys.get(n,n[0].lower()+n[1:])
    path=f'FilthyCasualPool/{n}.txt' if n=='PlanCandidate' else f'Minigames/Pool/{n}.txt'
    compiles+=f'    let aLP["{key}"] := CompileScript "{path}"\n'
    compiles+=f'    if eval (aLP["{key}"]) == 0\n        Print "FC Pool: {n} failed to compile"\n        return\n    endif\n'
candidate=(src/'PlanCandidate.txt').read_text()
candidate=candidate.replace('    let fPlanCut := fPlanNX * fPlanDX + fPlanNY * fPlanDY','''    let aLP["candidateNX"] := fPlanNX
    let aLP["candidateNY"] := fPlanNY
    DispatchEvent "FC:PoolCandidate" aLP
    if eval (aLP["candidateReachable"]) == 0
        let aLP["candidateDone"] := 1
        return
    endif
    let fPlanCut := fPlanNX * fPlanDX + fPlanNY * fPlanDY''')
write('PlanCandidate.txt',candidate)
start='''int opponent
array_var aLP
array_var aRefs
ref rTable
ref rBall
ref fnTick
ref fnRunning
ref rMarker
ref rPackage
int i
begin function {opponent}
    if eval PlayerRef.AuxVarGetFlt "*_FC_PoolActive"
        return
    endif
    let rTable := StringToRef "00107D7B"
    if eval (rTable.GetParentCell) != (player.GetParentCell) || player.IsInCombat || player.GetDead || MenuMode
        return
    endif
    if eval player.IsWeaponOut
        MessageEx "Pool: put your weapon away before playing."
        return
    endif
    if eval opponent && (SunnyRef.GetDead || SunnyRef.GetDisabled || SunnyRef.IsInCombat || (SunnyRef.GetParentCell) != (player.GetParentCell))
        MessageEx "Sunny is unavailable for pool."
        return
    endif
    let aLP := ar_Construct StringMap
DEFAULTS
COMPILES
    let fnTick := CompileScript "FilthyCasualPool/Tick.txt"
    let fnRunning := CompileScript "FilthyCasualPool/Running.txt"
    let aLP["close"] := CompileScript "FilthyCasualPool/Close.txt"
    if eval fnTick == 0 || fnRunning == 0 || (aLP["close"]) == 0
        return
    endif
    let aLP["table"] := rTable
    let aLP["yaw"] := rTable.GetAngle Z
    let aLP["mode"] := 1 + opponent
    let aLP["difficulty"] := 5
    let aLP["notice"] := ""
    let aLP["turn"] := 0
    let aLP["shotsLeft"] := 1
    let aLP["opponentRef"] := 0
    let aLP["opponentName"] := "Practice"
    if eval opponent
        let aLP["turn"] := 1
        let aLP["opponentRef"] := SunnyRef
        let aLP["opponentName"] := "Sunny Smiles"
        let aLP["actorRequired"] := 1
        let aLP["priorAIType"] := SunnyRef.GetCurrentAIPackage
        let aLP["priorPackage"] := SunnyRef.GetCurrentStablePackage
    endif
    let aLP["canStart"] := 0
    DispatchEvent "FC:PoolCheck" aLP
    if eval (aLP["canStart"]) == 0
        MessageEx "Pool: Sunny is busy with another activity."
        return
    endif
    let aLP["sunnyTick"] := CompileScript "FilthyCasualPool/SunnyTick.txt"
    let aLP["releaseSunny"] := CompileScript "FilthyCasualPool/ReleaseSunny.txt"
    let aLP["opponentAvailable"] := CompileScript "FilthyCasualPool/OpponentAvailable.txt"
    if eval opponent
        let rPackage := GetFormFromMod "Filthy Casual Pool.esp" "000802"
        let rMarker := PlayerRef.AuxVarGetRef "_FC_PoolMarker"
        if eval rMarker == 0
            let rMarker := rTable.PlaceAtMe XMarker 1
        endif
        if eval rPackage == 0 || rMarker == 0
            return
        endif
        rMarker.MoveTo rTable 120 0 -24
        rMarker.Enable
        PlayerRef.AuxVarSetRef "_FC_PoolMarker" rMarker
        let aLP["actorMarker"] := rMarker
        let aLP["actorPackage"] := rPackage
        let aLP["actorIdle"] := GetFormFromMod "Filthy Casual Pool.esp" "000803"
        SetPackageLocationReference rPackage rMarker
        SunnyRef.AddScriptPackage rPackage
    endif
    let aRefs := ar_Construct Array
REFS
    let aLP["ballRefs"] := aRefs
    let aLP["cueRef"] := GetFormFromMod "Pool Table Minigame.esp" "000AE0"
    let i := 0
    while i < 16
        let rBall := aRefs[i]
        if eval rBall == 0
            return
        endif
        rBall.Disable
        rBall.MoveTo rTable
        let i += 1
    loop
    let rBall := aLP["cueRef"]
    rBall.Disable
    rBall.MoveTo rTable
    Call (aLP["rack"]) aLP
    Call (aLP["clearTable"]) aLP
    let aLP["breakShot"] := opponent
    let aLP["vrEnd"] := 0
    let aLP["vrShot"] := 0
    let aLP["vrPlace"] := 0
    let aLP["vrShowCue"] := 0
    let aLP["actorFollow"] := 0
    let aLP["actorAimTime"] := 0
    let aLP["actorStarted"] := 0
    let aLP["actorAligned"] := 0
    let aLP["actorHitDone"] := 0
    let aLP["actorGapValid"] := 0
    let aLP["actorOwnIdle"] := 0
    let aLP["actorClockStall"] := 0
    let aLP["actorClockWait"] := 0
    let aLP["actorReady"] := 0
    let aLP["actorIdleResult"] := 0
    let aLP["actorIdleChosen"] := 0
    let aLP["actorSettled"] := 0
    let aLP["actorSettleTime"] := 0
    let aLP["lastTurn"] := -1
    let aLP["generation"] := (PlayerRef.AuxVarGetFlt "*_FC_PoolGeneration") + 1
    PlayerRef.AuxVarSetFlt "*_FC_PoolGeneration" (aLP["generation"])
    PlayerRef.AuxVarSetFlt "*_FC_PoolActive" 1
    MessageEx "Pool: hold both grips. Left hand guides. Slide the cue with your right. B leaves."
    CallWhile fnTick fnRunning 3 aLP
end
'''.replace('DEFAULTS',defaults).replace('COMPILES',compiles)
start=start.replace('REFS','\n'.join(f'    let aRefs[{i}] := GetFormFromMod "Pool Table Minigame.esp" "{0xB20+i:06X}"' for i in range(16)))
write('Start.txt',start)
write('Running.txt','''array_var aLP
begin function {aLP}
    SetFunctionValue ((PlayerRef.AuxVarGetFlt "*_FC_PoolActive") && (aLP["generation"]) == (PlayerRef.AuxVarGetFlt "*_FC_PoolGeneration"))
end
''')
write('ResetProps.txt','ref rObject\nbegin function {}\n'+''.join(f'    let rObject := GetFormFromMod "Pool Table Minigame.esp" "{fid:06X}"\n    if eval rObject\n        rObject.Disable\n    endif\n' for fid in [0xAE0,*range(0xB20,0xB30)])+'end\n')
write('Close.txt','''array_var aLP
array_var refs
ref rObject
int i
begin function {aLP}
    DispatchEvent "FC:PoolClose" aLP
    PlayerRef.AuxVarSetFlt "*_FC_PoolActive" 0
    Call (CompileScript "FilthyCasualPool/ReleaseSunny.txt")
    let refs := aLP["ballRefs"]
    let i := 0
    while i < 16
        let rObject := refs[i]
        if eval rObject
            rObject.Disable
        endif
        let i += 1
    loop
    let rObject := aLP["cueRef"]
    if eval rObject
        rObject.Disable
    endif
end
''')
write('Tick.txt','''array_var aLP
array_var balls
array_var cue
ref table
ref cueRef
ref rActor
ref rIdle
float dt
int steps
int valid
begin function {aLP}
    let table := aLP["table"]
    if eval (PlayerRef.GetParentCell) != (table.GetParentCell) || player.GetDead || player.IsInCombat || (player.GetDistance table) > 1200 || (aLP["vrEnd"])
        Call (aLP["close"]) aLP
        return
    endif
    if eval MenuMode
        DispatchEvent "FC:PoolSuspend" aLP
        return
    endif
    let dt := GetMinOf 0.0666667 (GetMaxOf 0 (GetSecondsPassed))
    let aLP["dt"] := dt
    let aLP["actorOwnIdle"] := 0
    if eval aLP["actorStarted"]
        let rActor := aLP["opponentRef"]
        let rIdle := aLP["actorIdle"]
        let aLP["actorOwnIdle"] := rActor.IsIdlePlayingEx rIdle
    endif
    let aLP["age"] += dt
    let aLP["frames"] += 1
    DispatchEvent "FC:PoolFrame" aLP
    let balls := aLP["balls"]
    let cue := balls[0]
    if eval aLP["moving"]
        let aLP["accum"] += dt
        let steps := 0
        while (aLP["accum"]) >= 0.008333333 && steps < 8
            Call (aLP["physics"]) aLP 0.008333333
            let aLP["accum"] -= 0.008333333
            let steps += 1
        loop
    else
        let aLP["accum"] := 0
        if eval (aLP["mode"]) == 2
            if eval (Call (aLP["opponentAvailable"]) aLP) == 0
                Call (aLP["close"]) aLP
                return
            endif
            if eval (aLP["shotPending"]) && (aLP["actorFollow"]) <= 0
                Call (aLP["resolveShot"]) aLP
                MessageEx "Pool: %z" (aLP["notice"])
                let aLP["aiPlanned"] := 0
                let aLP["aiAttempted"] := 0
                let aLP["aiPlanning"] := 0
                let aLP["actorPositioned"] := 0
                let aLP["actorIdleChosen"] := 0
                let aLP["actorSettled"] := 0
                let aLP["actorSettleTime"] := 0
                let aLP["actorAimTime"] := 0
                let aLP["actorStarted"] := 0
                let aLP["actorAligned"] := 0
                let aLP["actorHitDone"] := 0
                let aLP["actorGapValid"] := 0
                let aLP["actorOwnIdle"] := 0
                let aLP["actorClockStall"] := 0
    let aLP["actorClockWait"] := 0
            endif
            if eval (aLP["finished"])
                if eval (aLP["settled"]) == 0
                    if eval (aLP["winner"]) == 0
                        MessageEx "Pool: you win! B to leave the table."
                    else
                        MessageEx "Pool: Sunny wins. B to leave the table."
                    endif
                    let aLP["settled"] := 1
                endif
            elseif eval aLP["turn"]
                let aLP["aiWait"] += dt
                if eval (aLP["aiAttempted"]) == 0 && (aLP["aiWait"]) >= 0.8
                    let aLP["aiAttempted"] := 1
                    Call (aLP["planOpponent"]) aLP
                endif
                if eval aLP["aiPlanning"]
                    Call (aLP["planStepFn"]) aLP
                endif
                if eval (aLP["aiAttempted"]) && (aLP["aiPlanned"]) == 0 && (aLP["aiPlanning"]) == 0
                    Call (aLP["fallbackOpponent"]) aLP
                endif
                Call (aLP["sunnyTick"]) aLP
                if eval (aLP["aiPlanned"]) && (aLP["actorReady"]) && (aLP["actorContact"]) && (aLP["actorHitDone"]) == 0
                    Call (aLP["shotStart"]) aLP
                    let cue[4] := 1
                    let cue[2] := (aLP["nx"]) * (aLP["aiSpeed"])
                    let cue[3] := (aLP["ny"]) * (aLP["aiSpeed"])
                    let aLP["moving"] := 1
                    let aLP["shots"] += 1
                    let aLP["shotSound"] := 1
                    let aLP["aiWait"] := 0
                    let aLP["actorHitDone"] := 1
                endif
            endif
        elseif eval (cue[4]) == 0
            let aLP["inhand"] := 1
        endif
        if eval (aLP["turn"]) == 0 && (aLP["finished"]) == 0
            if eval aLP["inhand"]
                let valid := Call (aLP["valid"]) balls (aLP["vrBallX"]) (aLP["vrBallY"])
                if eval valid && (aLP["vrPlace"])
                    let cue[0] := aLP["vrBallX"]
                    let cue[1] := aLP["vrBallY"]
                    let cue[4] := 1
                    let aLP["inhand"] := 0
                endif
            elseif eval (aLP["vrShot"]) > 0
                if eval (aLP["mode"]) == 2
                    Call (aLP["shotStart"]) aLP
                endif
                let cue[2] := (aLP["vrNX"]) * (aLP["vrShot"])
                let cue[3] := (aLP["vrNY"]) * (aLP["vrShot"])
                let aLP["moving"] := 1
                let aLP["shots"] += 1
                let aLP["shotSound"] := 1
            endif
        endif
    endif
    let aLP["vrShot"] := 0
    let aLP["vrPlace"] := 0
    if eval (aLP["lastTurn"]) != (aLP["turn"])
        let aLP["lastTurn"] := aLP["turn"]
        if eval aLP["turn"]
            MessageEx "Pool: Sunny's turn."
        else
            if eval (aLP["playerGroup"]) == 1
                MessageEx "Pool: your turn. You are solids."
            elseif eval (aLP["playerGroup"]) == 2
                MessageEx "Pool: your turn. You are stripes."
            else
                MessageEx "Pool: your turn. Table is open."
            endif
        endif
    endif
    Call (aLP["drawBalls"]) aLP
    Call (aLP["audio"]) aLP dt
    let cueRef := aLP["cueRef"]
    if eval aLP["vrShowCue"]
        if eval cueRef.GetDisabled
            cueRef.Enable
        endif
    elseif eval cueRef.GetDisabled == 0
        DispatchEvent "FC:PoolDraw" aLP
        cueRef.Disable
    endif
    DispatchEvent "FC:PoolDraw" aLP
end
''')
# Override only the desktop entry point. Its camera and input scripts are never called.
override=OUT/'NVSE/user_defined_functions/Minigames/Pool'
override.mkdir(parents=True,exist_ok=True)
(override/'Activate.txt').write_text('''ref target
ref who
begin function {target,who}
    if eval who == player && target == (StringToRef "00107D7B")
        PlayerRef.AuxVarSetFlt "*_FC_PoolRequest" 1
    endif
end
''')
write('SunnyTopic.txt','''ref topic
begin function {topic}
    PlayerRef.AuxVarSetFlt "*_FC_PoolRequest" 2
end
''')
write('Init.txt','''ref fn
ref topic
ref greeting
array_var choices
int i
int iChoicePresent
begin function {}
    Print "FC Pool init begin"
    PlayerRef.AuxVarSetFlt "*_FC_PoolRequest" 0
    PlayerRef.AuxVarSetFlt "*_FC_PoolActive" 0
    Call (CompileScript "FilthyCasualPool/ReleaseSunny.txt")
    Call (CompileScript "FilthyCasualPool/ResetProps.txt")
    let fn := CompileScript "FilthyCasualPool/SunnyTopic.txt"
    let topic := GetFormFromMod "Filthy Casual Pool.esp" "000804"
    StartQuest topic
    let topic := GetFormFromMod "Filthy Casual Pool.esp" "000800"
    if eval fn && topic
        SetOnDialogTopicEventHandler fn 0 topic
        SetOnDialogTopicEventHandler fn 1 topic
        Print "FC Pool registering greeting choice"
        let greeting := StringToRef "00104E7E"
        let choices := DialogResponseRelatedGetAll greeting 1
        let iChoicePresent := 0
        let i := 0
        while i < (ar_Size choices)
            if eval choices[i] == topic
                let iChoicePresent := 1
            endif
            let i += 1
        loop
        if eval iChoicePresent == 0
            DialogResponseAddRelatedTopic greeting topic 1 0
        endif
    endif
end
''')
write('ReleaseSunny.txt','''array_var aRelease
ref rMarker
ref rPackage
ref rIdle
begin function {}
    let rPackage := GetFormFromMod "Filthy Casual Pool.esp" "000802"
    let aRelease := ar_Construct StringMap
    let aRelease["table"] := StringToRef "00107D7B"
    let aRelease["opponentRef"] := SunnyRef
    let aRelease["actorPackage"] := rPackage
    let aRelease["stablePackage"] := SunnyRef.GetCurrentStablePackage
    let aRelease["removePackage"] := 0
    DispatchEvent "FC:PoolRelease" aRelease
    Print "FC Pool release: ownership checked"
    let rIdle := PlayerRef.AuxVarGetRef "*_FC_PoolIdle"
    if eval rIdle && (SunnyRef.IsIdlePlayingEx rIdle)
        SunnyRef.ForceStopIdle
    endif
    PlayerRef.AuxVarErase "*_FC_PoolIdle"
    if eval aRelease["removePackage"]
        SunnyRef.RemoveScriptPackage
        Print "FC Pool release: own package removed"
        SunnyRef.EvaluatePackage
    endif
    if eval rPackage
        SetPackageLocationReference rPackage (StringToRef "00107D7B")
    endif
    let rMarker := PlayerRef.AuxVarGetRef "_FC_PoolMarker"
    if eval rMarker
        rMarker.Disable
    endif
    Print "FC Pool release: complete"
end
''')
write('OpponentAvailable.txt','''array_var aLP
ref rActor
ref rTable
begin function {aLP}
    let rActor := aLP["opponentRef"]
    let rTable := aLP["table"]
    SetFunctionValue (rActor && rActor.GetDead == 0 && rActor.GetDisabled == 0 && rActor.IsInCombat == 0 && (rActor.GetParentCell) == (rTable.GetParentCell))
end
''')
write('SunnyTick.txt','''array_var aLP
ref rActor
ref rMarker
ref rIdle
begin function {aLP}
    if eval aLP["actorStarted"]
        return
    endif
    let rActor := aLP["opponentRef"]
    let rMarker := aLP["actorMarker"]
    if eval (aLP["aiWait"]) > 45
        MessageEx "Pool: Sunny cannot reach this shot. Game ended."
        let aLP["vrEnd"] := 1
        return
    endif
    if eval (aLP["aiPlanned"]) == 0 || rActor == 0 || rMarker == 0
        return
    endif
    if eval aLP["actorMove"]
        rMarker.SetPosEx (aLP["actorX"]) (aLP["actorY"]) (aLP["actorZ"])
        rActor.EvaluatePackage
        let aLP["actorMove"] := 0
    endif
    if eval (rActor.GetDistance rMarker) < 24
        if eval (aLP["actorSettled"]) == 0
            rMarker.MoveTo rActor
            rActor.EvaluatePackage
            let aLP["actorSettled"] := 1
        endif
        if eval (aLP["actorIdleChosen"]) == 0
            return
        endif
        if eval (aLP["actorAligned"]) == 0
            ; <=3.54 units of XY correction at the already reached approach,
            ; before planting feet. Preserve the native floor height.
            rActor.SetPosEx (aLP["actorAlignX"]) (aLP["actorAlignY"]) (rActor.GetPos Z)
            rMarker.MoveTo rActor
            let aLP["actorAligned"] := 1
            let aLP["actorSettleTime"] := 0
            return
        endif
        let aLP["actorSettleTime"] += aLP["dt"]
        if eval (aLP["actorSettleTime"]) < 0.7
            return
        endif
        if eval (aLP["actorAlignmentOK"]) == 0
            let aLP["vrEnd"] := 1
            return
        endif
        rActor.SetAngle Z (aLP["actorYaw"])
        let rIdle := aLP["actorIdle"]
        PlayerRef.AuxVarSetRef "*_FC_PoolIdle" rIdle
        let aLP["actorIdleResult"] := rActor.ForcePlayIdle rIdle
        let aLP["actorStarted"] := 1
        let aLP["actorAimTime"] := 0
        let aLP["actorClockStall"] := 0
    let aLP["actorClockWait"] := 0
    endif
end
''')
# New records only: an additional top-level Sunny dialogue topic associated with
# the existing running Goodsprings dialogue quest. No vanilla record override.
def sub(t,b):return t.encode()+struct.pack('<H',len(b))+b
def z(s):return s.encode('cp1252')+b'\0'
def rec(t,id,b,flags=0):return struct.pack('<4sIIIIHH',t.encode(),len(b),flags,id,0,15,0)+b
def group(label,typ,b):return struct.pack('<4sI4siHHHH',b'GRUP',len(b)+24,label,typ,0,0,0,0)+b
def cond(fn,param):return struct.pack('<B3sfH2sIIII',0,b'\0'*3,1.,fn,b'\0'*2,param,0,0,0)
topic=0x01000800;info=0x01000801;quest=0x01000804
bank=json.loads((ROOT/'build/pool-vr/pose-bank.json').read_text())
header=sub('HEDR',struct.pack('<fII',1.34,5+len(bank),0x900+len(bank)))+sub('CNAM',z('Nikami'))+sub('MAST',z('FalloutNV.esm'))+sub('DATA',bytes(8))
tb=sub('EDID',z('FCPoolSunnyChallenge'))+sub('QSTI',struct.pack('<I',quest))+sub('FULL',z('Care for a game of pool?'))+sub('PNAM',struct.pack('<f',55))+sub('DATA',bytes([0,2]))
ib=sub('DATA',bytes([0,0,1,0]))+sub('QSTI',struct.pack('<I',quest))+sub('TPIC',struct.pack('<I',topic))+sub('PNAM',bytes(4))+sub('TRDT',struct.pack('<III B3sI B3s',0,50,0,1,bytes(3),0,1,bytes(3)))+sub('NAM1',z("Sure. Let's play."))+sub('NAM2',b'\0')+sub('NAM3',b'\0')+sub('CTDA',cond(72,0x104E84))+sub('CTDA',cond(67,0x106185))+sub('TCLF',struct.pack('<I',0xC8))+sub('SCHR',bytes.fromhex('0000000000000000000000000000000000000100'))+sub('NEXT',b'')+sub('SCHR',bytes.fromhex('0000000000000000000000000000000000000100'))
# Own travel package. Native record layout checked against GSSunny travel
# packages in the user's master. No scripts, conditions or quest edits copied.
pb=sub('EDID',z('FCPoolSunnyTravel'))+sub('PKDT',struct.pack('<IB3sI',0x00208202,6,bytes(3),0))+sub('PLDT',struct.pack('<III',0,0,0))+sub('PSDT',bytes.fromhex('ffff00ff00000000'))
for event in ('POBA','POEA','POCA'):
    pb+=sub(event,b'')+sub('INAM',bytes(4))+sub('SCHR',bytes.fromhex('0000000000000000000000000000000000000100'))+sub('TNAM',bytes(4))
idle=sub('EDID',z('FCPoolSunnyStroke'))+sub('MODL',z('Characters\\_Male\\IdleAnims\\FilthyCasualPool\\SpecialIdle_FCPool_D110_X5.kf'))+sub('ANAM',bytes(8))+sub('DATA',bytes([0x47])+bytes(7))
idle_records=rec('IDLE',0x01000803,idle)
for pose in bank:
    body=sub('EDID',z('FCPoolStroke'+str(pose['form'])))+sub('MODL',z(pose['path']))+sub('ANAM',bytes(8))+sub('DATA',bytes([0x47])+bytes(7))
    idle_records+=rec('IDLE',0x01000000+pose['form'],body)
quest_record=rec('QUST',quest,sub('EDID',z('FCPoolDialogueQuest'))+sub('FULL',z('Filthy Casual Pool'))+sub('DATA',struct.pack('<BB2sf',1,55,bytes(2),0.)))
# Quest must be registered before its topic info associations are loaded.
(OUT/'Filthy Casual Pool.esp').write_bytes(rec('TES4',0,header)+group(b'QUST',0,quest_record)+group(b'IDLE',0,idle_records)+group(b'PACK',0,rec('PACK',0x01000802,pb))+group(b'DIAL',0,rec('DIAL',topic,tb)+group(struct.pack('<I',topic),7,rec('INFO',info,ib))))

print('Staged VR scripts and Sunny dialogue:',OUT)
