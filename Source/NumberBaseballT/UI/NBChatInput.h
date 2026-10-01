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

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION()
    void OnChatInputTextCommitted(
        const FText& Text,
        ETextCommit::Type CommitMethod);
    // 블루프린트에 만든 입력창과 C++ 변수를 연결
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> EditableTextBox_ChatInput;
};
