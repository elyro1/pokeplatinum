#include "macros/scrcmd.inc"
#include "res/text/bank/iron_island.h"
#include "res/field/events/events_iron_island.h"


    ScriptEntry IronIsland_OnTransition
    ScriptEntry IronIsland_Sailor
    ScriptEntry IronIsland_Riley
    ScriptEntry IronIsland_Iris
    ScriptEntryEnd

IronIsland_OnTransition:
    SetFlag FLAG_FIRST_ARRIVAL_IRON_ISLAND_EXTERIOR
    End

IronIsland_Sailor:
    PlaySE SE_CONFIRM_sseq_3
    LockAll
    FacePlayer
    GetPlayerDir VAR_0x8004
    FacePlayer
    Message IronIsland_Text_WantToSailSomewhere
    ShowYesNoMenu VAR_RESULT
    CloseMessage
    GoToIfEq VAR_RESULT, MENU_YES, IronIsland_TakeShipToCanalave
    GoTo IronIsland_TellWheneverSetSail

IronIsland_TakeShipToCanalave:
    Message IronIsland_Text_AnchorsAweigh
    CloseMessage
    Call IronIsland_SailorEnterShip
    CallIfEq VAR_0x8004, DIR_SOUTH, IronIsland_PlayerEnterShipSouth
    CallIfEq VAR_0x8004, DIR_WEST, IronIsland_PlayerEnterShipWest
    TakeShipToCanalave
    ReleaseAll
    End

IronIsland_TellWheneverSetSail:
    Message IronIsland_Text_TellWheneverSetSail
    WaitButton
    CloseMessage
    ReleaseAll
    End

IronIsland_SailorEnterShip:
    ApplyMovement LOCALID_SAILOR, IronIsland_Movement_SailorFaceWest
    WaitMovement
    PlaySE SEQ_SE_DP_KAIDAN2_sseq
    ApplyMovement LOCALID_SAILOR, IronIsland_Movement_SetInvisible
    WaitMovement
    Return

IronIsland_PlayerEnterShipSouth:
    ApplyMovement LOCALID_PLAYER, IronIsland_Movement_PlayerWalkToShipSouth
    WaitMovement
    PlaySE SEQ_SE_DP_KAIDAN2_sseq
    ApplyMovement LOCALID_PLAYER, IronIsland_Movement_SetInvisible
    WaitMovement
    Return

IronIsland_PlayerEnterShipWest:
    ApplyMovement LOCALID_PLAYER, IronIsland_Movement_PlayerWalkToShipWest
    WaitMovement
    PlaySE SEQ_SE_DP_KAIDAN2_sseq
    ApplyMovement LOCALID_PLAYER, IronIsland_Movement_SetInvisible
    WaitMovement
    Return

    .balign 4, 0
IronIsland_Movement_SailorFaceWest:
    FaceWest
    Delay15
    EndMovement

    .balign 4, 0
IronIsland_Movement_SetInvisible:
    SetInvisible
    EndMovement

    .balign 4, 0
IronIsland_Movement_PlayerWalkToShipSouth:
    WalkNormalSouth
    FaceWest
    Delay15
    EndMovement

    .balign 4, 0
IronIsland_Movement_PlayerWalkToShipWest:
    WalkNormalWest
    Delay15
    EndMovement

IronIsland_Riley:
    PlaySE SE_CONFIRM_sseq_3
    LockAll
    FacePlayer
    BufferPlayerName 0
    Message IronIsland_Text_ImRileyHeresAGift
    SetVar VAR_0x8004, ITEM_HM04
    SetVar VAR_0x8005, 1
    Common_GiveItemQuantity
    CheckBadgeAcquired BADGE_ID_MINE, VAR_RESULT
    GoToIfEq VAR_RESULT, FALSE, IronIsland_YouNeedMineBadge
    GoTo IronIsland_YouCanJoinMeInside
    End

IronIsland_YouCanJoinMeInside:
    Message IronIsland_Text_YouCanJoinMeInside
    GoTo IronIsland_RileyEnterIronIsland
    End

