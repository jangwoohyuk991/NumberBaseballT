#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NBGameModeBase.generated.h"

class ANBPlayerController;

UCLASS()
class NUMBERBASEBALLT_API ANBGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    virtual void OnPostLogin(AController* NewPlayer) override;

    void PrintChatMessageString(
        ANBPlayerController* InChattingPlayerController,
        const FString& InChatMessageString);

    FString GenerateSecretNumber();

    bool IsGuessNumberString(const FString& InNumberString);

    FString JudgeResult(
        const FString& InSecretNumberString,
        const FString& InGuessNumberString);

    void IncreaseGuessCount(
        ANBPlayerController* InChattingPlayerController);

    void JudgeGame(
        ANBPlayerController* InChattingPlayerController,
        int32 InStrikeCount);

    void ResetGame();

protected:
    FString SecretNumberString;

    TArray<TObjectPtr<ANBPlayerController>> AllPlayerControllers;
};