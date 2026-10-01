#include "UI/NBChatInput.h"

#include "Components/EditableTextBox.h"
#include "Player/NBPlayerController.h"

void UNBChatInput::NativeConstruct()
{
    Super::NativeConstruct();

    if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(
        this,
        &ThisClass::OnChatInputTextCommitted) == false)
    {
        EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(
            this,
            &ThisClass::OnChatInputTextCommitted);
    }
}

void UNBChatInput::NativeDestruct()
{
    Super::NativeDestruct();

    if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(
        this,
        &ThisClass::OnChatInputTextCommitted) == true)
    {
        EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(
            this,
            &ThisClass::OnChatInputTextCommitted);
    }
}

void UNBChatInput::OnChatInputTextCommitted(
    const FText& Text,
    ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter)
    {
        APlayerController* OwningPlayerController = GetOwningPlayer();

        if (IsValid(OwningPlayerController) == true)
        {
            ANBPlayerController* OwningNBPlayerController =
                Cast<ANBPlayerController>(OwningPlayerController);

            if (IsValid(OwningNBPlayerController) == true)
            {
                OwningNBPlayerController->SetChatMessageString(
                    Text.ToString());

                EditableTextBox_ChatInput->SetText(FText());
            }
        }
    }
}