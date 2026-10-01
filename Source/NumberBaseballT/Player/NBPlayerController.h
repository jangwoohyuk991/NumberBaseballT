#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NBPlayerController.generated.h"

class UNBChatInput;

UCLASS()
class NUMBERBASEBALLT_API ANBPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    void SetChatMessageString(const FString& InChatMessageString);
    void PrintChatMessageString(const FString& InChatMessageString);

protected:
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UNBChatInput> ChatInputWidgetClass;

    UPROPERTY()
    TObjectPtr<UNBChatInput> ChatInputWidgetInstance;

    FString ChatMessageString;
};