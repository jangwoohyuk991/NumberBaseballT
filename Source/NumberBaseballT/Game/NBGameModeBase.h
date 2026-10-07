#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "NBGameModeBase.generated.h"

class ANBPlayerController;

UCLASS()
class NUMBERBASEBALLT_API ANBGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    virtual void OnPostLogin(AController* NewPlayer) override;

    virtual void BeginPlay() override;

    FString GenerateSecretNumber();

    bool IsGuessNumberString(
        const FString& InNumberString);

    FString JudgeResult(
        const FString& InSecretNumberString,
        const FString& InGuessNumberString);

    void PrintChatMessageString(
        ANBPlayerController* InChattingPlayerController,
        const FString& InChatMessageString);

    void IncreaseGuessCount(
        ANBPlayerController* InChattingPlayerController);

    void ResetGame();

    // 승리 또는 무승부로 리셋했다면 true를 반환한다.
    bool JudgeGame(
        ANBPlayerController* InChattingPlayerController,
        int32 InStrikeCount);

protected:
    void StartNextTurn();

    void StartTurn(
        ANBPlayerController* InPlayerController);

    void UpdateTurnTimer();

    void FinishTurn(
        int32 InStrikeCount);

protected:
    FString SecretNumberString;

    TArray<TObjectPtr<ANBPlayerController>> AllPlayerControllers;

    // 기본값을 30초로 설정한다.
    UPROPERTY(
        EditDefaultsOnly,
        Category = "Turn",
        meta = (ClampMin = "1"))
    int32 TurnTimeLimit = 30;

    FTimerHandle TurnTimerHandle;

    int32 CurrentTurnPlayerIndex = INDEX_NONE;

    float TurnEndTime = 0.f;

    bool bIsTurnActive = false;

    // 이번 턴에 유효한 숫자 야구 입력을 했는지 확인한다.
    bool bHasSubmittedThisTurn = false;
};