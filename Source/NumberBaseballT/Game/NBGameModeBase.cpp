#include "Game/NBGameModeBase.h"

#include "Game/NBGameStateBase.h"
#include "Player/NBPlayerController.h"
#include "Player/NBPlayerState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void ANBGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    SecretNumberString = GenerateSecretNumber();

    // 접속 처리가 BeginPlay보다 먼저 이루어진 경우에도 시작한다.
    if (AllPlayerControllers.Num() > 0)
    {
        StartNextTurn();
    }
}

void ANBGameModeBase::OnPostLogin(AController* NewPlayer)
{
    Super::OnPostLogin(NewPlayer);

    ANBPlayerController* NBPlayerController =
        Cast<ANBPlayerController>(NewPlayer);

    if (IsValid(NBPlayerController) == false)
    {
        return;
    }

    AllPlayerControllers.Add(NBPlayerController);

    NBPlayerController->NotificationText =
        FText::FromString(
            TEXT("Connected to the game server."));

    ANBPlayerState* NBPS =
        NBPlayerController->GetPlayerState<ANBPlayerState>();

    if (IsValid(NBPS) == true)
    {
        NBPS->PlayerNameString =
            TEXT("Player") +
            FString::FromInt(AllPlayerControllers.Num());

        ANBGameStateBase* NBGS =
            GetGameState<ANBGameStateBase>();

        if (IsValid(NBGS) == true)
        {
            NBGS->MulticastRPCBroadcastLoginMessage(
                NBPS->PlayerNameString);
        }
    }

    // 첫 플레이어가 접속하면 첫 턴을 시작한다.
    // 이미 진행 중이라면 새 플레이어는 다음 순서에 참여한다.
    if (HasActorBegunPlay() == true &&
        bIsTurnActive == false)
    {
        StartNextTurn();
    }
}

FString ANBGameModeBase::GenerateSecretNumber()
{
    TArray<int32> Numbers;

    for (int32 i = 1; i <= 9; ++i)
    {
        Numbers.Add(i);
    }

    FMath::RandInit(FDateTime::Now().GetTicks());

    Numbers = Numbers.FilterByPredicate(
        [](int32 Num)
        {
            return Num > 0;
        });

    FString Result;

    for (int32 i = 0; i < 3; ++i)
    {
        int32 Index =
            FMath::RandRange(0, Numbers.Num() - 1);

        Result.Append(
            FString::FromInt(Numbers[Index]));

        Numbers.RemoveAt(Index);
    }

    return Result;
}

bool ANBGameModeBase::IsGuessNumberString(
    const FString& InNumberString)
{
    bool bCanPlay = false;

    do
    {
        if (InNumberString.Len() != 3)
        {
            break;
        }

        bool bIsUnique = true;
        TSet<TCHAR> UniqueDigits;

        for (TCHAR C : InNumberString)
        {
            if (C < TEXT('1') || C > TEXT('9'))
            {
                bIsUnique = false;
                break;
            }

            UniqueDigits.Add(C);
        }

        if (bIsUnique == false ||
            UniqueDigits.Num() != 3)
        {
            break;
        }

        bCanPlay = true;

    } while (false);

    return bCanPlay;
}

FString ANBGameModeBase::JudgeResult(
    const FString& InSecretNumberString,
    const FString& InGuessNumberString)
{
    int32 StrikeCount = 0;
    int32 BallCount = 0;

    for (int32 i = 0; i < 3; ++i)
    {
        if (InSecretNumberString[i] ==
            InGuessNumberString[i])
        {
            StrikeCount++;
        }
        else
        {
            FString PlayerGuessChar =
                FString::Printf(
                    TEXT("%c"),
                    InGuessNumberString[i]);

            if (InSecretNumberString.Contains(
                PlayerGuessChar))
            {
                BallCount++;
            }
        }
    }

    if (StrikeCount == 0 && BallCount == 0)
    {
        return TEXT("OUT");
    }

    return FString::Printf(
        TEXT("%dS%dB"),
        StrikeCount,
        BallCount);
}