IronIsland_YouNeedMineBadge:
    Message IronIsland_Text_YouNeedMineBadge
    GoTo IronIsland_RileyEnterIronIsland
    End

IronIsland_RileyEnterIronIsland:
    CloseMessage
    ApplyMovement LOCALID_RILEY, IronIsland_Movement_RileyWalkOnSpotNorth
    WaitMovement
    PlaySE SEQ_SE_DP_KAIDAN2_sseq
    RemoveObject LOCALID_RILEY
    WaitSE SEQ_SE_DP_KAIDAN2_sseq
    ReleaseAll
    End

    .balign 4, 0
IronIsland_Movement_RileyWalkOnSpotNorth:
    WalkOnSpotNormalNorth
    EndMovement

IronIsland_Iris:
    PlaySE SE_CONFIRM_sseq_3
    LockAll
    FacePlayer
    CheckIrisAssets VAR_RESULT
    GoToIfEq VAR_RESULT, FALSE, IronIsland_Visitor
    GoToIfUnset FLAG_GAME_COMPLETED, IronIsland_IrisNotReady
    Message IronIsland_Text_IrisChallenge
    ShowYesNoMenu VAR_RESULT
    CloseMessage
    GoToIfEq VAR_RESULT, MENU_NO, IronIsland_IrisEnd
    StartTrainerBattle TRAINER_CHAMPION_IRIS
    CheckWonBattle VAR_RESULT
    GoToIfEq VAR_RESULT, FALSE, IronIsland_IrisLost
    Message IronIsland_Text_IrisVictory
    WaitButton
    CheckItem ITEM_AZURE_FLUTE, 1, VAR_RESULT
    GoToIfEq VAR_RESULT, TRUE, IronIsland_IrisRepeatReward
    Message IronIsland_Text_IrisAzureFlute
    SetVar VAR_0x8004, ITEM_AZURE_FLUTE
    SetVar VAR_0x8005, 1
    GoTo IronIsland_IrisGiveReward

IronIsland_IrisRepeatReward:
    Message IronIsland_Text_IrisRepeatReward
    SetVar VAR_0x8005, 50
    GetRandom VAR_RESULT, 7
    SetVar VAR_0x8004, ITEM_RARE_CANDY
    GoToIfEq VAR_RESULT, 0, IronIsland_IrisGiveReward
    SetVar VAR_0x8004, ITEM_HP_UP
    GoToIfEq VAR_RESULT, 1, IronIsland_IrisGiveReward
    SetVar VAR_0x8004, ITEM_PROTEIN
    GoToIfEq VAR_RESULT, 2, IronIsland_IrisGiveReward
    SetVar VAR_0x8004, ITEM_IRON
    GoToIfEq VAR_RESULT, 3, IronIsland_IrisGiveReward
    SetVar VAR_0x8004, ITEM_CALCIUM
    GoToIfEq VAR_RESULT, 4, IronIsland_IrisGiveReward
    SetVar VAR_0x8004, ITEM_ZINC
    GoToIfEq VAR_RESULT, 5, IronIsland_IrisGiveReward
    SetVar VAR_0x8004, ITEM_CARBOS

IronIsland_IrisGiveReward:
    CanFitItem VAR_0x8004, VAR_0x8005, VAR_RESULT
    GoToIfEq VAR_RESULT, FALSE, IronIsland_IrisBagFull
    Common_GiveItemQuantity
    CloseMessage
    GoTo IronIsland_IrisEnd

IronIsland_IrisBagFull:
    Common_MessageBagIsFull
    CloseMessage
IronIsland_IrisEnd:
    ReleaseAll
    End

IronIsland_IrisNotReady:
    Message IronIsland_Text_IrisNotReady
    WaitButton
    CloseMessage
    ReleaseAll
    End

IronIsland_IrisLost:
    BlackOutFromBattle
    ReleaseAll
    End

IronIsland_Visitor:
    Message IronIsland_Text_Visitor
    WaitButton
    CloseMessage
    ReleaseAll
    End
