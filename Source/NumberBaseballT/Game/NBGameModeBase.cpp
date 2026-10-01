#include "Game/NBGameModeBase.h"

#include "Game/NBGameStateBase.h"
#include "Player/NBPlayerController.h"
#include "Player/NBPlayerState.h"

void ANBGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    SecretNumberString = GenerateSecretNumber();

    // 필수 기능 가이드의 서버 정답 로그 확인용.
    UE_LOG(
        LogTemp,
        Log,
        TEXT("SecretNumber: %s"),
        *SecretNumberString);
}

void ANBGameModeBase::OnPostLogin(AController* NewPlayer)
{
    Super::OnPostLogin(NewPlayer);

    ANBPlayerController* NBPlayerController =
        Cast<ANBPlayerController>(NewPlayer);

    if (IsValid(NBPlayerController) == true)
    {
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

            ANBGameStateBase* NBGameStateBase =
                GetGameState<ANBGameStateBase>();

            if (IsValid(NBGameStateBase) == true)
            {
                NBGameStateBase->MulticastRPCBroadcastLoginMessage(
                    NBPS->PlayerNameString);
            }
        }
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

        Result.Append(FString::FromInt(Numbers[Index]));

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
            // 필수 구현사항: 허용 범위를 1~9로 한정한다.
            if (C < TEXT('1') || C > TEXT('9'))
            {
                bIsUnique = false;
                break;
            }

            UniqueDigits.Add(C);
        }

        // 필수 구현사항: 중복된 숫자는 허용하지 않는다.
        if (bIsUnique == false || UniqueDigits.Num() != 3)
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
        if (InSecretNumberString[i] == InGuessNumberString[i])
        {
            StrikeCount++;
        }
        else
        {
            FString PlayerGuessChar = FString::Printf(
                TEXT("%c"),
                InGuessNumberString[i]);

            if (InSecretNumberString.Contains(PlayerGuessChar))
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

    // 필수 구현사항: 마지막 세 글자가 아니라 입력 전체를 검사한다.
    if (IsGuessNumberString(InChatMessageString) == false)
    {
        // 강의의 처리: 유효한 숫자가 아니면 일반 채팅으로 전달한다.
        FString CombinedMessageString =
            NBPS->GetPlayerInfoString() +
            TEXT(": ") +
            InChatMessageString;

        for (const auto& Controller : AllPlayerControllers)
        {
            ANBPlayerController* PC = Controller.Get();

            if (IsValid(PC) == true)
            {
                PC->ClientRPCPrintChatMessageString(
                    CombinedMessageString);
            }
        }

        // 필수 구현사항: 잘못된 입력에는 재입력 안내를 출력한다.
        InChattingPlayerController->ClientRPCPrintChatMessageString(
            TEXT("다시 입력해주세요. 1~9의 중복되지 않는 숫자 3자리를 입력해주세요."));

        // 잘못된 입력은 시도 횟수를 증가시키지 않는다.
        return;
    }

    // 필수 구현사항: 기회를 소진한 플레이어의 추가 추측을 막는다.
    if (NBPS->CurrentGuessCount >= NBPS->MaxGuessCount)
    {
        InChattingPlayerController->ClientRPCPrintChatMessageString(
            TEXT("기회를 모두 사용했습니다. 다른 플레이어의 결과를 기다려주세요."));

        return;
    }

    FString JudgeResultString = JudgeResult(
        SecretNumberString,
        InChatMessageString);

    IncreaseGuessCount(InChattingPlayerController);

    // 서버에서 증가시킨 최신 횟수를 붙인다.
    FString CombinedMessageString =
        NBPS->GetPlayerInfoString() +
        TEXT(": ") +
        InChatMessageString +
        TEXT(" -> ") +
        JudgeResultString;

    for (const auto& Controller : AllPlayerControllers)
    {
        ANBPlayerController* PC = Controller.Get();

        if (IsValid(PC) == true)
        {
            PC->ClientRPCPrintChatMessageString(
                CombinedMessageString);
        }
    }

    int32 StrikeCount =
        FCString::Atoi(*JudgeResultString.Left(1));

    // 출력 반복문 밖에서 한 번만 판정한다.
    JudgeGame(InChattingPlayerController, StrikeCount);
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

void ANBGameModeBase::JudgeGame(
    ANBPlayerController* InChattingPlayerController,
    int32 InStrikeCount)
{
    if (InStrikeCount == 3)
    {
        ANBPlayerState* WinnerPS =
            InChattingPlayerController->GetPlayerState<ANBPlayerState>();

        if (IsValid(WinnerPS) == false)
        {
            return;
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
                    FText::FromString(CombinedMessageString);
            }
        }

        // 모든 플레이어에게 알린 뒤 한 번만 리셋한다.
        ResetGame();
        return;
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
                    FText::FromString(TEXT("Draw..."));
            }
        }

        // 모든 플레이어에게 알린 뒤 한 번만 리셋한다.
        ResetGame();
    }
}

void ANBGameModeBase::ResetGame()
{
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

    UE_LOG(
        LogTemp,
        Log,
        TEXT("SecretNumber: %s"),
        *SecretNumberString);
}