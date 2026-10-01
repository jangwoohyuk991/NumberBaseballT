#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "NBChatInput.generated.h"

class UEditableTextBox;

UCLASS()
class NUMBERBASEBALLT_API UNBChatInput : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    virtual void NativeDestruct() override;

protected:
    UFUNCTION()
    void OnChatInputTextCommitted(
        const FText& Text,
        ETextCommit::Type CommitMethod);

public:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> EditableTextBox_ChatInput;
};