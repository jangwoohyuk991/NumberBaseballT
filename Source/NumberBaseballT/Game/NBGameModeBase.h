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
    virtual void OnPostLogin(AController* NewPlayer) override;

    virtual void BeginPlay() override;

    FString GenerateSecretNumber();

    bool IsGuessNumberString(const FString& InNumberString);

    FString JudgeResult(
        const FString& InSecretNumberString,
        const FString& InGuessNumberString);

    void PrintChatMessageString(
        ANBPlayerController* InChattingPlayerController,
        const FString& InChatMessageString);

    void IncreaseGuessCount(
        ANBPlayerController* InChattingPlayerController);

    void ResetGame();

    void JudgeGame(
        ANBPlayerController* InChattingPlayerController,
        int InStrikeCount);

protected:
    FString SecretNumberString;

    TArray<TObjectPtr<ANBPlayerController>> AllPlayerControllers;
};