void ANBGameModeBase::PrintChatMessageString(
    ANBPlayerController* InChattingPlayerController,
    const FString& InChatMessageString)
{
    if (IsValid(InChattingPlayerController) == false)
    {
        return;
    }

    ANBPlayerState* NBPS =
        InChattingPlayerController->GetPlayerState<ANBPlayerState>();

    if (IsValid(NBPS) == false)
    {
        return;
    }

    // 이전 필수 구현 완료본의 일반 채팅 및 오류 안내를 유지한다.
    if (IsGuessNumberString(InChatMessageString) == false)
    {
        FString CombinedMessageString =
            NBPS->GetPlayerInfoString() +
            TEXT(": ") +
            InChatMessageString;

        for (
            TActorIterator<ANBPlayerController> It(GetWorld());
            It;
            ++It)
        {
            ANBPlayerController* PC = *It;

            if (IsValid(PC) == true)
            {
                PC->ClientRPCPrintChatMessageString(
                    CombinedMessageString);
            }
        }

        InChattingPlayerController
            ->ClientRPCPrintChatMessageString(
                TEXT(
                    "다시 입력하세요. 1~9의 중복되지 않은 "
                    "3자리 숫자를 입력해주세요."));

        return;
    }

    // 기존 최대 3회 제한을 유지한다.
    if (NBPS->CurrentGuessCount >= NBPS->MaxGuessCount)
    {
        InChattingPlayerController
            ->ClientRPCPrintChatMessageString(
                TEXT("3번의 기회를 모두 사용했습니다."));

        return;
    }

    if (bIsTurnActive == false ||
        AllPlayerControllers.IsValidIndex(
            CurrentTurnPlayerIndex) == false)
    {
        InChattingPlayerController
            ->ClientRPCPrintChatMessageString(
                TEXT("현재 진행 중인 턴이 없습니다."));

        return;
    }

    ANBPlayerController* CurrentTurnPlayer =
        AllPlayerControllers[CurrentTurnPlayerIndex].Get();

    // 현재 턴의 플레이어만 숫자 야구를 할 수 있다.
    if (CurrentTurnPlayer != InChattingPlayerController)
    {
        InChattingPlayerController
            ->ClientRPCPrintChatMessageString(
                TEXT("현재 본인의 턴이 아닙니다."));

        return;
    }

    // 화면 표시가 아직 갱신되지 않았더라도
    // 서버의 마감 시간을 지났다면 입력을 받지 않는다.
    if (GetWorld()->GetTimeSeconds() >= TurnEndTime)
    {
        FinishTurn(0);

        InChattingPlayerController
            ->ClientRPCPrintChatMessageString(
                TEXT("입력 시간이 종료되었습니다."));

        return;
    }

    bHasSubmittedThisTurn = true;

    FString JudgeResultString =
        JudgeResult(
            SecretNumberString,
            InChatMessageString);

    IncreaseGuessCount(InChattingPlayerController);

    FString CombinedMessageString =
        NBPS->GetPlayerInfoString() +
        TEXT(": ") +
        InChatMessageString +
        TEXT(" -> ") +
        JudgeResultString;

    for (
        TActorIterator<ANBPlayerController> It(GetWorld());
        It;
        ++It)
    {
        ANBPlayerController* PC = *It;

        if (IsValid(PC) == true)
        {
            PC->ClientRPCPrintChatMessageString(
                CombinedMessageString);
        }
    }

    int32 StrikeCount =
        FCString::Atoi(*JudgeResultString.Left(1));

    // 입력했으므로 횟수를 다시 증가시키지 않고 턴을 끝낸다.
    FinishTurn(StrikeCount);
}

void ANBGameModeBase::IncreaseGuessCount(
    ANBPlayerController* InChattingPlayerController)
{
    ANBPlayerState* NBPS =
        InChattingPlayerController->GetPlayerState<ANBPlayerState>();

    if (IsValid(NBPS) == true)
    {
        NBPS->CurrentGuessCount++;
    }
}

