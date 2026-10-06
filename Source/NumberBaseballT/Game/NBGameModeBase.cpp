#include "Game/NBGameModeBase.h"

#include "Game/NBGameStateBase.h"
#include "Player/NBPlayerController.h"
#include "EngineUtils.h"
#include "Player/NBPlayerState.h"

void ANBGameModeBase::OnPostLogin(AController* NewPlayer)
{
    Super::OnPostLogin(NewPlayer);

    ANBPlayerController* NBPlayerController =
        Cast<ANBPlayerController>(NewPlayer);

    if (IsValid(NBPlayerController) == true)
    {
        NBPlayerController->NotificationText =
            FText::FromString(
                TEXT("Connected to the game server."));

        AllPlayerControllers.Add(NBPlayerController);

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

void ANBGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    SecretNumberString = GenerateSecretNumber();
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
            // 정답과 같은 1~9 범위만 허용한다.
            if (C < TEXT('1') || C > TEXT('9'))
            {
                bIsUnique = false;
                break;
            }

            UniqueDigits.Add(C);
        }

        // 서로 다른 숫자가 정확히 3개인지 확인한다.
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
            FString PlayerGuessChar =
                FString::Printf(
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

    
    // 마지막 세 글자가 아니라 사용자 입력 전체를 검사한다.
    FString GuessNumberString = InChatMessageString;

    if (IsGuessNumberString(GuessNumberString) == false)
    {
        // 일반 채팅 공유 처리를 유지한다.
        FString CombinedMessageString =
            NBPS->GetPlayerInfoString() +
            TEXT(": ") +
            InChatMessageString;

        for (TActorIterator<ANBPlayerController> It(GetWorld());
            It;
            ++It)
        {
            ANBPlayerController* NBPlayerController = *It;

            if (IsValid(NBPlayerController) == true)
            {
                NBPlayerController->ClientRPCPrintChatMessageString(
                    CombinedMessageString);
            }
        }

        //  잘못된 입력 안내를 표시하고 기회를 유지한다.
        InChattingPlayerController->ClientRPCPrintChatMessageString(
            TEXT("다시 입력하세요. 1~9의 중복되지 않은 3자리 숫자를 입력해주세요."));

        return;
    }

    // 기회를 모두 쓴 플레이어의 추가 추측을 막는다.
    if (NBPS->CurrentGuessCount >= NBPS->MaxGuessCount)
    {
        InChattingPlayerController->ClientRPCPrintChatMessageString(
            TEXT("3번의 기회를 모두 사용했습니다."));

        return;
    }

    FString JudgeResultString =
        JudgeResult(
            SecretNumberString,
            GuessNumberString);

    IncreaseGuessCount(InChattingPlayerController);

    //  횟수를 증가시킨 뒤 Getter를 호출한다.
    FString CombinedMessageString =
        NBPS->GetPlayerInfoString() +
        TEXT(": ") +
        InChatMessageString +
        TEXT(" -> ") +
        JudgeResultString;

    for (TActorIterator<ANBPlayerController> It(GetWorld());
        It;
        ++It)
    {
        ANBPlayerController* NBPlayerController = *It;

        if (IsValid(NBPlayerController) == true)
        {
            NBPlayerController->ClientRPCPrintChatMessageString(
                CombinedMessageString);
        }
    }

    // 모든 플레이어에게 결과를 보낸 뒤 한 번 판정한다.
    int32 StrikeCount =
        FCString::Atoi(*JudgeResultString.Left(1));

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

void ANBGameModeBase::ResetGame()
{
    SecretNumberString = GenerateSecretNumber();

    for (const auto& NBPlayerController : AllPlayerControllers)
    {
        if (IsValid(NBPlayerController) == false)
        {
            continue;
        }

        ANBPlayerState* NBPS =
            NBPlayerController->GetPlayerState<ANBPlayerState>();

        if (IsValid(NBPS) == true)
        {
            NBPS->CurrentGuessCount = 0;
        }
    }
}

void ANBGameModeBase::JudgeGame(
    ANBPlayerController* InChattingPlayerController,
    int InStrikeCount)
{
    if (3 == InStrikeCount)
    {
        ANBPlayerState* NBPS =
            InChattingPlayerController->GetPlayerState<ANBPlayerState>();

        if (IsValid(NBPS) == false)
        {
            return;
        }

        FString CombinedMessageString =
            NBPS->PlayerNameString +
            TEXT(" has won the game.");

        for (const auto& NBPlayerController : AllPlayerControllers)
        {
            if (IsValid(NBPlayerController) == true)
            {
                NBPlayerController->NotificationText =
                    FText::FromString(CombinedMessageString);
            }
        }

        // 모든 플레이어에게 공지를 설정한 뒤 한 번 리셋한다.
        ResetGame();
        return;
    }

    bool bIsDraw = true;

    for (const auto& NBPlayerController : AllPlayerControllers)
    {
        if (IsValid(NBPlayerController) == false)
        {
            continue;
        }

        ANBPlayerState* NBPS =
            NBPlayerController->GetPlayerState<ANBPlayerState>();

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
        for (const auto& NBPlayerController : AllPlayerControllers)
        {
            if (IsValid(NBPlayerController) == true)
            {
                NBPlayerController->NotificationText =
                    FText::FromString(TEXT("Draw..."));
            }
        }

        // 모든 플레이어에게 공지를 설정한 뒤 한 번 리셋한다.
        ResetGame();
    }
}