void ANBGameModeBase::StartNextTurn()
{
    GetWorldTimerManager().ClearTimer(TurnTimerHandle);

    bIsTurnActive = false;

    ANBGameStateBase* NBGS =
        GetGameState<ANBGameStateBase>();

    if (IsValid(NBGS) == false)
    {
        return;
    }

    NBGS->RemainingTurnTime = 0;
    NBGS->CurrentTurnPlayerName = TEXT("");

    int32 PlayerCount = AllPlayerControllers.Num();

    if (PlayerCount == 0)
    {
        NBGS->ForceNetUpdate();
        return;
    }

    // 다음 순서부터 찾고, 기회가 남은 플레이어에게 턴을 준다.
    for (int32 Offset = 1; Offset <= PlayerCount; ++Offset)
    {
        int32 NextIndex =
            (CurrentTurnPlayerIndex + Offset) % PlayerCount;

        ANBPlayerController* PC =
            AllPlayerControllers[NextIndex].Get();

        if (IsValid(PC) == false)
        {
            continue;
        }

        ANBPlayerState* NBPS =
            PC->GetPlayerState<ANBPlayerState>();

        if (IsValid(NBPS) == false)
        {
            continue;
        }

        if (NBPS->CurrentGuessCount >= NBPS->MaxGuessCount)
        {
            continue;
        }

        CurrentTurnPlayerIndex = NextIndex;

        StartTurn(PC);
        return;
    }

    NBGS->ForceNetUpdate();
}

void ANBGameModeBase::StartTurn(
    ANBPlayerController* InPlayerController)
{
    ANBPlayerState* NBPS =
        InPlayerController->GetPlayerState<ANBPlayerState>();

    ANBGameStateBase* NBGS =
        GetGameState<ANBGameStateBase>();

    if (IsValid(NBPS) == false ||
        IsValid(NBGS) == false)
    {
        return;
    }

    int32 Duration = FMath::Max(1, TurnTimeLimit);

    bIsTurnActive = true;
    bHasSubmittedThisTurn = false;

    TurnEndTime =
        GetWorld()->GetTimeSeconds() +
        static_cast<float>(Duration);

    NBGS->RemainingTurnTime = Duration;
    NBGS->CurrentTurnPlayerName = NBPS->PlayerNameString;
    NBGS->ForceNetUpdate();

    // 서버에서 시간을 계산한다.
    GetWorldTimerManager().SetTimer(
        TurnTimerHandle,
        this,
        &ThisClass::UpdateTurnTimer,
        0.25f,
        true);
}

void ANBGameModeBase::UpdateTurnTimer()
{
    if (bIsTurnActive == false)
    {
        return;
    }

    ANBGameStateBase* NBGS =
        GetGameState<ANBGameStateBase>();

    if (IsValid(NBGS) == false)
    {
        return;
    }

    float SecondsLeft =
        TurnEndTime -
        GetWorld()->GetTimeSeconds();

    int32 NewRemainingTime =
        FMath::Max(
            0,
            FMath::CeilToInt(SecondsLeft));

    if (NBGS->RemainingTurnTime != NewRemainingTime)
    {
        NBGS->RemainingTurnTime = NewRemainingTime;
        NBGS->ForceNetUpdate();
    }

    if (SecondsLeft <= 0.f)
    {
        FinishTurn(0);
    }
}

void ANBGameModeBase::FinishTurn(
    int32 InStrikeCount)
{
    if (bIsTurnActive == false)
    {
        return;
    }

    bIsTurnActive = false;

    GetWorldTimerManager().ClearTimer(TurnTimerHandle);

    ANBGameStateBase* NBGS =
        GetGameState<ANBGameStateBase>();

    if (IsValid(NBGS) == true)
    {
        NBGS->RemainingTurnTime = 0;
        NBGS->ForceNetUpdate();
    }

    if (AllPlayerControllers.IsValidIndex(
        CurrentTurnPlayerIndex) == false)
    {
        StartNextTurn();
        return;
    }

    ANBPlayerController* PC =
        AllPlayerControllers[CurrentTurnPlayerIndex].Get();

    if (IsValid(PC) == false)
    {
        StartNextTurn();
        return;
    }

    ANBPlayerState* NBPS =
        PC->GetPlayerState<ANBPlayerState>();

    if (IsValid(NBPS) == false)
    {
        StartNextTurn();
        return;
    }

    // 유효한 숫자를 입력하지 않은 채 끝난 턴은 기회 1회 소모.
    if (bHasSubmittedThisTurn == false)
    {
        IncreaseGuessCount(PC);

        FString TimeoutMessage =
            NBPS->GetPlayerInfoString() +
            TEXT(": 시간 초과 -> 기회 1회 소모");

        for (
            TActorIterator<ANBPlayerController> It(GetWorld());
            It;
            ++It)
        {
            ANBPlayerController* OtherPC = *It;

            if (IsValid(OtherPC) == true)
            {
                OtherPC->ClientRPCPrintChatMessageString(
                    TimeoutMessage);
            }
        }
    }

    // 승리 또는 무승부면 ResetGame에서 새 턴을 시작한다.
    if (JudgeGame(PC, InStrikeCount) == true)
    {
        return;
    }

    StartNextTurn();
}

bool ANBGameModeBase::JudgeGame(
    ANBPlayerController* InChattingPlayerController,
    int32 InStrikeCount)
{
    if (InStrikeCount == 3)
    {
        ANBPlayerState* WinnerPS =
            InChattingPlayerController
            ->GetPlayerState<ANBPlayerState>();

        if (IsValid(WinnerPS) == false)
        {
            return false;
        }

        FString CombinedMessageString =
            WinnerPS->PlayerNameString +
            TEXT(" has won the game.");

        for (const auto& Controller : AllPlayerControllers)
        {
            ANBPlayerController* PC = Controller.Get();

            if (IsValid(PC) == true)
            {
                PC->NotificationText =
                    FText::FromString(
                        CombinedMessageString);
            }
        }

        ResetGame();
        return true;
    }

    bool bIsDraw = true;

    for (const auto& Controller : AllPlayerControllers)
    {
        ANBPlayerController* PC = Controller.Get();

        if (IsValid(PC) == false)
        {
            continue;
        }

        ANBPlayerState* NBPS =
            PC->GetPlayerState<ANBPlayerState>();

        if (IsValid(NBPS) == false)
        {
            bIsDraw = false;
            break;
        }

        if (NBPS->CurrentGuessCount < NBPS->MaxGuessCount)
        {
            bIsDraw = false;
            break;
        }
    }

    if (bIsDraw == true)
    {
        for (const auto& Controller : AllPlayerControllers)
        {
            ANBPlayerController* PC = Controller.Get();

            if (IsValid(PC) == true)
            {
                PC->NotificationText =
                    FText::FromString(
                        TEXT("Draw..."));
            }
        }

        ResetGame();
        return true;
    }

    return false;
}

void ANBGameModeBase::ResetGame()
{
    GetWorldTimerManager().ClearTimer(TurnTimerHandle);

    bIsTurnActive = false;
    bHasSubmittedThisTurn = false;

    SecretNumberString = GenerateSecretNumber();

    for (const auto& Controller : AllPlayerControllers)
    {
        ANBPlayerController* PC = Controller.Get();

        if (IsValid(PC) == false)
        {
            continue;
        }

        ANBPlayerState* NBPS =
            PC->GetPlayerState<ANBPlayerState>();

        if (IsValid(NBPS) == true)
        {
            NBPS->CurrentGuessCount = 0;
        }
    }

    CurrentTurnPlayerIndex = INDEX_NONE;

    StartNextTurn();
}